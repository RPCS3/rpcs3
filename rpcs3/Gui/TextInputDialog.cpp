#include "stdafx.h"
#include "TextInputDialog.h"

TextInputDialog::TextInputDialog(wxWindow* parent, const wxString& defvalue)
	: wxDialog(parent, wxID_ANY, "Input text", wxDefaultPosition)
{
	m_tctrl_text = new wxTextCtrl(this, wxID_ANY, defvalue);

	wxBoxSizer& s_text(*new wxBoxSizer(wxVERTICAL));
	s_text.Add(m_tctrl_text, 1, wxEXPAND);

	wxBoxSizer& s_btns(*new wxBoxSizer(wxHORIZONTAL));
	s_btns.Add(new wxButton(this, wxID_OK));
	s_btns.AddSpacer(30);
	s_btns.Add(new wxButton(this, wxID_CANCEL));

	wxBoxSizer& s_main(*new wxBoxSizer(wxVERTICAL));
	s_main.Add(&s_text, 1, wxEXPAND | wxUP | wxLEFT | wxRIGHT, 5);
	s_main.AddSpacer(30);
	s_main.Add(&s_btns, 0, wxCENTER | wxDOWN | wxLEFT | wxRIGHT, 5);

	SetSizerAndFit(&s_main);
	SetSize(250, -1);

	Connect(wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TextInputDialog::OnOk));
}

void TextInputDialog::OnOk(wxCommandEvent& event)
{
	m_result = m_tctrl_text->GetValue();
	EndModal(wxID_OK);
}

wxString& TextInputDialog::GetResult()
{
	return m_result;
}