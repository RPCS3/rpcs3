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
#include "GS.h"
#include "Gif.h"
#include "Vif_Dma.h"
#include "Vif.h"

// --------------------------------------------------------------------------------------
//  GIFpath -- the GIFtag Parser
// --------------------------------------------------------------------------------------

struct GSRegSIGBLID
{
	u32 SIGID;
	u32 LBLID;
};

enum GIF_FLG
{
	GIF_FLG_PACKED	= 0,
	GIF_FLG_REGLIST	= 1,
	GIF_FLG_IMAGE	= 2,
	GIF_FLG_IMAGE2	= 3
};

enum GIF_REG
{
	GIF_REG_PRIM	= 0x00,
	GIF_REG_RGBA	= 0x01,
	GIF_REG_STQ		= 0x02,
	GIF_REG_UV		= 0x03,
	GIF_REG_XYZF2	= 0x04,
	GIF_REG_XYZ2	= 0x05,
	GIF_REG_TEX0_1	= 0x06,
	GIF_REG_TEX0_2	= 0x07,
	GIF_REG_CLAMP_1	= 0x08,
	GIF_REG_CLAMP_2	= 0x09,
	GIF_REG_FOG		= 0x0a,
	GIF_REG_XYZF3	= 0x0c,
	GIF_REG_XYZ3	= 0x0d,
	GIF_REG_A_D		= 0x0e,
	GIF_REG_NOP		= 0x0f,
};

// GIFTAG
// Members of this structure are in CAPS to help visually denote that they are representative
// of actual hw register states of the GIF, unlike the internal tracking vars in GIFPath, which
// are modified during the GIFtag unpacking process.
struct GIFTAG
{
	u32 NLOOP	: 15;
	u32 EOP		: 1;
	u32 dummy0	: 16;
	u32 dummy1	: 14;
	u32 PRE		: 1;
	u32 PRIM	: 11;
	u32 FLG		: 2;
	u32 NREG	: 4;
	u32 REGS[2];

	GIFTAG() {}
};

// --------------------------------------------------------------------------------------
//  GIFPath -- PS2 GIFtag info (one for each path).
// --------------------------------------------------------------------------------------
// fixme: The real PS2 has a single internal PATH and 3 logical sources, not 3 entirely
// separate paths.  But for that to work properly we need also interlocked path sources.
// That is, when the GIF selects a source, it sticks to that source until an EOP.  Currently
// this is not emulated!

struct GIFPath
{
	const GIFTAG tag;	// The "original tag -- modification allowed only by SetTag(), so let's make it const.
	u8 regs[16];		// positioned after tag ensures 16-bit aligned (in case we SSE optimize later)

	u32 nloop;			// local copy nloop counts toward zero, and leaves the tag copy unmodified.
	u32 curreg;			// reg we left of on (for traversing through loops)
	u32 numregs;		// number of regs (when NREG is 0, numregs is 16)

	GIFPath();

	void PrepPackedRegs();
	void SetTag(const void* mem);
	bool StepReg();
	u8 GetReg();

	int ParseTag(GIF_PATH pathidx, const u8* pMem, u32 size);
};

typedef void (__fastcall *GIFRegHandler)(const u32* data);

struct GifPathStruct
{
	const GIFRegHandler	Handlers[0x100-0x60];		// handlers for 0x60->0x100
	GIFPath				path[3];

	__forceinline GIFPath& operator[]( int idx ) { return path[idx]; }
};

// --------------------------------------------------------------------------------------
//  SIGNAL / FINISH / LABEL   (WIP!!)
// --------------------------------------------------------------------------------------
// The current implementation for these is very incomplete, especially SIGNAL, which needs
// an extra VM-state status var to be handled correctly.
//

// SIGNAL : This register is a double-throw.  If the SIGNAL bit in CSR is clear, set the CSR
//   and raise a gsIrq.  If CSR is already *set*, then ignore all subsequent drawing operations
//   and writes to general purpose registers to the GS. (note: I'm pretty sure this includes
//   direct GS and GSreg accesses, as well as those coming through the GIFpath -- but that
//   behavior isn't confirmed yet).  Privileged writes are still active.
//
//   Ignorance continues until the SIGNAL bit in CSR is manually cleared by the EE.  And here's
//   the tricky part: the interrupt from the second SIGNAL is still pending, and should be
//   raised once the EE has reset the *IMR* mask for SIGNAL -- meaning setting the bit to 1
//   (disabled/masked) and then back to 0 (enabled/unmasked).
//
static void __fastcall RegHandlerSIGNAL(const u32* data)
{
	GIF_LOG("MTGS SIGNAL data %x_%x CSRw %x IMR %x CSRr\n",data[0], data[1], CSRw, GSIMR, GSCSRr);

	GSSIGLBLID.SIGID = (GSSIGLBLID.SIGID&~data[1])|(data[0]&data[1]);

	if ((CSRw & 0x1))
	{
		if (!(GSIMR&0x100) )
		{
			gsIrq();
		}

		GSCSRr |= 1; // signal
	}
}

