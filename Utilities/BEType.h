#pragma once

#include "types.h"

// 128-bit vector type and also se_storage<> storage type
union alignas(16) v128
{
	char _bytes[16];

	template <typename T, std::size_t N, std::size_t M>
	struct masked_array_t // array type accessed as (index ^ M)
	{
		T m_data[N];

	public:
		T& operator[](std::size_t index)
		{
			return m_data[index ^ M];
		}

		const T& operator[](std::size_t index) const
		{
			return m_data[index ^ M];
		}
	};

#if IS_LE_MACHINE == 1
	template <typename T, std::size_t N = 16 / sizeof(T)>
	using normal_array_t = masked_array_t<T, N, 0>;
	template <typename T, std::size_t N = 16 / sizeof(T)>
	using reversed_array_t = masked_array_t<T, N, N - 1>;
#endif

	normal_array_t<u64> _u64;
	normal_array_t<s64> _s64;
	reversed_array_t<u64> u64r;
	reversed_array_t<s64> s64r;

	normal_array_t<u32> _u32;
	normal_array_t<s32> _s32;
	reversed_array_t<u32> u32r;
	reversed_array_t<s32> s32r;

	normal_array_t<u16> _u16;
	normal_array_t<s16> _s16;
	reversed_array_t<u16> u16r;
	reversed_array_t<s16> s16r;

	normal_array_t<u8> _u8;
	normal_array_t<s8> _s8;
	reversed_array_t<u8> u8r;
	reversed_array_t<s8> s8r;

	normal_array_t<f32> _f;
	normal_array_t<f64> _d;
	reversed_array_t<f32> fr;
	reversed_array_t<f64> dr;

	__m128 vf;
	__m128i vi;
	__m128d vd;

	struct bit_array_128
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

			bit_element& operator=(const bool right)
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

			bit_element& operator=(const bit_element& right)
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
		bit_element operator[](u32 index)
		{
#if IS_LE_MACHINE == 1
			return bit_element(m_data[1 - (index >> 6)], 0x8000000000000000ull >> (index & 0x3F));
#endif
		}

		// Index 0 returns the MSB and index 127 returns the LSB
		bool operator[](u32 index) const
		{
#if IS_LE_MACHINE == 1
			return (m_data[1 - (index >> 6)] & (0x8000000000000000ull >> (index & 0x3F))) != 0;
#endif
		}
	} _bit;

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

	bool operator==(const v128& right) const
	{
		return _u64[0] == right._u64[0] && _u64[1] == right._u64[1];
	}

	bool operator!=(const v128& right) const
	{
		return _u64[0] != right._u64[0] || _u64[1] != right._u64[1];
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
};

template <typename T, std::size_t N, std::size_t M>
struct offset32_array<v128::masked_array_t<T, N, M>>
{
	template <typename Arg>
	static inline u32 index32(const Arg& arg)
	{
		return SIZE_32(T) * (static_cast<u32>(arg) ^ static_cast<u32>(M));
	}
};

inline v128 operator|(const v128& left, const v128& right)
{
	return v128::fromV(_mm_or_si128(left.vi, right.vi));
}

inline v128 operator&(const v128& left, const v128& right)
{
	return v128::fromV(_mm_and_si128(left.vi, right.vi));
}

inline v128 operator^(const v128& left, const v128& right)
{
	return v128::fromV(_mm_xor_si128(left.vi, right.vi));
}

inline v128 operator~(const v128& other)
{
	return v128::from64(~other._u64[0], ~other._u64[1]);
}

template <typename T, std::size_t Align, std::size_t Size>
struct se_storage
{
	using type = std::aligned_storage_t<Size, Align>;

	// Unoptimized generic byteswap for unaligned data
	static void reverse(u8* dst, const u8* src)
	{
		for (std::size_t i = 0; i < Size; i++)
		{
			dst[i] = src[Size - 1 - i];
		}
	}

	static type to(const T& src)
	{
		type result;
		reverse(reinterpret_cast<u8*>(&result), reinterpret_cast<const u8*>(&src));
		return result;
	}

	static T from(const type& src)
	{
		T result;
		reverse(reinterpret_cast<u8*>(&result), reinterpret_cast<const u8*>(&src));
		return result;
	}

	static type copy(const type& src)
	{
		type result;
		for (std::size_t i = 0; i < Size; i++)
		{
			reinterpret_cast<u8*>(&result)[i] = reinterpret_cast<const u8*>(&src)[i];
		}
		return result;
	}
};

