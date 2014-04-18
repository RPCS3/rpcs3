#pragma once

#define LOAD_SYMBOL(x) if(m_dll.HasSymbol(#x)) x = (_##x)m_dll.GetSymbol(#x)
struct Ps3EmuPlugin
{
	enum PS3EMULIB_TYPE
	{
		LIB_PAD		= (1 << 0),
		LIB_KB		= (1 << 1),
		LIB_MOUSE	= (1 << 2),
		LIB_FS		= (1 << 3),
	};

	typedef u32 (*_Ps3EmuLibGetType)();
	typedef u32 (*_Ps3EmuLibGetVersion)();
	typedef const char* (*_Ps3EmuLibGetName)();
	typedef const char* (*_Ps3EmuLibGetFullName)();
	typedef void (*_Ps3EmuLibSetSettingsPath)(const char* path);
	typedef void (*_Ps3EmuLibSetLogPath)(const char* path);
	typedef const char* (*_Ps3EmuLibGetSettingsName)();
	typedef const char* (*_Ps3EmuLibGetLogName)();
	typedef void (*_Ps3EmuLibConfigure)(PS3EMULIB_TYPE type);
	typedef void (*_Ps3EmuLibAbout)();
	typedef u32 (*_Ps3EmuLibTest)(PS3EMULIB_TYPE type);

	_Ps3EmuLibGetType			Ps3EmuLibGetType;
	_Ps3EmuLibGetVersion		Ps3EmuLibGetVersion;
	_Ps3EmuLibGetName			Ps3EmuLibGetName;
	_Ps3EmuLibGetFullName		Ps3EmuLibGetFullName;
	_Ps3EmuLibSetSettingsPath	Ps3EmuLibSetSettingsPath;
	_Ps3EmuLibSetLogPath		Ps3EmuLibSetLogPath;
	_Ps3EmuLibGetSettingsName	Ps3EmuLibGetSettingsName;
	_Ps3EmuLibGetLogName		Ps3EmuLibGetLogName;
	_Ps3EmuLibConfigure			Ps3EmuLibConfigure;
	_Ps3EmuLibAbout				Ps3EmuLibAbout;
	_Ps3EmuLibTest				Ps3EmuLibTest;

	wxDynamicLibrary m_dll;

	Ps3EmuPlugin()
	{
		Reset();
	}

	Ps3EmuPlugin(const wxString& path)
	{
		Load(path);
	}

	virtual ~Ps3EmuPlugin() throw()
	{
		Unload();
	}

	wxString FormatVersion()
	{
		if(!Ps3EmuLibGetVersion) return wxEmptyString;

		const u32 v = Ps3EmuLibGetVersion();

		const u8 v0 = v >> 24;
		const u8 v1 = v >> 16;
		const u8 v2 = v >> 8;
		const u8 v3 = v;

		if(!v2 && !v3) return wxString::Format("%d.%d", v0, v1);
		if(!v3) return wxString::Format("%d.%d.%d", v0, v1, v2);

		return wxString::Format("%d.%d.%d.%d", v0, v1, v2, v3);
	}

	void Load(const wxString& path)
	{
		if(m_dll.Load(path))
		{
			Init();
		}
		else
		{
			Reset();
		}
	}

	void Unload()
	{
		Reset();

		if(m_dll.IsLoaded()) m_dll.Unload();
	}

	virtual bool Test()
	{
		return
			m_dll.IsLoaded()			&&
			Ps3EmuLibGetType			&&
			Ps3EmuLibGetVersion			&&
			Ps3EmuLibGetName			&&
			Ps3EmuLibGetFullName		&&
			Ps3EmuLibSetSettingsPath	&&
			Ps3EmuLibSetLogPath			&&
			Ps3EmuLibGetSettingsName	&&
			Ps3EmuLibGetLogName			&&
			Ps3EmuLibConfigure			&&
			Ps3EmuLibAbout				&&
			Ps3EmuLibTest;
	}

protected:
	virtual void Init()
	{
		LOAD_SYMBOL(Ps3EmuLibGetType);
		LOAD_SYMBOL(Ps3EmuLibGetVersion);
		LOAD_SYMBOL(Ps3EmuLibGetName);
		LOAD_SYMBOL(Ps3EmuLibGetFullName);
		LOAD_SYMBOL(Ps3EmuLibSetSettingsPath);
		LOAD_SYMBOL(Ps3EmuLibSetLogPath);
		LOAD_SYMBOL(Ps3EmuLibGetSettingsName);
		LOAD_SYMBOL(Ps3EmuLibGetLogName);
		LOAD_SYMBOL(Ps3EmuLibConfigure);
		LOAD_SYMBOL(Ps3EmuLibAbout);
		LOAD_SYMBOL(Ps3EmuLibTest);
	}

