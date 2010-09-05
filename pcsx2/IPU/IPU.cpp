/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
#include "IPUdma.h"
#include "yuv2rgb.h"
#include "mpeg2lib/Mpeg.h"

#include "Vif.h"
#include "Gif.h"
#include "Vif_Dma.h"
#include <limits.h>

static __fi void IPU_INT0_FROM()
{
	if (ipu0dma.qwc > 0 && ipu0dma.chcr.STR) ipu0Interrupt();
}

tIPU_cmd ipu_cmd;

void ReorderBitstream();

// the BP doesn't advance and returns -1 if there is no data to be read
__aligned16 tIPU_BP g_BP;

void IPUWorker();

// Color conversion stuff, the memory layout is a total hack
// convert_data_buffer is a pointer to the internal rgb struct (the first param in convert_init_t)
//char convert_data_buffer[sizeof(convert_rgb_t)];
//char convert_data_buffer[0x1C];							// unused?
//u8 PCT[] = {'r', 'I', 'P', 'B', 'D', '-', '-', '-'};		// unused?

// Quantization matrix
static u16 vqclut[16];				//clut conversion table
static u8 s_thresh[2];				//thresholds for color conversions
int coded_block_pattern = 0;


u8 indx4[16*16/2];
__aligned16 decoder_t decoder;

__aligned16 u8 _readbits[80];	//local buffer (ring buffer)
u8* readbits = _readbits;		// always can decrement by one 1qw

__fi void IPUProcessInterrupt()
{
	if (ipuRegs.ctrl.BUSY && g_BP.IFC) IPUWorker();
}

/////////////////////////////////////////////////////////
// Register accesses (run on EE thread)
int ipuInit()
{
	memzero(ipuRegs);
	memzero(g_BP);
	memzero(decoder);

	decoder.picture_structure = FRAME_PICTURE;	//default: progressive...my guess:P

	ipu_fifo.init();
	ipu_cmd.clear();
	
	return 0;
}

void ipuReset()
{
	ipuInit();
}

void ReportIPU()
{
	//Console.WriteLn(g_nDMATransfer.desc());
	Console.WriteLn(ipu_fifo.in.desc());
	Console.WriteLn(ipu_fifo.out.desc());
	Console.WriteLn(g_BP.desc());
	Console.WriteLn("vqclut = 0x%x.", vqclut);
	Console.WriteLn("s_thresh = 0x%x.", s_thresh);
	Console.WriteLn("coded_block_pattern = 0x%x.", coded_block_pattern);
	Console.WriteLn("g_decoder = 0x%x.", &decoder);
	Console.WriteLn("mpeg2_scan = 0x%x.", &mpeg2_scan);
	Console.WriteLn(ipu_cmd.desc());
	Console.WriteLn("_readbits = 0x%x. readbits - _readbits, which is also frozen, is 0x%x.",
		_readbits, readbits - _readbits);
	Console.Newline();
}

void SaveStateBase::ipuFreeze()
{
	// Get a report of the status of the ipu variables when saving and loading savestates.
	//ReportIPU();
	FreezeTag("IPU");
	Freeze(ipu_fifo);

	Freeze(g_BP);
	Freeze(vqclut);
	Freeze(s_thresh);
	Freeze(coded_block_pattern);
	Freeze(decoder);
	Freeze(ipu_cmd);
	Freeze(_readbits);

	int temp = readbits - _readbits;
	Freeze(temp);

	if (IsLoading())
	{
		readbits = _readbits;
	}
}

void tIPU_CMD_IDEC::log() const
{
	IPU_LOG("IDEC command.");

	if (FB) IPU_LOG(" Skip %d	bits.", FB);
	IPU_LOG(" Quantizer step code=0x%X.", QSC);

	if (DTD == 0)
		IPU_LOG(" Does not decode DT.");
	else
		IPU_LOG(" Decodes DT.");

	if (SGN == 0)
		IPU_LOG(" No bias.");
	else
		IPU_LOG(" Bias=128.");

	if (DTE == 1) IPU_LOG(" Dither Enabled.");
	if (OFM == 0)
		IPU_LOG(" Output format is RGB32.");
	else
		IPU_LOG(" Output format is RGB16.");

	IPU_LOG("");
}

