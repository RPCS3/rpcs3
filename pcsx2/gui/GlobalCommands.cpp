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
#include "MainFrame.h"
#include "GSFrame.h"
#include "ApplyState.h"


#include "AppAccelerators.h"
#include "AppSaveStates.h"

#include "Utilities/HashMap.h"

// Various includes needed for dumping...
#include "GS.h"
#include "Dump.h"
#include "DebugTools/Debug.h"
#include "R3000A.h"

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
		(shift ? wxACCEL_SHIFT : 0) |
		(alt ? wxACCEL_ALT : 0),
		keycode
	).ToString();
}

LimiterModeType g_LimiterMode = Limit_Nominal;

namespace Implementations
{
	void Frameskip_Toggle()
	{
		g_Conf->EmuOptions.GS.FrameSkipEnable = !g_Conf->EmuOptions.GS.FrameSkipEnable;
		SetGSConfig().FrameSkipEnable = g_Conf->EmuOptions.GS.FrameSkipEnable;

		if( EmuConfig.GS.FrameSkipEnable )
			Console.WriteLn( "(FrameSkipping) Enabled : FrameDraws=%d, FrameSkips=%d", g_Conf->EmuOptions.GS.FramesToDraw, g_Conf->EmuOptions.GS.FramesToSkip );
		else
			Console.WriteLn( "(FrameSkipping) Disabled." );
	}

	void Framelimiter_TurboToggle()
	{
		ScopedCoreThreadPause pauser;
		
		if( !g_Conf->EmuOptions.GS.FrameLimitEnable )
		{
			g_Conf->EmuOptions.GS.FrameLimitEnable = true;
			g_LimiterMode = Limit_Turbo;
			g_Conf->EmuOptions.GS.LimitScalar = g_Conf->Framerate.TurboScalar;
			Console.WriteLn("(FrameLimiter) Turbo + FrameLimit ENABLED." );
			if ( g_Conf->Framerate.SkipOnTurbo == true) 
				g_Conf->EmuOptions.GS.FrameSkipEnable = true;
			else
				g_Conf->EmuOptions.GS.FrameSkipEnable = false;
		}
		else if( g_LimiterMode == Limit_Turbo )
		{
			GSsetVsync( g_Conf->EmuOptions.GS.VsyncEnable );
			g_LimiterMode = Limit_Nominal;
			g_Conf->EmuOptions.GS.LimitScalar = g_Conf->Framerate.NominalScalar;
			
			if ( g_Conf->Framerate.SkipOnLimit == true) 
			{
				Console.WriteLn("(FrameLimiter) Turbo DISABLED. Frameskip ENABLED" );
				g_Conf->EmuOptions.GS.FrameSkipEnable = true;
			}
			else
			{
				Console.WriteLn("(FrameLimiter) Turbo DISABLED." );
				g_Conf->EmuOptions.GS.FrameSkipEnable = false;
			}
		}
		else
		{
			GSsetVsync( false );
			g_LimiterMode = Limit_Turbo;
			g_Conf->EmuOptions.GS.LimitScalar = g_Conf->Framerate.TurboScalar;
			
			if ( g_Conf->Framerate.SkipOnTurbo == true)
			{
				Console.WriteLn("(FrameLimiter) Turbo + Frameskip ENABLED." );
				g_Conf->EmuOptions.GS.FrameSkipEnable = true;
			}
			else
			{
				Console.WriteLn("(FrameLimiter) Turbo ENABLED." );
				g_Conf->EmuOptions.GS.FrameSkipEnable = false;
			}
		}
		pauser.AllowResume();
	}

	void Framelimiter_SlomoToggle()
	{
		// Slow motion auto-enables the framelimiter even if it's disabled.
		// This seems like desirable and expected behavior.

		// FIXME: Inconsistent use of g_Conf->EmuOptions vs. EmuConfig.  Should figure
		// out a better consistency approach... -air

		ScopedCoreThreadPause pauser;
		GSsetVsync( g_Conf->EmuOptions.GS.VsyncEnable );
		if( g_LimiterMode == Limit_Slomo )
		{
			g_LimiterMode = Limit_Nominal;
			g_Conf->EmuOptions.GS.LimitScalar = g_Conf->Framerate.NominalScalar;
			Console.WriteLn("(FrameLimiter) SlowMotion DISABLED." );
		}
		else
		{
			g_LimiterMode = Limit_Slomo;
			g_Conf->EmuOptions.GS.LimitScalar = g_Conf->Framerate.SlomoScalar;
			Console.WriteLn("(FrameLimiter) SlowMotion ENABLED." );
			g_Conf->EmuOptions.GS.FrameLimitEnable = true;
		}
		pauser.AllowResume();
	}

