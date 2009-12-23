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

#include "xmmintrin.h"
#pragma once

template< typename T >
struct SizeChain
{
	int Size;
	T*  Chain;
};

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
protected:
	SizeChain<T> mBucket[hSize];

public:
	HashBucket() { 
		for (int i = 0; i < hSize; i++) {
			mBucket[i].Chain	= NULL;
			mBucket[i].Size		= 0;
		}
	}
	~HashBucket() { clear(); }
	int quickFind(u32 data) {
		return mBucket[data % hSize].Size;
	}
	__forceinline T* find(T* dataPtr) {
		u32 d = *((u32*)dataPtr);
		const SizeChain<T>& bucket( mBucket[d % hSize] );

		for (int i=bucket.Size; i; --i) {
			// This inline version seems about 1-2% faster in tests of games that average 1
			// program per bucket.  Games that average more should see a bigger improvement --air
			int result = _mm_movemask_ps( (__m128&) _mm_cmpeq_epi32( _mm_load_si128((__m128i*)&bucket.Chain[i]), _mm_load_si128((__m128i*)dataPtr) ) ) & 0x7;
			if( result == 0x7 ) return &bucket.Chain[i];

			// Dynamically generated function version, can't be inlined. :(
			//if ((((nVifCall)((void*)nVifMemCmp))(&bucket.Chain[i], dataPtr))==7) return &bucket.Chain[i];

			//if (!memcmp(&bucket.Chain[i], dataPtr, sizeof(T)-4)) return &c[i];	// old school version! >_<
		}
		if( bucket.Size > 3 ) DevCon.Warning( "recVifUnpk: Bucket 0x%04x has %d micro-programs", d % hSize, bucket.Size );
		return NULL;
	}
	__forceinline void add(const T& dataPtr) {
		u32 d = (u32&)dataPtr;
		SizeChain<T>& bucket( mBucket[d % hSize] );
		
		if( bucket.Chain = (T*)_aligned_realloc( bucket.Chain, sizeof(T)*(bucket.Size+1), 16), bucket.Chain==NULL ) {
			throw Exception::OutOfMemory(
				wxsFormat(L"Out of memory re-allocating hash bucket (bucket size=%d)", bucket.Size+1),
				wxEmptyString
			);
		}
		memcpy_fast(&bucket.Chain[bucket.Size++], &dataPtr, sizeof(T));
	}
	void clear() {
		for (int i = 0; i < hSize; i++) {
			safe_aligned_free(mBucket[i].Chain);
			mBucket[i].Size = 0;
		}
	}
};
