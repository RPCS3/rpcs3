#include "stdafx.h"
#include "version.h"

namespace utils
{
	std::string to_string(version_type type)
	{
		switch (type)
		{
		case version_type::pre_alpha: return "Pre-Alpha";
		case version_type::alpha: return "Alpha";
		case version_type::beta: return "Beta";
		case version_type::release_candidate: return "RC";
		case version_type::release: return "Release";
		}

		throw type;
	}

	uint version::to_hex() const
	{
		return (m_hi << 24) | (m_mid << 16) | (m_lo << 8) | ((uint(m_type) & 0xf) << 4) | (m_type_index & 0xf);
	}

	std::string version::to_string() const
	{
		std::string version = std::to_string(hi()) + "." + std::to_string(mid());

		if (lo())
		{
			version += "." + std::to_string(lo());
		}

		if (type() != version_type::release)
		{
			if (!postfix().empty())
			{
				version += "-" + postfix();
			}

			version += " " + utils::to_string(type());

			if (type_index() > 1)
			{
				version += " " + std::to_string(type_index());
			}
		}

		return version;
	}
}
