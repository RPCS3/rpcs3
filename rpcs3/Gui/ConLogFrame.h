#pragma once

class LogFrame : public wxPanel
{
	fs::file m_tty_file;

	wxAuiNotebook m_tabs;
	wxTextCtrl *m_log;
	wxTextCtrl *m_tty;

	//Copy Action in Context Menu
	wxTextDataObject* m_tdo;

	wxTimer m_timer;

	YAML::Node m_cfg_level;
	YAML::Node m_cfg_tty;

	logs::level get_cfg_level() const
	{
		return static_cast<logs::level>(m_cfg_level.as<uint>(4));
	}

	bool get_cfg_tty() const
	{
		return m_cfg_tty.as<bool>(true);
	}

public:
	LogFrame(wxWindow* parent);
	LogFrame(LogFrame &other) = delete;
	~LogFrame();

	bool Close(bool force = false);

private:
	virtual void Task(){};

	void OnQuit(wxCloseEvent& event);
	void OnRightClick(wxMouseEvent& event); // Show context menu
	void OnContextMenu(wxCommandEvent& event); // After select
	void OnTimer(wxTimerEvent& event);

	DECLARE_EVENT_TABLE();
};
