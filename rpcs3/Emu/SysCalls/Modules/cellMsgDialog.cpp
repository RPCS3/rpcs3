#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "rpcs3.h"
#include "Utilities/rMsgBox.h"
#include "Emu/SysCalls/lv2/sys_time.h"
#include "cellSysutil.h"
#include "cellMsgDialog.h"

extern Module *cellSysutil;

enum MsgDialogState
{
	msgDialogNone,
	msgDialogOpen,
	msgDialogClose,
	msgDialogAbort,
};

std::atomic<MsgDialogState> g_msg_dialog_state(msgDialogNone);
wxDialog* g_msg_dialog = nullptr;
wxGauge* m_gauge1 = nullptr;
wxGauge* m_gauge2 = nullptr;
wxStaticText* m_text1 = nullptr;
wxStaticText* m_text2 = nullptr;
u64 m_wait_until;

int cellMsgDialogOpen2(u32 type, mem_list_ptr_t<u8> msgString, mem_func_ptr_t<CellMsgDialogCallback> callback, u32 userData, u32 extParam)
{
	cellSysutil->Warning("cellMsgDialogOpen2(type=0x%x, msgString_addr=0x%x, callback_addr=0x%x, userData=0x%x, extParam=0x%x)",
		type, msgString.GetAddr(), callback.GetAddr(), userData, extParam);
	
	//type |= CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE;
	//type |= CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO;

	MsgDialogState old = msgDialogNone;
	if (!g_msg_dialog_state.compare_exchange_strong(old, msgDialogOpen))
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	thread t("MsgDialog thread", [=]()
	{
		switch (type & CELL_MSGDIALOG_TYPE_SE_TYPE)
		{
		case CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL: cellSysutil->Warning("Message: \n%s", msgString.GetString()); break;
		case CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR: cellSysutil->Error("Message: \n%s", msgString.GetString()); break;
		}

		switch (type & CELL_MSGDIALOG_TYPE_SE_MUTE) // TODO
		{
		case CELL_MSGDIALOG_TYPE_SE_MUTE_OFF: break;
		case CELL_MSGDIALOG_TYPE_SE_MUTE_ON: break;
		}

		switch (type & CELL_MSGDIALOG_TYPE_BG) // TODO
		{
		case CELL_MSGDIALOG_TYPE_BG_INVISIBLE: break; 
		case CELL_MSGDIALOG_TYPE_BG_VISIBLE: break;
		}

		switch (type & CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR) // TODO
		{
		case CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NO: break;
		default: break;
		}

		u64 status = CELL_MSGDIALOG_BUTTON_NONE;

		volatile bool m_signal = false;
		wxGetApp().CallAfter([&]()
		{
			wxWindow* parent = nullptr; // TODO: align it better

			m_gauge1 = nullptr;
			m_gauge2 = nullptr;
			m_text1 = nullptr;
			m_text2 = nullptr;
			wxButton* m_button_ok = nullptr;
			wxButton* m_button_yes = nullptr;
			wxButton* m_button_no = nullptr;

			g_msg_dialog = new wxDialog(parent, wxID_ANY, type & CELL_MSGDIALOG_TYPE_SE_TYPE ? "" : "Error", wxDefaultPosition, wxDefaultSize);

			g_msg_dialog->SetExtraStyle(g_msg_dialog->GetExtraStyle() | wxWS_EX_TRANSIENT);

			wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);

			wxStaticText* m_text = new wxStaticText(g_msg_dialog, wxID_ANY, wxString(msgString.GetString(), wxConvUTF8));
			sizer1->Add(m_text, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);

			switch (type & CELL_MSGDIALOG_TYPE_PROGRESSBAR)
			{
			case CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE:
				m_gauge2 = new wxGauge(g_msg_dialog, wxID_ANY, 100, wxDefaultPosition, wxSize(300, -1), wxGA_HORIZONTAL | wxGA_SMOOTH);
				m_text2 = new wxStaticText(g_msg_dialog, wxID_ANY, "");
				m_text2->SetAutoLayout(true);
				
			case CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE:
				m_gauge1 = new wxGauge(g_msg_dialog, wxID_ANY, 100, wxDefaultPosition, wxSize(300, -1), wxGA_HORIZONTAL | wxGA_SMOOTH);
				m_text1 = new wxStaticText(g_msg_dialog, wxID_ANY, "");
				m_text1->SetAutoLayout(true);
				
			case CELL_MSGDIALOG_TYPE_PROGRESSBAR_NONE:
				break;
			}

			if (m_gauge1)
			{
				sizer1->Add(m_text1, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 8);
				sizer1->Add(m_gauge1, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 16);
				m_gauge1->SetValue(0);
			}
			if (m_gauge2)
			{
				sizer1->Add(m_text2, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 8);
				sizer1->Add(m_gauge2, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 16);
				m_gauge2->SetValue(0);
			}
				
			wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);

			switch (type & CELL_MSGDIALOG_TYPE_BUTTON_TYPE)
			{
			case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE:
				break;

			case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO:
				m_button_yes = new wxButton(g_msg_dialog, wxID_YES);
				buttons->Add(m_button_yes, 0, wxALIGN_CENTER_HORIZONTAL | wxRIGHT, 8);
				m_button_no = new wxButton(g_msg_dialog, wxID_NO);
				buttons->Add(m_button_no, 0, wxALIGN_CENTER_HORIZONTAL, 16);

				sizer1->Add(buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);
				break;

			case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK:
				m_button_ok = new wxButton(g_msg_dialog, wxID_OK);
				buttons->Add(m_button_ok, 0, wxALIGN_CENTER_HORIZONTAL, 16);

				sizer1->Add(buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 16);
				break;
			}

			sizer1->AddSpacer(16);

			g_msg_dialog->SetSizerAndFit(sizer1);
			g_msg_dialog->Centre(wxBOTH);
			g_msg_dialog->Show();
			g_msg_dialog->Enable();

			g_msg_dialog->Bind(wxEVT_BUTTON, [&](wxCommandEvent& event)
			{
				status = (event.GetId() == wxID_NO) ? CELL_MSGDIALOG_BUTTON_NO : CELL_MSGDIALOG_BUTTON_YES /* OK */;
				g_msg_dialog->Hide();
				m_wait_until = get_system_time();
				g_msg_dialog_state = msgDialogClose;
			});


			g_msg_dialog->Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent& event)
			{
				if (type & CELL_MSGDIALOG_TYPE_DISABLE_CANCEL)
				{
				}
				else
				{
					status = CELL_MSGDIALOG_BUTTON_ESCAPE;
					g_msg_dialog->Hide();
					m_wait_until = get_system_time();
					g_msg_dialog_state = msgDialogClose;
				}
			});

			m_signal = true;
		});

		while (!m_signal)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		while (g_msg_dialog_state == msgDialogOpen || get_system_time() < m_wait_until)
		{
			if (Emu.IsStopped())
			{
				g_msg_dialog_state = msgDialogAbort;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		if (callback && (g_msg_dialog_state != msgDialogAbort))
			callback.async(status, userData);

		wxGetApp().CallAfter([&]()
		{
			delete g_msg_dialog;
			g_msg_dialog = nullptr;
		});

		g_msg_dialog_state = msgDialogNone;
	});
	t.detach();

	return CELL_OK;
}

