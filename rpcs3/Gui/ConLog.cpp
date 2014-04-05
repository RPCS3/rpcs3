#include "stdafx.h"

#include "Gui/ConLog.h"
#include <wx/listctrl.h>
#include "Ini.h"
#include <fstream>
#include <vector>

LogWriter ConLog;
LogFrame* ConLogFrame;

std::mutex g_cs_conlog;

static const uint max_item_count = 500;
static const uint buffer_size = 1024 * 64;

static const std::string g_log_colors[] =
{
	"Black", "Green", "White", "Yellow", "Red",
};

struct LogPacket
{
	const std::string m_prefix;
	const std::string m_text;
	const std::string m_colour;

	LogPacket(const std::string& prefix, const std::string& text, const std::string& colour)
		: m_prefix(prefix)
		, m_text(text)
		, m_colour(colour)
	{
		
	}
};

struct _LogBuffer : public MTPacketBuffer<LogPacket>
{
	_LogBuffer() : MTPacketBuffer<LogPacket>(buffer_size)
	{
	}

	void _push(const LogPacket& data)
	{
		const u32 sprefix = data.m_prefix.length();
		const u32 stext   = data.m_text.length();
		const u32 scolour = data.m_colour.length();

		m_buffer.Reserve(
			sizeof(u32) + sprefix +
			sizeof(u32) + stext +
			sizeof(u32) + scolour);

		u32 c_put = m_put;

		memcpy(&m_buffer[c_put], &sprefix, sizeof(u32));
		c_put += sizeof(u32);
		memcpy(&m_buffer[c_put], data.m_prefix.c_str(), sprefix);
		c_put += sprefix;

		memcpy(&m_buffer[c_put], &stext, sizeof(u32));
		c_put += sizeof(u32);
		memcpy(&m_buffer[c_put], data.m_text.c_str(), stext);
		c_put += stext;

		memcpy(&m_buffer[c_put], &scolour, sizeof(u32));
		c_put += sizeof(u32);
		memcpy(&m_buffer[c_put], data.m_colour.c_str(), scolour);
		c_put += scolour;

		m_put = c_put;
		CheckBusy();
	}

	LogPacket _pop()
	{
		u32 c_get = m_get;

		const u32& sprefix = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		const std::string prefix( (const char*) &m_buffer[c_get], sprefix);
		c_get += sprefix;

		const u32& stext = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		const std::string text( (const char*) &m_buffer[c_get], stext);
		c_get += stext;

		const u32& scolour = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		const std::string colour( (const char*) &m_buffer[c_get], scolour);
		c_get += scolour;

		m_get = c_get;
		if(!HasNewPacket()) Flush();

		return LogPacket(prefix, text, colour);
	}
} LogBuffer;

LogWriter::LogWriter()
{
	if(!m_logfile.Open(_PRGNAME_ ".log", wxFile::write))
	{
#ifndef QT_UI
		wxMessageBox("Can't create log file! (" _PRGNAME_ ".log)", wxMessageBoxCaptionStr, wxICON_ERROR);
#endif
	}
}

void LogWriter::WriteToLog(const std::string& prefix, const std::string& value, u8 lvl/*, wxColour bgcolour*/)
{
	std::string new_prefix = prefix;
	if(!prefix.empty())
	{
		if(NamedThreadBase* thr = GetCurrentNamedThread())
		{
			new_prefix += " : " + thr->GetThreadName();
		}
	}

	if(m_logfile.IsOpened() && !new_prefix.empty())
		m_logfile.Write(fmt::FromUTF8("[" + new_prefix + "]: " + value + "\n"));

	if(!ConLogFrame || Ini.HLELogLvl.GetValue() == 4 || (lvl != 0 && lvl <= Ini.HLELogLvl.GetValue()))
		return;

	std::lock_guard<std::mutex> lock(g_cs_conlog);

#ifdef QT_UI
	// TODO: Use ThreadBase instead, track main thread id
	if(QThread::currentThread() == qApp->thread())
#else
	if(wxThread::IsMain())
#endif
	{
		while(LogBuffer.IsBusy())
		{
			// need extra break condition?
			wxYieldIfNeeded();
		}
	}
	else
	{
		while (LogBuffer.IsBusy())
		{
			if (Emu.IsStopped())
			{
				break;
			}
			Sleep(1);
		}
	}

	//if(LogBuffer.put == LogBuffer.get) LogBuffer.Flush();

	LogBuffer.Push(LogPacket(new_prefix, value, g_log_colors[lvl]));
}


void LogWriter::SkipLn()
{
	WriteToLog("", "", 0);
}

BEGIN_EVENT_TABLE(LogFrame, wxPanel)
	EVT_CLOSE(LogFrame::OnQuit)
END_EVENT_TABLE()

LogFrame::LogFrame(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(600, 500))
	, ThreadBase("LogThread")
	, m_log(*new wxListView(this))
{
	m_log.InsertColumn(0, wxEmptyString);
	m_log.InsertColumn(1, "Log");
	m_log.SetBackgroundColour(wxColour("Black"));

	wxBoxSizer& s_main = *new wxBoxSizer(wxVERTICAL);
	s_main.Add(&m_log, 1, wxEXPAND);
	SetSizer(&s_main);
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
