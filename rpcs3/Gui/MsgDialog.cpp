#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"

#include "Emu/SysCalls/lv2/sys_time.h"
#include "MsgDialog.h"

void MsgDialogFrame::Create(u32 type, const char* msg)
{
	wxWindow* parent = nullptr; // TODO: align the window better

	m_gauge1 = nullptr;
	m_gauge2 = nullptr;
	m_text1 = nullptr;
	m_text2 = nullptr;
	m_button_ok = nullptr;
	m_button_yes = nullptr;
	m_button_no = nullptr;

	m_dialog = new wxDialog(parent, wxID_ANY, type & CELL_MSGDIALOG_TYPE_SE_TYPE ? "" : "Error", wxDefaultPosition, wxDefaultSize);

	m_dialog->SetExtraStyle(m_dialog->GetExtraStyle() | wxWS_EX_TRANSIENT);
	m_dialog->SetTransparent(127 + (type & CELL_MSGDIALOG_TYPE_BG) * (128 / CELL_MSGDIALOG_TYPE_BG_INVISIBLE));

	m_sizer1 = new wxBoxSizer(wxVERTICAL);

	m_text = new wxStaticText(m_dialog, wxID_ANY, wxString(msg, wxConvUTF8));
	m_sizer1->Add(m_text, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);

	switch (type & CELL_MSGDIALOG_TYPE_PROGRESSBAR)
	{
	case CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE:
		m_gauge2 = new wxGauge(m_dialog, wxID_ANY, 100, wxDefaultPosition, wxSize(300, -1), wxGA_HORIZONTAL | wxGA_SMOOTH);
		m_text2 = new wxStaticText(m_dialog, wxID_ANY, "");
		m_text2->SetAutoLayout(true);
		// fallthrough

	case CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE:
		m_gauge1 = new wxGauge(m_dialog, wxID_ANY, 100, wxDefaultPosition, wxSize(300, -1), wxGA_HORIZONTAL | wxGA_SMOOTH);
		m_text1 = new wxStaticText(m_dialog, wxID_ANY, "");
		m_text1->SetAutoLayout(true);
	}

	if (m_gauge1)
	{
		m_sizer1->Add(m_text1, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 8);
		m_sizer1->Add(m_gauge1, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 16);
		m_gauge1->SetValue(0);
	}
	if (m_gauge2)
	{
		m_sizer1->Add(m_text2, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 8);
		m_sizer1->Add(m_gauge2, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 16);
		m_gauge2->SetValue(0);
	}

	wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);

	switch (type & CELL_MSGDIALOG_TYPE_BUTTON_TYPE)
	{
	case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO:
		m_button_yes = new wxButton(m_dialog, wxID_YES);
		buttons->Add(m_button_yes, 0, wxALIGN_CENTER_HORIZONTAL | wxRIGHT, 8);
		m_button_no = new wxButton(m_dialog, wxID_NO);
		buttons->Add(m_button_no, 0, wxALIGN_CENTER_HORIZONTAL, 16);
		if ((type & CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR) == CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NO)
		{
			m_button_no->SetFocus();
		}
		else
		{
			m_button_yes->SetFocus();
		}

		m_sizer1->Add(buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);
		break;

	case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK:
		m_button_ok = new wxButton(m_dialog, wxID_OK);
		buttons->Add(m_button_ok, 0, wxALIGN_CENTER_HORIZONTAL, 16);
		if ((type & CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR) == CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_OK)
		{
			m_button_ok->SetFocus();
		}

		m_sizer1->Add(buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);
		break;
	}

	m_sizer1->AddSpacer(16);

	m_dialog->SetSizerAndFit(m_sizer1);
	m_dialog->Centre(wxBOTH);
	m_dialog->Show();
	m_dialog->Enable();

	m_dialog->Bind(wxEVT_BUTTON, [&](wxCommandEvent& event)
	{
		this->status = (event.GetId() == wxID_NO) ? CELL_MSGDIALOG_BUTTON_NO : CELL_MSGDIALOG_BUTTON_YES /* OK */;
		this->m_dialog->Hide();
		this->Close();
	});


	m_dialog->Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent& event)
	{
		if (type & CELL_MSGDIALOG_TYPE_DISABLE_CANCEL)
		{
		}
		else
		{
			this->status = CELL_MSGDIALOG_BUTTON_ESCAPE;
			this->m_dialog->Hide();
			this->Close();
		}
	});
}

void MsgDialogFrame::Destroy()
{
	delete m_dialog;
	m_dialog = nullptr;
}

void MsgDialogFrame::ProgressBarSetMsg(u32 index, const char* msg)
{
	if (m_dialog)
	{
		if (index == 0 && m_text1) m_text1->SetLabelText(wxString(msg, wxConvUTF8));
		if (index == 1 && m_text2) m_text2->SetLabelText(wxString(msg, wxConvUTF8));
		m_dialog->Layout();
		m_dialog->Fit();
	}
}

void MsgDialogFrame::ProgressBarReset(u32 index)
{
	if (m_dialog)
	{
		if (index == 0 && m_gauge1) m_gauge1->SetValue(0);
		if (index == 1 && m_gauge2) m_gauge2->SetValue(0);
	}
}

void MsgDialogFrame::ProgressBarInc(u32 index, u32 delta)
{
	if (m_dialog)
	{
		if (index == 0 && m_gauge1) m_gauge1->SetValue(m_gauge1->GetValue() + delta);
		if (index == 1 && m_gauge2) m_gauge2->SetValue(m_gauge2->GetValue() + delta);
	}
}
