#include "stdafx.h"
#include "Utilities/rPlatform.h"
#include "Utilities/StrFmt.h"

#include "Ini.h"
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
		ini.LoadFile(std::string(rPlatform::getConfigDir() + DEF_CONFIG_NAME).c_str());
		inited = true;
	}
	return &ini;
}

void saveIniFile()
{
	getIniFile()->SaveFile(std::string(rPlatform::getConfigDir() + DEF_CONFIG_NAME).c_str());
}

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
	std::size_t start = 0, found;
	std::vector<int> vec;
	for (int i = 0; i < 2 && (found = str.find_first_of('x', start)); i++) {
		try {
			vec.push_back(std::stoi(str.substr(start, found == std::string::npos ? found : found - start)));
		}
		catch (const std::invalid_argument& e) {
			return std::make_pair(-1, -1);
		}
		if (found == std::string::npos)
			break;
		start = found + 1;
	}
	if (vec.size() < 2 || vec[0] < 0 || vec[1] < 0)
		return std::make_pair(-1, -1);

	return std::make_pair(vec[0], vec[1]);
}

static std::string SizeToString(const std::pair<int, int>& size)
{
	return fmt::Format("%dx%d", size.first, size.second);
}

static WindowInfo StringToWindowInfo(const std::string& str)
{
	std::size_t start = 0, found;
	std::vector<int> vec;
	for (int i = 0; i < 4 && (found = str.find_first_of("x:", start)); i++) {
		try {
			vec.push_back(std::stoi(str.substr(start, found == std::string::npos ? found : found - start)));
		}
		catch (const std::invalid_argument& e) {
			return WindowInfo();
		}
		if (found == std::string::npos)
			break;
		start = found + 1;
	}
	if (vec.size() < 4 || vec[0] <= 0 || vec[1] <= 0 || vec[2] < 0 || vec[3] < 0)
		return WindowInfo();

	return WindowInfo(std::make_pair(vec[0], vec[1]), std::make_pair(vec[2], vec[3]));
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
}

bool Ini::Load(const std::string& section, const std::string& key, const bool def_value)
{
	return StringToBool(m_Config->GetValue(section.c_str(), key.c_str(), BoolToString(def_value).c_str()));
}

std::pair<int, int> Ini::Load(const std::string& section, const std::string& key, const std::pair<int, int> def_value)
{
	return StringToSize(m_Config->GetValue(section.c_str(), key.c_str(), SizeToString(def_value).c_str()));
}

std::string Ini::Load(const std::string& section, const std::string& key, const std::string& def_value)
{
	return std::string(m_Config->GetValue(section.c_str(), key.c_str(), def_value.c_str()));
}

WindowInfo Ini::Load(const std::string& section, const std::string& key, const WindowInfo& def_value)
{
	return StringToWindowInfo(m_Config->GetValue(section.c_str(), key.c_str(), WindowInfoToString(def_value).c_str()));
}
