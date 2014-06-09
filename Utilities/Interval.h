#pragma once

template<typename T>
struct BaseInterval
{
	static const uint64_t zero = 0ull;
	static const uint64_t notz = 0xffffffffffffffffull;

	T m_min, m_max;

	static BaseInterval<T> make(T min_value, T max_value)
	{
		BaseInterval<T> res = { min_value, max_value };
		return res;
	}

	static BaseInterval<T> make()
	{
		return make((T&)zero, (T&)notz);
	}

	bool getconst(T& result)
	{
		if (m_min == m_max)
		{
			result = m_min;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool isindef()
	{
		if (T == float)
		{

		}
	}
};