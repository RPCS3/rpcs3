#include "PrecompiledHeader.h"

#include "DisassemblyDialog.h"
#include "DebugTools/DebugInterface.h"
#include "DebugTools/DisassemblyManager.h"
#include "DebugTools/Breakpoints.h"
#include "BreakpointWindow.h"
#include "PathDefs.h"

#ifdef WIN32
#include <Windows.h>
#endif

BEGIN_EVENT_TABLE(DisassemblyDialog, wxFrame)
   EVT_COMMAND( wxID_ANY, debEVT_SETSTATUSBARTEXT, DisassemblyDialog::onDebuggerEvent )
   EVT_COMMAND( wxID_ANY, debEVT_UPDATELAYOUT, DisassemblyDialog::onDebuggerEvent )
   EVT_COMMAND( wxID_ANY, debEVT_GOTOINMEMORYVIEW, DisassemblyDialog::onDebuggerEvent )
   EVT_COMMAND( wxID_ANY, debEVT_RUNTOPOS, DisassemblyDialog::onDebuggerEvent )
   EVT_COMMAND( wxID_ANY, debEVT_GOTOINDISASM, DisassemblyDialog::onDebuggerEvent )
   EVT_COMMAND( wxID_ANY, debEVT_STEPOVER, DisassemblyDialog::onDebuggerEvent )
   EVT_COMMAND( wxID_ANY, debEVT_STEPINTO, DisassemblyDialog::onDebuggerEvent )
   EVT_COMMAND( wxID_ANY, debEVT_UPDATE, DisassemblyDialog::onDebuggerEvent )
   EVT_CLOSE( DisassemblyDialog::onClose )
END_EVENT_TABLE()


DebuggerHelpDialog::DebuggerHelpDialog(wxWindow* parent)
	: wxDialog(parent,wxID_ANY,L"Help")
{
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl* textControl = new wxTextCtrl(this,wxID_ANY,L"",wxDefaultPosition,wxDefaultSize,
		wxTE_MULTILINE|wxTE_READONLY);
	textControl->SetMinSize(wxSize(400,300));
	auto fileName = PathDefs::GetDocs().ToString()+L"/debugger.txt";
	wxTextFile file(fileName);
	if (file.Open())
	{
		wxString text = file.GetFirstLine();
		while (!file.Eof())
		{
			text += file.GetNextLine()+L"\r\n";
		}

		textControl->SetLabel(text);
		textControl->SetSelection(0,0);
	}

	sizer->Add(textControl,1,wxEXPAND);
	SetSizerAndFit(sizer);
}


CpuTabPage::CpuTabPage(wxWindow* parent, DebugInterface* _cpu)
	: wxPanel(parent), cpu(_cpu)
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(mainSizer);
	
	bottomTabs = new wxNotebook(this,wxID_ANY);
	disassembly = new CtrlDisassemblyView(this,cpu);
	registerList = new CtrlRegisterList(this,cpu);
	memory = new CtrlMemView(bottomTabs,cpu);

	// create register list and disassembly section
	wxBoxSizer* middleSizer = new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* registerSizer = new wxBoxSizer(wxVERTICAL);
	registerSizer->Add(registerList,1);
	
	middleSizer->Add(registerSizer,0,wxEXPAND|wxRIGHT,2);
	middleSizer->Add(disassembly,2,wxEXPAND);
	mainSizer->Add(middleSizer,3,wxEXPAND|wxBOTTOM,3);

	// create bottom section
	bottomTabs->AddPage(memory,L"Memory");
	mainSizer->Add(bottomTabs,1,wxEXPAND);

	mainSizer->Layout();
}

