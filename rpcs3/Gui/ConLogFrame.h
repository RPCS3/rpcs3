#pragma once
#include "Emu/ConLog.h"


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

extern LogFrame* ConLogFrame;