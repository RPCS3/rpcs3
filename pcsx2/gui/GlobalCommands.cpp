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

int limitOn = false;
namespace Implementations
{
	void Frameskip_Toggle()
	{
		limitOn ^= 1;
		Console.WriteLn("Framelimit mode changed to %d", limitOn ? 1 : 0);
		// FIXME : Reimplement framelimiting using new double-switch boolean system
	}

	void Framelimiter_TurboToggle()
	{
		limitOn ^= 1;
		Console.WriteLn("Framelimit mode changed to %d", limitOn ? 1 : 0);
	}

	void Framelimiter_MasterToggle()
	{
	}

	void Sys_Suspend()
	{
		CoreThread.Suspend();
	}

	void Sys_Resume()
	{
		CoreThread.Resume();
	}

	void Sys_TakeSnapshot()
	{
		GSmakeSnapshot( g_Conf->Folders.Snapshots.ToAscii().data() );
	}

	void Sys_RenderToggle()
	{
		if( g_plugins == NULL ) return;

		SaveSinglePluginHelper helper( PluginId_GS );
		renderswitch = !renderswitch;
	}

	void Sys_LoggingToggle()
	{
		// There's likely a better way to implement this, but this seemed useful.
		// I might add turning EE, VU0, and VU1 recs on and off by hotkey at some point, too.
		// --arcum42

		// FIXME: Some of the trace logs will require recompiler resets to be activated properly.
		//  But since those haven't been implemented yet, no point in implementing that here either.

		char* message;
		const_cast<Pcsx2Config&>(EmuConfig).Trace.Enabled = !EmuConfig.Trace.Enabled;

		if (EmuConfig.Trace.Enabled)
            sprintf(message, "Logging Enabled.");
        else
            sprintf(message, "Logging Disabled.");

		GSprintf(10, message);
	}

	void Sys_FreezeGS()
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

	void Sys_RecordingToggle()
	{
		g_Pcsx2Recording ^= 1;

		mtgsThread.WaitGS();		// make sure GS is in sync with the audio stream when we start.
		mtgsThread.SendSimplePacket(GS_RINGTYPE_RECORD, g_Pcsx2Recording, 0, 0);
		if( SPU2setupRecording != NULL ) SPU2setupRecording(g_Pcsx2Recording, NULL);
	}

	void Cpu_DumpRegisters()
	{
#ifdef PCSX2_DEVBUILD
		iDumpRegisters(cpuRegs.pc, 0);
		Console.Warning("hardware registers dumped EE:%x, IOP:%x\n", cpuRegs.pc, psxRegs.pc);
#endif
	}
}

// --------------------------------------------------------------------------------------
//  CommandDeclarations table
// --------------------------------------------------------------------------------------
// This is our manualized introspection/reflection table.  In a cool language like C# we'd
// have just grabbed this info from enumerating the members of a class and assigning
// properties to each method in the class.  But since this is C++, we have to do the the
// goold old fashioned way! :)

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

	{	"Sys_Suspend",
		Implementations::Sys_Suspend,
		NULL,
		NULL,
	},

	{	"Sys_TakeSnapshot",
		Implementations::Sys_TakeSnapshot,
		NULL,
		NULL,
	},

	{	"Sys_RenderswitchToggle",
		Implementations::Sys_RenderToggle,
		NULL,
		NULL,
	},

	{	"Sys_LoggingToggle",
		Implementations::Sys_LoggingToggle,
		NULL,
		NULL,
	},

	{	"Sys_FreezeGS",
		Implementations::Sys_FreezeGS,
		NULL,
		NULL,
	},
	{	"Sys_RecordingToggle",
		Implementations::Sys_RecordingToggle,
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
		Console.Warning(
			L"Kbd Accelerator '%s' is mapped multiple times.\n"
			L"\t'Command %s' is being replaced by '%s'",
			acode.ToString().c_str(), fromUTF8( result->Id ).c_str(), searchfor
		);
	}

	wxGetApp().GlobalCommands.TryGetValue( searchfor, result );

	if( result == NULL )
	{
		Console.Warning( L"Kbd Accelerator '%s' is mapped to unknown command '%s'",
			acode.ToString().c_str(), fromUTF8( searchfor ).c_str()
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

	GlobalAccels.Map( AAC( WXK_ESCAPE ),		"Sys_Suspend");
	GlobalAccels.Map( AAC( WXK_F8 ),			"Sys_TakeSnapshot");
	GlobalAccels.Map( AAC( WXK_F9 ),			"Sys_RenderswitchToggle");

	GlobalAccels.Map( AAC( WXK_F10 ),			"Sys_LoggingToggle");
	GlobalAccels.Map( AAC( WXK_F11 ),			"Sys_FreezeGS");
	GlobalAccels.Map( AAC( WXK_F12 ),			"Sys_RecordingToggle");
}
