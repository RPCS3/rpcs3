#include "stdafx.h"
#include "stdafx_gui.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "rpcs3.h"
#include "Debugger.h"
#include "InterpreterDisAsm.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/CPU/CPUThread.h"

extern bool user_asked_for_frame_capture;

class DbgEmuPanel : public wxPanel
{
	wxButton* m_btn_run;
	wxButton* m_btn_stop;
	wxButton* m_btn_restart;
	wxButton* m_btn_capture_frame;

public:
	DbgEmuPanel(wxWindow* parent) : wxPanel(parent)
	{
		m_btn_run = new wxButton(this, wxID_ANY, "Run");
		m_btn_run->Bind(wxEVT_BUTTON, &DbgEmuPanel::OnRun, this);

		m_btn_stop = new wxButton(this, wxID_ANY, "Stop");
		m_btn_stop->Bind(wxEVT_BUTTON, &DbgEmuPanel::OnStop, this);

		m_btn_restart = new wxButton(this, wxID_ANY, "Restart");
		m_btn_restart->Bind(wxEVT_BUTTON, &DbgEmuPanel::OnRestart, this);

		m_btn_capture_frame = new wxButton(this, wxID_ANY, "Capture frame");
		m_btn_capture_frame->Bind(wxEVT_BUTTON, &DbgEmuPanel::OnCaptureFrame, this);

		wxBoxSizer* s_b_main = new wxBoxSizer(wxHORIZONTAL);
		s_b_main->Add(m_btn_run,     wxSizerFlags().Border(wxALL, 5));
		s_b_main->Add(m_btn_stop,    wxSizerFlags().Border(wxALL, 5));
		s_b_main->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), 0, wxEXPAND);
		s_b_main->Add(m_btn_restart, wxSizerFlags().Border(wxALL, 5));
		s_b_main->Add(m_btn_capture_frame, wxSizerFlags().Border(wxALL, 5));

		SetSizerAndFit(s_b_main);
		Layout();

		UpdateUI();
		wxGetApp().Bind(wxEVT_DBG_COMMAND, &DbgEmuPanel::HandleCommand, this);
	}

	void UpdateUI()
	{
		m_btn_run->Enable(!Emu.IsStopped());
		m_btn_stop->Enable(!Emu.IsStopped());
		m_btn_restart->Enable(!Emu.GetPath().empty());
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

	void OnCaptureFrame(wxCommandEvent& event)
	{
		user_asked_for_frame_capture = true;
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
