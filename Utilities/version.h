#pragma once

#include "types.h"
#include <string>

namespace utils
{
	enum class version_type : uint
	{
		pre_alpha,
		alpha,
		beta,
		release_candidate,
		release
	};

	std::string to_string(version_type type);

	class version
	{
		uint m_hi;
		uint m_mid;
		uint m_lo;
		version_type m_type = version_type::release;
		uint m_type_index = 1;
		const char* m_postfix;

	public:
		constexpr version(uint hi, uint mid, uint lo, version_type type, uint type_index, const char* postfix)
			: m_hi(hi)
			, m_mid(mid)
			, m_lo(lo)
			, m_type(type)
			, m_type_index(type_index)
			, m_postfix(postfix)
		{
		}

		uint hi() const
		{
			return m_hi;
		}

		uint mid() const
		{
			return m_mid;
		}

		uint lo() const
		{
			return m_lo;
		}

		version_type type() const
		{
			return m_type;
		}

		std::string postfix() const
		{
			return m_postfix;
		}

		uint type_index() const
		{
			return m_type_index;
		}

		uint to_hex() const;
		std::string to_string() const;
	};
}
