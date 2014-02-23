#include "PrecompiledHeader.h"
#include "BreakpointWindow.h"


BEGIN_EVENT_TABLE(BreakpointWindow, wxDialog)
	EVT_RADIOBUTTON(wxID_ANY, BreakpointWindow::onRadioChange)
	EVT_BUTTON(wxID_OK, BreakpointWindow::onButtonOk)
END_EVENT_TABLE()


BreakpointWindow::BreakpointWindow( wxWindow* parent, DebugInterface* _cpu )
	: wxDialog(parent,wxID_ANY,L"Breakpoint"), cpu(_cpu)
{
	wxBoxSizer* topLevelSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* upperPart = new wxBoxSizer(wxHORIZONTAL);

	wxFlexGridSizer* leftSizer = new wxFlexGridSizer(2,2,7,7);
	
	// address
	wxStaticText* addressText = new wxStaticText(this,wxID_ANY,L"Address");
	editAddress = new wxTextCtrl(this,wxID_ANY,L"");

	leftSizer->Add(addressText,0,wxALIGN_CENTER_VERTICAL|wxBOTTOM,1);
	leftSizer->Add(editAddress,1);

	// size
	wxStaticText* sizeText = new wxStaticText(this,wxID_ANY,L"Size");
	editSize = new wxTextCtrl(this,wxID_ANY,L"");
	leftSizer->Add(sizeText,0,wxALIGN_CENTER_VERTICAL|wxBOTTOM,1);
	leftSizer->Add(editSize,1);

	// right part
	wxFlexGridSizer* rightSizer = new wxFlexGridSizer(3,2,7,7);

	radioMemory = new wxRadioButton(this,wxID_ANY,L"Memory",wxDefaultPosition,wxDefaultSize,wxRB_GROUP);
	radioExecute = new wxRadioButton(this,wxID_ANY,L"Execute");
	rightSizer->Add(radioMemory,1);
	rightSizer->Add(radioExecute,1,wxLEFT,8);

	checkRead = new wxCheckBox(this,wxID_ANY,L"Read");
	checkWrite = new wxCheckBox(this,wxID_ANY,L"Write");
	checkOnChange = new wxCheckBox(this,wxID_ANY,L"On change");
	rightSizer->Add(checkRead,1);
	rightSizer->Add(new wxStaticText(this,wxID_ANY,L""),1);
	rightSizer->Add(checkWrite,1);
	rightSizer->Add(checkOnChange,1,wxLEFT,8);

	upperPart->Add(leftSizer,1,wxLEFT|wxTOP|wxRIGHT,8);
	upperPart->Add(rightSizer,1,wxLEFT|wxTOP|wxRIGHT,8);

	// bottom part
	wxBoxSizer* conditionSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* conditionText = new wxStaticText(this,wxID_ANY,L"Condition");
	editCondition = new wxTextCtrl(this,wxID_ANY,L"");
	conditionSizer->Add(conditionText,0,wxALIGN_CENTER_VERTICAL|wxBOTTOM,1);
	conditionSizer->AddSpacer(7);
	conditionSizer->Add(editCondition,1,wxEXPAND);

	wxBoxSizer* bottomRowSizer = new wxBoxSizer(wxHORIZONTAL);
	checkEnabled = new wxCheckBox(this,wxID_ANY,L"Enabled");
	checkLog = new wxCheckBox(this,wxID_ANY,L"Log");
	buttonOk = new wxButton(this,wxID_OK,L"OK");
	buttonCancel = new wxButton(this,wxID_CANCEL,L"Cancel");
	bottomRowSizer->AddStretchSpacer();
	bottomRowSizer->Add(checkEnabled,0,wxALIGN_CENTER_VERTICAL|wxBOTTOM,1);
	bottomRowSizer->AddSpacer(8);
	bottomRowSizer->Add(checkLog,0,wxALIGN_CENTER_VERTICAL|wxBOTTOM,1);
	bottomRowSizer->AddSpacer(8);
	bottomRowSizer->Add(buttonOk,1);
	bottomRowSizer->AddSpacer(4);
	bottomRowSizer->Add(buttonCancel,1);

	// align labels
	int minWidth = std::max<int>(addressText->GetSize().x,sizeText->GetSize().x);
	minWidth = std::max<int>(minWidth,conditionText->GetSize().x);
	addressText->SetMinSize(wxSize(minWidth,addressText->GetSize().y));
	sizeText->SetMinSize(wxSize(minWidth,sizeText->GetSize().y));
	conditionText->SetMinSize(wxSize(minWidth,conditionText->GetSize().y));

	// set tab order
	radioMemory->MoveAfterInTabOrder(editAddress);
	radioExecute->MoveAfterInTabOrder(radioMemory);
	editSize->MoveAfterInTabOrder(radioExecute);
	checkRead->MoveAfterInTabOrder(editSize);
	checkWrite->MoveAfterInTabOrder(checkRead);
	checkOnChange->MoveAfterInTabOrder(checkWrite);
	editCondition->MoveAfterInTabOrder(checkOnChange);
	checkEnabled->MoveAfterInTabOrder(editCondition);
	checkLog->MoveAfterInTabOrder(checkEnabled);
	buttonOk->MoveAfterInTabOrder(checkLog);
	buttonCancel->MoveAfterInTabOrder(buttonOk);

	topLevelSizer->Add(upperPart,0);
	topLevelSizer->AddSpacer(5);
	topLevelSizer->Add(conditionSizer,0,wxLEFT|wxTOP|wxRIGHT|wxEXPAND,8);
	topLevelSizer->AddSpacer(5);
	topLevelSizer->Add(bottomRowSizer,0,wxLEFT|wxTOP|wxRIGHT|wxBOTTOM,8);
	SetSizer(topLevelSizer);
	topLevelSizer->Fit(this);

	buttonOk->SetDefault();

	// default values
	memory = true;
	onChange = false;
	read = write = true;
	enabled = log = true;
	address = -1;
	size = 1;
	condition[0] = 0;
	setDefaultValues();
}

