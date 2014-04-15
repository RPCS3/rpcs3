#pragma once

#include "Utilities/GNU.h"

template<typename T, int size = sizeof(T)> struct se_t;
template<typename T> struct se_t<T, 1> { static __forceinline void func(T& dst, const T src) { (u8&)dst = (u8&)src; } };
template<typename T> struct se_t<T, 2> { static __forceinline void func(T& dst, const T src) { (u16&)dst = _byteswap_ushort((u16&)src); } };
template<typename T> struct se_t<T, 4> { static __forceinline void func(T& dst, const T src) { (u32&)dst = _byteswap_ulong((u32&)src); } };
template<typename T> struct se_t<T, 8> { static __forceinline void func(T& dst, const T src) { (u64&)dst = _byteswap_uint64((u64&)src); } };


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

template<typename T, int size=sizeof(T)>
class be_t
{
	static_assert(size == 1 || size == 2 || size == 4 || size == 8, "Bad be_t type");
	T m_data;

public:
	typedef T type;

#ifdef _WIN32
	be_t(){}
#else
	be_t()	noexcept = default;
#endif

	be_t(const be_t<T,size>& value) = default;
	be_t(const T& value)
	{
		FromLE(value);
	}

	template<typename T1>
	explicit be_t(const be_t<T1>& value)
	{
		FromBE(value.ToBE());
	}
	
	const T& ToBE() const
	{
		return m_data;
	}

	T ToLE() const
	{
		T res;

		se_t<T>::func(res, m_data);

		return res;
	}

	void FromBE(const T& value)
	{
		m_data = value;
	}

	void FromLE(const T& value)
	{
		se_t<T>::func(m_data, value);
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
		res.FromBE(ToBE());
		return res;
	}

	be_t& operator = (const T& right)
	{
		FromLE(right);
		return *this;
	}

	be_t<T,size>& operator = (const be_t<T,size>& right) = default;

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

template<typename T, typename T1, T1 value> struct _se : public const_se_t<T, value> {};
template<typename T, typename T1, T1 value> struct _se<be_t<T>, T1, value> : public const_se_t<T, value> {};

#define se(t, x) _se<decltype(t), decltype(x), x>::value
#define se16(x) _se<u16, decltype(x), x>::value
#define se32(x) _se<u32, decltype(x), x>::value
#define se64(x) _se<u64, decltype(x), x>::value
