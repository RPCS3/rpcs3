/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
#include "App.h"
#include "AppSaveStates.h"

#include "Common.h"
#include "HostGui.h"

#include "GS.h"
#include "Elfheader.h"

StartupParams g_Startup;

// --------------------------------------------------------------------------------------
//  Saveslot Section
// --------------------------------------------------------------------------------------

static int StatesC = 0;
static const int StateSlotsCount = 10;

bool States_isSlotUsed(int num)
{
	if (ElfCRC == 0)
		return false;
	else
		return wxFileExists( SaveStateBase::GetFilename( num ) );
}

static volatile u32 IsSavingOrLoading = false;

class SysExecEvent_ClearSavingLoadingFlag : public SysExecEvent
{
public:
	wxString GetEventName() const { return L"ClearSavingLoadingFlag"; }

	virtual ~SysExecEvent_ClearSavingLoadingFlag() throw() { }
	SysExecEvent_ClearSavingLoadingFlag()
	{
	}
	
	SysExecEvent_ClearSavingLoadingFlag* Clone() const { return new SysExecEvent_ClearSavingLoadingFlag(); }
	
protected:
	void InvokeEvent()
	{
		AtomicExchange(IsSavingOrLoading, false);
	}
};

void States_FreezeCurrentSlot()
{
	if( AtomicExchange(IsSavingOrLoading, true) )
	{
		Console.WriteLn( "Load or save action is already pending." );
		return;
	}

	GSchangeSaveState( StatesC, SaveStateBase::GetFilename( StatesC ).ToUTF8() );
	StateCopy_SaveToSlot( StatesC );
	
	GetSysExecutorThread().PostIdleEvent( SysExecEvent_ClearSavingLoadingFlag() );
}

void States_DefrostCurrentSlot()
{
	if( AtomicExchange(IsSavingOrLoading, true) )
	{
		Console.WriteLn( "Load or save action is already pending." );
		return;
	}

	GSchangeSaveState( StatesC, SaveStateBase::GetFilename( StatesC ).ToUTF8() );
	StateCopy_LoadFromSlot( StatesC );

	GetSysExecutorThread().PostIdleEvent( SysExecEvent_ClearSavingLoadingFlag() );

	//SysStatus( wxsFormat( _("Loaded State (slot %d)"), StatesC ) );
}

static void OnSlotChanged()
{
	Console.Warning( " > Selected savestate slot %d", StatesC);

	if( GSchangeSaveState != NULL )
		GSchangeSaveState(StatesC, SaveStateBase::GetFilename(StatesC).mb_str());
}

int States_GetCurrentSlot()
{
	return StatesC;
}

void States_SetCurrentSlot( int slot )
{
	StatesC = std::min( std::max( slot, 0 ), StateSlotsCount );
	OnSlotChanged();
}

void States_CycleSlotForward()
{
	StatesC = (StatesC+1) % StateSlotsCount;
	OnSlotChanged();
}

void States_CycleSlotBackward()
{
	StatesC = (StatesC+StateSlotsCount-1) % StateSlotsCount;
	OnSlotChanged();
}