void BreakpointWindow::setDefaultValues()
{
	radioExecute->SetValue(!memory);
	radioMemory->SetValue(memory);
	checkRead->SetValue(read);
	checkWrite->SetValue(write);
	checkOnChange->SetValue(onChange);
	checkEnabled->SetValue(enabled);
	checkLog->SetValue(log);

	checkRead->Enable(memory);
	checkWrite->Enable(memory);
	checkOnChange->Enable(memory);
	checkRead->Enable(memory);
	editSize->Enable(memory);
	editCondition->Enable(!memory);
	checkLog->Enable(memory);

	wchar_t str[64];
	if (address != -1)
	{
		swprintf(str,64,L"0x%08X",address);
		editAddress->SetLabel(str);
	}
	
	swprintf(str,64,L"0x%08X",size);
	editSize->SetLabel(str);
	editCondition->SetLabel(wxString(condition,wxConvUTF8));	
}

void BreakpointWindow::loadFromMemcheck(MemCheck& memcheck)
{
	memory = true;

	read = (memcheck.cond & MEMCHECK_READ) != 0;
	write = (memcheck.cond & MEMCHECK_WRITE) != 0;
	onChange = (memcheck.cond & MEMCHECK_WRITE_ONCHANGE) != 0;

	switch (memcheck.result)
	{
	case MEMCHECK_BOTH:
		log = enabled = true;
		break;
	case MEMCHECK_LOG:
		log = true;
		enabled = false;
		break;
	case MEMCHECK_BREAK:
		log = false;
		enabled = true;
		break;
	case MEMCHECK_IGNORE:
		log = enabled = false;
		break;
	}

	address = memcheck.start;
	size = memcheck.end-address;

	setDefaultValues();
}

void BreakpointWindow::loadFromBreakpoint(BreakPoint& breakpoint)
{
	memory = false;

	enabled = breakpoint.enabled;
	address = breakpoint.addr;
	size = 1;

	if (breakpoint.hasCond)
	{
		strcpy(condition,breakpoint.cond.expressionString);
	} else {
		condition[0] = 0;
	}

	setDefaultValues();
}

void BreakpointWindow::initBreakpoint(u32 _address)
{
	memory = false;
	enabled = true;
	address = _address;
	size = 1;
	condition[0] = 0;

	setDefaultValues();
}

