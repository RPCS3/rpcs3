#pragma once
#include <string>
#include <cstdint>

namespace utils
{
	enum class version_type : std::uint8_t
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
		std::uint8_t m_hi;
		std::uint8_t m_mid;
		std::uint8_t m_lo;
		version_type m_type = version_type::release;
		std::uint8_t m_type_index = 1;
		std::string m_postfix;

	public:
		version(std::uint8_t hi, std::uint8_t mid, std::uint8_t lo = 0);

		version& type(version_type type, std::uint8_t type_index = 1);

		std::uint8_t hi() const
		{
			return m_hi;
		}

		std::uint8_t mid() const
		{
			return m_mid;
		}

		std::uint8_t lo() const
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

		version& postfix(const std::string& value)
		{
			m_postfix = value;
			return *this;
		}

		std::uint8_t type_index() const
		{
			return m_type_index;
		}

		std::uint16_t to_hex() const;
		std::string to_string() const;
	};
}