template <typename T>
struct se_storage<T, 2, 2>
{
	using type = u16;

	static constexpr u16 swap(u16 src)
	{
#if defined(__GNUG__)
		return __builtin_bswap16(src);
#else
		return _byteswap_ushort(src);
#endif
	}

	static inline u16 to(const T& src)
	{
		return swap(reinterpret_cast<const u16&>(src));
	}

	static inline T from(u16 src)
	{
		const u16 result = swap(src);
		return reinterpret_cast<const T&>(result);
	}

	static inline T copy(const T& src)
	{
		return src;
	}
};

template <typename T>
struct se_storage<T, 4, 4>
{
	using type = u32;

	static constexpr u32 swap(u32 src)
	{
#if defined(__GNUG__)
		return __builtin_bswap32(src);
#else
		return _byteswap_ulong(src);
#endif
	}

	static inline u32 to(const T& src)
	{
		return swap(reinterpret_cast<const u32&>(src));
	}

	static inline T from(u32 src)
	{
		const u32 result = swap(src);
		return reinterpret_cast<const T&>(result);
	}

	static inline T copy(const T& src)
	{
		return src;
	}
};

template <typename T>
struct se_storage<T, 8, 8>
{
	using type = u64;

	static constexpr u64 swap(u64 src)
	{
#if defined(__GNUG__)
		return __builtin_bswap64(src);
#else
		return _byteswap_uint64(src);
#endif
	}

	static inline u64 to(const T& src)
	{
		return swap(reinterpret_cast<const u64&>(src));
	}

	static inline T from(u64 src)
	{
		const u64 result = swap(src);
		return reinterpret_cast<const T&>(result);
	}

	static inline T copy(const T& src)
	{
		return src;
	}
};

template <typename T>
struct se_storage<T, 16, 16>
{
	using type = v128;

	static inline v128 swap(const v128& src)
	{
		return v128::from64(se_storage<u64>::swap(src._u64[1]), se_storage<u64>::swap(src._u64[0]));
	}

	static inline v128 to(const T& src)
	{
		return swap(reinterpret_cast<const v128&>(src));
	}

	static inline T from(const v128& src)
	{
		const v128 result = swap(src);
		return reinterpret_cast<const T&>(result);
	}

	static inline T copy(const T& src)
	{
		return src;
	}
};

// Switched endianness
template <typename T, std::size_t Align>
class se_t<T, true, Align>
{
	using type = typename std::remove_cv<T>::type;
	using stype = typename se_storage<type, Align>::type;
	using storage = se_storage<type, Align>;

	stype m_data;

	static_assert(!std::is_pointer<type>::value, "se_t<> error: invalid type (pointer)");
	static_assert(!std::is_reference<type>::value, "se_t<> error: invalid type (reference)");
	static_assert(!std::is_array<type>::value, "se_t<> error: invalid type (array)");
	static_assert(sizeof(type) == alignof(type), "se_t<> error: unexpected alignment");

public:
	se_t() = default;

	se_t(const se_t& right) = default;

	se_t(type value)
		: m_data(storage::to(value))
	{
	}

	type value() const
	{
		return storage::from(m_data);
	}

	se_t& operator=(const se_t&) = default;

	se_t& operator=(type value)
	{
		return m_data = storage::to(value), *this;
	}

	using simple_type = simple_t<T>;

	operator type() const
	{
		return storage::from(m_data);
	}
};

// Native endianness
template <typename T, std::size_t Align>
class se_t<T, false, Align>
{
	using type = typename std::remove_cv<T>::type;
	using stype = typename se_storage<type, Align>::type;
	using storage = se_storage<type, Align>;

	static_assert(!std::is_pointer<type>::value, "se_t<> error: invalid type (pointer)");
	static_assert(!std::is_reference<type>::value, "se_t<> error: invalid type (reference)");
	static_assert(!std::is_array<type>::value, "se_t<> error: invalid type (array)");
	static_assert(sizeof(type) == alignof(type), "se_t<> error: unexpected alignment");

	stype m_data;

public:
	se_t() = default;

	se_t(type value)
		: m_data(reinterpret_cast<const stype&>(value))
	{
	}

	type value() const
	{
		return storage::copy(reinterpret_cast<const type&>(m_data));
	}

	se_t& operator=(const se_t& value) = default;

	se_t& operator=(type value)
	{
		return m_data = reinterpret_cast<const stype&>(value), *this;
	}

	using simple_type = simple_t<T>;

