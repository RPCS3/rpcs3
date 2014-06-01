#pragma once

#include <utility>
#include "../Utilities/simpleini/SimpleIni.h"

//TODO: make thread safe/remove static singleton
CSimpleIniCaseA *getIniFile();

//TODO: move this to the gui module
struct WindowInfo
{
	std::pair<int, int> size;
	std::pair<int, int> position;

	//the (-1,-1) values are currently used because of wxWidgets using it gdicmn.h as default size and default postion
	WindowInfo(const std::pair<int, int> _size = { -1, -1 }, const std::pair<int, int> _position = { -1, -1 })
		: size(_size)
		, position(_position)
	{
	}

	//TODO: remove
	static WindowInfo GetDefault()
	{
		return WindowInfo({ -1, -1 }, { -1, -1 });
	}
};

class Ini
{
public:
	virtual ~Ini();

protected:
	CSimpleIniCaseA *m_Config;

	Ini();

	void Save(const std::string& section, const std::string& key, int value);
	void Save(const std::string& section, const std::string& key, bool value);
	void Save(const std::string& section, const std::string& key, std::pair<int, int> value);
	void Save(const std::string& section, const std::string& key, const std::string& value);
	void Save(const std::string& section, const std::string& key, WindowInfo value);

	int Load(const std::string& section, const std::string& key, const int def_value);
	bool Load(const std::string& section, const std::string& key, const bool def_value);
	std::pair<int, int> Load(const std::string& section, const std::string& key, const std::pair<int, int> def_value);
	std::string Load(const std::string& section, const std::string& key, const std::string& def_value);
	WindowInfo Load(const std::string& section, const std::string& key, const WindowInfo& def_value);
};

template<typename T> struct IniEntry : public Ini
{
	T m_value;
	std::string m_key;
	std::string m_section;

	IniEntry() : Ini()
	{
	}

	void Init(const std::string& section, const std::string& key)
	{
		m_key = key;
		m_section = section;
	}

	void SetValue(const T& value)
	{
		m_value = value;
	}

	T GetValue()
	{
		return m_value;
	}

	T LoadValue(const T& defvalue)
	{
		return Ini::Load(m_section, m_key, defvalue);
	}

	void SaveValue(const T& value)
	{
		Ini::Save(m_section, m_key, value);
	}

	void Save()
	{
		Ini::Save(m_section, m_key, m_value);
	}

	T Load(const T& defvalue)
	{
		return (m_value = Ini::Load(m_section, m_key, defvalue));
	}
};

class Inis
{
private:
	const std::string DefPath;

public:
	IniEntry<u8> CPUDecoderMode;
	IniEntry<u8> SPUDecoderMode;
	IniEntry<bool> CPUIgnoreRWErrors;
	IniEntry<u8> GSRenderMode;
	IniEntry<u8> GSResolution;
	IniEntry<u8> GSAspectRatio;
	IniEntry<bool> GSVSyncEnable;
	IniEntry<bool> GSLogPrograms;
	IniEntry<bool> GSDumpColorBuffers;
	IniEntry<bool> GSDumpDepthBuffer;
	IniEntry<u8> PadHandlerMode;
	IniEntry<u8> KeyboardHandlerMode;
	IniEntry<u8> MouseHandlerMode;
	IniEntry<u8> AudioOutMode;
	IniEntry<bool> AudioDumpToFile;
	IniEntry<bool> AudioConvertToU16;
	IniEntry<bool> HLELogging;
	IniEntry<bool> HLEHookStFunc;
	IniEntry<bool> HLESaveTTY;
	IniEntry<bool> HLEExitOnStop;
	IniEntry<u8> HLELogLvl;
	IniEntry<u8> SysLanguage;
	IniEntry<bool> SkipPamf;
	IniEntry<bool> HLEHideDebugConsole;