DisassemblyDialog::DisassemblyDialog(wxWindow* parent):
	wxFrame( parent, wxID_ANY, L"Debugger", wxDefaultPosition,wxDefaultSize,wxRESIZE_BORDER|wxCLOSE_BOX|wxCAPTION|wxSYSTEM_MENU ),
	currentCpu(NULL)
{

	topSizer = new wxBoxSizer( wxVERTICAL );
	wxPanel *panel = new wxPanel(this, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _("panel"));
	panel->SetSizer(topSizer);

	// create top row
	wxBoxSizer* topRowSizer = new wxBoxSizer(wxHORIZONTAL);

	breakRunButton = new wxButton(panel, wxID_ANY, L"Run");
	Connect(breakRunButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DisassemblyDialog::onBreakRunClicked));
	topRowSizer->Add(breakRunButton,0,wxRIGHT,8);

	stepIntoButton = new wxButton( panel, wxID_ANY, L"Step Into" );
	stepIntoButton->Enable(false);
	Connect( stepIntoButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DisassemblyDialog::onStepIntoClicked ) );
	topRowSizer->Add(stepIntoButton,0,wxBOTTOM,2);

	stepOverButton = new wxButton( panel, wxID_ANY, L"Step Over" );
	stepOverButton->Enable(false);
	Connect( stepOverButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DisassemblyDialog::onStepOverClicked ) );
	topRowSizer->Add(stepOverButton);
	
	stepOutButton = new wxButton( panel, wxID_ANY, L"Step Out" );
	stepOutButton->Enable(false);
	topRowSizer->Add(stepOutButton,0,wxRIGHT,8);
	
	breakpointButton = new wxButton( panel, wxID_ANY, L"Breakpoint" );
	Connect( breakpointButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DisassemblyDialog::onBreakpointClick ) );
	topRowSizer->Add(breakpointButton);

	topSizer->Add(topRowSizer,0,wxLEFT|wxRIGHT|wxTOP,3);

	// create middle part of the window
	wxNotebook* middleBook = new wxNotebook(panel,wxID_ANY);  
	middleBook->SetBackgroundColour(wxColour(0xFFF0F0F0));
	eeTab = new CpuTabPage(middleBook,&r5900Debug);
	iopTab = new CpuTabPage(middleBook,&r3000Debug);
	middleBook->AddPage(eeTab,L"R5900");
	middleBook->AddPage(iopTab,L"R3000");
	Connect(middleBook->GetId(),wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED,wxCommandEventHandler( DisassemblyDialog::onPageChanging));
	topSizer->Add(middleBook,3,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,3);
	currentCpu = eeTab;

	CreateStatusBar(1);
	
	SetMinSize(wxSize(1000,600));
	panel->GetSizer()->Fit(this);

	setDebugMode(true);
}

#ifdef WIN32
WXLRESULT DisassemblyDialog::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
	switch (nMsg)
	{
	case WM_SHOWWINDOW:
		{
			WXHWND hwnd = GetHWND();

			u32 style = GetWindowLong((HWND)hwnd,GWL_STYLE);
			style &= ~(WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
			SetWindowLong((HWND)hwnd,GWL_STYLE,style);

			u32 exStyle = GetWindowLong((HWND)hwnd,GWL_EXSTYLE);
			exStyle |= (WS_EX_CONTEXTHELP);
			SetWindowLong((HWND)hwnd,GWL_EXSTYLE,exStyle);
		}
		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_CONTEXTHELP)
		{
			DebuggerHelpDialog help(this);
			help.ShowModal();
			return 0;
		}
		break;
	}

	return wxFrame::MSWWindowProc(nMsg,wParam,lParam);
}
#endif

void DisassemblyDialog::onBreakRunClicked(wxCommandEvent& evt)
{	
	if (r5900Debug.isCpuPaused())
	{
		if (CBreakPoints::IsAddressBreakPoint(r5900Debug.getPC()))
			CBreakPoints::SetSkipFirst(r5900Debug.getPC());
		r5900Debug.resumeCpu();
	} else
		r5900Debug.pauseCpu();
}

void DisassemblyDialog::onStepOverClicked(wxCommandEvent& evt)
{	
	stepOver();
}

void DisassemblyDialog::onStepIntoClicked(wxCommandEvent& evt)
{	
	stepInto();
}

void DisassemblyDialog::onPageChanging(wxCommandEvent& evt)
{
	wxNotebook* notebook = (wxNotebook*)wxWindow::FindWindowById(evt.GetId());
	wxWindow* currentPage = notebook->GetCurrentPage();

	if (currentPage == eeTab)
		currentCpu = eeTab;
	else if (currentPage == iopTab)
		currentCpu = iopTab;
	else
		currentCpu = NULL;

	if (currentCpu != NULL)
	{
		currentCpu->getDisassembly()->SetFocus();
		currentCpu->Refresh();
	}
}

void DisassemblyDialog::stepOver()
{
	if (!r5900Debug.isAlive() || !r5900Debug.isCpuPaused() || currentCpu == NULL)
		return;
	

	// todo: breakpoints for iop
	if (currentCpu != eeTab)
		return;

	CtrlDisassemblyView* disassembly = currentCpu->getDisassembly();

	// If the current PC is on a breakpoint, the user doesn't want to do nothing.
	CBreakPoints::SetSkipFirst(r5900Debug.getPC());
	u32 currentPc = r5900Debug.getPC();

	MIPSAnalyst::MipsOpcodeInfo info = MIPSAnalyst::GetOpcodeInfo(&r5900Debug,r5900Debug.getPC());
	u32 breakpointAddress = currentPc+disassembly->getInstructionSizeAt(currentPc);
	if (info.isBranch)
	{
		if (info.isConditional == false)
		{
			if (info.isLinkedBranch)	// jal, jalr
			{
				// it's a function call with a delay slot - skip that too
				breakpointAddress += 4;
			} else {					// j, ...
				// in case of absolute branches, set the breakpoint at the branch target
				breakpointAddress = info.branchTarget;
			}
		} else {						// beq, ...
			if (info.conditionMet)
			{
				breakpointAddress = info.branchTarget;
			} else {
				breakpointAddress = currentPc+2*4;
				disassembly->scrollStepping(breakpointAddress);
			}
		}
	} else {
		disassembly->scrollStepping(breakpointAddress);
	}

	CBreakPoints::AddBreakPoint(breakpointAddress,true);
	r5900Debug.resumeCpu();
}


