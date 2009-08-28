/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#ifdef _WIN32
#include "RDebug/deci2.h"
#else
#include <sys/time.h>
#endif

#include <ctype.h>
#include <wx/file.h>

#include "IopCommon.h"
#include "HostGui.h"

#include "CDVD/IsoFSdrv.h"
#include "VUmicro.h"
#include "VU.h"
#include "iCore.h"
//#include "sVU_zerorec.h"
#include "BaseblockEx.h"		// included for devbuild block dumping (which may or may not work anymore?)

#include "GS.h"
#include "COP0.h"
#include "Cache.h"

#include "Dump.h"
#include "AppConfig.h"

using namespace std;
using namespace R5900;

static int g_Pcsx2Recording = 0; // true 1 if recording video and sound

// renderswitch - tells GSdx to go into dx9 sw if "renderswitch" is set.
bool renderswitch = false;

struct KeyModifiers keymodifiers = {false, false, false, false};

#define NUM_STATES 10
int StatesC = 0;
extern wxString strgametitle;


// ------------------------------------------------------------------------
// Force DevAssert to *not* inline for devel/debug builds (allows using breakpoints to trap
// assertions), and force it to inline for release builds (optimizes it out completely since
// IsDevBuild is false).  Since Devel builds typically aren't enabled with Global Optimization/
// LTCG, this currently isn't even necessary.  But might as well, in case we decide at a later
// date to re-enable LTCG for devel.
//
#ifdef PCSX2_DEVBUILD
#	define DEVASSERT_INLINE __noinline
#else
#	define DEVASSERT_INLINE __forceinline
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Assertion tool for Devel builds, intended for sanity checking and/or bounds checking
// variables in areas which are not performance critical.
//
// How it works: This function throws an exception of type Exception::AssertionFailure if
// the assertion conditional is false.  Typically for the end-user, this exception is handled
// by the general handler, which (should eventually) create some state dumps and other
// information for troubleshooting purposes.
//
// From a debugging environment, you can trap your DevAssert by either breakpointing the
// exception throw below, or by adding either Exception::AssertionFailure or
// Exception::LogicError to your First-Chance Exception catch list (Visual Studio, under
// the Debug->Exceptions menu/dialog).
//
DEVASSERT_INLINE void DevAssert( bool condition, const char* msg )
{
	if( IsDevBuild && !condition )
	{
		throw Exception::AssertionFailure( msg );
	}
}

// return value:
//   0 - Invalid or unknown disc.
//   1 - PS1 CD
//   2 - PS2 CD
int GetPS2ElfName( wxString& name )
{
	int f;
	char buffer[g_MaxPath];//if a file is longer...it should be shorter :D
	char *pos;
	TocEntry tocEntry;

	IsoFS_init();

	// check if the file exists
	if (IsoFS_findFile("SYSTEM.CNF;1", &tocEntry) != TRUE){
		Console::Status("GetElfName: SYSTEM.CNF not found; invalid cd image or no disc present.");
		return 0;//could not find; not a PS/PS2 cdvd
	}

	f=IsoFS_open("SYSTEM.CNF;1", 1);
	IsoFS_read(f, buffer, g_MaxPath);
	IsoFS_close(f);

	buffer[tocEntry.fileSize]='\0';

	pos=strstr(buffer, "BOOT2");
	if (pos==NULL){
		pos=strstr(buffer, "BOOT");
		if (pos==NULL) {
			Console::Error("PCSX2 Boot Error: This is not a Playstation or PS2 game!");
			return 0;
		}
		return 1;
	}
	pos+=strlen("BOOT2");
	while (pos && *pos && pos<&buffer[g_MaxPath]
		&& (*pos<'A' || (*pos>'Z' && *pos<'a') || *pos>'z'))
		pos++;
	if (!pos || *pos==0)
		return 0;

	// the filename is everything up to the first CR/LF/tab.. ?
	// Or up to any whitespace?  (I'm opting for first CRLF/tab, although the old code
	// apparently stopped on spaces too) --air
	name = wxStringTokenizer( wxString::FromAscii( pos ) ).GetNextToken();

#ifdef PCSX2_DEVBUILD
	FILE *fp;
	int i;

	fp = fopen("System.map", "r");
	if( fp == NULL ) return 2;

	u32 addr;

	Console::WriteLn("Loading System.map...");
	while (!feof(fp)) {
		fseek(fp, 8, SEEK_CUR);
		buffer[0] = '0'; buffer[1] = 'x';
		for (i=2; i<10; i++) buffer[i] = fgetc(fp); buffer[i] = 0;
		addr = strtoul(buffer, (char**)NULL, 0);
		fseek(fp, 3, SEEK_CUR);
		for (i=0; i<g_MaxPath; i++) {
			buffer[i] = fgetc(fp);
			if (buffer[i] == '\n' || buffer[i] == 0) break;
		}
		if (buffer[i] == 0) break;
		buffer[i] = 0;

		disR5900AddSym(addr, buffer);
	}
	fclose(fp);
#endif

	return 2;
}

#ifdef PCSX2_DEVBUILD

void SaveGSState(const wxString& file)
{
	if( g_SaveGSStream ) return;

	Console::WriteLn( "Saving GS State..." );
	Console::WriteLn( wxsFormat( L"\t%s", file.c_str() ) );

	g_fGSSave = new gzSavingState( file );

	g_SaveGSStream = 1;
	g_nLeftGSFrames = 2;

	g_fGSSave->Freeze( g_nLeftGSFrames );
}

