#include "stdafx.h"
#include "stdafx_gui.h"
#include "rpcs3.h"
#include "MainFrame.h"

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
#include "Crypto/unself.h"

#include "Loader/PUP.h"
#include "Loader/TAR.h"

#include "Utilities/Thread.h"
#include "Utilities/StrUtil.h"

#include <thread>

#ifndef _WIN32
#include "frame_icon.xpm"
#endif

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
	id_tools_decrypt_sprx_libraries,
	id_tools_install_firmware,
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
	, m_timer(this, id_update_dbg)
	, m_aui_mgr(this)
	, m_sys_menu_opened(false)
{

	SetLabel("RPCS3 v" + rpcs3::version.to_string());

	wxMenuBar* menubar = new wxMenuBar();

	wxMenu* menu_boot = new wxMenu();
	menubar->Append(menu_boot, "&File");
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
	menu_sys->Append(id_sys_send_open_menu, "Press &PS button")->Enable(false);
	menu_sys->Append(id_sys_send_exit, "Send &exit cmd")->Enable(false);

	wxMenu* menu_conf = new wxMenu();
	menubar->Append(menu_conf, "&Config");
	menu_conf->Append(id_config_emu, "&Settings");
	menu_conf->Append(id_config_pad, "&Controller settings");
	menu_conf->AppendSeparator();
	menu_conf->Append(id_config_autopause_manager, "&Auto pause settings");
	//menu_conf->AppendSeparator();
	//menu_conf->Append(id_config_vfs_manager, "Virtual &File system manager");
	//menu_conf->Append(id_config_vhdd_manager, "Virtual &hard drive manager");
	//menu_conf->Append(id_config_savedata_manager, "Save &data utility");

	wxMenu* menu_tools = new wxMenu();
	menubar->Append(menu_tools, "&Tools");
	//menu_tools->Append(id_tools_compiler, "&ELF compiler");
	menu_tools->Append(id_tools_cg_disasm, "&Cg disasm")->Enable();
	menu_tools->Append(id_tools_kernel_explorer, "&Kernel explorer")->Enable(false);
	menu_tools->Append(id_tools_memory_viewer, "&Memory viewer")->Enable(false);
	menu_tools->Append(id_tools_rsx_debugger, "&RSX debugger")->Enable(false);
	menu_tools->Append(id_tools_string_search, "&String search")->Enable(false);
	menu_tools->AppendSeparator();
	menu_tools->Append(id_tools_decrypt_sprx_libraries, "&Decrypt SPRX libraries");
	menu_tools->Append(id_tools_install_firmware, "&Install Firmware");

	wxMenu* menu_help = new wxMenu();
	menubar->Append(menu_help, "&Help");
	menu_help->Append(id_help_about, "&About...");

	SetMenuBar(menubar);
	SetIcon(wxICON(frame_icon));

	// Panels
	m_log_frame = new LogFrame(this);
	m_game_viewer = new GameViewer(this);
	m_debugger_frame = new DebuggerPanel(this);

	AddPane(m_game_viewer, "PS3 software", wxAUI_DOCK_CENTRE);
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
	Bind(wxEVT_MENU, &MainFrame::DecryptSPRXLibraries, this, id_tools_decrypt_sprx_libraries);
	Bind(wxEVT_MENU, &MainFrame::InstallFirmware, this, id_tools_install_firmware);


	Bind(wxEVT_MENU, &MainFrame::OpenELFCompiler, this, id_tools_compiler);
	Bind(wxEVT_MENU, &MainFrame::OpenKernelExplorer, this, id_tools_kernel_explorer);
	Bind(wxEVT_MENU, &MainFrame::OpenMemoryViewer, this, id_tools_memory_viewer);
	Bind(wxEVT_MENU, &MainFrame::OpenRSXDebugger, this, id_tools_rsx_debugger);
	Bind(wxEVT_MENU, &MainFrame::OpenStringSearch, this, id_tools_string_search);
	Bind(wxEVT_MENU, &MainFrame::OpenCgDisasm, this, id_tools_cg_disasm);

	Bind(wxEVT_MENU, &MainFrame::AboutDialogHandler, this, id_help_about);

	Bind(wxEVT_MENU, &MainFrame::UpdateUI, this, id_update_dbg);
	Bind(wxEVT_TIMER, &MainFrame::UpdateUI, this, id_update_dbg);
	Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent&)
	{
		DoSettings(false);
		TheApp->Exit();
	});

	wxGetApp().Bind(wxEVT_KEY_DOWN, &MainFrame::OnKeyDown, this);

	// Check for updates every ~10 ms
	m_timer.Start(10);
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

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	wxDirDialog ctrl(this, L"Select game folder", wxEmptyString);

	if (ctrl.ShowModal() == wxID_CANCEL)
	{
		if (stopped) Emu.Resume();
		return;
	}

	Emu.Stop();

	const std::string path = fmt::ToUTF8(ctrl.GetPath());
	
	if (!Emu.BootGame(path))
	{
		LOG_ERROR(GENERAL, "PS3 executable not found in selected folder (%s)", path);
	}
}

