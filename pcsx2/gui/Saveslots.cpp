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

#include "Common.h"
#include "HostGui.h"

#include "GS.h"

StartupParams g_Startup;

// Save state save-to-file (or slot) helpers.
void States_Save( const wxString& file )
{
	StateCopy_SaveToFile( file );
}

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

void States_FreezeCurrentSlot()
{
	GSchangeSaveState( StatesC, SaveStateBase::GetFilename( StatesC ).ToUTF8() );
	StateCopy_SaveToSlot( StatesC );
}

void States_DefrostCurrentSlot()
{
	GSchangeSaveState( StatesC, SaveStateBase::GetFilename( StatesC ).ToUTF8() );
	StateCopy_LoadFromSlot( StatesC );
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

