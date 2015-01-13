#pragma once

union _CRT_ALIGN(16) u128
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

			__forceinline operator bool() const
			{
				return (data & mask) != 0;
			}

			__forceinline bit_element& operator = (const bool right)
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

			__forceinline bit_element& operator = (const bit_element& right)
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

		bit_element operator [] (u32 index)
		{
			assert(index < 128);
			return bit_element(data[index / 64], 1ull << (index % 64));
		}

		const bool operator [] (u32 index) const
		{
			assert(index < 128);
			return (data[index / 64] & (1ull << (index % 64))) != 0;
		}

	} _bit;

	//operator u64() const { return _u64[0]; }
	//operator u32() const { return _u32[0]; }
	//operator u16() const { return _u16[0]; }
	//operator u8()  const { return _u8[0]; }

	//operator bool() const { return _u64[0] != 0 || _u64[1] != 0; }

	static u128 from64(u64 _0, u64 _1 = 0)
	{
		u128 ret;
		ret._u64[0] = _0;
		ret._u64[1] = _1;
		return ret;
	}

	static u128 from64r(u64 _1, u64 _0 = 0)
	{
		return from64(_0, _1);
	}

	static u128 from32(u32 _0, u32 _1 = 0, u32 _2 = 0, u32 _3 = 0)
	{
		u128 ret;
		ret._u32[0] = _0;
		ret._u32[1] = _1;
		ret._u32[2] = _2;
		ret._u32[3] = _3;
		return ret;
	}

	static u128 from32r(u32 _3, u32 _2 = 0, u32 _1 = 0, u32 _0 = 0)
	{
		return from32(_0, _1, _2, _3);
	}

	static u128 from32p(u32 value)
	{
		u128 ret;
		ret.vi = _mm_set1_epi32((int)value);
		return ret;
	}

	static u128 from8p(u8 value)
	{
		u128 ret;
		ret.vi = _mm_set1_epi8((char)value);
		return ret;
	}

	static u128 fromBit(u32 bit)
	{
		u128 ret = {};
		ret._bit[bit] = true;
		return ret;
	}

	static u128 fromV(__m128i value)
	{
		u128 ret;
		ret.vi = value;
		return ret;
	}

	static __forceinline u128 add8(const u128& left, const u128& right)
	{
		return fromV(_mm_add_epi8(left.vi, right.vi));
	}

	static __forceinline u128 sub8(const u128& left, const u128& right)
	{
		return fromV(_mm_sub_epi8(left.vi, right.vi));
	}

	static __forceinline u128 minu8(const u128& left, const u128& right)
	{
		return fromV(_mm_min_epu8(left.vi, right.vi));
	}

	static __forceinline u128 eq8(const u128& left, const u128& right)
	{
		return fromV(_mm_cmpeq_epi8(left.vi, right.vi));
	}

	static __forceinline u128 gtu8(const u128& left, const u128& right)
	{
		return fromV(_mm_cmpgt_epu8(left.vi, right.vi));
	}

	static __forceinline u128 leu8(const u128& left, const u128& right)
	{
		return fromV(_mm_cmple_epu8(left.vi, right.vi));
	}

	bool operator == (const u128& right) const
	{
		return (_u64[0] == right._u64[0]) && (_u64[1] == right._u64[1]);
	}

	bool operator != (const u128& right) const
	{
		return (_u64[0] != right._u64[0]) || (_u64[1] != right._u64[1]);
	}

	__forceinline u128 operator | (const u128& right) const
	{
		return fromV(_mm_or_si128(vi, right.vi));
	}

	__forceinline u128 operator & (const u128& right) const
	{
		return fromV(_mm_and_si128(vi, right.vi));
	}

	__forceinline u128 operator ^ (const u128& right) const
	{
		return fromV(_mm_xor_si128(vi, right.vi));
	}

	u128 operator ~ () const
	{
		return from64(~_u64[0], ~_u64[1]);
	}

	// result = (~left) & (right)
	static __forceinline u128 andnot(const u128& left, const u128& right)
	{
		return fromV(_mm_andnot_si128(left.vi, right.vi));
	}

	void clear()
	{
		_u64[1] = _u64[0] = 0;
	}

	std::string to_hex() const;

	std::string to_xyzw() const;

	static __forceinline u128 byteswap(const u128 val)
	{
		u128 ret;
		ret._u64[0] = _byteswap_uint64(val._u64[1]);
		ret._u64[1] = _byteswap_uint64(val._u64[0]);
		return ret;
	}
};

