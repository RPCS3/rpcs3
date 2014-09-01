#pragma once

#define re16(val) _byteswap_ushort(val)
#define re32(val) _byteswap_ulong(val)
#define re64(val) _byteswap_uint64(val)
#define re128(val) u128::byteswap(val)

template<typename T, int size = sizeof(T)> struct se_t;
template<typename T> struct se_t<T, 1> { static __forceinline void func(T& dst, const T src) { (u8&)dst = (u8&)src; } };
template<typename T> struct se_t<T, 2> { static __forceinline void func(T& dst, const T src) { (u16&)dst = _byteswap_ushort((u16&)src); } };
template<typename T> struct se_t<T, 4> { static __forceinline void func(T& dst, const T src) { (u32&)dst = _byteswap_ulong((u32&)src); } };
template<typename T> struct se_t<T, 8> { static __forceinline void func(T& dst, const T src) { (u64&)dst = _byteswap_uint64((u64&)src); } };

template<typename T> T re(const T val) { T res; se_t<T>::func(res, val); return res; }
template<typename T1, typename T2> void re(T1& dst, const T2 val) { se_t<T1>::func(dst, val); }

template<typename T, s64 _value, int size = sizeof(T)> struct const_se_t;
template<typename T, s64 _value> struct const_se_t<T, _value, 1>
{
	static const T value = (T)_value;
};

template<typename T, s64 _value> struct const_se_t<T, _value, 2>
{
	static const T value = ((_value >> 8) & 0xff) | ((_value << 8) & 0xff00);
};

template<typename T, s64 _value> struct const_se_t<T, _value, 4>
{
	static const T value = 
		((_value >> 24) & 0x000000ff) |
		((_value >>  8) & 0x0000ff00) |
		((_value <<  8) & 0x00ff0000) |
		((_value << 24) & 0xff000000);
};

template<typename T, s64 _value> struct const_se_t<T, _value, 8>
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
		T res;

		se_t<T, sizeof(T2)>::func(res, m_data);

		return res;
	}

	void FromBE(const T& value)
	{
		m_data = value;
	}

	void FromLE(const T& value)
	{
		se_t<T, sizeof(T2)>::func(m_data, value);
	}

	static be_t MakeFromLE(const T value)
	{
		be_t res;
		res.FromLE(value);
		return res;
	}

	static be_t MakeFromBE(const T value)
	{
		be_t res;
		res.FromBE(value);
		return res;
	}

	//template<typename T1>
	operator const T() const
	{
		return ToLE();
	}

	template<typename T1>
	operator const be_t<T1>() const
	{
		be_t<T1> res;
		if (sizeof(T1) < sizeof(T))
		{
			res.FromBE(ToBE() >> ((sizeof(T)-sizeof(T1)) * 8));
		}
		else if (sizeof(T1) > sizeof(T))
		{
			res.FromLE(ToLE());
		}
		else
		{
			res.FromBE(ToBE());
		}
		return res;
	}

	be_t& operator = (const T& right)
	{
		FromLE(right);
		return *this;
	}

	be_t& operator = (const be_t& right) = default;

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
struct is_be_t<be_t<T, T2>, T2> : public std::integral_constant<bool, true>
{
};

template<typename T>
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
	static const bool value = (sizeof(T2) > 1) && std::is_arithmetic<T>::value;

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
	typedef void type;
};

template<typename T, typename T1, T1 value> struct _se : public const_se_t<T, value> {};
template<typename T, typename T1, T1 value> struct _se<be_t<T>, T1, value> : public const_se_t<T, value> {};

#define se(t, x) _se<decltype(t), decltype(x), x>::value
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