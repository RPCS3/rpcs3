#pragma once

#include <emmintrin.h>

union u128
{
	u64 _u64[2];
	s64 _s64[2];
	u32 _u32[4];
	s32 _s32[4];
	u16 _u16[8];
	s16 _s16[8];
	u8  _u8[16];
	s8  _s8[16];
	float _f[4];
	double _d[2];
	__m128 xmm;

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

	static u128 from32(u32 _0, u32 _1 = 0, u32 _2 = 0, u32 _3 = 0)
	{
		u128 ret;
		ret._u32[0] = _0;
		ret._u32[1] = _1;
		ret._u32[2] = _2;
		ret._u32[3] = _3;
		return ret;
	}

	static u128 fromBit(u32 bit)
	{
		u128 ret = {};
		ret._bit[bit] = true;
		return ret;
	}

	void setBit(u32 bit)
	{
		_bit[bit] = true;
	}

	bool operator == (const u128& right) const
	{
		return (_u64[0] == right._u64[0]) && (_u64[1] == right._u64[1]);
	}

	bool operator != (const u128& right) const
	{
		return (_u64[0] != right._u64[0]) || (_u64[1] != right._u64[1]);
	}

	u128 operator | (const u128& right) const
	{
		return from64(_u64[0] | right._u64[0], _u64[1] | right._u64[1]);
	}

	u128 operator & (const u128& right) const
	{
		return from64(_u64[0] & right._u64[0], _u64[1] & right._u64[1]);
	}

	u128 operator ^ (const u128& right) const
	{
		return from64(_u64[0] ^ right._u64[0], _u64[1] ^ right._u64[1]);
	}

	u128 operator ~ () const
	{
		return from64(~_u64[0], ~_u64[1]);
	}

	void clear()
	{
		_u64[1] = _u64[0] = 0;
	}

	std::string to_hex() const
	{
		return fmt::Format("%16llx%16llx", _u64[1], _u64[0]);
	}

	std::string to_xyzw() const
	{
		return fmt::Format("x: %g y: %g z: %g w: %g", _f[3], _f[2], _f[1], _f[0]);
	}

	static __forceinline u128 byteswap(const u128 val)
	{
		u128 ret;
		ret._u64[0] = _byteswap_uint64(val._u64[1]);
		ret._u64[1] = _byteswap_uint64(val._u64[0]);
		return ret;
	}
};

#define re16(val) _byteswap_ushort(val)
#define re32(val) _byteswap_ulong(val)
#define re64(val) _byteswap_uint64(val)
#define re128(val) u128::byteswap(val)

template<typename T, int size = sizeof(T)> struct se_t;

template<typename T> struct se_t<T, 1>
{
	static __forceinline T func(const T src)
	{
		return src;
	}
};

template<typename T> struct se_t<T, 2>
{
	static __forceinline T func(const T src)
	{
		const u16 res = _byteswap_ushort((u16&)src);
		return (T&)res;
	}
};

template<typename T> struct se_t<T, 4>
{
	static __forceinline T func(const T src)
	{
		const u32 res = _byteswap_ulong((u32&)src);
		return (T&)res;
	}
};

template<typename T> struct se_t<T, 8>
{
	static __forceinline T func(const T src)
	{
		const u64 res = _byteswap_uint64((u64&)src);
		return (T&)res;
	}
};

//template<typename T> T re(const T val) { T res; se_t<T>::func(res, val); return res; }
//template<typename T1, typename T2> void re(T1& dst, const T2 val) { se_t<T1>::func(dst, val); }

template<typename T, u64 _value, int size = sizeof(T)> struct const_se_t;
template<typename T, u64 _value> struct const_se_t<T, _value, 1>
{
	static const T value = (T)_value;
};

template<typename T, u64 _value> struct const_se_t<T, _value, 2>
{
	static const T value = ((_value >> 8) & 0xff) | ((_value << 8) & 0xff00);
};

template<typename T, u64 _value> struct const_se_t<T, _value, 4>
{
	static const T value = 
		((_value >> 24) & 0x000000ff) |
		((_value >>  8) & 0x0000ff00) |
		((_value <<  8) & 0x00ff0000) |
		((_value << 24) & 0xff000000);
};

