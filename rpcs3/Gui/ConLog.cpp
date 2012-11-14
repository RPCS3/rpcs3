#include "stdafx.h"

#include "Gui/ConLog.h"
#include <wx/listctrl.h>
#include "Ini.h"

LogWriter ConLog;
LogFrame* ConLogFrame;
wxMutex mtx_wait;

static const uint max_item_count = 500;
static const uint buffer_size = 1024 * 64;

wxSemaphore m_conlog_sem;

struct LogPacket
{
	wxString m_prefix;
	wxString m_text;
	wxString m_colour;

	LogPacket(const wxString& prefix, const wxString& text, const wxString& colour)
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

	void Push(const LogPacket& data)
	{
		const u32 sprefix	= data.m_prefix.Len();
		const u32 stext		= data.m_text.Len();
		const u32 scolour	= data.m_colour.Len();

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

	LogPacket Pop()
	{
		LogPacket ret;

		u32 c_get = m_get;

		const u32& sprefix = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		if(sprefix) memcpy(wxStringBuffer(ret.m_prefix, sprefix), &m_buffer[c_get], sprefix);
		c_get += sprefix;

		const u32& stext = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		if(stext) memcpy(wxStringBuffer(ret.m_text, stext), &m_buffer[c_get], stext);
		c_get += stext;

		const u32& scolour = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		if(scolour) memcpy(wxStringBuffer(ret.m_colour, scolour), &m_buffer[c_get], scolour);
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
		wxMessageBox("Cann't create log file! (" _PRGNAME_ ".log)", wxMessageBoxCaptionStr, wxICON_ERROR);
	}
}

void LogWriter::WriteToLog(wxString prefix, wxString value, wxString colour/*, wxColour bgcolour*/)
{
	if(m_logfile.IsOpened())
		m_logfile.Write((prefix.IsEmpty() ? wxEmptyString : "[" + prefix + "]: ") + value + "\n");

	if(!ConLogFrame) return;

	wxMutexLocker locker(mtx_wait);

	while(LogBuffer.IsBusy()) Sleep(1);

	//if(LogBuffer.put == LogBuffer.get) LogBuffer.Flush();

	LogBuffer.Push(LogPacket(prefix, value, colour));
}

wxString FormatV(const wxString fmt, va_list args)
{
	int length = 256;
	wxString str;
	
	for(;;)
	{
		str.Clear();
		wxStringBuffer buf(str, length+1);
		memset(buf, 0, length+1);
		if(vsnprintf(buf, length, fmt, args) != -1) break;
		length *= 2;
	}

	return str;
}

void LogWriter::Write(const wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	const wxString& frmt = FormatV(fmt, list);

	va_end(list);

	WriteToLog("!", frmt, "White");
}

void LogWriter::Error(const wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	const wxString& frmt = FormatV(fmt, list);

	va_end(list);

	WriteToLog("E", frmt, "Red");
}

void LogWriter::Warning(const wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	const wxString& frmt = FormatV(fmt, list);

	va_end(list);

	WriteToLog("W", frmt, "Yellow");
}

void LogWriter::SkipLn()
{
	WriteToLog(wxEmptyString, wxEmptyString, "Black");
}

BEGIN_EVENT_TABLE(LogFrame, FrameBase)
	EVT_CLOSE(LogFrame::OnQuit)
END_EVENT_TABLE()

LogFrame::LogFrame()
	: FrameBase(NULL, wxID_ANY, "Log Console", wxEmptyString, wxSize(600, 450), wxDefaultPosition)
	, ThreadBase(false, "LogThread")
	, m_log(*new wxListView(this))
{
	wxBoxSizer& s_panel( *new wxBoxSizer(wxHORIZONTAL) );

	s_panel.Add(&m_log, wxSizerFlags().Expand());

	m_log.InsertColumn(0, wxEmptyString);
	m_log.InsertColumn(1, "Log");
	m_log.SetBackgroundColour(wxColour("Black"));

	m_log.SetColumnWidth(0, 18);

	SetSizerAndFit( &s_panel );

	Connect( wxEVT_SIZE, wxSizeEventHandler(LogFrame::OnResize) );
	Connect( m_log.GetId(), wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( LogFrame::OnColBeginDrag ));

	Show();
	ThreadBase::Start();
}

LogFrame::~LogFrame()
{
}

bool LogFrame::Close(bool force)
{
	Stop();
	ConLogFrame = NULL;
	return FrameBase::Close(force);
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

		const LogPacket item = LogBuffer.Pop();

		while(m_log.GetItemCount() > max_item_count)
		{
			m_log.DeleteItem(0);
		}

		const int cur_item = m_log.GetItemCount();

		m_log.InsertItem(cur_item, item.m_prefix);
		m_log.SetItem(cur_item, 1, item.m_text);
		m_log.SetItemTextColour(cur_item, item.m_colour);

		::SendMessage((HWND)m_log.GetHWND(), WM_VSCROLL, SB_BOTTOM, 0);
	}

	LogBuffer.Flush();
}

void LogFrame::OnColBeginDrag(wxListEvent& event)
{
	event.Veto();
}

void LogFrame::OnResize(wxSizeEvent& event)
{
	const wxSize size( GetClientSize() );

	m_log.SetSize( size );
	m_log.SetColumnWidth(1, size.GetWidth() - 4 - m_log.GetColumnWidth(0));

	event.Skip();
}

void LogFrame::OnQuit(wxCloseEvent& event)
{
	Stop();
	ConLogFrame = NULL;
	event.Skip();
}