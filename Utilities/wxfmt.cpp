#include "stdafx.h"
#include "wxfmt.h"
#pragma warning(push)
#pragma message("TODO: remove wx dependency: <wx/string.h>")
#pragma warning(disable : 4996)
#include <wx/string.h>
#pragma warning(pop)

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
