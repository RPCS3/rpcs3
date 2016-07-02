#pragma once

#include "Gui/Debugger.h"
#include "Gui/ConLogFrame.h"
#include "Gui/FrameBase.h"

class GameViewer;

class MainFrame : public FrameBase
{
	DebuggerPanel* m_debugger_frame;
	GameViewer* m_game_viewer;
	LogFrame * m_log_frame;
	wxAuiManager m_aui_mgr;
	bool m_sys_menu_opened;

public:
	MainFrame();
	~MainFrame();

	void AddPane(wxWindow* wind, const wxString& caption, int flags);
	void DoSettings(bool load);

private:
	void OnQuit(wxCloseEvent& event);

	void BootGame(wxCommandEvent& event);
	void BootGameAndRun(wxCommandEvent& event);
	void InstallPkg(wxCommandEvent& event);
	void BootElf(wxCommandEvent& event);
	void Pause(wxCommandEvent& event);
	void Stop(wxCommandEvent& event);
	void SendExit(wxCommandEvent& event);
	void SendOpenCloseSysMenu(wxCommandEvent& event);
	void Config(wxCommandEvent& event);
	void ConfigPad(wxCommandEvent& event);
	void ConfigVFS(wxCommandEvent& event);
	void ConfigVHDD(wxCommandEvent& event);
	void ConfigAutoPause(wxCommandEvent& event);
	void ConfigSaveData(wxCommandEvent& event);
	void OpenELFCompiler(wxCommandEvent& evt);
	void OpenKernelExplorer(wxCommandEvent& evt);
	void OpenMemoryViewer(wxCommandEvent& evt);
	void OpenRSXDebugger(wxCommandEvent& evt);
	void OpenStringSearch(wxCommandEvent& evt);
	void OpenCgDisasm(wxCommandEvent& evt);
	void DecryptSPRXLibraries(wxCommandEvent& event);
	void AboutDialogHandler(wxCommandEvent& event);
	void UpdateUI(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);

private:
	DECLARE_EVENT_TABLE()
};
