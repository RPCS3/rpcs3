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

enum TransferMode
{
	NORMAL_MODE = 0,
	CHAIN_MODE,
	INTERLEAVE_MODE,
	UNDEFINED_MODE
};

template <class T>
static __forceinline void UpperTagTransfer(T tag, u32* ptag)
{
	// Transfer upper part of tag to CHCR bits 31-15
	tag->chcr = (tag->chcr & 0xFFFF) | ((*ptag) & 0xFFFF0000);
}

template <class T>
static __forceinline void LowerTagTransfer(T tag, u32* ptag)
{
	//QWC set to lower 16bits of the tag
	tag->qwc = (u16)ptag[0];
}

// Transfer a tag. 
template <class T>
static __forceinline bool TransferTag(const char *s, T tag, u32* ptag)
{
	if (ptag == NULL)  					 // Is ptag empty?
	{
		Console::Error("%s BUSERR", params s);
		UpperTagTransfer(tag, ptag);
		
		// Set BEIS (BUSERR) in DMAC_STAT register
		psHu32(DMAC_STAT) |= DMAC_STAT_BEIS; 
		return false;
	}
	else
	{
		UpperTagTransfer(tag, ptag);
		LowerTagTransfer(tag, ptag);
		return true;
	}
}

enum pce_values
{
	PCE_NOTHING = 0, 
	PCE_RESERVED,
	PCE_DISABLED,
	PCE_ENABLED
};

enum tag_id
{
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

namespace ChainTags
{
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
	
	static __forceinline bool IRQ(u32 *tag)
	{
		return (tag[0] & 0x8000000);
	}
	
	static __forceinline bool IRQ(u32 tag)
	{
		return (tag & 0x8000000);
	}
}

namespace CHCR
{
	// Query the flags in the channel control register.
	template <class T>
	static __forceinline bool STR(T tag) { return (tag->chcr & CHCR_STR); }
	
	template <class T>
	static __forceinline bool TIE(T tag) { return (tag->chcr & CHCR_TIE); }
	
	template <class T>
	static __forceinline bool TTE(T tag) { return (tag->chcr & CHCR_TTE); }
	
	template <class T>
	static __forceinline u8 DIR(T tag) { return (tag->chcr & CHCR_DIR); }
	
	template <class T>
	static __forceinline TransferMode MOD(T tag)
	{
		return (TransferMode)((tag->chcr & CHCR_MOD) >> 2);
	}
	
	template <class T>
	static __forceinline u8 ASP(T tag)
	{
		return (TransferMode)((tag->chcr & CHCR_ASP) >> 2);
	}

	// Clear the individual flags.
	template <class T>
	static __forceinline void clearSTR(T tag) { tag->chcr &= ~CHCR_STR; }
	
	template <class T>
	static __forceinline void clearTIE(T tag) { tag->chcr &= ~CHCR_TIE; }
	
	template <class T>
	static __forceinline void clearTTE(T tag) { tag->chcr &= ~CHCR_TTE; }
	
	template <class T>
	static __forceinline void clearDIR(T tag) { tag->chcr &= ~CHCR_DIR; }
	
	// Set them.
	template <class T>
	static __forceinline void setSTR(T tag) { tag->chcr |= CHCR_STR; }
	
	template <class T>
	static __forceinline void setTIE(T tag) { tag->chcr |= CHCR_TIE; }
	
	template <class T>
	static __forceinline void setTTE(T tag) { tag->chcr |= CHCR_TTE; }
	
	template <class T>
	static __forceinline void setDIR(T tag) { tag->chcr |= CHCR_DIR; }
	
	template <class T>
	static __forceinline void setMOD(T tag, TransferMode mode)
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
	
	template <class T>
	static __forceinline void setASP(T tag, u8 num)
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
	template <class T>
	static __forceinline void Print(const char*  s, T tag)
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