#include "stdafx.h"
#include "DbgConsole.h"

BEGIN_EVENT_TABLE(DbgConsole, FrameBase)
	EVT_CLOSE(DbgConsole::OnQuit)
END_EVENT_TABLE()

DbgConsole::DbgConsole()
	: FrameBase(nullptr, wxID_ANY, "DbgConsole", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxDEFAULT_FRAME_STYLE, true)
	, ThreadBase("DbgConsole thread")
{
	m_console = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxSize(500, 500), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
	m_console->SetBackgroundColour(wxColor("Black"));
	m_console->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	m_color_white = new wxTextAttr(wxColour(255, 255, 255));
	m_color_red = new wxTextAttr(wxColour(255, 0, 0));
}

DbgConsole::~DbgConsole()
{
	ThreadBase::Stop();
	m_dbg_buffer.Flush();
}

void DbgConsole::Write(int ch, const wxString& text)
{
	while(m_dbg_buffer.IsBusy()) Sleep(1);
	m_dbg_buffer.Push(DbgPacket(ch, text));

	if(!IsAlive()) Start();
}

void DbgConsole::Clear()
{
	m_console->Clear();
}

void DbgConsole::Task()
{
	while(!TestDestroy())
	{
		if(!m_dbg_buffer.HasNewPacket())
		{
			Sleep(1);
			continue;
		}

		DbgPacket packet = m_dbg_buffer.Pop();
		m_console->SetDefaultStyle(packet.m_ch == 1 ? *m_color_red : *m_color_white);
		m_console->SetInsertionPointEnd();
		m_console->WriteText(packet.m_text);

		if(!DbgConsole::IsShown()) Show();
	}
}

void DbgConsole::OnQuit(wxCloseEvent& event)
{
	ThreadBase::Stop(false);
	Hide();
	//event.Skip();
}