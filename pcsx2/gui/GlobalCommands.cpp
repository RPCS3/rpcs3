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
#include "MainFrame.h"
#include "HostGui.h"
#include "SaveState.h"
#include "GS.h"

#include "Dump.h"
#include "DebugTools/Debug.h"

using namespace HashTools;

// renderswitch - tells GSdx to go into dx9 sw if "renderswitch" is set.
bool renderswitch = false;

static int g_Pcsx2Recording = 0; // true 1 if recording video and sound


KeyAcceleratorCode::KeyAcceleratorCode( const wxKeyEvent& evt )
{
	val32 = 0;

	keycode = evt.GetKeyCode();

	if( evt.AltDown() ) Alt();
	if( evt.CmdDown() ) Cmd();
	if( evt.ShiftDown() ) Shift();
}

wxString KeyAcceleratorCode::ToString() const
{
	// Let's use wx's string formatter:

	return wxAcceleratorEntry(
		(cmd ? wxACCEL_CMD : 0) |
		(shift ? wxACCEL_CMD : 0) |
		(alt ? wxACCEL_CMD : 0),
		keycode
		).ToString();
}


namespace Implementations
{
	void Frameskip_Toggle()
	{
		// FIXME : Reimplement framelimiting using new double-switch boolean system
	}

	void Framelimiter_TurboToggle()
	{
	}

	void Framelimiter_MasterToggle()
	{
	}

	void Emu_Suspend()
	{
		AppInvoke( CoreThread, Suspend() );
		AppInvoke( MainFrame, ApplySettings() );
	}

	void Emu_Resume()
	{
		AppInvoke( CoreThread, Resume() );
		AppInvoke( MainFrame, ApplySettings() );
	}

	void Emu_TakeSnapshot()
	{
		GSmakeSnapshot( g_Conf->Folders.Snapshots.ToAscii().data() );
	}

	void Emu_RenderToggle()
	{
		AppInvoke( CoreThread, Suspend() );
		renderswitch = !renderswitch;
		AppInvoke( CoreThread, Resume() );
	}

	void Emu_LoggingToggle()
	{
	#ifdef PCSX2_DEVBUILD
		// There's likely a better way to implement this, but this seemed useful.
		// I might add turning EE, VU0, and VU1 recs on and off by hotkey at some point, too.
		// --arcum42
		enableLogging = !enableLogging;

		if (enableLogging)
			GSprintf(10, "Logging Enabled.");
		else
			GSprintf(10,"Logging Disabled.");
	#endif
	}

	void Emu_FreezeGS()
	{
		// fixme : fix up gsstate mess and make it mtgs compatible -- air
#ifdef _STGS_GSSTATE_CODE
		wxString Text;
		if( strgametitle[0] != 0 )
		{
			// only take the first two words
			wxString gsText;

			wxStringTokenizer parts( strgametitle, L" " );

			wxString name( parts.GetNextToken() );	// first part
			wxString part2( parts.GetNextToken() );

			if( !!part2 )
				name += L"_" + part2;

			gsText.Printf( L"%s.%d.gs", name.c_str(), StatesC );
			Text = Path::Combine( g_Conf->Folders.Savestates, gsText );
		}
		else
		{
			Text = GetGSStateFilename();
		}
#endif

	}

	void Emu_RecordingToggle()
	{
		g_Pcsx2Recording ^= 1;

		mtgsThread->SendSimplePacket(GS_RINGTYPE_RECORD, g_Pcsx2Recording, 0, 0);
		if( SPU2setupRecording != NULL ) SPU2setupRecording(g_Pcsx2Recording, NULL);
	}

	void Cpu_DumpRegisters()
	{
#ifdef PCSX2_DEVBUILD
		iDumpRegisters(cpuRegs.pc, 0);
		Console::Notice("hardware registers dumped EE:%x, IOP:%x\n", cpuRegs.pc, psxRegs.pc);
#endif
	}
}

// --------------------------------------------------------------------------------------
//  CommandDeclarations table
// --------------------------------------------------------------------------------------