void DisassemblyDialog::stepInto()
{
	if (!r5900Debug.isAlive() || !r5900Debug.isCpuPaused() || currentCpu == NULL)
		return;
	
	// todo: breakpoints for iop
	if (currentCpu != eeTab)
		return;

	CtrlDisassemblyView* disassembly = currentCpu->getDisassembly();

	// If the current PC is on a breakpoint, the user doesn't want to do nothing.
	CBreakPoints::SetSkipFirst(r5900Debug.getPC());
	u32 currentPc = r5900Debug.getPC();

	MIPSAnalyst::MipsOpcodeInfo info = MIPSAnalyst::GetOpcodeInfo(&r5900Debug,r5900Debug.getPC());
	u32 breakpointAddress = currentPc+disassembly->getInstructionSizeAt(currentPc);
	if (info.isBranch)
	{
		if (info.isConditional == false)
		{
			breakpointAddress = info.branchTarget;
		} else {
			if (info.conditionMet)
			{
				breakpointAddress = info.branchTarget;
			} else {
				breakpointAddress = currentPc+2*4;
				disassembly->scrollStepping(breakpointAddress);
			}
		}
	}

	if (info.isSyscall)
		breakpointAddress = info.branchTarget;

	CBreakPoints::AddBreakPoint(breakpointAddress,true);
	r5900Debug.resumeCpu();
}


void DisassemblyDialog::onBreakpointClick(wxCommandEvent& evt)
{
	if (currentCpu == NULL)
		return;

	BreakpointWindow bpw(this,currentCpu->getCpu());
	if (bpw.ShowModal() == wxID_OK)
	{
		bpw.addBreakpoint();
		update();
	}
}

void DisassemblyDialog::onDebuggerEvent(wxCommandEvent& evt)
{
	wxEventType type = evt.GetEventType();
	if (type == debEVT_SETSTATUSBARTEXT)
	{
		GetStatusBar()->SetLabel(evt.GetString());
	} else if (type == debEVT_UPDATELAYOUT)
	{
		if (currentCpu != NULL)
			currentCpu->GetSizer()->Layout();
		topSizer->Layout();
		update();
	} else if (type == debEVT_GOTOINMEMORYVIEW)
	{
		if (currentCpu != NULL)
			currentCpu->getMemoryView()->gotoAddress(evt.GetInt());
	} else if (type == debEVT_RUNTOPOS)
	{
		// todo: breakpoints for iop
		if (currentCpu != eeTab)
			return;
		CBreakPoints::AddBreakPoint(evt.GetInt(),true);
		currentCpu->getCpu()->resumeCpu();
	} else if (type == debEVT_GOTOINDISASM)
	{
		if (currentCpu != NULL)
		{
			currentCpu->getDisassembly()->gotoAddress(r5900Debug.getPC());
			currentCpu->getDisassembly()->SetFocus();
			update();
		}
	} else if (type == debEVT_STEPOVER)
	{
		if (currentCpu != NULL)
			stepOver();
	} else if (type == debEVT_STEPINTO)
	{
		if (currentCpu != NULL)
			stepInto();
	} else if (type == debEVT_UPDATE)
	{
		update();
	}
}

void DisassemblyDialog::onClose(wxCloseEvent& evt)
{
	Hide();
}

void DisassemblyDialog::update()
{
	if (currentCpu != NULL)
	{
		stepOverButton->Enable(true);
		breakpointButton->Enable(true);
		currentCpu->Refresh();
	} else {
		stepOverButton->Enable(false);
		breakpointButton->Enable(false);
	}
}

void DisassemblyDialog::reset()
{
	eeTab->getDisassembly()->clearFunctions();
	iopTab->getDisassembly()->clearFunctions();
};

void DisassemblyDialog::setDebugMode(bool debugMode)
{
	bool running = r5900Debug.isAlive();
	
	eeTab->Enable(running);
	iopTab->Enable(running);

	if (running)
	{
		if (debugMode)
		{
			CBreakPoints::ClearTemporaryBreakPoints();
			breakRunButton->SetLabel(L"Run");

			stepOverButton->Enable(true);
			stepIntoButton->Enable(true);

			eeTab->getDisassembly()->gotoPc();
			iopTab->getDisassembly()->gotoPc();
			
			// Defocuses main window even when not debugging, causing savestate hotkeys to fail
			/*if (currentCpu != NULL)
				currentCpu->getDisassembly()->SetFocus();*/
		} else {
			breakRunButton->SetLabel(L"Break");

			stepIntoButton->Enable(false);
			stepOverButton->Enable(false);
			stepOutButton->Enable(false);
		}
	}

	update();
}