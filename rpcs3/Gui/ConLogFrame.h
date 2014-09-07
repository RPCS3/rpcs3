#pragma once
#include <wx/dataobj.h>

namespace Log
{
	struct LogListener;
}

class LogFrame
	: public wxPanel
{
	std::shared_ptr<Log::LogListener> listener;
	wxAuiNotebook m_tabs;
	wxTextCtrl *m_log;
	wxTextCtrl *m_tty;
	//Copy Action in Context Menu
	wxTextDataObject* m_tdo;

public:
	LogFrame(wxWindow* parent);
	LogFrame(LogFrame &other) = delete;
	~LogFrame();

	bool Close(bool force = false);

private:
	virtual void Task(){};

	void OnQuit(wxCloseEvent& event);
	void OnRightClick(wxMouseEvent& event);	//Show context menu
	void OnContextMenu(wxCommandEvent& event);	//After select

	DECLARE_EVENT_TABLE();
};