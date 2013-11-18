#include "stdafx.h"
#include "rpcs3.h"
#include "Ini.h"
#include "Emu/System.h"
#include <wx/msw/wrapwin.h>
#include "Gui/CompilerELF.h"

const wxEventType wxEVT_DBG_COMMAND = wxNewEventType();

IMPLEMENT_APP(Rpcs3App)
Rpcs3App* TheApp;

bool Rpcs3App::OnInit()
{
	TheApp = this;
	SetAppName(_PRGNAME_);
	wxInitAllImageHandlers();

	Ini.Load();

	m_MainFrame = new MainFrame();
	SetTopWindow(m_MainFrame);
	Emu.Init();

	(new CompilerELF(m_MainFrame))->Show();
	m_debugger_frame = new DebuggerPanel(m_MainFrame);
	ConLogFrame = new LogFrame(m_MainFrame);

	m_MainFrame->AddPane(ConLogFrame, "Log", wxAUI_DOCK_BOTTOM);
	m_MainFrame->AddPane(m_debugger_frame, "Debugger", wxAUI_DOCK_RIGHT);
	//ConLogFrame->Show();
	m_MainFrame->Show();

	m_MainFrame->DoSettings(true);
	return true;
}

void Rpcs3App::Exit()
{
	Emu.Stop();
	Ini.Save();

	if(ConLogFrame && !ConLogFrame->IsBeingDeleted()) ConLogFrame->Close();

	wxApp::Exit();
}

void Rpcs3App::SendDbgCommand(DbgCommand id, CPUThread* thr)
{
	wxCommandEvent event(wxEVT_DBG_COMMAND, id);
	event.SetClientData(thr);
	AddPendingEvent(event);
}

/*
CPUThread& GetCPU(const u8 core)
{
	return Emu.GetCPU().Get(core);
}*/

GameInfo CurGameInfo;