void tIPU_CMD_BDEC::log(int s_bdec) const
{
	IPU_LOG("BDEC(macroblock decode) command %x, num: 0x%x", cpuRegs.pc, s_bdec);
	if (FB) IPU_LOG(" Skip 0x%X bits.", FB);

	if (MBI)
		IPU_LOG(" Intra MB.");
	else
		IPU_LOG(" Non-intra MB.");

	if (DCR)
		IPU_LOG(" Resets DC prediction value.");
	else
		IPU_LOG(" Doesn't reset DC prediction value.");

	if (DT)
		IPU_LOG(" Use field DCT.");
	else
		IPU_LOG(" Use frame DCT.");

	IPU_LOG(" Quantizer step=0x%X", QSC);
}

void tIPU_CMD_CSC::log_from_YCbCr() const
{
	IPU_LOG("CSC(Colorspace conversion from YCbCr) command (%d).", MBC);
	if (OFM)
		IPU_LOG("Output format is RGB16. ");
	else
		IPU_LOG("Output format is RGB32. ");

	if (DTE) IPU_LOG("Dithering enabled.");
}

void tIPU_CMD_CSC::log_from_RGB32() const
{
	IPU_LOG("PACK (Colorspace conversion from RGB32) command.");

	if (OFM)
		IPU_LOG("Output format is RGB16. ");
	else
		IPU_LOG("Output format is INDX4. ");

	if (DTE) IPU_LOG("Dithering enabled.");

	IPU_LOG("Number of macroblocks to be converted: %d", MBC);
}


__fi u32 ipuRead32(u32 mem)
{
	// Note: It's assumed that mem's input value is always in the 0x10002000 page
	// of memory (if not, it's probably bad code).

	pxAssert((mem & ~0xff) == 0x10002000);
	mem &= 0xff;	// ipu repeats every 0x100

	//IPUProcessInterrupt();

	switch (mem)
	{
		ipucase(IPU_CTRL): // IPU_CTRL
			ipuRegs.ctrl.IFC = g_BP.IFC;
			ipuRegs.ctrl.CBP = coded_block_pattern;

			if (!ipuRegs.ctrl.BUSY)
				IPU_LOG("read32: IPU_CTRL=0x%08X", ipuRegs.ctrl._u32);

		return ipuRegs.ctrl._u32;

		ipucase(IPU_BP): // IPU_BP
			ipuRegs.ipubp = g_BP.BP & 0x7f;
			ipuRegs.ipubp |= g_BP.IFC << 8;
			ipuRegs.ipubp |= (g_BP.FP /*+ g_BP.bufferhasnew*/) << 16;

			IPU_LOG("read32: IPU_BP=0x%08X", ipuRegs.ipubp);
		return ipuRegs.ipubp;

		default:
			IPU_LOG("read32: Addr=0x%08X Value = 0x%08X", mem, psHu32(IPU_CMD + mem));
	}

	return psHu32(IPU_CMD + mem);
}

__fi u64 ipuRead64(u32 mem)
{
	// Note: It's assumed that mem's input value is always in the 0x10002000 page
	// of memory (if not, it's probably bad code).

	pxAssert((mem & ~0xff) == 0x10002000);
	mem &= 0xff;	// ipu repeats every 0x100

	//IPUProcessInterrupt();

	switch (mem)
	{
		ipucase(IPU_CMD): // IPU_CMD
			if (ipuRegs.cmd.DATA & 0xffffff)
				IPU_LOG("read64: IPU_CMD=BUSY=%x, DATA=%08X", ipuRegs.cmd.BUSY ? 1 : 0, ipuRegs.cmd.DATA);
			break;

		ipucase(IPU_CTRL):
			DevCon.Warning("reading 64bit IPU ctrl");
			break;

		ipucase(IPU_BP):
			DevCon.Warning("reading 64bit IPU top");
			break;

		ipucase(IPU_TOP): // IPU_TOP
			IPU_LOG("read64: IPU_TOP=%x,  bp = %d", ipuRegs.top, g_BP.BP);
			break;

		default:
			IPU_LOG("read64: Unknown=%x", mem);
			break;
	}
	return psHu64(IPU_CMD + mem);
}

