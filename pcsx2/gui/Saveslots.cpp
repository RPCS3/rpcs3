/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "GS.h"
#include "Elfheader.h"

// --------------------------------------------------------------------------------------
//  Saveslot Section
// --------------------------------------------------------------------------------------

static int StatesC = 0;
static const int StateSlotsCount = 10;
static wxMenuItem* g_loadBackupMenuItem =NULL;

bool States_isSlotUsed(int num)
{
	if (ElfCRC == 0)
		return false;
	else
		return wxFileExists( SaveStateBase::GetFilename( num ) );
}

// FIXME : Use of the IsSavingOrLoading flag is mostly a hack until we implement a
// complete thread to manage queuing savestate tasks, and zipping states to disk.  --air
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

void Sstates_updateLoadBackupMenuItem( bool isBeforeSave = false);

void States_FreezeCurrentSlot()
{
	// FIXME : Use of the IsSavingOrLoading flag is mostly a hack until we implement a
	// complete thread to manage queuing savestate tasks, and zipping states to disk.  --air

	if( wxGetApp().HasPendingSaves() || AtomicExchange(IsSavingOrLoading, true) )
	{
		Console.WriteLn( "Load or save action is already pending." );
		return;
	}
	Sstates_updateLoadBackupMenuItem( true );

	GSchangeSaveState( StatesC, SaveStateBase::GetFilename( StatesC ).ToUTF8() );
	StateCopy_SaveToSlot( StatesC );
	
	GetSysExecutorThread().PostIdleEvent( SysExecEvent_ClearSavingLoadingFlag() );
}

void _States_DefrostCurrentSlot( bool isFromBackup )
{
	if( AtomicExchange(IsSavingOrLoading, true) )
	{
		Console.WriteLn( "Load or save action is already pending." );
		return;
	}

	GSchangeSaveState( StatesC, SaveStateBase::GetFilename( StatesC ).ToUTF8() );
	StateCopy_LoadFromSlot( StatesC, isFromBackup );

	GetSysExecutorThread().PostIdleEvent( SysExecEvent_ClearSavingLoadingFlag() );

	Sstates_updateLoadBackupMenuItem();
}

void States_DefrostCurrentSlot()
{
	_States_DefrostCurrentSlot( false );
}

void States_DefrostCurrentSlotBackup()
{
	_States_DefrostCurrentSlot( true );
}


void States_registerLoadBackupMenuItem( wxMenuItem* loadBackupMenuItem )
{
	g_loadBackupMenuItem = loadBackupMenuItem;
}

static void OnSlotChanged()
{
	Console.Warning( " > Selected savestate slot %d", StatesC);

	if( GSchangeSaveState != NULL )
		GSchangeSaveState(StatesC, SaveStateBase::GetFilename(StatesC).mb_str());

	Sstates_updateLoadBackupMenuItem();
}

int States_GetCurrentSlot()
{
	return StatesC;
}

void Sstates_updateLoadBackupMenuItem( bool isBeforeSave )
{
	if( !g_loadBackupMenuItem )	return;

	int slot = States_GetCurrentSlot();
	wxString file = SaveStateBase::GetFilename( slot );
	g_loadBackupMenuItem->Enable( wxFileExists( isBeforeSave && g_Conf->EmuOptions.BackupSavestate ? file : file + L".backup" ) );
	wxString label;
	label.Printf(L"%s %d", _("Backup"), slot );
	g_loadBackupMenuItem->SetText( label );
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