#ifndef InterlockedCompareExchange
static __forceinline u128 InterlockedCompareExchange(volatile u128* dest, u128 exch, u128 comp)
{
#if defined(__GNUG__)
	auto res = __sync_val_compare_and_swap((volatile __int128_t*)dest, (__int128_t&)comp, (__int128_t&)exch);
	return (u128&)res;
#else
	_InterlockedCompareExchange128((volatile long long*)dest, exch._u64[1], exch._u64[0], (long long*)&comp);
	return comp;
#endif
}
#endif

static __forceinline bool InterlockedCompareExchangeTest(volatile u128* dest, u128 exch, u128 comp)
{
#if defined(__GNUG__)
	return __sync_bool_compare_and_swap((volatile __int128_t*)dest, (__int128_t&)comp, (__int128_t&)exch);
#else
	return _InterlockedCompareExchange128((volatile long long*)dest, exch._u64[1], exch._u64[0], (long long*)&comp) != 0;
#endif
}

#ifndef InterlockedExchange
static __forceinline u128 InterlockedExchange(volatile u128* dest, u128 value)
{
	while (true)
	{
		const u128 old = *(u128*)dest;
		if (InterlockedCompareExchangeTest(dest, value, old)) return old;
	}
}
#endif

#ifndef InterlockedOr
static __forceinline u128 InterlockedOr(volatile u128* dest, u128 value)
{
	while (true)
	{
		const u128 old = *(u128*)dest;
		if (InterlockedCompareExchangeTest(dest, old | value, old)) return old;
	}
}
#endif

#ifndef InterlockedAnd
static __forceinline u128 InterlockedAnd(volatile u128* dest, u128 value)
{
	while (true)
	{
		const u128 old = *(u128*)dest;
		if (InterlockedCompareExchangeTest(dest, old & value, old)) return old;
	}
}
#endif

#ifndef InterlockedXor
static __forceinline u128 InterlockedXor(volatile u128* dest, u128 value)
{
	while (true)
	{
		const u128 old = *(u128*)dest;
		if (InterlockedCompareExchangeTest(dest, old ^ value, old)) return old;
	}
}
#endif

#define re16(val) _byteswap_ushort(val)
#define re32(val) _byteswap_ulong(val)
#define re64(val) _byteswap_uint64(val)
#define re128(val) u128::byteswap(val)

template<typename T, int size = sizeof(T)> struct se_t;

template<typename T> struct se_t<T, 1>
{
	static __forceinline u8 to_be(const T& src)
	{
		return (u8&)src;
	}

	static __forceinline T from_be(const u8 src)
	{
		return (T&)src;
	}
};

template<typename T> struct se_t<T, 2>
{
	static __forceinline u16 to_be(const T& src)
	{
		return _byteswap_ushort((u16&)src);
	}

	static __forceinline T from_be(const u16 src)
	{
		const u16 res = _byteswap_ushort(src);
		return (T&)res;
	}
};

template<typename T> struct se_t<T, 4>
{
	static __forceinline u32 to_be(const T& src)
	{
		return _byteswap_ulong((u32&)src);
	}

	static __forceinline T from_be(const u32 src)
	{
		const u32 res = _byteswap_ulong(src);
		return (T&)res;
	}
};

template<typename T> struct se_t<T, 8>
{
	static __forceinline u64 to_be(const T& src)
	{
		return _byteswap_uint64((u64&)src);
	}

	static __forceinline T from_be(const u64 src)
	{
		const u64 res = _byteswap_uint64(src);
		return (T&)res;
	}
};

template<typename T> struct se_t<T, 16>
{
	static __forceinline u128 to_be(const T& src)
	{
		return u128::byteswap((u128&)src);
	}

	static __forceinline T from_be(const u128& src)
	{
		const u128 res = u128::byteswap(src);
		return (T&)res;
	}
};

template<typename T, T _value, size_t size = sizeof(T)> struct const_se_t;

