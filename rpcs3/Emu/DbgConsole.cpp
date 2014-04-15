#include "stdafx.h"
#include "DbgConsole.h"

BEGIN_EVENT_TABLE(DbgConsole, FrameBase)
	EVT_CLOSE(DbgConsole::OnQuit)
END_EVENT_TABLE()

DbgConsole::DbgConsole()
	: FrameBase(nullptr, wxID_ANY, "DbgConsole", "", wxDefaultSize, wxDefaultPosition, wxDEFAULT_FRAME_STYLE, true)
	, ThreadBase("DbgConsole thread")
	, m_output(nullptr)
{
	m_console = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxSize(500, 500), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
	m_console->SetBackgroundColour(wxColor("Black"));
	m_console->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	m_color_white = new wxTextAttr(wxColour(255, 255, 255));
	m_color_red = new wxTextAttr(wxColour(255, 0, 0));

	if (Ini.HLESaveTTY.GetValue())
		m_output = new wxFile("tty.log", wxFile::write);
}

DbgConsole::~DbgConsole()
{
	ThreadBase::Stop();
	m_dbg_buffer.Flush();

	safe_delete(m_console);
	safe_delete(m_color_white);
	safe_delete(m_color_red);
	safe_delete(m_output);
}

void DbgConsole::Write(int ch, const std::string& text)
{
	while (m_dbg_buffer.IsBusy())
	{
		if (Emu.IsStopped())
		{
			return;
		}
		Sleep(1);
	}
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
			if (Emu.IsStopped())
			{
				break;
			}
			Sleep(1);
			continue;
		}

		DbgPacket packet = m_dbg_buffer.Pop();
		m_console->SetDefaultStyle(packet.m_ch == 1 ? *m_color_red : *m_color_white);
		m_console->SetInsertionPointEnd();
		m_console->WriteText(fmt::FromUTF8(packet.m_text));

		if (m_output && Ini.HLESaveTTY.GetValue())
			m_output->Write(fmt::FromUTF8(packet.m_text));

		if(!DbgConsole::IsShown()) Show();
	}
}

void DbgConsole::OnQuit(wxCloseEvent& event)
{
	ThreadBase::Stop(false);
	Hide();

	if (m_output)
	{
		m_output->Close();
		m_output = nullptr;
	}

	//event.Skip();
}