#pragma once
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDecoder.h"
#include "Emu/CPU/CPUDisAsm.h"

class InterpreterDisAsmFrame : public wxPanel
{
	wxListView* m_list;
	CPUDisAsm* disasm;
	CPUDecoder* decoder;
	u64 PC;
	std::vector<u32> remove_markedPC;
	wxTextCtrl* m_regs;
	wxTextCtrl* m_calls;
	wxButton* m_btn_step;
	wxButton* m_btn_run;
	wxButton* m_btn_pause;
	AppConnector m_app_connector;
	u32 m_item_count;
	wxChoice* m_choice_units;

public:
	CPUThread* CPU;

public:
	InterpreterDisAsmFrame(wxWindow* parent);
	~InterpreterDisAsmFrame();

	void UpdateUnitList();

	u64 CentrePc(const u64 pc) const;
	void OnSelectUnit(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnResize(wxSizeEvent& event);
	void DoUpdate();
	void ShowAddr(const u64 addr);
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
	bool IsBreakPoint(u64 pc);
	void AddBreakPoint(u64 pc);
	bool RemoveBreakPoint(u64 pc);
};
