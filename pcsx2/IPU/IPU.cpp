/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

#include "Common.h"

#include "IPU.h"
#include "yuv2rgb.h"

#include "Vif.h"
#include "Tags.h"

using namespace std;			// for min / max

// Zero cycle IRQ schedules aren't really good, but the IPU uses them.
// Better to throw the IRQ inline:

#define IPU_INT0_FROM()  ipu0Interrupt()
//#define IPU_INT0_FROM()  CPU_INT( DMAC_FROM_IPU, 0 )

// IPU Inline'd IRQs : Calls the IPU interrupt handlers directly instead of
// feeding them through the EE's branch test. (see IPU.h for details)

#ifdef IPU_INLINE_IRQS
#	define IPU_INT_TO( cycles )  ipu1Interrupt()
#	define IPU_INT_FROM( cycles )  ipu0Interrupt()
#	define IPU_FORCEINLINE
#else
#	define IPU_INT_TO( cycles )  CPU_INT( DMAC_TO_IPU, cycles )
#	define IPU_INT_FROM( cycles )  CPU_INT( DMAC_FROM_IPU, cycles )
#	define IPU_FORCEINLINE __forceinline
#endif

static tIPU_DMA g_nDMATransfer;

// FIXME - g_nIPU0Data and Pointer are not saved in the savestate, which breaks savestates for some
// FMVs at random (if they get saved during the half frame of a 30fps rate).  The fix is complicated
// since coroutine is such a pita.  (air)

int g_nIPU0Data = 0; // data left to transfer
u8* g_pIPU0Pointer = NULL;
int g_nCmdPos[2] = {0}, g_nCmdIndex = 0;
int ipuCurCmd = 0xffffffff;

int FOreadpos = 0, FOwritepos = 0;
static int FIreadpos = 0, FIwritepos = 0;
PCSX2_ALIGNED16(u32 fifo_input[32]);
PCSX2_ALIGNED16(u32 fifo_output[32]);

void ReorderBitstream();

// the BP doesn't advance and returns -1 if there is no data to be read
tIPU_BP g_BP;
static coroutine_t s_routine; // used for executing BDEC/IDEC
static int s_RoutineDone = 0;
static u32 s_tempstack[0x4000]; // 64k

void IPUCMD_WRITE(u32 val);
void IPUWorker();
int IPU0dma();
int IPU1dma();

// Color conversion stuff, the memory layout is a total hack
// convert_data_buffer is a pointer to the internal rgb struct (the first param in convert_init_t)
//char convert_data_buffer[sizeof(convert_rgb_t)];
char convert_data_buffer[0x1C];

// Quantization matrix
static u8 niq[64];			//non-intraquant matrix
static u8 iq[64];			//intraquant matrix
u16 vqclut[16];			//clut conversion table
static u8 s_thresh[2];		//thresholds for color conversions
int coded_block_pattern = 0;
PCSX2_ALIGNED16(macroblock_8  mb8);
PCSX2_ALIGNED16(macroblock_16 mb16);
PCSX2_ALIGNED16(macroblock_rgb32 rgb32);
PCSX2_ALIGNED16(macroblock_rgb16 rgb16);

u8 indx4[16*16/2];
bool	mpeg2_inited = FALSE;		//mpeg2_idct_init() must be called only once
u8 PCT[] = {'r', 'I', 'P', 'B', 'D', '-', '-', '-'};
decoder_t g_decoder;						//static, only to place it in bss
decoder_t tempdec;

extern "C"
{
	extern u8 mpeg2_scan_norm[64];
	extern u8 mpeg2_scan_alt[64];
}

PCSX2_ALIGNED16(u8 _readbits[80]);	//local buffer (ring buffer)
u8* readbits = _readbits; // always can decrement by one 1qw

//#define SATURATE_4BITS(val) ((val)>15 ? 15 : (val))

__forceinline void IPUProcessInterrupt()
{
	if (ipuRegs->ctrl.BUSY) IPUWorker();
}

__forceinline void init_g_decoder()
{
	//other stuff
	g_decoder.intra_quantizer_matrix = (u8*)iq;
	g_decoder.non_intra_quantizer_matrix = (u8*)niq;
	g_decoder.picture_structure = FRAME_PICTURE;	//default: progressive...my guess:P
	g_decoder.mb8 = &mb8;
	g_decoder.mb16 = &mb16;
	g_decoder.rgb32 = &rgb32;
	g_decoder.rgb16 = &rgb16;
	g_decoder.stride = 16;
}

__forceinline void mpeg2_init()
{
	if (!mpeg2_inited)
	{
		mpeg2_idct_init();
		yuv2rgb_init();
		memzero(mb8.Y);
		memzero(mb8.Cb);
		memzero(mb8.Cr);
		memzero(mb16.Y);
		memzero(mb16.Cb);
		memzero(mb16.Cr);
		mpeg2_inited = TRUE;
	}
}

/////////////////////////////////////////////////////////
// Register accesses (run on EE thread)
int ipuInit()
{
	memzero(*ipuRegs);
	memzero(g_BP);
	init_g_decoder();
	g_nDMATransfer._u32 = 0;

	return 0;
}

void ipuReset()
{
	memzero(*ipuRegs);
	g_nDMATransfer._u32 = 0;
}

void ipuShutdown()
{
}

void ReportIPU()
{
	Console::WriteLn("g_nDMATransfer = 0x%x.", g_nDMATransfer._u32);
	Console::WriteLn("FIreadpos = 0x%x, FIwritepos = 0x%x.", FIreadpos, FIwritepos);
	Console::WriteLn("fifo_input = 0x%x.", fifo_input);
	Console::WriteLn("FOreadpos = 0x%x, FOwritepos = 0x%x.", FOreadpos, FOwritepos);
	Console::WriteLn("fifo_output = 0x%x.", fifo_output);
	Console::WriteLn("g_BP = 0x%x.", g_BP);
	Console::WriteLn("niq = 0x%x, iq = 0x%x.", niq, iq);
	Console::WriteLn("vqclut = 0x%x.", vqclut);
	Console::WriteLn("s_thresh = 0x%x.", s_thresh);
	Console::WriteLn("coded_block_pattern = 0x%x.", coded_block_pattern);
	Console::WriteLn("g_decoder = 0x%x.", g_decoder);
	Console::WriteLn("mpeg2_scan_norm = 0x%x, mpeg2_scan_alt = 0x%x.", mpeg2_scan_norm, mpeg2_scan_alt);
	Console::WriteLn("g_nCmdPos = 0x%x.", g_nCmdPos);
	Console::WriteLn("g_nCmdIndex = 0x%x.", g_nCmdIndex);
	Console::WriteLn("ipuCurCmd = 0x%x.", ipuCurCmd);
	Console::WriteLn("_readbits = 0x%x.", _readbits);
	Console::WriteLn("temp will equal 0x%x.", readbits - _readbits);
	Console::WriteLn("");
}
// fixme - ipuFreeze looks fairly broken. Should probably take a closer look at some point.

