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

#pragma once

// Allocates a Chunk of memory using SysMmapEx...
// This memory can be used to write recompiled code to.
// Its got basic resizing/growing ability too, but probably
// won't be too useful since growing the block buffer will
// invalidate any pointers pointing to the old block buffer.
// (so remember to invalidate those block pointers :D)
// It also deallocates itself on 'delete', so you can
// just use 'new' and 'delete' for initialization and
// deletion/cleanup respectfully...
class BlockBuffer {
protected:
	u32 mSize;		// Cur Size (in bytes)
	u32 mSizeT;		// Total Size (in bytes)
	u32 mGrowBy;	// amount to grow by when buffer fills (in bytes)
	u8* mData;		// Data Ptr (allocated via SysMmap)

	void alloc(int size) {
		mData = SysMmapEx(0, size, 0, "nVif_BlockBuffer");
		if (!mData) throw Exception::OutOfMemory(L"nVif recompiled code buffer (nVif_BlockBuffer)");
		clear();
	}
	void dealloc(u8* &dPtr, int size) {
		SafeSysMunmap( dPtr, size );
	}

public:
	virtual ~BlockBuffer() throw() {
		dealloc(mData, mSizeT);
	}

	BlockBuffer(u32 tSize, u32 growby=_1mb*2) {
		mSizeT	= tSize;
		mGrowBy	= growby;
		mSize	= 0;
		alloc(mSizeT);
	}

	void append(void *addr, u32 size) {
		if ((mSize + size) > mSizeT) grow(mSizeT + mGrowBy);
		memcpy_fast(&mData[mSize], addr, size);
		mSize += size;
	}

	// Increases the allocation size.  Warning:  Unlike 'realloc' this function will
	// CLEAR all contents of the buffer.  This is because copying contents of recompiled
	// caches is mostly useless since it invalidates any pointers into the block.
	//  (code relocation techniques are possible, but are difficult, potentially slow,
	//   and easily bug prone.  Not recommended at this time). --air
	void grow(u32 newSize) {
		pxAssume( newSize > mSizeT );
		u8* temp = mData;
		alloc(newSize);
		dealloc(temp, mSizeT);
		clear();
		mSizeT = newSize;
	}

	// clears the entire buffer to recompiler fill (0xcc), and sets mSize to 0.
	// (indicating none of the buffer is allocated).
	void clear() {
		if( mSize == 0 ) return;		// no clears needed if nothing's been written/modified
		mSize = 0;
		memset(mData, 0xcc, mSizeT);
	}

	u32  getCurSize()	{ return mSize;  }
	u32  getAllocSize()	{ return mSizeT; }
	u8*  getBlock()		{ return mData;  }
};
