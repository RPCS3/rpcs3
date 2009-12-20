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

extern __pagealigned u8 nVifMemCmp[__pagesize];

// HashBucket is a container which uses a built-in hash function
// to perform quick searches.
// T is a struct data type (note: size must be in multiples of 16 bytes!)
// hSize determines the number of buckets HashBucket will use for sorting.
// cmpSize is the size of data to consider 2 structs equal (see find())
// The hash function is determined by taking the first bytes of data and
// performing a modulus the size of hSize. So the most diverse-data should
// be in the first bytes of the struct. (hence why nVifBlock is specifically sorted)
template<typename T, int hSize, int cmpSize>
class HashBucket {
private:
	T*  mChain[hSize];
	int mSize [hSize];
public:
	HashBucket() { 
		for (int i = 0; i < hSize; i++) {
			mChain[i] = NULL;
			mSize [i] = 0;
		}
	}
	~HashBucket() { clear(); }
	int quickFind(u32 data) {
		int o = data % hSize;
		return mSize[o];
	}
	T* find(T* dataPtr) {
		u32 d = *((u32*)dataPtr);
		int o = d % hSize;
		int s = mSize[o];
		T*  c = mChain[o];
		for (int i = 0; i < s; i++) {
			//if (!memcmp(&c[i], dataPtr, cmpSize)) return &c[i];
			if ((((nVifCall)((void*)nVifMemCmp))(&c[i], dataPtr))==0xf) return &c[i];
		}
		return NULL;
	}
	void add(T* dataPtr) {
		u32 d = *(u32*)dataPtr;
		int o = d % hSize;
		int s = mSize[o]++;
		T*  c = mChain[o];
		T*  n = (T*)_aligned_malloc(sizeof(T)*(s+1), 16);
		if (s) { 
			memcpy(n, c, sizeof(T) * s);
			safe_aligned_free(c);
		}
		memcpy(&n[s], dataPtr, sizeof(T));
		mChain[o] = n;
	}
	void clear() {
		for (int i = 0; i < hSize; i++) {
			safe_aligned_free(mChain[i]);
			mSize[i] = 0;
		}
	}
};
