#pragma once

#include <utility>
#include "Utilities/simpleini/SimpleIni.h"

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

	void Init(const std::string& key, const std::string& section)
	{
		m_key = key;
		m_section = section;
	}

	void SetValue(const T& value)
	{
		m_value = value;
	}

	T GetValue() const
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
	// Core
	IniEntry<u8> CPUDecoderMode;
	IniEntry<u8> SPUDecoderMode;

	// Graphics
	IniEntry<u8> GSRenderMode;
	IniEntry<u8> GSResolution;
	IniEntry<u8> GSAspectRatio;
	IniEntry<u8> GSFrameLimit;
	IniEntry<bool> GSLogPrograms;
	IniEntry<bool> GSDumpColorBuffers;
	IniEntry<bool> GSDumpDepthBuffer;
	IniEntry<bool> GSReadColorBuffer;
	IniEntry<bool> GSVSyncEnable;
	IniEntry<bool> GS3DTV;

	// Audio
	IniEntry<u8> AudioOutMode;
	IniEntry<bool> AudioDumpToFile;
	IniEntry<bool> AudioConvertToU16;

	// Camera
	IniEntry<u8> Camera;
	IniEntry<u8> CameraType;

	// Input/Output
	IniEntry<u8> PadHandlerMode;
	IniEntry<u8> KeyboardHandlerMode;
	IniEntry<u8> MouseHandlerMode;
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

	// HLE/Miscs
	IniEntry<u8>   HLELogLvl;
	IniEntry<bool> HLELogging;
	IniEntry<bool> RSXLogging;
	IniEntry<bool> HLEHookStFunc;
	IniEntry<bool> HLESaveTTY;
	IniEntry<bool> HLEExitOnStop;
	IniEntry<bool> HLEAlwaysStart;

	//Auto Pause
	IniEntry<bool> DBGAutoPauseSystemCall;
	IniEntry<bool> DBGAutoPauseFunctionCall;

	// Language
	IniEntry<u8> SysLanguage;

