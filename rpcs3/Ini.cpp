#include "stdafx.h"
#include "Ini.h"

#include <algorithm>
#include <cctype>

#define DEF_CONFIG_NAME "./rpcs3.ini"

CSimpleIniCaseA *getIniFile()
{
	static bool inited = false;
	static CSimpleIniCaseA ini;
	if (inited == false)
	{
		ini.SetUnicode(true);
		ini.LoadFile(DEF_CONFIG_NAME);
		inited = true;
	}
	return &ini;
}

void saveIniFile()
{
	getIniFile()->SaveFile(DEF_CONFIG_NAME);
}

std::pair<int, int> rDefaultSize = { -1, -1 };
Inis Ini;

static bool StringToBool(const wxString& str)
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
	if(b) return "true";

	return "false";
}

//takes a string of format "[number]x[number]" and returns a pair of ints
//example input would be "123x456" and the returned value would be {123,456}
static std::pair<int, int> StringToSize(const std::string& str)
{
	std::pair<int, int> ret;

	std::string s[2] = { "", "" };

	for (uint i = 0, a = 0; i<str.size(); ++i)
	{
		if (!fmt::CmpNoCase(str.substr(i, 1), "x"))
		{
			if (++a >= 2) return rDefaultSize;
			continue;
		}

		s[a] += str.substr(i, 1);
	}

	if (s[0].empty() || s[1].empty())
	{
		return rDefaultSize;
	}

	try{
		ret.first = std::stoi(s[0]);
		ret.first = std::stoi(s[1]);
	}
	catch (const std::invalid_argument &e)
	{
		return rDefaultSize;
	}
	if (ret.first < 0 || ret.second < 0)
	{
		return rDefaultSize;
	}

	return ret;
}

static std::string SizeToString(const std::pair<int, int>& size)
{
	return fmt::Format("%dx%d", size.first, size.second);
}

static wxPoint StringToPosition(const wxString& str)
{
	wxPoint ret;

	wxString s[2] = {wxEmptyString, wxEmptyString};

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

static WindowInfo StringToWindowInfo(const std::string& str)
{
	WindowInfo ret = WindowInfo(rDefaultSize, rDefaultSize);

	std::string s[4] = { "", "", "", "" };

	for (uint i = 0, a = 0; i<str.size(); ++i)
	{
		if (!fmt::CmpNoCase(str.substr(i, 1), "x") || !fmt::CmpNoCase(str.substr(i, 1), ":"))
		{
			if (++a >= 4) return WindowInfo::GetDefault();
			continue;
		}

		s[a] += str.substr(i, 1);
	}

	if (s[0].empty() || s[1].empty() || s[2].empty() || s[3].empty())
	{
		return WindowInfo::GetDefault();
	}

	try{
		ret.size.first = std::stoi(s[0]);
		ret.size.second = std::stoi(s[1]);
		ret.position.first = std::stoi(s[2]);
		ret.position.second = std::stoi(s[3]);
	}
	catch (const std::invalid_argument &e)
	{
		return WindowInfo::GetDefault();
	}
	if (ret.size.first <= 0 || ret.size.second <= 0)
	{
		return WindowInfo::GetDefault();
	}

	return ret;
}

static std::string WindowInfoToString(const WindowInfo& wind)
{
	const int px = wind.position.first < -wind.size.first ? -1 : wind.position.first;
	const int py = wind.position.second < -wind.size.second ? -1 : wind.position.second;
	return fmt::Format("%dx%d:%dx%d", wind.size.first, wind.size.second, px, py);
}

//Ini
Ini::Ini()
{
		m_Config = getIniFile();
}

Ini::~Ini()
{
	saveIniFile();
}

//TODO: saving the file after each change seems like overkill but that's how wx did it
void Ini::Save(const std::string& section, const std::string& key, int value)
{
	m_Config->SetLongValue(section.c_str(), key.c_str(), value);
	saveIniFile();
}

void Ini::Save(const std::string& section, const std::string& key, bool value)
{
	m_Config->SetBoolValue(section.c_str(), key.c_str(), value);
	saveIniFile();
}

void Ini::Save(const std::string& section, const std::string& key, std::pair<int, int> value)
{
	m_Config->SetValue(section.c_str(), key.c_str(), SizeToString(value).c_str());
	saveIniFile();
}

void Ini::Save(const std::string& section, const std::string& key, const std::string& value)
{
	m_Config->SetValue(section.c_str(), key.c_str(), value.c_str());
	saveIniFile();
}

void Ini::Save(const std::string& section, const std::string& key, WindowInfo value)
{
	m_Config->SetValue(section.c_str(), key.c_str(), WindowInfoToString(value).c_str());
	saveIniFile();
}

int Ini::Load(const std::string& section, const std::string& key, const int def_value)
{
	return m_Config->GetLongValue(section.c_str(), key.c_str(), def_value);
	saveIniFile();
}

bool Ini::Load(const std::string& section, const std::string& key, const bool def_value)
{
	return StringToBool(m_Config->GetValue(section.c_str(), key.c_str(), BoolToString(def_value).c_str()));
	saveIniFile();
}

std::pair<int, int> Ini::Load(const std::string& section, const std::string& key, const std::pair<int, int> def_value)
{
	return StringToSize(m_Config->GetValue(section.c_str(), key.c_str(), SizeToString(def_value).c_str()));
	saveIniFile();
}

std::string Ini::Load(const std::string& section, const std::string& key, const std::string& def_value)
{
	return std::string(m_Config->GetValue(section.c_str(), key.c_str(), def_value.c_str()));
	saveIniFile();
}

WindowInfo Ini::Load(const std::string& section, const std::string& key, const WindowInfo& def_value)
{
	return StringToWindowInfo(m_Config->GetValue(section.c_str(), key.c_str(), WindowInfoToString(def_value).c_str()));
	saveIniFile();
}
