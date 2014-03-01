#pragma once

#include <wx/config.h>

struct WindowInfo
{
	wxSize size;
	wxPoint position;

	WindowInfo(const wxSize _size = wxDefaultSize, const wxPoint _position = wxDefaultPosition)
		: size(_size)
		, position(_position)
	{
	}

	static WindowInfo& GetDefault()
	{
		return *new WindowInfo(wxDefaultSize, wxDefaultPosition);
	}
};

class Ini
{
protected:
	wxConfigBase*	m_Config;

	Ini();
	virtual void Save(wxString key, int value);
	virtual void Save(wxString key, bool value);
	virtual void Save(wxString key, wxSize value);
	virtual void Save(wxString key, wxPoint value);
	virtual void Save(wxString key, wxString value);
	virtual void Save(wxString key, WindowInfo value);

	virtual int Load(wxString key, const int def_value);
	virtual bool Load(wxString key, const bool def_value);
	virtual wxSize Load(wxString key, const wxSize def_value);
	virtual wxPoint Load(wxString key, const wxPoint def_value);
	virtual wxString Load(wxString key, const wxString& def_value);
	virtual WindowInfo Load(wxString key, const WindowInfo& def_value);
};

template<typename T> struct IniEntry : public Ini
{
	T m_value;
	wxString m_key;

	IniEntry() : Ini()
	{
	}

	void Init(const wxString& key, const wxString& path)
	{
		m_key = key;
		m_Config->SetPath(path);
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
		return Ini::Load(m_key, defvalue);
	}

	void SaveValue(const T& value)
	{
		Ini::Save(m_key, value);
	}

	void Save()
	{
		Ini::Save(m_key, m_value);
	}

	T Load(const T& defvalue)
	{
		return (m_value = Ini::Load(m_key, defvalue));
	}
};

class Inis
{
private:
	const wxString DefPath;

public:
	IniEntry<u8> CPUDecoderMode;
	IniEntry<bool> CPUIgnoreRWErrors;
	IniEntry<u8> GSRenderMode;
	IniEntry<int> GSResolution;
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
	IniEntry<bool> HLELogging;
	IniEntry<bool> HLESaveTTY;
	IniEntry<bool> HLEExitOnStop;

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

public:
	Inis() : DefPath("EmuSettings")
	{
		wxString path;

		path = DefPath + "\\" + "CPU";
		CPUDecoderMode.Init("DecoderMode", path);
		CPUIgnoreRWErrors.Init("IgnoreRWErrors", path);

		path = DefPath + "\\" + "GS";
		GSRenderMode.Init("RenderMode", path);
		GSResolution.Init("Resolution", path);
		GSAspectRatio.Init("AspectRatio", path);
		GSVSyncEnable.Init("VSyncEnable", path);
		GSLogPrograms.Init("LogPrograms", path);
		GSDumpColorBuffers.Init("DumpColorBuffers", path);
		GSDumpDepthBuffer.Init("DumpDepthBuffer", path);

		path = DefPath + "\\" + "IO";
		PadHandlerMode.Init("PadHandlerMode", path);
		KeyboardHandlerMode.Init("KeyboardHandlerMode", path);
		MouseHandlerMode.Init("MouseHandlerMode", path);

		path = DefPath + "\\" + "ControlSetings";
		PadHandlerLeft.Init("PadHandlerLeft", path);
		PadHandlerDown.Init("PadHandlerDown", path);
		PadHandlerRight.Init("PadHandlerRight", path);
		PadHandlerUp.Init("PadHandlerUp", path);
		PadHandlerStart.Init("PadHandlerStart", path);
		PadHandlerR3.Init("PadHandlerR3", path);
		PadHandlerL3.Init("PadHandlerL3", path);
		PadHandlerSelect.Init("PadHandlerSelect", path);
		PadHandlerSquare.Init("PadHandlerSquare", path);
		PadHandlerCross.Init("PadHandlerCross", path);
		PadHandlerCircle.Init("PadHandlerCircle", path);
		PadHandlerTriangle.Init("PadHandlerTriangle", path);
		PadHandlerR1.Init("PadHandlerR1", path);
		PadHandlerL1.Init("PadHandlerL1", path);
		PadHandlerR2.Init("PadHandlerR2", path);
		PadHandlerL2.Init("PadHandlerL2", path);

		path = DefPath + "\\" + "Audio";
		AudioOutMode.Init("AudioOutMode", path);
		AudioDumpToFile.Init("AudioDumpToFile", path);

		path = DefPath + "\\" + "HLE";
		HLELogging.Init("HLELogging", path);
		HLESaveTTY.Init("HLESaveTTY", path);
		HLEExitOnStop.Init("HLEExitOnStop", path);
	}

	void Load()
	{
		CPUDecoderMode.Load(2);
		CPUIgnoreRWErrors.Load(false);
		GSRenderMode.Load(1);
		GSResolution.Load(4);
		GSAspectRatio.Load(2);
		GSVSyncEnable.Load(false);
		GSLogPrograms.Load(false);
		GSDumpColorBuffers.Load(true);
		GSDumpDepthBuffer.Load(true);
		PadHandlerMode.Load(1);
		KeyboardHandlerMode.Load(0);
		MouseHandlerMode.Load(0);
		AudioOutMode.Load(0);
		AudioDumpToFile.Load(0);
		HLELogging.Load(false);
		HLESaveTTY.Load(false);
		HLEExitOnStop.Load(false);

		PadHandlerLeft.Load(static_cast<int>('A'));
		PadHandlerDown.Load(static_cast<int>('S'));
		PadHandlerRight.Load(static_cast<int>('D'));
		PadHandlerUp.Load(static_cast<int>('W'));
		PadHandlerStart.Load(0);
		PadHandlerR3.Load(static_cast<int>('C'));
		PadHandlerL3.Load(static_cast<int>('Z'));
		PadHandlerSelect.Load(0);
		PadHandlerSquare.Load(static_cast<int>('J'));
		PadHandlerCross.Load(static_cast<int>('K'));
		PadHandlerCircle.Load(static_cast<int>('L'));
		PadHandlerTriangle.Load(static_cast<int>('I'));
		PadHandlerR1.Load(static_cast<int>('3'));
		PadHandlerL1.Load(static_cast<int>('1'));
		PadHandlerR2.Load(static_cast<int>('E'));
		PadHandlerL2.Load(static_cast<int>('Q'));
	}

	void Save()
	{
		CPUDecoderMode.Save();
		CPUIgnoreRWErrors.Save();
		GSRenderMode.Save();
		GSResolution.Save();
		GSAspectRatio.Save();
		GSVSyncEnable.Save();
		GSLogPrograms.Save();
		GSDumpColorBuffers.Save();
		GSDumpDepthBuffer.Save();
		PadHandlerMode.Save();
		KeyboardHandlerMode.Save();
		MouseHandlerMode.Save();
		AudioOutMode.Save();
		AudioDumpToFile.Save();
		HLELogging.Save();
		HLESaveTTY.Save();
		HLEExitOnStop.Save();

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
	}
};

extern Inis Ini;
