#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "rpcs3.h"
#include "InterpreterDisAsm.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/ARMv7/ARMv7DisAsm.h"
#include "Emu/ARMv7/ARMv7Decoder.h"

#include "InstructionEditor.h"
#include "RegisterEditor.h"

//static const int show_lines = 30;

u64 InterpreterDisAsmFrame::CentrePc(const u64 pc) const
{
	return pc/* - ((m_item_count / 2) * 4)*/;
}

InterpreterDisAsmFrame::InterpreterDisAsmFrame(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(500, 700), wxTAB_TRAVERSAL)
	, PC(0)
	, CPU(nullptr)
	, m_item_count(30)
	, disasm(nullptr)
{
	wxBoxSizer* s_p_main = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_b_main = new wxBoxSizer(wxHORIZONTAL);

	m_list = new wxListView(this);
	m_choice_units = new wxChoice(this, wxID_ANY);

	wxButton* b_go_to_addr = new wxButton(this, wxID_ANY, "Go To Address");
	wxButton* b_go_to_pc = new wxButton(this, wxID_ANY, "Go To PC");

	m_btn_step  = new wxButton(this, wxID_ANY, "Step");
	m_btn_run   = new wxButton(this, wxID_ANY, "Run");
	m_btn_pause = new wxButton(this, wxID_ANY, "Pause");

	s_b_main->Add(b_go_to_addr,   wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(b_go_to_pc,     wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(m_btn_step,     wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(m_btn_run,      wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(m_btn_pause,    wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(m_choice_units, wxSizerFlags().Border(wxALL, 5));

	//Registers
	m_regs = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_DONTWRAP|wxNO_BORDER|wxTE_RICH2);
	m_regs->SetEditable(false);

	//Call Stack
	m_calls = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_DONTWRAP|wxNO_BORDER|wxTE_RICH2);
	m_calls->SetEditable(false);

	m_list ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_regs ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_calls->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	wxBoxSizer* s_w_list = new wxBoxSizer(wxHORIZONTAL);
	s_w_list->Add(m_list, 2, wxEXPAND | wxLEFT | wxDOWN, 5);
	s_w_list->Add(m_regs, 1, wxEXPAND | wxRIGHT | wxDOWN, 5);
	s_w_list->Add(m_calls,1, wxEXPAND | wxRIGHT | wxDOWN, 5);

	s_p_main->Add(s_b_main, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	s_p_main->Add(s_w_list, 1, wxEXPAND | wxDOWN, 5);

	SetSizer(s_p_main);
	Layout();

	m_list->InsertColumn(0, "ASM");
	for(uint i=0; i<m_item_count; ++i)
	{
		m_list->InsertItem(m_list->GetItemCount(), wxEmptyString);
	}

	m_regs        ->Bind(wxEVT_TEXT,                &InterpreterDisAsmFrame::OnUpdate, this);
	b_go_to_addr  ->Bind(wxEVT_BUTTON,              &InterpreterDisAsmFrame::Show_Val, this);
	b_go_to_pc    ->Bind(wxEVT_BUTTON,              &InterpreterDisAsmFrame::Show_PC, this);
	m_btn_step    ->Bind(wxEVT_BUTTON,              &InterpreterDisAsmFrame::DoStep, this);
	m_btn_run     ->Bind(wxEVT_BUTTON,              &InterpreterDisAsmFrame::DoRun, this);
	m_btn_pause   ->Bind(wxEVT_BUTTON,              &InterpreterDisAsmFrame::DoPause, this);
	m_list        ->Bind(wxEVT_LIST_KEY_DOWN,       &InterpreterDisAsmFrame::InstrKey, this);
	m_list        ->Bind(wxEVT_LIST_ITEM_ACTIVATED, &InterpreterDisAsmFrame::DClick, this);
	m_list        ->Bind(wxEVT_MOUSEWHEEL,          &InterpreterDisAsmFrame::MouseWheel, this);
	m_choice_units->Bind(wxEVT_CHOICE,              &InterpreterDisAsmFrame::OnSelectUnit, this);

	Bind(wxEVT_SIZE, &InterpreterDisAsmFrame::OnResize, this);
	Bind(wxEVT_KEY_DOWN, &InterpreterDisAsmFrame::OnKeyDown, this);
	wxGetApp().Bind(wxEVT_DBG_COMMAND, &InterpreterDisAsmFrame::HandleCommand, this);

	ShowAddr(CentrePc(PC));
	UpdateUnitList();
}

InterpreterDisAsmFrame::~InterpreterDisAsmFrame()
{
}

void InterpreterDisAsmFrame::UpdateUnitList()
{
	m_choice_units->Freeze();
	m_choice_units->Clear();

	for (auto& t : Emu.GetCPU().GetAllThreads())
	{
		m_choice_units->Append(t->GetFName(), t.get());
	}

	m_choice_units->Thaw();
}

void InterpreterDisAsmFrame::OnSelectUnit(wxCommandEvent& event)
{
	CPU = (CPUThread*)event.GetClientData();

	decoder.reset();
	disasm = nullptr;

	class SPUDecoder : public CPUDecoder
	{
		std::unique_ptr<SPUDisAsm> disasm;

	public:
		SPUDecoder(SPUDisAsm* disasm)
			: disasm(disasm)
		{
		}

		virtual u32 DecodeMemory(const u32 address) override
		{
			disasm->do_disasm(vm::ps3::read32(address));

			return 4;
		}
	};

	if(CPU)
	{
		switch(CPU->get_type())
		{
		case CPU_THREAD_PPU:
		{
			PPUDisAsm* dis_asm = new PPUDisAsm(CPUDisAsm_InterpreterMode);
			decoder = std::make_unique<PPUDecoder>(dis_asm);
			disasm = dis_asm;
		}
		break;

		case CPU_THREAD_SPU:
		case CPU_THREAD_RAW_SPU:
		{
			SPUDisAsm* dis_asm = new SPUDisAsm(CPUDisAsm_InterpreterMode);
			decoder = std::make_unique<SPUDecoder>(dis_asm);
			disasm = dis_asm;
		}
		break;

		case CPU_THREAD_ARMv7:
		{
			//ARMv7DisAsm& dis_asm = *new ARMv7DisAsm(CPUDisAsm_InterpreterMode);
			//decoder = new ARMv7Decoder(dis_asm);
			//disasm = &dis_asm;
		}
		break;
		}
	}

	DoUpdate();
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
			wxCommandEvent ce;
			DoStep(ce);
			return;
		}
	}
	else
	{
		switch(event.GetKeyCode())
		{
		case WXK_PAGEUP:   ShowAddr( PC - (m_item_count * 2) * 4 ); return;
		case WXK_PAGEDOWN: ShowAddr( PC ); return;
		case WXK_UP:       ShowAddr( PC - (m_item_count + 1) * 4 ); return;
		case WXK_DOWN:     ShowAddr( PC - (m_item_count - 1) * 4 ); return;
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
	wxCommandEvent ce;
	Show_PC(ce);
	WriteRegs();
	WriteCallStack();
}

void InterpreterDisAsmFrame::ShowAddr(const u64 addr)
{
	PC = addr;
	m_list->Freeze();

	if(!CPU)
	{
		for(uint i=0; i<m_item_count; ++i, PC += 4)
		{
			m_list->SetItem(i, 0, wxString::Format("[%08llx] illegal address", PC));
		}
	}
	else
	{
		disasm->offset = (u8*)vm::base(CPU->get_offset());
		for(uint i=0, count = 4; i<m_item_count; ++i, PC += count)
		{
			if(!vm::check_addr(CPU->get_offset() + PC, 4))
			{
				m_list->SetItem(i, 0, wxString(IsBreakPoint(PC) ? ">>> " : "    ") + wxString::Format("[%08llx] illegal address", PC));
				count = 4;
				continue;
			}

			disasm->dump_pc = PC;
			count = decoder->DecodeMemory(CPU->get_offset() + PC);

			if(IsBreakPoint(PC))
			{
				m_list->SetItem(i, 0, fmt::FromUTF8(">>> " + disasm->last_opcode));
			}
			else
			{
				m_list->SetItem(i, 0, fmt::FromUTF8("    " + disasm->last_opcode));
			}

			wxColour colour;

			if(CPU->is_paused() && PC == CPU->get_pc())
			{
				colour = wxColour("Green");
			}
			else
			{
				colour = wxColour("White");

				for(u32 i=0; i<Emu.GetMarkedPoints().size(); ++i)
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
	}

	while(remove_markedPC.size())
	{
		u32 mpc = remove_markedPC[0];

		for(u32 i=0; i<remove_markedPC.size(); ++i)
		{
			if(remove_markedPC[i] == mpc)
			{
				remove_markedPC.erase( remove_markedPC.begin() + i--);
				continue;
			}

			if(remove_markedPC[i] > mpc) remove_markedPC[i]--;
		}

		Emu.GetMarkedPoints().erase(Emu.GetMarkedPoints().begin() + mpc);
	}

	m_list->SetColumnWidth(0, -1);
	m_list->Thaw();
}

void InterpreterDisAsmFrame::WriteRegs()
{
	if(!CPU)
	{
		m_regs->Clear();
		return;
	}

	const std::string data = CPU->RegsToString();

	m_regs->Freeze();
	m_regs->Clear();
	m_regs->WriteText(fmt::FromUTF8(data));
	m_regs->Thaw();
}

void InterpreterDisAsmFrame::WriteCallStack()
{
	if(!CPU)
	{
		m_calls->Clear();
		return;
	}

	m_calls->Freeze();
	m_calls->Clear();
	//m_calls->WriteText(fmt::FromUTF8(data));
	m_calls->Thaw();
}

void InterpreterDisAsmFrame::HandleCommand(wxCommandEvent& event)
{
	CPUThread* thr = (CPUThread*)event.GetClientData();
	event.Skip();

	if(!thr)
	{
		switch(event.GetId())
		{
		case DID_STOPPED_EMU:
			UpdateUnitList();
		break;

		case DID_PAUSED_EMU:
			//DoUpdate();
		break;
		}
	}
	else if(CPU && thr->get_id() == CPU->get_id())
	{
		switch(event.GetId())
		{
		case DID_PAUSE_THREAD:
			m_btn_run->Disable();
			m_btn_step->Disable();
			m_btn_pause->Disable();
		break;

		case DID_PAUSED_THREAD:
			m_btn_run->Enable();
			m_btn_step->Enable();
			m_btn_pause->Disable();
			DoUpdate();
		break;

		case DID_START_THREAD:
		case DID_EXEC_THREAD:
		case DID_RESUME_THREAD:
			m_btn_run->Disable();
			m_btn_step->Disable();
			m_btn_pause->Enable();
		break;

		case DID_REMOVE_THREAD:
		case DID_STOP_THREAD:
			m_btn_run->Disable();
			m_btn_step->Disable();
			m_btn_pause->Disable();

			if(event.GetId() == DID_REMOVE_THREAD)
			{
				//m_choice_units->SetSelection(-1);
				//wxCommandEvent event;
				//event.SetInt(-1);
				//event.SetClientData(nullptr);
				//OnSelectUnit(event);
				UpdateUnitList();
				//DoUpdate();
			}
		break;
		}
	}
	else
	{
		switch(event.GetId())
		{
		case DID_CREATE_THREAD:
			UpdateUnitList();
			if(m_choice_units->GetSelection() == -1)
			{
				//m_choice_units->SetSelection(0);
				//wxCommandEvent event;
				//event.SetInt(0);
				//event.SetClientData(&Emu.GetCPU().GetThreads()[0]);
				//OnSelectUnit(event);
			}
		break;

		case DID_REMOVED_THREAD:
			UpdateUnitList();
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

	if(CPU) p_pc->SetValue(wxString::Format("%x", CPU->get_pc()));

	if(diag->ShowModal() == wxID_OK)
	{
		unsigned long pc = CPU ? CPU->get_pc() : 0x0;
		p_pc->GetValue().ToULong(&pc, 16);
		Emu.GetMarkedPoints().push_back(pc);
		remove_markedPC.push_back(Emu.GetMarkedPoints().size()-1);
		ShowAddr(CentrePc(pc));
	}
}

void InterpreterDisAsmFrame::Show_PC(wxCommandEvent& WXUNUSED(event))
{
	if(CPU) ShowAddr(CentrePc(CPU->get_pc()));
}

void InterpreterDisAsmFrame::DoRun(wxCommandEvent& WXUNUSED(event))
{
	if(!CPU) return;

	if(CPU->is_paused()) CPU->resume();

	if(!Emu.IsPaused())
	{
		CPU->exec();
	}

	//ThreadBase::Start();
}

void InterpreterDisAsmFrame::DoPause(wxCommandEvent& WXUNUSED(event))
{
	//DoUpdate();
	if(CPU) CPU->pause();
}

void InterpreterDisAsmFrame::DoStep(wxCommandEvent& WXUNUSED(event))
{
	if(CPU) CPU->step();
}

void InterpreterDisAsmFrame::InstrKey(wxListEvent& event)
{
	long i = m_list->GetFirstSelected();
	if(i < 0 || !CPU)
	{
		event.Skip();
		return;
	}

	const u64 start_pc = PC - m_item_count*4;
	const u64 pc = start_pc + i*4;

	switch(event.GetKeyCode())
	{
	case 'E':
		// TODO:: Syphurith: It is said the InstructionEditorDialog would be immediately destroyed.
		InstructionEditorDialog(this, pc, CPU, decoder.get(), disasm);
		DoUpdate();
		return;
	case 'R':
		// TODO:: Syphurith: Eh Similiar for this one.
		RegisterEditorDialog(this, pc, CPU, decoder.get(), disasm);
		DoUpdate();
		return;
	}

	event.Skip();
}

void InterpreterDisAsmFrame::DClick(wxListEvent& event)
{
	long i = m_list->GetFirstSelected();
	if(i < 0) return;

	const u64 start_pc = PC - m_item_count*4;
	const u64 pc = start_pc + i*4;
	//ConLog.Write("pc=0x%llx", pc);

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
	for(u32 i=0; i<Emu.GetBreakPoints().size(); ++i)
	{
		if(Emu.GetBreakPoints()[i] == pc) return true;
	}

	return false;
}

void InterpreterDisAsmFrame::AddBreakPoint(u64 pc)
{
	for(u32 i=0; i<Emu.GetBreakPoints().size(); ++i)
	{
		if(Emu.GetBreakPoints()[i] == pc) return;
	}

	Emu.GetBreakPoints().push_back(pc);
}

bool InterpreterDisAsmFrame::RemoveBreakPoint(u64 pc)
{
	for(u32 i=0; i<Emu.GetBreakPoints().size(); ++i)
	{
		if(Emu.GetBreakPoints()[i] != pc) continue;
		Emu.GetBreakPoints().erase(Emu.GetBreakPoints().begin() + i);
		return true;
	}

	return false;
}