int cellMsgDialogOpenErrorCode(u32 errorCode, mem_func_ptr_t<CellMsgDialogCallback> callback, mem_ptr_t<void> userData, u32 extParam)
{
	cellSysutil->Warning("cellMsgDialogOpenErrorCode(errorCode=0x%x, callback_addr=0x%x, userData=%d, extParam=%d)",
		errorCode, callback.GetAddr(), userData, extParam);

	std::string errorMessage;
	switch (errorCode)
	{
		// Generic errors
	case 0x80010001: errorMessage = "The resource is temporarily unavailable."; break;
	case 0x80010002: errorMessage = "Invalid argument or flag."; break;
	case 0x80010003: errorMessage = "The feature is not yet implemented."; break;
	case 0x80010004: errorMessage = "Memory allocation failed."; break;
	case 0x80010005: errorMessage = "The resource with the specified identifier does not exist."; break;
	case 0x80010006: errorMessage = "The file does not exist."; break;
	case 0x80010007: errorMessage = "The file is in unrecognized format / The file is not a valid ELF file."; break;
	case 0x80010008: errorMessage = "Resource deadlock is avoided."; break;
	case 0x80010009: errorMessage = "Operation not permitted."; break;
	case 0x8001000A: errorMessage = "The device or resource is bus."; break;
	case 0x8001000B: errorMessage = "The operation is timed ou."; break;
	case 0x8001000C: errorMessage = "The operation is aborte."; break;
	case 0x8001000D: errorMessage = "Invalid memory access."; break;
	case 0x8001000F: errorMessage = "State of the target thread is invalid."; break;
	case 0x80010010: errorMessage = "Alignment is invalid."; break;
	case 0x80010011: errorMessage = "Shortage of the kernel resources."; break;
	case 0x80010012: errorMessage = "The file is a directory."; break;
	case 0x80010013: errorMessage = "Operation canceled."; break;
	case 0x80010014: errorMessage = "Entry already exists."; break;
	case 0x80010015: errorMessage = "Port is already connected."; break;
	case 0x80010016: errorMessage = "Port is not connected."; break;
	case 0x80010017: errorMessage = "Failure in authorizing SELF. Program authentication fail."; break;
	case 0x80010018: errorMessage = "The file is not MSELF."; break;
	case 0x80010019: errorMessage = "System version error."; break;
	case 0x8001001A: errorMessage = "Fatal system error occurred while authorizing SELF. SELF auth failure."; break;
	case 0x8001001B: errorMessage = "Math domain violation."; break;
	case 0x8001001C: errorMessage = "Math range violation."; break;
	case 0x8001001D: errorMessage = "Illegal multi-byte sequence in input."; break;
	case 0x8001001E: errorMessage = "File position error."; break;
	case 0x8001001F: errorMessage = "Syscall was interrupted."; break;
	case 0x80010020: errorMessage = "File too large."; break;
	case 0x80010021: errorMessage = "Too many links."; break;
	case 0x80010022: errorMessage = "File table overflow."; break;
	case 0x80010023: errorMessage = "No space left on device."; break;
	case 0x80010024: errorMessage = "Not a TTY."; break;
	case 0x80010025: errorMessage = "Broken pipe."; break;
	case 0x80010026: errorMessage = "Read-only filesystem."; break;
	case 0x80010027: errorMessage = "Illegal seek."; break;
	case 0x80010028: errorMessage = "Arg list too long."; break;
	case 0x80010029: errorMessage = "Access violation."; break;
	case 0x8001002A: errorMessage = "Invalid file descriptor."; break;
	case 0x8001002B: errorMessage = "Filesystem mounting failed."; break;
	case 0x8001002C: errorMessage = "Too many files open."; break;
	case 0x8001002D: errorMessage = "No device."; break;
	case 0x8001002E: errorMessage = "Not a directory."; break;
	case 0x8001002F: errorMessage = "No such device or IO."; break;
	case 0x80010030: errorMessage = "Cross-device link error."; break;
	case 0x80010031: errorMessage = "Bad Message."; break;
	case 0x80010032: errorMessage = "In progress."; break;
	case 0x80010033: errorMessage = "Message size error."; break;
	case 0x80010034: errorMessage = "Name too long."; break;
	case 0x80010035: errorMessage = "No lock."; break;
	case 0x80010036: errorMessage = "Not empty."; break;
	case 0x80010037: errorMessage = "Not supported."; break;
	case 0x80010038: errorMessage = "File-system specific error."; break;
	case 0x80010039: errorMessage = "Overflow occured."; break;
	case 0x8001003A: errorMessage = "Filesystem not mounted."; break;
	case 0x8001003B: errorMessage = "Not SData."; break;
	case 0x8001003C: errorMessage = "Incorrect version in sys_load_param."; break;
	case 0x8001003D: errorMessage = "Pointer is null."; break;
	case 0x8001003E: errorMessage = "Pointer is null."; break;
	default: errorMessage = "An error has occurred."; break;
	}

	char errorCodeHex[12];
	sprintf(errorCodeHex, "\n(%08x)", errorCode);
	errorMessage.append(errorCodeHex);

	u64 status;
	int res = rMessageBox(errorMessage, "Error", rICON_ERROR | rOK);
	switch (res)
	{
	case rOK: status = CELL_MSGDIALOG_BUTTON_OK; break;
	default:
		if (res)
		{
			status = CELL_MSGDIALOG_BUTTON_INVALID;
			break;
		}

		status = CELL_MSGDIALOG_BUTTON_NONE;
		break;
	}

	if (callback)
		callback(status, userData);

	return CELL_OK;
}

