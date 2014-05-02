#pragma once

class DbgFrame
	: public FrameBase
	, public ThreadBase
{
	rFile* m_output;
	wxTextCtrl* m_console;
	wxTextAttr* m_color_white;
	wxTextAttr* m_color_red;
	_DbgBuffer m_dbg_buffer;

public:
	DbgFrame();
	~DbgFrame();
	void Write(int ch, const std::string& text);
	void Clear();
	virtual void Task();

private:
	void OnQuit(wxCloseEvent& event);
	DECLARE_EVENT_TABLE();
};
