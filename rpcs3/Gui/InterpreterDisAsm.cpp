#include "stdafx.h"
#include "InterpreterDisAsm.h"

static const int show_lines = 30;
static const wxString& BreakPointsDBName = "BreakPoints.dat";
static const u16 bpdb_version = 0x1000;

u32 FixPc(const u32 pc)
{
	return pc - ((show_lines/2)*4);
}

InterpreterDisAsmFrame::InterpreterDisAsmFrame(const wxString& title, PPCThread* cpu)
	: FrameBase(NULL, wxID_ANY, title, "InterpreterDisAsmFrame", wxSize(500, 700))
	, ThreadBase(false, "DisAsmFrame Thread")
	, m_main_panel(*new wxPanel(this))
	, CPU(*cpu)
	, PC(0)
	, m_exec(false)
{
	if(CPU.IsSPU())
	{
		SPU_DisAsm& dis_asm = *new SPU_DisAsm(CPU, InterpreterMode);
		decoder = new SPU_Decoder(dis_asm);
		disasm = &dis_asm;
	}
	else
	{
		PPU_DisAsm& dis_asm = *new PPU_DisAsm(CPU, InterpreterMode);
		decoder = new PPU_Decoder(dis_asm);
		disasm = &dis_asm;
	}

	wxBoxSizer& s_p_main = *new wxBoxSizer(wxVERTICAL);
	wxBoxSizer& s_b_main = *new wxBoxSizer(wxHORIZONTAL);

	m_list = new wxListView(&m_main_panel);

	wxButton& b_go_to_addr = *new wxButton(&m_main_panel, wxID_ANY, "Go To Address");
	wxButton& b_go_to_pc = *new wxButton(&m_main_panel, wxID_ANY, "Go To PC");

	m_btn_step = new wxButton(&m_main_panel, wxID_ANY, "Step");
	m_btn_run = new wxButton(&m_main_panel, wxID_ANY, "Run");
	m_btn_pause = new wxButton(&m_main_panel, wxID_ANY, "Pause");

	s_b_main.Add(&b_go_to_addr,	wxSizerFlags().Border(wxALL, 5));
	s_b_main.Add(&b_go_to_pc,	wxSizerFlags().Border(wxALL, 5));
	s_b_main.Add(m_btn_step,	wxSizerFlags().Border(wxALL, 5));
	s_b_main.Add(m_btn_run,		wxSizerFlags().Border(wxALL, 5));
	s_b_main.Add(m_btn_pause,	wxSizerFlags().Border(wxALL, 5));

	m_regs = new wxTextCtrl(&m_main_panel, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_DONTWRAP|wxNO_BORDER|wxTE_RICH2);
	m_regs->SetMinSize(wxSize(495, 100));
	m_regs->SetEditable(false);

	m_list->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_regs->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	wxBoxSizer& s_w_list = *new wxBoxSizer(wxHORIZONTAL);
	s_w_list.Add(m_list);
	s_w_list.AddSpacer(5);
	s_w_list.Add(m_regs);

	s_p_main.AddSpacer(5);
	s_p_main.Add(&s_b_main);
	s_p_main.AddSpacer(5);
	s_p_main.Add(&s_w_list);
	s_p_main.AddSpacer(5);

	wxBoxSizer& s_p_main_h = *new wxBoxSizer(wxVERTICAL);
	s_p_main_h.AddSpacer(5);
	s_p_main_h.Add(&s_p_main);
	s_p_main_h.AddSpacer(5);
	m_main_panel.SetSizerAndFit(&s_p_main_h);

	wxBoxSizer& s_main = *new wxBoxSizer(wxVERTICAL);
	s_main.Add(&m_main_panel);
	SetSizerAndFit(&s_main);

	m_list->InsertColumn(0, "ASM");

	for(uint i=0; i<show_lines; ++i)
	{
		m_list->InsertItem(m_list->GetItemCount(), wxEmptyString);
	}

	Connect(m_regs->GetId(),		wxEVT_COMMAND_TEXT_UPDATED,		wxCommandEventHandler(InterpreterDisAsmFrame::OnUpdate));
	Connect(b_go_to_addr.GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(InterpreterDisAsmFrame::Show_Val));
	Connect(b_go_to_pc.GetId(),		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(InterpreterDisAsmFrame::Show_PC));
	Connect(m_btn_step->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(InterpreterDisAsmFrame::DoStep));
	Connect(m_btn_run->GetId(),		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(InterpreterDisAsmFrame::DoRun));
	Connect(m_btn_pause->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(InterpreterDisAsmFrame::DoPause));
	Connect(m_list->GetId(),		wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler(InterpreterDisAsmFrame::DClick));
	Connect(wxEVT_SIZE, wxSizeEventHandler(InterpreterDisAsmFrame::OnResize));
	wxGetApp().Connect(m_list->GetId(), wxEVT_MOUSEWHEEL, wxMouseEventHandler(InterpreterDisAsmFrame::MouseWheel), (wxObject*)0, this);
	wxGetApp().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(InterpreterDisAsmFrame::OnKeyDown), (wxObject*)0, this);

	m_btn_pause->Disable();
	//ShowPc(Loader.entry);
	WriteRegs();
	ThreadBase::Start();

	Load(BreakPointsDBName);
}