// FINISH : Enables end-of-draw signaling.  When FINISH is written it tells the GIF to
//   raise a gsIrq and set the FINISH bit of CSR when the current operation is finished.
//   As far as I can figure, this feature is meant for EE/GS synchronization when the EE
//   wants to utilize GS post-processing effects.  We don't need to emulate that part of
//   it since we flush/interlock the GS for those specific read operations.
//
//   However!  We should properly emulate handling partial-DMA transfers on PATH2 and
//   PATH3 of the GIF, which means only signaling FINISH if nloop==0.
//
static void __fastcall RegHandlerFINISH(const u32* data)
{
	GIF_LOG("GIFpath FINISH data %x_%x CSRw %x\n", data[0], data[1], CSRw);

	if ((CSRw & 0x2))
	{
		if (!(GSIMR&0x200))
			gsIrq();

		GSCSRr |= 2; // finish
	}
}

static void __fastcall RegHandlerLABEL(const u32* data)
{
	GIF_LOG( "GIFpath LABEL" );
	GSSIGLBLID.LBLID = (GSSIGLBLID.LBLID&~data[1])|(data[0]&data[1]);
}

static void __fastcall RegHandlerUNMAPPED(const u32* data)
{
	const int regidx = ((u8*)data)[8];

	// Known "unknowns":
	//  It's possible that anything above 0x63 should just be silently ignored, but in the
	//  offhand chance not, I'm documenting known cases of unknown register use here.
	//
	//  0x7F -->
	//   the bios likes to write to 0x7f using an EOP giftag with NLOOP set to 4.
	//   Not sure what it's trying to accomplish exactly.  Ignoring seems to work fine,
	//   and is probably the intended behavior (it's likely meant to be a NOP).
	//
	//  0xEE -->
	//   .hack Infection [PAL confirmed, NTSC unknown] uses 0xee when you zoom the camera.
	//   The use hasn't been researched yet so parameters are unknown.  Everything seems
	//   to work fine as usual -- The 0xEE address in common programming terms is typically
	//   left over uninitialized data, and this might be a case of that, which is to be
	//   silently ignored.
	//
	//  Guitar Hero 3+ : Massive spamming when using superVU (along with several VIF errors)
	//  Using microVU avoids the GIFtag errors, so probably just one of sVU's hacks conflicting
	//  with one of VIF's hacks, and causing corrupted packet data.

	if( regidx != 0x7f && regidx != 0xee )
		DbgCon.Warning( "Ignoring Unmapped GIFtag Register, Index = %02x", regidx );
}

#define INSERT_UNMAPPED_4	RegHandlerUNMAPPED, RegHandlerUNMAPPED, RegHandlerUNMAPPED, RegHandlerUNMAPPED,
#define INSERT_UNMAPPED_16	INSERT_UNMAPPED_4 INSERT_UNMAPPED_4 INSERT_UNMAPPED_4 INSERT_UNMAPPED_4
#define INSERT_UNMAPPED_64	INSERT_UNMAPPED_16 INSERT_UNMAPPED_16 INSERT_UNMAPPED_16 INSERT_UNMAPPED_16

static __aligned16 GifPathStruct s_gifPath =
{
	RegHandlerSIGNAL, RegHandlerFINISH, RegHandlerLABEL, RegHandlerUNMAPPED,

	// Rest are mapped to Unmapped
	INSERT_UNMAPPED_4  INSERT_UNMAPPED_4  INSERT_UNMAPPED_4
	INSERT_UNMAPPED_64 INSERT_UNMAPPED_64 INSERT_UNMAPPED_16
};

// --------------------------------------------------------------------------------------
//  GIFPath Method Implementations
// --------------------------------------------------------------------------------------

GIFPath::GIFPath() : tag()
{
	memzero( *this );
}

__forceinline bool GIFPath::StepReg()
{
	if ((++curreg & 0xf) == tag.NREG) {
		curreg = 0;
		if (--nloop == 0) {
			return false;
		}
	}
	return true;
}

__forceinline u8 GIFPath::GetReg() { return regs[curreg]; }

