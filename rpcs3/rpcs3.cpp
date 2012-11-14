#include "stdafx.h"
#include "rpcs3.h"
#include "Ini.h"
#include "Emu/System.h"
#include <wx/msw/wrapwin.h>

IMPLEMENT_APP(Rpcs3App)
Rpcs3App* TheApp;

bool Rpcs3App::OnInit()
{
	TheApp = this;
	SetAppName(_PRGNAME_);
	wxInitAllImageHandlers();

	Ini.Load();

	ConLogFrame = new LogFrame();
	ConLogFrame->Show();

	m_MainFrame = new MainFrame();
	Emu.Init();
	m_MainFrame->Show();

	return true;
}

void Rpcs3App::Exit()
{
	Emu.Stop();
	Ini.Save();

	if(ConLogFrame && !ConLogFrame->IsBeingDeleted()) ConLogFrame->Close();

	wxApp::Exit();
}

/*
CPUThread& GetCPU(const u8 core)
{
	return Emu.GetCPU().Get(core);
}*/

GameInfo CurGameInfo;