	virtual void Reset()
	{
		Ps3EmuLibGetType			= nullptr;
		Ps3EmuLibGetVersion			= nullptr;
		Ps3EmuLibGetName			= nullptr;
		Ps3EmuLibGetFullName		= nullptr;
		Ps3EmuLibSetSettingsPath	= nullptr;
		Ps3EmuLibSetLogPath			= nullptr;
		Ps3EmuLibGetSettingsName	= nullptr;
		Ps3EmuLibGetLogName			= nullptr;
		Ps3EmuLibConfigure			= nullptr;
		Ps3EmuLibAbout				= nullptr;
		Ps3EmuLibTest				= nullptr;
	}
};

struct Ps3EmuPluginPAD : public Ps3EmuPlugin
{
	typedef int (*_PadInit)(u32 max_connect);
	typedef int (*_PadEnd)();
	typedef int (*_PadClearBuf)(u32 port_no);
	typedef int (*_PadGetData)(u32 port_no, void* data);
	typedef int (*_PadGetDataExtra)(u32 port_no, u32* device_type, void* data);
	typedef int (*_PadSetActDirect)(u32 port_no, void* param);
	typedef int (*_PadGetInfo)(void* info);
	typedef int (*_PadGetInfo2)(void* info);
	typedef int (*_PadSetPortSetting)(u32 port_no, u32 port_setting);
	typedef int (*_PadLddRegisterController)();
	typedef int (*_PadLddUnregisterController)(int handle);
	typedef int (*_PadLddDataInsert)(int handle, void* data);
	typedef int (*_PadLddGetPortNo)(int handle);
	typedef int (*_PadPeriphGetInfo)(void* info);
	typedef int (*_PadPeriphGetData)(u32 port_no, void* data);
	typedef int (*_PadInfoPressMode)(u32 port_no);
	typedef int (*_PadSetPressMode)(u32 port_no, u8 mode);
	typedef int (*_PadInfoSensorMode)(u32 port_no);
	typedef int (*_PadSetSensorMode)(u32 port_no, u8 mode);
	typedef int (*_PadGetRawData)(u32 port_no, void* data);
	typedef int (*_PadDbgLddRegisterController)(u32 device_capability);
	typedef int (*_PadDbgLddSetDataInsertMode)(int handle, u8 mode);
	typedef int (*_PadDbgPeriphRegisterDevice)(u16 vid, u16 pid, u32 pclass_type, u32 pclass_profile, void* mapping);
	typedef int (*_PadDbgGetData)(u32 port_no, void* data);

	_PadInit						PadInit;
	_PadEnd							PadEnd;
	_PadClearBuf					PadClearBuf;
	_PadGetData						PadGetData;
	_PadGetDataExtra				PadGetDataExtra;
	_PadSetActDirect				PadSetActDirect;
	_PadGetInfo						PadGetInfo;
	_PadGetInfo2					PadGetInfo2;
	_PadSetPortSetting				PadSetPortSetting;
	_PadLddRegisterController		PadLddRegisterController;
	_PadLddUnregisterController		PadLddUnregisterController;
	_PadLddDataInsert				PadLddDataInsert;
	_PadLddGetPortNo				PadLddGetPortNo;
	_PadPeriphGetInfo				PadPeriphGetInfo;
	_PadPeriphGetData				PadPeriphGetData;
	_PadInfoPressMode				PadInfoPressMode;
	_PadSetPressMode				PadSetPressMode;
	_PadInfoSensorMode				PadInfoSensorMode;
	_PadSetSensorMode				PadSetSensorMode;
	_PadGetRawData					PadGetRawData;
	_PadDbgLddRegisterController	PadDbgLddRegisterController;
	_PadDbgLddSetDataInsertMode		PadDbgLddSetDataInsertMode;
	_PadDbgPeriphRegisterDevice		PadDbgPeriphRegisterDevice;
	_PadDbgGetData					PadDbgGetData;
	
	Ps3EmuPluginPAD()
	{
		Reset();
	}

	Ps3EmuPluginPAD(const wxString& path)
	{
		Load(path);
	}

