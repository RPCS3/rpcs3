#pragma once
#include "platform.h"
#include "basic_types.h"
#include <cstddef>
#include <stdexcept>

namespace common
{
	union v128
	{
		template<typename T, std::size_t N, std::size_t M> class masked_array_t // array type accessed as (index ^ M)
		{
			T m_data[N];

		public:
			T& operator [](std::size_t index)
			{
				return m_data[index ^ M];
			}

			const T& operator [](std::size_t index) const
			{
				return m_data[index ^ M];
			}

			T& at(std::size_t index)
			{
				return (index ^ M) < N ? m_data[index ^ M] : throw std::out_of_range(__FUNCTION__);
			}

			const T& at(std::size_t index) const
			{
				return (index ^ M) < N ? m_data[index ^ M] : throw std::out_of_range(__FUNCTION__);
			}
		};

	#ifdef IS_LE_MACHINE
		template<typename T, std::size_t N = 16 / sizeof(T)> using normal_array_t = masked_array_t<T, N, 0>;
		template<typename T, std::size_t N = 16 / sizeof(T)> using reversed_array_t = masked_array_t<T, N, N - 1>;
	#else
		template<typename T, std::size_t N = 16 / sizeof(T)> using normal_array_t = masked_array_t<T, N, N - 1>;
		template<typename T, std::size_t N = 16 / sizeof(T)> using reversed_array_t = masked_array_t<T, N, 0>;
	#endif

		normal_array_t<u64>   _u64;
		normal_array_t<s64>   _s64;
		reversed_array_t<u64> u64r;
		reversed_array_t<s64> s64r;

		normal_array_t<u32>   _u32;
		normal_array_t<s32>   _s32;
		reversed_array_t<u32> u32r;
		reversed_array_t<s32> s32r;

		normal_array_t<u16>   _u16;
		normal_array_t<s16>   _s16;
		reversed_array_t<u16> u16r;
		reversed_array_t<s16> s16r;

		normal_array_t<u8>    _u8;
		normal_array_t<s8>    _s8;
		reversed_array_t<u8>  u8r;
		reversed_array_t<s8>  s8r;

		normal_array_t<f32>   _f;
		normal_array_t<f64>   _d;
		reversed_array_t<f32> fr;
		reversed_array_t<f64> dr;

		__m128 vf;
		__m128i vi;
		__m128d vd;

		class bit_array_128
		{
			u64 m_data[2];

		public:
			class bit_element
			{
				u64& data;
				const u64 mask;

			public:
				bit_element(u64& data, const u64 mask)
					: data(data)
					, mask(mask)
				{
				}

				operator bool() const
				{
					return (data & mask) != 0;
				}

				bit_element& operator =(const bool right)
				{
					if (right)
					{
						data |= mask;
					}
					else
					{
						data &= ~mask;
					}
					return *this;
				}

				bit_element& operator =(const bit_element& right)
				{
					if (right)
					{
						data |= mask;
					}
					else
					{
						data &= ~mask;
					}
					return *this;
				}
			};

			// Index 0 returns the MSB and index 127 returns the LSB
			bit_element operator [](u32 index)
			{
	#ifdef IS_LE_MACHINE
				return bit_element(m_data[1 - (index >> 6)], 0x8000000000000000ull >> (index & 0x3F));
	#else
				return bit_element(m_data[index >> 6], 0x8000000000000000ull >> (index & 0x3F));
	#endif
			}

			// Index 0 returns the MSB and index 127 returns the LSB
			bool operator [](u32 index) const
			{
	#ifdef IS_LE_MACHINE
				return (m_data[1 - (index >> 6)] & (0x8000000000000000ull >> (index & 0x3F))) != 0;
	#else
				return (m_data[index >> 6] & (0x8000000000000000ull >> (index & 0x3F))) != 0;
	#endif
			}

			bit_element at(u32 index)
			{
				if (index >= 128) throw std::out_of_range(__FUNCTION__);

				return operator[](index);
			}

			bool at(u32 index) const
			{
				if (index >= 128) throw std::out_of_range(__FUNCTION__);

				return operator[](index);
			}
		}
		_bit;

		static v128 from64(u64 _0, u64 _1 = 0)
		{
			v128 ret;
			ret._u64[0] = _0;
			ret._u64[1] = _1;
			return ret;
		}

		static v128 from64r(u64 _1, u64 _0 = 0)
		{
			return from64(_0, _1);
		}

		static v128 from32(u32 _0, u32 _1 = 0, u32 _2 = 0, u32 _3 = 0)
		{
			v128 ret;
			ret._u32[0] = _0;
			ret._u32[1] = _1;
			ret._u32[2] = _2;
			ret._u32[3] = _3;
			return ret;
		}

		static v128 from32r(u32 _3, u32 _2 = 0, u32 _1 = 0, u32 _0 = 0)
		{
			return from32(_0, _1, _2, _3);
		}

