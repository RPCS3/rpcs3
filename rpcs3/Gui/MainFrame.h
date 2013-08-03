#pragma once
#include "GameViewer.h"
#include "wx/aui/aui.h"

class MainFrame : public FrameBase
{
	GameViewer* m_game_viewer;
	wxAuiManager m_aui_mgr;
	AppConnector m_app_connector;

public:
	MainFrame();
	~MainFrame();

	void AddPane(wxWindow* wind, const wxString& caption, int flags);
	void DoSettings(bool load);

private:
	void OnQuit(wxCloseEvent& event);

	void BootGame(wxCommandEvent& event);
	void BootElf(wxCommandEvent& event);
	void BootSelf(wxCommandEvent& event);
	void Pause(wxCommandEvent& event);
	void Stop(wxCommandEvent& event);
	void SendExit(wxCommandEvent& event);
	void Config(wxCommandEvent& event);
	void ConfigVFS(wxCommandEvent& event);
	void ConfigVHDD(wxCommandEvent& event);
	void UpdateUI(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);

private:
	DECLARE_EVENT_TABLE()
};