// Unpack the registers - registers are stored as a sequence of 4 bit values in the
// upper 64 bits of the GIFTAG.  That sucks for us when handling partialized GIF packets
// coming in from paths 2 and 3, so we unpack them into an 8 bit array here.
//
__forceinline void GIFPath::PrepPackedRegs()
{
	// Only unpack registers if we're starting a new pack.  Otherwise the unpacked
	// array should have already been initialized by a previous partial transfer.

	if (curreg != 0) return;

	u32 tempreg = tag.REGS[0];
	numregs		= ((tag.NREG-1)&0xf) + 1;

	for (u32 i = 0; i < numregs; i++) {
		if (i == 8) tempreg = tag.REGS[1];
		regs[i] = tempreg & 0xf;
		tempreg >>= 4;
	}
}

__forceinline void GIFPath::SetTag(const void* mem)
{
	const_cast<GIFTAG&>(tag) = *((GIFTAG*)mem);

	nloop	= tag.NLOOP;
	curreg	= 0;
}

void SaveStateBase::gifPathFreeze()
{
	FreezeTag( "GIFpath" );
	Freeze( s_gifPath.path );
}


static __forceinline void gsHandler(const u8* pMem)
{
	const int reg = pMem[8];

	if (reg == 0x50)
		vif1.BITBLTBUF._u64 = *(u64*)pMem;
	else if (reg == 0x52)
		vif1.TRXREG._u64 = *(u64*)pMem;
	else if (reg == 0x53)
	{
		// local -> host
		if ((pMem[0] & 3) == 1)
		{
			//Onimusha does TRXREG without BLTDIVIDE first, so we "assume" 32bit for this equation, probably isnt important.
			// ^ WTF, seriously? This is really important (pseudonym)
			u8 bpp = 32;

			switch(vif1.BITBLTBUF.SPSM & 7)
			{
			case 0:
				bpp = 32;
				break;
			case 1:
				bpp = 24;
				break;
			case 2:
				bpp = 16;
				break;
			case 3:
				bpp = 8;
				break;
			// 4 is 4 bit but this is forbidden
			default:
				Console.Error("Illegal format for GS upload: SPSM=0%02o", vif1.BITBLTBUF.SPSM);
			}

			VIF_LOG("GS Download %dx%d SPSM= bpp=%d", vif1.TRXREG.RRW, vif1.TRXREG.RRH, vif1.BITBLTBUF.SPSM, bpp);

			// qwords, rounded down; any extra bits are lost
			// games must take care to ensure transfer rectangles are exact multiples of a qword
			vif1.GSLastDownloadSize = vif1.TRXREG.RRW * vif1.TRXREG.RRH * bpp >> 7;

			gifRegs->stat.OPH = true;
		}
	}
	if (reg >= 0x60)
	{
		// Question: What happens if an app writes to uncharted register space on real PS2
		// hardware (handler 0x63 and higher)?  Probably a silent ignorance, but not tested
		// so just guessing... --air

		s_gifPath.Handlers[reg-0x60]((const u32*)pMem);
	}
}

#define incTag(x, y) do {				\
	pMem += (x);						\
	size -= (y);						\
	if (pMem>=vuMemEnd) pMem -= 0x4000;	\
} while(false)

#define aMin(x, y) std::min(x, y)

