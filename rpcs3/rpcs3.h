#pragma once

#include "Gui/MainFrame.h"
#include "Emu/DbgCommand.h"
#include <wx/app.h>
#include <wx/cmdline.h>

wxDECLARE_EVENT(wxEVT_DBG_COMMAND, wxCommandEvent);

class Rpcs3App : public wxApp
{
private:
	wxCmdLineParser parser;

public:
	MainFrame* m_MainFrame;

	virtual bool OnInit();       // RPCS3's entry point
	virtual void OnArguments(const wxCmdLineParser& parser);  // Handle arguments: Rpcs3App::argc, Rpcs3App::argv
	virtual void Exit();

	Rpcs3App();

	void SendDbgCommand(DbgCommand id, class cpu_thread* thr = nullptr);
};

DECLARE_APP(Rpcs3App)

extern Rpcs3App* TheApp;
