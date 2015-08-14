#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "rpcs3.h"
#include "MainFrame.h"

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#else
#include "frame_icon.xpm"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#include "git-version.h"
#include "Ini.h"
#include "Emu/SysCalls/Modules/cellSysutil.h"
#include "Emu/SysCalls/Modules/cellVideoOut.h"
#include "Gui/PADManager.h"
#include "Gui/VHDDManager.h"
#include "Gui/VFSManager.h"
#include "Gui/AboutDialog.h"
#include "Gui/GameViewer.h"
#include "Gui/CompilerELF.h"
#include "Gui/AutoPauseManager.h"
#include "Gui/SaveDataUtility.h"
#include "Gui/KernelExplorer.h"
#include "Gui/MemoryViewer.h"
#include "Gui/RSXDebugger.h"
#include "Gui/MemoryStringSearcher.h"
#include "Gui/LLEModulesManager.h"
#include "Gui/CgDisasm.h"

#include <wx/dynlib.h>

#include "Loader/PKG.h"

BEGIN_EVENT_TABLE(MainFrame, FrameBase)
	EVT_CLOSE(MainFrame::OnQuit)
END_EVENT_TABLE()

enum IDs
{
	id_boot_elf = 0x555,
	id_boot_game,
	id_boot_install_pkg,
	id_boot_exit,
	id_sys_pause,
	id_sys_stop,
	id_sys_send_open_menu,
	id_sys_send_exit,
	id_config_emu,
	id_config_pad,
	id_config_vfs_manager,
	id_config_vhdd_manager,
	id_config_autopause_manager,
	id_config_savedata_manager,
	id_config_lle_modules_manager,
	id_tools_compiler,
	id_tools_kernel_explorer,
	id_tools_memory_viewer,
	id_tools_rsx_debugger,
	id_tools_string_search,
	id_tools_cg_disasm,
	id_help_about,
	id_update_dbg
};

wxString GetPaneName()
{
	static int pane_num = 0;

	return wxString::Format("Pane_%d", pane_num++);
}

MainFrame::MainFrame()
	: FrameBase(nullptr, wxID_ANY, "", "MainFrame", wxSize(900, 600))
	, m_aui_mgr(this)
	, m_sys_menu_opened(false)
{

	SetLabel(wxString::Format(_PRGNAME_ " " RPCS3_GIT_VERSION));

	wxMenuBar* menubar = new wxMenuBar();

	wxMenu* menu_boot = new wxMenu();
	menubar->Append(menu_boot, "&Boot");
	menu_boot->Append(id_boot_elf, "Boot &ELF / SELF file");
	menu_boot->Append(id_boot_game, "Boot &game");
	menu_boot->AppendSeparator();
	menu_boot->Append(id_boot_install_pkg, "&Install PKG");
	menu_boot->AppendSeparator();
	menu_boot->Append(id_boot_exit, "&Exit");

	wxMenu* menu_sys = new wxMenu();
	menubar->Append(menu_sys, "&System");
	menu_sys->Append(id_sys_pause, "&Pause")->Enable(false);
	menu_sys->Append(id_sys_stop, "&Stop\tCtrl + S")->Enable(false);
	menu_sys->AppendSeparator();
	menu_sys->Append(id_sys_send_open_menu, "Send &open system menu cmd")->Enable(false);
	menu_sys->Append(id_sys_send_exit, "Send &exit cmd")->Enable(false);

	wxMenu* menu_conf = new wxMenu();
	menubar->Append(menu_conf, "&Config");
	menu_conf->Append(id_config_emu, "&Settings");
	menu_conf->Append(id_config_pad, "&PAD Settings");
	menu_conf->AppendSeparator();
	menu_conf->Append(id_config_autopause_manager, "&Auto Pause Settings");
	menu_conf->AppendSeparator();
	menu_conf->Append(id_config_vfs_manager, "Virtual &File System Manager");
	menu_conf->Append(id_config_vhdd_manager, "Virtual &HDD Manager");
	menu_conf->Append(id_config_savedata_manager, "Save &Data Utility");
	menu_conf->Append(id_config_lle_modules_manager, "&LLE Modules Manager");


	wxMenu* menu_tools = new wxMenu();
	menubar->Append(menu_tools, "&Tools");
	menu_tools->Append(id_tools_compiler, "&ELF Compiler");
	menu_tools->Append(id_tools_kernel_explorer, "&Kernel Explorer")->Enable(false);
	menu_tools->Append(id_tools_memory_viewer, "&Memory Viewer")->Enable(false);
	menu_tools->Append(id_tools_rsx_debugger, "&RSX Debugger")->Enable(false);
	menu_tools->Append(id_tools_string_search, "&String Search")->Enable(false);
	menu_tools->Append(id_tools_cg_disasm, "&Cg Disasm")->Enable();

	wxMenu* menu_help = new wxMenu();
	menubar->Append(menu_help, "&Help");
	menu_help->Append(id_help_about, "&About...");

	SetMenuBar(menubar);
#ifdef _WIN32
	SetIcon(wxICON(frame_icon));
#endif

	// Panels
	m_log_frame = new LogFrame(this);
	m_game_viewer = new GameViewer(this);
	m_debugger_frame = new DebuggerPanel(this);

	AddPane(m_game_viewer, "Game List", wxAUI_DOCK_CENTRE);
	AddPane(m_log_frame, "Log", wxAUI_DOCK_BOTTOM);
	AddPane(m_debugger_frame, "Debugger", wxAUI_DOCK_RIGHT);
	
	// Events
	Bind(wxEVT_MENU, &MainFrame::BootElf, this, id_boot_elf);
	Bind(wxEVT_MENU, &MainFrame::BootGame, this, id_boot_game);
	Bind(wxEVT_MENU, &MainFrame::InstallPkg, this, id_boot_install_pkg);
	Bind(wxEVT_MENU, [](wxCommandEvent&){ wxGetApp().Exit(); }, id_boot_exit);

	Bind(wxEVT_MENU, &MainFrame::Pause, this, id_sys_pause);
	Bind(wxEVT_MENU, &MainFrame::Stop, this, id_sys_stop);
	Bind(wxEVT_MENU, &MainFrame::SendOpenCloseSysMenu, this, id_sys_send_open_menu);
	Bind(wxEVT_MENU, &MainFrame::SendExit, this, id_sys_send_exit);

	Bind(wxEVT_MENU, &MainFrame::Config, this, id_config_emu);
	Bind(wxEVT_MENU, &MainFrame::ConfigPad, this, id_config_pad);
	Bind(wxEVT_MENU, &MainFrame::ConfigVFS, this, id_config_vfs_manager);
	Bind(wxEVT_MENU, &MainFrame::ConfigVHDD, this, id_config_vhdd_manager);
	Bind(wxEVT_MENU, &MainFrame::ConfigAutoPause, this, id_config_autopause_manager);
	Bind(wxEVT_MENU, &MainFrame::ConfigSaveData, this, id_config_savedata_manager);
	Bind(wxEVT_MENU, &MainFrame::ConfigLLEModules, this, id_config_lle_modules_manager);

	Bind(wxEVT_MENU, &MainFrame::OpenELFCompiler, this, id_tools_compiler);
	Bind(wxEVT_MENU, &MainFrame::OpenKernelExplorer, this, id_tools_kernel_explorer);
	Bind(wxEVT_MENU, &MainFrame::OpenMemoryViewer, this, id_tools_memory_viewer);
	Bind(wxEVT_MENU, &MainFrame::OpenRSXDebugger, this, id_tools_rsx_debugger);
	Bind(wxEVT_MENU, &MainFrame::OpenStringSearch, this, id_tools_string_search);
	Bind(wxEVT_MENU, &MainFrame::OpenCgDisasm, this, id_tools_cg_disasm);

	Bind(wxEVT_MENU, &MainFrame::AboutDialogHandler, this, id_help_about);

	Bind(wxEVT_MENU, &MainFrame::UpdateUI, this, id_update_dbg);

	wxGetApp().Bind(wxEVT_KEY_DOWN, &MainFrame::OnKeyDown, this);
	wxGetApp().Bind(wxEVT_DBG_COMMAND, &MainFrame::UpdateUI, this);
}

