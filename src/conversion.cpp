#include "conversion.h"
#include <algorithm>

////////////////////////////////////////////////////////////////
// Specializations for converting a string to specific types

namespace
{
	// we're not gonna mess with the mess that is all the isupper/etc. functions
	bool IsLower(char ch) { return 'a' <= ch && ch <= 'z'; }
	bool IsUpper(char ch) { return 'A' <= ch && ch <= 'Z'; }
	char ToLower(char ch) { return IsUpper(ch) ? ch + 'a' - 'A' : ch; }

	std::string tolower(const std::string& str)
	{
		std::string s(str);
		std::transform(s.begin(), s.end(), s.begin(), ToLower);
		return s;
	}

	template <typename T>
	bool IsEntirely(const std::string& str, T func)
	{
		for(std::size_t i=0;i<str.size();i++)
			if(!func(str[i]))
				return false;

		return true;
	}

	// IsFlexibleCase
	// . Returns true if 'str' is:
	//   . UPPERCASE
	//   . lowercase
	//   . Capitalized
	bool IsFlexibleCase(const std::string& str)
	{
		if(str.empty())
			return true;

		if(IsEntirely(str, IsLower))
			return true;

		bool firstcaps = IsUpper(str[0]);
		std::string rest = str.substr(1);
		return firstcaps && (IsEntirely(rest, IsLower) || IsEntirely(rest, IsUpper));
	}
}

namespace YAML
{
	bool Convert(const std::string& input, bool& b)
	{
		// we can't use iostream bool extraction operators as they don't
		// recognize all possible values in the table below (taken from
		// http://yaml.org/type/bool.html)
		static const struct {
			std::string truename, falsename;
		} names[] = {
			{ "y", "n" },
			{ "yes", "no" },
			{ "true", "false" },
			{ "on", "off" },
		};

		if(!IsFlexibleCase(input))
			return false;

		for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);i++) {
			if(names[i].truename == tolower(input)) {
				b = true;
				return true;
			}

			if(names[i].falsename == tolower(input)) {
				b = false;
				return true;
			}
		}

		return false;
	}
	
	bool Convert(const std::string& input, _Null& /*output*/)
	{
		return input.empty() || input == "~" || input == "null" || input == "Null" || input == "NULL";
	}
}

