#include "stdafx.h"
#include "stdafx_gui.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "rpcs3.h"
#include "Debugger.h"
#include "InterpreterDisAsm.h"
#include "Emu/CPU/CPUThread.h"

extern bool user_asked_for_frame_capture;

class DbgEmuPanel : public wxPanel
{
	wxButton* m_btn_run;
	wxButton* m_btn_stop;
	wxButton* m_btn_restart;
	wxButton* m_btn_capture_frame;
	system_state m_last_status = system_state::ready;

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
	}

	void UpdateUI()
	{
		const auto status = Emu.GetStatus();

		if (m_last_status != status)
		{
			m_last_status = status;

			m_btn_run->Enable(status != system_state::stopped);
			m_btn_stop->Enable(status != system_state::stopped);
			m_btn_restart->Enable(!Emu.GetPath().empty());
			m_btn_run->SetLabel(status == system_state::paused ? "Resume" : status == system_state::running ? "Pause" : "Run");
		}
	}

	void OnRun(wxCommandEvent& event)
	{
		if (Emu.IsReady())
		{
			Emu.Run();
		}
		else if (Emu.IsRunning())
		{
			Emu.Pause();
		}
		else if (Emu.IsPaused())
		{
			Emu.Resume();
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
};

DebuggerPanel::DebuggerPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(400, 600), wxTAB_TRAVERSAL)
{
	m_aui_mgr.SetManagedWindow(this);

	m_dbg_panel = new DbgEmuPanel(this);
	m_disasm_frame = new InterpreterDisAsmFrame(this);
	m_aui_mgr.AddPane(m_dbg_panel, wxAuiPaneInfo().Top());
	m_aui_mgr.AddPane(m_disasm_frame, wxAuiPaneInfo().Center().CaptionVisible(false).CloseButton().MaximizeButton());
	m_aui_mgr.Update();
}

DebuggerPanel::~DebuggerPanel()
{
	m_aui_mgr.UnInit();
}

void DebuggerPanel::UpdateUI()
{
	m_dbg_panel->UpdateUI();
	m_disasm_frame->UpdateUI();
}
