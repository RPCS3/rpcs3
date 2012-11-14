#pragma once
#include "Emu/Cell/PPCThread.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDecoder.h"
#include "Emu/Cell/SPUDisAsm.h"

class InterpreterDisAsmFrame
	: public FrameBase
	, public ThreadBase
{
	wxListView* m_list;
	wxPanel& m_main_panel;
	PPCThread& CPU;
	DisAsm* disasm;
	Decoder* decoder;
	u64 PC;
	Array<u64> markedPC;
	Array<u32> remove_markedPC;
	wxTextCtrl* m_regs;
	Array<u64> m_break_points;
	wxButton* m_btn_step;
	wxButton* m_btn_run;
	wxButton* m_btn_pause;
	volatile bool m_exec;

public:
	InterpreterDisAsmFrame(const wxString& title, PPCThread* cpu);
	~InterpreterDisAsmFrame();

	void Save(const wxString& path);
	void Load(const wxString& path);

	virtual void OnKeyDown(wxKeyEvent& event);
	void DoUpdate();
	void ShowAddr(const u64 addr);
	void WriteRegs();

	void OnUpdate(wxCommandEvent& event);
	void Show_Val(wxCommandEvent& event);
	void Show_PC(wxCommandEvent& event);
	void DoRun(wxCommandEvent& event);
	void DoPause(wxCommandEvent& event);
	void DoStep(wxCommandEvent& event);
	void DClick(wxListEvent& event);

	void OnResize(wxSizeEvent& event);
	void MouseWheel(wxMouseEvent& event);
	bool IsBreakPoint(u64 pc);
	void AddBreakPoint(u64 pc);
	bool RemoveBreakPoint(u64 pc);

	virtual void Task();
};