void SaveStateBase::ipuFreeze()
{
	IPUProcessInterrupt();

	FreezeTag("IPU");

	// old versions saved the IPU regs, but they're already saved as part of HW!
	//FreezeMem(ipuRegs, sizeof(IPUregisters));

	Freeze(g_nDMATransfer._u32);
	Freeze(FIreadpos);
	Freeze(FIwritepos);
	Freeze(fifo_input);
	Freeze(FOreadpos);
	Freeze(FOwritepos);
	Freeze(fifo_output);
	Freeze(g_BP);
	Freeze(niq);
	Freeze(iq);
	Freeze(vqclut);
	Freeze(s_thresh);
	Freeze(coded_block_pattern);
	Freeze(g_decoder);
	Freeze(mpeg2_scan_norm);
	Freeze(mpeg2_scan_alt);
	Freeze(g_nCmdPos);
	Freeze(g_nCmdIndex);
	Freeze(ipuCurCmd);

	Freeze(_readbits);

	int temp = readbits - _readbits;
	Freeze(temp);

	if (IsLoading())
	{
		readbits = _readbits;
		init_g_decoder();
		mpeg2_init();
	}
}

bool ipuCanFreeze()
{
	return (ipuCurCmd == 0xffffffff);
}

__forceinline u32 ipuRead32(u32 mem)
{
	// Note: It's assumed that mem's input value is always in the 0x10002000 page
	// of memory (if not, it's probably bad code).

	jASSUME((mem & ~0xff) == 0x10002000);
	mem &= 0xff;	// ipu repeats every 0x100

	IPUProcessInterrupt();

	switch (mem)
	{
		ipucase(IPU_CTRL): // IPU_CTRL
			ipuRegs->ctrl.IFC = g_BP.IFC;
			ipuRegs->ctrl.CBP = coded_block_pattern;

			if (!ipuRegs->ctrl.BUSY)
				IPU_LOG("Ipu read32: IPU_CTRL=0x%08X %x", ipuRegs->ctrl._u32, cpuRegs.pc);

			return ipuRegs->ctrl._u32;

		ipucase(IPU_BP): // IPU_BP

			ipuRegs->ipubp = g_BP.BP & 0x7f;
			ipuRegs->ipubp |= g_BP.IFC << 8;
			ipuRegs->ipubp |= (g_BP.FP + g_BP.bufferhasnew) << 16;

			IPU_LOG("Ipu read32: IPU_BP=0x%08X", *(u32*)&g_BP);
			return ipuRegs->ipubp;
	}

	return *(u32*)(((u8*)ipuRegs) + mem);
}

__forceinline u64 ipuRead64(u32 mem)
{
	// Note: It's assumed that mem's input value is always in the 0x10002000 page
	// of memory (if not, it's probably bad code).

	jASSUME((mem & ~0xff) == 0x10002000);
	mem &= 0xff;	// ipu repeats every 0x100

	IPUProcessInterrupt();

	switch (mem)
	{
		ipucase(IPU_CMD): // IPU_CMD
			if (ipuRegs->cmd.DATA & 0xffffff)
				IPU_LOG("Ipu read64: IPU_CMD=BUSY=%x, DATA=%08X", ipuRegs->cmd.BUSY ? 1 : 0, ipuRegs->cmd.DATA);
			break;

		ipucase(IPU_CTRL):
			DevCon::Notice("reading 64bit IPU ctrl");
			break;

		ipucase(IPU_BP):
			DevCon::Notice("reading 64bit IPU top");
			break;

		ipucase(IPU_TOP): // IPU_TOP
			IPU_LOG("Ipu read64: IPU_TOP=%x,  bp = %d", ipuRegs->top, g_BP.BP);
			break;

		default:
			IPU_LOG("Ipu read64: Unknown=%x", mem);
			break;
	}
	return *(u64*)(((u8*)ipuRegs) + mem);
}

void ipuSoftReset()
{
	mpeg2_init();
	FIFOto_clear();
	FIFOfrom_clear();

	coded_block_pattern = 0;

	ipuRegs->ctrl._u32 = 0;
	ipuRegs->top = 0;
	ipuCurCmd = 0xffffffff;

	g_BP.BP = 0;
	g_BP.FP = 0;
	g_BP.bufferhasnew = 0;

	g_nCmdIndex = 0;
	g_nCmdPos[0] = 0;
	g_nCmdPos[1] = 0;
}

__forceinline void ipuWrite32(u32 mem, u32 value)
{
	// Note: It's assumed that mem's input value is always in the 0x10002000 page
	// of memory (if not, it's probably bad code).

	jASSUME((mem & ~0xfff) == 0x10002000);
	mem &= 0xfff;

	IPUProcessInterrupt();

	switch (mem)
	{
		ipucase(IPU_CMD): // IPU_CMD
			IPU_LOG("Ipu write32: IPU_CMD=0x%08X", value);
			IPUCMD_WRITE(value);
			break;

		ipucase(IPU_CTRL): // IPU_CTRL
            // CTRL = the first 16 bits of ctrl [0x8000ffff], + value for the next 16 bits,
            // minus the reserved bits. (18-19; 27-29) [0x47f30000]
			ipuRegs->ctrl._u32 = (value & 0x47f30000) | (ipuRegs->ctrl._u32 & 0x8000ffff);
			if (ipuRegs->ctrl.IDP == 3)
			{
				Console::WriteLn("IPU Invalid Intra DC Precision, switching to 9 bits");
				ipuRegs->ctrl.IDP = 1;
			}

			if (ipuRegs->ctrl.RST & 0x1) ipuSoftReset(); // RESET

			IPU_LOG("Ipu write32: IPU_CTRL=0x%08X", value);
			break;

		default:
			IPU_LOG("Ipu write32: Unknown=%x", mem);
			*(u32*)((u8*)ipuRegs + mem) = value;
			break;
	}
}

__forceinline void ipuWrite64(u32 mem, u64 value)
{
	// Note: It's assumed that mem's input value is always in the 0x10002000 page
	// of memory (if not, it's probably bad code).

	jASSUME((mem & ~0xfff) == 0x10002000);
	mem &= 0xfff;

	IPUProcessInterrupt();

	switch (mem)
	{
		ipucase(IPU_CMD):
			IPU_LOG("Ipu write64: IPU_CMD=0x%08X", value);
			IPUCMD_WRITE((u32)value);
			break;

		default:
			IPU_LOG("Ipu write64: Unknown=%x", mem);
			*(u64*)((u8*)ipuRegs + mem) = value;
			break;
	}
}


//////////////////////////////////////////////////////
// IPU Commands (exec on worker thread only)

static void ipuBCLR(u32 val)
{
	FIFOto_clear();

	g_BP.BP = val & 0x7F;
	g_BP.FP = 0;
	g_BP.bufferhasnew = 0;
	ipuRegs->ctrl.BUSY = 0;
	ipuRegs->cmd.BUSY = 0;
	memzero_ptr<80>(readbits);
	IPU_LOG("Clear IPU input FIFO. Set Bit offset=0x%X", g_BP.BP);
}

