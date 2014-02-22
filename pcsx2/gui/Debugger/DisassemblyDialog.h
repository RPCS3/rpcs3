#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include "App.h"

#include "CtrlDisassemblyView.h"
#include "CtrlRegisterList.h"
#include "CtrlMemView.h"
#include "DebugEvents.h"

class DebuggerHelpDialog: public wxDialog
{
public:
	DebuggerHelpDialog(wxWindow* parent);
};

class CpuTabPage: public wxPanel
{
public:
	CpuTabPage(wxWindow* parent, DebugInterface* _cpu);
	DebugInterface* getCpu() { return cpu; };
	CtrlDisassemblyView* getDisassembly() { return disassembly; };
	CtrlRegisterList* getRegisterList() { return registerList; };
	CtrlMemView* getMemoryView() { return memory; };
	wxNotebook* getBottomTabs() { return bottomTabs; };
private:
	DebugInterface* cpu;
	CtrlDisassemblyView* disassembly;
	CtrlRegisterList* registerList;
	CtrlMemView* memory;
	wxNotebook* bottomTabs;
};

class DisassemblyDialog : public wxFrame
{
public:
	DisassemblyDialog( wxWindow* parent=NULL );
	virtual ~DisassemblyDialog() throw() {}

	static wxString GetNameStatic() { return L"DisassemblyDialog"; }
	wxString GetDialogName() const { return GetNameStatic(); }
	
	void update();
	void reset();
	void setDebugMode(bool debugMode);
	
#ifdef WIN32
	WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);
#endif

	DECLARE_EVENT_TABLE()
protected:
	void onBreakRunClicked(wxCommandEvent& evt);
	void onStepOverClicked(wxCommandEvent& evt);
	void onDebuggerEvent(wxCommandEvent& evt);
	void onPageChanging(wxCommandEvent& evt);
	void onBreakpointClick(wxCommandEvent& evt);
	void stepOver();
private:
	CpuTabPage* eeTab;
	CpuTabPage* iopTab;
	CpuTabPage* currentCpu;

	wxBoxSizer* topSizer;
	wxStatusBar* statusBar;
	wxButton* breakRunButton;
	wxButton* stepIntoButton;
	wxButton* stepOverButton;
	wxButton* stepOutButton;
	wxButton* breakpointButton;
};