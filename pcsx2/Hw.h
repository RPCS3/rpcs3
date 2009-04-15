/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __HW_H__
#define __HW_H__

extern u8  *psH; // hw mem

extern void CPU_INT( u32 n, s32 ecycle );

//////////////////////////////////////////////////////////////////////////
// Hardware FIFOs (128 bit access only!)
//
// VIF0   -- 0x10004000 -- PS2MEM_HW[0x4000]
// VIF1   -- 0x10005000 -- PS2MEM_HW[0x5000]
// GIF    -- 0x10006000 -- PS2MEM_HW[0x6000]
// IPUout -- 0x10007000 -- PS2MEM_HW[0x7000]
// IPUin  -- 0x10007010 -- PS2MEM_HW[0x7010]

void __fastcall ReadFIFO_page_4(u32 mem, mem128_t *out);
void __fastcall ReadFIFO_page_5(u32 mem, mem128_t *out);
void __fastcall ReadFIFO_page_6(u32 mem, mem128_t *out);
void __fastcall ReadFIFO_page_7(u32 mem, mem128_t *out);

void __fastcall WriteFIFO_page_4(u32 mem, const mem128_t *value);
void __fastcall WriteFIFO_page_5(u32 mem, const mem128_t *value);
void __fastcall WriteFIFO_page_6(u32 mem, const mem128_t *value);
void __fastcall WriteFIFO_page_7(u32 mem, const mem128_t *value);


//
// --- DMA ---
//

struct DMACh {
	u32 chcr;
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

// HW defines
enum HWaddress
{	
	RCNT0_COUNT		=	0x10000000,
	RCNT0_MODE		=	0x10000010,
	RCNT0_TARGET	=	0x10000020,
	RCNT0_HOLD		=	0x10000030,

	RCNT1_COUNT		=	0x10000800,
	RCNT1_MODE		=	0x10000810,
	RCNT1_TARGET	=	0x10000820,
	RCNT1_HOLD		=	0x10000830,

	RCNT2_COUNT		=	0x10001000,
	RCNT2_MODE		=	0x10001010,
	RCNT2_TARGET	=	0x10001020,

	RCNT3_COUNT		=	0x10001800,
	RCNT3_MODE		=	0x10001810,
	RCNT3_TARGET	=	0x10001820,

	IPU_CMD			=	0x10002000,
	IPU_CTRL			=	0x10002010,
	IPU_BP			=	0x10002020,
	IPU_TOP			=	0x10002030,

	GIF_CTRL			=	0x10003000,
	GIF_MODE		=	0x10003010,
	GIF_STAT			=	0x10003020,
	GIF_TAG0			=	0x10003040,
	GIF_TAG1			=	0x10003050,
	GIF_TAG2			=	0x10003060,
	GIF_TAG3			=	0x10003070,
	GIF_CNT			=	0x10003080,
	GIF_P3CNT		=	0x10003090,
	GIF_P3TAG		=	0x100030A0,
	GIF_FIFO			=	0x10006000,

	IPUout_FIFO		=	0x10007000,
	IPUin_FIFO		=	0x10007010,

//VIF0
	D0_CHCR			=	0x10008000,
	D0_MADR			=	0x10008010,
	D0_QWC			=	0x10008020,

//VIF1
	D1_CHCR			=	0x10009000,
	D1_MADR			=	0x10009010,
	D1_QWC			=	0x10009020,
	D1_TADR			=	0x10009030,
	D1_ASR0			=	0x10009040,
	D1_ASR1			=	0x10009050,
	D1_SADR			=	0x10009080,

//GS
	D2_CHCR			=	0x1000A000,
	D2_MADR			=	0x1000A010,
	D2_QWC			=	0x1000A020,
	D2_TADR			=	0x1000A030,
	D2_ASR0			=	0x1000A040,
	D2_ASR1			=	0x1000A050,
	D2_SADR			=	0x1000A080,

//fromIPU
	D3_CHCR			=	0x1000B000,
	D3_MADR			=	0x1000B010,
	D3_QWC			=	0x1000B020,
	D3_TADR			=	0x1000B030,
	D3_SADR			=	0x1000B080,

//toIPU
	D4_CHCR			=	0x1000B400,
	D4_MADR			=	0x1000B410,
	D4_QWC			=	0x1000B420,
	D4_TADR			=	0x1000B430,
	D4_SADR			=	0x1000B480,

//SIF0
	D5_CHCR			=	0x1000C000,
	D5_MADR			=	0x1000C010,
	D5_QWC			=	0x1000C020,

//SIF1
	D6_CHCR			=	0x1000C400,
	D6_MADR			=	0x1000C410,
	D6_QWC			=	0x1000C420,

//SIF2
	D7_CHCR			=	0x1000C800,
	D7_MADR			=	0x1000C810,
	D7_QWC			=	0x1000C820,

//fromSPR
	D8_CHCR			=	0x1000D000,
	D8_MADR			=	0x1000D010,
	D8_QWC			=	0x1000D020,
	D8_SADR			=	0x1000D080,

