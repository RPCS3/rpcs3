#include "stdafx.h"
#include "MainFrame.h"
#include "CompilerELF.h"

#include "Emu/System.h"
#include "Ini.h"
#include "svnrev.h"

BEGIN_EVENT_TABLE(MainFrame, FrameBase)
	EVT_CLOSE(MainFrame::OnQuit)
END_EVENT_TABLE()

enum IDs
{
	id_boot_elf = 0x555,
	id_boot_self,
	id_boot_game,
	id_sys_pause,
	id_sys_stop,
	id_config_emu,
};

MainFrame::MainFrame() : FrameBase(NULL, wxID_ANY, "", "MainFrame", wxSize(280, 180))
{
	SetLabel(wxString::Format(_PRGNAME_ " " _PRGVER_ " r%d" SVN_MOD " (" SVN_DATE ")", SVN_REV));
	wxMenuBar& menubar(*new wxMenuBar());

	wxMenu& menu_boot(*new wxMenu());
	wxMenu& menu_sys(*new wxMenu());
	wxMenu& menu_conf(*new wxMenu());

	menubar.Append(&menu_boot, "Boot");
	menubar.Append(&menu_sys, "System");
	menubar.Append(&menu_conf, "Config");

	menu_boot.Append(id_boot_game, "Boot game");
	menu_boot.AppendSeparator();
	menu_boot.Append(id_boot_elf, "Boot Elf");
	//menu_boot.Append(id_boot_self, "Boot Self");

	menu_sys.Append(id_sys_pause, "Pause");
	menu_sys.Append(id_sys_stop, "Stop");

	menu_conf.Append(id_config_emu, "Settings");

	SetMenuBar(&menubar);

	wxBoxSizer& s_panel( *new wxBoxSizer(wxHORIZONTAL) );

	m_game_viewer = new GameViewer(this);
	s_panel.Add( m_game_viewer, wxSizerFlags().Expand() );

	SetSizerAndFit( &s_panel );

	Connect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::OnResize));

	Connect( id_boot_game,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootGame) );
	Connect( id_boot_elf,   wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootElf) );
	Connect( id_boot_self,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::BootSelf) );

	Connect( id_sys_pause,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Pause) );
	Connect( id_sys_stop,   wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Stop) );

	Connect( id_config_emu, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Config) );
	wxGetApp().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainFrame::OnKeyDown), (wxObject*)0, this);

	UpdateUI();

	(new CompilerELF(NULL))->Show();
}

void MainFrame::OnResize(wxSizeEvent& event)
{
	m_game_viewer->DoResize(GetClientSize());
	event.Skip();
}

void MainFrame::BootGame(wxCommandEvent& WXUNUSED(event))
{
	bool stoped = false;

	if(Emu.IsRunned())
	{
		Emu.Pause();
		stoped = true;
	}

	wxDirDialog ctrl( this, L"Select game folder", wxEmptyString);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stoped) Emu.Resume();
		return;
	}

	Emu.Stop();

	const wxString& elf0  = ctrl.GetPath() + "\\PS3_GAME\\USRDIR\\BOOT.BIN";
	const wxString& elf1  = ctrl.GetPath() + "\\USRDIR\\BOOT.BIN";
	const wxString& elf2  = ctrl.GetPath() + "\\BOOT.BIN";
	const wxString& self0 = ctrl.GetPath() + "\\PS3_GAME\\USRDIR\\EBOOT.BIN";
	const wxString& self1 = ctrl.GetPath() + "\\USRDIR\\EBOOT.BIN";
	const wxString& self2 = ctrl.GetPath() + "\\EBOOT.BIN";

	if(wxFile::Access(elf0, wxFile::read))
	{
		Emu.SetElf(elf0);
		ConLog.Write("Elf: booting...");
	}
	else if(wxFile::Access(elf1, wxFile::read))
	{
		Emu.SetElf(elf1);
		ConLog.Write("Elf: booting...");
	}
	else if(wxFile::Access(elf2, wxFile::read))
	{
		Emu.SetElf(elf2);
		ConLog.Write("Elf: booting...");
	}
	else if(wxFile::Access(self0, wxFile::read))
	{
		goto _ELF_NOT_FOUND_;
		Emu.SetSelf(self0);
		ConLog.Warning("Self: booting...");
	}
	else if(wxFile::Access(self1, wxFile::read))
	{
		goto _ELF_NOT_FOUND_;
		Emu.SetSelf(self1);
		ConLog.Warning("Self: booting...");
	}
	else if(wxFile::Access(self2, wxFile::read))
	{
		goto _ELF_NOT_FOUND_;
		Emu.SetSelf(self2);
		ConLog.Warning("Self: booting...");
	}
	else
	{
		ConLog.Error("Not found ps3 game in selected folder! (%s)", ctrl.GetPath());
		return;
	}

	Emu.Run();

	ConLog.Write("Game: boot done.");
	return;

_ELF_NOT_FOUND_:
	ConLog.Error("Elf not found!");
}

void MainFrame::BootElf(wxCommandEvent& WXUNUSED(event))
{
	bool stoped = false;

	if(Emu.IsRunned())
	{
		Emu.Pause();
		stoped = true;
	}

	wxFileDialog ctrl(this, L"Select ELF", wxEmptyString, wxEmptyString, "*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stoped) Emu.Resume();
		return;
	}

	ConLog.Write("Elf: booting...");

	Emu.Stop();

	Emu.SetElf(ctrl.GetPath());
	Emu.Run();

	ConLog.Write("Elf: boot done.");
}

