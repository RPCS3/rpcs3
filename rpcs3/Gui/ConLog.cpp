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

struct LogPacket
{
	std::string m_prefix;
	std::string m_text;
	std::string m_colour;

	LogPacket(const std::string& prefix, const std::string& text, const std::string& colour)
		: m_prefix(prefix)
		, m_text(text)
		, m_colour(colour)
	{
		
	}

	LogPacket()
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
		const u32 sprefix	= data.m_prefix.length();
		const u32 stext		= data.m_text.length();
		const u32 scolour	= data.m_colour.length();

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
		LogPacket ret;

		u32 c_get = m_get;

		const u32& sprefix = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		ret.m_prefix.resize(sprefix);
		if(sprefix) memcpy((void*)ret.m_prefix.c_str(), &m_buffer[c_get], sprefix);
		c_get += sprefix;

		const u32& stext = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		ret.m_text.resize(stext);
		if(stext) memcpy((void*)ret.m_text.c_str(), &m_buffer[c_get], stext);
		c_get += stext;

		const u32& scolour = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		ret.m_colour.resize(scolour);
		if(scolour) memcpy((void*)ret.m_colour.c_str(), &m_buffer[c_get], scolour);
		c_get += scolour;

		m_get = c_get;
		if(!HasNewPacket()) Flush();

		return ret;
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

void LogWriter::WriteToLog(std::string prefix, std::string value, std::string colour/*, wxColour bgcolour*/)
{
	if(!prefix.empty())
	{
		if(NamedThreadBase* thr = GetCurrentNamedThread())
		{
			prefix += " : " + thr->GetThreadName();
		}
	}

	if(m_logfile.IsOpened())
		m_logfile.Write(wxString(prefix.empty() ? "" : std::string("[" + prefix + "]: ") + value + "\n").wx_str());

	if(!ConLogFrame) return;

	std::lock_guard<std::mutex> lock(g_cs_conlog);

#ifdef QT_UI
	// TODO: Use ThreadBase instead, track main thread id
	if(QThread::currentThread() == qApp->thread())
#else
	if(wxThread::IsMain())
#endif
	{
		while(LogBuffer.IsBusy()) wxYieldIfNeeded();
	}
	else
	{
		while(LogBuffer.IsBusy()) Sleep(1);
	}

	//if(LogBuffer.put == LogBuffer.get) LogBuffer.Flush();

	LogBuffer.Push(LogPacket(prefix, value, colour));
}

void LogWriter::Write(const wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	wxString frmt;
	frmt = wxString::FormatV(fmt, list);

	va_end(list);

	WriteToLog("!", (const char *)frmt.ToAscii(), "White");
}

void LogWriter::Error(const wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	wxString frmt;
	frmt = wxString::FormatV(fmt, list);

	va_end(list);

	WriteToLog("E", static_cast<const char *>(frmt), "Red");
}

void LogWriter::Warning(const wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	wxString frmt;
	frmt = wxString::FormatV(fmt, list);

	va_end(list);

	WriteToLog("W", static_cast<const char *>(frmt), "Yellow");
}

void LogWriter::Success(const wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	wxString frmt;
	frmt = wxString::FormatV(fmt, list);

	va_end(list);

	WriteToLog("S", static_cast<const char *>(frmt), "Green");
}

void LogWriter::SkipLn()
{
	WriteToLog("", "", "Black");
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

		while(m_log.GetItemCount() > max_item_count)
		{
			m_log.DeleteItem(0);
			wxThread::Yield();
		}

		const int cur_item = m_log.GetItemCount();

		m_log.InsertItem(cur_item, wxString(item.m_prefix).wx_str());
		m_log.SetItem(cur_item, 1, wxString(item.m_text).wx_str());
		m_log.SetItemTextColour(cur_item, wxString(item.m_colour).wx_str());
		m_log.SetColumnWidth(0, -1); // crashes on exit
		m_log.SetColumnWidth(1, -1);

		::SendMessage((HWND)m_log.GetHWND(), WM_VSCROLL, SB_BOTTOM, 0);
	}

	LogBuffer.Flush();
}

void LogFrame::OnQuit(wxCloseEvent& event)
{
	Stop(false);
	ConLogFrame = nullptr;
	event.Skip();
}