InterpreterDisAsmFrame::~InterpreterDisAsmFrame()
{
	ThreadBase::Stop();
	Save(BreakPointsDBName);
}

void InterpreterDisAsmFrame::Save(const wxString& path)
{
	wxFile f(path, wxFile::write);
	u32 break_count = m_break_points.GetCount();
	u32 marked_count = markedPC.GetCount();
	f.Write(&bpdb_version, sizeof(u16));
	f.Write(&break_count, sizeof(u32));
	f.Write(&marked_count, sizeof(u32));

	for(u32 i=0; i<break_count; ++i)
	{
		f.Write(&m_break_points[i], sizeof(u64));
	}

	for(u32 i=0; i<marked_count; ++i)
	{
		f.Write(&markedPC[i], sizeof(u64));
	}
}

void InterpreterDisAsmFrame::Load(const wxString& path)
{
	if(!wxFileExists(path)) return;

	wxFile f(path);
	u32 break_count, marked_count;
	u16 version;
	f.Read(&version, sizeof(u16));
	f.Read(&break_count, sizeof(u32));
	f.Read(&marked_count, sizeof(u32));

	if(version != bpdb_version ||
		(sizeof(u16) + break_count * sizeof(u64) + sizeof(u32) + marked_count * sizeof(u64) + sizeof(u32)) != f.Length())
	{
		ConLog.Error("'%s' is borken", path);
		return;
	}

	if(break_count > 0)
	{
		m_break_points.SetCount(break_count);

		for(u32 i=0; i<break_count; ++i)
		{
			f.Read(&m_break_points[i], sizeof(u64));
		}
	}

	if(marked_count > 0)
	{
		markedPC.SetCount(marked_count);

		for(u32 i=0; i<marked_count; ++i)
		{
			f.Read(&markedPC[i], sizeof(u64));
		}
	}
}

void InterpreterDisAsmFrame::OnKeyDown(wxKeyEvent& event)
{
	if(wxGetActiveWindow() != this)
	{
		event.Skip();
		return;
	}

	if(event.ControlDown())
	{
		if(event.GetKeyCode() == WXK_SPACE)
		{
			DoStep(wxCommandEvent());
			return;
		}
	}
	else
	{
		switch(event.GetKeyCode())
		{
		case WXK_PAGEUP:	ShowAddr( PC - (show_lines * 2) * 4 ); return;
		case WXK_PAGEDOWN:	ShowAddr( PC ); return;
		case WXK_UP:		ShowAddr( PC - (show_lines + 1) * 4 ); return;
		case WXK_DOWN:		ShowAddr( PC - (show_lines - 1) * 4 ); return;
		}
	}

	event.Skip();
}

void InterpreterDisAsmFrame::DoUpdate()
{
	Show_PC(wxCommandEvent());
	WriteRegs();
}

void InterpreterDisAsmFrame::ShowAddr(const u64 addr)
{
	PC = addr;
	m_list->Freeze();
	for(uint i=0; i<show_lines; ++i, PC += 4)
	{
		if(!Memory.IsGoodAddr(PC, 4))
		{
			m_list->SetItem(i, 0, wxString::Format("[%08llx] illegal address", PC));
			continue;
		}

		disasm->dump_pc = PC;
		decoder->Decode(Memory.Read32(PC));

		if(IsBreakPoint(PC))
		{
			m_list->SetItem(i, 0, ">>> " + disasm->last_opcode);
		}
		else
		{
			m_list->SetItem(i, 0, "    " + disasm->last_opcode);
		}

		wxColour colour;

		if(PC == CPU.PC)
		{
			colour = wxColour("Green");
		}
		else
		{
			colour = wxColour("White");

			for(u32 i=0; i<markedPC.GetCount(); ++i)
			{
				if(markedPC[i] == PC)
				{
					colour = wxColour("Wheat");
					break;
				}
			}
		}

		m_list->SetItemBackgroundColour( i, colour );
	}


	while(remove_markedPC.GetCount())
	{
		u32 mpc = remove_markedPC[0];

		for(u32 i=0; i<remove_markedPC.GetCount(); ++i)
		{
			if(remove_markedPC[i] == mpc)
			{
				remove_markedPC.RemoveAt(i--);
				continue;
			}

			if(remove_markedPC[i] > mpc) remove_markedPC[i]--;
		}

		markedPC.RemoveAt(mpc);
	}

	m_list->Thaw();
}

void InterpreterDisAsmFrame::WriteRegs()
{
	m_regs->Freeze();
	m_regs->Clear();
	m_regs->WriteText(CPU.RegsToString());
	m_regs->Thaw();
}

void InterpreterDisAsmFrame::OnUpdate(wxCommandEvent& event)
{
	//WriteRegs();
}

