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

#include "xmmintrin.h"
#pragma once

// Create some typecast operators for SIMD operations.  For some reason MSVC needs a
// handle/reference typecast to avoid error.  GCC (and presumably other compilers)
// generate an error if the handle/ref is used.  Honestly neither makes sense, since
// both typecasts should be perfectly valid >_<.  --air
#ifdef _MSC_VER
#	define cast_m128		__m128&
#	define cast_m128i		__m128i&
#	define cast_m128d		__m128d&
#else // defined(__GNUC__)
#	define cast_m128		__m128
#	define cast_m128i		__m128i
#	define cast_m128d		__m128d
#endif

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

		const __m128i* endpos = (__m128i*)&bucket.Chain[bucket.Size];
		const __m128i data128( _mm_load_si128((__m128i*)dataPtr) );

		for( const __m128i* chainpos = (__m128i*)bucket.Chain; chainpos<endpos; ++chainpos ) {
			// This inline SSE code is generally faster than using emitter code, since it inlines nicely. --air
			int result = _mm_movemask_ps( (cast_m128) _mm_cmpeq_epi32( data128, _mm_load_si128(chainpos) ) );
			if( (result&0x7) == 0x7 ) return (T*)chainpos;

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
		memcpy_const(&bucket.Chain[bucket.Size++], &dataPtr, sizeof(T));
	}
	void clear() {
		for (int i = 0; i < hSize; i++) {
			safe_aligned_free(mBucket[i].Chain);
			mBucket[i].Size = 0;
		}
	}
};
