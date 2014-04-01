#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include <wx/aui/aui.h>
#include "Loader/ELF64.h"
#include <wx/richtext/richtextctrl.h>

class CompilerELF : public FrameBase
{
	wxAuiManager m_aui_mgr;
	wxStatusBar& m_status_bar;
	bool m_disable_scroll;
	AppConnector m_app_connector;

public:
	CompilerELF(wxWindow* parent);
	~CompilerELF();

	wxTextCtrl* asm_list;
	wxTextCtrl* hex_list;
	wxTextCtrl* err_list;
	
	void MouseWheel(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	void OnUpdate(wxCommandEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void UpdateScroll(bool is_hex, int orient);

	void AnalyzeCode(wxCommandEvent& WXUNUSED(event))
	{
		DoAnalyzeCode(false);
	}

	void CompileCode(wxCommandEvent& WXUNUSED(event))
	{
		DoAnalyzeCode(true);
	}

	void LoadElf(wxCommandEvent& event);
	void LoadElf(const std::string& path);

	void SetTextStyle(const std::string& text, const wxColour& color, bool bold=false);
	void SetOpStyle(const std::string& text, const wxColour& color, bool bold = true);
	void DoAnalyzeCode(bool compile);

	void UpdateStatus(int offset=0);
};
