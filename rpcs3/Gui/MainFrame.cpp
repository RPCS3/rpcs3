#include "stdafx.h"
#include "MainFrame.h"
#include "CompilerELF.h"
#include "MemoryViewer.h"

#include "git-version.h"
#include "Emu/System.h"
#include "Ini.h"
#include "Emu/GS/sysutil_video.h"
#include "Gui/VHDDManager.h"
#include "Gui/VFSManager.h"
#include "Gui/AboutDialog.h"
#include <wx/dynlib.h>

#include "scetool/scetool.cpp"
#include "unpkg/unpkg.c"

BEGIN_EVENT_TABLE(MainFrame, FrameBase)
	EVT_CLOSE(MainFrame::OnQuit)
END_EVENT_TABLE()

enum IDs
{
	id_boot_elf = 0x555,
	id_boot_self,
	id_boot_game,
	id_boot_pkg,
	id_sys_pause,
	id_sys_stop,
	id_sys_send_open_menu,
	id_sys_send_exit,
	id_config_emu,
	id_config_vfs_manager,
	id_config_vhdd_manager,
	id_tools_compiler,
	id_tools_memory_viewer,
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

	wxMenuBar& menubar(*new wxMenuBar());

	wxMenu& menu_boot(*new wxMenu());
	wxMenu& menu_sys(*new wxMenu());
	wxMenu& menu_conf(*new wxMenu());
	wxMenu& menu_tools(*new wxMenu());
	wxMenu& menu_help(*new wxMenu());

	menubar.Append(&menu_boot, "Boot");
	menubar.Append(&menu_sys, "System");
	menubar.Append(&menu_conf, "Config");
	menubar.Append(&menu_tools, "Tools");
	menubar.Append(&menu_help, "Help");

	menu_boot.Append(id_boot_game, "Boot game");
	menu_boot.Append(id_boot_pkg, "Install PKG");
	menu_boot.AppendSeparator();
	menu_boot.Append(id_boot_elf, "Boot ELF");
	menu_boot.Append(id_boot_self, "Boot SELF");

	menu_sys.Append(id_sys_pause, "Pause")->Enable(false);
	menu_sys.Append(id_sys_stop, "Stop\tCtrl + S")->Enable(false);
	menu_sys.AppendSeparator();
	menu_sys.Append(id_sys_send_open_menu, "Send open system menu cmd")->Enable(false);
	menu_sys.Append(id_sys_send_exit, "Send exit cmd")->Enable(false);

	menu_conf.Append(id_config_emu, "Settings");
	menu_conf.AppendSeparator();
	menu_conf.Append(id_config_vfs_manager, "Virtual File System Manager");
	menu_conf.Append(id_config_vhdd_manager, "Virtual HDD Manager");

	menu_tools.Append(id_tools_compiler, "ELF Compiler");
	menu_tools.Append(id_tools_memory_viewer, "Memory Viewer");

	menu_help.Append(id_help_about, "About...");

	SetMenuBar(&menubar);

	m_game_viewer = new GameViewer(this);
	AddPane(m_game_viewer, "Game List", wxAUI_DOCK_BOTTOM);
	
