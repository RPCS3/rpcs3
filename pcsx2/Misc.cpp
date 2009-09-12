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

#include "AppConfig.h"

using namespace std;
using namespace R5900;

struct KeyModifiers keymodifiers = {false, false, false, false};

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
		throw Exception::LogicError( msg );
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
		if( !Path::IsRelative( file ) )
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
		throw Exception::PluginOpenError( PluginId_GS );
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

void _memset16_unaligned( void* dest, u16 data, size_t size )
{
	jASSUME( (size & 0x1) == 0 );

	u16* dst = (u16*)dest;
	for(int i=size; i; --i, ++dst )
		*dst = data;
}
