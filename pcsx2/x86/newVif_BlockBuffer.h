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

class BlockBuffer {
private:
	u32 mSize;  // Cur Size
	u32 mSizeT; // Total Size
	u8* mData;  // Data Ptr
	void grow(u32 newSize) {
		u8* temp = new u8[newSize];
		memcpy(temp, mData, mSizeT);
		safe_delete( mData );
		mData = temp;
	}
public:
	BlockBuffer(u32 tSize) { mSizeT = tSize; mSize = 0; mData = new u8[mSizeT]; }
	virtual ~BlockBuffer() { safe_delete(mData); }
	void append(void *addr, u32 size) {
		if (mSize + size > mSizeT) grow(mSize*2 + size);
		memcpy(&mData[mSize], addr, size);
		mSize += size;
	}
	void clear()    { mSize = 0; }
	u32  getSize()  { return mSize; }
	u8*  getBlock() { return mData; }
};