void ipuSoftReset()
{
	ipu_fifo.clear();

	coded_block_pattern = 0;

	ipuRegs.ctrl.reset();
	ipuRegs.top = 0;
	ipu_cmd.clear();
	ipuRegs.cmd.BUSY = 0;

	g_BP.BP = 0;
	g_BP.FP = 0;
	//g_BP.bufferhasnew = 0;
}

__fi bool ipuWrite32(u32 mem, u32 value)
{
	// Note: It's assumed that mem's input value is always in the 0x10002000 page
	// of memory (if not, it's probably bad code).

	pxAssert((mem & ~0xfff) == 0x10002000);
	mem &= 0xfff;

	IPUProcessInterrupt();

	switch (mem)
	{
		ipucase(IPU_CMD): // IPU_CMD
			IPU_LOG("write32: IPU_CMD=0x%08X", value);
			IPUCMD_WRITE(value);
		return false;

		ipucase(IPU_CTRL): // IPU_CTRL
            // CTRL = the first 16 bits of ctrl [0x8000ffff], + value for the next 16 bits,
            // minus the reserved bits. (18-19; 27-29) [0x47f30000]
			ipuRegs.ctrl.write(value);
			if (ipuRegs.ctrl.IDP == 3)
			{
				Console.WriteLn("IPU Invalid Intra DC Precision, switching to 9 bits");
				ipuRegs.ctrl.IDP = 1;
			}

			if (ipuRegs.ctrl.RST) ipuSoftReset(); // RESET

			IPU_LOG("write32: IPU_CTRL=0x%08X", value);
		return false;
	}
	return true;
}

// returns FALSE when the writeback is handled, TRUE if the caller should do the
// writeback itself.
__fi bool ipuWrite64(u32 mem, u64 value)
{
	// Note: It's assumed that mem's input value is always in the 0x10002000 page
	// of memory (if not, it's probably bad code).

	pxAssert((mem & ~0xfff) == 0x10002000);
	mem &= 0xfff;

	IPUProcessInterrupt();

	switch (mem)
	{
		ipucase(IPU_CMD):
			IPU_LOG("write64: IPU_CMD=0x%08X", value);
			IPUCMD_WRITE((u32)value);
		return false;
	}

	return true;
}


//////////////////////////////////////////////////////
// IPU Commands (exec on worker thread only)

static void ipuBCLR(u32 val)
{
	ipu_fifo.in.clear();

	g_BP.BP = val & 0x7F;
	g_BP.FP = 0;
	//g_BP.bufferhasnew = 0;
	ipuRegs.ctrl.BUSY = 0;
	ipuRegs.cmd.BUSY = 0;
	memzero(_readbits);
	IPU_LOG("Clear IPU input FIFO. Set Bit offset=0x%X", g_BP.BP);
}

static bool ipuIDEC(u32 val, bool resume)
{
	tIPU_CMD_IDEC idec(val);

	if (!resume)
	{
		idec.log();
		g_BP.BP += idec.FB;//skip FB bits

	//from IPU_CTRL
		ipuRegs.ctrl.PCT = I_TYPE; //Intra DECoding;)

		decoder.coding_type			= ipuRegs.ctrl.PCT;
		decoder.mpeg1				= ipuRegs.ctrl.MP1;
		decoder.q_scale_type		= ipuRegs.ctrl.QST;
		decoder.intra_vlc_format	= ipuRegs.ctrl.IVF;
		decoder.scantype			= ipuRegs.ctrl.AS;
		decoder.intra_dc_precision	= ipuRegs.ctrl.IDP;

	//from IDEC value
		decoder.quantizer_scale		= idec.QSC;
		decoder.frame_pred_frame_dct= !idec.DTD;
		decoder.sgn = idec.SGN;
		decoder.dte = idec.DTE;
		decoder.ofm = idec.OFM;

	//other stuff
		decoder.dcr = 1; // resets DC prediction value
	}

	return mpeg2sliceIDEC();
}

static int s_bdec = 0;

