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


#ifndef __DMAC_H__
#define __DMAC_H__

extern u8  *psH; // hw mem

//
// --- DMA ---
//

union tDMA_CHCR {
	struct {
		u32 DIR : 1;
		u32 reserved1 : 1;
		u32 MOD : 2;
		u32 ASP : 2;
		u32 TTE : 1;
		u32 TIE : 1;
		u32 STR : 1;
		u32 reserved2 : 7;
		u32 TAG : 16;
	};
	u32 _u32;
	
	tDMA_CHCR( u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Chcr: 0x%x", _u32); }
};

union tDMA_SADR {
	struct {
		u32 ADDR : 14;
		u32 reserved2 : 18;
	};
	u32 _u32;
	
	tDMA_SADR(u32 val) { _u32 = val; }
	
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Sadr: 0x%x", _u32); }
};

union tDMA_MADR {
	struct {
		u32 ADDR : 31; // Transfer memory address
		u32 SPR : 1; // Memory/SPR Address
	};
	u32 _u32;
	
	tDMA_MADR(u32 val) { _u32 = val; }
	
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Madr: 0x%x", _u32); }
};

union tDMA_TADR {
	struct {
		u32 ADDR : 31; // Next Tag address
		u32 SPR : 1; // Memory/SPR Address
	};
	u32 _u32;
	
	tDMA_TADR(u32 val) { _u32 = val; }
	
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Tadr: 0x%x", _u32); }
};

union tDMA_ASR { // The Address Stack Register
	struct {
		u32 ADDR : 31; // Tag memory address
		u32 SPR : 1; // Memory/SPR Address
	};
	u32 _u32;
	
	tDMA_ASR(u32 val) { _u32 = val; }
	
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Asr: 0x%x", _u32); }
};

union tDMA_QWC {
	struct {
		u32 QWC : 16;
		u32 reserved2 : 16;
	};
	u32 _u32;
	
	tDMA_QWC(u32 val) { _u32 = val; }
	
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"QWC: 0x%x", _u32); }
};

struct DMACh {
	tDMA_CHCR chcr;
	u32 null0[3];
	u32 madr;
	u32 null1[3];
	u16 qwc; u16 pad;
	u32 null2[3];
	u32 tadr;
	u32 null3[3];
	u32 asr0;
	u32 null4[3];
	u32 asr1;
	u32 null5[11];
	u32 sadr;
};


enum INTCIrqs
{
	INTC_GS = 0,
	INTC_SBUS,
	INTC_VBLANK_S,
	INTC_VBLANK_E,
	INTC_VIF0,
	INTC_VIF1,
	INTC_VU0,
	INTC_VU1,
	INTC_IPU,
	INTC_TIM0,
	INTC_TIM1,
	INTC_TIM2,
	INTC_TIM3,
	INTC_SFIFO,
	INTVU0_WD
};

enum dmac_conditions
{
	DMAC_STAT_SIS	= (1<<13),	 // stall condition
	DMAC_STAT_MEIS	= (1<<14),	 // mfifo empty
	DMAC_STAT_BEIS	= (1<<15),	 // bus error
	DMAC_STAT_SIM	= (1<<29),	 // stall mask
	DMAC_STAT_MEIM	= (1<<30)	 // mfifo mask
};

enum DMACIrqs
{
	DMAC_VIF0	= 0,
	DMAC_VIF1,
	DMAC_GIF,
	DMAC_FROM_IPU,
	DMAC_TO_IPU,
	DMAC_SIF0,
	DMAC_SIF1,
	DMAC_SIF2,
	DMAC_FROM_SPR,
	DMAC_TO_SPR,

	// We're setting error conditions through hwDmacIrq, so these correspond to the conditions above.
	DMAC_STALL_SIS		= 13, // SIS
	DMAC_MFIFO_EMPTY	= 14, // MEIS
	DMAC_BUS_ERROR	= 15      // BEIS
};

//DMA interrupts & masks
enum DMAInter
{
	BEISintr = 0x00008000,
	VIF0intr = 0x00010001,
	VIF1intr = 0x00020002,
	GIFintr =  0x00040004,
	IPU0intr = 0x00080008,
	IPU1intr = 0x00100010,
	SIF0intr = 0x00200020,
	SIF1intr = 0x00400040,
	SIF2intr = 0x00800080,
	SPR0intr = 0x01000100,
	SPR1intr = 0x02000200,
	SISintr  = 0x20002000,
	MEISintr = 0x40004000
};

union tDMAC_CTRL {
	struct {
		u32 DMAE : 1;
		u32 RELE : 1;
		u32 MFD : 2;
		u32 STS : 2;
		u32 STD : 2;
		u32 RCYC : 3;
		u32 reserved1 : 21;
	};
	u32 _u32;
	
	tDMAC_CTRL(u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Ctrl: 0x%x", _u32); }
};

union tDMAC_STAT {
	struct {
		u32 CIS : 10;
		u32 reserved1 : 3;
		u32 SIS : 1;
		u32 MEIS : 1;
		u32 BEIS : 1;
		u32 CIM : 10;
		u32 reserved2 : 3;
		u32 SIM : 1;
		u32 MEIM : 1;
		u32 reserved3 : 1;
	};
	u32 _u32;
	
	tDMAC_STAT(u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Stat: 0x%x", _u32); }
};

union tDMAC_PCR {
	struct {
		u32 CPC : 10;
		u32 reserved1 : 6;
		u32 CDE : 10;
		u32 reserved2 : 5;
		u32 PCE : 1;
	};
	u32 _u32;
	
