#pragma once

#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"

class InterpreterDisAsmFrame : public wxPanel
{
	wxListView* m_list;
	std::unique_ptr<CPUDisAsm> m_disasm;
	u32 m_pc;
	wxTextCtrl* m_regs;
	wxButton* m_btn_step;
	wxButton* m_btn_run;
	wxButton* m_btn_pause;
	u32 m_item_count;
	wxChoice* m_choice_units;

	u64 m_threads_created = 0;
	u64 m_threads_deleted = 0;
	u32 m_last_pc = -1;
	u32 m_last_stat = 0;

public:
	std::weak_ptr<cpu_thread> cpu;

public:
	InterpreterDisAsmFrame(wxWindow* parent);
	~InterpreterDisAsmFrame();

	void UpdateUI();
	void UpdateUnitList();

	u32 GetPc() const;
	u32 CentrePc(u32 pc) const;
	void OnSelectUnit(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnResize(wxSizeEvent& event);
	void DoUpdate();
	void ShowAddr(u32 addr);
	void WriteRegs();

	void OnUpdate(wxCommandEvent& event);
	void Show_Val(wxCommandEvent& event);
	void Show_PC(wxCommandEvent& event);
	void DoRun(wxCommandEvent& event);
	void DoPause(wxCommandEvent& event);
	void DoStep(wxCommandEvent& event);
	void InstrKey(wxListEvent& event);
	void DClick(wxListEvent& event);

	void MouseWheel(wxMouseEvent& event);
	bool IsBreakPoint(u32 pc);
	void AddBreakPoint(u32 pc);
	void RemoveBreakPoint(u32 pc);
};
