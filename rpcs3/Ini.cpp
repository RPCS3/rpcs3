#include "stdafx.h"
#include "Ini.h"

#include <wx/msw/iniconf.h>

Inis Ini;

static bool StringToBool(const wxString str)
{
	if(
		!str.CmpNoCase("enable") ||
		!str.CmpNoCase("e") ||
		!str.CmpNoCase("1") ||
		!str.CmpNoCase("true") ||
		!str.CmpNoCase("t") )
	{
		return true;
	}

	return false;
}

static wxString BoolToString(const bool b)
{
	if(b) return "enable";

	return "disable";
}

static wxSize StringToSize(const wxString str)
{
	wxSize ret;

	wxString s[2] = {wxEmptyString};

	for(uint i=0, a=0; i<str.Length(); ++i)
	{
		if(!str(i, 1).CmpNoCase("x"))
		{
			if(++a >= 2) return wxDefaultSize;
			continue;
		}

		s[a] += str(i, 1);
	}
	
	if(s[0].IsEmpty() || s[1].IsEmpty())
	{
		return wxDefaultSize;
	}

	s[0].ToLong((long*)&ret.x);
	s[1].ToLong((long*)&ret.y);

	if(ret.x <= 0 || ret.y <= 0)
	{
		return wxDefaultSize;
	}

	return ret;
}

static wxString SizeToString(const wxSize size)
{
	return wxString::Format("%dx%d", size.x, size.y);
}

static wxPoint StringToPosition(const wxString str)
{
	wxPoint ret;

	wxString s[2] = {wxEmptyString};

	for(uint i=0, a=0; i<str.Length(); ++i)
	{
		if(!str(i, 1).CmpNoCase("x"))
		{
			if(++a >= 2) return wxDefaultPosition;
			continue;
		}

		s[a] += str(i, 1);
	}
	
	if(s[0].IsEmpty() || s[1].IsEmpty())
	{
		return wxDefaultPosition;
	}

	s[0].ToLong((long*)&ret.x);
	s[1].ToLong((long*)&ret.y);

	if(ret.x <= 0 || ret.y <= 0)
	{
		return wxDefaultPosition;
	}

	return ret;
}

static wxString PositionToString(const wxPoint position)
{
	return wxString::Format("%dx%d", position.x, position.y);
}

static WindowInfo StringToWindowInfo(const wxString str)
{
	WindowInfo ret = WindowInfo(wxDefaultSize, wxDefaultPosition);

	wxString s[4] = {wxEmptyString};

	for(uint i=0, a=0; i<str.Length(); ++i)
	{
		if(!str(i, 1).CmpNoCase("x") || !str(i, 1).CmpNoCase(":"))
		{
			if(++a >= 4) return WindowInfo::GetDefault();
			continue;
		}

		s[a] += str(i, 1);
	}
	
	if(s[0].IsEmpty() || s[1].IsEmpty() || s[2].IsEmpty() || s[3].IsEmpty())
	{
		return WindowInfo::GetDefault();
	}

	s[0].ToLong((long*)&ret.size.x);
	s[1].ToLong((long*)&ret.size.y);
	s[2].ToLong((long*)&ret.position.x);
	s[3].ToLong((long*)&ret.position.y);

	if(ret.size.x <= 0 || ret.size.y <= 0)
	{
		return WindowInfo::GetDefault();
	}

	return ret;
}

static wxString WindowInfoToString(const WindowInfo wind)
{
	const int px = wind.position.x < -wind.size.x ? -1 : wind.position.x;
	const int py = wind.position.y < -wind.size.y ? -1 : wind.position.y;
	return wxString::Format("%dx%d:%dx%d", wind.size.x, wind.size.y, px, py);
}

//Ini
Ini::Ini()
{
	m_Config = new wxIniConfig( wxEmptyString, wxEmptyString,
			wxGetCwd() + "\\rpcs3.ini",
			wxEmptyString, wxCONFIG_USE_LOCAL_FILE );
}

void Ini::Save(wxString key, int value)
{
	m_Config->Write(key, value);
}

void Ini::Save(wxString key, bool value)
{
	m_Config->Write(key, BoolToString(value));
}

void Ini::Save(wxString key, wxSize value)
{
	m_Config->Write(key, SizeToString(value));
}

void Ini::Save(wxString key, wxPoint value)
{
	m_Config->Write(key, PositionToString(value));
}

void Ini::Save(wxString key, wxString value)
{
	m_Config->Write(key, value);
}

void Ini::Save(wxString key, WindowInfo value)
{
	m_Config->Write(key, WindowInfoToString(value));
}

int Ini::Load(wxString key, const int def_value)
{
	return m_Config->Read(key, def_value);
}

bool Ini::Load(wxString key, const bool def_value)
{
	return StringToBool(m_Config->Read(key, BoolToString(def_value)));
}

wxSize Ini::Load(wxString key, const wxSize def_value)
{
	return StringToSize(m_Config->Read(key, SizeToString(def_value)));
}

wxPoint Ini::Load(wxString key, const wxPoint def_value)
{
	return StringToPosition(m_Config->Read(key, PositionToString(def_value)));
}

wxString Ini::Load(wxString key, const wxString& def_value)
{
	return m_Config->Read(key, def_value);
}

WindowInfo Ini::Load(wxString key, const WindowInfo& def_value)
{
	return StringToWindowInfo(m_Config->Read(key, WindowInfoToString(def_value)));
}