static __forceinline BOOL ipuIDEC(u32 val)
{
	tIPU_CMD_IDEC idec(val);

	IPU_LOG("IPU IDEC command.");

	if (idec.FB) IPU_LOG(" Skip %d	bits.", idec.FB);
	IPU_LOG(" Quantizer step code=0x%X.", idec.QSC);

	if (idec.DTD == 0)
		IPU_LOG(" Does not decode DT.");
	else
		IPU_LOG(" Decodes DT.");

	if (idec.SGN == 0)
		IPU_LOG(" No bias.");
	else
		IPU_LOG(" Bias=128.");

	if (idec.DTE == 1) IPU_LOG(" Dither Enabled.");
	if (idec.OFM == 0)
		IPU_LOG(" Output format is RGB32.");
	else
		IPU_LOG(" Output format is RGB16.");

	IPU_LOG("");

	g_BP.BP += idec.FB;//skip FB bits
	//from IPU_CTRL
	ipuRegs->ctrl.PCT = I_TYPE; //Intra DECoding;)
	g_decoder.coding_type = ipuRegs->ctrl.PCT;
	g_decoder.mpeg1 = ipuRegs->ctrl.MP1;
	g_decoder.q_scale_type	= ipuRegs->ctrl.QST;
	g_decoder.intra_vlc_format = ipuRegs->ctrl.IVF;
	g_decoder.scan = ipuRegs->ctrl.AS ? mpeg2_scan_alt : mpeg2_scan_norm;
	g_decoder.intra_dc_precision = ipuRegs->ctrl.IDP;
	//from IDEC value
	g_decoder.quantizer_scale = idec.QSC;
	g_decoder.frame_pred_frame_dct = !idec.DTD;
	g_decoder.sgn = idec.SGN;
	g_decoder.dte = idec.DTE;
	g_decoder.ofm = idec.OFM;
	//other stuff
	g_decoder.dcr = 1;//resets DC prediction value

	s_routine = so_create(mpeg2sliceIDEC, &s_RoutineDone, s_tempstack, sizeof(s_tempstack));
	assert(s_routine != NULL);
	so_call(s_routine);
	if (s_RoutineDone) s_routine = NULL;

	return s_RoutineDone;
}

static int s_bdec = 0;

static __forceinline BOOL ipuBDEC(u32 val)
{
	tIPU_CMD_BDEC bdec(val);

	IPU_LOG("IPU BDEC(macroblock decode) command %x, num: 0x%x", cpuRegs.pc, s_bdec);
	if (bdec.FB) IPU_LOG(" Skip 0x%X bits.", bdec.FB);

	if (bdec.MBI)
		IPU_LOG(" Intra MB.");
	else
		IPU_LOG(" Non-intra MB.");

	if (bdec.DCR)
		IPU_LOG(" Resets DC prediction value.");
	else
		IPU_LOG(" Doesn't reset DC prediction value.");

	if (bdec.DT)
		IPU_LOG(" Use field DCT.");
	else
		IPU_LOG(" Use frame DCT.");

	IPU_LOG(" Quantizer step=0x%X", bdec.QSC);

	if( IsDebugBuild )
		s_bdec++;

	g_BP.BP += bdec.FB;//skip FB bits
	g_decoder.coding_type = I_TYPE;
	g_decoder.mpeg1 = ipuRegs->ctrl.MP1;
	g_decoder.q_scale_type	= ipuRegs->ctrl.QST;
	g_decoder.intra_vlc_format = ipuRegs->ctrl.IVF;
	g_decoder.scan = ipuRegs->ctrl.AS ? mpeg2_scan_alt : mpeg2_scan_norm;
	g_decoder.intra_dc_precision = ipuRegs->ctrl.IDP;
	//from BDEC value
	/* JayteeMaster: the quantizer (linear/non linear) depends on the q_scale_type */
	g_decoder.quantizer_scale = g_decoder.q_scale_type ? non_linear_quantizer_scale [bdec.QSC] : bdec.QSC << 1;
	g_decoder.macroblock_modes = bdec.DT ? DCT_TYPE_INTERLACED : 0;
	g_decoder.dcr = bdec.DCR;
	g_decoder.macroblock_modes |= bdec.MBI ? MACROBLOCK_INTRA : MACROBLOCK_PATTERN;

	memzero(mb8);
	memzero(mb16);

	s_routine = so_create(mpeg2_slice, &s_RoutineDone, s_tempstack, sizeof(s_tempstack));
	assert(s_routine != NULL);
	so_call(s_routine);

	if (s_RoutineDone) s_routine = NULL;
	return s_RoutineDone;
}

static BOOL __fastcall ipuVDEC(u32 val)
{
	switch (g_nCmdPos[0])
	{
		case 0:
			ipuRegs->cmd.DATA = 0;
			if (!getBits32((u8*)&g_decoder.bitstream_buf, 0)) return FALSE;

			g_decoder.bitstream_bits = -16;
			BigEndian(g_decoder.bitstream_buf, g_decoder.bitstream_buf);

			switch ((val >> 26) & 3)
			{
				case 0://Macroblock Address Increment
					g_decoder.mpeg1 = ipuRegs->ctrl.MP1;
					ipuRegs->cmd.DATA = get_macroblock_address_increment(&g_decoder);
					break;

				case 1://Macroblock Type	//known issues: no error detected
					g_decoder.frame_pred_frame_dct = 1;//prevent DCT_TYPE_INTERLACED
					g_decoder.coding_type = ipuRegs->ctrl.PCT;
					ipuRegs->cmd.DATA = get_macroblock_modes(&g_decoder);
					break;

				case 2://Motion Code		//known issues: no error detected
					ipuRegs->cmd.DATA = get_motion_delta(&g_decoder, 0);
					break;

				case 3://DMVector
					ipuRegs->cmd.DATA = get_dmv(&g_decoder);
					break;
			}

			g_BP.BP += (g_decoder.bitstream_bits + 16);
			if ((int)g_BP.BP < 0)
			{
				g_BP.BP += 128;
				ReorderBitstream();
			}

			FillInternalBuffer(&g_BP.BP, 1, 0);

			ipuRegs->cmd.DATA = (ipuRegs->cmd.DATA & 0xFFFF) | ((g_decoder.bitstream_bits + 16) << 16);
			ipuRegs->ctrl.ECD = (ipuRegs->cmd.DATA == 0);

		case 1:
			if (!getBits32((u8*)&ipuRegs->top, 0))
			{
				g_nCmdPos[0] = 1;
				return FALSE;
			}

			BigEndian(ipuRegs->top, ipuRegs->top);

			IPU_LOG("IPU VDEC command data 0x%x(0x%x). Skip 0x%X bits/Table=%d (%s), pct %d",
			        ipuRegs->cmd.DATA, ipuRegs->cmd.DATA >> 16, val & 0x3f, (val >> 26) & 3, (val >> 26) & 1 ?
			        ((val >> 26) & 2 ? "DMV" : "MBT") : (((val >> 26) & 2 ? "MC" : "MBAI")), ipuRegs->ctrl.PCT);
			return TRUE;

			jNO_DEFAULT
	}

	return FALSE;
}

static BOOL ipuFDEC(u32 val)
{
	if (!getBits32((u8*)&ipuRegs->cmd.DATA, 0)) return FALSE;

	BigEndian(ipuRegs->cmd.DATA, ipuRegs->cmd.DATA);
	ipuRegs->top = ipuRegs->cmd.DATA;

	IPU_LOG("FDEC read: 0x%8.8x", ipuRegs->top);

	return TRUE;
}