	DMAC_CTRL		=	0x1000E000,
	DMAC_STAT		=	0x1000E010,
	DMAC_PCR		=	0x1000E020,
	DMAC_SQWC		=	0x1000E030,
	DMAC_RBSR		=	0x1000E040,
	DMAC_RBOR		=	0x1000E050,
	DMAC_STADR		=	0x1000E060,

	INTC_STAT		=	0x1000F000,
	INTC_MASK		=	0x1000F010,

	SBUS_F220		=	0x1000F220,
	SBUS_SMFLG		=	0x1000F230,
	SBUS_F240		=	0x1000F240,

	DMAC_ENABLER	=	0x1000F520,
	DMAC_ENABLEW	=	0x1000F590,

	GS_PMODE		=	0x12000000,
	GS_SMODE1		=	0x12000010,
	GS_SMODE2		=	0x12000020,
	GS_SRFSH		=	0x12000030,
	GS_SYNCH1		=	0x12000040,
	GS_SYNCH2		=	0x12000050,
	GS_SYNCV		=	0x12000060,
	GS_DISPFB1		=	0x12000070,
	GS_DISPLAY1		=	0x12000080,
	GS_DISPFB2		=	0x12000090,
	GS_DISPLAY2		=	0x120000A0,
	GS_EXTBUF		=	0x120000B0,
	GS_EXTDATA		=	0x120000C0,
	GS_EXTWRITE		=	0x120000D0,
	GS_BGCOLOR		=	0x120000E0,
	GS_CSR			=	0x12001000,
	GS_IMR			=	0x12001010,
	GS_BUSDIR		=	0x12001040,
	GS_SIGLBLID		=	0x12001080
};

#define SBFLG_IOPALIVE 0x10000
#define SBFLG_IOPSYNC 0x40000

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
};
	
#define DMAC_STAT_SIS (1<<13) // stall condition
#define DMAC_STAT_MEIS (1<<14) // mfifo empty
#define DMAC_STAT_BEIS (1<<15) // bus error
#define DMAC_STAT_SIM (1<<29) // stall mask
#define DMAC_STAT_MEIM (1<<30) // mfifo mask

enum DMACIrqs
{
	DMAC_VIF0 = 0,
	DMAC_VIF1,
	DMAC_GIF,
	DMAC_FROM_IPU,
	DMAC_TO_IPU,
	DMAC_SIF0,
	DMAC_SIF1,
	DMAC_SIF2,
	DMAC_FROM_SPR,
	DMAC_TO_SPR,
	DMAC_ERROR = 15,
};

#define VIF0_STAT_VPS_W (1)
#define VIF0_STAT_VPS_D (2)
#define VIF0_STAT_VPS_T (3)
#define VIF0_STAT_VPS	(3)
#define VIF0_STAT_VEW	(1<<2)
#define VIF0_STAT_MRK	(1<<6)
#define VIF0_STAT_DBF	(1<<7)
#define VIF0_STAT_VSS	(1<<8)
#define VIF0_STAT_VFS	(1<<9)
#define VIF0_STAT_VIS	(1<<10)
#define VIF0_STAT_INT	(1<<11)
#define VIF0_STAT_ER0	(1<<12)
#define VIF0_STAT_ER1	(1<<13)

