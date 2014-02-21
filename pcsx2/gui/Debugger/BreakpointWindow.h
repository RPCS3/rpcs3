#pragma once
#include <wx/wx.h>
#include "DebugTools/DebugInterface.h"
#include "DebugTools/Breakpoints.h"

class BreakpointWindow : public wxDialog
{
public:
	BreakpointWindow( wxWindow* parent, DebugInterface* _cpu );
	void loadFromMemcheck(MemCheck& memcheck);
	void loadFromBreakpoint(BreakPoint& breakpoint);
	void initBreakpoint(u32 _address);
	void addBreakpoint();

	DECLARE_EVENT_TABLE()
protected:
	void onRadioChange(wxCommandEvent& evt);
	void onButtonOk(wxCommandEvent& evt);
private:
	void setDefaultValues();
	bool fetchDialogData();

	DebugInterface* cpu;
	
	wxTextCtrl* editAddress;
	wxTextCtrl* editSize;
	wxRadioButton* radioMemory;
	wxRadioButton* radioExecute;
	wxCheckBox* checkRead;
	wxCheckBox* checkWrite;
	wxCheckBox* checkOnChange;
	wxTextCtrl* editCondition;
	wxCheckBox* checkEnabled;
	wxCheckBox* checkLog;
	wxButton* buttonOk;
	wxButton* buttonCancel;

	bool memory;
	bool read;
	bool write;
	bool enabled;
	bool log;
	bool onChange;
	u32 address;
	u32 size;
	char condition[128];
	PostfixExpression compiledCondition;
};