static __forceinline BOOL ipuSETIQ(u32 val)
{
	int i;

	if ((val >> 27) & 1)
	{
		g_nCmdPos[0] += getBits((u8*)niq + g_nCmdPos[0], 512 - 8 * g_nCmdPos[0], 1); // 8*8*8

		IPU_LOG("Read non-intra quantization matrix from IPU FIFO.");
		for (i = 0; i < 8; i++)
		{
			IPU_LOG("%02X %02X %02X %02X %02X %02X %02X %02X",
			        niq[i * 8 + 0], niq[i * 8 + 1], niq[i * 8 + 2], niq[i * 8 + 3],
			        niq[i * 8 + 4], niq[i * 8 + 5], niq[i * 8 + 6], niq[i * 8 + 7]);
		}
	}
	else
	{
		g_nCmdPos[0] += getBits((u8*)iq + 8 * g_nCmdPos[0], 512 - 8 * g_nCmdPos[0], 1);

		IPU_LOG("Read intra quantization matrix from IPU FIFO.");
		for (i = 0; i < 8; i++)
		{
			IPU_LOG("%02X %02X %02X %02X %02X %02X %02X %02X",
			        iq[i * 8 + 0], iq[i * 8 + 1], iq[i * 8 + 2], iq[i *8 + 3],
			        iq[i * 8 + 4], iq[i * 8 + 5], iq[i * 8 + 6], iq[i *8 + 7]);
		}
	}

	return g_nCmdPos[0] == 64;
}

static __forceinline BOOL ipuSETVQ(u32 val)
{
	g_nCmdPos[0] += getBits((u8*)vqclut + g_nCmdPos[0], 256 - 8 * g_nCmdPos[0], 1); // 16*2*8

	if (g_nCmdPos[0] == 32)
	{
		IPU_LOG("IPU SETVQ command.\nRead VQCLUT table from IPU FIFO.");
		IPU_LOG(
		    "%02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d "
		    "%02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d"
		    "%02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d "
		    "%02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d",
		    vqclut[0] >> 10, (vqclut[0] >> 5) & 0x1F, vqclut[0] & 0x1F,
		    vqclut[1] >> 10, (vqclut[1] >> 5) & 0x1F, vqclut[1] & 0x1F,
		    vqclut[2] >> 10, (vqclut[2] >> 5) & 0x1F, vqclut[2] & 0x1F,
		    vqclut[3] >> 10, (vqclut[3] >> 5) & 0x1F, vqclut[3] & 0x1F,
		    vqclut[4] >> 10, (vqclut[4] >> 5) & 0x1F, vqclut[4] & 0x1F,
		    vqclut[5] >> 10, (vqclut[5] >> 5) & 0x1F, vqclut[5] & 0x1F,
		    vqclut[6] >> 10, (vqclut[6] >> 5) & 0x1F, vqclut[6] & 0x1F,
		    vqclut[7] >> 10, (vqclut[7] >> 5) & 0x1F, vqclut[7] & 0x1F,
		    vqclut[8] >> 10, (vqclut[8] >> 5) & 0x1F, vqclut[8] & 0x1F,
		    vqclut[9] >> 10, (vqclut[9] >> 5) & 0x1F, vqclut[9] & 0x1F,
		    vqclut[10] >> 10, (vqclut[10] >> 5) & 0x1F, vqclut[10] & 0x1F,
		    vqclut[11] >> 10, (vqclut[11] >> 5) & 0x1F, vqclut[11] & 0x1F,
		    vqclut[12] >> 10, (vqclut[12] >> 5) & 0x1F, vqclut[12] & 0x1F,
		    vqclut[13] >> 10, (vqclut[13] >> 5) & 0x1F, vqclut[13] & 0x1F,
		    vqclut[14] >> 10, (vqclut[14] >> 5) & 0x1F, vqclut[14] & 0x1F,
		    vqclut[15] >> 10, (vqclut[15] >> 5) & 0x1F, vqclut[15] & 0x1F);
	}

	return g_nCmdPos[0] == 32;
}

// IPU Transfers are split into 8Qwords so we need to send ALL the data
static BOOL __fastcall ipuCSC(u32 val)
{
	tIPU_CMD_CSC csc(val);

	IPU_LOG("IPU CSC(Colorspace conversion from YCbCr) command (%d).", csc.MBC);
	if (csc.OFM)
		IPU_LOG("Output format is RGB16. ");
	else
		IPU_LOG("Output format is RGB32. ");

	if (csc.DTE) IPU_LOG("Dithering enabled.");

	//Console::WriteLn("CSC");
	for (;g_nCmdIndex < (int)csc.MBC; g_nCmdIndex++)
	{

		if (g_nCmdPos[0] < 3072 / 8)
		{
			g_nCmdPos[0] += getBits((u8*) & mb8 + g_nCmdPos[0], 3072 - 8 * g_nCmdPos[0], 1);

			if (g_nCmdPos[0] < 3072 / 8) return FALSE;

			ipu_csc(&mb8, &rgb32, 0);
			if (csc.OFM) ipu_dither(&rgb32, &rgb16, csc.DTE);
		}

		if (csc.OFM)
		{
			while (g_nCmdPos[1] < 32)
			{
				g_nCmdPos[1] += FIFOfrom_write(((u32*) & rgb16) + 4 * g_nCmdPos[1], 32 - g_nCmdPos[1]);

				if (g_nCmdPos[1] <= 0) return FALSE;
			}
		}
		else
		{
			while (g_nCmdPos[1] < 64)
			{
				g_nCmdPos[1] += FIFOfrom_write(((u32*) & rgb32) + 4 * g_nCmdPos[1], 64 - g_nCmdPos[1]);

				if (g_nCmdPos[1] <= 0) return FALSE;
			}
		}

		g_nCmdPos[0] = 0;
		g_nCmdPos[1] = 0;
	}

	return TRUE;
}

// Todo - Need to add the same stop and start code as CSC
static BOOL ipuPACK(u32 val)
{
	tIPU_CMD_CSC  csc(val);

	IPU_LOG("IPU PACK (Colorspace conversion from RGB32) command.");

	if (csc.OFM)
		IPU_LOG("Output format is RGB16. ");
	else
		IPU_LOG("Output format is INDX4. ");

	if (csc.DTE) IPU_LOG("Dithering enabled.");

	IPU_LOG("Number of macroblocks to be converted: %d", csc.MBC);

	for (;g_nCmdIndex < (int)csc.MBC; g_nCmdIndex++)
	{
		if (g_nCmdPos[0] < 512)
		{
			g_nCmdPos[0] += getBits((u8*) & mb8 + g_nCmdPos[0], 512 - 8 * g_nCmdPos[0], 1);

			if (g_nCmdPos[0] < 64) return FALSE;

			ipu_csc(&mb8, &rgb32, 0);
			ipu_dither(&rgb32, &rgb16, csc.DTE);

			if (csc.OFM) ipu_vq(&rgb16, indx4);
		}

		if (csc.OFM)
		{
			g_nCmdPos[1] += FIFOfrom_write(((u32*) & rgb16) + 4 * g_nCmdPos[1], 32 - g_nCmdPos[1]);

			if (g_nCmdPos[1] < 32) return FALSE;
		}
		else
		{
			g_nCmdPos[1] += FIFOfrom_write(((u32*)indx4) + 4 * g_nCmdPos[1], 8 - g_nCmdPos[1]);

			if (g_nCmdPos[1] < 8) return FALSE;
		}

		g_nCmdPos[0] = 0;
		g_nCmdPos[1] = 0;
	}

	return TRUE;
}

static void ipuSETTH(u32 val)
{
	s_thresh[0] = (val & 0xff);
	s_thresh[1] = ((val >> 16) & 0xff);
	IPU_LOG("IPU SETTH (Set threshold value)command %x.", val&0xff00ff);
}

///////////////////////
// IPU Worker Thread //
///////////////////////
__forceinline void IPU_INTERRUPT() //dma
{
	hwIntcIrq(INTC_IPU);
}

