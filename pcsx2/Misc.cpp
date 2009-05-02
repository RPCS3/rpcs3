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

#include "CDVDisodrv.h"
#include "VUmicro.h"
#include "VU.h"
#include "iCore.h"
#include "iVUzerorec.h"
#include "BaseblockEx.h"		// included for devbuild block dumping (which may or may not work anymore?)

#include "GS.h"
#include "COP0.h"
#include "Cache.h"

#include "Dump.h"

using namespace std;
using namespace R5900;

PcsxConfig Config;
u32 BiosVersion;
static int g_Pcsx2Recording = 0; // true 1 if recording video and sound
bool renderswitch = 0;

struct KeyModifiers keymodifiers = {false, false, false, false};

#define NUM_STATES 10
int StatesC = 0;
extern wxString strgametitle;


#define DIRENTRY_SIZE 16

#if defined(_MSC_VER)
#pragma pack(1)
#endif

struct romdir
{
	char fileName[10];
	u16 extInfoSize;
	u32 fileSize;
#if defined(_MSC_VER)
};
#pragma pack()				//+22
#else
} __attribute__((packed));
#endif

u32 GetBiosVersion() {
	unsigned int fileOffset=0;
	s8 *ROMVER;
	char vermaj[8];
	char vermin[8];
	struct romdir *rd;
	u32 version;
	int i;

	for (i=0; i<512*1024; i++) {
		rd = (struct romdir*)&psRu8(i);
		if (strncmp(rd->fileName, "RESET", 5) == 0)
			break; /* found romdir */
	}
	if (i == 512*1024) return -1;

	while(strlen(rd->fileName) > 0){
		if (strcmp(rd->fileName, "ROMVER") == 0){	// found romver
			ROMVER = &psRs8(fileOffset);

			strncpy(vermaj, (char *)(ROMVER+ 0), 2); vermaj[2] = 0;
			strncpy(vermin, (char *)(ROMVER+ 2), 2); vermin[2] = 0;
			version = strtol(vermaj, (char**)NULL, 0) << 8;
			version|= strtol(vermin, (char**)NULL, 0);

			return version;
		}

		if ((rd->fileSize % 0x10)==0)
			fileOffset += rd->fileSize;
		else
			fileOffset += (rd->fileSize + 0x10) & 0xfffffff0;

		rd++;
	}

	return -1;
}

//2002-09-22 (Florin)
bool IsBIOS(const wxString& filename, wxString& description)
{
	uint fileOffset=0;
	romdir rd;

	wxFileName Bios( g_Conf.Folders.Bios + filename );
	wxFile fp( Bios.GetFullPath().c_str() );

	if( !fp.IsOpened() ) return FALSE;

	int biosFileSize = fp.Length();
	if( biosFileSize <= 0) return FALSE;

	while( (fp.Tell() < 512*1024) && (fp.Read( &rd, DIRENTRY_SIZE ) == DIRENTRY_SIZE) )
	{
		if (strcmp(rd.fileName, "RESET") == 0)
			break;	// found romdir
	}

	if ((strcmp(rd.fileName, "RESET") != 0) || (rd.fileSize == 0)) {
		return FALSE;	//Unable to locate ROMDIR structure in file or a ioprpXXX.img
	}

	bool found = false;

	while(strlen(rd.fileName) > 0)
	{
		if (strcmp(rd.fileName, "ROMVER") == 0)	// found romver
		{
			char aROMVER[14+1];		// ascii version loaded from disk.

			uint filepos = fp.Tell();
			fp.Seek( fileOffset );
			if( fp.Read( &aROMVER, 14 ) == 0 ) break;
			fp.Seek( filepos );	//go back

			const char zonefail[2] = { aROMVER[4], '\0' };	// the default "zone" (unknown code)
			const char* zone = zonefail;

			switch(aROMVER[4])
			{
				case 'T': zone = "T10K  "; break;
				case 'X': zone = "Test  "; break;
				case 'J': zone = "Japan "; break;
				case 'A': zone = "USA   "; break;
				case 'E': zone = "Europe"; break;
				case 'H': zone = "HK    "; break;
				case 'P': zone = "Free  "; break;
				case 'C': zone = "China "; break;
			}

			const wxString romver( wxString::FromAscii(aROMVER) );

			description.Printf( L"%s v%c%c.%c%c(%c%c/%c%c/%c%c%c%c) %s", wxString::FromAscii(zone).ToAscii().data(),
				romver[0], romver[1],	// ver major
				romver[2], romver[3],	// ver minor
				romver[12], romver[13],	// day
				romver[10], romver[11],	// month
				romver[6], romver[7], romver[8], romver[9],	// year!
				(aROMVER[5]=='C') ? L"Console" : (aROMVER[5]=='D') ? L"Devel" : L""
			);
			found = true;
		}

		if ((rd.fileSize % 0x10)==0)
			fileOffset += rd.fileSize;
		else
			fileOffset += (rd.fileSize + 0x10) & 0xfffffff0;

		if (fp.Read( &rd, DIRENTRY_SIZE ) != DIRENTRY_SIZE) break;
	}
	fileOffset-=((rd.fileSize + 0x10) & 0xfffffff0) - rd.fileSize;

	if (found)
	{
		if ( biosFileSize < (int)fileOffset)
		{
			description << ((biosFileSize*100)/(int)fileOffset) << L"%";
			// we force users to have correct bioses,
			// not that lame scph10000 of 513KB ;-)
		}
		return true;
	}

	return false;	//fail quietly
}

