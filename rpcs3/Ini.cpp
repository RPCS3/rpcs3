#include "stdafx.h"
#include "Ini.h"

#include "Utilities/StrFmt.h"

#include <algorithm>
#include <cctype>
#include <regex>

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
std::pair<long, long> rDefaultPosition = { -1, -1 };
Inis Ini;

static bool StringToBool(const std::string& str)
{
	return std::regex_match(str.begin(), str.end(),
		std::regex("1|e|t|enable|true", std::regex_constants::icase));
}

static inline std::string BoolToString(const bool b)
{
	return b ? "true" : "false";
}

//takes a string of format "[number]x[number]" and returns a pair of ints
//example input would be "123x456" and the returned value would be {123,456}
static std::pair<int, int> StringToSize(const std::string& str)
{
	std::pair<int, int> ret;

#if 1
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
#else
	// Requires GCC 4.9 or new stdlib for Clang
	std::sregex_token_iterator first(str.begin(), str.end(), std::regex("x"), -1), last;
	std::vector<std::string> s(first, last);
	if (vec.size() < 2)
#endif
		return rDefaultSize;

	try {
		ret.first = std::stoi(s[0]);
		ret.second = std::stoi(s[1]);
	}
	catch (const std::invalid_argument& e) {
		return rDefaultSize;
	}

	if (ret.first < 0 || ret.second < 0)
		return rDefaultSize;

	return ret;
}

static std::string SizeToString(const std::pair<int, int>& size)
{
	return fmt::Format("%dx%d", size.first, size.second);
}

// Unused?
/*static std::pair<long, long> StringToPosition(const std::string& str)
{
	std::pair<long, long> ret;
	std::sregex_token_iterator first(str.begin(), str.end(), std::regex("x"), -1), last;
	std::vector<std::string> vec(first, last);
	if (vec.size() < 2)
		return rDefaultPosition;

	ret.first = std::strtol(vec.at(0).c_str(), nullptr, 10);
	ret.second = std::strtol(vec.at(1).c_str(), nullptr, 10);

	if (ret.first <= 0 || ret.second <= 0)
		return rDefaultPosition;

	return ret;
}*/

static WindowInfo StringToWindowInfo(const std::string& str)
{
	WindowInfo ret = WindowInfo(rDefaultSize, rDefaultSize);

#if 1
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
#else
	// Requires GCC 4.9 or new stdlib for Clang
	std::sregex_token_iterator first(str.begin(), str.end(), std::regex("x|:"), -1), last;
	std::vector<std::string> s(first, last);
	if (vec.size() < 4)
#endif
		return WindowInfo::GetDefault();

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