static const GlobalCommandDescriptor CommandDeclarations[] =
{
	{	"States_FreezeCurrentSlot",
		States_FreezeCurrentSlot,
		wxLt( "Save state" ),
		wxLt( "Saves the virtual machine state to the current slot." ),
	},

	{	"States_DefrostCurrentSlot",
		States_DefrostCurrentSlot,
		wxLt( "Load state" ),
		wxLt( "Loads a virtual machine state from the current slot." ),
	},

	{	"States_CycleSlotForward",
		States_CycleSlotForward,
		wxLt( "Cycle to next slot" ),
		wxLt( "Cycles the current save slot in +1 fashion!" ),
	},

	{	"States_CycleSlotBackward",
		States_CycleSlotBackward,
		wxLt( "Cycle to prev slot" ),
		wxLt( "Cycles the current save slot in -1 fashion!" ),
	},

	{	"Frameskip_Toggle",
		Implementations::Frameskip_Toggle,
		NULL,
		NULL,
	},

	{	"Framelimiter_TurboToggle",
		Implementations::Framelimiter_TurboToggle,
		NULL,
		NULL,
	},

	{	"Framelimiter_MasterToggle",
		Implementations::Framelimiter_MasterToggle,
		NULL,
		NULL,
	},

	{	"Emu_Suspend",
		Implementations::Emu_Suspend,
		NULL,
		NULL,
	},

	{	"Emu_TakeSnapshot",
		Implementations::Emu_TakeSnapshot,
		NULL,
		NULL,
	},

	{	"Emu_RenderswitchToggle",
		Implementations::Emu_RenderToggle,
		NULL,
		NULL,
	},

	{	"Emu_LoggingToggle",
		Implementations::Emu_LoggingToggle,
		NULL,
		NULL,
	},

	{	"Emu_FreezeGS",
		Implementations::Emu_FreezeGS,
		NULL,
		NULL,
	},
	{	"Emu_RecordingToggle",
		Implementations::Emu_RecordingToggle,
		NULL,
		NULL,
	},


	// Command Declarations terminator:
	// (must always be last in list!!)
	{ NULL }
};

AcceleratorDictionary::AcceleratorDictionary() :
	_parent( 0, 0xffffffff )
{
}

void AcceleratorDictionary::Map( const KeyAcceleratorCode& acode, const char *searchfor )
{
	const GlobalCommandDescriptor* result = NULL;
	TryGetValue( acode.val32, result );

	if( result != NULL )
	{
		Console::Notice( wxsFormat(
			L"Kbd Accelerator '%s' is mapped multiple times.\n"
			L"\t'Command %s' is being replaced by '%s'",
			acode.ToString().c_str(), wxString::FromUTF8( result->Id ).c_str(), searchfor )
		);
	}

	wxGetApp().GlobalCommands.TryGetValue( searchfor, result );

	if( result == NULL )
	{
		Console::Notice( wxsFormat( L"Kbd Accelerator '%s' is mapped to unknown command '%s'",
			acode.ToString().c_str(), wxString::FromUTF8( searchfor ).c_str() )
		);
	}
	else
	{
		operator[](acode.val32) = result;
	}
}

void Pcsx2App::BuildCommandHash()
{
	const GlobalCommandDescriptor* curcmd = CommandDeclarations;
	while( curcmd->Invoke != NULL )
	{
		GlobalCommands[curcmd->Id] = curcmd;
		curcmd++;
	}

}

void Pcsx2App::InitDefaultGlobalAccelerators()
{
	typedef KeyAcceleratorCode AAC;

	GlobalAccels.Map( AAC( WXK_F1 ),			"States_FreezeCurrentSlot" );
	GlobalAccels.Map( AAC( WXK_F3 ),			"States_DefrostCurrentSlot" );
	GlobalAccels.Map( AAC( WXK_F2 ),			"States_CycleSlotForward" );
	GlobalAccels.Map( AAC( WXK_F2 ).Shift(),	"States_CycleSlotBackward" );

	GlobalAccels.Map( AAC( WXK_F4 ),			"Frameskip_Toggle");
	GlobalAccels.Map( AAC( WXK_TAB ),			"Framelimiter_TurboToggle" );
	GlobalAccels.Map( AAC( WXK_TAB ).Shift(),	"Framelimiter_MasterToggle" );

	GlobalAccels.Map( AAC( WXK_ESCAPE ),		"Emu_Suspend");
	GlobalAccels.Map( AAC( WXK_F8 ),			"Emu_TakeSnapshot");
	GlobalAccels.Map( AAC( WXK_F9 ),			"Emu_RenderswitchToggle");

	GlobalAccels.Map( AAC( WXK_F10 ),			"Emu_LoggingToggle");
	GlobalAccels.Map( AAC( WXK_F11 ),			"Emu_FreezeGS");
	GlobalAccels.Map( AAC( WXK_F12 ),			"Emu_RecordingToggle");
}