void IPUCMD_WRITE(u32 val)
{
	// don't process anything if currently busy
	if (ipuRegs->ctrl.BUSY) Console::WriteLn("IPU BUSY!"); // wait for thread

	ipuRegs->ctrl.ECD = 0;
	ipuRegs->ctrl.SCD = 0; //clear ECD/SCD
	ipuRegs->cmd.DATA = val;
	g_nCmdPos[0] = 0;

	switch (ipuRegs->cmd.CMD)
	{
		case SCE_IPU_BCLR:
			ipuBCLR(val);
			IPU_INTERRUPT(); //DMAC_TO_IPU
			return;

		case SCE_IPU_VDEC:

			g_BP.BP += val & 0x3F;

			// check if enough data in queue
			if (ipuVDEC(val)) return;

			ipuRegs->cmd.BUSY = 0x80000000;
			ipuRegs->topbusy = 0x80000000;
			break;

		case SCE_IPU_FDEC:
			IPU_LOG("IPU FDEC command. Skip 0x%X bits, FIFO 0x%X qwords, BP 0x%X, FP %d, CHCR 0x%x, %x",
			        val & 0x3f, g_BP.IFC, (int)g_BP.BP, g_BP.FP, ipu1dma->chcr._u32, cpuRegs.pc);
			g_BP.BP += val & 0x3F;
			if (ipuFDEC(val)) return;
			ipuRegs->cmd.BUSY = 0x80000000;
			ipuRegs->topbusy = 0x80000000;
			break;

		case SCE_IPU_SETTH:
			ipuSETTH(val);
			hwIntcIrq(INTC_IPU);
			return;

		case SCE_IPU_SETIQ:
			IPU_LOG("IPU SETIQ command.");
			if (val & 0x3f) IPU_LOG("Skip %d bits.", val & 0x3f);
			g_BP.BP += val & 0x3F;
			if (ipuSETIQ(ipuRegs->cmd.DATA)) return;
			break;

		case SCE_IPU_SETVQ:
			if (ipuSETVQ(ipuRegs->cmd.DATA)) return;
			break;

		case SCE_IPU_CSC:
			g_nCmdPos[1] = 0;
			g_nCmdIndex = 0;

			if (ipuCSC(ipuRegs->cmd.DATA))
			{
				if (ipu0dma->qwc > 0 && ipu0dma->chcr.STR)  IPU_INT0_FROM();
				return;
			}
			break;

		case SCE_IPU_PACK:
			g_nCmdPos[1] = 0;
			g_nCmdIndex = 0;
			if (ipuPACK(ipuRegs->cmd.DATA)) return;
			break;

		case SCE_IPU_IDEC:
			if (ipuIDEC(val))
			{
				// idec done, ipu0 done too
				if (ipu0dma->qwc > 0 && ipu0dma->chcr.STR) IPU_INT0_FROM();
				return;
			}
			ipuRegs->topbusy = 0x80000000;
			// have to resort to the thread
			ipuCurCmd = val >> 28;
			ipuRegs->ctrl.BUSY = 1;
			return;

		case SCE_IPU_BDEC:
			if (ipuBDEC(val))
			{
				if (ipu0dma->qwc > 0 && ipu0dma->chcr.STR) IPU_INT0_FROM();
				if (ipuRegs->ctrl.SCD || ipuRegs->ctrl.ECD) hwIntcIrq(INTC_IPU);
				return;
			}
			ipuRegs->topbusy = 0x80000000;
			ipuCurCmd = val >> 28;
			ipuRegs->ctrl.BUSY = 1;
			return;
	}

	// have to resort to the thread
	ipuCurCmd = val >> 28;
	ipuRegs->ctrl.BUSY = 1;
	hwIntcIrq(INTC_IPU);
}

void IPUWorker()
{
	assert(ipuRegs->ctrl.BUSY);

	switch (ipuCurCmd)
	{
		case SCE_IPU_VDEC:
			if (!ipuVDEC(ipuRegs->cmd.DATA))
			{
				hwIntcIrq(INTC_IPU);
				return;
			}
			ipuRegs->cmd.BUSY = 0;
			ipuRegs->topbusy = 0;
			break;

		case SCE_IPU_FDEC:
			if (!ipuFDEC(ipuRegs->cmd.DATA))
			{
				hwIntcIrq(INTC_IPU);
				return;
			}
			ipuRegs->cmd.BUSY = 0;
			ipuRegs->topbusy = 0;
			break;

		case SCE_IPU_SETIQ:
			if (!ipuSETIQ(ipuRegs->cmd.DATA))
			{
				hwIntcIrq(INTC_IPU);
				return;
			}
			break;

		case SCE_IPU_SETVQ:
			if (!ipuSETVQ(ipuRegs->cmd.DATA))
			{
				hwIntcIrq(INTC_IPU);
				return;
			}
			break;

		case SCE_IPU_CSC:
			if (!ipuCSC(ipuRegs->cmd.DATA))
			{
				hwIntcIrq(INTC_IPU);
				return;
			}
			if (ipu0dma->qwc > 0 && ipu0dma->chcr.STR)  IPU_INT0_FROM();
			break;

		case SCE_IPU_PACK:
			if (!ipuPACK(ipuRegs->cmd.DATA))
			{
				hwIntcIrq(INTC_IPU);
				return;
			}
			break;

		case SCE_IPU_IDEC:
			so_call(s_routine);
			if (!s_RoutineDone)
			{
				hwIntcIrq(INTC_IPU);
				return;
			}

			ipuRegs->ctrl.OFC = 0;
			ipuRegs->ctrl.BUSY = 0;
			ipuRegs->topbusy = 0;
			ipuRegs->cmd.BUSY = 0;
			ipuCurCmd = 0xffffffff;

			// CHECK!: IPU0dma remains when IDEC is done, so we need to clear it
			if (ipu0dma->qwc > 0 && ipu0dma->chcr.STR) IPU_INT0_FROM();
			s_routine = NULL;
			break;

		case SCE_IPU_BDEC:
			so_call(s_routine);
			if (!s_RoutineDone)
			{
				hwIntcIrq(INTC_IPU);
				return;
			}

			ipuRegs->ctrl.BUSY = 0;
			ipuRegs->topbusy = 0;
			ipuRegs->cmd.BUSY = 0;
			ipuCurCmd = 0xffffffff;

			if (ipu0dma->qwc > 0 && ipu0dma->chcr.STR) IPU_INT0_FROM();
			s_routine = NULL;
			if (ipuRegs->ctrl.SCD || ipuRegs->ctrl.ECD) hwIntcIrq(INTC_IPU);
			return;

		default:
			Console::WriteLn("Unknown IPU command: %x", ipuRegs->cmd.CMD);
			break;
	}

	// success
	ipuRegs->ctrl.BUSY = 0;
	ipuCurCmd = 0xffffffff;
}

/////////////////
// Buffer reader

// move the readbits queue
__forceinline void inc_readbits()
{
	readbits += 16;
	if (readbits >= _readbits + 64)
	{
		// move back
		*(u64*)(_readbits) = *(u64*)(_readbits + 64);
		*(u64*)(_readbits + 8) = *(u64*)(_readbits + 72);
		readbits = _readbits;
	}
}

// returns the pointer of readbits moved by 1 qword
__forceinline u8* next_readbits()
{
	return readbits + 16;
}

// returns the pointer of readbits moved by 1 qword
u8* prev_readbits()
{
	if (readbits < _readbits + 16) return _readbits + 48 - (readbits - _readbits);

	return readbits -16;
}