void InterpreterDisAsmFrame::Show_Val(wxCommandEvent& WXUNUSED(event))
{
	wxDialog* diag = new wxDialog(this, wxID_ANY, "Set value", wxDefaultPosition);

	wxBoxSizer* s_panel(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));
	wxTextCtrl* p_pc(new wxTextCtrl(diag, wxID_ANY));

	s_panel->Add(p_pc);
	s_panel->AddSpacer(8);
	s_panel->Add(s_b_panel);

	s_b_panel->Add(new wxButton(diag, wxID_OK), wxLEFT, 0, 5);
	s_b_panel->AddSpacer(5);
	s_b_panel->Add(new wxButton(diag, wxID_CANCEL), wxRIGHT, 0, 5);

	diag->SetSizerAndFit( s_panel );

	p_pc->SetLabel(wxString::Format("%llx", CPU.PC));

	if(diag->ShowModal() == wxID_OK)
	{
		u64 pc = CPU.PC;
		sscanf(p_pc->GetLabel(), "%llx", &pc);
		remove_markedPC.AddCpy(markedPC.AddCpy(pc));
		ShowAddr(FixPc(pc));
	}
}

void InterpreterDisAsmFrame::Show_PC(wxCommandEvent& WXUNUSED(event))
{
	ShowAddr(FixPc(CPU.PC));
}

extern bool dump_enable;
void InterpreterDisAsmFrame::DoRun(wxCommandEvent& WXUNUSED(event))
{
	m_exec = true;
	/*
	bool dump_status = dump_enable;
	if(Emu.IsPaused()) Emu.Run();
	while(Emu.IsRunned())
	{
		CPU.Exec();
		if(IsBreakPoint(CPU.PC) || dump_status != dump_enable) break;
	}

	DoUpdate();
	*/
}

void InterpreterDisAsmFrame::DoPause(wxCommandEvent& WXUNUSED(event))
{
	Emu.Pause();
}

void InterpreterDisAsmFrame::DoStep(wxCommandEvent& WXUNUSED(event))
{
	CPU.Exec();
	DoUpdate();
}

void InterpreterDisAsmFrame::DClick(wxListEvent& event)
{
	long i = m_list->GetFirstSelected();
	if(i < 0) return;

	const u64 start_pc = PC - show_lines*4;
	const u64 pc = start_pc + i*4;
	//ConLog.Write("pc=0x%x", pc);

	if(!Memory.IsGoodAddr(pc, 4)) return;

	if(IsBreakPoint(pc))
	{
		RemoveBreakPoint(pc);
	}
	else
	{
		AddBreakPoint(pc);
	}

	ShowAddr(start_pc);
}
	
void InterpreterDisAsmFrame::OnResize(wxSizeEvent& event)
{
	const wxSize& size( GetClientSize() );

	wxTextCtrl& regs = *m_regs;
	wxListView& list = *m_list;

	const int list_size = size.GetWidth() * 0.75;
	const int regs_size = size.GetWidth() * 0.25 - 5;
	list.SetMinSize(wxSize(list_size,  size.GetHeight()-55));
	regs.SetMinSize(wxSize(regs_size,  size.GetHeight()-55));
	m_main_panel.SetSize( size );
	m_main_panel.GetSizer()->RecalcSizes();

	list.SetColumnWidth(0, list_size-8);

	event.Skip();
}

void InterpreterDisAsmFrame::MouseWheel(wxMouseEvent& event)
{
	const int value = (event.m_wheelRotation / event.m_wheelDelta);
	if(event.ControlDown())
	{
		ShowAddr( PC - (show_lines * (value + 1)) * 4);
	}
	else
	{
		ShowAddr( PC - (show_lines + value) * 4 );
	}

	event.Skip();
}

bool InterpreterDisAsmFrame::IsBreakPoint(u64 pc)
{
	for(u32 i=0; i<m_break_points.GetCount(); ++i)
	{
		if(m_break_points[i] == pc) return true;
	}

	return false;
}

void InterpreterDisAsmFrame::AddBreakPoint(u64 pc)
{
	for(u32 i=0; i<m_break_points.GetCount(); ++i)
	{
		if(m_break_points[i] == pc) return;
	}

	m_break_points.AddCpy(pc);
}

bool InterpreterDisAsmFrame::RemoveBreakPoint(u64 pc)
{
	for(u32 i=0; i<m_break_points.GetCount(); ++i)
	{
		if(m_break_points[i] != pc) continue;
		m_break_points.RemoveAt(i);
		return true;
	}

	return false;
}

void InterpreterDisAsmFrame::Task()
{
	while(!TestDestroy())
	{
		Sleep(1);

		if(!m_exec) continue;

		m_btn_step->Disable();
		m_btn_run->Disable();
		m_btn_pause->Enable();

		bool dump_status = dump_enable;
		if(Emu.IsPaused()) Emu.Resume();

		while(Emu.IsRunned() && !TestDestroy())
		{
			CPU.Exec();
			if(IsBreakPoint(CPU.PC) || dump_status != dump_enable) break;
		}

		DoUpdate();

		m_btn_step->Enable();
		m_btn_run->Enable();
		m_btn_pause->Disable();

		m_exec = false;
	}
}