	tDMAC_PCR(u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Pcr: 0x%x", _u32); }
};

union tDMAC_SQWC {
	struct {
		u32 SQWC : 8;
		u32 reserved1 : 8;
		u32 TQWC : 8;
		u32 reserved2 : 8;
	};
	u32 _u32;
	
	tDMAC_SQWC(u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Sqwc: 0x%x", _u32); }
};

union tDMAC_RBSR {
	struct {
		u32 RMSK : 31;
		u32 reserved1 : 1;
	};
	u32 _u32;
	
	tDMAC_RBSR(u32 val) { _u32 = val; }
	
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Rbsr: 0x%x", _u32); }
};

union tDMAC_RBOR {
	struct {
		u32 ADDR : 31;
		u32 reserved1 : 1;
	};
	u32 _u32;
	
	tDMAC_RBOR(u32 val) { _u32 = val; }
	
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Rbor: 0x%x", _u32); }
};

union tDMAC_STADR {
	struct {
		u32 ADDR : 31;
		u32 reserved1 : 1;
	};
	u32 _u32;
	
	tDMAC_STADR(u32 val) { _u32 = val; }
	
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Stadr: 0x%x", _u32); }
};

struct DMACregisters
{
	tDMAC_CTRL	ctrl;
	u32 padding[3];
	tDMAC_STAT	stat;
	u32 padding1[3];
	tDMAC_PCR	pcr;
	u32 padding2[3];

	tDMAC_SQWC	sqwc;
	u32 padding3[3];
	tDMAC_RBSR	rbsr;
	u32 padding4[3];
	tDMAC_RBOR	rbor;
	u32 padding5[3];
	tDMAC_STADR	stadr;
};

// Currently guesswork.
union tINTC_STAT {
	struct {
		u32 interrupts : 10;
	    u32 placeholder : 22;
	};
	u32 _u32;
	
	tINTC_STAT(u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Stat: 0x%x", _u32); }
};

union tINTC_MASK {
	struct {
	    u32 int_mask : 10;
	    u32 placeholder:22;
	};
	u32 _u32;
	
	tINTC_MASK(u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	wxString desc() { return wxsFormat(L"Mask: 0x%x", _u32); }
};

struct INTCregisters
{
	tINTC_STAT  stat;
	u32 padding[3];
	tINTC_MASK  mask;
};
    
#define dmacRegs ((DMACregisters*)(PS2MEM_HW+0xE000))
#define intcRegs ((INTCregisters*)(PS2MEM_HW+0xF000))

// Note: Dma addresses are guaranteed to be aligned to 16 bytes (128 bits)
static __forceinline void *dmaGetAddr(u32 addr) {
	u8 *ptr;

//	if (addr & 0xf) { DMA_LOG("*PCSX2*: DMA address not 128bit aligned: %8.8x", addr); }

	//  Need to check the physical address as well as just the "SPR" flag, as VTLB doesnt seem to handle it
	if ((addr & 0x80000000) || (addr & 0x70000000) == 0x70000000) return (void*)&psS[addr & 0x3ff0];

	ptr = (u8*)vtlb_GetPhyPtr(addr&0x1FFFFFF0);
	if (ptr == NULL) {
		Console.Error( "*PCSX2*: DMA error: %8.8x", addr);
		return NULL;
	}
	return ptr;
}

static __forceinline u32 *_dmaGetAddr(DMACh *dma, u32 addr, u32 num)
{
	u32 *ptr = (u32*)dmaGetAddr(addr);
	if (ptr == NULL)
	{
		// DMA Error
		dmacRegs->stat.BEIS = true; // BUS Error

		// DMA End
		dmacRegs->stat.set_flags(1 << num);
		dma->chcr.STR = false;
	}

	return ptr;
}

void hwIntcIrq(int n);
void hwDmacIrq(int n);

bool hwDmacSrcChainWithStack(DMACh *dma, int id);
bool hwDmacSrcChain(DMACh *dma, int id);

extern void intcInterrupt();
extern void dmacInterrupt();

#endif


// Everything after this comment is obsolete, and can be ignored.
#ifdef PCSX2_VIRTUAL_MEM

#define dmaGetAddrBase(addr) (((addr) & 0x80000000) ? (void*)&PS2MEM_SCRATCH[(addr) & 0x3ff0] : (void*)(PS2MEM_BASE+TRANSFORM_ADDR(addr)))

#ifdef _WIN32
extern PSMEMORYMAP* memLUT;
#endif

// VM-version of dmaGetAddr -- Left in for references purposes for now (air)
static __forceinline u8* dmaGetAddr(u32 mem)
{
	u8* p, *pbase;
	mem &= ~0xf;

	if( (mem&0xffff0000) == 0x50000000 ) {// reserved scratch pad mem
		Console.WriteLn("dmaGetAddr: reserved scratch pad mem");
		return NULL;//(u8*)&PS2MEM_SCRATCH[(mem) & 0x3ff0];
	}

	p = (u8*)dmaGetAddrBase(mem);

#ifdef _WIN32
	// do manual LUT since IPU/SPR seems to use addrs 0x3000xxxx quite often
	// linux doesn't suffer from this because it has better vm support
	if( memLUT[ (p-PS2MEM_BASE)>>12 ].aPFNs == NULL ) {
		Console.WriteLn("dmaGetAddr: memLUT PFN warning");
		return NULL;//p;
	}

	pbase = (u8*)memLUT[ (p-PS2MEM_BASE)>>12 ].aVFNs[0];
	if( pbase != NULL ) p = pbase + ((u32)p&0xfff);
#endif

	return p;
}
#endif