	void Framelimiter_MasterToggle()
	{
		ScopedCoreThreadPause pauser;
		g_Conf->EmuOptions.GS.FrameLimitEnable = !g_Conf->EmuOptions.GS.FrameLimitEnable;
		GSsetVsync( g_Conf->EmuOptions.GS.FrameLimitEnable && g_Conf->EmuOptions.GS.VsyncEnable );
		Console.WriteLn("(FrameLimiter) %s.", g_Conf->EmuOptions.GS.FrameLimitEnable ? "ENABLED" : "DISABLED" );
		pauser.AllowResume();
	}

	void UpdateImagePosition()
	{
		//AppApplySettings() would have been nicer, since it also immidiately affects the GUI (if open).
		//However, the events sequence it generates also "depresses" Shift/CTRL/etc, so consecutive zoom with CTRL down breaks.
		//Since zoom only affects the window viewport anyway, we can live with directly calling it.
		if (GSFrame* gsFrame = wxGetApp().GetGsFramePtr())
			if (GSPanel* woot = gsFrame->GetViewport())
				woot->DoResize();
	}

	void GSwindow_CycleAspectRatio()
	{
		AspectRatioType& art = g_Conf->GSWindow.AspectRatio;
		wxString arts(L"Not modified");
		switch( art )
		{
			case AspectRatio_Stretch:	art = AspectRatio_4_3; arts = L"AspectRatio_4_3"; break;
			case AspectRatio_4_3:		art = AspectRatio_16_9; arts = L"AspectRatio_16:9"; break;
			case AspectRatio_16_9:		art = AspectRatio_Stretch; arts = L"AspectRatio_Stretch";break;
		}

		Console.WriteLn(L"(GSwindow) Aspect ratio: %s.", arts.c_str());
		UpdateImagePosition();
	}

	void SetOffset(float x, float y)
	{
		g_Conf->GSWindow.OffsetX = x;
		g_Conf->GSWindow.OffsetY = y;
		Console.WriteLn(L"(GSwindow) Offset: x=%f, y=%f", x,y);

		UpdateImagePosition();

	}

	void GSwindow_OffsetYplus(){
		SetOffset(g_Conf->GSWindow.OffsetX.ToFloat(), g_Conf->GSWindow.OffsetY.ToFloat()+1);
	}

	void GSwindow_OffsetYminus(){
		SetOffset(g_Conf->GSWindow.OffsetX.ToFloat(), g_Conf->GSWindow.OffsetY.ToFloat()-1);
	}

	void GSwindow_OffsetXplus(){
		SetOffset(g_Conf->GSWindow.OffsetX.ToFloat()+1, g_Conf->GSWindow.OffsetY.ToFloat());
	}

	void GSwindow_OffsetXminus(){
		SetOffset(g_Conf->GSWindow.OffsetX.ToFloat()-1, g_Conf->GSWindow.OffsetY.ToFloat());
	}

	void GSwindow_OffsetReset(){
		SetOffset(0,0);
	}

	void SetZoomY(float zoom)
	{
		if( zoom <= 0 )
			return;
		g_Conf->GSWindow.StretchY = zoom;
		Console.WriteLn(L"(GSwindow) Vertical stretch: %f", zoom);

		UpdateImagePosition();
	}

	void GSwindow_ZoomInY()
	{
		SetZoomY( g_Conf->GSWindow.StretchY.ToFloat()+1 );
	}
	void GSwindow_ZoomOutY()
	{
		SetZoomY( g_Conf->GSWindow.StretchY.ToFloat()-1 );
	}
	void GSwindow_ZoomResetY()
	{
		SetZoomY( 100 );
	}

	void SetZoom(float zoom)
	{
		if( zoom < 0 )
			return;
		g_Conf->GSWindow.Zoom = zoom;
		
		if ( zoom == 0 ) 
            Console.WriteLn(L"(GSwindow) Zoom: 0 (auto, no black bars)");
		else 
            Console.WriteLn(L"(GSwindow) Zoom: %f", zoom);
		
		UpdateImagePosition();
	}


	void GSwindow_ZoomIn()
	{
		float z = g_Conf->GSWindow.Zoom.ToFloat();
		if( z==0 ) z = 100;
		z++;
		SetZoom( z );
	}
	void GSwindow_ZoomOut()
	{
		float z = g_Conf->GSWindow.Zoom.ToFloat();
		if( z==0 ) z = 100;
		z--;
		SetZoom( z );
	}
	void GSwindow_ZoomToggle()
	{
		float z = g_Conf->GSWindow.Zoom.ToFloat();
		if( z==100 )	z = 0;
		else			z = 100;

		SetZoom( z );
	}


