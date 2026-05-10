#include "stdafx.h"
#include "version.h"

#include <regex>

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

		return "Unknown";
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
			version += '.';
			version += std::to_string(lo());
		}

		if (type() != version_type::release)
		{
			if (!postfix().empty())
			{
				version += "-" + postfix();
			}

			version += ' ';
			version += utils::to_string(type());

			if (type_index() > 1)
			{
				version += " " + std::to_string(type_index());
			}
		}

		return version;
	}

	// Based on https://www.geeksforgeeks.org/compare-two-version-numbers/
	int compare_versions(const std::string& v1, const std::string& v2, bool& ok)
	{
		// Check if both version strings are valid
		ok = std::regex_match(v1, std::regex("[0-9.]*")) && std::regex_match(v2, std::regex("[0-9.]*"));

		if (!ok)
		{
			return -1;
		}

		// vnum stores each numeric part of version
		int vnum1 = 0;
		int vnum2 = 0;

		// Loop until both strings are processed
		for (usz i = 0, j = 0; (i < v1.length() || j < v2.length());)
		{
			// Storing numeric part of version 1 in vnum1
			while (i < v1.length() && v1[i] != '.')
			{
				vnum1 = vnum1 * 10 + (v1[i] - '0');
				i++;
			}

			// Storing numeric part of version 2 in vnum2
			while (j < v2.length() && v2[j] != '.')
			{
				vnum2 = vnum2 * 10 + (v2[j] - '0');
				j++;
			}

			if (vnum1 > vnum2)
				return 1;

			if (vnum2 > vnum1)
				return -1;

			// If equal, reset variables and go for next numeric part
			vnum1 = vnum2 = 0;
			i++;
			j++;
		}

		return 0;
	}
}