MainFrame::~MainFrame()
{
	m_aui_mgr.UnInit();
}

void MainFrame::AddPane(wxWindow* wind, const wxString& caption, int flags)
{
	wind->SetSize(-1, 300);
	m_aui_mgr.AddPane(wind, wxAuiPaneInfo().Name(GetPaneName()).Caption(caption).Direction(flags).CloseButton(false).MaximizeButton());
}

void MainFrame::DoSettings(bool load)
{
	IniEntry<std::string> ini;
	ini.Init("Settings", "MainFrameAui");

	if(load)
	{
		m_aui_mgr.LoadPerspective(fmt::FromUTF8(ini.LoadValue(fmt::ToUTF8(m_aui_mgr.SavePerspective()))));
	}
	else
	{
		ini.SaveValue(fmt::ToUTF8(m_aui_mgr.SavePerspective()));
	}
}

void MainFrame::BootGame(wxCommandEvent& WXUNUSED(event))
{
	bool stopped = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	wxDirDialog ctrl(this, L"Select game folder", wxEmptyString);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stopped) Emu.Resume();
		return;
	}

	Emu.Stop();
	
	if(Emu.BootGame(ctrl.GetPath().ToStdString()))
	{
		LOG_SUCCESS(HLE, "Game: boot done.");

		if (Ini.HLEAlwaysStart.GetValue() && Emu.IsReady())
		{
			Emu.Run();
		}
	}
	else
	{
		LOG_ERROR(HLE, "PS3 executable not found in selected folder (%s)", fmt::ToUTF8(ctrl.GetPath())); // passing std::string (test)
	}
}

void MainFrame::InstallPkg(wxCommandEvent& WXUNUSED(event))
{
	bool stopped = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	wxFileDialog ctrl(this, L"Select PKG", wxEmptyString, wxEmptyString, "PKG files (*.pkg)|*.pkg|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stopped) Emu.Resume();
		return;
	}

	Emu.Stop();
	
	// Open and install PKG file
	fs::file pkg_f(ctrl.GetPath().ToStdString(), o_read);

	if (pkg_f)
	{
		Emu.GetVFS().Init("/");
		std::string local_path;
		Emu.GetVFS().GetDevice("/dev_hdd0/game/", local_path);
		PKGLoader::Install(pkg_f, local_path + "/");

		// Refresh game list
		m_game_viewer->Refresh();
	}
}

void MainFrame::BootElf(wxCommandEvent& WXUNUSED(event))
{
	bool stopped = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	wxFileDialog ctrl(this, L"Select (S)ELF", wxEmptyString, wxEmptyString,
		"(S)ELF files (*BOOT.BIN;*.elf;*.self)|*BOOT.BIN;*.elf;*.self"
		"|ELF files (BOOT.BIN;*.elf)|BOOT.BIN;*.elf"
		"|SELF files (EBOOT.BIN;*.self)|EBOOT.BIN;*.self"
		"|BOOT files (*BOOT.BIN)|*BOOT.BIN"
		"|BIN files (*.bin)|*.bin"
		"|All files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stopped) Emu.Resume();
		return;
	}

	LOG_NOTICE(HLE, "(S)ELF: booting...");

	Emu.Stop();
	Emu.SetPath(fmt::ToUTF8(ctrl.GetPath()));
	Emu.Load();

	LOG_SUCCESS(HLE, "(S)ELF: boot done.");
	
	if (Ini.HLEAlwaysStart.GetValue() && Emu.IsReady())
	{
		Emu.Run();
	}
}