void LoadGSState(const wxString& file)
{
	int ret;
	gzLoadingState* f;

	Console::Status( "Loading GS State..." );

	try
	{
		f = new gzLoadingState( file );
	}
	catch( Exception::FileNotFound& )
	{
		// file not found? try prefixing with sstates folder:
		if( !Path::IsRooted( file ) )
		{
			f = new gzLoadingState( Path::Combine( g_Conf->Folders.Savestates, file ) );

			// If this load attempt fails, then let the exception bubble up to
			// the caller to deal with...
		}
	}

	// Always set gsIrq callback -- GS States are always exclusionary of MTGS mode
	GSirqCallback( gsIrq );

	ret = GSopen(&pDsp, "PCSX2", 0);
	if (ret != 0)
	{
		delete f;
		throw Exception::PluginFailure( "GS" );
	}

	ret = PADopen((void *)&pDsp);

	f->Freeze(g_nLeftGSFrames);
	f->gsFreeze();

	f->FreezePlugin( "GS", gsSafeFreeze );

	RunGSState( *f );

	delete( f );

	g_plugins->Close( PluginId_GS );
	g_plugins->Close( PluginId_PAD );
}

#endif

char* mystrlwr( char* string )
{
	assert( string != NULL );
	while ( 0 != ( *string++ = (char)tolower( *string ) ) );
    return string;
}

static wxString GetGSStateFilename()
{
	return Path::Combine( g_Conf->Folders.Savestates, wxsFormat( L"/%8.8X.%d.gs", ElfCRC, StatesC ) );
}

void ProcessFKeys(int fkey, struct KeyModifiers *keymod)
{
	assert(fkey >= 1 && fkey <= 12 );

	switch(fkey)
	{
		case 1:
			try
			{
				gzSavingState( SaveState::GetFilename( StatesC ) ).FreezeAll();
				HostGui::ResetMenuSlots();
			}
			catch( Exception::BaseException& ex )
			{
				// 99% of the time this is a file permission error and the
				// cpu state is intact so just display a passive msg to console without
				// raising an exception.

				Console::Error( "Error!  Could not save state to slot %d", params StatesC );
				Console::Error( ex.LogMessage() );
			}
			break;

		case 2:
			if( keymod->shift )
				StatesC = (StatesC+NUM_STATES-1) % NUM_STATES;
			else
				StatesC = (StatesC+1) % NUM_STATES;

			Console::Notice( " > Selected savestate slot %d", params StatesC);

			if( GSchangeSaveState != NULL )
				GSchangeSaveState(StatesC, SaveState::GetFilename(StatesC).mb_str());
			break;

		case 3:
			try
			{
				gzLoadingState joe( SaveState::GetFilename( StatesC ) );	// throws exception on version mismatch
				cpuReset();
				SysClearExecutionCache();
				joe.FreezeAll();
			}
			catch( Exception::StateLoadError& )
			{
				// At this point the cpu hasn't been reset, so we can return
				// control to the user safely... (and silently)
			}
			catch( Exception::FileNotFound& )
			{
				Console::Notice( "Saveslot %d cannot be loaded; slot does not exist (file not found)", params StatesC );
			}
			catch( Exception::RuntimeError& ex )
			{
				// This is the bad one.  Chances are the cpu has been reset, so emulation has
				// to be aborted.  Sorry user!  We'll give you some info for your trouble:

				ClosePlugins( true );

				throw Exception::CpuStateShutdown(
					// english log message:
					wxsFormat( L"Error!  Could not load from saveslot %d\n", StatesC ) + ex.LogMessage(),

					// translated message:
					wxsFormat( _("Error loading saveslot %d.  Emulator reset."), StatesC )
				);
			}
			break;

		case 4:
			// FIXME : Reimplement framelimiting using new oolean system
			//CycleFrameLimit(keymod->shift ? -1 : 1);
			break;

		// note: VK_F5-VK_F7 are reserved for GS
		case 8:
			GSmakeSnapshot( g_Conf->Folders.Snapshots.ToAscii().data() );
			break;

		case 9: //gsdx "on the fly" renderer switching
			StateRecovery::MakeGsOnly();
			g_EmulationInProgress = false;

			// close GS and PAD together, because the PAD depends on the GS window.
			g_plugins->Close( PluginId_PAD );
			safe_delete( mtgsThread );

			renderswitch = !renderswitch;
			StateRecovery::Recover();
			HostGui::BeginExecution(); // also sets g_EmulationInProgress to true later
			break;
			
#ifdef PCSX2_DEVBUILD
		case 10:
			// There's likely a better way to implement this, but this seemed useful.
			// I might add turning EE, VU0, and VU1 recs on and off by hotkey at some point, too.
			// --arcum42
			enableLogging = !enableLogging;

			if (enableLogging)
				GSprintf(10, "Logging Enabled.");
			else
				GSprintf(10,"Logging Disabled.");

			break;

		case 11:
			Console::Notice( "Cannot make gsstates in MTGS mode" );

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
			break;
#endif
#endif

		case 12:
			if( keymod->shift )
			{
#ifdef PCSX2_DEVBUILD
				iDumpRegisters(cpuRegs.pc, 0);
				Console::Notice("hardware registers dumped EE:%x, IOP:%x\n", params cpuRegs.pc, psxRegs.pc);
#endif
			}
			else
			{
				g_Pcsx2Recording ^= 1;

				mtgsThread->SendSimplePacket(GS_RINGTYPE_RECORD, g_Pcsx2Recording, 0, 0);
				if( SPU2setupRecording != NULL ) SPU2setupRecording(g_Pcsx2Recording, NULL);
			}
			break;
	}
}

void _memset16_unaligned( void* dest, u16 data, size_t size )
{
	jASSUME( (size & 0x1) == 0 );

	u16* dst = (u16*)dest;
	for(int i=size; i; --i, ++dst )
		*dst = data;
}
