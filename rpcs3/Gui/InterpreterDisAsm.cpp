#include "stdafx.h"
#include "InterpreterDisAsm.h"

//static const int show_lines = 30;

u32 InterpreterDisAsmFrame::CentrePc(const u32 pc) const
{
	return pc - ((m_item_count / 2) * 4);
}

InterpreterDisAsmFrame::InterpreterDisAsmFrame(wxWindow* parent, PPCThread* cpu)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(500, 700), wxTAB_TRAVERSAL)
	, ThreadBase(false, "DisAsmFrame Thread")
	, CPU(*cpu)
	, PC(0)
	, m_item_count(30)
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

	m_list = new wxListView(this);

	wxButton& b_go_to_addr = *new wxButton(this, wxID_ANY, "Go To Address");
	wxButton& b_go_to_pc = *new wxButton(this, wxID_ANY, "Go To PC");

	m_btn_step = new wxButton(this, wxID_ANY, "Step");
	m_btn_run = new wxButton(this, wxID_ANY, "Run");
	m_btn_pause = new wxButton(this, wxID_ANY, "Pause");

	s_b_main.Add(&b_go_to_addr,	wxSizerFlags().Border(wxALL, 5));
	s_b_main.Add(&b_go_to_pc,	wxSizerFlags().Border(wxALL, 5));
	s_b_main.Add(m_btn_step,	wxSizerFlags().Border(wxALL, 5));
	s_b_main.Add(m_btn_run,		wxSizerFlags().Border(wxALL, 5));
	s_b_main.Add(m_btn_pause,	wxSizerFlags().Border(wxALL, 5));

	m_regs = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_DONTWRAP|wxNO_BORDER|wxTE_RICH2);
	m_regs->SetMinSize(wxSize(495, 100));
	m_regs->SetEditable(false);

	m_list->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_regs->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	wxBoxSizer& s_w_list = *new wxBoxSizer(wxHORIZONTAL);
	s_w_list.Add(m_list, 2, wxEXPAND | wxLEFT | wxDOWN, 5);
	s_w_list.Add(m_regs, 1, wxEXPAND | wxRIGHT | wxDOWN, 5);

	s_p_main.Add(&s_b_main, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	s_p_main.Add(&s_w_list, 1, wxEXPAND | wxDOWN, 5);

	SetSizer(&s_p_main);
	Layout();

	m_list->InsertColumn(0, "ASM");
	for(uint i=0; i<m_item_count; ++i)
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
	m_app_connector.Connect(m_list->GetId(), wxEVT_MOUSEWHEEL, wxMouseEventHandler(InterpreterDisAsmFrame::MouseWheel), (wxObject*)0, this);
	m_app_connector.Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(InterpreterDisAsmFrame::OnKeyDown), (wxObject*)0, this);

	m_app_connector.Connect(wxEVT_DBG_COMMAND, wxCommandEventHandler(InterpreterDisAsmFrame::HandleCommand), (wxObject*)0, this);
	WriteRegs();
}

InterpreterDisAsmFrame::~InterpreterDisAsmFrame()
{
	ThreadBase::Stop();
}

void InterpreterDisAsmFrame::OnKeyDown(wxKeyEvent& event)
{
	if(wxGetActiveWindow() != wxGetTopLevelParent(this))
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
		case WXK_PAGEUP:	ShowAddr( PC - (m_item_count * 2) * 4 ); return;
		case WXK_PAGEDOWN:	ShowAddr( PC ); return;
		case WXK_UP:		ShowAddr( PC - (m_item_count + 1) * 4 ); return;
		case WXK_DOWN:		ShowAddr( PC - (m_item_count - 1) * 4 ); return;
		}
	}

	event.Skip();
}