static __fi bool ipuBDEC(u32 val, bool resume)
{
	tIPU_CMD_BDEC bdec(val);

	if (!resume)
	{
		bdec.log(s_bdec);
		if (IsDebugBuild) s_bdec++;

	g_BP.BP += bdec.FB;//skip FB bits
		decoder.coding_type			= I_TYPE;
		decoder.mpeg1				= ipuRegs.ctrl.MP1;
		decoder.q_scale_type		= ipuRegs.ctrl.QST;
		decoder.intra_vlc_format	= ipuRegs.ctrl.IVF;
		decoder.scantype			= ipuRegs.ctrl.AS;
		decoder.intra_dc_precision	= ipuRegs.ctrl.IDP;

	//from BDEC value
		decoder.quantizer_scale		= decoder.q_scale_type ? non_linear_quantizer_scale [bdec.QSC] : bdec.QSC << 1;
		decoder.macroblock_modes	= bdec.DT ? DCT_TYPE_INTERLACED : 0;
		decoder.dcr					= bdec.DCR;
		decoder.macroblock_modes	|= bdec.MBI ? MACROBLOCK_INTRA : MACROBLOCK_PATTERN;

		memzero_sse_a(decoder.mb8);
		memzero_sse_a(decoder.mb16);
	}

	return mpeg2_slice();
}

static bool __fastcall ipuVDEC(u32 val)
{
	switch (ipu_cmd.pos[0])
	{
		case 0:
			ipuRegs.cmd.DATA = 0;
			if (!getBits32((u8*)&decoder.bitstream_buf, 0)) return false;

			decoder.bitstream_bits = -16;
			BigEndian(decoder.bitstream_buf, decoder.bitstream_buf);

			switch ((val >> 26) & 3)
			{
				case 0://Macroblock Address Increment
					decoder.mpeg1 = ipuRegs.ctrl.MP1;
					ipuRegs.cmd.DATA = get_macroblock_address_increment();
					break;

				case 1://Macroblock Type
					decoder.frame_pred_frame_dct = 1;
					decoder.coding_type = ipuRegs.ctrl.PCT;
					ipuRegs.cmd.DATA = get_macroblock_modes();
					break;

				case 2://Motion Code
					ipuRegs.cmd.DATA = get_motion_delta(0);
					break;

				case 3://DMVector
					ipuRegs.cmd.DATA = get_dmv();
					break;
			}

			g_BP.BP += (int)decoder.bitstream_bits + 16;

			if ((int)g_BP.BP < 0)
			{
				g_BP.BP += 128;
				ReorderBitstream();
			}

			ipuRegs.cmd.DATA = (ipuRegs.cmd.DATA & 0xFFFF) | ((decoder.bitstream_bits + 16) << 16);
			ipuRegs.ctrl.ECD = (ipuRegs.cmd.DATA == 0);

		case 1:
			if (!getBits32((u8*)&ipuRegs.top, 0))
			{
				ipu_cmd.pos[0] = 1;
				return false;
			}

			BigEndian(ipuRegs.top, ipuRegs.top);

			IPU_LOG("VDEC command data 0x%x(0x%x). Skip 0x%X bits/Table=%d (%s), pct %d",
			        ipuRegs.cmd.DATA, ipuRegs.cmd.DATA >> 16, val & 0x3f, (val >> 26) & 3, (val >> 26) & 1 ?
			        ((val >> 26) & 2 ? "DMV" : "MBT") : (((val >> 26) & 2 ? "MC" : "MBAI")), ipuRegs.ctrl.PCT);
			return true;

			jNO_DEFAULT
	}

	return false;
}

static __fi bool ipuFDEC(u32 val)
{
	if (!getBits32((u8*)&ipuRegs.cmd.DATA, 0)) return false;

	BigEndian(ipuRegs.cmd.DATA, ipuRegs.cmd.DATA);
	ipuRegs.top = ipuRegs.cmd.DATA;

	IPU_LOG("FDEC read: 0x%08x", ipuRegs.top);

	return true;
}