void ReorderBitstream()
{
	readbits = prev_readbits();
	g_BP.FP = 2;
}

// IPU has a 2qword internal buffer whose status is pointed by FP.
// If FP is 1, there's 1 qword in buffer. Second qword is only loaded
// incase there are less than 32bits available in the first qword.
// \return Number of bits available (clamps at 16 bits)
u16 __fastcall FillInternalBuffer(u32 * pointer, u32 advance, u32 size)
{
	if (g_BP.FP == 0)
	{
		if (FIFOto_read(next_readbits()) == 0) return 0;

		inc_readbits();
		g_BP.FP = 1;
	}
	else if ((g_BP.FP < 2) && (*(int*)pointer + size) >= 128)
	{
		if (FIFOto_read(next_readbits())) g_BP.FP += 1;
	}

	if (*(int*)pointer >= 128)
	{
		assert(g_BP.FP >= 1);

		if (g_BP.FP > 1) inc_readbits();

		if (advance)
		{
			g_BP.FP--;
			*pointer &= 127;
		}
	}

	return (g_BP.FP >= 1) ? g_BP.FP * 128 - (*(int*)pointer) : 0;
}

// whenever reading fractions of bytes. The low bits always come from the next byte
// while the high bits come from the current byte
u8 __fastcall getBits32(u8 *address, u32 advance)
{
	register u32 mask, shift = 0;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 32) < 32) return 0;

	readpos = readbits + (int)g_BP.BP / 8;

	if (g_BP.BP & 7)
	{
		shift = g_BP.BP & 7;
		mask = (0xff >> shift);
		mask = mask | (mask << 8) | (mask << 16) | (mask << 24);

		*(u32*)address = ((~mask & *(u32*)(readpos + 1)) >> (8 - shift)) | (((mask) & *(u32*)readpos) << shift);
	}
	else
	{
		*(u32*)address = *(u32*)readpos;
	}

	if (advance) g_BP.BP += 32;

	return 1;
}

__forceinline u8 __fastcall getBits16(u8 *address, u32 advance)
{
	register u32 mask, shift = 0;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 16) < 16) return 0;

	readpos = readbits + (int)g_BP.BP / 8;

	if (g_BP.BP & 7)
	{
		shift = g_BP.BP & 7;
		mask = (0xff >> shift);
		mask = mask | (mask << 8);

		*(u16*)address = ((~mask & *(u16*)(readpos + 1)) >> (8 - shift)) | (((mask) & *(u16*)readpos) << shift);
	}
	else
	{
		*(u16*)address = *(u16*)readpos;
	}

	if (advance) g_BP.BP += 16;

	return 1;
}

u8 __fastcall getBits8(u8 *address, u32 advance)
{
	register u32 mask, shift = 0;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 8) < 8)
		return 0;

	readpos = readbits + (int)g_BP.BP / 8;

	if (g_BP.BP & 7)
	{
		shift = g_BP.BP & 7;
		mask = (0xff >> shift);

		*(u8*)address = (((~mask) & readpos[1]) >> (8 - shift)) | (((mask) & *readpos) << shift);
	}
	else
	{
		*(u8*)address = *(u8*)readpos;
	}

	if (advance) g_BP.BP += 8;

	return 1;
}

int __fastcall getBits(u8 *address, u32 size, u32 advance)
{
	register u32 mask = 0, shift = 0, howmuch;
	u8* oldbits, *oldaddr = address;
	u32 pointer = 0, temp;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 8) < 8) return 0;

	oldbits = readbits;
	// Backup the current BP in case of VDEC/FDEC
	pointer = g_BP.BP;

	if (pointer & 7)
	{
		address--;
		while (size)
		{
			if (shift == 0)
			{
				*++address = 0;
				shift = 8;
			}

			temp = shift; // Lets not pass a register to min.
			howmuch = min(min(8 - (pointer & 7), 128 - pointer), min(size, temp));

			if (FillInternalBuffer(&pointer, advance, 8) < 8)
			{
				if (advance) g_BP.BP = pointer;
				return address - oldaddr;
			}

			mask = ((0xFF >> (pointer & 7)) << (8 - howmuch - (pointer & 7))) & 0xFF;
			mask &= readbits[((pointer) >> 3)];
			mask >>= 8 - howmuch - (pointer & 7);
			pointer += howmuch;
			size -= howmuch;
			shift -= howmuch;
			*address |= mask << shift;
		}
		++address;
	}
	else
	{
		u8* readmem;
		while (size)
		{
			if (FillInternalBuffer(&pointer, advance, 8) < 8)
			{
				if (advance) g_BP.BP = pointer;
				return address -oldaddr;
			}

			howmuch = min(128 - pointer, size);
			size -= howmuch;

			readmem = readbits + (pointer >> 3);
			pointer += howmuch;
			howmuch >>= 3;

			while (howmuch >= 4)
			{
				*(u32*)address = *(u32*)readmem;
				howmuch -= 4;
				address += 4;
				readmem += 4;
			}

			switch (howmuch)
			{
				case 3:
					address[2] = readmem[2];
				case 2:
					address[1] = readmem[1];
				case 1:
					address[0] = readmem[0];
				case 0:
					break;

					jNO_DEFAULT
			}

			address += howmuch;
		}
	}

	// If not advance then reset the Reading buffer value
	if (advance)
		g_BP.BP = pointer;
	else
		readbits = oldbits; // restore the last pointer

	return address - oldaddr;
}

///////////////////// CORE FUNCTIONS /////////////////
void Skl_YUV_To_RGB32_MMX(u8 *RGB, const int Dst_BpS, const u8 *Y, const u8 *U, const u8 *V,
                          const int Src_BpS, const int Width, const int Height);

void __fastcall ipu_csc(macroblock_8 *mb8, macroblock_rgb32 *rgb32, int sgn)
{
	int i;
	u8* p = (u8*)rgb32;

	yuv2rgb_sse2();

	if (s_thresh[0] > 0)
	{
		for (i = 0; i < 64*4; i++, p += 4)
		{
			if ((p[0] < s_thresh[0]) && (p[1] < s_thresh[0]) && (p[2] < s_thresh[0]))
				*(u32*)p = 0;
			else
				p[3] = ((p[1] < s_thresh[1]) && (p[2] < s_thresh[1]) && (p[3] < s_thresh[1])) ? 0x40 : 0x80;
		}
	}
	else if (s_thresh[1] > 0)
	{
		for (i = 0; i < 64*4; i++, p += 4)
		{
			p[3] = ((p[1] < s_thresh[1]) && (p[2] < s_thresh[1]) && (p[3] < s_thresh[1])) ? 0x40 : 0x80;
		}
	}
	else
	{
		for (i = 0; i < 64; i++, p += 16)
		{
			p[3] = p[7] = p[11] = p[15] = 0x80;
		}
	}
}

void __fastcall ipu_dither(const macroblock_rgb32* rgb32, macroblock_rgb16 *rgb16, int dte)
{
	int i, j;
	for (i = 0; i < 16; ++i)
	{
		for (j = 0; j < 16; ++j)
		{
			rgb16->c[i][j].r = rgb32->c[i][j].r >> 3;
			rgb16->c[i][j].g = rgb32->c[i][j].g >> 3;
			rgb16->c[i][j].b = rgb32->c[i][j].b >> 3;
			rgb16->c[i][j].a = rgb32->c[i][j].a == 0x40;
		}
	}
}