void BreakpointWindow::onRadioChange(wxCommandEvent& evt)
{
	memory = radioMemory->GetValue();
	
	checkRead->Enable(memory);
	checkWrite->Enable(memory);
	checkOnChange->Enable(memory);
	checkRead->Enable(memory);
	editSize->Enable(memory);
	editCondition->Enable(!memory);
	checkLog->Enable(memory);
}

bool BreakpointWindow::fetchDialogData()
{
	wchar_t errorMessage[512];
	PostfixExpression exp;

	memory = radioMemory->GetValue();
	read = checkRead->GetValue();
	write = checkWrite->GetValue();
	enabled = checkEnabled->GetValue();
	log = checkLog->GetValue();
	onChange = checkOnChange->GetValue();

	// parse address
	wxCharBuffer addressText = editAddress->GetLabel().ToUTF8();
	if (cpu->initExpression(addressText,exp) == false)
	{
		swprintf(errorMessage,512,L"Invalid expression \"%s\".",editAddress->GetLabel().wchar_str().data());
		wxMessageBox(errorMessage,L"Error",wxICON_ERROR);
		return false;
	}

	u64 value;
	if (cpu->parseExpression(exp,value) == false)
	{
		swprintf(errorMessage,512,L"Invalid expression \"%s\".",editAddress->GetLabel().wchar_str().data());
		wxMessageBox(errorMessage,L"Error",wxICON_ERROR);
		return false;
	}
	address = value;

	if (memory)
	{
		// parse size
		wxCharBuffer sizeText = editSize->GetLabel().ToUTF8();
		if (cpu->initExpression(sizeText,exp) == false)
		{
			swprintf(errorMessage,512,L"Invalid expression \"%s\".",editSize->GetLabel().wchar_str().data());
			wxMessageBox(errorMessage,L"Error",wxICON_ERROR);
			return false;
		}

		if (cpu->parseExpression(exp,value) == false)
		{
			swprintf(errorMessage,512,L"Invalid expression \"%s\".",editSize->GetLabel().wchar_str().data());
			wxMessageBox(errorMessage,L"Error",wxICON_ERROR);
			return false;
		}
		size = value;
	}

	// condition
	wxCharBuffer conditionText = editCondition->GetLabel().ToUTF8();
	strncpy(condition,conditionText,sizeof(condition));
	condition[sizeof(condition)-1] = 0;

	compiledCondition.clear();
	if (condition[0] != 0)
	{
		if (cpu->initExpression(condition,compiledCondition) == false)
		{
			swprintf(errorMessage,512,L"Invalid expression \"%s\".",editCondition->GetLabel().wchar_str().data());
			wxMessageBox(errorMessage,L"Error",wxICON_ERROR);
			return false;
		}
	}

	return true;
}

void BreakpointWindow::onButtonOk(wxCommandEvent& evt)
{
	if (fetchDialogData() == true)
		evt.Skip();
}

void BreakpointWindow::addBreakpoint()
{
	if (memory)
	{
		// add memcheck
		int cond = 0;
		if (read)
			cond |= MEMCHECK_READ;
		if (write)
			cond |= MEMCHECK_WRITE;
		if (onChange)
			cond |= MEMCHECK_WRITE_ONCHANGE;

		MemCheckResult result;
		if (log && enabled) result = MEMCHECK_BOTH;
		else if (log) result = MEMCHECK_LOG;
		else if (enabled) result = MEMCHECK_BREAK;
		else result = MEMCHECK_IGNORE;

		CBreakPoints::AddMemCheck(address, address + size, (MemCheckCondition)cond, result);
	} else {
		// add breakpoint
		CBreakPoints::AddBreakPoint(address,false);

		if (condition[0] != 0)
		{
			BreakPointCond cond;
			cond.debug = cpu;
			strcpy(cond.expressionString,condition);
			cond.expression = compiledCondition;
			CBreakPoints::ChangeBreakPointAddCond(address,cond);
		}

		if (enabled == false)
		{
			CBreakPoints::ChangeBreakPoint(address,false);
		}
	}
}