		static v128 from32p(u32 value)
		{
			v128 ret;
			ret.vi = _mm_set1_epi32(static_cast<s32>(value));
			return ret;
		}

		static v128 from16p(u16 value)
		{
			v128 ret;
			ret.vi = _mm_set1_epi16(static_cast<s16>(value));
			return ret;
		}

		static v128 from8p(u8 value)
		{
			v128 ret;
			ret.vi = _mm_set1_epi8(static_cast<s8>(value));
			return ret;
		}

		static v128 fromBit(u32 bit)
		{
			v128 ret = {};
			ret._bit[bit] = true;
			return ret;
		}

		static v128 fromV(__m128i value)
		{
			v128 ret;
			ret.vi = value;
			return ret;
		}

		static v128 fromF(__m128 value)
		{
			v128 ret;
			ret.vf = value;
			return ret;
		}

		static v128 fromD(__m128d value)
		{
			v128 ret;
			ret.vd = value;
			return ret;
		}

		static inline v128 add8(const v128& left, const v128& right)
		{
			return fromV(_mm_add_epi8(left.vi, right.vi));
		}

		static inline v128 add16(const v128& left, const v128& right)
		{
			return fromV(_mm_add_epi16(left.vi, right.vi));
		}

		static inline v128 add32(const v128& left, const v128& right)
		{
			return fromV(_mm_add_epi32(left.vi, right.vi));
		}

		static inline v128 addfs(const v128& left, const v128& right)
		{
			return fromF(_mm_add_ps(left.vf, right.vf));
		}

		static inline v128 addfd(const v128& left, const v128& right)
		{
			return fromD(_mm_add_pd(left.vd, right.vd));
		}

		static inline v128 sub8(const v128& left, const v128& right)
		{
			return fromV(_mm_sub_epi8(left.vi, right.vi));
		}

		static inline v128 sub16(const v128& left, const v128& right)
		{
			return fromV(_mm_sub_epi16(left.vi, right.vi));
		}

		static inline v128 sub32(const v128& left, const v128& right)
		{
			return fromV(_mm_sub_epi32(left.vi, right.vi));
		}

		static inline v128 subfs(const v128& left, const v128& right)
		{
			return fromF(_mm_sub_ps(left.vf, right.vf));
		}

		static inline v128 subfd(const v128& left, const v128& right)
		{
			return fromD(_mm_sub_pd(left.vd, right.vd));
		}

		static inline v128 maxu8(const v128& left, const v128& right)
		{
			return fromV(_mm_max_epu8(left.vi, right.vi));
		}

		static inline v128 minu8(const v128& left, const v128& right)
		{
			return fromV(_mm_min_epu8(left.vi, right.vi));
		}

		static inline v128 eq8(const v128& left, const v128& right)
		{
			return fromV(_mm_cmpeq_epi8(left.vi, right.vi));
		}

		static inline v128 eq16(const v128& left, const v128& right)
		{
			return fromV(_mm_cmpeq_epi16(left.vi, right.vi));
		}

		static inline v128 eq32(const v128& left, const v128& right)
		{
			return fromV(_mm_cmpeq_epi32(left.vi, right.vi));
		}

		bool operator ==(const v128& right) const
		{
			return _u64[0] == right._u64[0] && _u64[1] == right._u64[1];
		}

		bool operator !=(const v128& right) const
		{
			return _u64[0] != right._u64[0] || _u64[1] != right._u64[1];
		}

		bool is_any_1() const // check if any bit is 1
		{
			return _u64[0] || _u64[1];
		}

		bool is_any_0() const // check if any bit is 0
		{
			return ~_u64[0] || ~_u64[1];
		}

		// result = (~left) & (right)
		static inline v128 andnot(const v128& left, const v128& right)
		{
			return fromV(_mm_andnot_si128(left.vi, right.vi));
		}

		void clear()
		{
			_u64[0] = 0;
			_u64[1] = 0;
		}

		std::string to_hex() const;

		std::string to_xyzw() const;

		static inline v128 byteswap(const v128 val)
		{
			return fromV(_mm_shuffle_epi8(val.vi, _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)));
		}
	};

	static_assert(alignof(v128) == 16, "bad v128 implementation");

	inline v128 operator |(const v128& left, const v128& right)
	{
		return v128::fromV(_mm_or_si128(left.vi, right.vi));
	}

	inline v128 operator &(const v128& left, const v128& right)
	{
		return v128::fromV(_mm_and_si128(left.vi, right.vi));
	}

	inline v128 operator ^(const v128& left, const v128& right)
	{
		return v128::fromV(_mm_xor_si128(left.vi, right.vi));
	}

	inline v128 operator ~(const v128& other)
	{
		return v128::from64(~other._u64[0], ~other._u64[1]);
	}
}
