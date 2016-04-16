#pragma once

#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"

class InterpreterDisAsmFrame : public wxPanel
{
	wxListView* m_list;
	std::unique_ptr<CPUDisAsm> m_disasm;
	u32 m_pc;
	wxTextCtrl* m_regs;
	wxTextCtrl* m_calls;
	wxButton* m_btn_step;
	wxButton* m_btn_run;
	wxButton* m_btn_pause;
	u32 m_item_count;
	wxChoice* m_choice_units;

public:
	cpu_thread* cpu;

public:
	InterpreterDisAsmFrame(wxWindow* parent);
	~InterpreterDisAsmFrame();

	void UpdateUnitList();

	u32 GetPc() const;
	u32 CentrePc(u32 pc) const;
	void OnSelectUnit(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnResize(wxSizeEvent& event);
	void DoUpdate();
	void ShowAddr(u32 addr);
	void WriteRegs();
	void WriteCallStack();

	void HandleCommand(wxCommandEvent& event);
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
	bool RemoveBreakPoint(u32 pc);
};
