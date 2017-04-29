#pragma once
#include <wx/listctrl.h>
#include <wx/aui/aui.h>

class DbgEmuPanel;
class InterpreterDisAsmFrame;

class DebuggerPanel : public wxPanel
{
	wxAuiManager m_aui_mgr;

	DbgEmuPanel* m_dbg_panel;
	InterpreterDisAsmFrame* m_disasm_frame;

public:
	DebuggerPanel(wxWindow* parent);
	~DebuggerPanel();

	void UpdateUI();
};
