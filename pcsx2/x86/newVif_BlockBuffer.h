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
	u32 mSize;  // Cur Size
	u32 mSizeT; // Total Size
	u8* mData;  // Data Ptr
	void alloc(int size) {
		mData = SysMmapEx(NULL, size, 0, "nVif_BlockBuffer");
		if (!mData) throw Exception::OutOfMemory("nVif Error: Failed to allocate recompiler memory!");
		memset(mData, 0xcc, size);
	}
	void dealloc(u8* &dPtr, int size) {
		if (dPtr) { HostSys::Munmap(dPtr, size); dPtr = NULL; }
	}
public:
	BlockBuffer(u32 tSize)	{ mSizeT = tSize; mSize = 0; alloc(mSizeT); }
	~BlockBuffer()			{ dealloc(mData, mSizeT); }
	void append(void *addr, u32 size) {
		if (mSize + size > mSizeT) grow(mSize*2 + size);
		memcpy(&mData[mSize], addr, size);
		mSize += size;
	}
	void grow(u32 newSize) {
		u8* temp = mData;
		alloc  (newSize);
		memcpy (mData, temp, mSize);
		dealloc(temp, mSizeT);
		mSizeT = newSize;
	}
	void clear()      { mSize = 0; }
	u32  getCurSize() { return mSize;  }
	u32  getSize()    { return mSizeT; }
	u8*  getBlock()   { return mData;  }
};
