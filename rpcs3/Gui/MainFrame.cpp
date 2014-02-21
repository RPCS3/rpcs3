#include "stdafx.h"
#include "MainFrame.h"
#include "CompilerELF.h"
#include "MemoryViewer.h"
#include "RSXDebugger.h"

#include "git-version.h"
#include "Ini.h"
#include "Emu/GS/sysutil_video.h"
#include "Gui/VHDDManager.h"
#include "Gui/VFSManager.h"
#include "Gui/AboutDialog.h"
#include <wx/dynlib.h>

#include "Loader/PKG.h"

BEGIN_EVENT_TABLE(MainFrame, FrameBase)
	EVT_CLOSE(MainFrame::OnQuit)
END_EVENT_TABLE()

enum IDs
{
	id_boot_elf = 0x555,
	id_boot_game,
	id_install_pkg,
	id_sys_pause,
	id_sys_stop,
	id_sys_send_open_menu,
	id_sys_send_exit,
	id_config_emu,
	id_config_pad,
	id_config_vfs_manager,
	id_config_vhdd_manager,
	id_tools_compiler,
	id_tools_memory_viewer,
	id_tools_rsx_debugger,
	id_help_about,
	id_update_dbg,
};

enum PadIDs
{
	id_pad_left,
	id_pad_down,
	id_pad_right,
	id_pad_up,
	id_pad_start,
	id_pad_r3,
	id_pad_l3,
	id_pad_select,
	id_pad_square,
	id_pad_cross,
	id_pad_circle,
	id_pad_triangle,
	id_pad_r1,
	id_pad_l1,
	id_pad_r2,
	id_pad_l2,
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

	wxMenuBar& menubar(*new wxMenuBar());

	wxMenu& menu_boot(*new wxMenu());
	menubar.Append(&menu_boot, "Boot");
	menu_boot.Append(id_boot_game, "Boot game");
	menu_boot.Append(id_install_pkg, "Install PKG");
	menu_boot.AppendSeparator();
	menu_boot.Append(id_boot_elf, "Boot (S)ELF");

	wxMenu& menu_sys(*new wxMenu());
	menubar.Append(&menu_sys, "System");
	menu_sys.Append(id_sys_pause, "Pause")->Enable(false);
	menu_sys.Append(id_sys_stop, "Stop\tCtrl + S")->Enable(false);
	menu_sys.AppendSeparator();
	menu_sys.Append(id_sys_send_open_menu, "Send open system menu cmd")->Enable(false);
	menu_sys.Append(id_sys_send_exit, "Send exit cmd")->Enable(false);

	wxMenu& menu_conf(*new wxMenu());
	menubar.Append(&menu_conf, "Config");
	menu_conf.Append(id_config_emu, "Settings");
	menu_conf.Append(id_config_pad, "PAD Settings");
	menu_conf.AppendSeparator();
	menu_conf.Append(id_config_vfs_manager, "Virtual File System Manager");
	menu_conf.Append(id_config_vhdd_manager, "Virtual HDD Manager");

	wxMenu& menu_tools(*new wxMenu());
	menubar.Append(&menu_tools, "Tools");
	menu_tools.Append(id_tools_compiler, "ELF Compiler");
	menu_tools.Append(id_tools_memory_viewer, "Memory Viewer");
	menu_tools.Append(id_tools_rsx_debugger, "RSX Debugger");

	wxMenu& menu_help(*new wxMenu());
	menubar.Append(&menu_help, "Help");
	menu_help.Append(id_help_about, "About...");

	SetMenuBar(&menubar);

	// Panels
	m_game_viewer = new GameViewer(this);
	m_debugger_frame = new DebuggerPanel(this);
	ConLogFrame = new LogFrame(this);

	AddPane(m_game_viewer, "Game List", wxAUI_DOCK_BOTTOM);
	AddPane(ConLogFrame, "Log", wxAUI_DOCK_BOTTOM);
	AddPane(m_debugger_frame, "Debugger", wxAUI_DOCK_RIGHT);
	
