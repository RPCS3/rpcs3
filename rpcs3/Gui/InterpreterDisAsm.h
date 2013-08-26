#pragma once
#include "Emu/Cell/PPCThread.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDecoder.h"
#include "Emu/Cell/SPUDisAsm.h"

class InterpreterDisAsmFrame
	: public wxPanel
	, public ThreadBase
{
	wxListView* m_list;
	PPC_DisAsm* disasm;
	PPC_Decoder* decoder;
	u64 PC;
	Array<u32> remove_markedPC;
	wxTextCtrl* m_regs;
	wxButton* m_btn_step;
	wxButton* m_btn_run;
	wxButton* m_btn_pause;
	AppConnector m_app_connector;
	u32 m_item_count;
	wxChoice* m_choice_units;

public:
	PPCThread* CPU;

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

	void HandleCommand(wxCommandEvent& event);
	void OnUpdate(wxCommandEvent& event);
	void Show_Val(wxCommandEvent& event);
	void Show_PC(wxCommandEvent& event);
	void DoRun(wxCommandEvent& event);
	void DoPause(wxCommandEvent& event);
	void DoStep(wxCommandEvent& event);
	void DClick(wxListEvent& event);

	void MouseWheel(wxMouseEvent& event);
	bool IsBreakPoint(u64 pc);
	void AddBreakPoint(u64 pc);
	bool RemoveBreakPoint(u64 pc);

	virtual void Task();
};