void MainFrame::Pause(wxCommandEvent& WXUNUSED(event))
{
	if(Emu.IsReady())
	{
		Emu.Run();
	}
	else if(Emu.IsPaused())
	{
		Emu.Resume();
	}
	else if(Emu.IsRunning())
	{
		Emu.Pause();
	}
}

void MainFrame::Stop(wxCommandEvent& WXUNUSED(event))
{
	Emu.Stop();
}

void MainFrame::SendExit(wxCommandEvent& event)
{
	sysutilSendSystemCommand(CELL_SYSUTIL_REQUEST_EXITGAME, 0);
}

void MainFrame::SendOpenCloseSysMenu(wxCommandEvent& event)
{
	sysutilSendSystemCommand(m_sys_menu_opened ? CELL_SYSUTIL_SYSTEM_MENU_CLOSE : CELL_SYSUTIL_SYSTEM_MENU_OPEN, 0);
	m_sys_menu_opened = !m_sys_menu_opened;
	wxCommandEvent ce;
	UpdateUI(ce);
}

void MainFrame::Config(wxCommandEvent& WXUNUSED(event))
{
	bool paused = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		paused = true;
	}

	wxDialog diag(this, wxID_ANY, "Settings", wxDefaultPosition);
	static const u32 width = 458;
	static const u32 height = 520;

	// Settings panels
	wxNotebook* nb_config = new wxNotebook(&diag, wxID_ANY, wxPoint(6,6), wxSize(width, height));
	wxPanel* p_system     = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_core       = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_graphics   = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_audio      = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_io         = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_misc       = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_networking = new wxPanel(nb_config, wxID_ANY);

	nb_config->AddPage(p_core,       wxT("Core"));
	nb_config->AddPage(p_graphics,   wxT("Graphics"));
	nb_config->AddPage(p_audio,      wxT("Audio"));
	nb_config->AddPage(p_io,         wxT("Input / Output"));
	nb_config->AddPage(p_misc,       wxT("Miscellaneous"));
	nb_config->AddPage(p_networking, wxT("Networking"));
	nb_config->AddPage(p_system,     wxT("System"));

	wxBoxSizer* s_subpanel_system     = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_core       = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_graphics   = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_audio      = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_io         = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_misc       = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_networking = new wxBoxSizer(wxVERTICAL);

	// Core settings
	wxStaticBoxSizer* s_round_cpu_decoder = new wxStaticBoxSizer(wxVERTICAL, p_core, _("CPU"));
	wxStaticBoxSizer* s_round_spu_decoder = new wxStaticBoxSizer(wxVERTICAL, p_core, _("SPU"));
	wxStaticBoxSizer* s_round_llvm = new wxStaticBoxSizer(wxVERTICAL, p_core, _("LLVM config"));
	wxStaticBoxSizer* s_round_llvm_range = new wxStaticBoxSizer(wxHORIZONTAL, p_core, _("Excluded block range"));
	wxStaticBoxSizer* s_round_llvm_threshold = new wxStaticBoxSizer(wxHORIZONTAL, p_core, _("Compilation threshold"));

	// Graphics
	wxStaticBoxSizer* s_round_gs_render = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Render"));
	wxStaticBoxSizer* s_round_gs_d3d_adaptater = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("D3D Adaptater"));
	wxStaticBoxSizer* s_round_gs_res    = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Default resolution"));
	wxStaticBoxSizer* s_round_gs_aspect = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Default aspect ratio"));
	wxStaticBoxSizer* s_round_gs_frame_limit = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Frame limit"));

	// Input / Output
	wxStaticBoxSizer* s_round_io_pad_handler      = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Pad Handler"));
	wxStaticBoxSizer* s_round_io_keyboard_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Keyboard Handler"));
	wxStaticBoxSizer* s_round_io_mouse_handler    = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Mouse Handler"));
	wxStaticBoxSizer* s_round_io_camera           = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Camera"));
	wxStaticBoxSizer* s_round_io_camera_type      = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Camera type"));
	
	// Audio
	wxStaticBoxSizer* s_round_audio_out = new wxStaticBoxSizer(wxVERTICAL, p_audio, _("Audio Out"));

	// Miscellaneous
	wxStaticBoxSizer* s_round_hle_log_lvl = new wxStaticBoxSizer(wxVERTICAL, p_misc, _("Log Level"));

	// Networking
	wxStaticBoxSizer* s_round_net_status  = new wxStaticBoxSizer(wxVERTICAL, p_networking, _("Connection status"));
	wxStaticBoxSizer* s_round_net_interface = new wxStaticBoxSizer(wxVERTICAL, p_networking, _("Network adapter"));

	// System
	wxStaticBoxSizer* s_round_sys_lang = new wxStaticBoxSizer(wxVERTICAL, p_system, _("Language"));

	wxComboBox* cbox_cpu_decoder      = new wxComboBox(p_core, wxID_ANY);
	wxComboBox* cbox_spu_decoder      = new wxComboBox(p_core, wxID_ANY);
	wxComboBox* cbox_gs_render        = new wxComboBox(p_graphics, wxID_ANY);
	wxComboBox* cbox_gs_d3d_adaptater = new wxComboBox(p_graphics, wxID_ANY);
	wxComboBox* cbox_gs_resolution    = new wxComboBox(p_graphics, wxID_ANY);
	wxComboBox* cbox_gs_aspect        = new wxComboBox(p_graphics, wxID_ANY);
	wxComboBox* cbox_gs_frame_limit   = new wxComboBox(p_graphics, wxID_ANY);
	wxComboBox* cbox_pad_handler      = new wxComboBox(p_io, wxID_ANY);
	wxComboBox* cbox_keyboard_handler = new wxComboBox(p_io, wxID_ANY);
	wxComboBox* cbox_mouse_handler    = new wxComboBox(p_io, wxID_ANY);
	wxComboBox* cbox_camera           = new wxComboBox(p_io, wxID_ANY);
	wxComboBox* cbox_camera_type      = new wxComboBox(p_io, wxID_ANY);
	wxComboBox* cbox_audio_out        = new wxComboBox(p_audio, wxID_ANY);
	wxComboBox* cbox_hle_loglvl       = new wxComboBox(p_misc, wxID_ANY);
	wxComboBox* cbox_net_status       = new wxComboBox(p_networking, wxID_ANY);
	wxComboBox* cbox_net_interface    = new wxComboBox(p_networking, wxID_ANY);
	wxComboBox* cbox_sys_lang         = new wxComboBox(p_system, wxID_ANY);

	wxCheckBox* chbox_core_llvm_exclud    = new wxCheckBox(p_core, wxID_ANY, "Enable exclusion of compiled blocks");
	wxCheckBox* chbox_core_hook_stfunc    = new wxCheckBox(p_core, wxID_ANY, "Hook static functions");
	wxCheckBox* chbox_core_load_liblv2    = new wxCheckBox(p_core, wxID_ANY, "Load liblv2.sprx");
	wxCheckBox* chbox_gs_log_prog         = new wxCheckBox(p_graphics, wxID_ANY, "Log vertex/fragment programs");
	wxCheckBox* chbox_gs_dump_depth       = new wxCheckBox(p_graphics, wxID_ANY, "Write Depth Buffer");
	wxCheckBox* chbox_gs_dump_color       = new wxCheckBox(p_graphics, wxID_ANY, "Write Color Buffers");
	wxCheckBox* chbox_gs_read_color       = new wxCheckBox(p_graphics, wxID_ANY, "Read Color Buffer");
	wxCheckBox* chbox_gs_vsync            = new wxCheckBox(p_graphics, wxID_ANY, "VSync");
	wxCheckBox* chbox_gs_debug_output     = new wxCheckBox(p_graphics, wxID_ANY, "Debug Output");
	wxCheckBox* chbox_gs_3dmonitor        = new wxCheckBox(p_graphics, wxID_ANY, "3D Monitor");
	wxCheckBox* chbox_audio_dump          = new wxCheckBox(p_audio, wxID_ANY, "Dump to file");
	wxCheckBox* chbox_audio_conv          = new wxCheckBox(p_audio, wxID_ANY, "Convert to 16 bit");
	wxCheckBox* chbox_hle_logging         = new wxCheckBox(p_misc, wxID_ANY, "Log everything");
	wxCheckBox* chbox_rsx_logging         = new wxCheckBox(p_misc, wxID_ANY, "RSX Logging");
	wxCheckBox* chbox_hle_savetty         = new wxCheckBox(p_misc, wxID_ANY, "Save TTY output to file");
	wxCheckBox* chbox_hle_exitonstop      = new wxCheckBox(p_misc, wxID_ANY, "Exit RPCS3 when process finishes");
	wxCheckBox* chbox_hle_always_start    = new wxCheckBox(p_misc, wxID_ANY, "Always start after boot");

	wxTextCtrl* txt_dbg_range_min   = new wxTextCtrl(p_core, wxID_ANY);
	wxTextCtrl* txt_dbg_range_max = new wxTextCtrl(p_core, wxID_ANY);
	wxTextCtrl* txt_llvm_threshold = new wxTextCtrl(p_core, wxID_ANY);

	//Auto Pause
	wxCheckBox* chbox_dbg_ap_systemcall   = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at System Call");
	wxCheckBox* chbox_dbg_ap_functioncall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at Function Call");

	//Custom EmulationDir
	wxCheckBox* chbox_emulationdir_enable = new wxCheckBox(p_system, wxID_ANY, "Use Path Below as EmulationDir ? (Need Restart)");
	wxTextCtrl* txt_emulationdir_path     = new wxTextCtrl(p_system, wxID_ANY, Emu.GetEmulatorPath());

	cbox_cpu_decoder->Append("PPU Interpreter");
	cbox_cpu_decoder->Append("PPU Interpreter 2");
	cbox_cpu_decoder->Append("PPU JIT (LLVM)");

	cbox_spu_decoder->Append("SPU Interpreter");
	cbox_spu_decoder->Append("SPU Interpreter 2");
	cbox_spu_decoder->Append("SPU JIT (ASMJIT)");

	cbox_gs_render->Append("Null");
	cbox_gs_render->Append("OpenGL");
