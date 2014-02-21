#include "PrecompiledHeader.h"
#include "DebugEvents.h"

DEFINE_LOCAL_EVENT_TYPE( debEVT_SETSTATUSBARTEXT )
DEFINE_LOCAL_EVENT_TYPE( debEVT_UPDATELAYOUT )
DEFINE_LOCAL_EVENT_TYPE( debEVT_GOTOINMEMORYVIEW )
DEFINE_LOCAL_EVENT_TYPE( debEVT_GOTOINDISASM )
DEFINE_LOCAL_EVENT_TYPE( debEVT_RUNTOPOS )
DEFINE_LOCAL_EVENT_TYPE( debEVT_MAPLOADED )
DEFINE_LOCAL_EVENT_TYPE( debEVT_STEPOVER )
DEFINE_LOCAL_EVENT_TYPE( debEVT_UPDATE )

bool parseExpression(const char* exp, DebugInterface* cpu, u64& dest)
{
	PostfixExpression postfix;
	if (cpu->initExpression(exp,postfix) == false) return false;
	return cpu->parseExpression(postfix,dest);
}

void displayExpressionError(wxWindow* parent)
{
	wxMessageBox(wxString(getExpressionError(),wxConvUTF8),L"Invalid expression",wxICON_ERROR);
}

bool executeExpressionWindow(wxWindow* parent, DebugInterface* cpu, u64& dest)
{
	wxString result = wxGetTextFromUser(L"Enter expression",L"Expression",wxEmptyString,parent);
	if (result.empty())
		return false;

	wxCharBuffer expression = result.ToUTF8();
	if (parseExpression(expression, cpu, dest) == false)
	{
		displayExpressionError(parent);
		return false;
	}

	return true;
}