	Connect( id_boot_game,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootGame) );
	Connect( id_boot_pkg,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootPkg) );
	Connect( id_boot_elf,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootElf) );
	Connect( id_boot_self,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootSelf) );

	Connect( id_sys_pause,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Pause) );
	Connect( id_sys_stop,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Stop) );
	Connect( id_sys_send_open_menu,	wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::SendOpenCloseSysMenu) );
	Connect( id_sys_send_exit,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::SendExit) );

	Connect( id_config_emu,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Config) );
	Connect( id_config_vfs_manager,	wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ConfigVFS) );
	Connect( id_config_vhdd_manager,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ConfigVHDD) );

	Connect( id_tools_compiler,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OpenELFCompiler));
	Connect( id_tools_memory_viewer,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OpenMemoryViewer));

	Connect( id_help_about,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::AboutDialogHandler) );

	Connect( id_update_dbg,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::UpdateUI) );

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

	wxString elf[6] = {
		"\\PS3_GAME\\USRDIR\\BOOT.BIN",
		"\\USRDIR\\BOOT.BIN",
		"\\BOOT.BIN",
		"\\PS3_GAME\\USRDIR\\EBOOT.BIN",
		"\\USRDIR\\EBOOT.BIN",
		"\\EBOOT.BIN"
	};

	for(int i=0;i<6;i++)
	{
		if(wxFile::Access(ctrl.GetPath() + elf[i], wxFile::read))
		{
			ConLog.Write("SELF: booting...");

			Emu.Stop();
	
			wxString fileIn = ctrl.GetPath()+elf[i]; 
			wxString fileOut = (ctrl.GetPath()+elf[i])+".elf";

			// Check if the data really needs to be decrypted.
			FILE *f;
			if((f = fopen(fileIn.mb_str(), "rb")) == NULL)
			{
				ConLog.Error("Could not open game boot file!");
				return;
			}
			
			// Get the key version.
			fseek(f, 0x08, SEEK_SET);
			s16 key_version;
			fread(&key_version, 1, sizeof(key_version), f);
			be_t<u16> key_version_be;
			key_version_be.FromLE(key_version);

			// Get the real elf offset.
			fseek(f, 0x10, SEEK_SET);
			s64 elf_offset;
			fread(&elf_offset, 1, sizeof(elf_offset), f);
			be_t<u64> elf_offset_be;
			elf_offset_be.FromLE(elf_offset);

			fclose(f);

			if(key_version_be.ToBE() == 0x8000)
			{
				ConLog.Warning("Debug SELF detected! Removing fake header...");
				FILE *in;
				FILE *out;
				in = fopen(fileIn, "rb");
				out = fopen(fileOut, "wb");

				// Start at the real elf offset.
				fseek(in, elf_offset_be.ToBE(), SEEK_SET);
				
				// Copy the data.
				int c;
				while ((c = fgetc(in)) != EOF)
					fputc(c, out);

				fclose(out);
				fclose(in);
			}
			else
			{
				scetool_decrypt((scetool::s8 *)fileIn.mb_str(), (scetool::s8 *)fileOut.mb_str());
			}
			
			Emu.SetPath((ctrl.GetPath()+elf[i])+".elf");
			Emu.Load();
			if (!wxRemoveFile((ctrl.GetPath()+elf[i])+".elf"))
				ConLog.Warning("Could not delete the decrypted ELF file");

			ConLog.Write("Game: boot done.");
			return;
		}
	}
	
	ConLog.Error("Ps3 executable not found in selected folder (%s)", ctrl.GetPath().mb_str());
	return;
}


void MainFrame::BootPkg(wxCommandEvent& WXUNUSED(event))
{
	bool stopped = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	wxFileDialog ctrl (this, L"Select PKG", wxEmptyString, wxEmptyString, "*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stopped) Emu.Resume();
		return;
	}

	ConLog.Write("PKG: extracting...");

	Emu.Stop();

	wxString fileName = ctrl.GetPath();
	pkg_unpack((const char *)fileName.mb_str());

	if (!wxRemoveFile(ctrl.GetPath()+".dec"))
		ConLog.Warning("Could not delete the decoded DEC file");

	ConLog.Write("PKG: extract done.");
}

void MainFrame::BootElf(wxCommandEvent& WXUNUSED(event))
{
	bool stopped = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	wxFileDialog ctrl(this, L"Select ELF", wxEmptyString, wxEmptyString, "*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stopped) Emu.Resume();
		return;
	}

	ConLog.Write("ELF: booting...");

	Emu.Stop();

	Emu.SetPath(ctrl.GetPath());
	Emu.Load();

	ConLog.Write("ELF: boot done.");
}