#if defined(DX12_SUPPORT)
	cbox_gs_render->Append("DirectX 12");
#endif

	cbox_gs_d3d_adaptater->Append("WARP");
	cbox_gs_d3d_adaptater->Append("default");
	cbox_gs_d3d_adaptater->Append("renderer 0");
	cbox_gs_d3d_adaptater->Append("renderer 1");
	cbox_gs_d3d_adaptater->Append("renderer 2");

	for(int i = 1; i < WXSIZEOF(ResolutionTable); ++i)
	{
		cbox_gs_resolution->Append(wxString::Format("%dx%d", ResolutionTable[i].width.value(), ResolutionTable[i].height.value()));
	}

	cbox_gs_aspect->Append("4:3");
	cbox_gs_aspect->Append("16:9");

	for (auto item : { "Off", "50", "59.94", "30", "60", "Auto" })
		cbox_gs_frame_limit->Append(item);

	cbox_pad_handler->Append("Null");
	cbox_pad_handler->Append("Windows");
#if defined (_WIN32)
	cbox_pad_handler->Append("XInput");
#endif
	//cbox_pad_handler->Append("DirectInput");

	cbox_keyboard_handler->Append("Null");
	cbox_keyboard_handler->Append("Windows");
	//cbox_keyboard_handler->Append("DirectInput");

	cbox_mouse_handler->Append("Null");
	cbox_mouse_handler->Append("Windows");
	//cbox_mouse_handler->Append("DirectInput");

	cbox_audio_out->Append("Null");
	cbox_audio_out->Append("OpenAL");