	void Sys_Suspend()
	{
		CoreThread.Suspend();
		sMainFrame.SetFocus();
	}

	void Sys_Resume()
	{
		CoreThread.Resume();
	}

	void Sys_TakeSnapshot()
	{
		GSmakeSnapshot( g_Conf->Folders.Snapshots.ToAscii() );
	}

	void Sys_RenderToggle()
	{
		ScopedCoreThreadPause paused_core( new SysExecEvent_SaveSinglePlugin(PluginId_GS) );
		renderswitch = !renderswitch;
		paused_core.AllowResume();
	}

	void Sys_LoggingToggle()
	{
		// There's likely a better way to implement this, but this seemed useful.
		// I might add turning EE, VU0, and VU1 recs on and off by hotkey at some point, too.
		// --arcum42

		// FIXME: Some of the trace logs will require recompiler resets to be activated properly.
#ifdef PCSX2_DEVBUILD		
		SetTraceConfig().Enabled = !EmuConfig.Trace.Enabled;
		GSprintf(10, const_cast<char*>(EmuConfig.Trace.Enabled ? "Logging Enabled." : "Logging Disabled."));
#endif
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
		ScopedCoreThreadPause paused_core;
		paused_core.AllowResume();

		g_Pcsx2Recording ^= 1;

		GetMTGS().WaitGS();		// make sure GS is in sync with the audio stream when we start.
		if( GSsetupRecording != NULL ) GSsetupRecording(g_Pcsx2Recording, NULL);
		if( SPU2setupRecording != NULL ) SPU2setupRecording(g_Pcsx2Recording, NULL);
	}

	void Cpu_DumpRegisters()
	{
#ifdef PCSX2_DEVBUILD
		iDumpRegisters(cpuRegs.pc, 0);
		Console.Warning("hardware registers dumped EE:%x, IOP:%x\n", cpuRegs.pc, psxRegs.pc);
#endif
	}

	void FullscreenToggle()
	{
		if( GSFrame* gsframe = wxGetApp().GetGsFramePtr() )
			gsframe->ShowFullScreen( !gsframe->IsFullScreen() );
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
		pxL( "Save state" ),
		pxL( "Saves the virtual machine state to the current slot." ),
	},

	{	"States_DefrostCurrentSlot",
		States_DefrostCurrentSlot,
		pxL( "Load state" ),
		pxL( "Loads a virtual machine state from the current slot." ),
	},

	{	"States_DefrostCurrentSlotBackup",
		States_DefrostCurrentSlotBackup,
		pxL( "Load State Backup" ),
		pxL( "Loads virtual machine state backup for current slot." ),
	},

	{	"States_CycleSlotForward",
		States_CycleSlotForward,
		pxL( "Cycle to next slot" ),
		pxL( "Cycles the current save slot in +1 fashion!" ),
	},

