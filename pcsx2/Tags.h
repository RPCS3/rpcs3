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

// This is meant to be a collection of generic functions dealing with tags.
// I kept seeing the same code over and over with different structure names
// and the same members, and figured it'd be a good spot to use templates...

// Actually, looks like I didn't need templates after all... :)

enum mfd_type 
{
	NO_MFD,
	MFD_RESERVED,
	MFD_VIF1,
	MFD_GIF
};

enum sts_type 
{
	NO_STS,
	STS_SIF0,
	STS_fromSPR,
	STS_fromIPU
};

enum std_type 
{
	NO_STD,
	STD_VIF1,
	STD_GIF,
	STD_SIF1
};

enum d_ctrl_flags
{
	CTRL_DMAE = 0x1, // 0/1 - disables/enables all DMAs
	CTRL_RELE = 0x2, // 0/1 - cycle stealing off/on
	CTRL_MFD = 0xC, // Memory FIFO drain channel (mfd_type)
	CTRL_STS = 0x30, // Stall Control source channel (sts type)
	CTRL_STD = 0xC0, // Stall Controll drain channel (std_type)
	CTRL_RCYC = 0x100 // Release cycle (8/16/32/64/128/256)
	// When cycle stealing is on, the release cycle sets the period to release
	// the bus to EE.
};

enum pce_values
{
	PCE_NOTHING = 0, 
	PCE_RESERVED,
	PCE_DISABLED,
	PCE_ENABLED
};

enum tag_id
{
	TAG_CNTS = 0,	
	TAG_REFE = 0, 	// Transfer Packet According to ADDR field, clear STR, and end
	TAG_CNT, 		// Transfer QWC following the tag.
	TAG_NEXT,		// Transfer QWC following tag. TADR = ADDR
	TAG_REF,			// Transfer QWC from ADDR field
	TAG_REFS,		// Transfer QWC from ADDR field (Stall Control)
	TAG_CALL,		// Transfer QWC following the tag, save succeeding tag
	TAG_RET,			// Transfer QWC following the tag, load next tag
	TAG_END			// Transfer QWC following the tag
};

enum chcr_flags
{
	CHCR_DIR = 0x1, 	// Direction: 0 - to memory, 1 - from memory. VIF1 & SIF2 only.
	CHCR_MOD1 = 0x4, 
	CHCR_MOD2 = 0x8,
	CHCR_MOD = 0xC, 	// MOD1 & MOD2; Holds which of the Transfer modes above is used.
	CHCR_ASP1 = 0x10,
	CHCR_ASP2 = 0x20,
	CHCR_ASP = 0x30, 	// ASP1 & ASP2; Address stack pointer. 0, 1, or 2 addresses.
	CHCR_TTE = 0x40, 	// Tag Transfer Enable. 0 - Diable / 1 - Enable.
	CHCR_TIE = 0x80, 	// Tag Interrupt Enable. 0 - Diable / 1 - Enable.
	CHCR_STR = 0x100 	// Start. 0 while stopping DMA, 1 while it's running.
};

enum TransferMode
{
	NORMAL_MODE = 0,
	CHAIN_MODE,
	INTERLEAVE_MODE,
	UNDEFINED_MODE
};

namespace Tag
{
	// Transfer functions,
	static __forceinline void UpperTransfer(DMACh *tag, u32* ptag)
	{
		// Transfer upper part of tag to CHCR bits 31-15
		tag->chcr = (tag->chcr & 0xFFFF) | ((*ptag) & 0xFFFF0000);
	}

	static __forceinline void LowerTransfer(DMACh *tag, u32* ptag)
	{
		//QWC set to lower 16bits of the tag
		tag->qwc = (u16)ptag[0];
	}

	static __forceinline bool Transfer(const char *s, DMACh *tag, u32* ptag)
	{
		if (ptag == NULL)  					 // Is ptag empty?
		{
			Console::Error("%s BUSERR", params s);
			UpperTransfer(tag, ptag);
			
			// Set BEIS (BUSERR) in DMAC_STAT register
			psHu32(DMAC_STAT) |= DMAC_STAT_BEIS; 
			return false;
		}
		else
		{
			UpperTransfer(tag, ptag);
			LowerTransfer(tag, ptag);
			return true;
		}
	}
	
	/*// Not sure if I'll need this one.
	static __forceinline bool SafeTransfer(const char *s, DMACh *tag, u32* ptag)
	{
		if (ptag == NULL)  					 // Is ptag empty?
		{
			Console::Error("%s BUSERR", params s);
			
			// Set BEIS (BUSERR) in DMAC_STAT register
			psHu32(DMAC_STAT) |= DMAC_STAT_BEIS; 
			return false;
		}
		else
		{
			UpperTransfer(tag, ptag);
			LowerTransfer(tag, ptag);
			return true;
		}
	}*/
	
	static __forceinline void UnsafeTransfer(DMACh *tag, u32* ptag)
	{
		UpperTransfer(tag, ptag);
		LowerTransfer(tag, ptag);
	}
	
	// Untested
	static __forceinline u16 QWC(u32 *tag)
	{
		return (tag[0] & 0xffff);
	}
	
