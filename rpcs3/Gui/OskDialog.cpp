#include "stdafx.h"
#include "stdafx_gui.h"
#include "rpcs3.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"
#include "wx/ustring.h"
#include "Emu/Cell/lv2/sys_time.h"
#include "MsgDialog.h"

void MsgDialogFrame::CreateOsk(const std::string& msg, char16_t* osk_text)
{
	if (m_dialog) m_dialog->Destroy();
	osk_button_ok = nullptr;
	osk_text_input = nullptr;
	osk_text_return = osk_text;

	m_dialog = new wxDialog(nullptr, wxID_ANY, msg, wxDefaultPosition, wxDefaultSize);
	wxSizer* osk_dialog_sizer = new wxBoxSizer(wxVERTICAL);
	wxSizer* osk_text_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizer* osk_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	osk_text_input = new wxTextCtrl(m_dialog, wxID_OK, wxEmptyString, wxDefaultPosition, wxSize(200, -1));
	osk_text_sizer->Add(osk_text_input, 0, wxALL, 4);
	if (type.default_cursor == 0)
	{
		osk_text_input->SetFocus();
	}

	osk_text_input->Bind(wxEVT_TEXT, [&](wxCommandEvent& event)
	{
		wxUString wx_osk_string = osk_text_input->GetValue();
		std::memcpy(osk_text_return, wx_osk_string.utf16_str(), wx_osk_string.size() * 2);
		on_osk_input_entered();
	});

	osk_button_ok = new wxButton(m_dialog, wxID_OK);
	osk_button_sizer->Add(osk_button_ok, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);

	osk_dialog_sizer->Add(osk_text_sizer, 0, wxCENTER);
	osk_dialog_sizer->Add(osk_button_sizer, 0, wxCENTER);

	m_dialog->SetSizerAndFit(osk_dialog_sizer);
	m_dialog->Centre(wxBOTH);
	m_dialog->Show();
	m_dialog->Enable();

	m_dialog->Bind(wxEVT_BUTTON, [&](wxCommandEvent& event)
	{
		on_close(CELL_MSGDIALOG_BUTTON_OK);
	});

	m_dialog->Bind(wxEVT_CLOSE_WINDOW, [on_close = on_close](wxCloseEvent& event)
	{
		on_close(CELL_MSGDIALOG_BUTTON_ESCAPE);
	});
}