int cellMsgDialogClose(float delay)
{
	cellSysutil->Warning("cellMsgDialogClose(delay=%f)", delay);

	MsgDialogState old = msgDialogOpen;
	if (!g_msg_dialog_state.compare_exchange_strong(old, msgDialogClose))
	{
		if (old == msgDialogNone)
		{
			return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
		}
		else
		{
			return CELL_SYSUTIL_ERROR_BUSY;
		}
	}

	if (delay < 0.0f) delay = 0.0f;
	m_wait_until = get_system_time() + (u64)(delay * 1000);
	return CELL_OK;
}

int cellMsgDialogAbort()
{
	cellSysutil->Warning("cellMsgDialogAbort()");

	MsgDialogState old = msgDialogOpen;
	if (!g_msg_dialog_state.compare_exchange_strong(old, msgDialogAbort))
	{
		if (old == msgDialogNone)
		{
			return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
		}
		else
		{
			return CELL_SYSUTIL_ERROR_BUSY;
		}
	}

	m_wait_until = get_system_time();
	return CELL_OK;
}

int cellMsgDialogProgressBarSetMsg(u32 progressBarIndex, mem_list_ptr_t<u8> msgString)
{
	cellSysutil->Warning("cellMsgDialogProgressBarSetMsg(progressBarIndex=%d, msgString_addr=0x%x): '%s'",
		progressBarIndex, msgString.GetAddr(), msgString.GetString());

	if (g_msg_dialog_state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= (m_gauge1 ? 1u : 0u) + (m_gauge2 ? 1u : 0u))
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	std::string text(msgString.GetString());

	wxGetApp().CallAfter([text, progressBarIndex]()
	{
		if (g_msg_dialog && !Emu.IsStopped())
		{
			if (progressBarIndex == 0 && m_text1) m_text1->SetLabelText(fmt::FromUTF8(text));
			if (progressBarIndex == 1 && m_text2) m_text2->SetLabelText(fmt::FromUTF8(text));
			g_msg_dialog->Layout();
			g_msg_dialog->Fit();
		}
	});
	return CELL_OK;
}