static bool ipuSETIQ(u32 val)
{
	int i;

	if ((val >> 27) & 1)
	{
		u8 (&niq)[64] = decoder.niq;

		for(;ipu_cmd.pos[0] < 8; ipu_cmd.pos[0]++)
		{
			if (!getBits64((u8*)niq + 8 * ipu_cmd.pos[0], 1)) return false;
		}

		IPU_LOG("Read non-intra quantization matrix from FIFO.");
		for (i = 0; i < 8; i++)
		{
			IPU_LOG("%02X %02X %02X %02X %02X %02X %02X %02X",
			        niq[i * 8 + 0], niq[i * 8 + 1], niq[i * 8 + 2], niq[i * 8 + 3],
			        niq[i * 8 + 4], niq[i * 8 + 5], niq[i * 8 + 6], niq[i * 8 + 7]);
		}
	}
	else
	{
		u8 (&iq)[64] = decoder.iq;

		for(;ipu_cmd.pos[0] < 8; ipu_cmd.pos[0]++)
		{
			if (!getBits64((u8*)iq + 8 * ipu_cmd.pos[0], 1)) return false;
		}

		IPU_LOG("Read intra quantization matrix from FIFO.");
		for (i = 0; i < 8; i++)
		{
			IPU_LOG("%02X %02X %02X %02X %02X %02X %02X %02X",
			        iq[i * 8 + 0], iq[i * 8 + 1], iq[i * 8 + 2], iq[i *8 + 3],
			        iq[i * 8 + 4], iq[i * 8 + 5], iq[i * 8 + 6], iq[i *8 + 7]);
		}
	}

	return true;
}

static bool ipuSETVQ(u32 val)
{
	for(;ipu_cmd.pos[0] < 4; ipu_cmd.pos[0]++)
	{
		if (!getBits64(((u8*)vqclut) + 8 * ipu_cmd.pos[0], 1)) return false;
	}

	IPU_LOG("SETVQ command.\nRead VQCLUT table from FIFO.");
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

	return true;
}

// IPU Transfers are split into 8Qwords so we need to send ALL the data
static bool __fastcall ipuCSC(u32 val)
{
	tIPU_CMD_CSC csc(val);
	csc.log_from_YCbCr();

	for (;ipu_cmd.index < (int)csc.MBC; ipu_cmd.index++)
	{
		for(;ipu_cmd.pos[0] < 48; ipu_cmd.pos[0]++)
		{
			if (!getBits64((u8*)&decoder.mb8 + 8 * ipu_cmd.pos[0], 1)) return false;
		}

		ipu_csc(decoder.mb8, decoder.rgb32, 0);
		if (csc.OFM) ipu_dither(decoder.rgb32, decoder.rgb16, csc.DTE);
		
		if (csc.OFM)
		{
			while (ipu_cmd.pos[1] < 32)
			{
				ipu_cmd.pos[1] += ipu_fifo.out.write(((u32*) & decoder.rgb16) + 4 * ipu_cmd.pos[1], 32 - ipu_cmd.pos[1]);

				if (ipu_cmd.pos[1] <= 0) return false;
			}
		}
		else
		{
			while (ipu_cmd.pos[1] < 64)
			{
				ipu_cmd.pos[1] += ipu_fifo.out.write(((u32*) & decoder.rgb32) + 4 * ipu_cmd.pos[1], 64 - ipu_cmd.pos[1]);

				if (ipu_cmd.pos[1] <= 0) return false;
			}
		}

		ipu_cmd.pos[0] = 0;
		ipu_cmd.pos[1] = 0;
	}

	return true;
}

// Todo - Need to add the same stop and start code as CSC
static bool ipuPACK(u32 val)
{
	tIPU_CMD_CSC  csc(val);
	csc.log_from_RGB32();

	for (;ipu_cmd.index < (int)csc.MBC; ipu_cmd.index++)
	{
		for(;ipu_cmd.pos[0] < 8; ipu_cmd.pos[0]++)
		{
			if (!getBits64((u8*)&decoder.mb8 + 8 * ipu_cmd.pos[0], 1)) return false;
		}

		ipu_csc(decoder.mb8, decoder.rgb32, 0);
		ipu_dither(decoder.rgb32, decoder.rgb16, csc.DTE);

		if (csc.OFM) ipu_vq(decoder.rgb16, indx4);

		if (csc.OFM)
		{
			ipu_cmd.pos[1] += ipu_fifo.out.write(((u32*) & decoder.rgb16) + 4 * ipu_cmd.pos[1], 32 - ipu_cmd.pos[1]);

			if (ipu_cmd.pos[1] < 32) return false;
		}
		else
		{
			ipu_cmd.pos[1] += ipu_fifo.out.write(((u32*)indx4) + 4 * ipu_cmd.pos[1], 8 - ipu_cmd.pos[1]);

			if (ipu_cmd.pos[1] < 8) return false;
		}

		ipu_cmd.pos[0] = 0;
		ipu_cmd.pos[1] = 0;
	}

	return TRUE;
}