template<u8 _value> struct const_se_t<u8, _value, 1>
{
	static const u8 value = _value;
};

template<u16 _value> struct const_se_t<u16, _value, 2>
{
	static const u16 value = ((_value >> 8) & 0xff) | ((_value << 8) & 0xff00);
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

template<typename T, size_t size = sizeof(T)>
struct be_storage_t
{
	static_assert(!size, "Bad be_storage_t type");
};

template<typename T>
struct be_storage_t<T, 1>
{
	typedef u8 type;
};

template<typename T>
struct be_storage_t<T, 2>
{
	typedef u16 type;
};

template<typename T>
struct be_storage_t<T, 4>
{
	typedef u32 type;
};

template<typename T>
struct be_storage_t<T, 8>
{
	typedef u64 type;
};

template<typename T>
struct be_storage_t<T, 16>
{
	typedef u128 type;
};

#define IS_LE_MACHINE

template<typename T, typename T2 = T>
class be_t
{
public:
	typedef typename std::remove_cv<T>::type type;
	typedef typename be_storage_t<T2>::type stype;

private:
	stype m_data;

	template<typename Tto, typename Tfrom, int mode>
	struct _convert
	{
		static __forceinline be_t<Tto>& func(Tfrom& be_value)
		{
			Tto res = be_value;
			return (be_t<Tto>&)res;
		}
	};

	template<typename Tto, typename Tfrom>
	struct _convert<Tto, Tfrom, 1>
	{
		static __forceinline be_t<Tto>& func(Tfrom& be_value)
		{
			Tto res = se_t<Tto, sizeof(Tto)>::func(se_t<Tfrom, sizeof(Tfrom)>::func(be_value));
			return (be_t<Tto>&)res;
		}
	};

	template<typename Tto, typename Tfrom>
	struct _convert<Tto, Tfrom, 2>
	{
		static __forceinline be_t<Tto>& func(Tfrom& be_value)
		{
			Tto res = be_value >> ((sizeof(Tfrom)-sizeof(Tto)) * 8);
			return (be_t<Tto>&)res;
		}
	};

	const stype& ToBE() const
	{
		return m_data;
	}

	type ToLE() const
	{
		return se_t<type, sizeof(T2)>::from_be(m_data);
	}

	void FromBE(const stype& value)
	{
		m_data = value;
	}

	void FromLE(const type& value)
	{
		m_data = se_t<type, sizeof(T2)>::to_be(value);
	}

public:
	static be_t MakeFromLE(const type& value)
	{
		stype data = se_t<type, sizeof(T2)>::to_be(value);
		return (be_t&)data;
	}

	static be_t MakeFromBE(const stype& value)
	{
		return (be_t&)value;
	}

	//make be_t from current machine byte ordering
	static be_t make(const type& value)
	{
#ifdef IS_LE_MACHINE
		return MakeFromLE(value);
#else
		return MakeFromBE(value);
#endif
	}

	//get value in current machine byte ordering
	__forceinline type value() const
	{
#ifdef IS_LE_MACHINE
		return ToLE();
#else
		return ToBE();
#endif
	}

	const stype& data() const
	{
		return ToBE();
	}
	
	be_t& operator = (const be_t& value) = default;

	be_t& operator = (const type& value)
	{
		m_data = se_t<type, sizeof(T2)>::to_be(value);

		return *this;
	}

	operator type() const
	{
		return value();
	}

	template<typename T1>
	operator const be_t<T1>() const
	{
		return be_t<T1>::make(value());
		//return _convert<T1, T, ((sizeof(T1) > sizeof(T)) ? 1 : (sizeof(T1) < sizeof(T) ? 2 : 0))>::func(m_data);
	}

	template<typename T1> be_t& operator += (T1 right) { return *this = T(*this) + right; }
	template<typename T1> be_t& operator -= (T1 right) { return *this = T(*this) - right; }
	template<typename T1> be_t& operator *= (T1 right) { return *this = T(*this) * right; }
	template<typename T1> be_t& operator /= (T1 right) { return *this = T(*this) / right; }
	template<typename T1> be_t& operator %= (T1 right) { return *this = T(*this) % right; }
	template<typename T1> be_t& operator &= (T1 right) { return *this = T(*this) & right; }
	template<typename T1> be_t& operator |= (T1 right) { return *this = T(*this) | right; }
	template<typename T1> be_t& operator ^= (T1 right) { return *this = T(*this) ^ right; }
	template<typename T1> be_t& operator <<= (T1 right) { return *this = T(*this) << right; }
	template<typename T1> be_t& operator >>= (T1 right) { return *this = T(*this) >> right; }

	template<typename T1> be_t& operator += (const be_t<T1>& right) { return *this = ToLE() + right.ToLE(); }
	template<typename T1> be_t& operator -= (const be_t<T1>& right) { return *this = ToLE() - right.ToLE(); }
	template<typename T1> be_t& operator *= (const be_t<T1>& right) { return *this = ToLE() * right.ToLE(); }
	template<typename T1> be_t& operator /= (const be_t<T1>& right) { return *this = ToLE() / right.ToLE(); }
	template<typename T1> be_t& operator %= (const be_t<T1>& right) { return *this = ToLE() % right.ToLE(); }
	template<typename T1> be_t& operator &= (const be_t<T1>& right) { return *this = ToBE() & right.ToBE(); }
	template<typename T1> be_t& operator |= (const be_t<T1>& right) { return *this = ToBE() | right.ToBE(); }
	template<typename T1> be_t& operator ^= (const be_t<T1>& right) { return *this = ToBE() ^ right.ToBE(); }

	template<typename T1> be_t operator & (const be_t<T1>& right) const { be_t<T> res; res.FromBE(ToBE() & right.ToBE()); return res; }
	template<typename T1> be_t operator | (const be_t<T1>& right) const { be_t<T> res; res.FromBE(ToBE() | right.ToBE()); return res; }
	template<typename T1> be_t operator ^ (const be_t<T1>& right) const { be_t<T> res; res.FromBE(ToBE() ^ right.ToBE()); return res; }

	template<typename T1> bool operator == (T1 right) const { return (T1)ToLE() == right; }
	template<typename T1> bool operator != (T1 right) const { return !(*this == right); }
	template<typename T1> bool operator >  (T1 right) const { return (T1)ToLE() >  right; }
	template<typename T1> bool operator <  (T1 right) const { return (T1)ToLE() <  right; }
	template<typename T1> bool operator >= (T1 right) const { return (T1)ToLE() >= right; }
	template<typename T1> bool operator <= (T1 right) const { return (T1)ToLE() <= right; }

	template<typename T1> bool operator == (const be_t<T1>& right) const { return ToBE() == right.ToBE(); }
	template<typename T1> bool operator != (const be_t<T1>& right) const { return !(*this == right); }
	template<typename T1> bool operator >  (const be_t<T1>& right) const { return (T1)ToLE() >  right.ToLE(); }
	template<typename T1> bool operator <  (const be_t<T1>& right) const { return (T1)ToLE() <  right.ToLE(); }
	template<typename T1> bool operator >= (const be_t<T1>& right) const { return (T1)ToLE() >= right.ToLE(); }
	template<typename T1> bool operator <= (const be_t<T1>& right) const { return (T1)ToLE() <= right.ToLE(); }

	be_t operator++ (int) { be_t res = *this; *this += 1; return res; }
	be_t operator-- (int) { be_t res = *this; *this -= 1; return res; }
	be_t& operator++ () { *this += 1; return *this; }
	be_t& operator-- () { *this -= 1; return *this; }
};

template<typename T, typename T2 = T>
struct is_be_t : public std::integral_constant<bool, false> {};

template<typename T, typename T2>
struct is_be_t<be_t<T, T2>, T2> : public std::integral_constant<bool, true> {};

template<typename T, typename T2 = T>
struct remove_be_t
{
	typedef T type;
};

template<typename T, typename T2>
struct remove_be_t<be_t<T, T2>>
{
	typedef T type;
};

template<typename T, typename T2 = T>
class to_be_t
{
	template<typename TT, typename TT2, bool is_need_swap>
	struct _be_type_selector
	{
		typedef TT type;
	};

	template<typename TT, typename TT2>
	struct _be_type_selector<TT, TT2, true>
	{
		typedef be_t<TT, TT2> type;
	};

public:
	//true if need swap endianes for be
	static const bool value = (sizeof(T2) > 1) && (std::is_arithmetic<T>::value || std::is_enum<T>::value);

	//be_t<T, size> if need swap endianes, T otherwise
	typedef typename _be_type_selector< T, T2, value >::type type;

	typedef typename _be_type_selector< T, T2, !is_be_t<T, T2>::value >::type forced_type;
};

template<typename T>
class to_be_t<T, void>
{
public:
	//true if need swap endianes for be
	static const bool value = false;

	//be_t<T, size> if need swap endianes, T otherwise
	typedef void type;
};

template<typename T>
class to_be_t<T, const void>
{
public:
	//true if need swap endianes for be
	static const bool value = false;

	//be_t<T, size> if need swap endianes, T otherwise
	typedef const void type;
};

template<typename T, typename T2 = T>
struct invert_be_t
{
	typedef typename to_be_t<T, T2>::type type;
};

template<typename T, typename T2>
struct invert_be_t<be_t<T, T2>>
{
	typedef T type;
};

template<typename T, typename T1, T1 value> struct _se : public const_se_t<T, value> {};
template<typename T, typename T1, T1 value> struct _se<be_t<T>, T1, value> : public const_se_t<T, value> {};

//#define se(t, x) _se<decltype(t), decltype(x), x>::value
#define se16(x) _se<u16, decltype(x), x>::value
#define se32(x) _se<u32, decltype(x), x>::value
#define se64(x) _se<u64, decltype(x), x>::value

template<typename T> __forceinline static u8 Read8(T& f)
{
	u8 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline static u16 Read16(T& f)
{
	be_t<u16> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline static u32 Read32(T& f)
{
	be_t<u32> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline static u64 Read64(T& f)
{
	be_t<u64> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline static u16 Read16LE(T& f)
{
	u16 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline static u32 Read32LE(T& f)
{
	u32 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline static u64 Read64LE(T& f)
{
	u64 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline static void Write8(T& f, const u8 data)
{
	f.Write(&data, sizeof(data));
}

template<typename T> __forceinline static void Write16LE(T& f, const u16 data)
{
	f.Write(&data, sizeof(data));
}

template<typename T> __forceinline static void Write32LE(T& f, const u32 data)
{
	f.Write(&data, sizeof(data));
}

template<typename T> __forceinline static void Write64LE(T& f, const u64 data)
{
	f.Write(&data, sizeof(data));
}

template<typename T> __forceinline static void Write16(T& f, const u16 data)
{
	Write16LE(f, re16(data));
}

template<typename T> __forceinline static void Write32(T& f, const u32 data)
{
	Write32LE(f, re32(data));
}

template<typename T> __forceinline static void Write64(T& f, const u64 data)
{
	Write64LE(f, re64(data));
}

template<typename Tto, typename Tfrom>
struct convert_le_be_t
{
	static Tto func(Tfrom&& value)
	{
		return (Tto)value;
	}
};

template<typename Tt, typename Tt1, typename Tfrom>
struct convert_le_be_t<be_t<Tt, Tt1>, Tfrom>
{
	static be_t<Tt, Tt1> func(Tfrom&& value)
	{
		return be_t<Tt, Tt1>::make(value);
	}
};

template<typename Tt, typename Tt1, typename Tf, typename Tf1>
struct convert_le_be_t<be_t<Tt, Tt1>, be_t<Tf, Tf1>>
{
	static be_t<Tt, Tt1> func(be_t<Tf, Tf1>&& value)
	{
		return value;
	}
};

template<typename Tto, typename Tf, typename Tf1>
struct convert_le_be_t<Tto, be_t<Tf, Tf1>>
{
	static Tto func(be_t<Tf, Tf1>&& value)
	{
		return value.value();
	}
};

template<typename Tto, typename Tfrom>
__forceinline Tto convert_le_be(Tfrom&& value)
{
	return convert_le_be_t<Tto, Tfrom>::func(value);
}

template<typename Tto, typename Tfrom>
__forceinline void convert_le_be(Tto& dst, Tfrom&& src)
{
	dst = convert_le_be_t<Tto, Tfrom>::func(src);
}
