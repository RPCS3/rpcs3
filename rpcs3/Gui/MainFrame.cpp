#include "stdafx.h"
#include "stdafx_gui.h"
#include "rpcs3.h"
#include "MainFrame.h"
#include "rpcs3_version.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Gui/PADManager.h"
#include "Gui/AboutDialog.h"
#include "Gui/GameViewer.h"
#include "Gui/AutoPauseManager.h"
#include "Gui/SaveDataUtility.h"
#include "Gui/KernelExplorer.h"
#include "Gui/MemoryViewer.h"
#include "Gui/RSXDebugger.h"
#include "Gui/SettingsDialog.h"
#include "Gui/MemoryStringSearcher.h"
#include "Gui/CgDisasm.h"
#include "Crypto/unpkg.h"

#include "Utilities/Thread.h"

#include <thread>

#ifndef _WIN32
#include "frame_icon.xpm"
#endif

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

	SetLabel(std::string(_PRGNAME_ " v") + rpcs3::version.to_string());

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
	//menu_conf->AppendSeparator();
	//menu_conf->Append(id_config_vfs_manager, "Virtual &File System Manager");
	//menu_conf->Append(id_config_vhdd_manager, "Virtual &HDD Manager");
	//menu_conf->Append(id_config_savedata_manager, "Save &Data Utility");

	wxMenu* menu_tools = new wxMenu();
	menubar->Append(menu_tools, "&Tools");
	//menu_tools->Append(id_tools_compiler, "&ELF Compiler");
	menu_tools->Append(id_tools_cg_disasm, "&Cg Disasm")->Enable();
	menu_tools->Append(id_tools_kernel_explorer, "&Kernel Explorer")->Enable(false);
	menu_tools->Append(id_tools_memory_viewer, "&Memory Viewer")->Enable(false);
	menu_tools->Append(id_tools_rsx_debugger, "&RSX Debugger")->Enable(false);
	menu_tools->Append(id_tools_string_search, "&String Search")->Enable(false);

	wxMenu* menu_help = new wxMenu();
	menubar->Append(menu_help, "&Help");
	menu_help->Append(id_help_about, "&About...");

	SetMenuBar(menubar);
	SetIcon(wxICON(frame_icon));

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
	Bind(wxEVT_MENU, &MainFrame::ConfigAutoPause, this, id_config_autopause_manager);
	Bind(wxEVT_MENU, &MainFrame::ConfigVFS, this, id_config_vfs_manager);
	Bind(wxEVT_MENU, &MainFrame::ConfigVHDD, this, id_config_vhdd_manager);
	Bind(wxEVT_MENU, &MainFrame::ConfigSaveData, this, id_config_savedata_manager);

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

	LOG_NOTICE(GENERAL, "%s", (std::string(_PRGNAME_ " v") + rpcs3::version.to_string()).c_str());
	LOG_NOTICE(GENERAL, "");
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
	auto&& cfg = g_gui_cfg[ini_name]["aui"];

	if (load)
	{
		const auto& perspective = fmt::FromUTF8(cfg.Scalar());

		m_aui_mgr.LoadPerspective(perspective.empty() ? m_aui_mgr.SavePerspective() : perspective);
	}
	else
	{
		cfg = fmt::ToUTF8(m_aui_mgr.SavePerspective());
		save_gui_cfg();
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
	
	if(!Emu.BootGame(ctrl.GetPath().ToStdString()))
	{
		LOG_ERROR(GENERAL, "PS3 executable not found in selected folder (%s)", fmt::ToUTF8(ctrl.GetPath())); // passing std::string (test)
	}
}

