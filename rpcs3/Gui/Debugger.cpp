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
		m_btn_run		= new wxButton(this, wxID_ANY, "Run");
		m_btn_stop		= new wxButton(this, wxID_ANY, "Stop");
		m_btn_restart	= new wxButton(this, wxID_ANY, "Restart");

		wxBoxSizer& s_b_main = *new wxBoxSizer(wxHORIZONTAL);

		s_b_main.Add(m_btn_run,		wxSizerFlags().Border(wxALL, 5));
		s_b_main.Add(m_btn_stop,	wxSizerFlags().Border(wxALL, 5));
		s_b_main.Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), 0, wxEXPAND);
		s_b_main.Add(m_btn_restart,	wxSizerFlags().Border(wxALL, 5));

		SetSizer(&s_b_main);
		Layout();

		UpdateUI();
		Connect(m_btn_run->GetId(),		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(DbgEmuPanel::OnRun));
		Connect(m_btn_stop->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(DbgEmuPanel::OnStop));
		Connect(m_btn_restart->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(DbgEmuPanel::OnRestart));

		m_app_connector.Connect(wxEVT_DBG_COMMAND, wxCommandEventHandler(DbgEmuPanel::HandleCommand), (wxObject*)0, this);
	}

	void UpdateUI()
	{
		m_btn_run->Enable(!Emu.IsStopped());
		m_btn_stop->Enable(!Emu.IsStopped());
		m_btn_restart->Enable(!Emu.m_path.IsEmpty());
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
		}

		UpdateUI();
		event.Skip();
	}
};

DebuggerPanel::DebuggerPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(400, 600), wxTAB_TRAVERSAL)
{
	m_aui_mgr.SetManagedWindow(this);

	m_nb = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT |
		wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_SCROLL_BUTTONS |
		wxAUI_NB_WINDOWLIST_BUTTON | wxAUI_NB_TAB_MOVE | wxNO_BORDER);

	m_aui_mgr.AddPane(new DbgEmuPanel(this), wxAuiPaneInfo().Top());
	m_aui_mgr.AddPane(m_nb, wxAuiPaneInfo().Center().CaptionVisible(false).CloseButton().MaximizeButton());
	m_aui_mgr.Update();

	m_app_connector.Connect(wxEVT_DBG_COMMAND, wxCommandEventHandler(DebuggerPanel::HandleCommand), (wxObject*)0, this);
}

DebuggerPanel::~DebuggerPanel()
{
	m_aui_mgr.UnInit();
}

void DebuggerPanel::UpdateUI()
{
}

void DebuggerPanel::HandleCommand(wxCommandEvent& event)
{
	PPCThread* thr = (PPCThread*)event.GetClientData();

	switch(event.GetId())
	{
	case DID_CREATE_THREAD:
		m_nb->AddPage(new InterpreterDisAsmFrame(m_nb, thr), thr->GetFName());
	break;

	case DID_REMOVE_THREAD:
		for(uint i=0; i<m_nb->GetPageCount(); ++i)
		{
			InterpreterDisAsmFrame* page = (InterpreterDisAsmFrame*)m_nb->GetPage(i);

			if(page->CPU.GetId() == thr->GetId())
			{
				m_nb->DeletePage(i);
				break;
			}
		}
	break;
	}

	event.Skip();
}