#if defined (_WIN32)
	cbox_audio_out->Append("XAudio2");
#endif

	cbox_camera->Append("Null");
	cbox_camera->Append("Connected");

	cbox_camera_type->Append("Unknown");
	cbox_camera_type->Append("EyeToy");
	cbox_camera_type->Append("PlayStation Eye");
	cbox_camera_type->Append("USB Video Class 1.1");

	cbox_hle_loglvl->Append("All");
	cbox_hle_loglvl->Append("Warnings");
	cbox_hle_loglvl->Append("Success");
	cbox_hle_loglvl->Append("Errors");
	cbox_hle_loglvl->Append("Nothing");

	cbox_net_status->Append("IP Obtained");
	cbox_net_status->Append("Obtaining IP");
	cbox_net_status->Append("Connecting");
	cbox_net_status->Append("Disconnected");

#ifdef _WIN32
	PIP_ADAPTER_INFO pAdapterInfo;
	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
	ULONG buflen = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(buflen);
	}

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR)
	{
		PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			std::string adapterName = fmt::Format("%s", pAdapter->Description);
			cbox_net_interface->Append(adapterName);
			pAdapter = pAdapter->Next;
		}
	}
	else
	{
		LOG_ERROR(HLE, "Call to GetAdaptersInfo failed.");
	}
#else
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		LOG_ERROR(HLE, "Call to getifaddrs returned negative.");
	}

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		if (ifa->ifa_addr == NULL)
		{
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET || family == AF_INET6)
		{
			std::string adapterName = fmt::Format("%s", ifa->ifa_name);
			cbox_net_interface->Append(adapterName);
		}
	}

	freeifaddrs(ifaddr);