	{	"States_CycleSlotBackward",
		States_CycleSlotBackward,
		pxL( "Cycle to prev slot" ),
		pxL( "Cycles the current save slot in -1 fashion!" ),
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

	{	"Framelimiter_SlomoToggle",
		Implementations::Framelimiter_SlomoToggle,
		NULL,
		NULL,
	},

	{	"Framelimiter_MasterToggle",
		Implementations::Framelimiter_MasterToggle,
		NULL,
		NULL,
		true,
	},

	{	"GSwindow_CycleAspectRatio",
		Implementations::GSwindow_CycleAspectRatio,
		NULL,
		NULL,
		true,
	},

	{	"GSwindow_ZoomIn",
		Implementations::GSwindow_ZoomIn,
		NULL,
		NULL,
	},

	{	"GSwindow_ZoomOut",
		Implementations::GSwindow_ZoomOut,
		NULL,
		NULL,
	},

	{	"GSwindow_ZoomToggle",
		Implementations::GSwindow_ZoomToggle,
		NULL,
		NULL,
	},

	{	"GSwindow_ZoomInY",	Implementations::GSwindow_ZoomInY, NULL, NULL, },
	{	"GSwindow_ZoomOutY",	Implementations::GSwindow_ZoomOutY, NULL, NULL, },
	{	"GSwindow_ZoomResetY",	Implementations::GSwindow_ZoomResetY, NULL, NULL, },

	{	"GSwindow_OffsetYminus",	Implementations::GSwindow_OffsetYminus, NULL, NULL, },
	{	"GSwindow_OffsetYplus",		Implementations::GSwindow_OffsetYplus, NULL, NULL, },
	{	"GSwindow_OffsetXminus",	Implementations::GSwindow_OffsetXminus, NULL, NULL, },
	{	"GSwindow_OffsetXplus",		Implementations::GSwindow_OffsetXplus, NULL, NULL, },
	{	"GSwindow_OffsetReset",		Implementations::GSwindow_OffsetReset, NULL, NULL, },

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

	{	"FullscreenToggle",
		Implementations::FullscreenToggle,
		NULL,
		NULL,
	},

	// Command Declarations terminator:
	// (must always be last in list!!)
	{ NULL }
};

CommandDictionary::CommandDictionary() {}

CommandDictionary::~CommandDictionary() throw() {}


AcceleratorDictionary::AcceleratorDictionary()
	: _parent( 0, 0xffffffff )
{
}

AcceleratorDictionary::~AcceleratorDictionary() throw() {}

void AcceleratorDictionary::Map( const KeyAcceleratorCode& _acode, const char *searchfor )
{
	// Search override mapping at ini file
	KeyAcceleratorCode acode = _acode;
	wxString overrideStr;
	wxAcceleratorEntry codeParser;	//Provides string parsing capabilities
	wxFileConfig cfg(L"", L"", L"" , GetUiKeysFilename(), wxCONFIG_USE_GLOBAL_FILE );
	if( cfg.Read( wxString::FromUTF8(searchfor), &overrideStr) )
	{
		overrideStr = wxString(L"\t") + overrideStr;
		if( codeParser.FromString( overrideStr ) ) // needs a '\t' prefix (originally used for wxMenu accelerators parsing)...
		{
			//ini file contains alternative parsable key combination for current 'searchfor'.
			acode = codeParser;
			Console.WriteLn(Color_StrongGreen, L"Overriding '%s': assigning %s (instead of %s)",
				fromUTF8( searchfor ).c_str(), acode.ToString().c_str(), _acode.ToString().c_str());
		}
	}
	// End of overrides section

	const GlobalCommandDescriptor* result = NULL;
	TryGetValue( acode.val32, result );

	if( result != NULL )
	{
		Console.Warning(
			L"Kbd Accelerator '%s' is mapped multiple times.\n"
			L"\t'Command %s' is being replaced by '%s'",
			acode.ToString().c_str(), fromUTF8( result->Id ).c_str(), fromUTF8( searchfor ).c_str()
		);
	}

	wxGetApp().GlobalCommands->TryGetValue( searchfor, result );

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
	if( !GlobalCommands ) GlobalCommands = new CommandDictionary;

	const GlobalCommandDescriptor* curcmd = CommandDeclarations;
	while( curcmd->Invoke != NULL )
	{
		(*GlobalCommands)[curcmd->Id] = curcmd;
		curcmd++;
	}
}

void Pcsx2App::InitDefaultGlobalAccelerators()
{
	typedef KeyAcceleratorCode AAC;

	if( !GlobalAccels ) GlobalAccels = new AcceleratorDictionary;

	GlobalAccels->Map( AAC( WXK_F1 ),			"States_FreezeCurrentSlot" );
	GlobalAccels->Map( AAC( WXK_F3 ),			"States_DefrostCurrentSlot" );
	GlobalAccels->Map( AAC( WXK_F2 ),			"States_CycleSlotForward" );
	GlobalAccels->Map( AAC( WXK_F2 ).Shift(),	"States_CycleSlotBackward" );

	GlobalAccels->Map( AAC( WXK_F4 ),			"Framelimiter_MasterToggle");
	GlobalAccels->Map( AAC( WXK_F4 ).Shift(),	"Frameskip_Toggle");

	/*GlobalAccels->Map( AAC( WXK_ESCAPE ),		"Sys_Suspend");
	GlobalAccels->Map( AAC( WXK_F8 ),			"Sys_TakeSnapshot");
	GlobalAccels->Map( AAC( WXK_F8 ).Shift(),	"Sys_TakeSnapshot");
	GlobalAccels->Map( AAC( WXK_F8 ).Shift().Cmd(),"Sys_TakeSnapshot");
	GlobalAccels->Map( AAC( WXK_F9 ),			"Sys_RenderswitchToggle");

	GlobalAccels->Map( AAC( WXK_F10 ),			"Sys_LoggingToggle");
	GlobalAccels->Map( AAC( WXK_F11 ),			"Sys_FreezeGS");
	GlobalAccels->Map( AAC( WXK_F12 ),			"Sys_RecordingToggle");

	GlobalAccels->Map( AAC( WXK_RETURN ).Alt(),	"FullscreenToggle" );*/
}