void MainFrame::BootSelf(wxCommandEvent& WXUNUSED(event))
{
	bool stoped = false;

	if(Emu.IsRunned())
	{
		Emu.Pause();
		stoped = true;
	}

	wxFileDialog ctrl(this, L"Select SELF", wxEmptyString, wxEmptyString, "*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		if(stoped) Emu.Resume();
		return;
	}

	ConLog.Write("SELF: booting...");

	Emu.Stop();

	Emu.SetSelf(ctrl.GetPath());
	Emu.Run();

	ConLog.Write("SELF: boot done.");
}

void MainFrame::Pause(wxCommandEvent& WXUNUSED(event))
{
	if(Emu.IsPaused())
	{
		Emu.Resume();
	}
	else if(Emu.IsRunned())
	{
		Emu.Pause();
	}
}

void MainFrame::Stop(wxCommandEvent& WXUNUSED(event))
{
	Emu.Stop();
}

void MainFrame::Config(wxCommandEvent& WXUNUSED(event))
{
	//TODO

	bool paused = false;

	if(Emu.IsRunned())
	{
		Emu.Pause();
		paused = true;
	}

	wxDialog* diag = new wxDialog(this, wxID_ANY, "Settings", wxDefaultPosition);

	wxBoxSizer* s_panel(new wxBoxSizer(wxVERTICAL));

	wxStaticBoxSizer* s_round_cpu( new wxStaticBoxSizer( wxVERTICAL, diag, _("CPU") ) );
	wxStaticBoxSizer* s_round_cpu_decoder( new wxStaticBoxSizer( wxVERTICAL, diag, _("Decoder") ) );

	wxStaticBoxSizer* s_round_gs( new wxStaticBoxSizer( wxHORIZONTAL, diag, _("GS") ) );
	wxStaticBoxSizer* s_round_gs_render( new wxStaticBoxSizer( wxVERTICAL, diag, _("Render") ) );

	wxStaticBoxSizer* s_round_pad( new wxStaticBoxSizer( wxHORIZONTAL, diag, _("Pad") ) );
	wxStaticBoxSizer* s_round_pad_handler( new wxStaticBoxSizer( wxVERTICAL, diag, _("Handler") ) );

	wxComboBox* cbox_cpu_decoder = new wxComboBox(diag, wxID_ANY);
	wxComboBox* cbox_gs_render = new wxComboBox(diag, wxID_ANY);
	wxComboBox* cbox_pad_handler = new wxComboBox(diag, wxID_ANY);

	//cbox_cpu_decoder->Append("DisAsm");
	cbox_cpu_decoder->Append("Interpreter & DisAsm");
	cbox_cpu_decoder->Append("Interpreter");

	cbox_gs_render->Append("Null");
	cbox_gs_render->Append("OpenGL");
	//cbox_gs_render->Append("Software");

	cbox_pad_handler->Append("Null");
	cbox_pad_handler->Append("Windows");
	//cbox_pad_handler->Append("DirectInput");

	cbox_cpu_decoder->SetSelection(Ini.CPUDecoderMode.GetValue() ? Ini.CPUDecoderMode.GetValue() - 1 : 0);
	cbox_gs_render->SetSelection(Ini.GSRenderMode.GetValue());
	cbox_pad_handler->SetSelection(Ini.PadHandlerMode.GetValue());

	s_round_cpu_decoder->Add(cbox_cpu_decoder);
	s_round_cpu->Add(s_round_cpu_decoder);

	s_round_gs_render->Add(cbox_gs_render);
	s_round_gs->Add(s_round_gs_render);

	s_round_pad_handler->Add(cbox_pad_handler);
	s_round_pad->Add(s_round_pad_handler);

	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));

	s_b_panel->Add(new wxButton(diag, wxID_OK));
	s_b_panel->AddSpacer(5);
	s_b_panel->Add(new wxButton(diag, wxID_CANCEL));

	s_panel->Add(s_round_cpu);
	s_panel->AddSpacer(5);
	s_panel->Add(s_round_gs);
	s_panel->AddSpacer(5);
	s_panel->Add(s_round_pad);
	s_panel->AddSpacer(8);
	s_panel->Add(s_b_panel, wxRIGHT);

	diag->SetSizerAndFit( s_panel );
	
	if(diag->ShowModal() == wxID_OK)
	{
		Ini.CPUDecoderMode.SetValue(cbox_cpu_decoder->GetSelection() + 1);
		Ini.GSRenderMode.SetValue(cbox_gs_render->GetSelection());
		Ini.PadHandlerMode.SetValue(cbox_pad_handler->GetSelection());
		Ini.Save();
	}

	delete diag;

	if(paused) Emu.Resume();
}

void MainFrame::OnQuit(wxCloseEvent& event)
{
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
	if(wxGetActiveWindow() == this && event.ControlDown())
	{
		switch(event.GetKeyCode())
		{
		case 'C': case 'c': if(Emu.IsPaused()) Emu.Resume(); return;
		case 'S': case 's': if(!Emu.IsStopped()) Emu.Stop(); return;
		case 'R': case 'r': if(Emu.m_path.Len()) {Emu.Stop(); Emu.Run();} return;
		}
	}

	event.Skip();
}

void MainFrame::UpdateUI()
{
	wxMenuBar& menubar( *GetMenuBar() );
	wxMenuItem& pause = *menubar.FindItem( id_sys_pause );
	wxMenuItem& stop  = *menubar.FindItem( id_sys_stop );

	if(Emu.IsRunned())
	{
		pause.SetText("Pause");
		pause.Enable();
		stop.Enable();
	}
	else
	{
		pause.SetText("Resume");
		pause.Enable(Emu.IsPaused());
		stop.Enable(Emu.IsPaused());
	}
}