void MainFrame::InstallPkg(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog ctrl(this, L"Select PKG", wxEmptyString, wxEmptyString, "PKG files (*.pkg)|*.pkg|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	if (ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	Emu.Stop();

	const std::string path = fmt::ToUTF8(ctrl.GetPath());

	// Open PKG file
	fs::file pkg_f(path);

	if (!pkg_f || pkg_f.size() < 64)
	{
		LOG_ERROR(LOADER, "PKG: Failed to open %s", path);
		return;
	}

	// Get title ID
	std::vector<char> title_id(9);
	pkg_f.seek(55);
	pkg_f.read(title_id);
	pkg_f.seek(0);

	// Get full path
	const auto& local_path = Emu.GetGameDir() + std::string(std::begin(title_id), std::end(title_id));

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
				return;
			}

			// TODO: Ask user to delete files on cancellation/failure?
			progress = -1.;
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), std::abs(progress) < 1.)
		{
			// Update progress window
			if (!pdlg.Update(static_cast<int>(progress * pdlg.GetRange())))
			{
				// Installation cancelled (signal with negative value)
				progress -= 1.;
				break;
			}
		}
		
		if (progress > 0.)
		{
			pdlg.Update(pdlg.GetRange());
			std::this_thread::sleep_for(100ms);
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

	if (Emu.IsReady()) LOG_SUCCESS(LOADER, "(S)ELF: boot done.");
}

