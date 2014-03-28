#pragma once
#include <wx/listctrl.h>
#include "Ini.h"
#include "Gui/FrameBase.h"

class LogWriter
{
	wxFile m_logfile;
	wxColour m_txtcolour;

	//wxString m_prefix;
	//wxString m_value;

	virtual void WriteToLog(wxString prefix, wxString value, u8 lvl);

public:
	LogWriter();

	virtual void Write(const wxString fmt, ...);
	virtual void Error(const wxString fmt, ...);
	virtual void Warning(const wxString fmt, ...);
	virtual void Success(const wxString fmt, ...);
	virtual void SkipLn();	
};

class LogFrame 
	: public wxPanel
	, public ThreadBase
{
	wxListView& m_log;

public:
	LogFrame(wxWindow* parent);
	~LogFrame();

	bool Close(bool force = false);

private:
	virtual void Task();

	void OnQuit(wxCloseEvent& event);
	
	DECLARE_EVENT_TABLE();
};

extern LogWriter ConLog;
extern LogFrame* ConLogFrame;