#endif

	cbox_sys_lang->Append("Japanese");
	cbox_sys_lang->Append("English (US)");
	cbox_sys_lang->Append("French");
	cbox_sys_lang->Append("Spanish");
	cbox_sys_lang->Append("German");
	cbox_sys_lang->Append("Italian");
	cbox_sys_lang->Append("Dutch");
	cbox_sys_lang->Append("Portuguese (PT)");
	cbox_sys_lang->Append("Russian");
	cbox_sys_lang->Append("Korean");
	cbox_sys_lang->Append("Chinese (Trad.)");
	cbox_sys_lang->Append("Chinese (Simp.)");
	cbox_sys_lang->Append("Finnish");
	cbox_sys_lang->Append("Swedish");
	cbox_sys_lang->Append("Danish");
	cbox_sys_lang->Append("Norwegian");
	cbox_sys_lang->Append("Polish");
	cbox_sys_lang->Append("English (UK)");

	// Get values from .ini
	chbox_core_llvm_exclud   ->SetValue(Ini.LLVMExclusionRange.GetValue());
	chbox_gs_log_prog        ->SetValue(Ini.GSLogPrograms.GetValue());
	chbox_gs_dump_depth      ->SetValue(Ini.GSDumpDepthBuffer.GetValue());
	chbox_gs_dump_color      ->SetValue(Ini.GSDumpColorBuffers.GetValue());
	chbox_gs_read_color      ->SetValue(Ini.GSReadColorBuffer.GetValue());
	chbox_gs_vsync           ->SetValue(Ini.GSVSyncEnable.GetValue());
	chbox_gs_debug_output    ->SetValue(Ini.GSDebugOutputEnable.GetValue());
	chbox_gs_3dmonitor       ->SetValue(Ini.GS3DTV.GetValue());
	chbox_audio_dump         ->SetValue(Ini.AudioDumpToFile.GetValue());
	chbox_audio_conv         ->SetValue(Ini.AudioConvertToU16.GetValue());
	chbox_hle_logging        ->SetValue(Ini.HLELogging.GetValue());
	chbox_rsx_logging        ->SetValue(Ini.RSXLogging.GetValue());
	chbox_hle_savetty        ->SetValue(Ini.HLESaveTTY.GetValue());
	chbox_hle_exitonstop     ->SetValue(Ini.HLEExitOnStop.GetValue());
	chbox_hle_always_start   ->SetValue(Ini.HLEAlwaysStart.GetValue());
	chbox_core_hook_stfunc   ->SetValue(Ini.HookStFunc.GetValue());
	chbox_core_load_liblv2   ->SetValue(Ini.LoadLibLv2.GetValue());

	//Auto Pause related
	chbox_dbg_ap_systemcall  ->SetValue(Ini.DBGAutoPauseSystemCall.GetValue());
	chbox_dbg_ap_functioncall->SetValue(Ini.DBGAutoPauseFunctionCall.GetValue());

	//Custom EmulationDir
	chbox_emulationdir_enable->SetValue(Ini.SysEmulationDirPathEnable.GetValue());
	txt_emulationdir_path    ->SetValue(Ini.SysEmulationDirPath.GetValue());

	cbox_cpu_decoder     ->SetSelection(Ini.CPUDecoderMode.GetValue() ? Ini.CPUDecoderMode.GetValue() : 0);
	txt_dbg_range_min    ->SetValue(std::to_string(Ini.LLVMMinId.GetValue()));
	txt_dbg_range_max    ->SetValue(std::to_string(Ini.LLVMMaxId.GetValue()));
	txt_llvm_threshold   ->SetValue(std::to_string(Ini.LLVMThreshold.GetValue()));
	cbox_spu_decoder     ->SetSelection(Ini.SPUDecoderMode.GetValue() ? Ini.SPUDecoderMode.GetValue() : 0);
	cbox_gs_render       ->SetSelection(Ini.GSRenderMode.GetValue());
	cbox_gs_d3d_adaptater->SetSelection(Ini.GSD3DAdaptater.GetValue());
	cbox_gs_resolution   ->SetSelection(ResolutionIdToNum(Ini.GSResolution.GetValue()) - 1);
	cbox_gs_aspect       ->SetSelection(Ini.GSAspectRatio.GetValue() - 1);
	cbox_gs_frame_limit  ->SetSelection(Ini.GSFrameLimit.GetValue());
	cbox_pad_handler     ->SetSelection(Ini.PadHandlerMode.GetValue());
	cbox_keyboard_handler->SetSelection(Ini.KeyboardHandlerMode.GetValue());
	cbox_mouse_handler   ->SetSelection(Ini.MouseHandlerMode.GetValue());
	cbox_audio_out       ->SetSelection(Ini.AudioOutMode.GetValue());
	cbox_camera          ->SetSelection(Ini.Camera.GetValue());
	cbox_camera_type     ->SetSelection(Ini.CameraType.GetValue());
	cbox_hle_loglvl      ->SetSelection(Ini.HLELogLvl.GetValue());
	cbox_net_status      ->SetSelection(Ini.NETStatus.GetValue());
	cbox_net_interface   ->SetSelection(Ini.NETInterface.GetValue());
	cbox_sys_lang        ->SetSelection(Ini.SysLanguage.GetValue());
	
	// Core
	s_round_cpu_decoder->Add(cbox_cpu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_spu_decoder->Add(cbox_spu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm->Add(chbox_core_llvm_exclud, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_range->Add(txt_dbg_range_min, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_range->Add(txt_dbg_range_max, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm->Add(s_round_llvm_range, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_threshold->Add(txt_llvm_threshold, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm->Add(s_round_llvm_threshold, wxSizerFlags().Border(wxALL, 5).Expand());

	// Rendering
	s_round_gs_render->Add(cbox_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_d3d_adaptater->Add(cbox_gs_d3d_adaptater, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_res->Add(cbox_gs_resolution, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_aspect->Add(cbox_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_frame_limit->Add(cbox_gs_frame_limit, wxSizerFlags().Border(wxALL, 5).Expand());

	// Input/Output
	s_round_io_pad_handler->Add(cbox_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_keyboard_handler->Add(cbox_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_mouse_handler->Add(cbox_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_camera->Add(cbox_camera, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_camera_type->Add(cbox_camera_type, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_audio_out->Add(cbox_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_hle_log_lvl->Add(cbox_hle_loglvl, wxSizerFlags().Border(wxALL, 5).Expand());

	// Networking
	s_round_net_status->Add(cbox_net_status, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_net_interface->Add(cbox_net_interface, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_sys_lang->Add(cbox_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

	// Core
	s_subpanel_core->Add(s_round_cpu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core->Add(s_round_spu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core->Add(s_round_llvm, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core->Add(chbox_core_hook_stfunc, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core->Add(chbox_core_load_liblv2, wxSizerFlags().Border(wxALL, 5).Expand());

	// Graphics
	s_subpanel_graphics->Add(s_round_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(s_round_gs_d3d_adaptater, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(s_round_gs_res, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(s_round_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(s_round_gs_frame_limit, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_log_prog, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_dump_depth, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_dump_color, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_read_color, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_vsync, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_debug_output, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_3dmonitor, wxSizerFlags().Border(wxALL, 5).Expand());

	// Input - Output
	s_subpanel_io->Add(s_round_io_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io->Add(s_round_io_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io->Add(s_round_io_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io->Add(s_round_io_camera, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io->Add(s_round_io_camera_type, wxSizerFlags().Border(wxALL, 5).Expand());

	// Audio
	s_subpanel_audio->Add(s_round_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_audio->Add(chbox_audio_dump, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_audio->Add(chbox_audio_conv, wxSizerFlags().Border(wxALL, 5).Expand());

	// Miscellaneous
	s_subpanel_misc->Add(s_round_hle_log_lvl, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_logging, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_rsx_logging, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_savetty, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_exitonstop, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_always_start, wxSizerFlags().Border(wxALL, 5).Expand());

	// Auto Pause
	s_subpanel_misc->Add(chbox_dbg_ap_systemcall, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_dbg_ap_functioncall, wxSizerFlags().Border(wxALL, 5).Expand());

	// Networking
	s_subpanel_networking->Add(s_round_net_status, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_networking->Add(s_round_net_interface, wxSizerFlags().Border(wxALL, 5).Expand());

	// System
	s_subpanel_system->Add(s_round_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

	// Custom EmulationDir
	s_subpanel_system->Add(chbox_emulationdir_enable, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_system->Add(txt_emulationdir_path, wxSizerFlags().Border(wxALL, 5).Expand());
	
	// Buttons
	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));
	s_b_panel->Add(new wxButton(&diag, wxID_OK), wxSizerFlags().Border(wxALL, 5).Bottom());
	s_b_panel->Add(new wxButton(&diag, wxID_CANCEL), wxSizerFlags().Border(wxALL, 5).Bottom());

	// Resize panels 
	diag.SetSizerAndFit(s_subpanel_core, false);
	diag.SetSizerAndFit(s_subpanel_graphics, false);
	diag.SetSizerAndFit(s_subpanel_io, false);
	diag.SetSizerAndFit(s_subpanel_audio, false);
	diag.SetSizerAndFit(s_subpanel_misc, false);
	diag.SetSizerAndFit(s_subpanel_networking, false);
	diag.SetSizerAndFit(s_subpanel_system, false);
	diag.SetSizerAndFit(s_b_panel, false);
	
	diag.SetSize(width + 26, height + 80);

	if(diag.ShowModal() == wxID_OK)
	{
		Ini.CPUDecoderMode.SetValue(cbox_cpu_decoder->GetSelection());
		long minllvmid, maxllvmid;
		txt_dbg_range_min->GetValue().ToLong(&minllvmid);
		txt_dbg_range_max->GetValue().ToLong(&maxllvmid);
		Ini.LLVMExclusionRange.SetValue(chbox_core_llvm_exclud->GetValue());
		Ini.LLVMMinId.SetValue(minllvmid);
		Ini.LLVMMaxId.SetValue(maxllvmid);
		long llvmthreshold;
		txt_llvm_threshold->GetValue().ToLong(&llvmthreshold);
		Ini.LLVMThreshold.SetValue(llvmthreshold);
		Ini.SPUDecoderMode.SetValue(cbox_spu_decoder->GetSelection());
		Ini.HookStFunc.SetValue(chbox_core_hook_stfunc->GetValue());
		Ini.LoadLibLv2.SetValue(chbox_core_load_liblv2->GetValue());
		Ini.GSRenderMode.SetValue(cbox_gs_render->GetSelection());
		Ini.GSD3DAdaptater.SetValue(cbox_gs_d3d_adaptater->GetSelection());
		Ini.GSResolution.SetValue(ResolutionNumToId(cbox_gs_resolution->GetSelection() + 1));
		Ini.GSAspectRatio.SetValue(cbox_gs_aspect->GetSelection() + 1);
		Ini.GSFrameLimit.SetValue(cbox_gs_frame_limit->GetSelection());
		Ini.GSLogPrograms.SetValue(chbox_gs_log_prog->GetValue());
		Ini.GSDumpDepthBuffer.SetValue(chbox_gs_dump_depth->GetValue());
		Ini.GSDumpColorBuffers.SetValue(chbox_gs_dump_color->GetValue());
		Ini.GSReadColorBuffer.SetValue(chbox_gs_read_color->GetValue());
		Ini.GSVSyncEnable.SetValue(chbox_gs_vsync->GetValue());
		Ini.GSDebugOutputEnable.SetValue(chbox_gs_debug_output->GetValue());
		Ini.GS3DTV.SetValue(chbox_gs_3dmonitor->GetValue());
		Ini.PadHandlerMode.SetValue(cbox_pad_handler->GetSelection());
		Ini.KeyboardHandlerMode.SetValue(cbox_keyboard_handler->GetSelection());
		Ini.MouseHandlerMode.SetValue(cbox_mouse_handler->GetSelection());
		Ini.AudioOutMode.SetValue(cbox_audio_out->GetSelection());
		Ini.AudioDumpToFile.SetValue(chbox_audio_dump->GetValue());
		Ini.AudioConvertToU16.SetValue(chbox_audio_conv->GetValue());
		Ini.Camera.SetValue(cbox_camera->GetSelection());
		Ini.CameraType.SetValue(cbox_camera_type->GetSelection());
		Ini.HLELogging.SetValue(chbox_hle_logging->GetValue());
		Ini.RSXLogging.SetValue(chbox_rsx_logging->GetValue());
		Ini.HLESaveTTY.SetValue(chbox_hle_savetty->GetValue());
		Ini.HLEExitOnStop.SetValue(chbox_hle_exitonstop->GetValue());
		Ini.HLELogLvl.SetValue(cbox_hle_loglvl->GetSelection());
		Ini.NETStatus.SetValue(cbox_net_status->GetSelection());
		Ini.NETInterface.SetValue(cbox_net_interface->GetSelection());
		Ini.SysLanguage.SetValue(cbox_sys_lang->GetSelection());
		Ini.HLEAlwaysStart.SetValue(chbox_hle_always_start->GetValue());

		//Auto Pause
		Ini.DBGAutoPauseFunctionCall.SetValue(chbox_dbg_ap_functioncall->GetValue());
		Ini.DBGAutoPauseSystemCall.SetValue(chbox_dbg_ap_systemcall->GetValue());

		//Custom EmulationDir
		Ini.SysEmulationDirPathEnable.SetValue(chbox_emulationdir_enable->GetValue());
		Ini.SysEmulationDirPath.SetValue(txt_emulationdir_path->GetValue().ToStdString());

		Ini.Save();
	}

	if(paused) Emu.Resume();
}

void MainFrame::ConfigPad(wxCommandEvent& WXUNUSED(event))
{
	PADManager(this).ShowModal();
}

void MainFrame::ConfigVFS(wxCommandEvent& WXUNUSED(event))
{
	VFSManagerDialog(this).ShowModal();
}

void MainFrame::ConfigVHDD(wxCommandEvent& WXUNUSED(event))
{
	VHDDManagerDialog(this).ShowModal();
}

void MainFrame::ConfigAutoPause(wxCommandEvent& WXUNUSED(event))
{
	AutoPauseManagerDialog(this).ShowModal();
}

void MainFrame::ConfigSaveData(wxCommandEvent& event)
{
	SaveDataListDialog(this, true).ShowModal();
}

void MainFrame::ConfigLLEModules(wxCommandEvent& event)
{
	(new LLEModulesManagerFrame(this))->Show();
}

void MainFrame::OpenELFCompiler(wxCommandEvent& WXUNUSED(event))
{
	(new CompilerELF(this)) -> Show();
}

void MainFrame::OpenKernelExplorer(wxCommandEvent& WXUNUSED(event))
{
	(new KernelExplorer(this)) -> Show();
}

void MainFrame::OpenMemoryViewer(wxCommandEvent& WXUNUSED(event))
{
	(new MemoryViewerPanel(this)) -> Show();
}

void MainFrame::OpenRSXDebugger(wxCommandEvent& WXUNUSED(event))
{
	(new RSXDebugger(this)) -> Show();
}

void MainFrame::OpenStringSearch(wxCommandEvent& WXUNUSED(event))
{
	(new MemoryStringSearcher(this)) -> Show();
}

void MainFrame::OpenCgDisasm(wxCommandEvent& WXUNUSED(event))
{
	(new CgDisasm(this))->Show();
}

void MainFrame::AboutDialogHandler(wxCommandEvent& WXUNUSED(event))
{
	AboutDialog(this).ShowModal();
}

void MainFrame::UpdateUI(wxCommandEvent& event)
{
	event.Skip();

	bool is_running, is_stopped, is_ready;

	if(event.GetEventType() == wxEVT_DBG_COMMAND)
	{
		switch(event.GetId())
		{
			case DID_START_EMU:
			case DID_STARTED_EMU:
				is_running = true;
				is_stopped = false;
				is_ready = false;
			break;

			case DID_STOP_EMU:
			case DID_STOPPED_EMU:
				is_running = false;
				is_stopped = true;
				is_ready = false;
				m_sys_menu_opened = false;
			break;

			case DID_PAUSE_EMU:
			case DID_PAUSED_EMU:
				is_running = false;
				is_stopped = false;
				is_ready = false;
			break;

			case DID_RESUME_EMU:
			case DID_RESUMED_EMU:
				is_running = true;
				is_stopped = false;
				is_ready = false;
			break;

			case DID_READY_EMU:
				is_running = false;
				is_stopped = false;
				is_ready = true;
			break;

			case DID_REGISTRED_CALLBACK:
				is_running = Emu.IsRunning();
				is_stopped = Emu.IsStopped();
				is_ready = Emu.IsReady();
			break;

			default:
				return;
		}

		if (event.GetId() == DID_STOPPED_EMU)
		{
			if (Ini.HLEExitOnStop.GetValue())
			{
				wxGetApp().Exit();
			}
		}
	}
	else
	{
		is_running = Emu.IsRunning();
		is_stopped = Emu.IsStopped();
		is_ready = Emu.IsReady();
	}

	// Update menu items based on the state of the emulator
	wxMenuBar& menubar( *GetMenuBar() );

	// Emulation
	wxMenuItem& pause = *menubar.FindItem(id_sys_pause);
	wxMenuItem& stop  = *menubar.FindItem(id_sys_stop);
	pause.SetItemLabel(is_running ? "&Pause\tCtrl + P" : is_ready ? "&Start\tCtrl + E" : "&Resume\tCtrl + E");
	pause.Enable(!is_stopped);
	stop.Enable(!is_stopped);

	// PS3 Commands
	wxMenuItem& send_exit = *menubar.FindItem(id_sys_send_exit);
	wxMenuItem& send_open_menu = *menubar.FindItem(id_sys_send_open_menu);
	bool enable_commands = !is_stopped;
	send_open_menu.SetItemLabel(wxString::Format("Send &%s system menu cmd", (m_sys_menu_opened ? "close" : "open")));
	send_open_menu.Enable(enable_commands);
	send_exit.Enable(enable_commands);

	// Tools
	wxMenuItem& kernel_explorer = *menubar.FindItem(id_tools_kernel_explorer);
	wxMenuItem& memory_viewer = *menubar.FindItem(id_tools_memory_viewer);
	wxMenuItem& rsx_debugger = *menubar.FindItem(id_tools_rsx_debugger);
	wxMenuItem& string_search = *menubar.FindItem(id_tools_string_search);
	kernel_explorer.Enable(!is_stopped);
	memory_viewer.Enable(!is_stopped);
	rsx_debugger.Enable(!is_stopped);
	string_search.Enable(!is_stopped);
	


	//m_aui_mgr.Update();

	//wxCommandEvent refit( wxEVT_COMMAND_MENU_SELECTED, id_update_dbg );
	//GetEventHandler()->AddPendingEvent( refit );
}

void MainFrame::OnQuit(wxCloseEvent& event)
{
	DoSettings(false);
	TheApp->Exit();
}

void MainFrame::OnKeyDown(wxKeyEvent& event)
{
	if(wxGetActiveWindow() /*== this*/ && event.ControlDown())
	{
		switch(event.GetKeyCode())
		{
		case 'E': case 'e': if(Emu.IsPaused()) Emu.Resume(); else if(Emu.IsReady()) Emu.Run(); return;
		case 'P': case 'p': if(Emu.IsRunning()) Emu.Pause(); return;
		case 'S': case 's': if(!Emu.IsStopped()) Emu.Stop(); return;
		case 'R': case 'r': if(!Emu.m_path.empty()) {Emu.Stop(); Emu.Run();} return;
		}
	}

	event.Skip();
}
