#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#define IS_LE_MACHINE // only draft

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
		return (index ^ M) < N ? m_data[index ^ M] : throw std::out_of_range("Masked array");
	}

	const T& at(std::size_t index) const
	{
		return (index ^ M) < N ? m_data[index ^ M] : throw std::out_of_range("Masked array");
	}
};

union v128
{
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

			inline operator bool() const
			{
				return (data & mask) != 0;
			}

			inline bit_element& operator =(const bool right)
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

			inline bit_element& operator =(const bit_element& right)
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
			if (index >= 128) throw std::out_of_range("Bit element");

			return operator[](index);
		}

		bool at(u32 index) const
		{
			if (index >= 128) throw std::out_of_range("Bit element");

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

	inline bool is_any_1() const // check if any bit is 1
	{
		return _u64[0] || _u64[1];
	}

	inline bool is_any_0() const // check if any bit is 0
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

CHECK_SIZE_ALIGN(v128, 16, 16);

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

template<typename T, std::size_t Size = sizeof(T)> struct se_storage
{
	static_assert(!Size, "Bad se_storage<> type");
};

template<typename T> struct se_storage<T, 2>
{
	using type = u16;

	[[deprecated]] static constexpr u16 swap(u16 src) // for reference
	{
		return (src >> 8) | (src << 8);
	}

	static inline u16 to(const T& src)
	{
		return _byteswap_ushort(reinterpret_cast<const u16&>(src));
	}

	static inline T from(u16 src)
	{
		const u16 result = _byteswap_ushort(src);
		return reinterpret_cast<const T&>(result);
	}
};

template<typename T> struct se_storage<T, 4>
{
	using type = u32;

	[[deprecated]] static constexpr u32 swap(u32 src) // for reference
	{
		return (src >> 24) | (src << 24) | ((src >> 8) & 0x0000ff00) | ((src << 8) & 0x00ff0000);
	}

	static inline u32 to(const T& src)
	{
		return _byteswap_ulong(reinterpret_cast<const u32&>(src));
	}

	static inline T from(u32 src)
	{
		const u32 result = _byteswap_ulong(src);
		return reinterpret_cast<const T&>(result);
	}
};

template<typename T> struct se_storage<T, 8>
{
	using type = u64;

	[[deprecated]] static constexpr u64 swap(u64 src) // for reference
	{
		return (src >> 56) | (src << 56) |
			((src >> 40) & 0x000000000000ff00) |
			((src >> 24) & 0x0000000000ff0000) |
			((src >> 8)  & 0x00000000ff000000) |
			((src << 8)  & 0x000000ff00000000) |
			((src << 24) & 0x0000ff0000000000) |
			((src << 40) & 0x00ff000000000000);
	}

	static inline u64 to(const T& src)
	{
		return _byteswap_uint64(reinterpret_cast<const u64&>(src));
	}

	static inline T from(u64 src)
	{
		const u64 result = _byteswap_uint64(src);
		return reinterpret_cast<const T&>(result);
	}
};

template<typename T> struct se_storage<T, 16>
{
	using type = v128;

	static inline v128 to(const T& src)
	{
		return v128::byteswap(reinterpret_cast<const v128&>(src));
	}

	static inline T from(const v128& src)
	{
		const v128 result = v128::byteswap(src);
		return reinterpret_cast<const T&>(result);
	}
};

template<typename T> using se_storage_t = typename se_storage<T>::type;

template<typename T1, typename T2> struct se_convert
{
	using type_from = std::remove_cv_t<T1>;
	using type_to = std::remove_cv_t<T2>;
	using stype_from = se_storage_t<std::remove_cv_t<T1>>;
	using stype_to = se_storage_t<std::remove_cv_t<T2>>;
	using storage_from = se_storage<std::remove_cv_t<T1>>;
	using storage_to = se_storage<std::remove_cv_t<T2>>;

	static inline std::enable_if_t<std::is_same<type_from, type_to>::value, stype_to> convert(const stype_from& data)
	{
		return data;
	}

	static inline stype_to convert(const stype_from& data, ...)
	{
		return storage_to::to(storage_from::from(data));
	}
};

static struct se_raw_tag_t {} const se_raw{};

template<typename T, bool Se = true> class se_t;

// se_t with switched endianness
template<typename T> class se_t<T, true>
{
	using type = std::remove_cv_t<T>;
	using stype = se_storage_t<type>;
	using storage = se_storage<type>;

	stype m_data;

	static_assert(!std::is_union<type>::value && !std::is_class<type>::value || std::is_same<type, v128>::value || std::is_same<type, u128>::value, "se_t<> error: invalid type (struct or union)");
	static_assert(!std::is_pointer<type>::value, "se_t<> error: invalid type (pointer)");
	static_assert(!std::is_reference<type>::value, "se_t<> error: invalid type (reference)");
	static_assert(!std::is_array<type>::value, "se_t<> error: invalid type (array)");
	static_assert(!std::is_enum<type>::value, "se_t<> error: invalid type (enumeration), use integral type instead");
	static_assert(alignof(type) == alignof(stype), "se_t<> error: unexpected alignment");

	template<typename T2, bool = std::is_integral<T2>::value> struct bool_converter
	{
		static inline bool to_bool(const se_t<T2>& value)
		{
			return static_cast<bool>(value.value());
		}
	};

	template<typename T2> struct bool_converter<T2, true>
	{
		static inline bool to_bool(const se_t<T2>& value)
		{
			return value.m_data != 0;
		}
	};

public:
	se_t() = default;

	se_t(const se_t& right) = default;

	inline se_t(type value)
		: m_data(storage::to(value))
	{
	}
	
	// construct directly from raw data (don't use)
	inline se_t(const stype& raw_value, const se_raw_tag_t&)
		: m_data(raw_value)
	{
	}

	inline type value() const
	{
		return storage::from(m_data);
	}

	// access underlying raw data (don't use)
	inline const stype& raw_data() const noexcept
	{
		return m_data;
	}
	
	se_t& operator =(const se_t&) = default;

	inline se_t& operator =(type value)
	{
		return m_data = storage::to(value), *this;
	}

	inline operator type() const
	{
		return storage::from(m_data);
	}

	// optimization
	explicit inline operator bool() const
	{
		return bool_converter<type>::to_bool(*this);
	}

	// optimization
	template<typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T, T2), se_t&> operator &=(const se_t<T2>& right)
	{
		return m_data &= right.raw_data(), *this;
	}

	// optimization
	template<typename CT> inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator &=(CT right)
	{
		return m_data &= storage::to(right), *this;
	}

	// optimization
	template<typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T, T2), se_t&> operator |=(const se_t<T2>& right)
	{
		return m_data |= right.raw_data(), *this;
	}

	// optimization
	template<typename CT> inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator |=(CT right)
	{
		return m_data |= storage::to(right), *this;
	}

	// optimization
	template<typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T, T2), se_t&> operator ^=(const se_t<T2>& right)
	{
		return m_data ^= right.raw_data(), *this;
	}

	// optimization
	template<typename CT> inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator ^=(CT right)
	{
		return m_data ^= storage::to(right), *this;
	}
};