void __fastcall ipu_vq(macroblock_rgb16 *rgb16, u8* indx4)
{
	Console::Error("IPU: VQ not implemented");
}

void __fastcall ipu_copy(const macroblock_8 *mb8, macroblock_16 *mb16)
{
	const u8	*s = (const u8*)mb8;
	s16	*d = (s16*)mb16;
	int i;
	for (i = 0; i < 256; i++) *d++ = *s++;		//Y  bias	- 16
	for (i = 0; i < 64; i++) *d++ = *s++;		//Cr bias	- 128
	for (i = 0; i < 64; i++) *d++ = *s++;		//Cb bias	- 128
}

///////////////////// IPU DMA ////////////////////////
void FIFOto_clear()
{
	memzero(fifo_input);
	g_BP.IFC = 0;
	ipuRegs->ctrl.IFC = 0;
	FIreadpos = 0;
	FIwritepos = 0;
}

int FIFOto_read(void *value)
{
	// wait until enough data
	if (g_BP.IFC == 0)
	{
		// This is the only spot that wants a return value for IPU1dma.
		if (IPU1dma() == 0) return 0;
		assert(g_BP.IFC > 0);
	}

	// transfer 1 qword, split into two transfers
	for (int i = 0; i <= 3; i++)
	{
		((u32*)value)[i] = fifo_input[FIreadpos + i];
		fifo_input[FIreadpos + i] = 0;
	}

	FIreadpos = (FIreadpos + 4) & 31;
	g_BP.IFC--;
	return 1;
}

int FIFOto_write(u32* pMem, int size)
{
	int transsize;
	int firsttrans = min(size, 8 - (int)g_BP.IFC);

	g_BP.IFC += firsttrans;
	transsize = firsttrans;

	while (transsize-- > 0)
	{
		for (int i = 0; i <= 3; i++)
		{
			fifo_input[FIwritepos + i] = pMem[i];
		}
		FIwritepos = (FIwritepos + 4) & 31;
		pMem += 4;
	}

	return firsttrans;
}

static __forceinline bool IPU1chain(int &totalqwc)
{
	if (ipu1dma->qwc > 0)
	{
		int qwc = ipu1dma->qwc;
		u32 *pMem;

		pMem = (u32*)dmaGetAddr(ipu1dma->madr);

		if (pMem == NULL)
		{
			Console::Error("ipu1dma NULL!");
			return true;
		}

		qwc = FIFOto_write(pMem, qwc);
		ipu1dma->madr += qwc<< 4;
		ipu1dma->qwc -= qwc;
		totalqwc += qwc;

		if (ipu1dma->qwc > 0)
		{
			g_nDMATransfer.ACTV1 = 1;
			return true;
		}
	}
	return false;
}

// Remind me to give this a better name. --arcum42
static __forceinline bool IncreaseTadr(u32 tag)
{
	switch (Tag::Id(tag))
	{
		case TAG_REFE:  // refe
			ipu1dma->tadr += 16;
			return true;

		case TAG_END: // end
			ipu1dma->tadr = ipu1dma->madr;
			return true;
	}
	return false;
}

extern void gsInterrupt();

static __forceinline bool ipuDmacSrcChain(DMACh *tag, u32 *ptag)
{
	switch (Tag::Id(ptag))
	{
		case TAG_REFE: // refe
			// do not change tadr
			tag->madr = ptag[1];
			return true;
			break;

		case TAG_CNT: // cnt
			tag->madr = tag->tadr + 16;
			// Set the taddr to the next tag
			tag->tadr += 16 + (tag->qwc << 4);
			break;

		case TAG_NEXT: // next
			tag->madr = tag->tadr + 16;
			tag->tadr = ptag[1];
			break;

		case TAG_REF: // ref
			tag->madr = ptag[1];
			tag->tadr += 16;
			break;

		case TAG_END: // end
			// do not change tadr
			tag->madr = tag->tadr + 16;
			return true;
			break;

		default:
			Console::Error("IPU ERROR: different transfer mode!, Please report to PCSX2 Team");
			break;
	}

	return false;
}

static __forceinline void flushGIF()
{
	while(gif->chcr.STR && (vif1Regs->mskpath3 == 0))
	{
		GIF_LOG("Flushing gif chcr %x tadr %x madr %x qwc %x", gif->chcr._u32, gif->tadr, gif->madr, gif->qwc);
		gsInterrupt();
	}
}

int IPU1dma()
{
	u32 *ptag;
	bool done = false;
	int ipu1cycles = 0, totalqwc = 0;

	assert(!ipu1dma->chcr.TTE);

	if (!(ipu1dma->chcr.STR) || (cpuRegs.interrupt & (1 << DMAC_TO_IPU))) return 0;

	assert(g_nDMATransfer.TIE1 == 0);

	//We need to make sure GIF has flushed before sending IPU data, it seems to REALLY screw FFX videos
	flushGIF();

	// in kh, qwc == 0 when dma_actv1 is set
	if ((g_nDMATransfer.ACTV1) && ipu1dma->qwc > 0)
	{
		if (IPU1chain(totalqwc)) return totalqwc;

		//Check TIE bit of CHCR and IRQ bit of tag
		if (ipu1dma->chcr.TIE && (g_nDMATransfer.DOTIE1))
		{
			Console::WriteLn("IPU1 TIE");

			IPU_INT_TO(totalqwc * BIAS);
			g_nDMATransfer.TIE1 = 1;
			g_nDMATransfer.DOTIE1 = 0;
			g_nDMATransfer.ACTV1 = 0;

			return totalqwc;
		}

		if (ipu1dma->chcr.MOD == NORMAL_MODE) // If mode is normal mode.
		{
			IPU_INT_TO(totalqwc * BIAS);
			return totalqwc;
		}
		else
		{
			// Chain mode.
			u32 tag = ipu1dma->chcr._u32; // upper bits describe current tag

			if (ipu1dma->chcr.TIE && Tag::IRQ(tag))
			{
				ptag = (u32*)dmaGetAddr(ipu1dma->tadr);

				IncreaseTadr(tag);

				Tag::UpperTransfer(ipu1dma, ptag);

				IPU_LOG("IPU dmaIrq Set");
				IPU_INT_TO(totalqwc * BIAS);
				g_nDMATransfer.TIE1 = 1;
				return totalqwc;
			}

			if (IncreaseTadr(tag))
			{
				IPU_INT_TO((1 + totalqwc)*BIAS);
				return totalqwc;
			}
		}

		g_nDMATransfer.DOTIE1 = 0;
		g_nDMATransfer.ACTV1 = 0;
	}

	// Normal Mode & qwc is finished
	if ((ipu1dma->chcr.MOD == NORMAL_MODE) && (ipu1dma->qwc == 0))
	{
		//Console::WriteLn("ipu1 normal empty qwc?");
		return totalqwc;
	}

	// Transfer Dn_QWC from Dn_MADR to GIF
	if (ipu1dma->qwc > 0)
	{
		IPU_LOG("dmaIPU1 Normal size=%d, addr=%lx, fifosize=%x",
		        ipu1dma->qwc, ipu1dma->madr, 8 - g_BP.IFC);

		if (!IPU1chain(totalqwc)) IPU_INT_TO((ipu1cycles + totalqwc) * BIAS);

		return totalqwc;
	}
	else
	{
		// Chain Mode & ipu1dma->qwc is 0
		ptag = (u32*)dmaGetAddr(ipu1dma->tadr);  //Set memory pointer to TADR

		// Transfer the tag.
		if (!(Tag::Transfer("IPU1", ipu1dma, ptag))) return totalqwc;

		ipu1cycles += 1; // Add 1 cycles from the QW read for the tag

		done = ipuDmacSrcChain(ipu1dma, ptag);

		IPU_LOG("dmaIPU1 dmaChain %8.8x_%8.8x size=%d, addr=%lx, fifosize=%x",
		        ptag[1], ptag[0], ipu1dma->qwc, ipu1dma->madr, 8 - g_BP.IFC);

		g_nDMATransfer.DOTIE1 = (ipu1dma->chcr.TIE && Tag::IRQ(ptag));

		if (ipu1dma->qwc == 0)
		{
			//Check TIE bit of CHCR and IRQ bit of tag
			if (g_nDMATransfer.DOTIE1)
			{
				Console::WriteLn("IPU1 TIE");

				if (IPU1chain(totalqwc)) return totalqwc;

				if (done)
				{
					ptag = (u32*)dmaGetAddr(ipu1dma->tadr);

					IncreaseTadr(ptag[0]);

					// Transfer the last of ptag into chcr.
					Tag::UpperTransfer(ipu1dma, ptag);
				}

				IPU_INT_TO(ipu1cycles + totalqwc * BIAS);  // Should it be (ipu1cycles + totalqwc) * BIAS?
				g_nDMATransfer.TIE1 = 1;
				return totalqwc;
			}
			else
			{
				//Britney Dance beat does a blank NEXT tag, for some odd reason the fix doesnt work if after IPU1Chain O_o
				if (!done) IPU1dma();
				if (IPU1chain(totalqwc)) return totalqwc;
			}

			IncreaseTadr(ptag[0]);
		}
		else
		{
			if (IPU1chain(totalqwc)) return totalqwc;
		}
	}

	IPU_INT_TO((ipu1cycles + totalqwc) * BIAS);
	return totalqwc;
}

