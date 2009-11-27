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

#ifndef __VIF_H__
#define __VIF_H__

enum vif0_stat_flags
{
	VIF0_STAT_VPS_W 	= (1),
	VIF0_STAT_VPS_D 	= (2),
	VIF0_STAT_VPS_T		= (3),
	VIF0_STAT_VPS 		= (3),
	VIF0_STAT_VEW		= (1<<2),
	VIF0_STAT_MRK		= (1<<6),
	VIF0_STAT_DBF		= (1<<7),
	VIF0_STAT_VSS		= (1<<8),
	VIF0_STAT_VFS		= (1<<9),
	VIF0_STAT_VIS		= (1<<10),
	VIF0_STAT_INT		= (1<<11),
	VIF0_STAT_ER0		= (1<<12),
	VIF0_STAT_ER1		= (1<<13),
	VIF0_STAT_FQC		= (15<<24)
};

enum vif1_stat_flags
{
	VIF1_STAT_VPS_W 	= (1),
	VIF1_STAT_VPS_D 	= (2),
	VIF1_STAT_VPS_T		= (3),
	VIF1_STAT_VPS 		= (3),
	VIF1_STAT_VEW		= (1<<2),
	VIF1_STAT_VGW		= (1<<3),
	VIF1_STAT_MRK		= (1<<6),
	VIF1_STAT_DBF		= (1<<7),
	VIF1_STAT_VSS		= (1<<8),
	VIF1_STAT_VFS		= (1<<9),
	VIF1_STAT_VIS		= (1<<10),
	VIF1_STAT_INT		= (1<<11),
	VIF1_STAT_ER0		= (1<<12),
	VIF1_STAT_ER1		= (1<<13),
	VIF1_STAT_FDR 		= (1<<23),
	VIF1_STAT_FQC		= (31<<24)
};

// These are the stat flags that are the same for vif0 & vif1,
// for occassions where we don't neccessarily know which we are using.
enum vif_stat_flags
{
	VIF_STAT_VPS_W 	= (1),
	VIF_STAT_VPS_D 	= (2),
	VIF_STAT_VPS_T		= (3),
	VIF_STAT_VPS 		= (3),
	VIF_STAT_VEW		= (1<<2),
	VIF_STAT_MRK		= (1<<6),
	VIF_STAT_DBF		= (1<<7),
	VIF_STAT_VSS		= (1<<8),
	VIF_STAT_VFS		= (1<<9),
	VIF_STAT_VIS		= (1<<10),
	VIF_STAT_INT		= (1<<11),
	VIF_STAT_ER0		= (1<<12),
	VIF_STAT_ER1		= (1<<13)
};

enum vif_status
{
    VPS_IDLE = 0,
    VPS_WAITING = 1,
    VPS_DECODING = 2,
    VPS_TRANSFERRING = 3 // And decompressing.
};

//
// Bitfield Structure
//
union tVIF_STAT {
	struct {
		u32 VPS : 2; // Vif(0/1) status; 00 - idle, 01 - waiting for data following vifcode, 10 - decoding vifcode, 11 - decompressing/trasferring data follwing vifcode.
		u32 VEW : 1; // E-bit wait (1 - wait, 0 - don't wait)
		u32 VGW : 1; // Status waiting for the end of gif transfer (Vif1 only)
		u32 reserved : 2;
		u32 MRK : 1; // Mark Detect
		u32 DBF : 1; // Double Buffer Flag
		u32 VSS : 1; // Stopped by STOP
		u32 VFS : 1; // Stopped by ForceBreak
		u32 VIS : 1; // Vif Interrupt Stall
		u32 INT : 1; // Intereupt by the i bit.
		u32 ER0 : 1;  // DmaTag Mismatch error.
		u32 ER1 : 1; // VifCode error
		u32 reserved2 : 9;
		u32 FDR : 1; // VIF/FIFO transfer direction. (false - memory -> Vif, true - Vif -> memory)
		u32 FQC : 5; // Amount of data. Up to 8 qwords on Vif0, 16 on Vif1.
	};
	u32 _u32;

