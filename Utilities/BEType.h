#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#define IS_LE_MACHINE // only draft

union v128
{
	u64 _u64[2];
	s64 _s64[2];

	class u64_reversed_array_2
	{
		u64 data[2];

	public:
		u64& operator [] (s32 index)
		{
			return data[1 - index];
		}

		const u64& operator [] (s32 index) const
		{
			return data[1 - index];
		}

	} u64r;

	u32 _u32[4];
	s32 _s32[4];

	class u32_reversed_array_4
	{
		u32 data[4];

	public:
		u32& operator [] (s32 index)
		{
			return data[3 - index];
		}

		const u32& operator [] (s32 index) const
		{
			return data[3 - index];
		}

	} u32r;

	u16 _u16[8];
	s16 _s16[8];

	class u16_reversed_array_8
	{
		u16 data[8];

	public:
		u16& operator [] (s32 index)
		{
			return data[7 - index];
		}

		const u16& operator [] (s32 index) const
		{
			return data[7 - index];
		}

	} u16r;

	u8  _u8[16];
	s8  _s8[16];

	class u8_reversed_array_16
	{
		u8 data[16];

	public:
		u8& operator [] (s32 index)
		{
			return data[15 - index];
		}

		const u8& operator [] (s32 index) const
		{
			return data[15 - index];
		}

	} u8r;

	float _f[4];
	double _d[2];
	__m128 vf;
	__m128i vi;
	__m128d vd;

	class bit_array_128
	{
		u64 data[2];

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

			force_inline operator bool() const
			{
				return (data & mask) != 0;
			}

			force_inline bit_element& operator = (const bool right)
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

			force_inline bit_element& operator = (const bit_element& right)
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
		bit_element operator [] (u32 index)
		{
			assert(index < 128);

#ifdef IS_LE_MACHINE
			return bit_element(data[1 - (index >> 6)], 0x8000000000000000ull >> (index & 0x3F));
#else
			return bit_element(data[index >> 6], 0x8000000000000000ull >> (index & 0x3F));
#endif
		}