// se_t with native endianness
template<typename T> class se_t<T, false>
{
	using type = std::remove_cv_t<T>;

	type m_data;

	static_assert(!std::is_union<type>::value && !std::is_class<type>::value || std::is_same<type, v128>::value || std::is_same<type, u128>::value, "se_t<> error: invalid type (struct or union)");
	static_assert(!std::is_pointer<type>::value, "se_t<> error: invalid type (pointer)");
	static_assert(!std::is_reference<type>::value, "se_t<> error: invalid type (reference)");
	static_assert(!std::is_array<type>::value, "se_t<> error: invalid type (array)");
	static_assert(!std::is_enum<type>::value, "se_t<> error: invalid type (enumeration), use integral type instead");

public:
	se_t() = default;

	se_t(const se_t&) = default;

	inline se_t(type value)
		: m_data(value)
	{
	}

	inline type value() const
	{
		return m_data;
	}

	se_t& operator =(const se_t& value) = default;

	inline se_t& operator =(type value)
	{
		return m_data = value, *this;
	}

	inline operator type() const
	{
		return m_data;
	}

	template<typename CT> inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator &=(const CT& right)
	{
		return m_data &= right, *this;
	}

	template<typename CT> inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator |=(const CT& right)
	{
		return m_data |= right, *this;
	}

	template<typename CT> inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator ^=(const CT& right)
	{
		return m_data ^= right, *this;
	}
};