void MainFrame::DecryptSPRXLibraries(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog ctrl(this, L"Select SPRX files", wxEmptyString, wxEmptyString,
		"SPRX files (*.sprx)|*sprx",
		wxFD_OPEN | wxFD_MULTIPLE);

	if (ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	Emu.Stop();

	wxArrayString modules;
	ctrl.GetPaths(modules);

	LOG_NOTICE(GENERAL, "Decrypting SPRX libraries...");

	for (const wxString& module : modules)
	{
		std::string prx_path = fmt::ToUTF8(module);
		const std::string& prx_dir = fs::get_parent_dir(prx_path);

		fs::file elf_file(prx_path);

		if (elf_file && elf_file.size() >= 4 && elf_file.read<u32>() == "SCE\0"_u32)
		{
			const std::size_t prx_ext_pos = prx_path.find_last_of('.');
			const std::string& prx_ext = fmt::to_upper(prx_path.substr(prx_ext_pos != -1 ? prx_ext_pos : prx_path.size()));
			const std::string& prx_name = prx_path.substr(prx_dir.size());

			elf_file = decrypt_self(std::move(elf_file));

			prx_path.erase(prx_path.size() - 4, 1); // change *.sprx to *.prx

			if (elf_file)
			{
				if (fs::file new_file{prx_path, fs::rewrite})
				{
					new_file.write(elf_file.to_string());
					LOG_SUCCESS(GENERAL, "Decrypted %s", prx_dir + prx_name);
				}
				else
				{
					LOG_ERROR(GENERAL, "Failed to create %s", prx_path);
				}				
			}
			else
			{
				LOG_ERROR(GENERAL, "Failed to decrypt %s", prx_dir + prx_name);
			}
		}
	}

	LOG_NOTICE(GENERAL, "Finished decrypting all SPRX libraries.");
}

void MainFrame::InstallFirmware(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog ctrl(this, L"Select PS3UPDAT.PUP file", wxEmptyString, wxEmptyString,
		"PS3 update file (PS3UPDAT.PUP)|PS3UPDAT.PUP",
		wxFD_OPEN);

	if (ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	Emu.Stop();

	const std::string path = fmt::ToUTF8(ctrl.GetPath());

	fs::file pup_f(path);
	pup_object pup(pup_f);
	if (!pup)
	{
		LOG_ERROR(GENERAL, "Error while installing firmware: PUP file is invalid.");
		wxMessageBox("Error while installing firmware: PUP file is invalid.", "Failure!", wxOK | wxICON_ERROR, this);
		return;
	}

	fs::file update_files_f = pup.get_file(0x300);
	tar_object update_files(update_files_f);
	auto updatefilenames = update_files.get_filenames();

	updatefilenames.erase(std::remove_if(
		updatefilenames.begin(), updatefilenames.end(), [](std::string s) { return s.find("dev_flash_") == std::string::npos; }),
	updatefilenames.end());

	wxProgressDialog pdlg("Firmware Installer", "Please wait, unpacking...", updatefilenames.size(), this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT);

	// Synchronization variable
	atomic_t<int> progress(0);
	{
		// Run asynchronously
		scope_thread worker("Firmware Installer", [&]
		{
			for (auto updatefilename : updatefilenames)
			{
				if (progress == -1) break;

				fs::file updatefile = update_files.get_file(updatefilename);

				SCEDecrypter self_dec(updatefile);
				self_dec.LoadHeaders();
				self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
				self_dec.DecryptData();

				auto dev_flash_tar_f = self_dec.MakeFile();
				if (dev_flash_tar_f.size() < 3) {
					LOG_ERROR(GENERAL, "Error while installing firmware: PUP contents are invalid.");
					wxMessageBox("Error while installing firmware: PUP contents are invalid.", "Failure!", wxOK | wxICON_ERROR, this);
					progress = -1;
				}

				tar_object dev_flash_tar(dev_flash_tar_f[2]);
				if (!dev_flash_tar.extract(fs::get_config_dir()))
				{
					LOG_ERROR(GENERAL, "Error while installing firmware: TAR contents are invalid.");
					wxMessageBox("Error while installing firmware: TAR contents are invalid.", "Failure!", wxOK | wxICON_ERROR, this);
					progress = -1;
				}

				if(progress >= 0)
					progress += 1;
			}
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), std::abs(progress) < pdlg.GetRange())
		{
			// Update progress window
			if (!pdlg.Update(static_cast<int>(progress)))
			{
				// Installation cancelled (signal with negative value)
				progress = -1;
				break;
			}
		}

		update_files_f.close();
		pup_f.close();

		if (progress > 0)
		{
			pdlg.Update(pdlg.GetRange());
			std::this_thread::sleep_for(100ms);
		}
	}
	pdlg.Close();

	if (progress > 0)
	{
		LOG_SUCCESS(GENERAL, "Successfully installed PS3 firmware.");
		wxMessageBox("Successfully installed PS3 firmware and LLE Modules!", "Success!", wxOK, this);
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

// This is ugly, but PS3 headers shall not be included there.
extern void sysutil_send_system_cmd(u64 status, u64 param);

void MainFrame::SendExit(wxCommandEvent& event)
{
	sysutil_send_system_cmd(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0);
}

void MainFrame::SendOpenCloseSysMenu(wxCommandEvent& event)
{
	sysutil_send_system_cmd(m_sys_menu_opened ? 0x0132 /* CELL_SYSUTIL_SYSTEM_MENU_CLOSE */ : 0x0131 /* CELL_SYSUTIL_SYSTEM_MENU_OPEN */, 0);
	m_sys_menu_opened = !m_sys_menu_opened;
	wxCommandEvent ce;
	UpdateUI(ce);
}

void MainFrame::Config(wxCommandEvent& WXUNUSED(event))
{
	SettingsDialog(this, "");
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

void MainFrame::UpdateUI(wxEvent& event)
{
	const bool is_running = Emu.IsRunning();
	const bool is_stopped = Emu.IsStopped();
	const bool is_ready = Emu.IsReady();

	// Update menu items based on the state of the emulator
	wxMenuBar& menubar( *GetMenuBar() );

	// Emulation
	wxMenuItem& pause = *menubar.FindItem(id_sys_pause);
	wxMenuItem& stop  = *menubar.FindItem(id_sys_stop);
	pause.SetItemLabel(is_running ? "&Pause\tCtrl + P" : is_ready ? "&Start\tCtrl + F" : "&Resume\tCtrl + F");
	pause.Enable(!is_stopped);
	stop.Enable(!is_stopped);

	// PS3 Commands
	wxMenuItem& send_exit = *menubar.FindItem(id_sys_send_exit);
	wxMenuItem& send_open_menu = *menubar.FindItem(id_sys_send_open_menu);
	bool enable_commands = !is_stopped;
	send_open_menu.SetItemLabel(wxString::Format("Press &PS buton %s", (m_sys_menu_opened ? "close" : "open")));
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

	// Debugger
	m_debugger_frame->UpdateUI();

	// Logs
	m_log_frame->UpdateUI();
}

void MainFrame::OnKeyDown(wxKeyEvent& event)
{
	if(wxGetActiveWindow() /*== this*/ && event.ControlDown())
	{
		switch(event.GetKeyCode())
		{
		case 'F': case 'f': if(Emu.IsPaused()) Emu.Resume(); else if(Emu.IsReady()) Emu.Run(); return;
		case 'P': case 'p': if(Emu.IsRunning()) Emu.Pause(); return;
		case 'S': case 's': if(!Emu.IsStopped()) Emu.Stop(); return;
		case 'R': case 'r': if(!Emu.GetPath().empty()) {Emu.Stop(); Emu.Run();} return;
		}
	}

	event.Skip();
}
