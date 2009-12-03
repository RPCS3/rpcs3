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
// It's now going to be mostly depreciated for Dmac.h.

#include "Dmac.h"

// Transfer functions using u32. Eventually should be phased out for the tDMA_TAG functions.
namespace Tag
{
	static __forceinline void UpperTransfer(DMACh *tag, u32* ptag)
	{
		tag->chcrTransfer((tDMA_TAG*)ptag);
	}

	static __forceinline void LowerTransfer(DMACh *tag, u32* ptag)
	{
	    tag->qwcTransfer((tDMA_TAG*)ptag);
	}

	static __forceinline bool Transfer(const char *s, DMACh *tag, u32* ptag)
	{
	    tag->transfer(s, (tDMA_TAG*)ptag);
	}

	static __forceinline void UnsafeTransfer(DMACh *tag, u32* ptag)
	{
	    tag->unsafeTransfer((tDMA_TAG*)ptag);
	}
}

// Misc Tag functions, soon to be obsolete.
namespace Tag
{
	static __forceinline tag_id Id(u32* tag)
	{
	    return (tag_id)(((tDMA_TAG)tag[0]).ID);
	}

	static __forceinline tag_id Id(u32 tag)
	{
	    return (tag_id)(((tDMA_TAG)tag).ID);
	}

	static __forceinline bool IRQ(u32 *tag)
	{
	    return !!((tDMA_TAG)tag[0]).IRQ;
	}

	static __forceinline bool IRQ(u32 tag)
	{
	    return !!((tDMA_TAG)tag).IRQ;
	}
}