int GetPS2ElfName( wxString& name )
{
	int f;
	char buffer[g_MaxPath];//if a file is longer...it should be shorter :D
	char *pos;
	TocEntry tocEntry;

	CDVDFS_init();

	// check if the file exists
	if (CDVD_findfile("SYSTEM.CNF;1", &tocEntry) != TRUE){
		Console::Error("Boot Error > SYSTEM.CNF not found");
		return 0;//could not find; not a PS/PS2 cdvd
	}

	f=CDVDFS_open("SYSTEM.CNF;1", 1);
	CDVDFS_read(f, buffer, g_MaxPath);
	CDVDFS_close(f);

	buffer[tocEntry.fileSize]='\0';

	pos=strstr(buffer, "BOOT2");
	if (pos==NULL){
		pos=strstr(buffer, "BOOT");
		if (pos==NULL) {
			Console::Error("Boot Error > This is not a PS2 game!");
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
			f = new gzLoadingState( Path::Combine( g_Conf.Folders.Savestates, file ) );

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

	ret = PAD1open((void *)&pDsp);

	f->Freeze(g_nLeftGSFrames);
	f->gsFreeze();

	f->FreezePlugin( "GS", gsSafeFreeze );

	RunGSState( *f );

	delete( f );

	GSclose();
	PAD1close();
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
	return Path::Combine( g_Conf.Folders.Savestates, wxsFormat( L"/%8.8X.%d.gs", ElfCRC, StatesC ) );
}

void CycleFrameLimit(int dir)
{
	const char* limitMsg;
	u32 newOptions;
	u32 curFrameLimit = Config.Options & PCSX2_FRAMELIMIT_MASK;
	u32 newFrameLimit;
	static u32 oldFrameLimit = PCSX2_FRAMELIMIT_LIMIT;

	if( dir == 0 ) {
		// turn off limit or restore previous limit mode
		if (curFrameLimit) {
			oldFrameLimit = curFrameLimit;
			newFrameLimit = 0;
		} else
			newFrameLimit = oldFrameLimit;
	}
	else if (dir > 0)	// next
	{
		newFrameLimit = curFrameLimit + PCSX2_FRAMELIMIT_LIMIT;
		if( newFrameLimit > PCSX2_FRAMELIMIT_SKIP )
			newFrameLimit = 0;
	}
	else	// previous
	{
		if( newFrameLimit == 0 )
			newFrameLimit = PCSX2_FRAMELIMIT_SKIP;
		else
			newFrameLimit = curFrameLimit - PCSX2_FRAMELIMIT_LIMIT;
	}

	newOptions = (Config.Options & ~PCSX2_FRAMELIMIT_MASK) | newFrameLimit;

	gsResetFrameSkip();

	switch(newFrameLimit) {
		case PCSX2_FRAMELIMIT_NORMAL:
			limitMsg = "None/Normal";
			break;
		case PCSX2_FRAMELIMIT_LIMIT:
			limitMsg = "Limit";
			break;
		case PCSX2_FRAMELIMIT_SKIP:
			if( GSsetFrameSkip == NULL )
			{
				newOptions &= ~PCSX2_FRAMELIMIT_MASK;
				Console::Notice("Notice: GS Plugin does not support frameskipping.");
				limitMsg = "None/Normal";
			}
			else
			{
				// When enabling Skipping we have to make sure Skipper (GS) and Limiter (EE)
				// are properly synchronized.
				gsDynamicSkipEnable();
				limitMsg = "Skip";
			}

			break;
	}
	Threading::AtomicExchange( Config.Options, newOptions );

	Console::Notice("Frame Limit Mode Changed: %s", params limitMsg );

	// [Air]: Do we really want to save runtime changes to frameskipping?
	//SaveConfig();
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
			catch( Exception::StateLoadError_Recoverable& )
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
					wxsFormat( L"Error loading saveslot %d.  Emulator reset.", StatesC )
				);
			}
			break;

		case 4:
			CycleFrameLimit(keymod->shift ? -1 : 1);
			break;

		// note: VK_F5-VK_F7 are reserved for GS
		case 8:
			GSmakeSnapshot( g_Conf.Folders.Snapshots.ToAscii().data() );
			break;

		case 9: //gsdx "on the fly" renderer switching
			if (!renderswitch)
			{
				StateRecovery::MakeGsOnly();
				g_EmulationInProgress = false;
				CloseGS();
				renderswitch = true;	//go to dx9 sw
				StateRecovery::Recover();
				HostGui::BeginExecution(); //also sets g_EmulationInProgress to true later
			}
			else
			{
				StateRecovery::MakeGsOnly();
				g_EmulationInProgress = false;
				CloseGS();
				renderswitch = false;	//return to default renderer
				StateRecovery::Recover();
				HostGui::BeginExecution(); //also sets g_EmulationInProgress to true later
			}
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
			if( mtgsThread != NULL )
			{
				Console::Notice( "Cannot make gsstates in MTGS mode" );
			}
			else
			{
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
					Text = Path::Combine( g_Conf.Folders.Savestates, gsText );
				}
				else
				{
					Text = GetGSStateFilename();
				}

				SaveGSState(Text);
			}
			break;
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

				if( mtgsThread != NULL )
					mtgsThread->SendSimplePacket(GS_RINGTYPE_RECORD, g_Pcsx2Recording, 0, 0);
				else if( GSsetupRecording != NULL )
					GSsetupRecording(g_Pcsx2Recording, NULL);

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
