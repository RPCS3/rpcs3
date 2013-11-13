#pragma once
#pragma warning(disable: 4739)

template<typename T>
class be_t
{
	static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Bad be_t type");
	T m_data;

public:
	be_t()
	{
	}

	be_t(const T& value)
	{
		FromLE(value);
	}

	template<typename T1>
	be_t(const be_t<T1>& value)
	{
		FromBE(value.ToBE());
	}

	T ToBE() const
	{
		return m_data;
	}

	T ToLE() const
	{
		T res;

		switch(sizeof(T))
		{
		case 1:
			(u8&)res = (u8&)m_data;
		break;

		case 2:
			(u16&)res = _byteswap_ushort((u16&)m_data);
		break;

		case 4:
			(u32&)res = _byteswap_ulong((u32&)m_data);
		break;

		case 8:
			(u64&)res = _byteswap_uint64((u64&)m_data);
		break;

		default:
			assert(0);
		break;
		}

		return res;
	}

	void FromBE(const T& value)
	{
		m_data = value;
	}

	void FromLE(const T& value)
	{
		switch(sizeof(T))
		{
		case 1:
			(u8&)m_data = (u8&)value;
		return;

		case 2:
			(u16&)m_data = _byteswap_ushort((u16&)value);
		return;

		case 4:
			(u32&)m_data = _byteswap_ulong((u32&)value);
		return;

		case 8:
			(u64&)m_data = _byteswap_uint64((u64&)value);
		return;
		}

		assert(0);
		m_data = value;
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

	be_t& operator = (const be_t& right)
	{
		m_data = right.m_data;
		return *this;
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

	template<typename T1> bool operator == (T1 right) const { return ToLE() == right; }
	template<typename T1> bool operator != (T1 right) const { return !(*this == right); }
	template<typename T1> bool operator >  (T1 right) const { return ToLE() >  right; }
	template<typename T1> bool operator <  (T1 right) const { return ToLE() <  right; }
	template<typename T1> bool operator >= (T1 right) const { return ToLE() >= right; }
	template<typename T1> bool operator <= (T1 right) const { return ToLE() <= right; }

	template<typename T1> bool operator == (const be_t<T1>& right) const { return ToBE() == right.ToBE(); }
	template<typename T1> bool operator != (const be_t<T1>& right) const { return !(*this == right); }
	template<typename T1> bool operator >  (const be_t<T1>& right) const { return ToLE() >  right.ToLE(); }
	template<typename T1> bool operator <  (const be_t<T1>& right) const { return ToLE() <  right.ToLE(); }
	template<typename T1> bool operator >= (const be_t<T1>& right) const { return ToLE() >= right.ToLE(); }
	template<typename T1> bool operator <= (const be_t<T1>& right) const { return ToLE() <= right.ToLE(); }
};