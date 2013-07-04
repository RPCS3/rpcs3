#pragma once

template<int size, typename T> __forceinline static T sign(const T value)
{
	if(value & (1 << (size - 1)))
	{
		return value - (1 << size);
	}

	return value;
}

enum CodeFieldType
{
	FIELD_IMM,
	FIELD_R_GPR,
	FIELD_R_FPR,
	FIELD_R_VPR,
	FIELD_R_CR,
	FIELD_BRANCH,
};

class CodeFieldBase
{
public:
	CodeFieldType m_type;

public:
	CodeFieldBase(CodeFieldType type = FIELD_IMM) : m_type(type)
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

template<uint from, uint to=from>
class CodeField : public CodeFieldBase
{
	static_assert(from <= to, "too big 'from' value");
	static_assert(to <= 31, "too big 'to' value");

public:
	CodeField(CodeFieldType type = FIELD_IMM) : CodeFieldBase(type)
	{
	}

	static const u32 shift = 31 - to;
	static const u32 mask = ((1ULL << ((to - from) + 1)) - 1) << shift;
	
	static __forceinline void encode(u32& data, u32 value)
	{
		data &= ~mask;
		data |= (value << shift) & mask;
	}

	static __forceinline u32 decode(u32 data)
	{
		return (data & mask) >> shift;
	}

	virtual u32 operator ()(u32 data) const
	{
		return decode(data);
	}

	virtual void operator()(u32& data, u32 value) const
	{
		encode(data, value);
	}
};

static CodeField<0, 31> GetCode;

template<uint from1, uint to1, uint from2, uint to2 = from2, uint offset = 0>
class DoubleCodeField : public CodeField<from1, to1>
{
	static_assert(from2 <= to2, "too big from2 value");
	static_assert(to2 <= 31, "too big to2 value");

public:
	DoubleCodeField(CodeFieldType type = FIELD_IMM) : CodeField(type)
	{
	}

	static const u32 shift2 = 31 - to2;
	static const u32 mask2 = ((1 << ((to2 - from2) + 1)) - 1) << shift2;
	
	static __forceinline void encode(u32& data, u32 value)
	{
		data &= ~(mask | mask2);
		data |= ((value << shift) & mask) | (((value >> offset) << shift2) & mask2);
	}

	static __forceinline u32 decode(u32 data)
	{
		return ((data & mask) >> shift) | (((data & mask2) >> shift2) << offset);
	}

	virtual u32 operator ()(u32 data) const
	{
		return decode(data);
	}

	virtual void operator()(u32& data, u32 value) const
	{
		encode(data, value);
	}
};

template<uint from, uint to = from, uint _size = to - from + 1>
class CodeFieldSigned : public CodeField<from, to>
{
public:
	CodeFieldSigned(CodeFieldType type = FIELD_IMM) : CodeField(type)
	{
	}

	static const int size = _size;

	static __forceinline u32 decode(u32 data)
	{
		return sign<size>((data & mask) >> shift);
	}

	virtual u32 operator ()(u32 data) const
	{
		return decode(data);
	}
};

template<uint from, uint to = from, uint _offset = 0>
class CodeFieldOffset : public CodeField<from, to>
{
	static const int offset = _offset;

public:
	CodeFieldOffset(CodeFieldType type = FIELD_IMM) : CodeField(type)
	{
	}

	static __forceinline u32 decode(u32 data)
	{
		return ((data & mask) >> shift) << offset;
	}

	static __forceinline void encode(u32& data, u32 value)
	{
		data &= ~mask;
		data |= ((value >> offset) << shift) & mask;
	}

	virtual u32 operator ()(u32 data) const
	{
		return decode(data);
	}

	virtual void operator ()(u32& data, u32 value) const
	{
		return encode(data, value);
	}
};

template<uint from, uint to = from, uint _offset = 0, uint size = to - from + 1>
class CodeFieldSignedOffset : public CodeFieldSigned<from, to, size>
{
	static const int offset = _offset;

public:
	CodeFieldSignedOffset(CodeFieldType type = FIELD_IMM) : CodeFieldSigned(type)
	{
	}

	static __forceinline u32 decode(u32 data)
	{
		return sign<size>((data & mask) >> shift) << offset;
	}

	static __forceinline void encode(u32& data, u32 value)
	{
		data &= ~mask;
		data |= ((value >> offset) << shift) & mask;
	}

	virtual u32 operator ()(u32 data) const
	{
		return decode(data);
	}

	virtual void operator ()(u32& data, u32 value) const
	{
		return encode(data, value);
	}
};

