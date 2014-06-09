#pragma once
#include "Gui/MainFrame.h"

class CPUThread;

wxDECLARE_EVENT(wxEVT_DBG_COMMAND, wxCommandEvent);


class Rpcs3App : public wxApp
{
public:
	MainFrame* m_MainFrame;

	virtual bool OnInit();       // RPCS3's entry point
	virtual void OnArguments();  // Handle arguments: Rpcs3App::argc, Rpcs3App::argv
	virtual void Exit();

	Rpcs3App();

	void SendDbgCommand(DbgCommand id, CPUThread* thr=nullptr);
};

DECLARE_APP(Rpcs3App)

//extern CPUThread& GetCPU(const u8 core);

extern Rpcs3App* TheApp;
static const u64 PS3_CLK = 3200000000;