	IniEntry<int> PadHandlerLStickLeft;
	IniEntry<int> PadHandlerLStickDown;
	IniEntry<int> PadHandlerLStickRight;
	IniEntry<int> PadHandlerLStickUp;
	IniEntry<int> PadHandlerLeft;
	IniEntry<int> PadHandlerDown;
	IniEntry<int> PadHandlerRight;
	IniEntry<int> PadHandlerUp;
	IniEntry<int> PadHandlerStart;
	IniEntry<int> PadHandlerR3;
	IniEntry<int> PadHandlerL3;
	IniEntry<int> PadHandlerSelect;
	IniEntry<int> PadHandlerSquare;
	IniEntry<int> PadHandlerCross;
	IniEntry<int> PadHandlerCircle;
	IniEntry<int> PadHandlerTriangle;
	IniEntry<int> PadHandlerR1;
	IniEntry<int> PadHandlerL1;
	IniEntry<int> PadHandlerR2;
	IniEntry<int> PadHandlerL2;
	IniEntry<int> PadHandlerRStickLeft;
	IniEntry<int> PadHandlerRStickDown;
	IniEntry<int> PadHandlerRStickRight;
	IniEntry<int> PadHandlerRStickUp;

public:
	Inis() : DefPath("EmuSettings")
	{
		std::string path;

		path = DefPath;
		CPUDecoderMode.Init(path, "CPU_DecoderMode");
		CPUIgnoreRWErrors.Init(path, "CPU_IgnoreRWErrors");
		SPUDecoderMode.Init(path, "CPU_SPUDecoderMode");

		GSRenderMode.Init(path, "GS_RenderMode");
		GSResolution.Init(path, "GS_Resolution");
		GSAspectRatio.Init(path, "GS_AspectRatio");
		GSVSyncEnable.Init(path, "GS_VSyncEnable");
		GSLogPrograms.Init(path, "GS_LogPrograms");
		GSDumpColorBuffers.Init(path, "GS_DumpColorBuffers");
		GSDumpDepthBuffer.Init(path, "GS_DumpDepthBuffer");
		SkipPamf.Init(path, "GS_SkipPamf");

		PadHandlerMode.Init(path, "IO_PadHandlerMode");
		KeyboardHandlerMode.Init(path, "IO_KeyboardHandlerMode");
		MouseHandlerMode.Init(path, "IO_MouseHandlerMode");

		PadHandlerLStickLeft.Init(path, "ControlSetings_PadHandlerLStickLeft");
		PadHandlerLStickDown.Init(path, "ControlSetings_PadHandlerLStickDown");
		PadHandlerLStickRight.Init(path, "ControlSetings_PadHandlerLStickRight");
		PadHandlerLStickUp.Init(path, "ControlSetings_PadHandlerLStickUp");
		PadHandlerLeft.Init(path, "ControlSetings_PadHandlerLeft");
		PadHandlerDown.Init(path, "ControlSetings_PadHandlerDown");
		PadHandlerRight.Init(path, "ControlSetings_PadHandlerRight");
		PadHandlerUp.Init(path, "ControlSetings_PadHandlerUp");
		PadHandlerStart.Init(path, "ControlSetings_PadHandlerStart");
		PadHandlerR3.Init(path, "ControlSetings_PadHandlerR3");
		PadHandlerL3.Init(path, "ControlSetings_PadHandlerL3");
		PadHandlerSelect.Init(path, "ControlSetings_PadHandlerSelect");
		PadHandlerSquare.Init(path, "ControlSetings_PadHandlerSquare");
		PadHandlerCross.Init(path, "ControlSetings_PadHandlerCross");
		PadHandlerCircle.Init(path, "ControlSetings_PadHandlerCircle");
		PadHandlerTriangle.Init(path, "ControlSetings_PadHandlerTriangle");
		PadHandlerR1.Init(path, "ControlSetings_PadHandlerR1");
		PadHandlerL1.Init(path, "ControlSetings_PadHandlerL1");
		PadHandlerR2.Init(path, "ControlSetings_PadHandlerR2");
		PadHandlerL2.Init(path, "ControlSetings_PadHandlerL2");
		PadHandlerRStickLeft.Init(path, "ControlSetings_PadHandlerRStickLeft");
		PadHandlerRStickDown.Init(path, "ControlSetings_PadHandlerRStickDown");
		PadHandlerRStickRight.Init(path, "ControlSetings_PadHandlerRStickRight");
		PadHandlerRStickUp.Init(path, "ControlSetings_PadHandlerRStickUp");

		AudioOutMode.Init(path, "Audio_AudioOutMode");
		AudioDumpToFile.Init(path, "Audio_AudioDumpToFile");
		AudioConvertToU16.Init(path, "Audio_AudioConvertToU16");

		HLELogging.Init(path, "HLE_HLELogging");
		HLEHookStFunc.Init(path, "HLE_HLEHookStFunc");
		HLESaveTTY.Init(path, "HLE_HLESaveTTY");
		HLEExitOnStop.Init(path, "HLE_HLEExitOnStop");
		HLELogLvl.Init(path, "HLE_HLELogLvl");
		HLEHideDebugConsole.Init(path, "HLE_HLEHideDebugConsole");

		SysLanguage.Init(path, "Sytem_SysLanguage");
	}

