#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "rpcs3.h"
#include "InterpreterDisAsm.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/PSP2/ARMv7Thread.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/PSP2/ARMv7DisAsm.h"

#include "InstructionEditor.h"
#include "RegisterEditor.h"

//static const int show_lines = 30;
#include <map>
std::map<u32, bool> g_breakpoints;

u32 InterpreterDisAsmFrame::GetPc() const
{
	switch (cpu->type)
	{
	case cpu_type::ppu: return static_cast<PPUThread*>(cpu)->pc;
	case cpu_type::spu: return static_cast<SPUThread*>(cpu)->pc;
	case cpu_type::arm: return static_cast<ARMv7Thread*>(cpu)->PC;
	}
	
	return 0xabadcafe;
}

u32 InterpreterDisAsmFrame::CentrePc(u32 pc) const
{
	return pc/* - ((m_item_count / 2) * 4)*/;
}

InterpreterDisAsmFrame::InterpreterDisAsmFrame(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(500, 700), wxTAB_TRAVERSAL)
	, m_pc(0)
	, cpu(nullptr)
	, m_item_count(30)
{
	wxBoxSizer* s_p_main = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_b_main = new wxBoxSizer(wxHORIZONTAL);

	m_list = new wxListView(this);
	m_choice_units = new wxChoice(this, wxID_ANY);

	wxButton* b_go_to_addr = new wxButton(this, wxID_ANY, "Go To Address");
	wxButton* b_go_to_pc = new wxButton(this, wxID_ANY, "Go To PC");

	m_btn_step = new wxButton(this, wxID_ANY, "Step");
	m_btn_run = new wxButton(this, wxID_ANY, "Run");
	m_btn_pause = new wxButton(this, wxID_ANY, "Pause");

	s_b_main->Add(b_go_to_addr, wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(b_go_to_pc, wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(m_btn_step, wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(m_btn_run, wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(m_btn_pause, wxSizerFlags().Border(wxALL, 5));
	s_b_main->Add(m_choice_units, wxSizerFlags().Border(wxALL, 5));

	//Registers
	m_regs = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_DONTWRAP | wxNO_BORDER | wxTE_RICH2);
	m_regs->SetEditable(false);

	//Call Stack
	m_calls = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_DONTWRAP | wxNO_BORDER | wxTE_RICH2);
	m_calls->SetEditable(false);

	m_list->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_regs->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_calls->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	wxBoxSizer* s_w_list = new wxBoxSizer(wxHORIZONTAL);
	s_w_list->Add(m_list, 2, wxEXPAND | wxLEFT | wxDOWN, 5);
	s_w_list->Add(m_regs, 1, wxEXPAND | wxRIGHT | wxDOWN, 5);
	s_w_list->Add(m_calls, 1, wxEXPAND | wxRIGHT | wxDOWN, 5);

	s_p_main->Add(s_b_main, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	s_p_main->Add(s_w_list, 1, wxEXPAND | wxDOWN, 5);

	SetSizer(s_p_main);
	Layout();

	m_list->InsertColumn(0, "ASM");
	for (uint i = 0; i<m_item_count; ++i)
	{
		m_list->InsertItem(m_list->GetItemCount(), wxEmptyString);
	}

	m_regs->Bind(wxEVT_TEXT, &InterpreterDisAsmFrame::OnUpdate, this);
	b_go_to_addr->Bind(wxEVT_BUTTON, &InterpreterDisAsmFrame::Show_Val, this);
	b_go_to_pc->Bind(wxEVT_BUTTON, &InterpreterDisAsmFrame::Show_PC, this);
	m_btn_step->Bind(wxEVT_BUTTON, &InterpreterDisAsmFrame::DoStep, this);
	m_btn_run->Bind(wxEVT_BUTTON, &InterpreterDisAsmFrame::DoRun, this);
	m_btn_pause->Bind(wxEVT_BUTTON, &InterpreterDisAsmFrame::DoPause, this);
	m_list->Bind(wxEVT_LIST_KEY_DOWN, &InterpreterDisAsmFrame::InstrKey, this);
	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &InterpreterDisAsmFrame::DClick, this);
	m_list->Bind(wxEVT_MOUSEWHEEL, &InterpreterDisAsmFrame::MouseWheel, this);
	m_choice_units->Bind(wxEVT_CHOICE, &InterpreterDisAsmFrame::OnSelectUnit, this);

	Bind(wxEVT_SIZE, &InterpreterDisAsmFrame::OnResize, this);
	Bind(wxEVT_KEY_DOWN, &InterpreterDisAsmFrame::OnKeyDown, this);
	wxGetApp().Bind(wxEVT_DBG_COMMAND, &InterpreterDisAsmFrame::HandleCommand, this);

	ShowAddr(CentrePc(m_pc));
	UpdateUnitList();
}

InterpreterDisAsmFrame::~InterpreterDisAsmFrame()
{
}

void InterpreterDisAsmFrame::UpdateUnitList()
{
	m_choice_units->Freeze();
	m_choice_units->Clear();

	idm::select<PPUThread, SPUThread, RawSPUThread, ARMv7Thread>([&](u32, cpu_thread& cpu)
	{
		m_choice_units->Append(cpu.get_name(), &cpu);
	});

	m_choice_units->Thaw();
}

void InterpreterDisAsmFrame::OnSelectUnit(wxCommandEvent& event)
{
	m_disasm.reset();

	if (cpu = (cpu_thread*)event.GetClientData())
	{
		switch (cpu->type)
		{
		case cpu_type::ppu:
		{
			m_disasm = std::make_unique<PPUDisAsm>(CPUDisAsm_InterpreterMode);
			break;
		}

		case cpu_type::spu:
		{
			m_disasm = std::make_unique<SPUDisAsm>(CPUDisAsm_InterpreterMode);
			break;
		}

		case cpu_type::arm:
		{
			m_disasm = std::make_unique<ARMv7DisAsm>(CPUDisAsm_InterpreterMode);
			break;
		}
		}
	}

	DoUpdate();
}

void InterpreterDisAsmFrame::OnKeyDown(wxKeyEvent& event)
{
	if (wxGetActiveWindow() != wxGetTopLevelParent(this))
	{
		event.Skip();
		return;
	}

	if (event.ControlDown())
	{
		if (event.GetKeyCode() == WXK_SPACE)
		{
			wxCommandEvent ce;
			DoStep(ce);
			return;
		}
	}
	else
	{
		switch (event.GetKeyCode())
		{
		case WXK_PAGEUP:   ShowAddr(m_pc - (m_item_count * 2) * 4); return;
		case WXK_PAGEDOWN: ShowAddr(m_pc); return;
		case WXK_UP:       ShowAddr(m_pc - (m_item_count + 1) * 4); return;
		case WXK_DOWN:     ShowAddr(m_pc - (m_item_count - 1) * 4); return;
		}
	}

	event.Skip();
}

void InterpreterDisAsmFrame::OnResize(wxSizeEvent& event)
{
	event.Skip();

	if (0)
	{
		if (!m_list->GetItemCount())
		{
			m_list->InsertItem(m_list->GetItemCount(), wxEmptyString);
		}

		int size = 0;
		m_list->DeleteAllItems();
		int item = 0;
		while (size < m_list->GetSize().GetHeight())
		{
			item = m_list->GetItemCount();
			m_list->InsertItem(item, wxEmptyString);
			wxRect rect;
			m_list->GetItemRect(item, rect);

			size = rect.GetBottom();
		}

		if (item)
		{
			m_list->DeleteItem(--item);
		}

		m_item_count = item;
		ShowAddr(m_pc);
	}
}

void InterpreterDisAsmFrame::DoUpdate()
{
	wxCommandEvent ce;
	Show_PC(ce);
	WriteRegs();
	WriteCallStack();
}

void InterpreterDisAsmFrame::ShowAddr(u32 addr)
{
	m_pc = addr;
	m_list->Freeze();

	if (!cpu)
	{
		for (uint i = 0; i<m_item_count; ++i, m_pc += 4)
		{
			m_list->SetItem(i, 0, wxString::Format("[%08x] illegal address", m_pc));
		}
	}
	else
	{
		const u32 cpu_offset = cpu->type == cpu_type::spu ? static_cast<SPUThread&>(*cpu).offset : 0;
		m_disasm->offset = (u8*)vm::base(cpu_offset);
		for (uint i = 0, count = 4; i<m_item_count; ++i, m_pc += count)
		{
			if (!vm::check_addr(cpu_offset + m_pc, 4))
			{
				m_list->SetItem(i, 0, wxString(IsBreakPoint(m_pc) ? ">>> " : "    ") + wxString::Format("[%08x] illegal address", m_pc));
				count = 4;
				continue;
			}

			count = m_disasm->disasm(m_disasm->dump_pc = m_pc);

			m_list->SetItem(i, 0, wxString(IsBreakPoint(m_pc) ? ">>> " : "    ") + fmt::FromUTF8(m_disasm->last_opcode));

			wxColour colour;

			if (cpu->state.test(cpu_state_pause) && m_pc == GetPc())
			{
				colour = wxColour("Green");
			}
			else
			{
				colour = wxColour(IsBreakPoint(m_pc) ? "Wheat" : "White");
			}

			m_list->SetItemBackgroundColour(i, colour);
		}
	}

	m_list->SetColumnWidth(0, -1);
	m_list->Thaw();
}

void InterpreterDisAsmFrame::WriteRegs()
{
	if (!cpu)
	{
		m_regs->Clear();
		return;
	}

	m_regs->Freeze();
	m_regs->Clear();
	m_regs->WriteText(fmt::FromUTF8(cpu->dump()));
	m_regs->Thaw();
}

void InterpreterDisAsmFrame::WriteCallStack()
{
	if (!cpu)
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
	cpu_thread* thr = (cpu_thread*)event.GetClientData();
	event.Skip();

	if (!thr)
	{
		switch (event.GetId())
		{
		case DID_STOPPED_EMU:
			UpdateUnitList();
			break;

		case DID_PAUSED_EMU:
			//DoUpdate();
			break;
		}
	}
	else if (cpu && thr == cpu)
	{
		switch (event.GetId())
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

			if (event.GetId() == DID_REMOVE_THREAD)
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
		switch (event.GetId())
		{
		case DID_CREATE_THREAD:
			UpdateUnitList();
			if (m_choice_units->GetSelection() == -1)
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

	diag->SetSizerAndFit(s_panel);

	if (cpu) p_pc->SetValue(wxString::Format("%x", GetPc()));

	if (diag->ShowModal() == wxID_OK)
	{
		unsigned long pc = cpu ? GetPc() : 0x0;
		p_pc->GetValue().ToULong(&pc, 16);
		ShowAddr(CentrePc(pc));
	}
}

void InterpreterDisAsmFrame::Show_PC(wxCommandEvent& WXUNUSED(event))
{
	if (cpu) ShowAddr(CentrePc(GetPc()));
}

void InterpreterDisAsmFrame::DoRun(wxCommandEvent& WXUNUSED(event))
{
	if (cpu && cpu->state.test(cpu_state_pause))
	{
		cpu->state -= cpu_state::dbg_pause;
		(*cpu)->lock_notify();
	}
}

void InterpreterDisAsmFrame::DoPause(wxCommandEvent& WXUNUSED(event))
{
	if (cpu)
	{
		cpu->state += cpu_state::dbg_pause;
	}
}

void InterpreterDisAsmFrame::DoStep(wxCommandEvent& WXUNUSED(event))
{
	if (cpu)
	{
		if (cpu->state.atomic_op([](bitset_t<cpu_state>& state) -> bool
		{
			state += cpu_state::dbg_step;
			return state.test_and_reset(cpu_state::dbg_pause);
		}))
		{
			(*cpu)->lock_notify();
		}
	}
}

void InterpreterDisAsmFrame::InstrKey(wxListEvent& event)
{
	long i = m_list->GetFirstSelected();
	if (i < 0 || !cpu)
	{
		event.Skip();
		return;
	}

	const u32 start_pc = m_pc - m_item_count * 4;
	const u32 pc = start_pc + i * 4;

	switch (event.GetKeyCode())
	{
	case 'E':
	{
		InstructionEditorDialog dlg(this, pc, cpu, m_disasm.get());
		DoUpdate();
		return;
	}
	case 'R':
	{
		RegisterEditorDialog dlg(this, pc, cpu, m_disasm.get());
		DoUpdate();
		return;
	}
	}

	event.Skip();
}

void InterpreterDisAsmFrame::DClick(wxListEvent& event)
{
	long i = m_list->GetFirstSelected();
	if (i < 0) return;

	const u32 start_pc = m_pc - m_item_count * 4;
	const u32 pc = start_pc + i * 4;
	//ConLog.Write("pc=0x%llx", pc);

	if (IsBreakPoint(pc))
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

	ShowAddr(m_pc - (event.ControlDown() ? m_item_count * (value + 1) : m_item_count + value) * 4);

	event.Skip();
}

bool InterpreterDisAsmFrame::IsBreakPoint(u32 pc)
{
	return g_breakpoints.count(pc) != 0;
}

void InterpreterDisAsmFrame::AddBreakPoint(u32 pc)
{
	g_breakpoints.emplace(pc, false);
}

bool InterpreterDisAsmFrame::RemoveBreakPoint(u32 pc)
{
	return g_breakpoints.erase(pc) != 0;
}