		// Index 0 returns the MSB and index 127 returns the LSB
		const bool operator [] (u32 index) const
		{
			assert(index < 128);

#ifdef IS_LE_MACHINE
			return (data[1 - (index >> 6)] & (0x8000000000000000ull >> (index & 0x3F))) != 0;
#else
			return (data[index >> 6] & (0x8000000000000000ull >> (index & 0x3F))) != 0;
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

	static force_inline v128 add8(const v128& left, const v128& right)
	{
		return fromV(_mm_add_epi8(left.vi, right.vi));
	}

	static force_inline v128 add16(const v128& left, const v128& right)
	{
		return fromV(_mm_add_epi16(left.vi, right.vi));
	}

	static force_inline v128 add32(const v128& left, const v128& right)
	{
		return fromV(_mm_add_epi32(left.vi, right.vi));
	}

	static force_inline v128 addfs(const v128& left, const v128& right)
	{
		return fromF(_mm_add_ps(left.vf, right.vf));
	}

	static force_inline v128 addfd(const v128& left, const v128& right)
	{
		return fromD(_mm_add_pd(left.vd, right.vd));
	}

	static force_inline v128 sub8(const v128& left, const v128& right)
	{
		return fromV(_mm_sub_epi8(left.vi, right.vi));
	}

	static force_inline v128 sub16(const v128& left, const v128& right)
	{
		return fromV(_mm_sub_epi16(left.vi, right.vi));
	}

	static force_inline v128 sub32(const v128& left, const v128& right)
	{
		return fromV(_mm_sub_epi32(left.vi, right.vi));
	}

	static force_inline v128 subfs(const v128& left, const v128& right)
	{
		return fromF(_mm_sub_ps(left.vf, right.vf));
	}

	static force_inline v128 subfd(const v128& left, const v128& right)
	{
		return fromD(_mm_sub_pd(left.vd, right.vd));
	}

	static force_inline v128 maxu8(const v128& left, const v128& right)
	{
		return fromV(_mm_max_epu8(left.vi, right.vi));
	}

	static force_inline v128 minu8(const v128& left, const v128& right)
	{
		return fromV(_mm_min_epu8(left.vi, right.vi));
	}

	static force_inline v128 eq8(const v128& left, const v128& right)
	{
		return fromV(_mm_cmpeq_epi8(left.vi, right.vi));
	}

	static force_inline v128 eq16(const v128& left, const v128& right)
	{
		return fromV(_mm_cmpeq_epi16(left.vi, right.vi));
	}

	static force_inline v128 eq32(const v128& left, const v128& right)
	{
		return fromV(_mm_cmpeq_epi32(left.vi, right.vi));
	}

	bool operator == (const v128& right) const
	{
		return (_u64[0] == right._u64[0]) && (_u64[1] == right._u64[1]);
	}

	bool operator != (const v128& right) const
	{
		return (_u64[0] != right._u64[0]) || (_u64[1] != right._u64[1]);
	}

	force_inline bool is_any_1() const // check if any bit is 1
	{
		return _u64[0] || _u64[1];
	}

	force_inline bool is_any_0() const // check if any bit is 0
	{
		return ~_u64[0] || ~_u64[1];
	}

	// result = (~left) & (right)
	static force_inline v128 andnot(const v128& left, const v128& right)
	{
		return fromV(_mm_andnot_si128(left.vi, right.vi));
	}

	void clear()
	{
		_u64[1] = _u64[0] = 0;
	}

	std::string to_hex() const;

	std::string to_xyzw() const;

	static force_inline v128 byteswap(const v128 val)
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

static force_inline v128 sync_val_compare_and_swap(volatile v128* dest, v128 comp, v128 exch)
{
#if !defined(_MSC_VER)
	auto res = __sync_val_compare_and_swap((volatile __int128_t*)dest, (__int128_t&)comp, (__int128_t&)exch);
	return (v128&)res;
#else
	_InterlockedCompareExchange128((volatile long long*)dest, exch._u64[1], exch._u64[0], (long long*)&comp);
	return comp;
#endif
}

static force_inline bool sync_bool_compare_and_swap(volatile v128* dest, v128 comp, v128 exch)
{
#if !defined(_MSC_VER)
	return __sync_bool_compare_and_swap((volatile __int128_t*)dest, (__int128_t&)comp, (__int128_t&)exch);
#else
	return _InterlockedCompareExchange128((volatile long long*)dest, exch._u64[1], exch._u64[0], (long long*)&comp) != 0;
#endif
}

static force_inline v128 sync_lock_test_and_set(volatile v128* dest, v128 value)
{
	while (true)
	{
		const v128 old = *(v128*)dest;
		if (sync_bool_compare_and_swap(dest, old, value)) return old;
	}
}

static force_inline v128 sync_fetch_and_or(volatile v128* dest, v128 value)
{
	while (true)
	{
		const v128 old = *(v128*)dest;
		if (sync_bool_compare_and_swap(dest, old, value | old)) return old;
	}
}

static force_inline v128 sync_fetch_and_and(volatile v128* dest, v128 value)
{
	while (true)
	{
		const v128 old = *(v128*)dest;
		if (sync_bool_compare_and_swap(dest, old, value & old)) return old;
	}
}

static force_inline v128 sync_fetch_and_xor(volatile v128* dest, v128 value)
{
	while (true)
	{
		const v128 old = *(v128*)dest;
		if (sync_bool_compare_and_swap(dest, old, value ^ old)) return old;
	}
}

template<typename T, std::size_t Size = sizeof(T)> struct se_t;

template<typename T> struct se_t<T, 2>
{
	static force_inline u16 to(const T& src)
	{
		return _byteswap_ushort((u16&)src);
	}

	static force_inline T from(const u16 src)
	{
		const u16 res = _byteswap_ushort(src);
		return (T&)res;
	}
};

template<typename T> struct se_t<T, 4>
{
	static force_inline u32 to(const T& src)
	{
		return _byteswap_ulong((u32&)src);
	}

	static force_inline T from(const u32 src)
	{
		const u32 res = _byteswap_ulong(src);
		return (T&)res;
	}
};

template<typename T> struct se_t<T, 8>
{
	static force_inline u64 to(const T& src)
	{
		return _byteswap_uint64((u64&)src);
	}

	static force_inline T from(const u64 src)
	{
		const u64 res = _byteswap_uint64(src);
		return (T&)res;
	}
};

template<typename T> struct se_t<T, 16>
{
	static force_inline v128 to(const T& src)
	{
		return v128::byteswap((v128&)src);
	}

	static force_inline T from(const v128& src)
	{
		const v128 res = v128::byteswap(src);
		return (T&)res;
	}
};

template<typename T, T _value, std::size_t size = sizeof(T)> struct const_se_t;

template<u16 _value> struct const_se_t<u16, _value, 2>
{
	static const u16 value =
		((_value >> 8) & 0x00ff) |
		((_value << 8) & 0xff00);
};

template<u32 _value> struct const_se_t<u32, _value, 4>
{
	static const u32 value = 
		((_value >> 24) & 0x000000ff) |
		((_value >>  8) & 0x0000ff00) |
		((_value <<  8) & 0x00ff0000) |
		((_value << 24) & 0xff000000);
};

template<u64 _value> struct const_se_t<u64, _value, 8>
{
	static const u64 value = 
		((_value >> 56) & 0x00000000000000ff) |
		((_value >> 40) & 0x000000000000ff00) |
		((_value >> 24) & 0x0000000000ff0000) |
		((_value >>  8) & 0x00000000ff000000) |
		((_value <<  8) & 0x000000ff00000000) |
		((_value << 24) & 0x0000ff0000000000) |
		((_value << 40) & 0x00ff000000000000) |
		((_value << 56) & 0xff00000000000000);
};

template<typename T, size_t size = sizeof(T)> struct be_storage
{
	static_assert(!size, "Bad be_storage_t<> type");
};

template<typename T> struct be_storage<T, 2>
{
	using type = u16;
};

template<typename T> struct be_storage<T, 4>
{
	using type = u32;
};

template<typename T> struct be_storage<T, 8>
{
	using type = u64;
};

template<typename T> struct be_storage<T, 16>
{
	using type = v128;
};

template<typename T> using be_storage_t = typename be_storage<T>::type;

template<typename T> class be_t
{
	// TODO (complicated cases like int-float conversions are not handled correctly)
	template<typename Tto, typename Tfrom, int mode>
	struct _convert
	{
		static force_inline be_t<Tto>& func(Tfrom& be_value)
		{
			Tto res = be_value;
			return (be_t<Tto>&)res;
		}
	};

	template<typename Tto, typename Tfrom>
	struct _convert<Tto, Tfrom, 1>
	{
		static force_inline be_t<Tto>& func(Tfrom& be_value)
		{
			Tto res = se_t<Tto, sizeof(Tto)>::func(se_t<Tfrom, sizeof(Tfrom)>::func(be_value));
			return (be_t<Tto>&)res;
		}
	};

	template<typename Tto, typename Tfrom>
	struct _convert<Tto, Tfrom, 2>
	{
		static force_inline be_t<Tto>& func(Tfrom& be_value)
		{
			Tto res = be_value >> ((sizeof(Tfrom) - sizeof(Tto)) * 8);
			return (be_t<Tto>&)res;
		}
	};

public:
	using type = std::remove_cv_t<T>;
	using stype = be_storage_t<std::remove_cv_t<T>>;

#ifdef IS_LE_MACHINE
	stype m_data; // don't access directly
#else
	type m_data; // don't access directly
#endif

	static_assert(!std::is_class<type>::value, "be_t<> error: invalid type (class or structure)");
	static_assert(!std::is_union<type>::value || std::is_same<type, v128>::value, "be_t<> error: invalid type (union)");
	static_assert(!std::is_pointer<type>::value, "be_t<> error: invalid type (pointer)");
	static_assert(!std::is_reference<type>::value, "be_t<> error: invalid type (reference)");
	static_assert(!std::is_array<type>::value, "be_t<> error: invalid type (array)");
	static_assert(!std::is_enum<type>::value, "be_t<> error: invalid type (enumeration), use integral type instead");
	static_assert(__alignof(type) == __alignof(stype), "be_t<> error: unexpected alignment");

	be_t() = default;

	be_t(const be_t&) = default;

	template<typename = std::enable_if_t<std::is_constructible<type, type>::value>> be_t(const type& value)
#ifdef IS_LE_MACHINE
		: m_data(se_t<type, sizeof(stype)>::to(value))
#else
		: m_data(value)
#endif
	{
	}

	// get value in current machine byte ordering
	force_inline type value() const
	{
#ifdef IS_LE_MACHINE
		return se_t<type, sizeof(stype)>::from(m_data);
#else
		return m_data;
#endif
	}

	// get underlying data without any byte order manipulation
	const stype& data() const
	{
#ifdef IS_LE_MACHINE
		return m_data;
#else
		return reinterpret_cast<const stype&>(m_data);
#endif
	}
	
	be_t& operator =(const be_t&) = default;

	template<typename CT> std::enable_if_t<std::is_assignable<type&, CT>::value, be_t&> operator =(const CT& value)
	{
#ifdef IS_LE_MACHINE
		m_data = se_t<type, sizeof(stype)>::to(value);
#else
		m_data = value;
#endif

		return *this;
	}

	//template<typename CT, typename = std::enable_if_t<std::is_convertible<type, CT>::value>> operator CT() const
	//{
	//	return value();
	//}

	operator type() const
	{
		return value();
	}

	// conversion to another be_t type
	//template<typename T1> operator be_t<T1>() const
	//{
	//	return value();
	//	//return _convert<T1, T, ((sizeof(T1) > sizeof(T)) ? 1 : (sizeof(T1) < sizeof(T) ? 2 : 0))>::func(m_data);
	//}

	template<typename T1> be_t& operator +=(const T1& right) { return *this = value() + right; }
	template<typename T1> be_t& operator -=(const T1& right) { return *this = value() - right; }
	template<typename T1> be_t& operator *=(const T1& right) { return *this = value() * right; }
	template<typename T1> be_t& operator /=(const T1& right) { return *this = value() / right; }
	template<typename T1> be_t& operator %=(const T1& right) { return *this = value() % right; }

	template<typename T1> be_t& operator <<=(const T1& right) { return *this = value() << right; }
	template<typename T1> be_t& operator >>=(const T1& right) { return *this = value() >> right; }

	template<typename T1> be_t& operator &=(const T1& right) { return m_data &= be_t(right).data(), *this; }
	template<typename T1> be_t& operator |=(const T1& right) { return m_data |= be_t(right).data(), *this; }
	template<typename T1> be_t& operator ^=(const T1& right) { return m_data ^= be_t(right).data(), *this; }

	be_t operator ++(int) { be_t res = *this; *this += 1; return res; }
	be_t operator --(int) { be_t res = *this; *this -= 1; return res; }
	be_t& operator ++() { *this += 1; return *this; }
	be_t& operator --() { *this -= 1; return *this; }
};

template<typename T1, typename T2> inline std::enable_if_t<std::is_same<T1, T2>::value && std::is_integral<T1>::value, bool> operator ==(const be_t<T1>& left, const be_t<T2>& right)
{
	return left.data() == right.data();
}

template<typename T1, typename T2> inline std::enable_if_t<std::is_same<T1, T2>::value && std::is_integral<T1>::value, bool> operator !=(const be_t<T1>& left, const be_t<T2>& right)
{
	return left.data() != right.data();
}

template<typename T1, typename T2> inline std::enable_if_t<std::is_same<T1, T2>::value && std::is_integral<T1>::value, be_t<T1>> operator &(const be_t<T1>& left, const be_t<T2>& right)
{
	be_t<T1> result;
	result.m_data = left.data() & right.data();
	return result;
}

template<typename T1, typename T2> inline std::enable_if_t<std::is_same<T1, T2>::value && std::is_integral<T1>::value, be_t<T1>> operator |(const be_t<T1>& left, const be_t<T2>& right)
{
	be_t<T1> result;
	result.m_data = left.data() | right.data();
	return result;
}

template<typename T1, typename T2> inline std::enable_if_t<std::is_same<T1, T2>::value && std::is_integral<T1>::value, be_t<T1>> operator ^(const be_t<T1>& left, const be_t<T2>& right)
{
	be_t<T1> result;
	result.m_data = left.data() ^ right.data();
	return result;
}

template<typename T1> inline std::enable_if_t<std::is_integral<T1>::value, be_t<T1>> operator ~(const be_t<T1>& arg)
{
	be_t<T1> result;
	result.m_data = ~arg.data();
	return result;
}

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
	using type = std::conditional_t<std::is_arithmetic<T>::value || std::is_enum<T>::value || std::is_same<T, v128>::value, be_t<T>, T>;
};

// be_t<T> if possible, T otherwise
template<typename T> using to_be_t = typename to_be<T>::type;

template<typename T> struct to_be<const T>
{
	// move const qualifier
	using type = const to_be_t<T>;
};

template<typename T> struct to_be<volatile T>
{
	// move volatile qualifier
	using type = volatile to_be_t<T>;
};

template<> struct to_be<void> { using type = void; };
template<> struct to_be<bool> { using type = bool; };
template<> struct to_be<char> { using type = char; };
template<> struct to_be<u8> { using type = u8; };
template<> struct to_be<s8> { using type = s8; };

template<typename T> class le_t
{
public:
	using type = std::remove_cv_t<T>;
	using stype = be_storage_t<std::remove_cv_t<T>>;

	type m_data; // don't access directly

	static_assert(!std::is_class<type>::value, "le_t<> error: invalid type (class or structure)");
	static_assert(!std::is_union<type>::value || std::is_same<type, v128>::value, "le_t<> error: invalid type (union)");
	static_assert(!std::is_pointer<type>::value, "le_t<> error: invalid type (pointer)");
	static_assert(!std::is_reference<type>::value, "le_t<> error: invalid type (reference)");
	static_assert(!std::is_array<type>::value, "le_t<> error: invalid type (array)");
	static_assert(!std::is_enum<type>::value, "le_t<> error: invalid type (enumeration), use integral type instead");
	static_assert(__alignof(type) == __alignof(stype), "le_t<> error: unexpected alignment");

	le_t() = default;

	le_t(const le_t&) = default;

	template<typename = std::enable_if_t<std::is_constructible<type, type>::value>> le_t(const type& value)
		: m_data(value)
	{
	}

	type value() const
	{
		return m_data;
	}

	const stype& data() const
	{
		return reinterpret_cast<const stype&>(m_data);
	}

	le_t& operator =(const le_t& value) = default;

	template<typename CT> std::enable_if_t<std::is_assignable<type&, CT>::value, le_t&> operator =(const CT& value)
	{
		m_data = value;
		return *this;
	}

	operator type() const
	{
		return value();
	}

	// conversion to another le_t type
	//template<typename T1> operator le_t<T1>() const
	//{
	//	return value();
	//}

	template<typename T1> le_t& operator +=(const T1& right) { return *this = value() + right; }
	template<typename T1> le_t& operator -=(const T1& right) { return *this = value() - right; }
	template<typename T1> le_t& operator *=(const T1& right) { return *this = value() * right; }
	template<typename T1> le_t& operator /=(const T1& right) { return *this = value() / right; }
	template<typename T1> le_t& operator %=(const T1& right) { return *this = value() % right; }

	template<typename T1> le_t& operator <<=(const T1& right) { return *this = value() << right; }
	template<typename T1> le_t& operator >>=(const T1& right) { return *this = value() >> right; }

	template<typename T1> le_t& operator &=(const T1& right) { return m_data &= le_t(right).data(), *this; }
	template<typename T1> le_t& operator |=(const T1& right) { return m_data |= le_t(right).data(), *this; }
	template<typename T1> le_t& operator ^=(const T1& right) { return m_data ^= le_t(right).data(), *this; }

	le_t operator ++(int) { le_t res = *this; *this += 1; return res; }
	le_t operator --(int) { le_t res = *this; *this -= 1; return res; }
	le_t& operator ++() { *this += 1; return *this; }
	le_t& operator --() { *this -= 1; return *this; }
};

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
	using type = std::conditional_t<std::is_arithmetic<T>::value || std::is_enum<T>::value || std::is_same<T, v128>::value, le_t<T>, T>;
};

// le_t<T> if possible, T otherwise
template<typename T> using to_le_t = typename to_le<T>::type;

template<typename T> struct to_le<const T>
{
	// move const qualifier
	using type = const to_le_t<T>;
};

template<typename T> struct to_le<volatile T>
{
	// move volatile qualifier
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

template<typename T> struct to_ne<be_t<T>>
{
	using type = T;
};

template<typename T> struct to_ne<le_t<T>>
{
	using type = T;
};

// restore native endianness for T: returns T for be_t<T> or le_t<T>, T otherwise
template<typename T> using to_ne_t = typename to_ne<T>::type;

template<typename T> struct to_ne<const T>
{
	// move const qualifier
	using type = const to_ne_t<T>;
};

template<typename T> struct to_ne<volatile T>
{
	// move volatile qualifier
	using type = volatile to_ne_t<T>;
};