public:
	Inis() : DefPath("EmuSettings")
	{
		std::string path;

		path = DefPath;

		// Core
		CPUDecoderMode.Init("CPU_DecoderMode", path);
		SPUDecoderMode.Init("CPU_SPUDecoderMode", path);

		// Graphics
		GSRenderMode.Init("GS_RenderMode", path);
		GSResolution.Init("GS_Resolution", path);
		GSAspectRatio.Init("GS_AspectRatio", path);
		GSFrameLimit.Init("GS_FrameLimit", path);
		GSLogPrograms.Init("GS_LogPrograms", path);
		GSDumpColorBuffers.Init("GS_DumpColorBuffers", path);
		GSDumpDepthBuffer.Init("GS_DumpDepthBuffer", path);
		GSReadColorBuffer.Init("GS_GSReadColorBuffer", path);
		GSVSyncEnable.Init("GS_VSyncEnable", path);
		GS3DTV.Init("GS_3DTV", path);

		// Audio
		AudioOutMode.Init("Audio_AudioOutMode", path);
		AudioDumpToFile.Init("Audio_AudioDumpToFile", path);
		AudioConvertToU16.Init("Audio_AudioConvertToU16", path);

		// Camera
		Camera.Init("Camera", path);
		CameraType.Init("Camera_Type", path);

		// Input/Output
		PadHandlerMode.Init("IO_PadHandlerMode", path);
		KeyboardHandlerMode.Init("IO_KeyboardHandlerMode", path);
		MouseHandlerMode.Init("IO_MouseHandlerMode", path);
		PadHandlerLStickLeft.Init("ControlSetings_PadHandlerLStickLeft", path);
		PadHandlerLStickDown.Init("ControlSetings_PadHandlerLStickDown", path);
		PadHandlerLStickRight.Init("ControlSetings_PadHandlerLStickRight", path);
		PadHandlerLStickUp.Init("ControlSetings_PadHandlerLStickUp", path);
		PadHandlerLeft.Init("ControlSetings_PadHandlerLeft", path);
		PadHandlerDown.Init("ControlSetings_PadHandlerDown", path);
		PadHandlerRight.Init("ControlSetings_PadHandlerRight", path);
		PadHandlerUp.Init("ControlSetings_PadHandlerUp", path);
		PadHandlerStart.Init("ControlSetings_PadHandlerStart", path);
		PadHandlerR3.Init("ControlSetings_PadHandlerR3", path);
		PadHandlerL3.Init("ControlSetings_PadHandlerL3", path);
		PadHandlerSelect.Init("ControlSetings_PadHandlerSelect", path);
		PadHandlerSquare.Init("ControlSetings_PadHandlerSquare", path);
		PadHandlerCross.Init("ControlSetings_PadHandlerCross", path);
		PadHandlerCircle.Init("ControlSetings_PadHandlerCircle", path);
		PadHandlerTriangle.Init("ControlSetings_PadHandlerTriangle", path);
		PadHandlerR1.Init("ControlSetings_PadHandlerR1", path);
		PadHandlerL1.Init("ControlSetings_PadHandlerL1", path);
		PadHandlerR2.Init("ControlSetings_PadHandlerR2", path);
		PadHandlerL2.Init("ControlSetings_PadHandlerL2", path);
		PadHandlerRStickLeft.Init("ControlSetings_PadHandlerRStickLeft", path);
		PadHandlerRStickDown.Init("ControlSetings_PadHandlerRStickDown", path);
		PadHandlerRStickRight.Init("ControlSetings_PadHandlerRStickRight", path);
		PadHandlerRStickUp.Init("ControlSetings_PadHandlerRStickUp", path);

		// HLE/Misc
		HLELogging.Init("HLE_HLELogging", path);
		RSXLogging.Init("RSX_Logging", path);
		HLEHookStFunc.Init("HLE_HLEHookStFunc", path);
		HLESaveTTY.Init("HLE_HLESaveTTY", path);
		HLEExitOnStop.Init("HLE_HLEExitOnStop", path);
		HLELogLvl.Init("HLE_HLELogLvl", path);
		HLEAlwaysStart.Init("HLE_HLEAlwaysStart", path);

		// Auto Pause
		DBGAutoPauseFunctionCall.Init("DBG_AutoPauseFunctionCall", path);
		DBGAutoPauseSystemCall.Init("DBG_AutoPauseSystemCall", path);

		// Language
		SysLanguage.Init("Sytem_SysLanguage", path);
	}

	void Load()
	{
		// Core
		CPUDecoderMode.Load(1);
		SPUDecoderMode.Load(1);

		// Graphics
		GSRenderMode.Load(1);
		GSResolution.Load(4);
		GSAspectRatio.Load(2);
		GSFrameLimit.Load(0);
		GSLogPrograms.Load(false);
		GSDumpColorBuffers.Load(false);
		GSDumpDepthBuffer.Load(false);
		GSReadColorBuffer.Load(false);
		GSVSyncEnable.Load(false);
		GS3DTV.Load(false);

		// Audio
		AudioOutMode.Load(1);
		AudioDumpToFile.Load(false);
		AudioConvertToU16.Load(false);

		// Camera
		Camera.Load(0);
		CameraType.Load(2);

		// Input/Ouput
		PadHandlerMode.Load(1);
		KeyboardHandlerMode.Load(0);
		MouseHandlerMode.Load(0);
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

		// HLE/Miscs
		HLELogging.Load(false);
		RSXLogging.Load(false);
		HLEHookStFunc.Load(false);
		HLESaveTTY.Load(false);
		HLEExitOnStop.Load(false);
		HLELogLvl.Load(3);
		HLEAlwaysStart.Load(true);

		//Auto Pause
		DBGAutoPauseFunctionCall.Load(false);
		DBGAutoPauseSystemCall.Load(false);

		// Language
		SysLanguage.Load(1);

	}

	void Save()
	{
		// CPU/SPU
		CPUDecoderMode.Save();
		SPUDecoderMode.Save();

		// Graphics
		GSRenderMode.Save();
		GSResolution.Save();
		GSAspectRatio.Save();
		GSFrameLimit.Save();
		GSLogPrograms.Save();
		GSDumpColorBuffers.Save();
		GSDumpDepthBuffer.Save();
		GSReadColorBuffer.Save();
		GSVSyncEnable.Save();
		GS3DTV.Save();

		// Audio 
		AudioOutMode.Save();
		AudioDumpToFile.Save();
		AudioConvertToU16.Save();

		// Camera
		Camera.Save();
		CameraType.Save();

		// Input/Output
		PadHandlerMode.Save();
		KeyboardHandlerMode.Save();
		MouseHandlerMode.Save();
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

		// HLE/Miscs
		HLELogging.Save();
		RSXLogging.Save();
		HLEHookStFunc.Save();
		HLESaveTTY.Save();
		HLEExitOnStop.Save();
		HLELogLvl.Save();
		HLEAlwaysStart.Save();

		//Auto Pause
		DBGAutoPauseFunctionCall.Save();
		DBGAutoPauseSystemCall.Save();

		// Language
		SysLanguage.Save();
	}
};

extern Inis Ini;
