#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "rpcs3.h"
#include "MainFrame.h"

#include "git-version.h"
#include "Ini.h"
#include "Emu/SysCalls/Modules/cellSysutil.h"
#include "Emu/RSX/sysutil_video.h"
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
#include "Gui/LLEModulesManager.h"

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
	id_help_about,
	id_update_dbg,
};

wxString GetPaneName()
{
	static int pane_num = 0;

	return wxString::Format("Pane_%d", pane_num++);
}

MainFrame::MainFrame()
	: FrameBase(nullptr, wxID_ANY, "", "MainFrame", wxSize(800, 600))
	, m_aui_mgr(this)
	, m_sys_menu_opened(false)
{

#ifdef _DEBUG
	SetLabel(wxString::Format(_PRGNAME_ " git-" RPCS3_GIT_VERSION));
#else
	SetLabel(wxString::Format(_PRGNAME_ " " _PRGVER_));
#endif

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

	wxMenu* menu_help = new wxMenu();
	menubar->Append(menu_help, "&Help");
	menu_help->Append(id_help_about, "&About...");

	SetMenuBar(menubar);

	// Panels
	m_game_viewer = new GameViewer(this);
	m_debugger_frame = new DebuggerPanel(this);
	m_log_frame = new LogFrame(this);

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
		LOG_ERROR(HLE, "PS3 executable not found in selected folder (%s)", ctrl.GetPath().wx_str());
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

	wxFileDialog ctrl (this, L"Select PKG", wxEmptyString, wxEmptyString, "PKG files (*.pkg)|*.pkg|All files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stopped) Emu.Resume();
		return;
	}

	Emu.Stop();
	
	// Open and install PKG file
	std::string filePath = ctrl.GetPath().ToStdString();
	rFile pkg_f(filePath, rFile::read); // TODO: Use VFS to install PKG files

	if (pkg_f.IsOpened())
	{
		PKGLoader pkg(pkg_f);
		pkg.Install("/dev_hdd0/game/");
		pkg.Close();

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
	static const u32 width = 425;
	static const u32 height = 400;

	// Settings panels
	wxNotebook* nb_config = new wxNotebook(&diag, wxID_ANY, wxPoint(6,6), wxSize(width, height));
	wxPanel* p_system     = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_cpu        = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_graphics   = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_audio      = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_camera     = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_io         = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_hle        = new wxPanel(nb_config, wxID_ANY);

	nb_config->AddPage(p_cpu,      wxT("Core"));
	nb_config->AddPage(p_graphics, wxT("Graphics"));
	nb_config->AddPage(p_audio,    wxT("Audio"));
	nb_config->AddPage(p_camera,   wxT("Camera"));
	nb_config->AddPage(p_io,       wxT("Input / Output"));
	nb_config->AddPage(p_hle,      wxT("HLE / Misc."));
	nb_config->AddPage(p_system,   wxT("System"));

	wxBoxSizer* s_subpanel_system   = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_cpu      = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_graphics = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_audio    = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_camera   = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_io       = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_hle      = new wxBoxSizer(wxVERTICAL);

	// CPU/SPU settings
	wxStaticBoxSizer* s_round_cpu_decoder = new wxStaticBoxSizer(wxVERTICAL, p_cpu, _("CPU"));
	wxStaticBoxSizer* s_round_spu_decoder = new wxStaticBoxSizer(wxVERTICAL, p_cpu, _("SPU"));

	// Graphics
	wxStaticBoxSizer* s_round_gs_render = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Render"));
	wxStaticBoxSizer* s_round_gs_res    = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Default resolution"));
	wxStaticBoxSizer* s_round_gs_aspect = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Default aspect ratio"));

	// Input / Output
	wxStaticBoxSizer* s_round_io_pad_handler      = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Pad Handler"));
	wxStaticBoxSizer* s_round_io_keyboard_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Keyboard Handler"));
	wxStaticBoxSizer* s_round_io_mouse_handler    = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Mouse Handler"));
	
	// Audio
	wxStaticBoxSizer* s_round_audio_out = new wxStaticBoxSizer(wxVERTICAL, p_audio, _("Audio Out"));

	// Camera
	wxStaticBoxSizer* s_round_camera      = new wxStaticBoxSizer(wxVERTICAL, p_camera, _("Camera"));
	wxStaticBoxSizer* s_round_camera_type = new wxStaticBoxSizer(wxVERTICAL, p_camera, _("Camera type"));

	// HLE / Misc.
	wxStaticBoxSizer* s_round_hle_log_lvl = new wxStaticBoxSizer(wxVERTICAL, p_hle, _("Log Level"));

	// System
	wxStaticBoxSizer* s_round_sys_lang = new wxStaticBoxSizer(wxVERTICAL, p_system, _("Language"));

	wxComboBox* cbox_cpu_decoder      = new wxComboBox(p_cpu, wxID_ANY);
	wxComboBox* cbox_spu_decoder      = new wxComboBox(p_cpu, wxID_ANY);
	wxComboBox* cbox_gs_render        = new wxComboBox(p_graphics, wxID_ANY);
	wxComboBox* cbox_gs_resolution    = new wxComboBox(p_graphics, wxID_ANY);
	wxComboBox* cbox_gs_aspect        = new wxComboBox(p_graphics, wxID_ANY);
	wxComboBox* cbox_pad_handler      = new wxComboBox(p_io, wxID_ANY);
	wxComboBox* cbox_keyboard_handler = new wxComboBox(p_io, wxID_ANY);
	wxComboBox* cbox_mouse_handler    = new wxComboBox(p_io, wxID_ANY);
	wxComboBox* cbox_audio_out        = new wxComboBox(p_audio, wxID_ANY);
	wxComboBox* cbox_camera           = new wxComboBox(p_camera, wxID_ANY);
	wxComboBox* cbox_camera_type      = new wxComboBox(p_camera, wxID_ANY);
	wxComboBox* cbox_hle_loglvl       = new wxComboBox(p_hle, wxID_ANY);
	wxComboBox* cbox_sys_lang         = new wxComboBox(p_system, wxID_ANY);

	wxCheckBox* chbox_gs_log_prog         = new wxCheckBox(p_graphics, wxID_ANY, "Log vertex/fragment programs");
	wxCheckBox* chbox_gs_dump_depth       = new wxCheckBox(p_graphics, wxID_ANY, "Write Depth Buffer");
	wxCheckBox* chbox_gs_dump_color       = new wxCheckBox(p_graphics, wxID_ANY, "Write Color Buffers");
	wxCheckBox* chbox_gs_vsync            = new wxCheckBox(p_graphics, wxID_ANY, "VSync");
	wxCheckBox* chbox_gs_3dmonitor        = new wxCheckBox(p_graphics, wxID_ANY, "3D Monitor");
	wxCheckBox* chbox_audio_dump          = new wxCheckBox(p_audio, wxID_ANY, "Dump to file");
	wxCheckBox* chbox_audio_conv          = new wxCheckBox(p_audio, wxID_ANY, "Convert to 16 bit");
	wxCheckBox* chbox_hle_logging         = new wxCheckBox(p_hle, wxID_ANY, "Log all SysCalls");
	wxCheckBox* chbox_rsx_logging         = new wxCheckBox(p_hle, wxID_ANY, "RSX Logging");
	wxCheckBox* chbox_hle_hook_stfunc     = new wxCheckBox(p_hle, wxID_ANY, "Hook static functions");
	wxCheckBox* chbox_hle_savetty         = new wxCheckBox(p_hle, wxID_ANY, "Save TTY output to file");
	wxCheckBox* chbox_hle_exitonstop      = new wxCheckBox(p_hle, wxID_ANY, "Exit RPCS3 when process finishes");
	wxCheckBox* chbox_hle_always_start    = new wxCheckBox(p_hle, wxID_ANY, "Always start after boot");

	//Auto Pause
	wxCheckBox* chbox_dbg_ap_systemcall   = new wxCheckBox(p_hle, wxID_ANY, "Auto Pause at System Call");
	wxCheckBox* chbox_dbg_ap_functioncall = new wxCheckBox(p_hle, wxID_ANY, "Auto Pause at Function Call");

	cbox_cpu_decoder->Append("PPU Interpreter");
	cbox_cpu_decoder->Append("PPU JIT (LLVM)");

	cbox_spu_decoder->Append("SPU Interpreter");
	cbox_spu_decoder->Append("SPU JIT (ASMJIT)");

	cbox_gs_render->Append("Null");
	cbox_gs_render->Append("OpenGL");
	//cbox_gs_render->Append("Software");

	for(int i = 1; i < WXSIZEOF(ResolutionTable); ++i)
	{
		cbox_gs_resolution->Append(wxString::Format("%dx%d", ResolutionTable[i].width.ToLE(), ResolutionTable[i].height.ToLE()));
	}

	cbox_gs_aspect->Append("4:3");
	cbox_gs_aspect->Append("16:9");

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

	cbox_camera->Append("Null");

	cbox_camera_type->Append("Unknown");
	cbox_camera_type->Append("EyeToy");
	cbox_camera_type->Append("PlayStation Eye");
	cbox_camera_type->Append("USB Video Class 1.1");

	cbox_hle_loglvl->Append("All");
	cbox_hle_loglvl->Append("Success");
	cbox_hle_loglvl->Append("Warnings");
	cbox_hle_loglvl->Append("Errors");
	cbox_hle_loglvl->Append("Nothing");

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
	chbox_gs_log_prog        ->SetValue(Ini.GSLogPrograms.GetValue());
	chbox_gs_dump_depth      ->SetValue(Ini.GSDumpDepthBuffer.GetValue());
	chbox_gs_dump_color      ->SetValue(Ini.GSDumpColorBuffers.GetValue());
	chbox_gs_vsync           ->SetValue(Ini.GSVSyncEnable.GetValue());
	chbox_gs_3dmonitor       ->SetValue(Ini.GS3DTV.GetValue());
	chbox_audio_dump         ->SetValue(Ini.AudioDumpToFile.GetValue());
	chbox_audio_conv         ->SetValue(Ini.AudioConvertToU16.GetValue());
	chbox_hle_logging        ->SetValue(Ini.HLELogging.GetValue());
	chbox_rsx_logging        ->SetValue(Ini.RSXLogging.GetValue());
	chbox_hle_hook_stfunc    ->SetValue(Ini.HLEHookStFunc.GetValue());
	chbox_hle_savetty        ->SetValue(Ini.HLESaveTTY.GetValue());
	chbox_hle_exitonstop     ->SetValue(Ini.HLEExitOnStop.GetValue());
	chbox_hle_always_start   ->SetValue(Ini.HLEAlwaysStart.GetValue());

	//Auto Pause related
	chbox_dbg_ap_systemcall  ->SetValue(Ini.DBGAutoPauseSystemCall.GetValue());
	chbox_dbg_ap_functioncall->SetValue(Ini.DBGAutoPauseFunctionCall.GetValue());

	cbox_cpu_decoder     ->SetSelection(Ini.CPUDecoderMode.GetValue() ? Ini.CPUDecoderMode.GetValue() - 1 : 0);
	cbox_spu_decoder     ->SetSelection(Ini.SPUDecoderMode.GetValue() ? Ini.SPUDecoderMode.GetValue() - 1 : 0);
	cbox_gs_render       ->SetSelection(Ini.GSRenderMode.GetValue());
	cbox_gs_resolution   ->SetSelection(ResolutionIdToNum(Ini.GSResolution.GetValue()) - 1);
	cbox_gs_aspect       ->SetSelection(Ini.GSAspectRatio.GetValue() - 1);
	cbox_pad_handler     ->SetSelection(Ini.PadHandlerMode.GetValue());
	cbox_keyboard_handler->SetSelection(Ini.KeyboardHandlerMode.GetValue());
	cbox_mouse_handler   ->SetSelection(Ini.MouseHandlerMode.GetValue());
	cbox_audio_out       ->SetSelection(Ini.AudioOutMode.GetValue());
	cbox_camera          ->SetSelection(Ini.Camera.GetValue());
	cbox_camera_type     ->SetSelection(Ini.CameraType.GetValue());
	cbox_hle_loglvl      ->SetSelection(Ini.HLELogLvl.GetValue());
	cbox_sys_lang        ->SetSelection(Ini.SysLanguage.GetValue());
	
	// Enable/Disable parameters
	chbox_audio_dump->Enable(Emu.IsStopped());
	chbox_audio_conv->Enable(Emu.IsStopped());
	chbox_hle_logging->Enable(Emu.IsStopped());
	chbox_rsx_logging->Enable(Emu.IsStopped());
	chbox_hle_hook_stfunc->Enable(Emu.IsStopped());

	s_round_cpu_decoder->Add(cbox_cpu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_spu_decoder->Add(cbox_spu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_gs_render->Add(cbox_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_res->Add(cbox_gs_resolution, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_aspect->Add(cbox_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_io_pad_handler->Add(cbox_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_keyboard_handler->Add(cbox_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_mouse_handler->Add(cbox_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_audio_out->Add(cbox_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_camera->Add(cbox_camera, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_camera_type->Add(cbox_camera_type, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_hle_log_lvl->Add(cbox_hle_loglvl, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_sys_lang->Add(cbox_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

	// Core
	s_subpanel_cpu->Add(s_round_cpu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_cpu->Add(s_round_spu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());

	// Graphics
	s_subpanel_graphics->Add(s_round_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(s_round_gs_res, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(s_round_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_log_prog, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_dump_depth, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_dump_color, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_vsync, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(chbox_gs_3dmonitor, wxSizerFlags().Border(wxALL, 5).Expand());

	// Input - Output
	s_subpanel_io->Add(s_round_io_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io->Add(s_round_io_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io->Add(s_round_io_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());

	// Audio
	s_subpanel_audio->Add(s_round_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_audio->Add(chbox_audio_dump, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_audio->Add(chbox_audio_conv, wxSizerFlags().Border(wxALL, 5).Expand());

	// Camera
	s_subpanel_camera->Add(s_round_camera, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_camera->Add(s_round_camera_type, wxSizerFlags().Border(wxALL, 5).Expand());

	// HLE / Misc.
	s_subpanel_hle->Add(s_round_hle_log_lvl, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_hle->Add(chbox_hle_logging, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_hle->Add(chbox_rsx_logging, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_hle->Add(chbox_hle_hook_stfunc, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_hle->Add(chbox_hle_savetty, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_hle->Add(chbox_hle_exitonstop, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_hle->Add(chbox_hle_always_start, wxSizerFlags().Border(wxALL, 5).Expand());

	//Auto Pause
	s_subpanel_hle->Add(chbox_dbg_ap_systemcall, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_hle->Add(chbox_dbg_ap_functioncall, wxSizerFlags().Border(wxALL, 5).Expand());

	// System
	s_subpanel_system->Add(s_round_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());
	
	// Buttons
	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));
	s_b_panel->Add(new wxButton(&diag, wxID_OK), wxSizerFlags().Border(wxALL, 5).Bottom());
	s_b_panel->Add(new wxButton(&diag, wxID_CANCEL), wxSizerFlags().Border(wxALL, 5).Bottom());

	// Resize panels 
	diag.SetSizerAndFit(s_subpanel_cpu, false);
	diag.SetSizerAndFit(s_subpanel_graphics, false);
	diag.SetSizerAndFit(s_subpanel_io, false);
	diag.SetSizerAndFit(s_subpanel_audio, false);
	diag.SetSizerAndFit(s_subpanel_camera, false);
	diag.SetSizerAndFit(s_subpanel_hle, false);
	diag.SetSizerAndFit(s_subpanel_system, false);
	diag.SetSizerAndFit(s_b_panel, false);
	
	diag.SetSize(width+26, height+80);

	if(diag.ShowModal() == wxID_OK)
	{
		Ini.CPUDecoderMode.SetValue(cbox_cpu_decoder->GetSelection() + 1);
		Ini.SPUDecoderMode.SetValue(cbox_spu_decoder->GetSelection() + 1);
		Ini.GSRenderMode.SetValue(cbox_gs_render->GetSelection());
		Ini.GSResolution.SetValue(ResolutionNumToId(cbox_gs_resolution->GetSelection() + 1));
		Ini.GSAspectRatio.SetValue(cbox_gs_aspect->GetSelection() + 1);
		Ini.GSLogPrograms.SetValue(chbox_gs_log_prog->GetValue());
		Ini.GSDumpDepthBuffer.SetValue(chbox_gs_dump_depth->GetValue());
		Ini.GSDumpColorBuffers.SetValue(chbox_gs_dump_color->GetValue());
		Ini.GSVSyncEnable.SetValue(chbox_gs_vsync->GetValue());
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
		Ini.HLEHookStFunc.SetValue(chbox_hle_hook_stfunc->GetValue());
		Ini.HLESaveTTY.SetValue(chbox_hle_savetty->GetValue());
		Ini.HLEExitOnStop.SetValue(chbox_hle_exitonstop->GetValue());
		Ini.HLELogLvl.SetValue(cbox_hle_loglvl->GetSelection());
		Ini.SysLanguage.SetValue(cbox_sys_lang->GetSelection());
		Ini.HLEAlwaysStart.SetValue(chbox_hle_always_start->GetValue());

		//Auto Pause
		Ini.DBGAutoPauseFunctionCall.SetValue(chbox_dbg_ap_functioncall->GetValue());
		Ini.DBGAutoPauseSystemCall.SetValue(chbox_dbg_ap_systemcall->GetValue());

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
	wxMenuItem& pause = *menubar.FindItem( id_sys_pause );
	wxMenuItem& stop  = *menubar.FindItem( id_sys_stop );
	pause.SetItemLabel(is_running ? "&Pause\tCtrl + P" : is_ready ? "&Start\tCtrl + E" : "&Resume\tCtrl + E");
	pause.Enable(!is_stopped);
	stop.Enable(!is_stopped);

	// PS3 Commands
	wxMenuItem& send_exit = *menubar.FindItem( id_sys_send_exit );
	wxMenuItem& send_open_menu = *menubar.FindItem( id_sys_send_open_menu );
	bool enable_commands = !is_stopped;
	send_open_menu.SetItemLabel(wxString::Format("Send &%s system menu cmd", (m_sys_menu_opened ? "close" : "open")));
	send_open_menu.Enable(enable_commands);
	send_exit.Enable(enable_commands);

	// Tools
	wxMenuItem& kernel_explorer = *menubar.FindItem(id_tools_kernel_explorer);
	wxMenuItem& memory_viewer = *menubar.FindItem(id_tools_memory_viewer);
	wxMenuItem& rsx_debugger = *menubar.FindItem(id_tools_rsx_debugger);
	kernel_explorer.Enable(!is_stopped);
	memory_viewer.Enable(!is_stopped);
	rsx_debugger.Enable(!is_stopped);


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
