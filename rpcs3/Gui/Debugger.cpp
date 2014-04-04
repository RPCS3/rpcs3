#include "stdafx.h"
#include <wx/statline.h>

#include "Debugger.h"
#include "Emu/Memory/Memory.h"
#include "InterpreterDisAsm.h"

class DbgEmuPanel : public wxPanel
{
	AppConnector m_app_connector;

	wxButton* m_btn_run;
	wxButton* m_btn_stop;
	wxButton* m_btn_restart;

public:
	DbgEmuPanel(wxWindow* parent) : wxPanel(parent)
	{
		m_btn_run     = new wxButton(this, wxID_ANY, "Run");
		m_btn_stop    = new wxButton(this, wxID_ANY, "Stop");
		m_btn_restart = new wxButton(this, wxID_ANY, "Restart");

		wxBoxSizer& s_b_main = *new wxBoxSizer(wxHORIZONTAL);

		s_b_main.Add(m_btn_run,     wxSizerFlags().Border(wxALL, 5));
		s_b_main.Add(m_btn_stop,    wxSizerFlags().Border(wxALL, 5));
		s_b_main.Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), 0, wxEXPAND);
		s_b_main.Add(m_btn_restart, wxSizerFlags().Border(wxALL, 5));

		SetSizerAndFit(&s_b_main);
		Layout();

		UpdateUI();
		Connect(m_btn_run->GetId(),     wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DbgEmuPanel::OnRun));
		Connect(m_btn_stop->GetId(),    wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DbgEmuPanel::OnStop));
		Connect(m_btn_restart->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DbgEmuPanel::OnRestart));

		m_app_connector.Connect(wxEVT_DBG_COMMAND, wxCommandEventHandler(DbgEmuPanel::HandleCommand), (wxObject*)0, this);
	}

	void UpdateUI()
	{
		m_btn_run->Enable(!Emu.IsStopped());
		m_btn_stop->Enable(!Emu.IsStopped());
		m_btn_restart->Enable(!Emu.m_path.empty());
	}

	void OnRun(wxCommandEvent& event)
	{
		if(Emu.IsRunning())
		{
			Emu.Pause();
		}
		else if(Emu.IsPaused())
		{
			Emu.Resume();
		}
		else
		{
			Emu.Run();
		}
	}

	void OnStop(wxCommandEvent& event)
	{
		Emu.Stop();
	}

	void OnRestart(wxCommandEvent& event)
	{
		Emu.Stop();
		Emu.Load();
	}

	void HandleCommand(wxCommandEvent& event)
	{
		event.Skip();

		switch(event.GetId())
		{
		case DID_STOP_EMU:
			m_btn_run->SetLabel("Run");
		break;

		case DID_PAUSE_EMU:
			m_btn_run->SetLabel("Resume");
		break;

		case DID_START_EMU:
		case DID_RESUME_EMU:
			m_btn_run->SetLabel("Pause");
		break;

		case DID_EXIT_THR_SYSCALL:
			Emu.GetCPU().RemoveThread(((PPCThread*)event.GetClientData())->GetId());
		break;
		}

		UpdateUI();
	}
};

DebuggerPanel::DebuggerPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(400, 600), wxTAB_TRAVERSAL)
{
	m_aui_mgr.SetManagedWindow(this);

	m_aui_mgr.AddPane(new DbgEmuPanel(this), wxAuiPaneInfo().Top());
	m_aui_mgr.AddPane(new InterpreterDisAsmFrame(this), wxAuiPaneInfo().Center().CaptionVisible(false).CloseButton().MaximizeButton());
	m_aui_mgr.Update();
}

DebuggerPanel::~DebuggerPanel()
{
	m_aui_mgr.UnInit();
}

void DebuggerPanel::UpdateUI()
{
}
