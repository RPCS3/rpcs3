#pragma once

template<uint size, typename T> force_inline static T sign(const T value)
{
	static_assert(size > 0 && size < sizeof(T) * 8, "Bad sign size");

	if(value & (T(1) << (size - 1)))
	{
		return value - (T(1) << size);
	}

	return value;
}

class CodeFieldBase
{
public:
	u32 m_type;

public:
	CodeFieldBase(u32 type) : m_type(type)
	{
	}

	virtual u32 operator ()(u32 data) const=0;
	virtual void operator()(u32& data, u32 value) const=0;

	virtual u32 operator[](u32 value) const
	{
		u32 result = 0;
		(*this)(result, value);
		return result;
	}
};