// se_t with native endianness (alias)
template<typename T> using nse_t = se_t<T, false>;

template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator +=(se_t<T, Se>& left, const T1& right)
{
	auto value = left.value();
	return left = (value += right);
}

template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator -=(se_t<T, Se>& left, const T1& right)
{
	auto value = left.value();
	return left = (value -= right);
}

template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator *=(se_t<T, Se>& left, const T1& right)
{
	auto value = left.value();
	return left = (value *= right);
}

template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator /=(se_t<T, Se>& left, const T1& right)
{
	auto value = left.value();
	return left = (value /= right);
}

template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator %=(se_t<T, Se>& left, const T1& right)
{
	auto value = left.value();
	return left = (value %= right);
}

template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator <<=(se_t<T, Se>& left, const T1& right)
{
	auto value = left.value();
	return left = (value <<= right);
}

template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator >>=(se_t<T, Se>& left, const T1& right)
{
	auto value = left.value();
	return left = (value >>= right);
}

template<typename T, bool Se> inline se_t<T, Se> operator ++(se_t<T, Se>& left, int)
{
	auto value = left.value();
	auto result = value++;
	left = value;
	return result;
}

template<typename T, bool Se> inline se_t<T, Se> operator --(se_t<T, Se>& left, int)
{
	auto value = left.value();
	auto result = value--;
	left = value;
	return result;
}

template<typename T, bool Se> inline se_t<T, Se>& operator ++(se_t<T, Se>& right)
{
	auto value = right.value();
	return right = ++value;
}

template<typename T, bool Se> inline se_t<T, Se>& operator --(se_t<T, Se>& right)
{
	auto value = right.value();
	return right = --value;
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2), bool> operator ==(const se_t<T1>& left, const se_t<T2>& right)
{
	return left.raw_data() == right.raw_data();
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2), bool> operator ==(const se_t<T1>& left, T2 right)
{
	return left.raw_data() == se_storage<T1>::to(right);
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2), bool> operator ==(T1 left, const se_t<T2>& right)
{
	return se_storage<T2>::to(left) == right.raw_data();
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2), bool> operator !=(const se_t<T1>& left, const se_t<T2>& right)
{
	return left.raw_data() != right.raw_data();
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2), bool> operator !=(const se_t<T1>& left, T2 right)
{
	return left.raw_data() != se_storage<T1>::to(right);
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2), bool> operator !=(T1 left, const se_t<T2>& right)
{
	return se_storage<T2>::to(left) != right.raw_data();
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2) && sizeof(T1) >= 4, se_t<decltype(T1() & T2())>> operator &(const se_t<T1>& left, const se_t<T2>& right)
{
	return{ left.raw_data() & right.raw_data(), se_raw };
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2) && sizeof(T1) >= 4, se_t<decltype(T1() & T2())>> operator &(const se_t<T1>& left, T2 right)
{
	return{ left.raw_data() & se_storage<T1>::to(right), se_raw };
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2) && sizeof(T2) >= 4, se_t<decltype(T1() & T2())>> operator &(T1 left, const se_t<T2>& right)
{
	return{ se_storage<T2>::to(left) & right.raw_data(), se_raw };
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2) && sizeof(T1) >= 4, se_t<decltype(T1() | T2())>> operator |(const se_t<T1>& left, const se_t<T2>& right)
{
	return{ left.raw_data() | right.raw_data(), se_raw };
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2) && sizeof(T1) >= 4, se_t<decltype(T1() | T2())>> operator |(const se_t<T1>& left, T2 right)
{
	return{ left.raw_data() | se_storage<T1>::to(right), se_raw };
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2) && sizeof(T2) >= 4, se_t<decltype(T1() | T2())>> operator |(T1 left, const se_t<T2>& right)
{
	return{ se_storage<T2>::to(left) | right.raw_data(), se_raw };
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2) && sizeof(T1) >= 4, se_t<decltype(T1() ^ T2())>> operator ^(const se_t<T1>& left, const se_t<T2>& right)
{
	return{ left.raw_data() ^ right.raw_data(), se_raw };
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2) && sizeof(T1) >= 4, se_t<decltype(T1() ^ T2())>> operator ^(const se_t<T1>& left, T2 right)
{
	return{ left.raw_data() ^ se_storage<T1>::to(right), se_raw };
}

