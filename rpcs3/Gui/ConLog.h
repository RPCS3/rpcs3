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

	void WriteToLog(const std::string& prefix, const std::string& value, u8 lvl);

public:
	LogWriter();

	template <typename ...Arg>
	void Write(const std::string &fmt, Arg&&... args)
	{
		if (!Ini.LogWrite.GetValue())
			return;

		std::string frmt = fmt::Format(fmt, std::forward<Arg>(args)...);
		WriteToLog("!", frmt, 2);
	}
	
	template <typename ...Arg>
	void Error(const std::string &fmt, Arg&&... args)
	{
		if (!Ini.LogError.GetValue())
			return;

		std::string frmt = fmt::Format(fmt, std::forward<Arg>(args)...);
		WriteToLog("E", frmt, 4);
	}
	
	template <typename ...Arg>
	void Warning(const std::string &fmt, Arg&&... args)
	{
		if (!Ini.LogWarning.GetValue())
			return;

		std::string frmt = fmt::Format(fmt, std::forward<Arg>(args)...);
		WriteToLog("W", frmt, 3);
	}
	
	template <typename ...Arg>
	void Success(const std::string &fmt, Arg&&... args)
	{
		if (!Ini.LogSuccess.GetValue())
			return;

		std::string frmt = fmt::Format(fmt, std::forward<Arg>(args)...);
		WriteToLog("S", frmt, 1);
	}
	
	virtual void SkipLn();	
};

class LogFrame 
	: public FrameBase
	, public ThreadBase
{
	wxListView& m_log;

public:
	LogFrame(wxWindow *parent);
	~LogFrame();

	bool Close(bool force = false);

private:
	virtual void Task();

	void Settings(wxCommandEvent& event);
	void UpdateListSize(wxSizeEvent& event);
	void OnQuit(wxCloseEvent& event);
	
	DECLARE_EVENT_TABLE();
};

extern LogWriter ConLog;
extern LogFrame* ConLogFrame;
