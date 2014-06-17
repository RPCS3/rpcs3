#pragma once
#include "Utilities/Log.h"


class LogFrame
	: public wxPanel
{
	std::shared_ptr<Log::LogListener> listener;
	wxAuiNotebook m_tabs;
	wxTextCtrl *m_log;
	wxTextCtrl *m_tty;

public:
	LogFrame(wxWindow* parent);
	LogFrame(LogFrame &other) = delete;
	~LogFrame();

	bool Close(bool force = false);

private:
	virtual void Task(){};

	void OnQuit(wxCloseEvent& event);

	DECLARE_EVENT_TABLE();
};