	// Events
	Connect( id_boot_game,			 wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootGame) );
	Connect( id_install_pkg,		 wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::InstallPkg) );
	Connect( id_boot_elf,			 wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootElf) );

	Connect( id_sys_pause,           wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Pause) );
	Connect( id_sys_stop,            wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Stop) );
	Connect( id_sys_send_open_menu,	 wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::SendOpenCloseSysMenu) );
	Connect( id_sys_send_exit,       wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::SendExit) );

	Connect( id_config_emu,          wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Config) );
	Connect( id_config_pad,          wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ConfigPad) );
	Connect( id_config_vfs_manager,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ConfigVFS) );
	Connect( id_config_vhdd_manager, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ConfigVHDD) );

	Connect( id_tools_compiler,		 wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OpenELFCompiler));
	Connect( id_tools_memory_viewer, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OpenMemoryViewer));
	Connect( id_tools_rsx_debugger,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OpenRSXDebugger));

	Connect( id_help_about,          wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::AboutDialogHandler) );

	Connect( id_update_dbg,          wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::UpdateUI) );

	m_app_connector.Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainFrame::OnKeyDown), (wxObject*)0, this);
	m_app_connector.Connect(wxEVT_DBG_COMMAND, wxCommandEventHandler(MainFrame::UpdateUI), (wxObject*)0, this);
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
	IniEntry<wxString> ini;
	ini.Init("Settings", "MainFrameAui");

	if(load)
	{
		m_aui_mgr.LoadPerspective(ini.LoadValue(m_aui_mgr.SavePerspective()));
	}
	else
	{
		ini.SaveValue(m_aui_mgr.SavePerspective());
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
		ConLog.Success("Game: boot done.");
	}
	else
	{
		ConLog.Error("Ps3 executable not found in selected folder (%s)", ctrl.GetPath().wx_str());
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
	std::string filePath = ctrl.GetPath();
	wxFile pkg_f(filePath, wxFile::read); // TODO: Use VFS to install PKG files

	if (pkg_f.IsOpened())
	{
		PKGLoader pkg(pkg_f);
		pkg.Install("/dev_hdd0/game/");
		pkg.Close();
	}

	// Refresh game list
	m_game_viewer->Refresh();
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

	ConLog.Write("(S)ELF: booting...");

	Emu.Stop();

	Emu.SetPath(ctrl.GetPath());
	Emu.Load();

	ConLog.Success("(S)ELF: boot done.");
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
	Emu.GetCallbackManager().m_exit_callback.Handle(0x0101, 0);
}

void MainFrame::SendOpenCloseSysMenu(wxCommandEvent& event)
{
	Emu.GetCallbackManager().m_exit_callback.Handle(m_sys_menu_opened ? 0x0132 : 0x0131, 0);
	m_sys_menu_opened = !m_sys_menu_opened;
	wxCommandEvent ce;
	UpdateUI(ce);
}

