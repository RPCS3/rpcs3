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
#include "Emu/Cell/PPUInterpreter.h"

#include "InstructionEditor.h"
#include "RegisterEditor.h"

//static const int show_lines = 30;
#include <map>

std::map<u32, bool> g_breakpoints;

extern void ppu_breakpoint(u32 addr);

u32 InterpreterDisAsmFrame::GetPc() const
{
	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		return 0;
	}

	switch (g_system)
	{
	case system_type::ps3: return cpu->id_type() == 1 ? static_cast<ppu_thread*>(cpu.get())->cia : static_cast<SPUThread*>(cpu.get())->pc;
	case system_type::psv: return static_cast<ARMv7Thread*>(cpu.get())->PC;
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
	, m_item_count(30)
{
	wxBoxSizer* s_p_main = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_b_main = new wxBoxSizer(wxHORIZONTAL);

	m_list = new wxListView(this);
	m_choice_units = new wxChoice(this, wxID_ANY);

	wxButton* b_go_to_addr = new wxButton(this, wxID_ANY, "Go to address");
	wxButton* b_go_to_pc = new wxButton(this, wxID_ANY, "Go to PC");

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

	m_list->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_regs->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	wxBoxSizer* s_w_list = new wxBoxSizer(wxHORIZONTAL);
	s_w_list->Add(m_list, 2, wxEXPAND | wxLEFT | wxDOWN, 5);
	s_w_list->Add(m_regs, 1, wxEXPAND | wxRIGHT | wxDOWN, 5);

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

	ShowAddr(CentrePc(m_pc));
	UpdateUnitList();
}

InterpreterDisAsmFrame::~InterpreterDisAsmFrame()
{
}

void InterpreterDisAsmFrame::UpdateUI()
{
	UpdateUnitList();

	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		if (m_last_pc != -1 || m_last_stat)
		{
			m_last_pc = -1;
			m_last_stat = 0;
			DoUpdate();

			m_btn_run->Disable();
			m_btn_step->Disable();
			m_btn_pause->Disable();
		}
	}
	else
	{
		const auto cia = GetPc();
		const auto state = cpu->state.load();

		if (m_last_pc != cia || m_last_stat != static_cast<u32>(state))
		{
			m_last_pc = cia;
			m_last_stat = static_cast<u32>(state);
			DoUpdate();

			if (test(state & cpu_flag::dbg_pause))
			{
				m_btn_run->Enable();
				m_btn_step->Enable();
				m_btn_pause->Disable();
			}
			else
			{
				m_btn_run->Disable();
				m_btn_step->Disable();
				m_btn_pause->Enable();
			}
		}
	}

	if (Emu.IsStopped())
	{
		g_breakpoints.clear();
	}
}

void InterpreterDisAsmFrame::UpdateUnitList()
{
	const u64 threads_created = cpu_thread::g_threads_created;
	const u64 threads_deleted = cpu_thread::g_threads_deleted;

	if (threads_created != m_threads_created || threads_deleted != m_threads_deleted)
	{
		m_threads_created = threads_created;
		m_threads_deleted = threads_deleted;
	}
	else
	{
		// Nothing to do
		return;
	}

	m_choice_units->Freeze();
	m_choice_units->Clear();

	const auto on_select = [&](u32, cpu_thread& cpu)
	{
		m_choice_units->Append(cpu.get_name(), &cpu);
	};

	idm::select<ppu_thread>(on_select);
	idm::select<ARMv7Thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);

	m_choice_units->Thaw();
	m_choice_units->Update();
}

void InterpreterDisAsmFrame::OnSelectUnit(wxCommandEvent& event)
{
	m_disasm.reset();

	const auto on_select = [&](u32, cpu_thread& cpu)
	{
		return event.GetClientData() == &cpu;
	};

	if (event.GetClientData() == nullptr)
	{
		// Nothing selected
	}
	else if (auto ppu = idm::select<ppu_thread>(on_select))
	{
		m_disasm = std::make_unique<PPUDisAsm>(CPUDisAsm_InterpreterMode);
		cpu = ppu.ptr;
	}
	else if (auto spu1 = idm::select<SPUThread>(on_select))
	{
		m_disasm = std::make_unique<SPUDisAsm>(CPUDisAsm_InterpreterMode);
		cpu = spu1.ptr;
	}
	else if (auto rspu = idm::select<RawSPUThread>(on_select))
	{
		m_disasm = std::make_unique<SPUDisAsm>(CPUDisAsm_InterpreterMode);
		cpu = rspu.ptr;
	}
	else if (auto arm = idm::select<ARMv7Thread>(on_select))
	{
		m_disasm = std::make_unique<ARMv7DisAsm>(CPUDisAsm_InterpreterMode);
		cpu = arm.ptr;
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
}

void InterpreterDisAsmFrame::ShowAddr(u32 addr)
{
	m_pc = addr;
	m_list->Freeze();

	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		for (uint i = 0; i<m_item_count; ++i, m_pc += 4)
		{
			m_list->SetItem(i, 0, wxString::Format("[%08x] illegal address", m_pc));
		}
	}
	else
	{
		const u32 cpu_offset = g_system == system_type::ps3 && cpu->id_type() != 1 ? static_cast<SPUThread&>(*cpu).offset : 0;
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

			if (test(cpu->state & cpu_state_pause) && m_pc == GetPc())
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
	const auto cpu = this->cpu.lock();

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

	const auto cpu = this->cpu.lock();

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
	if (const auto cpu = this->cpu.lock()) ShowAddr(CentrePc(GetPc()));
}

void InterpreterDisAsmFrame::DoRun(wxCommandEvent& WXUNUSED(event))
{
	const auto cpu = this->cpu.lock();

	if (cpu && cpu->state.test_and_reset(cpu_flag::dbg_pause))
	{
		if (!test(cpu->state, cpu_flag::dbg_pause + cpu_flag::dbg_global_pause))
		{
			cpu->notify();
		}
	}
}

void InterpreterDisAsmFrame::DoPause(wxCommandEvent& WXUNUSED(event))
{
	if (const auto cpu = this->cpu.lock())
	{
		cpu->state += cpu_flag::dbg_pause;
	}
}

void InterpreterDisAsmFrame::DoStep(wxCommandEvent& WXUNUSED(event))
{
	if (const auto cpu = this->cpu.lock())
	{
		if (test(cpu_flag::dbg_pause, cpu->state.fetch_op([](bs_t<cpu_flag>& state)
		{
			state += cpu_flag::dbg_step;
			state -= cpu_flag::dbg_pause;
		})))
		{
			cpu->notify();
		}
	}
}

void InterpreterDisAsmFrame::InstrKey(wxListEvent& event)
{
	const auto cpu = this->cpu.lock();

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
	ppu_breakpoint(pc);
}

void InterpreterDisAsmFrame::RemoveBreakPoint(u32 pc)
{
	g_breakpoints.erase(pc);
	ppu_breakpoint(pc);
}
