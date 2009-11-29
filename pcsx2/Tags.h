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

// This is meant to be a collection of generic functions dealing with tags.
#include "Dmac.h"

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

enum d_ctrl_flags
{
	CTRL_DMAE = 0x1, // 0/1 - disables/enables all DMAs
	CTRL_RELE = 0x2, // 0/1 - cycle stealing off/on
	CTRL_MFD = 0xC, // Memory FIFO drain channel (mfd_type)
	CTRL_STS = 0x30, // Stall Control source channel (sts type)
	CTRL_STD = 0xC0, // Stall Control drain channel (std_type)
	CTRL_RCYC = 0x100 // Release cycle (8/16/32/64/128/256)
	// When cycle stealing is on, the release cycle sets the period to release
	// the bus to EE.
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

namespace Tag
{
	// Transfer functions,
	static __forceinline void UpperTransfer(DMACh *tag, u32* ptag)
	{
		// Transfer upper part of tag to CHCR bits 31-15
		//tag->chcr._u32 = (tag->chcr._u32 & 0xFFFF) | ((*ptag) & 0xFFFF0000);
		tag->chcr.TAG = ((*ptag) >> 16);
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
			Console.Error("%s BUSERR", s);
			UpperTransfer(tag, ptag);

			// Set BEIS (BUSERR) in DMAC_STAT register
			dmacRegs->stat.BEIS = true;
			return false;
		}
		else
		{
			UpperTransfer(tag, ptag);
			LowerTransfer(tag, ptag);
			return true;
		}
	}

	static __forceinline void UnsafeTransfer(DMACh *tag, u32* ptag)
	{
		UpperTransfer(tag, ptag);
		LowerTransfer(tag, ptag);
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
		return !!(tag[0] >> 31);
	}

	static __forceinline bool IRQ(u32 tag)
	{
		return !!(tag >> 31);
	}
}
