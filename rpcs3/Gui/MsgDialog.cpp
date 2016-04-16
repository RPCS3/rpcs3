#include "stdafx.h"
#include "stdafx_gui.h"
#include "rpcs3.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"

#include "Emu/Cell/lv2/sys_time.h"
#include "MsgDialog.h"

MsgDialogFrame::~MsgDialogFrame()
{
	if (m_dialog) wxGetApp().CallAfter([dialog = m_dialog]
	{
		dialog->Destroy();
	});
}

void MsgDialogFrame::Create(const std::string& msg)
{
	if (m_dialog) m_dialog->Destroy();

	wxWindow* parent = wxGetApp().m_MainFrame; // TODO

	m_gauge1 = nullptr;
	m_gauge2 = nullptr;
	m_text1 = nullptr;
	m_text2 = nullptr;
	m_button_ok = nullptr;
	m_button_yes = nullptr;
	m_button_no = nullptr;

	m_dialog = new wxDialog(nullptr, wxID_ANY, type.se_normal ? "Normal dialog" : "Error dialog", wxDefaultPosition, wxDefaultSize);

	m_dialog->SetExtraStyle(m_dialog->GetExtraStyle() | wxWS_EX_TRANSIENT);
	m_dialog->SetTransparent(type.bg_invisible ? 255 : 192);

	m_sizer1 = new wxBoxSizer(wxVERTICAL);

	m_text = new wxStaticText(m_dialog, wxID_ANY, wxString(msg.c_str(), wxConvUTF8));
	m_sizer1->Add(m_text, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);

	if (type.progress_bar_count >= 2)
	{
		m_gauge2 = new wxGauge(m_dialog, wxID_ANY, 100, wxDefaultPosition, wxSize(300, -1), wxGA_HORIZONTAL | wxGA_SMOOTH);
		m_text2 = new wxStaticText(m_dialog, wxID_ANY, "");
		m_text2->SetAutoLayout(true);
	}

	if (type.progress_bar_count >= 1)
	{
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

	m_buttons = new wxBoxSizer(wxHORIZONTAL);

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO)
	{
		m_button_yes = new wxButton(m_dialog, wxID_YES);
		m_buttons->Add(m_button_yes, 0, wxRIGHT, 8);
		m_button_no = new wxButton(m_dialog, wxID_NO);
		m_buttons->Add(m_button_no, 0, 0, 16);

		if (type.default_cursor == 1)
		{
			m_button_no->SetFocus();
		}
		else
		{
			m_button_yes->SetFocus();
		}

		m_sizer1->Add(m_buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK)
	{
		m_button_ok = new wxButton(m_dialog, wxID_OK);
		m_buttons->Add(m_button_ok, 0, 0, 16);

		if (type.default_cursor == 0)
		{
			m_button_ok->SetFocus();
		}

		m_sizer1->Add(m_buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);
	}

	m_sizer1->AddSpacer(16);

	m_dialog->SetSizerAndFit(m_sizer1);
	m_dialog->Centre(wxBOTH);
	m_dialog->Show();
	m_dialog->Enable();

	m_dialog->Bind(wxEVT_BUTTON, [on_close = on_close](wxCommandEvent& event)
	{
		on_close(event.GetId() == wxID_NO ? CELL_MSGDIALOG_BUTTON_NO : CELL_MSGDIALOG_BUTTON_YES);
	});

	m_dialog->Bind(wxEVT_CLOSE_WINDOW, [on_close = on_close, type = type](wxCloseEvent& event)
	{
		if (!type.disable_cancel)
		{
			on_close(CELL_MSGDIALOG_BUTTON_ESCAPE);
		}
	});
}

void MsgDialogFrame::ProgressBarSetMsg(u32 index, const std::string& msg)
{
	if (m_dialog)
	{
		if (index == 0 && m_text1) m_text1->SetLabelText(wxString(msg.c_str(), wxConvUTF8));
		if (index == 1 && m_text2) m_text2->SetLabelText(wxString(msg.c_str(), wxConvUTF8));
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