#define VIF1_STAT_VPS_W (1)
#define VIF1_STAT_VPS_D (2)
#define VIF1_STAT_VPS_T (3)
#define VIF1_STAT_VPS	(3)
#define VIF1_STAT_VEW	(1<<2)
#define VIF1_STAT_VGW	(1<<3)
#define VIF1_STAT_MRK	(1<<6)
#define VIF1_STAT_DBF	(1<<7)
#define VIF1_STAT_VSS	(1<<8)
#define VIF1_STAT_VFS	(1<<9)
#define VIF1_STAT_VIS	(1<<10)
#define VIF1_STAT_INT	(1<<11)
#define VIF1_STAT_ER0	(1<<12)
#define VIF1_STAT_ER1	(1<<13)
#define VIF1_STAT_FDR	(1<<23)

#define VIF_STAT_VPS_W (1)
#define VIF_STAT_VPS_D (2)
#define VIF_STAT_VPS_T (3)
#define VIF_STAT_VPS	(3)
#define VIF_STAT_VEW	(1<<2)
#define VIF_STAT_VGW	(1<<3)
#define VIF_STAT_MRK	(1<<6)
#define VIF_STAT_DBF	(1<<7)
#define VIF_STAT_VSS	(1<<8)
#define VIF_STAT_VFS	(1<<9)
#define VIF_STAT_VIS	(1<<10)
#define VIF_STAT_INT	(1<<11)
#define VIF_STAT_ER0	(1<<12)
#define VIF_STAT_ER1	(1<<13)
#define VIF_STAT_FDR	(1<<23)

//DMA interrupts & masks
enum DMAInter
{	
	BEISintr = 0x8000,
	VIF0intr = 0x10001,
	VIF1intr = 0x20002,
	GIFintr = 0x40004,
	IPU0intr = 0x80008,
	IPU1intr = 0x100010,
	SIF0intr = 0x200020,
	SIF1intr  =0x400040,
	SIF2intr = 0x800080,
	SPR0intr = 0x1000100,
	SPR1intr = 0x2000200,
	SISintr  = 0x20002000,
	MEISintr = 0x40004000
};

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
		Console::WriteLn("dmaGetAddr: reserved scratch pad mem");
		return NULL;//(u8*)&PS2MEM_SCRATCH[(mem) & 0x3ff0];
	}

	p = (u8*)dmaGetAddrBase(mem);

#ifdef _WIN32	
	// do manual LUT since IPU/SPR seems to use addrs 0x3000xxxx quite often
	// linux doesn't suffer from this because it has better vm support
	if( memLUT[ (p-PS2MEM_BASE)>>12 ].aPFNs == NULL ) {
		Console::WriteLn("dmaGetAddr: memLUT PFN warning");
		return NULL;//p;
	}

	pbase = (u8*)memLUT[ (p-PS2MEM_BASE)>>12 ].aVFNs[0];
	if( pbase != NULL ) p = pbase + ((u32)p&0xfff);
#endif

	return p;
}

#else

// Note: Dma addresses are guaranteed to be aligned to 16 bytes (128 bits)
static __forceinline void *dmaGetAddr(u32 addr) {
	u8 *ptr;

//	if (addr & 0xf) { DMA_LOG("*PCSX2*: DMA address not 128bit aligned: %8.8x", addr); }
	
	//  teh sux why the f00k 0xE0000000
	if (addr & 0x80000000) return (void*)&psS[addr & 0x3ff0];

	ptr = (u8*)vtlb_GetPhyPtr(addr&0x1FFFFFF0);
	if (ptr == NULL) {
		Console::Error("*PCSX2*: DMA error: %8.8x", params addr);
		return NULL;
	}
	return ptr;
}

#endif 

static __forceinline u32 *_dmaGetAddr(DMACh *dma, u32 addr, u32 num) 
{
	u32 *ptr = (u32*)dmaGetAddr(addr); 
	if (ptr == NULL)  
	{
		// DMA Error
		psHu32(DMAC_STAT) |= DMAC_STAT_BEIS; /* BUS error */
	
		// DMA End
		psHu32(DMAC_STAT) |= 1<<num;  
		dma->chcr &= ~0x100; 
	}
	
	return ptr;
}