// optimization
template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2) && sizeof(T2) >= 4, se_t<decltype(T1() ^ T2())>> operator ^(T1 left, const se_t<T2>& right)
{
	return{ se_storage<T2>::to(left) ^ right.raw_data(), se_raw };
}

// optimization
template<typename T> inline std::enable_if_t<IS_INTEGRAL(T) && sizeof(T) >= 4, se_t<decltype(~T())>> operator ~(const se_t<T>& right)
{
	return{ ~right.raw_data(), se_raw };
}

#ifdef IS_LE_MACHINE
template<typename T> using be_t = se_t<T, true>;
template<typename T> using le_t = se_t<T, false>;
#else
template<typename T> using be_t = se_t<T, false>;
template<typename T> using le_t = se_t<T, true>;
#endif

template<typename T> struct is_be_t : public std::integral_constant<bool, false>
{
};

template<typename T> struct is_be_t<be_t<T>> : public std::integral_constant<bool, true>
{
};

template<typename T> struct is_be_t<const T> : public std::integral_constant<bool, is_be_t<T>::value>
{
};

template<typename T> struct is_be_t<volatile T> : public std::integral_constant<bool, is_be_t<T>::value>
{
};

// to_be_t helper struct
template<typename T> struct to_be
{
	using type = std::conditional_t<std::is_arithmetic<T>::value || std::is_enum<T>::value || std::is_same<T, v128>::value || std::is_same<T, u128>::value, be_t<T>, T>;
};

// be_t<T> if possible, T otherwise
template<typename T> using to_be_t = typename to_be<T>::type;

template<typename T> struct to_be<const T> // move const qualifier
{
	using type = const to_be_t<T>;
};

template<typename T> struct to_be<volatile T> // move volatile qualifier
{
	using type = volatile to_be_t<T>;
};

template<> struct to_be<void> { using type = void; };
template<> struct to_be<bool> { using type = bool; };
template<> struct to_be<char> { using type = char; };
template<> struct to_be<u8> { using type = u8; };
template<> struct to_be<s8> { using type = s8; };

template<typename T> struct is_le_t : public std::integral_constant<bool, false>
{
};

template<typename T> struct is_le_t<le_t<T>> : public std::integral_constant<bool, true>
{
};

template<typename T> struct is_le_t<const T> : public std::integral_constant<bool, is_le_t<T>::value>
{
};

template<typename T> struct is_le_t<volatile T> : public std::integral_constant<bool, is_le_t<T>::value>
{
};

template<typename T> struct to_le
{
	using type = std::conditional_t<std::is_arithmetic<T>::value || std::is_enum<T>::value || std::is_same<T, v128>::value || std::is_same<T, u128>::value, le_t<T>, T>;
};

// le_t<T> if possible, T otherwise
template<typename T> using to_le_t = typename to_le<T>::type;

template<typename T> struct to_le<const T> // move const qualifier
{
	using type = const to_le_t<T>;
};

template<typename T> struct to_le<volatile T> // move volatile qualifier
{
	using type = volatile to_le_t<T>;
};

template<> struct to_le<void> { using type = void; };
template<> struct to_le<bool> { using type = bool; };
template<> struct to_le<char> { using type = char; };
template<> struct to_le<u8> { using type = u8; };
template<> struct to_le<s8> { using type = s8; };

// to_ne_t helper struct
template<typename T> struct to_ne
{
	using type = T;
};

template<typename T, bool Se> struct to_ne<se_t<T, Se>>
{
	using type = T;
};

// restore native endianness for T: returns T for be_t<T> or le_t<T>, T otherwise
template<typename T> using to_ne_t = typename to_ne<T>::type;

template<typename T> struct to_ne<const T> // move const qualifier
{
	using type = const to_ne_t<T>;
};

template<typename T> struct to_ne<volatile T> // move volatile qualifier
{
	using type = volatile to_ne_t<T>;
};
