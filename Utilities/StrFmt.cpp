#include "stdafx.h"
#include "StrFmt.h"

static const std::string fmt::placeholder = "???";


//wrapper to deal with advance sprintf formating options with automatic length finding
//can't take strings by reference because of "va_start", so overload it with char *
std::string fmt::FormatV(const char *fmt, va_list args)
{
	int length = 256;
	std::string str;

	for (;;)
	{
		std::vector<char> buffptr(length);
		size_t printlen = vsnprintf(buffptr.data(), length, fmt, args);
		if (printlen >= 0 && printlen < length)
		{
			str = std::string(buffptr.data(), printlen);
			break;
		}
		length *= 2;
	}
	return str;
}

std::string fmt::FormatV(std::string fmt, va_list args)
{
	std::string str = FormatV(fmt.c_str(), args);
	return str;
}

//convert a wxString to a std::string encoded in utf8
//CAUTION, only use this to interface with wxWidgets classes
std::string fmt::ToUTF8(const wxString& right)
{
	auto ret = std::string(((const char *) right.utf8_str()));
	return ret;
}

//convert a std::string encoded in utf8 to a wxString
//CAUTION, only use this to interface with wxWidgets classes
wxString fmt::FromUTF8(const std::string& right)
{
	auto ret = wxString::FromUTF8(right.c_str());
	return ret;
}

//TODO: remove this after every snippet that uses it is gone
//WARNING: not fully compatible with CmpNoCase from wxString
int fmt::CmpNoCase(const std::string& a, const std::string& b)
{
	if (a.length() != b.length())
	{
		return -1;
	}
	else
	{
		return std::equal(a.begin(),
			a.end(),
			b.begin(),
			[](const char& a, const char& b){return tolower(a) == tolower(b); })
			? 0 : -1;
	}
}