	virtual bool Test()
	{
		return Ps3EmuPlugin::Test()		&&
			PadInit						&&
			PadEnd						&&
			PadClearBuf					&&
			PadGetData					&&
			PadGetDataExtra				&&
			PadSetActDirect				&&
			PadGetInfo					&&
			PadGetInfo2					&&
			PadSetPortSetting			&&
			PadLddRegisterController	&&
			PadLddUnregisterController	&&
			PadLddDataInsert			&&
			PadLddGetPortNo				&&
			PadPeriphGetInfo			&&
			PadPeriphGetData			&&
			PadInfoPressMode			&&
			PadSetPressMode				&&
			PadInfoSensorMode			&&
			PadSetSensorMode			&&
			PadGetRawData				&&
			PadDbgLddRegisterController	&&
			PadDbgLddSetDataInsertMode	&&
			PadDbgPeriphRegisterDevice	&&
			PadDbgGetData;
	}

protected:
	virtual void Init()
	{
		LOAD_SYMBOL(PadInit);
		LOAD_SYMBOL(PadEnd);
		LOAD_SYMBOL(PadClearBuf);
		LOAD_SYMBOL(PadGetData);
		LOAD_SYMBOL(PadGetDataExtra);
		LOAD_SYMBOL(PadSetActDirect);
		LOAD_SYMBOL(PadGetInfo);
		LOAD_SYMBOL(PadGetInfo2);
		LOAD_SYMBOL(PadSetPortSetting);
		LOAD_SYMBOL(PadLddRegisterController);
		LOAD_SYMBOL(PadLddUnregisterController);
		LOAD_SYMBOL(PadLddDataInsert);
		LOAD_SYMBOL(PadLddGetPortNo);
		LOAD_SYMBOL(PadPeriphGetInfo);
		LOAD_SYMBOL(PadPeriphGetData);
		LOAD_SYMBOL(PadInfoPressMode);
		LOAD_SYMBOL(PadSetPressMode);
		LOAD_SYMBOL(PadInfoSensorMode);
		LOAD_SYMBOL(PadSetSensorMode);
		LOAD_SYMBOL(PadGetRawData);
		LOAD_SYMBOL(PadDbgLddRegisterController);
		LOAD_SYMBOL(PadDbgLddSetDataInsertMode);
		LOAD_SYMBOL(PadDbgPeriphRegisterDevice);
		LOAD_SYMBOL(PadDbgGetData);

		Ps3EmuPlugin::Init();
	}

	virtual void Reset()
	{
		PadInit						= nullptr;
		PadEnd						= nullptr;
		PadClearBuf					= nullptr;
		PadGetData					= nullptr;
		PadGetDataExtra				= nullptr;
		PadSetActDirect				= nullptr;
		PadGetInfo					= nullptr;
		PadGetInfo2					= nullptr;
		PadSetPortSetting			= nullptr;
		PadLddRegisterController	= nullptr;
		PadLddUnregisterController	= nullptr;
		PadLddDataInsert			= nullptr;
		PadLddGetPortNo				= nullptr;
		PadPeriphGetInfo			= nullptr;
		PadPeriphGetData			= nullptr;
		PadInfoPressMode			= nullptr;
		PadSetPressMode				= nullptr;
		PadInfoSensorMode			= nullptr;
		PadSetSensorMode			= nullptr;
		PadGetRawData				= nullptr;
		PadDbgLddRegisterController	= nullptr;
		PadDbgLddSetDataInsertMode	= nullptr;
		PadDbgPeriphRegisterDevice	= nullptr;
		PadDbgGetData				= nullptr;

		Ps3EmuPlugin::Reset();
	}
};

struct PluginsManager
{
	struct PluginInfo
	{
		wxString path;
		wxString name;
		u32 type;
	};

	std::vector<PluginInfo> m_plugins;
	std::vector<PluginInfo*> m_pad_plugins;

	void Load(const wxString& path)
	{
		if(!wxDirExists(path)) return;

		m_plugins.clear();
		wxDir dir(path);

		wxArrayString res;
		wxDir::GetAllFiles(path, &res, "*.dll", wxDIR_FILES);

		for(u32 i=0; i<res.GetCount(); ++i)
		{
			ConLog.Write("loading " + res[i] + "...");
			Ps3EmuPlugin l(res[i]);
			if(!l.Test()) continue;

			m_plugins.emplace_back();
			PluginInfo& info = m_plugins.back();
			info.path = res[i];
			info.name.Printf("%s v%s", l.Ps3EmuLibGetFullName(), l.FormatVersion());
			info.type = l.Ps3EmuLibGetType();
		}
	}

	wxSizer* GetNewComboBox(wxWindow* parent, const wxString& name, wxComboBox*& cb)
	{
		wxBoxSizer* s_panel(new wxBoxSizer(wxHORIZONTAL));

		s_panel->Add(new wxStaticText(parent, wxID_ANY, name), wxSizerFlags().Border(wxRIGHT, 5).Center().Left());
		s_panel->Add(cb = new wxComboBox(parent, wxID_ANY), wxSizerFlags().Center().Right());

		return s_panel;
	}

	void Dialog()
	{
		wxDialog dial(nullptr, wxID_ANY, "Select plugins...", wxDefaultPosition);

		wxBoxSizer& s_panel(*new wxBoxSizer(wxVERTICAL));

		wxComboBox* cbox_pad_plugins;
		s_panel.Add(GetNewComboBox(&dial, "Pad", cbox_pad_plugins), wxSizerFlags().Border(wxALL, 5).Expand());

		for(u32 i=0; i<m_plugins.size(); ++i)
		{
			if(m_plugins[i].type & Ps3EmuPlugin::LIB_PAD)
			{
				m_pad_plugins.push_back(&m_plugins[i]);
				cbox_pad_plugins->Append(m_plugins[i].name + " (" + wxFileName(m_plugins[i].path).GetName() + ")");
			}
		}

		if(cbox_pad_plugins->GetCount()) cbox_pad_plugins->Select(0);

		dial.SetSizerAndFit(&s_panel);
		dial.ShowModal();
	}
};