void MainFrame::Config(wxCommandEvent& WXUNUSED(event))
{
	//TODO

	bool paused = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		paused = true;
	}

	wxDialog diag(this, wxID_ANY, "Settings", wxDefaultPosition);

	wxBoxSizer* s_panel(new wxBoxSizer(wxHORIZONTAL));
	wxBoxSizer* s_subpanel1(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_subpanel2(new wxBoxSizer(wxVERTICAL));

	wxStaticBoxSizer* s_round_cpu( new wxStaticBoxSizer( wxVERTICAL, &diag, _("CPU") ) );
	wxStaticBoxSizer* s_round_cpu_decoder( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Decoder") ) );

	wxStaticBoxSizer* s_round_gs( new wxStaticBoxSizer( wxVERTICAL, &diag, _("GS") ) );
	wxStaticBoxSizer* s_round_gs_render( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Render") ) );
	wxStaticBoxSizer* s_round_gs_res( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Default resolution") ) );
	wxStaticBoxSizer* s_round_gs_aspect( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Default aspect ratio") ) );

	wxStaticBoxSizer* s_round_io( new wxStaticBoxSizer( wxVERTICAL, &diag, _("IO") ) );
	wxStaticBoxSizer* s_round_io_pad_handler( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Pad Handler") ) );
	wxStaticBoxSizer* s_round_io_keyboard_handler( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Keyboard Handler") ) );
	wxStaticBoxSizer* s_round_io_mouse_handler( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Mouse Handler") ) );
	
	wxStaticBoxSizer* s_round_audio( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Audio") ) );
	wxStaticBoxSizer* s_round_audio_out( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Audio Out") ) );

	wxStaticBoxSizer* s_round_hle( new wxStaticBoxSizer( wxVERTICAL, &diag, _("HLE") ) );

	wxComboBox* cbox_cpu_decoder = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_gs_render = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_gs_resolution = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_gs_aspect = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_handler = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_keyboard_handler = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_mouse_handler = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_audio_out = new wxComboBox(&diag, wxID_ANY);

	wxCheckBox* chbox_cpu_ignore_rwerrors = new wxCheckBox(&diag, wxID_ANY, "Ignore Read/Write errors");
	wxCheckBox* chbox_gs_log_prog   = new wxCheckBox(&diag, wxID_ANY, "Log vertex/fragment programs");
	wxCheckBox* chbox_gs_dump_depth = new wxCheckBox(&diag, wxID_ANY, "Dump Depth Buffer");
	wxCheckBox* chbox_gs_dump_color = new wxCheckBox(&diag, wxID_ANY, "Dump Color Buffers");
	wxCheckBox* chbox_gs_vsync = new wxCheckBox(&diag, wxID_ANY, "VSync");
	wxCheckBox* chbox_hle_logging = new wxCheckBox(&diag, wxID_ANY, "Log all SysCalls");

	//cbox_cpu_decoder->Append("DisAsm");
	cbox_cpu_decoder->Append("Interpreter & DisAsm");
	cbox_cpu_decoder->Append("Interpreter");

	for(int i=1; i<WXSIZEOF(ResolutionTable); ++i)
	{
		cbox_gs_resolution->Append(wxString::Format("%dx%d", ResolutionTable[i].width, ResolutionTable[i].height));
	}

	cbox_gs_aspect->Append("4:3");
	cbox_gs_aspect->Append("16:9");

	cbox_gs_render->Append("Null");
	cbox_gs_render->Append("OpenGL");
	//cbox_gs_render->Append("Software");

	cbox_pad_handler->Append("Null");
	cbox_pad_handler->Append("Windows");
	//cbox_pad_handler->Append("DirectInput");

	cbox_keyboard_handler->Append("Null");
	cbox_keyboard_handler->Append("Windows");
	//cbox_keyboard_handler->Append("DirectInput");

	cbox_mouse_handler->Append("Null");
	cbox_mouse_handler->Append("Windows");
	//cbox_mouse_handler->Append("DirectInput");

	cbox_audio_out->Append("Null");

	chbox_cpu_ignore_rwerrors->SetValue(Ini.CPUIgnoreRWErrors.GetValue());
	chbox_gs_log_prog->SetValue(Ini.GSLogPrograms.GetValue());
	chbox_gs_dump_depth->SetValue(Ini.GSDumpDepthBuffer.GetValue());
	chbox_gs_dump_color->SetValue(Ini.GSDumpColorBuffers.GetValue());
	chbox_gs_vsync->SetValue(Ini.GSVSyncEnable.GetValue());
	chbox_hle_logging->SetValue(Ini.HLELogging.GetValue());

	cbox_cpu_decoder->SetSelection(Ini.CPUDecoderMode.GetValue() ? Ini.CPUDecoderMode.GetValue() - 1 : 0);
	cbox_gs_render->SetSelection(Ini.GSRenderMode.GetValue());
	cbox_gs_resolution->SetSelection(ResolutionIdToNum(Ini.GSResolution.GetValue()) - 1);
	cbox_gs_aspect->SetSelection(Ini.GSAspectRatio.GetValue() - 1);
	cbox_pad_handler->SetSelection(Ini.PadHandlerMode.GetValue());
	cbox_keyboard_handler->SetSelection(Ini.KeyboardHandlerMode.GetValue());
	cbox_mouse_handler->SetSelection(Ini.MouseHandlerMode.GetValue());
	cbox_audio_out->SetSelection(Ini.AudioOutMode.GetValue());

	s_round_cpu_decoder->Add(cbox_cpu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_cpu->Add(s_round_cpu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_cpu->Add(chbox_cpu_ignore_rwerrors, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_gs_render->Add(cbox_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_res->Add(cbox_gs_resolution, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_aspect->Add(cbox_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs->Add(s_round_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs->Add(s_round_gs_res, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs->Add(s_round_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs->Add(chbox_gs_log_prog, wxSizerFlags().Border(wxALL, 5));
	s_round_gs->Add(chbox_gs_dump_depth, wxSizerFlags().Border(wxALL, 5));
	s_round_gs->Add(chbox_gs_dump_color, wxSizerFlags().Border(wxALL, 5));
	s_round_gs->Add(chbox_gs_vsync, wxSizerFlags().Border(wxALL, 5));

	s_round_io_pad_handler->Add(cbox_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_keyboard_handler->Add(cbox_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_mouse_handler->Add(cbox_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io->Add(s_round_io_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io->Add(s_round_io_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io->Add(s_round_io_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_audio_out->Add(cbox_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_audio->Add(s_round_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_hle->Add(chbox_hle_logging, wxSizerFlags().Border(wxALL, 5).Expand());

	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));

	s_b_panel->Add(new wxButton(&diag, wxID_OK), wxSizerFlags().Border(wxALL, 5).Center());
	s_b_panel->Add(new wxButton(&diag, wxID_CANCEL), wxSizerFlags().Border(wxALL, 5).Center());

	//wxBoxSizer* s_conf_panel(new wxBoxSizer(wxHORIZONTAL));

	s_subpanel1->Add(s_round_cpu, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel1->Add(s_round_gs, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel1->Add(s_b_panel, wxSizerFlags().Border(wxALL, 8).Expand());
	s_subpanel2->Add(s_round_io, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel2->Add(s_round_audio, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel2->Add(s_round_hle, wxSizerFlags().Border(wxALL, 5).Expand());

	s_panel->Add(s_subpanel1, wxSizerFlags().Border(wxALL, 5).Expand());
	s_panel->Add(s_subpanel2, wxSizerFlags().Border(wxALL, 5).Expand());

	diag.SetSizerAndFit( s_panel );
	
	if(diag.ShowModal() == wxID_OK)
	{
		Ini.CPUDecoderMode.SetValue(cbox_cpu_decoder->GetSelection() + 1);
		Ini.CPUIgnoreRWErrors.SetValue(chbox_cpu_ignore_rwerrors->GetValue());
		Ini.GSRenderMode.SetValue(cbox_gs_render->GetSelection());
		Ini.GSResolution.SetValue(ResolutionNumToId(cbox_gs_resolution->GetSelection() + 1));
		Ini.GSAspectRatio.SetValue(cbox_gs_aspect->GetSelection() + 1);
		Ini.GSVSyncEnable.SetValue(chbox_gs_vsync->GetValue());
		Ini.GSLogPrograms.SetValue(chbox_gs_log_prog->GetValue());
		Ini.GSDumpDepthBuffer.SetValue(chbox_gs_dump_depth->GetValue());
		Ini.GSDumpColorBuffers.SetValue(chbox_gs_dump_color->GetValue());
		Ini.PadHandlerMode.SetValue(cbox_pad_handler->GetSelection());
		Ini.KeyboardHandlerMode.SetValue(cbox_keyboard_handler->GetSelection());
		Ini.MouseHandlerMode.SetValue(cbox_mouse_handler->GetSelection());
		Ini.AudioOutMode.SetValue(cbox_audio_out->GetSelection());
		Ini.HLELogging.SetValue(chbox_hle_logging->GetValue());

		Ini.Save();
	}

	if(paused) Emu.Resume();
}

void MainFrame::ConfigPad(wxCommandEvent& WXUNUSED(event))
{
	bool paused = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		paused = true;
	}

	wxDialog diag(this, wxID_ANY, "PAD Settings", wxDefaultPosition);

	wxBoxSizer* s_panel(new wxBoxSizer(wxHORIZONTAL));
	wxBoxSizer* s_subpanel1(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_subpanel2(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_subpanel3(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_subpanel4(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_subpanel5(new wxBoxSizer(wxVERTICAL));

	wxStaticBoxSizer* s_round_pad_controls( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Controls") ) );
	wxStaticBoxSizer* s_round_pad_left(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("LEFT") ) );
	wxStaticBoxSizer* s_round_pad_down(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("DOWN") ) );
	wxStaticBoxSizer* s_round_pad_right(	new wxStaticBoxSizer( wxVERTICAL, &diag, _("RIGHT") ) );
	wxStaticBoxSizer* s_round_pad_up(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("UP") ) );

	wxStaticBoxSizer* s_round_pad_shifts_l( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Shifts") ) );
	wxStaticBoxSizer* s_round_pad_l1(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("L1") ) );
	wxStaticBoxSizer* s_round_pad_l2(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("L2") ) );
	wxStaticBoxSizer* s_round_pad_l3(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("L3") ) );

	wxStaticBoxSizer* s_round_pad_system(	new wxStaticBoxSizer( wxVERTICAL, &diag, _("System") ) );
	wxStaticBoxSizer* s_round_pad_select(	new wxStaticBoxSizer( wxVERTICAL, &diag, _("SELECT") ) );
	wxStaticBoxSizer* s_round_pad_start(	new wxStaticBoxSizer( wxVERTICAL, &diag, _("START") ) );

	wxStaticBoxSizer* s_round_pad_shifts_r( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Shifts") ) );
	wxStaticBoxSizer* s_round_pad_r1(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("R1") ) );
	wxStaticBoxSizer* s_round_pad_r2(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("R2") ) );
	wxStaticBoxSizer* s_round_pad_r3(		new wxStaticBoxSizer( wxVERTICAL, &diag, _("R3") ) );

	wxStaticBoxSizer* s_round_pad_buttons(	new wxStaticBoxSizer( wxVERTICAL, &diag, _("Buttons") ) );
	wxStaticBoxSizer* s_round_pad_square(	new wxStaticBoxSizer( wxVERTICAL, &diag, _("SQUARE") ) );
	wxStaticBoxSizer* s_round_pad_cross(	new wxStaticBoxSizer( wxVERTICAL, &diag, _("CROSS") ) );
	wxStaticBoxSizer* s_round_pad_circle(	new wxStaticBoxSizer( wxVERTICAL, &diag, _("CIRCLE") ) );
	wxStaticBoxSizer* s_round_pad_triangle( new wxStaticBoxSizer( wxVERTICAL, &diag, _("TRIANGLE") ) );


	wxComboBox* cbox_pad_left =		new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_down =		new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_right =	new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_up =		new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_start =	new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_r3 =		new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_l3 =		new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_select =	new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_square =	new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_cross =	new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_circle =	new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_triangle = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_r1 =		new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_l1 =		new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_r2 =		new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_l2 =		new wxComboBox(&diag, wxID_ANY);

	for(int i=0; i<128; i++)
	{
		cbox_pad_left->Append		(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_down->Append		(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_right->Append		(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_up->Append			(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_r3->Append			(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_l3->Append			(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_square->Append		(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_cross->Append		(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_circle->Append		(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_triangle->Append	(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_r1->Append			(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_l1->Append			(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_r2->Append			(wxString::Format("%c", static_cast<char>(i) ) );
		cbox_pad_l2->Append			(wxString::Format("%c", static_cast<char>(i) ) );
	}

	cbox_pad_start->Append("Enter");
	cbox_pad_select->Append("Space");

	cbox_pad_left->SetSelection		(Ini.PadHandlerLeft.GetValue());
	cbox_pad_down->SetSelection		(Ini.PadHandlerDown.GetValue());
	cbox_pad_right->SetSelection	(Ini.PadHandlerRight.GetValue());
	cbox_pad_up->SetSelection		(Ini.PadHandlerUp.GetValue());
	cbox_pad_start->SetSelection	(Ini.PadHandlerStart.GetValue());
	cbox_pad_r3->SetSelection		(Ini.PadHandlerR3.GetValue());
	cbox_pad_l3->SetSelection		(Ini.PadHandlerL3.GetValue());
	cbox_pad_select->SetSelection	(Ini.PadHandlerSelect.GetValue());
	cbox_pad_square->SetSelection	(Ini.PadHandlerSquare.GetValue());
	cbox_pad_cross->SetSelection	(Ini.PadHandlerCross.GetValue());
	cbox_pad_circle->SetSelection	(Ini.PadHandlerCircle.GetValue());
	cbox_pad_triangle->SetSelection	(Ini.PadHandlerTriangle.GetValue());
	cbox_pad_r1->SetSelection		(Ini.PadHandlerR1.GetValue());
	cbox_pad_l1->SetSelection		(Ini.PadHandlerL1.GetValue());
	cbox_pad_r2->SetSelection		(Ini.PadHandlerR2.GetValue());
	cbox_pad_l2->SetSelection		(Ini.PadHandlerL2.GetValue());

	s_round_pad_left->Add(cbox_pad_left, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_down->Add(cbox_pad_down, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_right->Add(cbox_pad_right, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_up->Add(cbox_pad_up, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_start->Add(cbox_pad_start, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_r3->Add(cbox_pad_r3, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_l3->Add(cbox_pad_l3, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_select->Add(cbox_pad_select, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_square->Add(cbox_pad_square, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_cross->Add(cbox_pad_cross, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_circle->Add(cbox_pad_circle, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_triangle->Add(cbox_pad_triangle, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_r1->Add(cbox_pad_r1, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_l1->Add(cbox_pad_l1, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_r2->Add(cbox_pad_r2, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_l2->Add(cbox_pad_l2, wxSizerFlags().Border(wxALL, 5).Expand());


	s_round_pad_controls->Add(s_round_pad_left, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_controls->Add(s_round_pad_down, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_controls->Add(s_round_pad_right, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_controls->Add(s_round_pad_up, wxSizerFlags().Border(wxALL, 5).Expand());

	
	s_round_pad_shifts_l->Add(s_round_pad_l1, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_shifts_l->Add(s_round_pad_l2, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_shifts_l->Add(s_round_pad_l3, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_pad_system->Add(s_round_pad_start, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_system->Add(s_round_pad_select, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_pad_shifts_r->Add(s_round_pad_r1, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_shifts_r->Add(s_round_pad_r2, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_shifts_r->Add(s_round_pad_r3, wxSizerFlags().Border(wxALL, 5).Expand());


	s_round_pad_buttons->Add(s_round_pad_square, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_buttons->Add(s_round_pad_cross, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_buttons->Add(s_round_pad_circle, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_pad_buttons->Add(s_round_pad_triangle, wxSizerFlags().Border(wxALL, 5).Expand());

	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));

	s_b_panel->Add(new wxButton(&diag, wxID_OK), wxSizerFlags().Border(wxALL, 5).Center());
	s_b_panel->Add(new wxButton(&diag, wxID_CANCEL), wxSizerFlags().Border(wxALL, 5).Center());

	s_subpanel1->Add(s_round_pad_controls, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel2->Add(s_round_pad_shifts_l, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel3->Add(s_round_pad_system, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel3->Add(s_b_panel, wxSizerFlags().Border(wxALL, 8).Expand());
	s_subpanel4->Add(s_round_pad_shifts_r, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel5->Add(s_round_pad_buttons, wxSizerFlags().Border(wxALL, 5).Expand());


	s_panel->Add(s_subpanel1, wxSizerFlags().Border(wxALL, 5).Expand());
	s_panel->Add(s_subpanel2, wxSizerFlags().Border(wxALL, 5).Expand());
	s_panel->Add(s_subpanel3, wxSizerFlags().Border(wxALL, 5).Expand());
	s_panel->Add(s_subpanel4, wxSizerFlags().Border(wxALL, 5).Expand());
	s_panel->Add(s_subpanel5, wxSizerFlags().Border(wxALL, 5).Expand());

	diag.SetSizerAndFit( s_panel );
	
	if(diag.ShowModal() == wxID_OK)
	{
		Ini.PadHandlerLeft.SetValue(cbox_pad_left->GetSelection());
		Ini.PadHandlerDown.SetValue(cbox_pad_down->GetSelection());
		Ini.PadHandlerRight.SetValue(cbox_pad_right->GetSelection());
		Ini.PadHandlerUp.SetValue(cbox_pad_up->GetSelection());
		Ini.PadHandlerStart.SetValue(cbox_pad_start->GetSelection());
		Ini.PadHandlerR3.SetValue(cbox_pad_r3->GetSelection());
		Ini.PadHandlerL3.SetValue(cbox_pad_l3->GetSelection());
		Ini.PadHandlerSelect.SetValue(cbox_pad_select->GetSelection());
		Ini.PadHandlerSquare.SetValue(cbox_pad_square->GetSelection());
		Ini.PadHandlerCross.SetValue(cbox_pad_cross->GetSelection());
		Ini.PadHandlerCircle.SetValue(cbox_pad_circle->GetSelection());
		Ini.PadHandlerTriangle.SetValue(cbox_pad_triangle->GetSelection());
		Ini.PadHandlerR1.SetValue(cbox_pad_r1->GetSelection());
		Ini.PadHandlerL1.SetValue(cbox_pad_l1->GetSelection());
		Ini.PadHandlerR2.SetValue(cbox_pad_r2->GetSelection());
		Ini.PadHandlerL2.SetValue(cbox_pad_l2->GetSelection());

		Ini.Save();
	}

	if(paused) Emu.Resume();
}

void MainFrame::ConfigVFS(wxCommandEvent& WXUNUSED(event))
{
	VFSManagerDialog(this).ShowModal();
}

void MainFrame::ConfigVHDD(wxCommandEvent& WXUNUSED(event))
{
	VHDDManagerDialog(this).ShowModal();
}

void MainFrame::OpenELFCompiler(wxCommandEvent& WXUNUSED(event))
{
	(new CompilerELF(this)) -> Show();
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
	}
	else
	{
		is_running = Emu.IsRunning();
		is_stopped = Emu.IsStopped();
		is_ready = Emu.IsReady();
	}

	wxMenuBar& menubar( *GetMenuBar() );
	wxMenuItem& pause = *menubar.FindItem( id_sys_pause );
	wxMenuItem& stop  = *menubar.FindItem( id_sys_stop );
	wxMenuItem& send_exit = *menubar.FindItem( id_sys_send_exit );
	wxMenuItem& send_open_menu = *menubar.FindItem( id_sys_send_open_menu );
	pause.SetItemLabel(is_running ? "Pause\tCtrl + P" : is_ready ? "Start\tCtrl + C" : "Resume\tCtrl + C");
	pause.Enable(!is_stopped);
	stop.Enable(!is_stopped);
	//send_exit.Enable(false);
	bool enable_commands = !is_stopped && Emu.GetCallbackManager().m_exit_callback.m_callbacks.GetCount();

	send_open_menu.SetItemLabel(wxString::Format("Send %s system menu cmd", wxString(m_sys_menu_opened ? "close" : "open").wx_str()));
	send_open_menu.Enable(enable_commands);
	send_exit.Enable(enable_commands);

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
		case 'R': case 'r': if(!Emu.m_path.IsEmpty()) {Emu.Stop(); Emu.Run();} return;
		}
	}

	event.Skip();
}
