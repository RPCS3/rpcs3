#pragma once

class TextInputDialog : public wxDialog
{
	wxTextCtrl* m_tctrl_text;
	wxString m_result;

public:
	TextInputDialog(wxWindow* parent, const wxString& defvalue = wxEmptyString);
	void OnOk(wxCommandEvent& event);

	wxString& GetResult();
};