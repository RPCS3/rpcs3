#include "stdafx_gui.h"

#include <wx/listctrl.h>
#include <wx/clipbrd.h>
#include <fstream>

#include "Ini.h"
#include "Utilities/Log.h"
#include "Gui/ConLogFrame.h"

wxDEFINE_EVENT(EVT_LOG_COMMAND, wxCommandEvent);

//amount of memory in bytes used to buffer log messages for the gui
const int BUFFER_MAX_SIZE = 1048576; // 1MB

//amount of characters in the TextCtrl text-buffer for the emulation log
const int GUI_BUFFER_MAX_SIZE = 1048576; // 1MB

enum
{
	id_log_copy,	//Copy log to ClipBoard
	id_log_clear	//Clear log
};

struct wxWriter : Log::LogListener
{
	wxTextCtrl *m_log;
	wxTextCtrl *m_tty;
	wxTextAttr m_color_white;
	wxTextAttr m_color_yellow;
	wxTextAttr m_color_red;
	MTRingbuffer<char, BUFFER_MAX_SIZE> messages;
	std::atomic<bool> newLog;
	bool inited;

	wxWriter(wxTextCtrl* p_log, wxTextCtrl* p_tty) :
		m_color_white(wxColour(255, 255, 255)) ,
		m_color_yellow(wxColour(255, 255, 0)) ,
		m_color_red(wxColour(255, 0, 0)) ,
		m_log(p_log),
		m_tty(p_tty),
		newLog(false),
		inited(false)
	{
			m_log->Bind(EVT_LOG_COMMAND, [this](wxCommandEvent &evt){this->write(evt);});
	}

	wxWriter(wxWriter &other) = delete;

	//read messages from buffer and write them to the screen
	void write(wxCommandEvent &)
	{
		if (messages.size() > 0)
		{
			messages.lockGet();
			size_t size = messages.size();
			std::vector<char> local_messages(size);
			messages.popN(&local_messages.front(), size);
			messages.unlockGet();
			newLog = false;

			u32 cursor = 0;
			u32 removed = 0;
			while (cursor < local_messages.size())
			{
				Log::LogMessage msg = Log::LogMessage::deserialize(local_messages.data() + cursor, &removed);
				cursor += removed;
				if (removed <= 0)
				{
					break;
				}
				wxTextCtrl *llogcon = (msg.mType == Log::TTY) ? m_tty : m_log;
				if (llogcon)
				{
					switch (msg.mServerity)
					{
					case Log::Notice:
						llogcon->SetDefaultStyle(m_color_white);
						break;
					case Log::Warning:
						llogcon->SetDefaultStyle(m_color_yellow);
						break;
					case Log::Error:
						llogcon->SetDefaultStyle(m_color_red);
						break;
					default:
						break;
					}
					llogcon->AppendText(fmt::FromUTF8(msg.mText));
				}
			}
			if (m_log->GetLastPosition() > GUI_BUFFER_MAX_SIZE)
			{
				m_log->Remove(0, m_log->GetLastPosition() - (GUI_BUFFER_MAX_SIZE/2));
			}
		}
	}

	//put message into the log buffer
	void log(Log::LogMessage msg)
	{
		u8 logLevel = Ini.HLELogLvl.GetValue();
		if (msg.mType != Log::TTY && logLevel != 0)
		{
			if (logLevel > static_cast<u32>(msg.mServerity))
			{
				return;
			}
		}

		size_t size = msg.size();
		std::vector<char> temp_buffer(size);
		msg.serialize(temp_buffer.data());
		messages.pushRange(temp_buffer.begin(), temp_buffer.end());
		if (!newLog.load())
		{
			newLog = true;
			m_log->GetEventHandler()->QueueEvent(new wxCommandEvent(EVT_LOG_COMMAND));
		}
	}
};

BEGIN_EVENT_TABLE(LogFrame, wxPanel)
EVT_CLOSE(LogFrame::OnQuit)
END_EVENT_TABLE()

LogFrame::LogFrame(wxWindow* parent)
: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(600, 500))
, m_tabs(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS)
, m_log(new wxTextCtrl(&m_tabs, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2))
, m_tty(new wxTextCtrl(&m_tabs, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2))
{
	listener.reset(new wxWriter(m_log,m_tty));
	m_tty->SetBackgroundColour(wxColour("Black"));
	m_log->SetBackgroundColour(wxColour("Black"));
	m_tty->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_tabs.AddPage(m_log, "Log");
	m_tabs.AddPage(m_tty, "TTY");

	Log::LogManager::getInstance().addListener(listener);

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);
	s_main->Add(&m_tabs, 1, wxEXPAND);
	SetSizer(s_main);
	Layout();

	m_log->Bind(wxEVT_RIGHT_DOWN, &LogFrame::OnRightClick, this);
	Bind(wxEVT_MENU, &LogFrame::OnContextMenu, this, id_log_clear);
	Bind(wxEVT_MENU, &LogFrame::OnContextMenu, this, id_log_copy);

	Show();
}

LogFrame::~LogFrame()
{
	Log::LogManager::getInstance().removeListener(listener);
}

bool LogFrame::Close(bool force)
{
	return wxWindowBase::Close(force);
}

void LogFrame::OnQuit(wxCloseEvent& event)
{
	event.Skip();
}

//Deals with the RightClick on Log Console, shows up the Context Menu.
void LogFrame::OnRightClick(wxMouseEvent& event)
{
	wxMenu* menu = new wxMenu();

	menu->Append(id_log_copy, "&Copy");
	menu->AppendSeparator();
	menu->Append(id_log_clear, "C&lear");

	PopupMenu(menu);
}
//Well you can bind more than one control to a single handler.
void LogFrame::OnContextMenu(wxCommandEvent& event)
{
	int id = event.GetId();
	switch (id)
	{
	case id_log_clear:
		m_log->Clear();
		break;
	case id_log_copy:
		if (wxTheClipboard->Open())
		{
			m_tdo = new wxTextDataObject(m_log->GetStringSelection());
			if (m_tdo->GetTextLength() > 0)
			{
				wxTheClipboard->SetData(new wxTextDataObject(m_log->GetStringSelection()));
			}
			wxTheClipboard->Close();
		}
		break;
	default:
		event.Skip();
	}
}