static void ipuSETTH(u32 val)
{
	s_thresh[0] = (val & 0xff);
	s_thresh[1] = ((val >> 16) & 0xff);
	IPU_LOG("SETTH (Set threshold value)command %x.", val&0xff00ff);
}

// --------------------------------------------------------------------------------------
//  CORE Functions (referenced from MPEG library)
// --------------------------------------------------------------------------------------
__fi void ipu_csc(macroblock_8& mb8, macroblock_rgb32& rgb32, int sgn)
{
	int i;
	u8* p = (u8*)&rgb32;

	yuv2rgb();

	if (s_thresh[0] > 0)
	{
		for (i = 0; i < 16*16; i++, p += 4)
		{
			if ((p[0] < s_thresh[0]) && (p[1] < s_thresh[0]) && (p[2] < s_thresh[0]))
				*(u32*)p = 0;
			else if ((p[0] < s_thresh[1]) && (p[1] < s_thresh[1]) && (p[2] < s_thresh[1]))
				p[3] = 0x40;
		}
	}
	else if (s_thresh[1] > 0)
	{
		for (i = 0; i < 16*16; i++, p += 4)
		{
			if ((p[0] < s_thresh[1]) && (p[1] < s_thresh[1]) && (p[2] < s_thresh[1]))
				p[3] = 0x40;
		}
	}
	if (sgn)
	{
		for (i = 0; i < 16*16; i++, p += 4)
		{
			*(u32*)p ^= 0x808080;
		}
	}
}

__fi void ipu_dither(const macroblock_rgb32& rgb32, macroblock_rgb16& rgb16, int dte)
{
	int i, j;
	for (i = 0; i < 16; ++i)
	{
		for (j = 0; j < 16; ++j)
		{
			rgb16.c[i][j].r = rgb32.c[i][j].r >> 3;
			rgb16.c[i][j].g = rgb32.c[i][j].g >> 3;
			rgb16.c[i][j].b = rgb32.c[i][j].b >> 3;
			rgb16.c[i][j].a = rgb32.c[i][j].a == 0x40;
		}
	}
}

__fi void ipu_vq(macroblock_rgb16& rgb16, u8* indx4)
{
	Console.Error("IPU: VQ not implemented");
}

__fi void ipu_copy(const macroblock_8& mb8, macroblock_16& mb16)
{
	const u8	*s = (const u8*)&mb8;
	s16	*d = (s16*)&mb16;
	int i;
	for (i = 0; i < 256; i++) *d++ = *s++;		//Y  bias	- 16
	for (i = 0; i < 64; i++) *d++ = *s++;		//Cr bias	- 128
	for (i = 0; i < 64; i++) *d++ = *s++;		//Cb bias	- 128
}


// --------------------------------------------------------------------------------------
//  Buffer reader
// --------------------------------------------------------------------------------------

// move the readbits queue
__fi void inc_readbits()
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
__fi u8* next_readbits()
{
	return readbits + 16;
}

// returns the pointer of readbits moved by 1 qword
u8* prev_readbits()
{
	if (readbits < _readbits + 16) return _readbits + 48 - (readbits - _readbits);

	return readbits - 16;
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
		if (ipu_fifo.in.read(next_readbits()) == 0) return 0;

		inc_readbits();
		g_BP.FP = 1;
	}

	if ((g_BP.FP < 2) && ((*(int*)pointer + size) >= 128))
	{
		if (ipu_fifo.in.read(next_readbits())) g_BP.FP += 1;
	}

	if (*(int*)pointer >= 128)
	{
		pxAssert(g_BP.FP >= 1);

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
u8 __fastcall getBits128(u8 *address, u32 advance)
{
	u64 mask2;
	u128 mask;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 128) < 128) return 0;

	readpos = readbits + (int)g_BP.BP / 8;

	if (uint shift = (g_BP.BP & 7))
	{
		mask2 = 0xff >> shift;
		mask.lo = mask2 | (mask2 << 8) | (mask2 << 16) | (mask2 << 24) | (mask2 << 32) | (mask2 << 40) | (mask2 << 48) | (mask2 << 56);
		mask.hi = mask2 | (mask2 << 8) | (mask2 << 16) | (mask2 << 24) | (mask2 << 32) | (mask2 << 40) | (mask2 << 48) | (mask2 << 56);		

		u128 notMask;
		u128 data = *(u128*)(readpos + 1);
		notMask.lo = ~mask.lo & data.lo;
		notMask.hi = ~mask.hi & data.hi;
		notMask.lo >>= 8 - shift;
		notMask.lo |= (notMask.hi & (ULLONG_MAX >> (64 - shift))) << (64 - shift);
		notMask.hi >>= 8 - shift;

		mask.hi = (((*(u128*)readpos).hi & mask.hi) << shift) | (((*(u128*)readpos).lo & mask.lo) >> (64 - shift));
		mask.lo = ((*(u128*)readpos).lo & mask.lo) << shift;
		
		notMask.lo |= mask.lo;
		notMask.hi |= mask.hi;
		*(u128*)address = notMask;
	}
	else
	{
		*(u128*)address = *(u128*)readpos;
	}

	if (advance) g_BP.BP += 128;

	return 1;
}

