#pragma once
#include <wx/listctrl.h>
#include <wx/aui/aui.h>

class DebuggerPanel : public wxFrame
{
	wxAuiManager m_aui_mgr;

public:
	DebuggerPanel(wxWindow* parent);
	~DebuggerPanel();

	void UpdateUI();
};
