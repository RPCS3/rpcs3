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


#ifndef __HW_H__
#define __HW_H__

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

// HW defines
enum EERegisterAddresses
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
	IPU_CTRL		=	0x10002010,
	IPU_BP			=	0x10002020,
	IPU_TOP			=	0x10002030,

	GIF_CTRL		=	0x10003000,
	GIF_MODE		=	0x10003010,
	GIF_STAT		=	0x10003020,
	GIF_TAG0		=	0x10003040,
	GIF_TAG1		=	0x10003050,
	GIF_TAG2		=	0x10003060,
	GIF_TAG3		=	0x10003070,
	GIF_CNT			=	0x10003080,
	GIF_P3CNT		=	0x10003090,
	GIF_P3TAG		=	0x100030A0,

	// Vif Memory Locations
	VIF0_STAT		= 	0x10003800,
	VIF0_FBRST		= 	0x10003810,
	VIF0_ERR		= 	0x10003820,
	VIF0_MARK		= 	0x10003830,
	VIF0_CYCLE		=	0x10003840,
	VIF0_MODE		= 	0x10003850,
	VIF0_NUM		= 	0x10003860,
	VIF0_MASK		= 	0x10003870,
	VIF0_CODE		= 	0x10003880,
	VIF0_ITOPS		=	0x10003890,
	VIF0_ITOP		= 	0x100038d0,
	VIF0_TOP		=	0x100038e0,
	VIF0_R0			= 	0x10003900,
	VIF0_R1			= 	0x10003910,
	VIF0_R2			= 	0x10003920,
	VIF0_R3			=	0x10003930,
	VIF0_C0			= 	0x10003940,
	VIF0_C1			= 	0x10003950,
	VIF0_C2			=	0x10003960,
	VIF0_C3			= 	0x10003970,

	VIF1_STAT		=	0x10003c00,
	VIF1_FBRST		=	0x10003c10,
	VIF1_ERR		= 	0x10003c20,
	VIF1_MARK		= 	0x10003c30,
	VIF1_CYCLE		=	0x10003c40,
	VIF1_MODE		= 	0x10003c50,
	VIF1_NUM		=	0x10003c60,
	VIF1_MASK		=	0x10003c70,
	VIF1_CODE		=	0x10003c80,
	VIF1_ITOPS		= 	0x10003c90,
	VIF1_BASE		=	0x10003ca0,
	VIF1_OFST		= 	0x10003cb0,
	VIF1_TOPS		= 	0x10003cc0,
	VIF1_ITOP		= 	0x10003cd0,
	VIF1_TOP		= 	0x10003ce0,
	VIF1_R0			= 	0x10003d00,
	VIF1_R1			= 	0x10003d10,
	VIF1_R2			=	0x10003d20,
	VIF1_R3			=	0x10003d30,
	VIF1_C0			=	0x10003d40,
	VIF1_C1			=	0x10003d50,
	VIF1_C2			= 	0x10003d60,
	VIF1_C3			= 	0x10003d70,

	VIF0_FIFO		=	0x10004000,
	VIF1_FIFO		=	0x10005000,
	GIF_FIFO		=	0x10006000,

	IPUout_FIFO		=	0x10007000,
	IPUin_FIFO		=	0x10007010,

//VIF0
	D0_CHCR			=	0x10008000,
	D0_MADR			=	0x10008010,
	D0_QWC			=	0x10008020,
	D0_TADR			=	0x10008030,
	D0_ASR0			=	0x10008040,
	D0_ASR1			=	0x10008050,

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
	D6_TADR			=	0x1000C430,

//SIF2
	D7_CHCR			=	0x1000C800,
	D7_MADR			=	0x1000C810,
	D7_QWC			=	0x1000C820,

//fromSPR
	D8_CHCR			=	0x1000D000,
	D8_MADR			=	0x1000D010,
	D8_QWC			=	0x1000D020,
	D8_SADR			=	0x1000D080,
	D9_CHCR		=	0x1000D400,

	DMAC_CTRL		=	0x1000E000,
	DMAC_STAT		=	0x1000E010,
	DMAC_PCR		=	0x1000E020,
	DMAC_SQWC		=	0x1000E030,
	DMAC_RBSR		=	0x1000E040,
	DMAC_RBOR		=	0x1000E050,
	DMAC_STADR		=	0x1000E060,

	INTC_STAT		=	0x1000F000,
	INTC_MASK		=	0x1000F010,

	SIO_LCR			=	0x1000F100,
	SIO_LSR			=	0x1000F110,
	SIO_IER			=	0x1000F120,
	SIO_ISR			=	0x1000F130,//
	SIO_FCR			=	0x1000F140,
	SIO_BGR			=	0x1000F150,
	SIO_TXFIFO		=	0x1000F180,
	SIO_RXFIFO		=	0x1000F1C0,

	SBUS_F200		=	0x1000F200,	//MSCOM
	SBUS_F210		=	0x1000F210,	//SMCOM
	SBUS_F220		=	0x1000F220,	//MSFLG
	SBUS_F230		=	0x1000F230,	//SMFLG
	SBUS_F240		=	0x1000F240,
	SBUS_F250		=	0x1000F250,
	SBUS_F260		=	0x1000F260,

	MCH_RICM		=	0x1000F430,
	MCH_DRD			=	0x1000F440,

	DMAC_ENABLER	=	0x1000F520,
	DMAC_ENABLEW	=	0x1000F590
};

enum GSRegisterAddresses
{
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

// bleh, I'm graindead -- air
union tGS_SMODE2
{
	struct
	{
		u32 INT:1;
		u32 FFMD:1;
		u32 DPMS:2;
		u32 _PAD2:28;
		u32 _PAD3:32;
	};

	u64 _u64;

	bool IsInterlaced() const { return INT; }
};

void hwReset();

// hw read functions
extern mem8_t  hwRead8 (u32 mem);
extern mem16_t hwRead16(u32 mem);

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

extern void __fastcall hwWrite32_page_00( u32 mem, mem32_t value );
extern void __fastcall hwWrite32_page_01( u32 mem, mem32_t value );
extern void __fastcall hwWrite32_page_02( u32 mem, mem32_t value );
extern void __fastcall hwWrite32_page_03( u32 mem, mem32_t value );
extern void __fastcall hwWrite32_page_0B( u32 mem, mem32_t value );
extern void __fastcall hwWrite32_page_0E( u32 mem, mem32_t value );
extern void __fastcall hwWrite32_page_0F( u32 mem, mem32_t value );
extern void __fastcall hwWrite32_generic( u32 mem, mem32_t value );

extern void __fastcall hwWrite64_page_00( u32 mem, const mem64_t* srcval );
extern void __fastcall hwWrite64_page_01( u32 mem, const mem64_t* srcval );
extern void __fastcall hwWrite64_page_02( u32 mem, const mem64_t* srcval );
extern void __fastcall hwWrite64_page_03( u32 mem, const mem64_t* srcval );
extern void __fastcall hwWrite64_page_0E( u32 mem, const mem64_t* srcval );
extern void __fastcall hwWrite64_generic( u32 mem, const mem64_t* srcval );

extern void __fastcall hwWrite128_generic(u32 mem, const mem128_t *srcval);

bool hwMFIFOWrite(u32 addr, u8 *data, u32 size);

extern const int rdram_devices;
extern int rdram_sdevid;

#endif /* __HW_H__ */