template<typename T, u64 _value> struct const_se_t<T, _value, 8>
{
	static const T value = 
		((_value >> 56) & 0x00000000000000ff) |
		((_value >> 40) & 0x000000000000ff00) |
		((_value >> 24) & 0x0000000000ff0000) |
		((_value >>  8) & 0x00000000ff000000) |
		((_value <<  8) & 0x000000ff00000000) |
		((_value << 24) & 0x0000ff0000000000) |
		((_value << 40) & 0x00ff000000000000) |
		((_value << 56) & 0xff00000000000000);
};

template<typename T, typename T2 = T>
class be_t
{
	static_assert(sizeof(T2) == 1 || sizeof(T2) == 2 || sizeof(T2) == 4 || sizeof(T2) == 8, "Bad be_t type");
	T m_data;

public:
	typedef T type;
	
	const T& ToBE() const
	{
		return m_data;
	}

	T ToLE() const
	{
		return se_t<T, sizeof(T2)>::func(m_data);
	}

	void FromBE(const T& value)
	{
		m_data = value;
	}

	void FromLE(const T& value)
	{
		m_data = se_t<T, sizeof(T2)>::func(value);
	}

	static be_t MakeFromLE(const T value)
	{
		T data = se_t<T, sizeof(T2)>::func(value);
		return (be_t&)data;
	}

	static be_t MakeFromBE(const T value)
	{
		return (be_t&)value;
	}

	//template<typename T1>
	operator const T() const
	{
		return ToLE();
	}

	be_t& operator = (const be_t& value) = default;

	be_t& operator = (T value)
	{
		m_data = se_t<T, sizeof(T2)>::func(value);

		return *this;
	}

	template<typename T1>
	operator const be_t<T1>() const
	{
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
				Tto res = se_t<Tto, sizeof(Tto)>::func(se_t<Tfrom, sizeof(Tfrom)>::func(be_value););
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

		return _convert<T1, T, ((sizeof(T1) > sizeof(T)) ? 1 : (sizeof(T1) < sizeof(T) ? 2 : 0))>::func(m_data);
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

template<typename T, typename T2>
class be_t<const T, T2>
{
	static_assert(sizeof(T2) == 1 || sizeof(T2) == 2 || sizeof(T2) == 4 || sizeof(T2) == 8, "Bad be_t type");
	const T m_data;

public:
	typedef const T type;

	const T& ToBE() const
	{
		return m_data;
	}

	const T ToLE() const
	{
		return se_t<const T, sizeof(T2)>::func(m_data);
	}

	static be_t MakeFromLE(const T value)
	{
		const T data = se_t<const T, sizeof(T2)>::func(value);
		return (be_t&)data;
	}

	static be_t MakeFromBE(const T value)
	{
		return (be_t&)value;
	}

	//template<typename T1>
	operator const T() const
	{
		return ToLE();
	}

	template<typename T1>
	operator const be_t<T1>() const
	{
		if (sizeof(T1) > sizeof(T) || std::is_floating_point<T>::value || std::is_floating_point<T1>::value)
		{
			T1 res = se_t<T1, sizeof(T1)>::func(ToLE());
			return (be_t<T1>&)res;
		}
		else if (sizeof(T1) < sizeof(T))
		{
			T1 res = ToBE() >> ((sizeof(T) - sizeof(T1)) * 8);
			return (be_t<T1>&)res;
		}
		else
		{
			T1 res = ToBE();
			return (be_t<T1>&)res;
		}
	}

	template<typename T1> be_t operator & (const be_t<T1>& right) const { const T res = ToBE() & right.ToBE(); return (be_t&)res; }
	template<typename T1> be_t operator | (const be_t<T1>& right) const { const T res = ToBE() | right.ToBE(); return (be_t&)res; }
	template<typename T1> be_t operator ^ (const be_t<T1>& right) const { const T res = ToBE() ^ right.ToBE(); return (be_t&)res; }

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