void FIFOfrom_clear()
{
	memzero(fifo_output);
	ipuRegs->ctrl.OFC = 0;
	FOreadpos = 0;
	FOwritepos = 0;
}

int FIFOfrom_write(const u32 *value, int size)
{
	int transsize, firsttrans;

	if ((int)ipuRegs->ctrl.OFC >= 8) IPU0dma();

	transsize = min(size, 8 - (int)ipuRegs->ctrl.OFC);
	firsttrans = transsize;

	while (transsize-- > 0)
	{
		for (int i = 0; i <= 3; i++)
		{
			fifo_output[FOwritepos + i] = ((u32*)value)[i];
		}
		FOwritepos = (FOwritepos + 4) & 31;
		value += 4;
	}

	ipuRegs->ctrl.OFC += firsttrans;
	IPU0dma();
	//Console::WriteLn("Written %d qwords, %d", firsttrans,ipuRegs->ctrl.OFC);

	return firsttrans;
}

static __forceinline void _FIFOfrom_readsingle(void *value)
{
	// transfer 1 qword, split into two transfers
	for (int i = 0; i <= 3; i++)
	{
		((u32*)value)[i] = fifo_output[FOreadpos + i];
		fifo_output[FOreadpos + i] = 0;
	}
	FOreadpos = (FOreadpos + 4) & 31;
}

void FIFOfrom_readsingle(void *value)
{
	if (ipuRegs->ctrl.OFC > 0)
	{
		ipuRegs->ctrl.OFC--;
		_FIFOfrom_readsingle(value);
	}
}

void FIFOfrom_read(void *value, int size)
{
	ipuRegs->ctrl.OFC -= size;
	while (size > 0)
	{
		_FIFOfrom_readsingle(value);
		value = (u32*)value + 4;
		size--;
	}
}

int IPU0dma()
{
	int readsize;
	void* pMem;

	if ((!(ipu0dma->chcr.STR) || (cpuRegs.interrupt & (1 << DMAC_FROM_IPU))) || (ipu0dma->qwc == 0))
		return 0;

	assert(!(ipu0dma->chcr.TTE));

	IPU_LOG("dmaIPU0 chcr = %lx, madr = %lx, qwc  = %lx",
	        ipu0dma->chcr._u32, ipu0dma->madr, ipu0dma->qwc);

	assert((ipu0dma->chcr._u32 & 0xC) == 0);
	pMem = (u32*)dmaGetAddr(ipu0dma->madr);

	readsize = min(ipu0dma->qwc, (u16)ipuRegs->ctrl.OFC);
	FIFOfrom_read(pMem, readsize);

	ipu0dma->madr += readsize << 4;
	ipu0dma->qwc -= readsize; // note: qwc is u16

	if (ipu0dma->qwc == 0)
	{
		if (dmacRegs->ctrl.STS == STS_fromIPU)   // STS == fromIPU
		{
			dmacRegs->stadr.ADDR = ipu0dma->madr;
			switch (dmacRegs->ctrl.STD)
			{
				case NO_STD:
					break;
				case STD_GIF: // GIF
					g_nDMATransfer.GIFSTALL = 1;
					break;
				case STD_VIF1: // VIF
					g_nDMATransfer.VIFSTALL = 1;
					break;
				case STD_SIF1:
					g_nDMATransfer.SIFSTALL = 1;
					break;
			}
		}
		IPU_INT_FROM(readsize*BIAS);
	}

	return readsize;
}

__forceinline void dmaIPU0() // fromIPU
{
	if (ipuRegs->ctrl.BUSY) IPUWorker();
}

__forceinline void dmaIPU1() // toIPU
{
	IPU1dma();
	if (ipuRegs->ctrl.BUSY) IPUWorker();
}

extern void GIFdma();

void ipu0Interrupt()
{
	IPU_LOG("ipu0Interrupt: %x", cpuRegs.cycle);

	if (g_nDMATransfer.FIREINT0)
	{
		g_nDMATransfer.FIREINT0 = 0;
		hwIntcIrq(INTC_IPU);
	}

	if (g_nDMATransfer.GIFSTALL)
	{
		// gif
		g_nDMATransfer.GIFSTALL = 0;
		if (gif->chcr.STR) GIFdma();
	}

	if (g_nDMATransfer.VIFSTALL)
	{
		// vif
		g_nDMATransfer.VIFSTALL = 0;
		if (vif1ch->chcr.STR) dmaVIF1();
	}

	if (g_nDMATransfer.SIFSTALL)
	{
		// sif
		g_nDMATransfer.SIFSTALL = 0;

		// Not totally sure whether this needs to be done or not, so I'm
		// leaving it commented out for the moment.
		//if (sif1dma->chcr.STR) SIF1Dma();
	}

	if (g_nDMATransfer.TIE0)
	{
		g_nDMATransfer.TIE0 = 0;
	}

	ipu0dma->chcr.STR = 0;
	hwDmacIrq(DMAC_FROM_IPU);
}

IPU_FORCEINLINE void ipu1Interrupt()
{
	IPU_LOG("ipu1Interrupt %x:", cpuRegs.cycle);

	if (g_nDMATransfer.FIREINT1)
	{
		hwIntcIrq(INTC_IPU);
		g_nDMATransfer.FIREINT1 = 0;
	}

	if (g_nDMATransfer.TIE1)
		g_nDMATransfer.TIE1 = 0;
	else
		ipu1dma->chcr.STR = 0;

	hwDmacIrq(DMAC_TO_IPU);
}