void MainFrame::BootSelf(wxCommandEvent& WXUNUSED(event))
{
	bool stopped = false;

	if(Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	wxFileDialog ctrl(this, L"Select SELF", wxEmptyString, wxEmptyString, "*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stopped) Emu.Resume();
		return;
	}

	ConLog.Write("SELF: booting...");

	Emu.Stop();
	
	wxString fileIn = ctrl.GetPath();
	wxString fileOut = ctrl.GetPath()+".elf";

	// Check if the data really needs to be decrypted.
	if(!wxFileExists(fileIn))
	{
		ConLog.Error("Could not open SELF file!");
		return;
	}
	
	wxFile f(fileIn);
	// Get the key version.
	f.Seek(0x08);
	be_t<u16> key_version;
	f.Read(&key_version, sizeof(key_version));

	if(key_version.ToBE() == const_se_t<u16, 0x8000>::value)
	{
		ConLog.Warning("Debug SELF detected! Removing fake header...");

		// Get the real elf offset.
		f.Seek(0x10);
		be_t<u64> elf_offset;
		f.Read(&elf_offset, sizeof(elf_offset));

		// Start at the real elf offset.
		f.Seek(elf_offset);

		wxFile out(fileOut, wxFile::write);

		// Copy the data.
		char buf[2048];
		while (ssize_t size = f.Read(buf, 2048))
			out.Write(buf, size);
	}
	else
	{
		if (!scetool_decrypt((scetool::s8 *)fileIn.mb_str(), (scetool::s8 *)fileOut.mb_str()))
		{
			ConLog.Write("SELF: Could not decrypt file");
			return;
		}
	}

	f.Close();

	Emu.SetPath(fileOut);
	Emu.Load();
	if (!wxRemoveFile(fileOut))
		ConLog.Warning("Could not delete the decrypted ELF file");

	ConLog.Write("SELF: boot done.");
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
	wxStaticBoxSizer* s_round_pad_handler( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Pad Handler") ) );
	wxStaticBoxSizer* s_round_keyboard_handler( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Keyboard Handler") ) );
	wxStaticBoxSizer* s_round_mouse_handler( new wxStaticBoxSizer( wxVERTICAL, &diag, _("Mouse Handler") ) );

	wxComboBox* cbox_cpu_decoder = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_gs_render = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_gs_resolution = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_gs_aspect = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_pad_handler = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_keyboard_handler = new wxComboBox(&diag, wxID_ANY);
	wxComboBox* cbox_mouse_handler = new wxComboBox(&diag, wxID_ANY);

	wxCheckBox* chbox_gs_dump_depth = new wxCheckBox(&diag, wxID_ANY, "Dump Depth Buffer");
	wxCheckBox* chbox_gs_dump_color = new wxCheckBox(&diag, wxID_ANY, "Dump Color Buffers");
	wxCheckBox* chbox_gs_vsync = new wxCheckBox(&diag, wxID_ANY, "VSync");

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

	chbox_gs_dump_depth->SetValue(Ini.GSDumpDepthBuffer.GetValue());
	chbox_gs_dump_color->SetValue(Ini.GSDumpColorBuffers.GetValue());
	chbox_gs_vsync->SetValue(Ini.GSVSyncEnable.GetValue());

	cbox_cpu_decoder->SetSelection(Ini.CPUDecoderMode.GetValue() ? Ini.CPUDecoderMode.GetValue() - 1 : 0);
	cbox_gs_render->SetSelection(Ini.GSRenderMode.GetValue());
	cbox_gs_resolution->SetSelection(ResolutionIdToNum(Ini.GSResolution.GetValue()) - 1);
	cbox_gs_aspect->SetSelection(Ini.GSAspectRatio.GetValue() - 1);
	cbox_pad_handler->SetSelection(Ini.PadHandlerMode.GetValue());
	cbox_keyboard_handler->SetSelection(Ini.KeyboardHandlerMode.GetValue());
	cbox_mouse_handler->SetSelection(Ini.MouseHandlerMode.GetValue());

	s_round_cpu_decoder->Add(cbox_cpu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_cpu->Add(s_round_cpu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_gs_render->Add(cbox_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_res->Add(cbox_gs_resolution, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_aspect->Add(cbox_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs->Add(s_round_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs->Add(s_round_gs_res, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs->Add(s_round_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs->Add(chbox_gs_dump_depth, wxSizerFlags().Border(wxALL, 5));
	s_round_gs->Add(chbox_gs_dump_color, wxSizerFlags().Border(wxALL, 5));
	s_round_gs->Add(chbox_gs_vsync, wxSizerFlags().Border(wxALL, 5));

	s_round_pad_handler->Add(cbox_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_keyboard_handler->Add(cbox_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_mouse_handler->Add(cbox_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io->Add(s_round_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io->Add(s_round_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io->Add(s_round_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());

	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));

	s_b_panel->Add(new wxButton(&diag, wxID_OK), wxSizerFlags().Border(wxALL, 5).Center());
	s_b_panel->Add(new wxButton(&diag, wxID_CANCEL), wxSizerFlags().Border(wxALL, 5).Center());

	//wxBoxSizer* s_conf_panel(new wxBoxSizer(wxHORIZONTAL));

	s_subpanel1->Add(s_round_cpu, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel1->Add(s_round_gs, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel2->Add(s_round_io, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel1->Add(s_b_panel, wxSizerFlags().Border(wxALL, 8).Expand());

	s_subpanel2->AddSpacer(180);
	s_panel->Add(s_subpanel1, wxSizerFlags().Border(wxALL, 5).Expand());
	s_panel->Add(s_subpanel2, wxSizerFlags().Border(wxALL, 5).Expand());

	diag.SetSizerAndFit( s_panel );
	
	if(diag.ShowModal() == wxID_OK)
	{
		Ini.CPUDecoderMode.SetValue(cbox_cpu_decoder->GetSelection() + 1);
		Ini.GSRenderMode.SetValue(cbox_gs_render->GetSelection());
		Ini.GSResolution.SetValue(ResolutionNumToId(cbox_gs_resolution->GetSelection() + 1));
		Ini.GSAspectRatio.SetValue(cbox_gs_aspect->GetSelection() + 1);
		Ini.GSVSyncEnable.SetValue(chbox_gs_vsync->GetValue());
		Ini.GSDumpDepthBuffer.SetValue(chbox_gs_dump_depth->GetValue());
		Ini.GSDumpColorBuffers.SetValue(chbox_gs_dump_color->GetValue());
		Ini.PadHandlerMode.SetValue(cbox_pad_handler->GetSelection());
		Ini.KeyboardHandlerMode.SetValue(cbox_keyboard_handler->GetSelection());
		Ini.MouseHandlerMode.SetValue(cbox_mouse_handler->GetSelection());

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
	pause.SetText(is_running ? "Pause\tCtrl + P" : is_ready ? "Start\tCtrl + C" : "Resume\tCtrl + C");
	pause.Enable(!is_stopped);
	stop.Enable(!is_stopped);
	//send_exit.Enable(false);
	bool enable_commands = !is_stopped && Emu.GetCallbackManager().m_exit_callback.m_callbacks.GetCount();

	send_open_menu.SetText(wxString::Format("Send %s system menu cmd", m_sys_menu_opened ? "close" : "open"));
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

struct state_hdr
{
	u32 magic;
	u16 version;
	u32 mem_count;
	u32 mem_offset;
	u32 mem_size;
	u32 hle_count;
	u32 hle_offset;
};

static const u32 state_magic = *(u32*)"R3SS";
static const u32 state_version = 0x1000;

void MakeSaveState(wxFile& f)
{
	const ArrayF<MemoryBlock>& mb = Memory.MemoryBlocks;
	
	state_hdr state;
	memset(&state, 0, sizeof(state_hdr));
	
	state.magic = state_magic;
	state.version = state_version;
	state.mem_count = mb.GetCount();
	//state.hle_count = mb.GetCount();
	
	state.mem_offset = sizeof(state_hdr) + 4;
	
	f.Seek(state.mem_offset);
	for(u32 i=0; i<state.mem_count; ++i)
	{
		state.mem_size += mb[i].GetSize();
		f.Write(mb[i].GetMem(), mb[i].GetSize());
	}

	state.hle_offset = state.mem_offset + state.mem_size + 4;

	f.Seek(0);
	f.Write(&state, sizeof(state_hdr));
}

void MainFrame::OnKeyDown(wxKeyEvent& event)
{
	if(wxGetActiveWindow() /*== this*/ && event.ControlDown())
	{
		switch(event.GetKeyCode())
		{
		case 'C': case 'c': if(Emu.IsPaused()) Emu.Resume(); else if(Emu.IsReady()) Emu.Run(); return;
		case 'P': case 'p': if(Emu.IsRunning()) Emu.Pause(); return;
		case 'S': case 's': if(!Emu.IsStopped()) Emu.Stop(); return;
		case 'R': case 'r': if(!Emu.m_path.IsEmpty()) {Emu.Stop(); Emu.Run();} return;
		}
	}

	event.Skip();
}