void hwInit();
void hwReset();
void hwShutdown();

// hw read functions
extern u8   hwRead8 (u32 mem);
extern u16  hwRead16(u32 mem);

extern mem32_t __fastcall hwRead32_page_00(u32 mem);
extern mem32_t __fastcall hwRead32_page_01(u32 mem);
extern mem32_t __fastcall hwRead32_page_02(u32 mem);
extern mem32_t __fastcall hwRead32_page_0F(u32 mem);
extern mem32_t __fastcall hwRead32_page_0F_INTC_HACK(u32 mem);
extern mem32_t __fastcall hwRead32_generic(u32 mem);

extern void __fastcall hwRead64_page_00(u32 mem, mem64_t* result );
extern void __fastcall hwRead64_page_01(u32 mem, mem64_t* result );
extern void __fastcall hwRead64_page_02(u32 mem, mem64_t* result );
extern void __fastcall hwRead64_generic_INTC_HACK(u32 mem, mem64_t *out);
extern void __fastcall hwRead64_generic(u32 mem, mem64_t* result );

extern void __fastcall hwRead128_page_00(u32 mem, mem128_t* result );
extern void __fastcall hwRead128_page_01(u32 mem, mem128_t* result );
extern void __fastcall hwRead128_page_02(u32 mem, mem128_t* result );
extern void __fastcall hwRead128_generic(u32 mem, mem128_t *out);

// hw write functions
extern void hwWrite8 (u32 mem, u8  value);
extern void hwWrite16(u32 mem, u16 value);

extern void __fastcall hwWrite32_page_00( u32 mem, u32 value );
extern void __fastcall hwWrite32_page_01( u32 mem, u32 value );
extern void __fastcall hwWrite32_page_02( u32 mem, u32 value );
extern void __fastcall hwWrite32_page_03( u32 mem, u32 value );
extern void __fastcall hwWrite32_page_0B( u32 mem, u32 value );
extern void __fastcall hwWrite32_page_0E( u32 mem, u32 value );
extern void __fastcall hwWrite32_page_0F( u32 mem, u32 value );
extern void __fastcall hwWrite32_generic( u32 mem, u32 value );

extern void __fastcall hwWrite64_page_02( u32 mem, const mem64_t* srcval );
extern void __fastcall hwWrite64_page_03( u32 mem, const mem64_t* srcval );
extern void __fastcall hwWrite64_page_0E( u32 mem, const mem64_t* srcval );
extern void __fastcall hwWrite64_generic( u32 mem, const mem64_t* srcval );

extern void __fastcall hwWrite128_generic(u32 mem, const mem128_t *srcval);

// legacy - used for debugging sometimes
//extern mem32_t __fastcall hwRead32(u32 mem);
//extern void __fastcall hwWrite32(u32 mem, u32 value);

//extern void hwWrite64(u32 mem, u64 value);
//extern void hwWrite128(u32 mem, const u64 *value);

void hwIntcIrq(int n);
void hwDmacIrq(int n);

int  hwMFIFORead(u32 addr, u8 *data, u32 size);
int  hwMFIFOWrite(u32 addr, u8 *data, u32 size);

bool  hwDmacSrcChainWithStack(DMACh *dma, int id);
bool  hwDmacSrcChain(DMACh *dma, int id);

int hwConstRead8 (u32 x86reg, u32 mem, u32 sign);
int hwConstRead16(u32 x86reg, u32 mem, u32 sign);
int hwConstRead32(u32 x86reg, u32 mem);
void hwConstRead64(u32 mem, int mmreg);
void hwConstRead128(u32 mem, int xmmreg);

void hwConstWrite8 (u32 mem, int mmreg);
void hwConstWrite16(u32 mem, int mmreg);
void hwConstWrite32(u32 mem, int mmreg);
void hwConstWrite64(u32 mem, int mmreg);
void hwConstWrite128(u32 mem, int xmmreg);

extern void  intcInterrupt();
extern void  dmacInterrupt();

extern int rdram_devices, rdram_sdevid;

#endif /* __HW_H__ */