	// Untested
	static __forceinline pce_values PCE(u32 *tag)
	{
		return (pce_values)((tag[0] >> 22) & 0x3);
	}
	
	static __forceinline tag_id Id(u32* tag)
	{
		return (tag_id)((tag[0] >> 28) & 0x7);
	}
	
	static __forceinline tag_id Id(u32 tag)
	{
		return (tag_id)((tag >> 28) & 0x7);
	}
	
	static __forceinline bool IRQ(u32 *tag)
	{
		return !!(tag[0] & 0x8000000);
	}
	
	static __forceinline bool IRQ(u32 tag)
	{
		return !!(tag & 0x8000000);
	}
}

namespace CHCR
{
	// Query the flags in the channel control register.
	static __forceinline bool STR(DMACh *tag) { return !!(tag->chcr & CHCR_STR); }
	static __forceinline bool TIE(DMACh *tag) { return !!(tag->chcr & CHCR_TIE); }
	static __forceinline bool TTE(DMACh *tag) { return !!(tag->chcr & CHCR_TTE); }
	static __forceinline u8 DIR(DMACh *tag) { return (tag->chcr & CHCR_DIR); }

	static __forceinline TransferMode MOD(DMACh *tag)
	{
		return (TransferMode)((tag->chcr & CHCR_MOD) >> 2);
	}
	
	static __forceinline u8 ASP(DMACh *tag)
	{
		
		return (TransferMode)((tag->chcr & CHCR_ASP) >> 4);
	}

	// Clear the individual flags.
	static __forceinline void clearSTR(DMACh *tag) { tag->chcr &= ~CHCR_STR; }
	static __forceinline void clearTIE(DMACh *tag) { tag->chcr &= ~CHCR_TIE; }
	static __forceinline void clearTTE(DMACh *tag) { tag->chcr &= ~CHCR_TTE; }
	static __forceinline void clearDIR(DMACh *tag) { tag->chcr &= ~CHCR_DIR; }
	
	// Set them.
	static __forceinline void setSTR(DMACh *tag) { tag->chcr |= CHCR_STR; }
	static __forceinline void setTIE(DMACh *tag) { tag->chcr |= CHCR_TIE; }
	static __forceinline void setTTE(DMACh *tag) { tag->chcr |= CHCR_TTE; }
	static __forceinline void setDIR(DMACh *tag) { tag->chcr |= CHCR_DIR; }
	
	static __forceinline void setMOD(DMACh *tag, TransferMode mode)
	{
		if (mode & (1 << 0))
			tag->chcr |= CHCR_MOD1; 
		else
			tag->chcr &= CHCR_MOD1; 
			
		if (mode & (1 << 1)) 
			tag->chcr |= CHCR_MOD2;
		else
			tag->chcr &= CHCR_MOD2;
	}
	
	static __forceinline void setASP(DMACh *tag, u8 num)
	{
		if (num & (1 << 0))
			tag->chcr |= CHCR_ASP1; 
		else
			tag->chcr &= CHCR_ASP2; 
			
		if (num & (1 << 1)) 
			tag->chcr |= CHCR_ASP1;
		else
			tag->chcr &= CHCR_ASP2;
	}
	
	// Print information about a chcr tag.
	static __forceinline void Print(const char*  s, DMACh *tag)
	{
		u8 num_addr = ASP(tag);
		TransferMode mode = MOD(tag);
		
		Console::Write("%s chcr %s mem: ", params s, (DIR(tag)) ? "from" : "to");
		
		if (mode == NORMAL_MODE)
			Console::Write(" normal mode; ");
		else if (mode == CHAIN_MODE)
			Console::Write(" chain mode; ");
		else if (mode == INTERLEAVE_MODE)
			Console::Write(" interleave mode; ");
		else
			Console::Write(" ?? mode; ");
		
		if (num_addr != 0) Console::Write("ASP = %d;", params num_addr);
		if (TTE(tag)) Console::Write("TTE;");
		if (TIE(tag)) Console::Write("TIE;");
		if (STR(tag)) Console::Write(" (DMA started)."); else Console::Write(" (DMA stopped).");
		Console::WriteLn("");
	}
}

namespace QWC
{
	static __forceinline bool Empty(DMACh *tag)
	{
		return (tag->qwc == 0);
	}
	
	
	static __forceinline void Clear(DMACh *tag)
	{
		tag->qwc = 0;
	}
}

namespace D_CTRL
{
	static __forceinline bool DMAE() { return !!(psHu32(DMAC_CTRL) & CTRL_DMAE); }
	static __forceinline bool RELE() { return !!(psHu32(DMAC_CTRL) & CTRL_RELE); }
	static __forceinline bool MFD() 
	{ 
		return (mfd_type)((psHu32(DMAC_CTRL) & CTRL_MFD) >> 2);
	}
	static __forceinline bool STS() 
	{ 
		return (sts_type)((psHu32(DMAC_CTRL) & CTRL_STS) >> 2);
	}
	static __forceinline bool STD() 
	{ 
		return (std_type)((psHu32(DMAC_CTRL) & CTRL_STD) >> 2);
	}
	static __forceinline bool RCLC() 
	{ 
		return ((((psHu32(DMAC_CTRL) & CTRL_RCYC) >> 3) + 1) * 8);
	}
}
