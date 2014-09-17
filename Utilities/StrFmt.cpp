#include "stdafx.h"
#include "StrFmt.h"
#include <wx/string.h>

extern const std::string fmt::placeholder = "???";

std::string replace_first(const std::string& src, const std::string& from, const std::string& to)
{
	auto pos = src.find(from);

	if (pos == std::string::npos)
	{
		return src;
	}

	return (pos ? src.substr(0, pos) + to : to) + std::string(src.c_str() + pos + from.length());
}

std::string replace_all(std::string src, const std::string& from, const std::string& to)
{
	for (auto pos = src.find(from); pos != std::string::npos; src.find(from, pos + 1))
	{
		src = (pos ? src.substr(0, pos) + to : to) + std::string(src.c_str() + pos + from.length());
		pos += to.length();
	}

	return src;
}

//TODO: move this wx Stuff somewhere else
//convert a wxString to a std::string encoded in utf8
//CAUTION, only use this to interface with wxWidgets classes
std::string fmt::ToUTF8(const wxString& right)
{
	auto ret = std::string(((const char *)right.utf8_str()));
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

//TODO: remove this after every snippet that uses it is gone
//WARNING: not fully compatible with CmpNoCase from wxString
void fmt::Replace(std::string &str, const std::string &searchterm, const std::string& replaceterm)
{
	size_t cursor = 0;
	do
	{
		cursor = str.find(searchterm, cursor);
		if (cursor != std::string::npos)
		{
			str.replace(cursor, searchterm.size(), replaceterm);
			cursor += replaceterm.size();
		}
		else
		{
			break;
		}
	} while (true);
}

std::vector<std::string> fmt::rSplit(const std::string& source, const std::string& delim)
{
	std::vector<std::string> ret;
	size_t cursor = 0;
	do
	{
		size_t prevcurs = cursor;
		cursor = source.find(delim, cursor);
		if (cursor != std::string::npos)
		{
			ret.push_back(source.substr(prevcurs,cursor-prevcurs));
			cursor += delim.size();
		}
		else
		{
			ret.push_back(source.substr(prevcurs));
			break;
		}
	} while (true);
	return ret;
}