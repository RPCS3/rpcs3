#include "stdafx.h"

#include <wx/listctrl.h>
#include <fstream>
#include <vector>
#include <mutex>

#include "Ini.h"
#include "Utilities/Thread.h"
#include "Utilities/StrFmt.h"
#include "Emu/ConLog.h"
#include "Gui/ConLogFrame.h"

#include "Utilities/BEType.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"


LogFrame* ConLogFrame;

BEGIN_EVENT_TABLE(LogFrame, wxPanel)
	EVT_CLOSE(LogFrame::OnQuit)
END_EVENT_TABLE()

LogFrame::LogFrame(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(600, 500))
	, ThreadBase("LogThread")
	, m_log(*new wxListView(this))
{
	m_log.InsertColumn(0, "Thread");
	m_log.InsertColumn(1, "Log");
	m_log.SetBackgroundColour(wxColour("Black"));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);
	s_main->Add(&m_log, 1, wxEXPAND);
	SetSizer(s_main);
	Layout();
	
	Show();
	ThreadBase::Start();
}

LogFrame::~LogFrame()
{
}

bool LogFrame::Close(bool force)
{
	Stop(false);
	ConLogFrame = nullptr;
	return wxWindowBase::Close(force);
}

void LogFrame::Task()
{
	while(!TestDestroy())
	{
		if(!LogBuffer.HasNewPacket())
		{
			Sleep(1);
			continue;
		}
		else
		{
			wxThread::Yield();
		}

		const LogPacket item = LogBuffer.Pop();

		wxListView& m_log = this->m_log; //makes m_log capturable by the lambda
		//queue adding the log message to the gui element in the main thread
		wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter([item, &m_log]()
		{
			while (m_log.GetItemCount() > max_item_count)
			{
				m_log.DeleteItem(0);
				wxThread::Yield();
			}

			const int cur_item = m_log.GetItemCount();

			m_log.InsertItem(cur_item, fmt::FromUTF8(item.m_prefix));
			m_log.SetItem(cur_item, 1, fmt::FromUTF8(item.m_text));
			m_log.SetItemTextColour(cur_item, fmt::FromUTF8(item.m_colour));
			m_log.SetColumnWidth(0, -1);
			m_log.SetColumnWidth(1, -1);
		});

#ifdef _WIN32
		::SendMessage((HWND)m_log.GetHWND(), WM_VSCROLL, SB_BOTTOM, 0);
#endif
	}

	LogBuffer.Flush();
}

void LogFrame::OnQuit(wxCloseEvent& event)
{
	Stop(false);
	ConLogFrame = nullptr;
	event.Skip();
}
