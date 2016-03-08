#pragma once

class TextInputDialog : public wxDialog
{
	wxTextCtrl* m_tctrl_text;
	std::string m_result;

public:
	TextInputDialog(wxWindow* parent, const std::string& defvalue = "", const std::string& label = "Input text");
	void OnOk(wxCommandEvent& event);

	std::string GetResult();
};