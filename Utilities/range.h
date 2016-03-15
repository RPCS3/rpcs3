#pragma once
#include <cstddef>

template<typename Type>
class range
{
	Type m_begin;
	Type m_end;

public:
	using type = Type;

	constexpr range(Type begin, Type end)
		: m_begin(begin), m_end(end)
	{
	}

	constexpr range(Type point)
		: m_begin(point), m_end(point + 1)
	{
	}


	constexpr range()
		: m_begin{}
		, m_end{}
	{
	}

	range& set(Type begin, Type end)
	{
		m_begin = begin;
		m_end = end;

		return *this;
	}

	range& set(range& other)
	{
		return set(other.begin(), other.end());
	}

	range& begin(Type value)
	{
		m_begin = value;
		return *this;
	}

	range& end(Type value)
	{
		m_end = value;
		return *this;
	}

	range& size(Type value)
	{
		m_end = m_begin + value;
		return *this;
	}

	void extend(const range& other)
	{
		m_begin = std::min(m_begin, other.m_begin);
		m_end = std::max(m_end, other.m_end);
	}

	constexpr bool valid() const
	{
		return m_begin <= m_end;
	}

	constexpr Type begin() const
	{
		return m_begin;
	}

	constexpr Type end() const
	{
		return m_end;
	}

	constexpr Type size() const
	{
		return m_end - m_begin;
	}

	constexpr bool contains(Type point) const
	{
		return point >= m_begin && point < m_end;
	}

	constexpr bool overlaps(const range& rhs) const
	{
		return m_begin < rhs.m_end && m_end > rhs.m_begin;
	}

	constexpr bool operator == (const range& rhs) const
	{
		return m_begin == rhs.m_begin && m_end == rhs.m_end;
	}

	constexpr bool operator != (const range& rhs) const
	{
		return m_begin != rhs.m_begin || m_end != rhs.m_end;
	}

	constexpr range operator / (Type rhs) const
	{
		return{ m_begin / rhs, m_end / rhs };
	}

	constexpr range operator * (Type rhs) const
	{
		return{ m_begin * rhs, m_end * rhs };
	}

	constexpr range operator + (Type rhs) const
	{
		return{ m_begin + rhs, m_end + rhs };
	}

	constexpr range operator - (Type rhs) const
	{
		return{ m_begin - rhs, m_end - rhs };
	}

	range& operator /= (Type rhs)
	{
		m_begin /= rhs;
		m_end /= rhs;
		return *this;
	}

	range& operator *= (Type rhs)
	{
		m_begin *= rhs;
		m_end *= rhs;
		return *this;
	}

	range& operator += (Type rhs)
	{
		m_begin += rhs;
		m_end += rhs;
		return *this;
	}

	range& operator -= (Type rhs)
	{
		m_begin -= rhs;
		m_end -= rhs;
		return *this;
	}

	constexpr range operator &(const range& rhs) const
	{
		return{ std::max(m_begin, rhs.m_begin), std::min(m_end, rhs.m_end) };
	}
};