int cellMsgDialogProgressBarReset(u32 progressBarIndex)
{
	cellSysutil->Warning("cellMsgDialogProgressBarReset(progressBarIndex=%d)", progressBarIndex);

	if (g_msg_dialog_state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= (m_gauge1 ? 1u : 0u) + (m_gauge2 ? 1u : 0u))
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	wxGetApp().CallAfter([=]()
	{
		if (g_msg_dialog)
		{
			if (progressBarIndex == 0 && m_gauge1) m_gauge1->SetValue(0);
			if (progressBarIndex == 1 && m_gauge2) m_gauge2->SetValue(0);
		}
	});
	return CELL_OK;
}

int cellMsgDialogProgressBarInc(u32 progressBarIndex, u32 delta)
{
	cellSysutil->Warning("cellMsgDialogProgressBarInc(progressBarIndex=%d, delta=%d)", progressBarIndex, delta);

	if (g_msg_dialog_state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= (m_gauge1 ? 1u : 0u) + (m_gauge2 ? 1u : 0u))
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	wxGetApp().CallAfter([=]()
	{
		if (g_msg_dialog)
		{
			if (progressBarIndex == 0 && m_gauge1) m_gauge1->SetValue(m_gauge1->GetValue() + delta);
			if (progressBarIndex == 1 && m_gauge2) m_gauge2->SetValue(m_gauge2->GetValue() + delta);
		}
	});
	return CELL_OK;
}