	void Load()
	{
		CPUDecoderMode.Load(2);
		CPUIgnoreRWErrors.Load(true);
		SPUDecoderMode.Load(1);
		GSRenderMode.Load(1);
		GSResolution.Load(4);
		GSAspectRatio.Load(2);
		GSVSyncEnable.Load(false);
		GSLogPrograms.Load(false);
		GSDumpColorBuffers.Load(false);
		GSDumpDepthBuffer.Load(false);
		SkipPamf.Load(false);
		PadHandlerMode.Load(1);
		KeyboardHandlerMode.Load(0);
		MouseHandlerMode.Load(0);
		AudioOutMode.Load(1);
		AudioDumpToFile.Load(false);
		AudioConvertToU16.Load(false);
		HLELogging.Load(false);
		HLEHookStFunc.Load(false);
		HLESaveTTY.Load(false);
		HLEExitOnStop.Load(false);
		HLELogLvl.Load(0);
		SysLanguage.Load(1);
		HLEHideDebugConsole.Load(false);

		PadHandlerLStickLeft.Load(314); //WXK_LEFT
		PadHandlerLStickDown.Load(317); //WXK_DOWN
		PadHandlerLStickRight.Load(316); //WXK_RIGHT
		PadHandlerLStickUp.Load(315); //WXK_UP
		PadHandlerLeft.Load(static_cast<int>('A'));
		PadHandlerDown.Load(static_cast<int>('S'));
		PadHandlerRight.Load(static_cast<int>('D'));
		PadHandlerUp.Load(static_cast<int>('W'));
		PadHandlerStart.Load(13); //WXK_RETURN
		PadHandlerR3.Load(static_cast<int>('C'));
		PadHandlerL3.Load(static_cast<int>('Z'));
		PadHandlerSelect.Load(32); //WXK_SPACE
		PadHandlerSquare.Load(static_cast<int>('J'));
		PadHandlerCross.Load(static_cast<int>('K'));
		PadHandlerCircle.Load(static_cast<int>('L'));
		PadHandlerTriangle.Load(static_cast<int>('I'));
		PadHandlerR1.Load(static_cast<int>('3'));
		PadHandlerL1.Load(static_cast<int>('1'));
		PadHandlerR2.Load(static_cast<int>('E'));
		PadHandlerL2.Load(static_cast<int>('Q'));
		PadHandlerRStickLeft.Load(313); //WXK_HOME
		PadHandlerRStickDown.Load(367); //WXK_PAGEDOWN
		PadHandlerRStickRight.Load(312); //WXK_END
		PadHandlerRStickUp.Load(366); //WXK_PAGEUP
	}

	void Save()
	{
		CPUDecoderMode.Save();
		CPUIgnoreRWErrors.Save();
		SPUDecoderMode.Save();
		GSRenderMode.Save();
		GSResolution.Save();
		GSAspectRatio.Save();
		GSVSyncEnable.Save();
		GSLogPrograms.Save();
		GSDumpColorBuffers.Save();
		GSDumpDepthBuffer.Save();
		SkipPamf.Save();
		PadHandlerMode.Save();
		KeyboardHandlerMode.Save();
		MouseHandlerMode.Save();
		AudioOutMode.Save();
		AudioDumpToFile.Save();
		AudioConvertToU16.Save();
		HLELogging.Save();
		HLEHookStFunc.Save();
		HLESaveTTY.Save();
		HLEExitOnStop.Save();
		HLELogLvl.Save();
		SysLanguage.Save();
		HLEHideDebugConsole.Save();

		PadHandlerLStickLeft.Save();
		PadHandlerLStickDown.Save();
		PadHandlerLStickRight.Save();
		PadHandlerLStickUp.Save();
		PadHandlerLeft.Save();
		PadHandlerDown.Save();
		PadHandlerRight.Save();
		PadHandlerUp.Save();
		PadHandlerStart.Save();
		PadHandlerR3.Save();
		PadHandlerL3.Save();
		PadHandlerSelect.Save();
		PadHandlerSquare.Save();
		PadHandlerCross.Save();
		PadHandlerCircle.Save();
		PadHandlerTriangle.Save();
		PadHandlerR1.Save();
		PadHandlerL1.Save();
		PadHandlerR2.Save();
		PadHandlerL2.Save();
		PadHandlerRStickLeft.Save();
		PadHandlerRStickDown.Save();
		PadHandlerRStickRight.Save();
		PadHandlerRStickUp.Save();
	}
};

extern Inis Ini;