// Parameters:
//   size (path1)   - difference between the end of VU memory and pMem.
//   size (path2/3) - max size of incoming data stream, in qwc (simd128)
__forceinline int GIFPath::ParseTag(GIF_PATH pathidx, const u8* pMem, u32 size)
{
	const u8*	vuMemEnd  =  pMem + (size<<4);	// End of VU1 Mem
	if (pathidx==GIF_PATH_1) size = 0x400;		// VU1 mem size
	u32	startSize =  size;						// Start Size

	while (size > 0) {
		if (!nloop) {

			SetTag(pMem);
			incTag(16, 1);

			//if (pathidx == GIF_PATH_3) {
			switch(pathidx)
			{
				case GIF_PATH_1:
					if (tag.FLG&2)	GSTransferStatus.PTH1 = IMAGE_MODE;
					else			GSTransferStatus.PTH1 = TRANSFER_MODE;
					break;
				case GIF_PATH_2:
					if (tag.FLG&2)	GSTransferStatus.PTH2 = IMAGE_MODE;
					else			GSTransferStatus.PTH2 = TRANSFER_MODE;
					break;
				case GIF_PATH_3:
					if (tag.FLG&2)	GSTransferStatus.PTH3 = IMAGE_MODE;
					else			GSTransferStatus.PTH3 = TRANSFER_MODE;
					break;
			}
			//}
		}
		else
		{
			switch(tag.FLG) {
				case GIF_FLG_PACKED:
					GIF_LOG("Packed Mode");
					PrepPackedRegs();
					do {
						if (GetReg() == 0xe) {
							gsHandler(pMem);
						}
						incTag(16, 1);
					} while(StepReg() && size > 0);
				break;
				case GIF_FLG_REGLIST:
				{
					GIF_LOG("Reglist Mode");
					size *= 2;

					do { incTag(8, 1); }
					while(StepReg() && size > 0);

					if (size & 1) { incTag(8, 1); }
					size /= 2;
				}
				break;
				case GIF_FLG_IMAGE:
				case GIF_FLG_IMAGE2:
				{
					GIF_LOG("IMAGE Mode");
					int len = aMin(size, nloop);
					incTag(( len * 16 ), len);
					nloop -= len;
				}
				break;
			}
		}
		if(pathidx == GIF_PATH_1)
		{
			if(nloop > 0 && size == 0 && !tag.EOP) //Need to check all of this, some cases VU will send info (like the BIOS) but be incomplete
			{
				switch(tag.FLG)
				{
					case GIF_FLG_PACKED:
						size = nloop * numregs;
					break;

					case GIF_FLG_REGLIST:
						size = (nloop * numregs) / 2;
					break;

					default:
						size = nloop;
					break;
				}
				startSize += size;
				if(startSize >= 0x3fff)
				{
					size = 0;
					Console.Warning("GIFTAG error, size exceeded VU memory size");
				}
			}
		}
		if (tag.EOP && !nloop) {
			if (pathidx != GIF_PATH_2) {
				break;
			}
		}
	}
	/*if(((GSTransferStatus.PTH1 & 0x2) + (GSTransferStatus.PTH2 & 0x2) + ((GSTransferStatus.PTH3 & 0x2) && !vif1Regs->mskpath3)) < 0x4 )
		Console.Warning("CHK PTH1 %x, PTH2 %x, PTH3 %x", GSTransferStatus.PTH1, GSTransferStatus.PTH2, GSTransferStatus.PTH3);*/

	size = (startSize - size);


		if (tag.EOP && !nloop) {
			//Console.Warning("Finishing path %x", pathidx);
			switch(pathidx)
			{
				case GIF_PATH_1:
					GSTransferStatus.PTH1 = STOPPED_MODE;
					break;
				case GIF_PATH_2:
					GSTransferStatus.PTH2 = STOPPED_MODE;
					break;
				case GIF_PATH_3:
					GSTransferStatus.PTH3 = STOPPED_MODE;
					break;
			}
		}
	if (pathidx == GIF_PATH_3 && gif->chcr.STR) { //Make sure we are really doing a DMA and not using FIFO
		//GIF_LOG("Path3 end EOP %x NLOOP %x Status %x", tag.EOP, nloop, GSTransferStatus.PTH3);
		gif->madr += size * 16;
		gif->qwc  -= size;
	} else if (pathidx == GIF_PATH_2 && !nloop) { //Path2 is odd, but always provides the correct size
		GSTransferStatus.PTH2 = STOPPED_MODE;
	}

	return size;
}

// Processes a GIFtag & packet, and throws out some gsIRQs as needed.
// Used to keep interrupts in sync with the EE, while the GS itself
// runs potentially several frames behind.
// Parameters:
//   size  - max size of incoming data stream, in qwc (simd128)
__forceinline int GIFPath_ParseTag(GIF_PATH pathidx, const u8* pMem, u32 size)
{
#ifdef PCSX2_GSRING_SAMPLING_STATS
	static uptr profStartPtr = 0;
	static uptr profEndPtr = 0;
	if (profStartPtr == 0) {
		__asm
		{
		__beginfunc:
			mov profStartPtr, offset __beginfunc;
			mov profEndPtr, offset __endfunc;
		}
		ProfilerRegisterSource( "GSRingBufCopy", (void*)profStartPtr, profEndPtr - profStartPtr );
	}
#endif

	int retSize = s_gifPath[pathidx].ParseTag(pathidx, pMem, size);

#ifdef PCSX2_GSRING_SAMPLING_STATS
	__asm
	{
		__endfunc:
			nop;
	}
#endif
	return retSize;
}

// Clears all GIFpath data to zero.
void GIFPath_Reset()
{
	memzero( s_gifPath.path );
}

// This is a hackfix tool provided for "canceling" the contents of the GIFpath when
// invalid GIFdma states are encountered (typically needed for PATH3 only).
__forceinline void GIFPath_Clear( GIF_PATH pathidx )
{
	memzero(s_gifPath.path[pathidx]);
	if( GSgifSoftReset == NULL ) return;
	GetMTGS().SendSimplePacket( GS_RINGTYPE_SOFTRESET, (1<<pathidx), 0, 0 );
}
