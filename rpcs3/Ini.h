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
	IniEntry<u8> GSRenderMode;
	IniEntry<int> GSResolution;
	IniEntry<u8> GSAspectRatio;
	IniEntry<bool> GSVSyncEnable;
	IniEntry<u8> PadHandlerMode;

public:
	Inis() : DefPath("EmuSettings")
	{
		wxString path;

		path = DefPath + "\\" + "CPU";
		CPUDecoderMode.Init("DecoderMode", path);

		path = DefPath + "\\" + "GS";
		GSRenderMode.Init("RenderMode", path);
		GSResolution.Init("Resolution", path);
		GSAspectRatio.Init("AspectRatio", path);
		GSVSyncEnable.Init("VSyncEnable", path);

		path = DefPath + "\\" + "Pad";
		PadHandlerMode.Init("HandlerMode", path);
	}

	void Load()
	{
		CPUDecoderMode.Load(2);
		GSRenderMode.Load(0);
		GSResolution.Load(4);
		GSAspectRatio.Load(1);
		GSVSyncEnable.Load(false);
		PadHandlerMode.Load(0);
	}

	void Save()
	{
		CPUDecoderMode.Save();
		GSRenderMode.Save();
		GSResolution.Save();
		GSAspectRatio.Save();
		GSVSyncEnable.Save();
		PadHandlerMode.Save();
	}
};

extern Inis Ini;