void InterpreterDisAsmFrame::OnResize(wxSizeEvent& event)
{
	event.Skip();

	if(0)
	{
		if(!m_list->GetItemCount())
		{
			m_list->InsertItem(m_list->GetItemCount(), wxEmptyString);
		}

		int size = 0;
		m_list->DeleteAllItems();
		int item = 0;
		while(size < m_list->GetSize().GetHeight())
		{
			item = m_list->GetItemCount();
			m_list->InsertItem(item, wxEmptyString);
			wxRect rect;
			m_list->GetItemRect(item, rect);

			size = rect.GetBottom();
		}

		if(item)
		{
			m_list->DeleteItem(--item);
		}

		m_item_count = item;
		ShowAddr(PC);
	}
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
	for(uint i=0; i<m_item_count; ++i, PC += 4)
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

		if((!CPU.IsRunned() || !Emu.IsRunned()) && PC == CPU.PC)
		{
			colour = wxColour("Green");
		}
		else
		{
			colour = wxColour("White");

			for(u32 i=0; i<Emu.GetMarkedPoints().GetCount(); ++i)
			{
				if(Emu.GetMarkedPoints()[i] == PC)
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

		Emu.GetMarkedPoints().RemoveAt(mpc);
	}

	m_list->SetColumnWidth(0, -1);
	m_list->Thaw();
}

void InterpreterDisAsmFrame::WriteRegs()
{
	m_regs->Freeze();
	m_regs->Clear();
	m_regs->WriteText(CPU.RegsToString());
	m_regs->Thaw();
}

void InterpreterDisAsmFrame::HandleCommand(wxCommandEvent& event)
{
	PPCThread* thr = (PPCThread*)event.GetClientData();
	event.Skip();

	if(!thr)
	{
		switch(event.GetId())
		{
		case DID_STOP_EMU:
		case DID_PAUSE_EMU:
			DoUpdate();
		break;
		}
	}
	else if(thr->GetId() == CPU.GetId())
	{
		switch(event.GetId())
		{
		case DID_PAUSE_THREAD:
			m_btn_run->Enable();
			m_btn_step->Enable();
			m_btn_pause->Disable();

		case DID_CREATE_THREAD:
			DoUpdate();
		break;


		case DID_START_THREAD:
		case DID_EXEC_THREAD:
		case DID_RESUME_THREAD:
			m_btn_run->Disable();
			m_btn_step->Disable();
			m_btn_pause->Enable();

			if(event.GetId() == DID_START_THREAD)
			{
				DoUpdate();
			}
		break;

		case DID_REMOVE_THREAD:
		case DID_STOP_THREAD:
			m_btn_run->Disable();
			m_btn_step->Disable();
			m_btn_pause->Disable();
		break;
		}
	}
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
		remove_markedPC.AddCpy(Emu.GetMarkedPoints().AddCpy(pc));
		ShowAddr(CentrePc(pc));
	}
}

void InterpreterDisAsmFrame::Show_PC(wxCommandEvent& WXUNUSED(event))
{
	ShowAddr(CentrePc(CPU.PC));
}

extern bool dump_enable;
void InterpreterDisAsmFrame::DoRun(wxCommandEvent& WXUNUSED(event))
{
	if(CPU.IsPaused()) CPU.Resume();

	if(!Emu.IsPaused())
	{
		CPU.Exec();
	}

	//ThreadBase::Start();
}

void InterpreterDisAsmFrame::DoPause(wxCommandEvent& WXUNUSED(event))
{
	CPU.Pause();
}

void InterpreterDisAsmFrame::DoStep(wxCommandEvent& WXUNUSED(event))
{
	ThreadBase::Start();
}

void InterpreterDisAsmFrame::DClick(wxListEvent& event)
{
	long i = m_list->GetFirstSelected();
	if(i < 0) return;

	const u64 start_pc = PC - m_item_count*4;
	const u64 pc = start_pc + i*4;
	//ConLog.Write("pc=0x%llx", pc);

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

void InterpreterDisAsmFrame::MouseWheel(wxMouseEvent& event)
{
	const int value = (event.m_wheelRotation / event.m_wheelDelta);

	ShowAddr( PC - (event.ControlDown() ? m_item_count * (value + 1) : m_item_count + value) * 4);

	event.Skip();
}

bool InterpreterDisAsmFrame::IsBreakPoint(u64 pc)
{
	for(u32 i=0; i<Emu.GetBreakPoints().GetCount(); ++i)
	{
		if(Emu.GetBreakPoints()[i] == pc) return true;
	}

	return false;
}

void InterpreterDisAsmFrame::AddBreakPoint(u64 pc)
{
	for(u32 i=0; i<Emu.GetBreakPoints().GetCount(); ++i)
	{
		if(Emu.GetBreakPoints()[i] == pc) return;
	}

	Emu.GetBreakPoints().AddCpy(pc);
}

bool InterpreterDisAsmFrame::RemoveBreakPoint(u64 pc)
{
	for(u32 i=0; i<Emu.GetBreakPoints().GetCount(); ++i)
	{
		if(Emu.GetBreakPoints()[i] != pc) continue;
		Emu.GetBreakPoints().RemoveAt(i);
		return true;
	}

	return false;
}

void InterpreterDisAsmFrame::Task()
{
	wxGetApp().SendDbgCommand(DID_RESUME_THREAD, &CPU);

	bool dump_status = dump_enable;

	//CPU.InitTls();

	try
	{
		do
		{
			CPU.ExecOnce();
		}
		while(CPU.IsRunned() && Emu.IsRunned() && !TestDestroy() && !IsBreakPoint(CPU.PC) && dump_status == dump_enable);
	}
	catch(const wxString& e)
	{
		ConLog.Error(e);
	}
	catch(...)
	{
		ConLog.Error("Unhandled exception.");
	}

	//CPU.FreeTls();

	wxGetApp().SendDbgCommand(DID_PAUSE_THREAD, &CPU);
}