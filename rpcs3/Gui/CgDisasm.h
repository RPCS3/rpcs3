#pragma once

#include <wx/wx.h>

enum CgDisasmIds
{
	id_open_file
};

class CgDisasm : public wxFrame
{
private:
	wxTextCtrl* m_disasm_text;
	wxTextCtrl* m_glsl_text;
	DECLARE_EVENT_TABLE();

public:
	CgDisasm(wxWindow* parent);

	void OpenCg(wxCommandEvent& event);
	virtual void OnSize(wxSizeEvent& event);
};