void MainFrame::InstallPkg(wxCommandEvent& WXUNUSED(event))
{
	const bool paused = Emu.Pause();

	wxFileDialog ctrl(this, L"Select PKG", wxEmptyString, wxEmptyString, "PKG files (*.pkg)|*.pkg|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	if (ctrl.ShowModal() == wxID_CANCEL)
	{
		if (paused) Emu.Resume();
		return;
	}

	Emu.Stop();

	// Open PKG file
	fs::file pkg_f(ctrl.GetPath().ToStdString());

	if (!pkg_f || pkg_f.size() < 64)
	{
		LOG_ERROR(LOADER, "PKG: Failed to open %s", ctrl.GetPath().ToStdString());
		return;
	}

	// Get title ID
	std::vector<char> title_id(9);
	pkg_f.seek(55);
	pkg_f.read(title_id);
	pkg_f.seek(0);

	// Get full path
	const auto& local_path = vfs::get("/dev_hdd0/game/") + std::string(std::begin(title_id), std::end(title_id));

	if (!fs::create_dir(local_path))
	{
		if (fs::is_dir(local_path))
		{
			if (wxMessageDialog(this, "Another installation found. Do you want to overwrite it?", "PKG Decrypter / Installer", wxYES_NO | wxCENTRE).ShowModal() != wxID_YES)
			{
				LOG_ERROR(LOADER, "PKG: Cancelled installation to existing directory %s", local_path);
				return;
			}
		}
		else
		{
			LOG_ERROR(LOADER, "PKG: Could not create the installation directory %s", local_path);
			return;
		}
	}

	wxProgressDialog pdlg("PKG Installer", "Please wait, unpacking...", 1000, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT);

	// Synchronization variable
	atomic_t<double> progress(0.);
	{
		// Run PKG unpacking asynchronously
		scope_thread worker("PKG Installer", [&]
		{
			if (pkg_install(pkg_f, local_path + '/', progress))
			{
				progress = 1.;
			}

			// TODO: Ask user to delete files on cancellation/failure?
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), progress < 1.)
		{
			// Update progress window
			if (!pdlg.Update(static_cast<int>(progress * pdlg.GetRange())))
			{
				// Installation cancelled (signal with negative value)
				progress -= 1.;
				break;
			}
		}
	}

	pdlg.Close();

	if (progress >= 1.)
	{
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

	LOG_NOTICE(LOADER, "(S)ELF: booting...");

	Emu.Stop();
	Emu.SetPath(fmt::ToUTF8(ctrl.GetPath()));
	Emu.Load();

	LOG_SUCCESS(LOADER, "(S)ELF: boot done.");
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

// This is ugly, but PS3 headers shall not be included there.
extern void sysutilSendSystemCommand(u64 status, u64 param);

void MainFrame::SendExit(wxCommandEvent& event)
{
	sysutilSendSystemCommand(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0);
}

void MainFrame::SendOpenCloseSysMenu(wxCommandEvent& event)
{
	sysutilSendSystemCommand(m_sys_menu_opened ? 0x0132 /* CELL_SYSUTIL_SYSTEM_MENU_CLOSE */ : 0x0131 /* CELL_SYSUTIL_SYSTEM_MENU_OPEN */, 0);
	m_sys_menu_opened = !m_sys_menu_opened;
	wxCommandEvent ce;
	UpdateUI(ce);
}

void MainFrame::Config(wxCommandEvent& WXUNUSED(event))
{
	SettingsDialog(this);
}

void MainFrame::ConfigPad(wxCommandEvent& WXUNUSED(event))
{
	PADManager(this).ShowModal();
}

void MainFrame::ConfigVFS(wxCommandEvent& WXUNUSED(event))
{
	//VFSManagerDialog(this).ShowModal();
}

void MainFrame::ConfigVHDD(wxCommandEvent& WXUNUSED(event))
{
	//VHDDManagerDialog(this).ShowModal();
}

void MainFrame::ConfigAutoPause(wxCommandEvent& WXUNUSED(event))
{
	AutoPauseManagerDialog(this).ShowModal();
}

void MainFrame::ConfigSaveData(wxCommandEvent& event)
{
	SaveDataListDialog(this, true).ShowModal();
}

void MainFrame::OpenELFCompiler(wxCommandEvent& WXUNUSED(event))
{
	//(new CompilerELF(this))->Show();
}

void MainFrame::OpenKernelExplorer(wxCommandEvent& WXUNUSED(event))
{
	(new KernelExplorer(this))->Show();
}

void MainFrame::OpenMemoryViewer(wxCommandEvent& WXUNUSED(event))
{
	(new MemoryViewerPanel(this))->Show();
}

void MainFrame::OpenRSXDebugger(wxCommandEvent& WXUNUSED(event))
{
	(new RSXDebugger(this))->Show();
}

void MainFrame::OpenStringSearch(wxCommandEvent& WXUNUSED(event))
{
	(new MemoryStringSearcher(this))->Show();
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
		case 'R': case 'r': if(!Emu.GetPath().empty()) {Emu.Stop(); Emu.Run();} return;
		}
	}

	event.Skip();
}
