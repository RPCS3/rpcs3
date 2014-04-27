/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "DebugEvents.h"

DEFINE_LOCAL_EVENT_TYPE( debEVT_SETSTATUSBARTEXT )
DEFINE_LOCAL_EVENT_TYPE( debEVT_UPDATELAYOUT )
DEFINE_LOCAL_EVENT_TYPE( debEVT_GOTOINMEMORYVIEW )
DEFINE_LOCAL_EVENT_TYPE( debEVT_GOTOINDISASM )
DEFINE_LOCAL_EVENT_TYPE( debEVT_RUNTOPOS )
DEFINE_LOCAL_EVENT_TYPE( debEVT_MAPLOADED )
DEFINE_LOCAL_EVENT_TYPE( debEVT_STEPOVER )
DEFINE_LOCAL_EVENT_TYPE( debEVT_STEPINTO )
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

bool executeExpressionWindow(wxWindow* parent, DebugInterface* cpu, u64& dest, const wxString& defaultValue)
{
	wxString result = wxGetTextFromUser(L"Enter expression",L"Expression",defaultValue,parent);
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