	operator type() const
	{
		return storage::copy(reinterpret_cast<const type&>(m_data));
	}
};

// se_t<> with native endianness
template <typename T, std::size_t Align = alignof(T)>
using nse_t = se_t<T, false, Align>;

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator+=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value += right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator-=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value -= right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator*=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value *= right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator/=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value /= right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator%=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value %= right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator&=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value &= right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator|=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value |= right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator^=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value ^= right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator<<=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value <<= right);
}

template <typename T, bool Se, std::size_t Align, typename T1>
inline se_t<T, Se, Align>& operator>>=(se_t<T, Se, Align>& left, const T1& right)
{
	auto value = left.value();
	return left = (value >>= right);
}

template <typename T, bool Se, std::size_t Align>
inline se_t<T, Se, Align> operator++(se_t<T, Se, Align>& left, int)
{
	auto value = left.value();
	auto result = value++;
	left = value;
	return result;
}

template <typename T, bool Se, std::size_t Align>
inline se_t<T, Se, Align> operator--(se_t<T, Se, Align>& left, int)
{
	auto value = left.value();
	auto result = value--;
	left = value;
	return result;
}

template <typename T, bool Se, std::size_t Align>
inline se_t<T, Se, Align>& operator++(se_t<T, Se, Align>& right)
{
	auto value = right.value();
	return right = ++value;
}

template <typename T, bool Se, std::size_t Align>
inline se_t<T, Se, Align>& operator--(se_t<T, Se, Align>& right)
{
	auto value = right.value();
	return right = --value;
}

#if IS_LE_MACHINE == 1
template <typename T, std::size_t Align = alignof(T)>
using be_t = se_t<T, true, Align>;
template <typename T, std::size_t Align = alignof(T)>
using le_t = se_t<T, false, Align>;
#endif

// Type converter: converts native endianness arithmetic/enum types to appropriate se_t<> type
template <typename T, bool Se, typename = void>
struct to_se
{
	template <typename T2, typename = void>
	struct to_se_
	{
		using type = T2;
	};

	template <typename T2>
	struct to_se_<T2, std::enable_if_t<std::is_arithmetic<T2>::value || std::is_enum<T2>::value>>
	{
		using type = se_t<T2, Se>;
	};

	// Convert arithmetic and enum types
	using type = typename to_se_<T>::type;
};

template <bool Se>
struct to_se<v128, Se>
{
	using type = se_t<v128, Se>;
};

template <bool Se>
struct to_se<u128, Se>
{
	using type = se_t<u128, Se>;
};

template <bool Se>
struct to_se<s128, Se>
{
	using type = se_t<s128, Se>;
};

template <bool Se>
struct to_se<bool, Se>
{
	using type = bool;
};

template <bool Se>
struct to_se<char, Se>
{
	using type = char;
};

template <bool Se>
struct to_se<u8, Se>
{
	using type = u8;
};

template <bool Se>
struct to_se<s8, Se>
{
	using type = s8;
};

template <typename T, bool Se>
struct to_se<const T, Se, std::enable_if_t<!std::is_array<T>::value>>
{
	// Move const qualifier
	using type = const typename to_se<T, Se>::type;
};

template <typename T, bool Se>
struct to_se<volatile T, Se, std::enable_if_t<!std::is_array<T>::value && !std::is_const<T>::value>>
{
	// Move volatile qualifier
	using type = volatile typename to_se<T, Se>::type;
};

template <typename T, bool Se>
struct to_se<T[], Se>
{
	// Move array qualifier
	using type = typename to_se<T, Se>::type[];
};

template <typename T, bool Se, std::size_t N>
struct to_se<T[N], Se>
{
	// Move array qualifier
	using type = typename to_se<T, Se>::type[N];
};

// BE/LE aliases for to_se<>
#if IS_LE_MACHINE == 1
template <typename T>
using to_be_t = typename to_se<T, true>::type;
template <typename T>
using to_le_t = typename to_se<T, false>::type;
#endif

// BE/LE aliases for atomic_t
#if IS_LE_MACHINE == 1
template <typename T>
using atomic_be_t = atomic_t<be_t<T>>;
template <typename T>
using atomic_le_t = atomic_t<le_t<T>>;
#endif

template <typename T, bool Se, std::size_t Align>
struct fmt_unveil<se_t<T, Se, Align>, void>
{
	using type = typename fmt_unveil<T>::type;

	static inline auto get(const se_t<T, Se, Align>& arg)
	{
		return fmt_unveil<T>::get(arg);
	}
};