// whenever reading fractions of bytes. The low bits always come from the next byte
// while the high bits come from the current byte
u8 __fastcall getBits64(u8 *address, u32 advance)
{
	register u64 mask = 0;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 64) < 64) return 0;

	readpos = readbits + (int)g_BP.BP / 8;

	if (uint shift = (g_BP.BP & 7))
	{
		mask = (0xff >> shift);
		mask = mask | (mask << 8) | (mask << 16) | (mask << 24) | (mask << 32) | (mask << 40) | (mask << 48) | (mask << 56);

		*(u64*)address = ((~mask & *(u64*)(readpos + 1)) >> (8 - shift)) | (((mask) & *(u64*)readpos) << shift);
	}
	else
	{
		*(u64*)address = *(u64*)readpos;
	}

	if (advance) g_BP.BP += 64;

	return 1;
}

// whenever reading fractions of bytes. The low bits always come from the next byte
// while the high bits come from the current byte
u8 __fastcall getBits32(u8 *address, u32 advance)
{
	u32 mask;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 32) < 32) return 0;

	readpos = readbits + (int)g_BP.BP / 8;

	if (uint shift = (g_BP.BP & 7))
	{
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

__fi u8 __fastcall getBits16(u8 *address, u32 advance)
{
	u32 mask;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 16) < 16) return 0;

	readpos = readbits + (int)g_BP.BP / 8;

	if (uint shift = (g_BP.BP & 7))
	{
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
	u32 mask;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	if (FillInternalBuffer(&g_BP.BP, 1, 8) < 8)
		return 0;

	readpos = readbits + (int)g_BP.BP / 8;

	if (uint shift = (g_BP.BP & 7))
			{
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

// --------------------------------------------------------------------------------------
//  IPU Worker / Dispatcher
// --------------------------------------------------------------------------------------
void IPUCMD_WRITE(u32 val)
{
	// don't process anything if currently busy
	if (ipuRegs.ctrl.BUSY) Console.WriteLn("IPU BUSY!"); // wait for thread

	ipuRegs.ctrl.ECD = 0;
	ipuRegs.ctrl.SCD = 0; //clear ECD/SCD
	ipu_cmd.clear();
	ipu_cmd.current = val;

	switch (val >> 28)
	{
		case SCE_IPU_BCLR:
			ipuBCLR(val);
			hwIntcIrq(INTC_IPU); //DMAC_TO_IPU
			return;

		case SCE_IPU_VDEC:

			g_BP.BP += val & 0x3F;

			// check if enough data in queue
			if (ipuVDEC(val)) return;

			ipuRegs.cmd.BUSY = 0x80000000;
			ipuRegs.topbusy = 0x80000000;
			break;

		case SCE_IPU_FDEC:
			IPU_LOG("FDEC command. Skip 0x%X bits, FIFO 0x%X qwords, BP 0x%X, FP %d, CHCR 0x%x",
			        val & 0x3f, g_BP.IFC, (int)g_BP.BP, g_BP.FP, ipu1dma.chcr._u32);
			g_BP.BP += val & 0x3F;
			if (ipuFDEC(val)) return;
			ipuRegs.cmd.BUSY = 0x80000000;
			ipuRegs.topbusy = 0x80000000;
			break;

		case SCE_IPU_SETTH:
			ipuSETTH(val);
			hwIntcIrq(INTC_IPU);
			return;

		case SCE_IPU_SETIQ:
			IPU_LOG("SETIQ command.");
			if (val & 0x3f) IPU_LOG("Skip %d bits.", val & 0x3f);
			g_BP.BP += val & 0x3F;
			if (ipuSETIQ(val)) return;
			break;

		case SCE_IPU_SETVQ:
			if (ipuSETVQ(val)) return;
			break;

		case SCE_IPU_CSC:
			ipu_cmd.pos[1] = 0;
			ipu_cmd.index = 0;

			if (ipuCSC(val))
			{
				IPU_INT0_FROM();
				return;
			}
			break;

		case SCE_IPU_PACK:
			ipu_cmd.pos[1] = 0;
			ipu_cmd.index = 0;
			if (ipuPACK(val)) return;
			break;

		case SCE_IPU_IDEC:
			if (ipuIDEC(val, false))
			{
				// idec done, ipu0 done too
				IPU_INT0_FROM();
				return;
			}

			ipuRegs.topbusy = 0x80000000;
			break;

		case SCE_IPU_BDEC:
			if (ipuBDEC(val, false))
			{
				IPU_INT0_FROM();
				if (ipuRegs.ctrl.SCD || ipuRegs.ctrl.ECD) hwIntcIrq(INTC_IPU);
				return;
			}
			else
			{
				ipuRegs.topbusy = 0x80000000;
			}
			break;
	}

	// have to resort to the thread
	ipuRegs.ctrl.BUSY = 1;
	if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
}

void IPUWorker()
{
	pxAssert(ipuRegs.ctrl.BUSY);

	switch (ipu_cmd.CMD)
	{
		case SCE_IPU_VDEC:
			if (!ipuVDEC(ipu_cmd.current))
			{
				if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
				return;
			}
			ipuRegs.cmd.BUSY = 0;
			ipuRegs.topbusy = 0;
			break;

		case SCE_IPU_FDEC:
			if (!ipuFDEC(ipu_cmd.current))
			{
				if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
				return;
			}
			ipuRegs.cmd.BUSY = 0;
			ipuRegs.topbusy = 0;
			break;

		case SCE_IPU_SETIQ:
			if (!ipuSETIQ(ipu_cmd.current))
			{
				if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
				return;
			}
			break;

		case SCE_IPU_SETVQ:
			if (!ipuSETVQ(ipu_cmd.current))
			{
				if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
				return;
			}
			break;

		case SCE_IPU_CSC:
			if (!ipuCSC(ipu_cmd.current))
			{
				if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
				return;
			}
			IPU_INT0_FROM();
			break;

		case SCE_IPU_PACK:
			if (!ipuPACK(ipu_cmd.current))
			{
				if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
				return;
			}
			break;

		case SCE_IPU_IDEC:
			if (!ipuIDEC(ipu_cmd.current, true))
			{
				if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
				return;
			}

			ipuRegs.ctrl.OFC = 0;
			ipuRegs.ctrl.BUSY = 0;
			ipuRegs.topbusy = 0;
			ipuRegs.cmd.BUSY = 0;
			ipu_cmd.current = 0xffffffff;

			// CHECK!: IPU0dma remains when IDEC is done, so we need to clear it
			IPU_INT0_FROM();
			break;

		case SCE_IPU_BDEC:
			if (!ipuBDEC(ipu_cmd.current, true))
			{
				if(ipu1dma.chcr.STR == false) hwIntcIrq(INTC_IPU);
				return;
			}

			ipuRegs.ctrl.BUSY = 0;
			ipuRegs.topbusy = 0;
			ipuRegs.cmd.BUSY = 0;
			ipu_cmd.current = 0xffffffff;

			IPU_INT0_FROM();
			if (ipuRegs.ctrl.SCD || ipuRegs.ctrl.ECD) hwIntcIrq(INTC_IPU);
			return;

		default:
			Console.WriteLn("Unknown IPU command: %08x", ipu_cmd.current);
			break;
	}

	// success
	ipuRegs.ctrl.BUSY = 0;
	ipu_cmd.current = 0xffffffff;
}
