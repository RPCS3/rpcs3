#pragma once
#include "GameViewer.h"

class MainFrame : public FrameBase
{
	GameViewer* m_game_viewer;

public:
	MainFrame();

private:
	virtual void OnQuit(wxCloseEvent& event);
	virtual void OnResize(wxSizeEvent& event);

	virtual void BootGame(wxCommandEvent& event);
	virtual void BootElf(wxCommandEvent& event);
	virtual void BootSelf(wxCommandEvent& event);
	virtual void Pause(wxCommandEvent& event);
	virtual void Stop(wxCommandEvent& event);
	virtual void Config(wxCommandEvent& event);
	virtual void OnKeyDown(wxKeyEvent& event);

public:
	virtual void UpdateUI();

private:
	DECLARE_EVENT_TABLE()
};