	tVIF_STAT(u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
};

union tVIF_FBRST {
	struct {
		u32 RST : 1; // Resets Vif(0/1) when written.
		u32 FBK : 1; // Causes a Forcebreak to Vif((0/1) when true. (Stall)
		u32 STP : 1; // Stops after the end of the Vifcode in progress when true. (Stall)
		u32 STC : 1; // Cancels the Vif(0/1) stall and clears Vif Stats VSS, VFS, VIS, INT, ER0 & ER1.
		u32 reserved : 28;
	};
	u32 _u32;
	
	tVIF_FBRST(u32 val) { _u32 = val; }
	
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
};

union tVIF_ERR {
	struct {
		u32 MII : 1; // Masks Stat INT.
		u32 ME0 : 1; // Masks Stat Err0.
		u32 ME1 : 1; // Masks Stat Err1.
		u32 reserved : 29;
	};
	u32 _u32;
	
	tVIF_ERR(u32 val) { _u32 = val; }
	
	void write(u32 val) { _u32 = val; }
	bool test(u32 flags) { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
};

struct vifCycle {
	u8 cl, wl;
	u8 pad[2];
};

struct VIFregisters {
	tVIF_STAT stat;
	u32 pad0[3];
	u32 fbrst;
	u32 pad1[3];
	tVIF_ERR err;
	u32 pad2[3];
	u32 mark;
	u32 pad3[3];
	vifCycle cycle; //data write cycle
	u32 pad4[3];
	u32 mode;
	u32 pad5[3];
	u32 num;
	u32 pad6[3];
	u32 mask;
	u32 pad7[3];
	u32 code;
	u32 pad8[3];
	u32 itops;
	u32 pad9[3];
	u32 base;      // Not used in VIF0
	u32 pad10[3];
	u32 ofst;      // Not used in VIF0
	u32 pad11[3];
	u32 tops;      // Not used in VIF0
	u32 pad12[3];
	u32 itop;
	u32 pad13[3];
	u32 top;       // Not used in VIF0
	u32 pad14[3];
	u32 mskpath3;
	u32 pad15[3];
	u32 r0;        // row0 register
	u32 pad16[3];
	u32 r1;        // row1 register
	u32 pad17[3];
	u32 r2;        // row2 register
	u32 pad18[3];
	u32 r3;        // row3 register
	u32 pad19[3];
	u32 c0;        // col0 register
	u32 pad20[3];
	u32 c1;        // col1 register
	u32 pad21[3];
	u32 c2;        // col2 register
	u32 pad22[3];
	u32 c3;        // col3 register
	u32 pad23[3];
	u32 offset;    // internal UNPACK offset
	u32 addr;
};

extern "C"
{
	// these use cdecl for Asm code references.
	extern VIFregisters *vifRegs;
	extern u32* vifMaskRegs;
	extern u32* vifRow;
	extern u32* _vifCol;
}

#define vif0Regs ((VIFregisters*)&PS2MEM_HW[0x3800])
#define vif1Regs ((VIFregisters*)&PS2MEM_HW[0x3c00])

extern void dmaVIF0();
extern void dmaVIF1();
extern void mfifoVIF1transfer(int qwc);
extern bool VIF0transfer(u32 *data, int size, bool istag);
extern bool VIF1transfer(u32 *data, int size, bool istag);
extern void vifMFIFOInterrupt();

// --------------------------------------------------------------------------------------
//  VIF SEE-optimized Masking Mess
// --------------------------------------------------------------------------------------

struct VifMaskTypes
{
	u32	Row0[4], Col0[4];
	u32	Row1[4], Col1[4];
};

extern __aligned16 VifMaskTypes g_vifmask;

extern void __fastcall SetNewMask(u32* vif1masks, u32* hasmask, u32 mask, u32 oldmask);

#define XMM_R0			xmm0
#define XMM_R1			xmm1
#define XMM_R2			xmm2
#define XMM_WRITEMASK	xmm3
#define XMM_ROWMASK		xmm4
#define XMM_ROWCOLMASK	xmm5
#define XMM_ROW			xmm6
#define XMM_COL			xmm7

#define XMM_R3			XMM_COL

#endif /* __VIF_H__ */
