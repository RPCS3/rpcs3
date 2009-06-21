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

#include "IopCommon.h"
#include "HostGui.h"

#include "CDVD/CDVDisodrv.h"
#include "VUmicro.h"
#include "VU.h"
#include "iCore.h"
#include "sVU_zerorec.h"
#include "BaseblockEx.h"		// included for devbuild block dumping (which may or may not work anymore?)

#include "GS.h"
#include "COP0.h"
#include "Cache.h"

#include "Paths.h"
#include "Dump.h"

using namespace std;
using namespace R5900;

PcsxConfig Config;
u32 BiosVersion;
char CdromId[12];
static int g_Pcsx2Recording = 0; // true 1 if recording video and sound
bool renderswitch = 0;

struct KeyModifiers keymodifiers = {false, false, false, false};

#define NUM_STATES 10
int StatesC = 0;

extern char strgametitle[256];

struct LangDef {
	char id[8];
	char name[64];
};

LangDef sLangs[] = {
	{ "ar_AR", N_("Arabic") },
	{ "bg_BG", N_("Bulgarian") },
	{ "ca_CA", N_("Catalan") },
	{ "cz_CZ", N_("Czech") },
	{ "du_DU",  N_("Dutch")  },
	{ "de_DE", N_("German") },
	{ "el_EL", N_("Greek") },
	{ "en_US", N_("English") },
	{ "fr_FR", N_("French") },
	{ "hb_HB" , N_("Hebrew") },
	{ "hu_HU", N_("Hungarian") },
	{ "it_IT", N_("Italian") },
	{ "ja_JA", N_("Japanese") },
	{ "pe_PE", N_("Persian") },
	{ "po_PO", N_("Portuguese") },
	{ "po_BR", N_("Portuguese BR") },
	{ "pl_PL" , N_("Polish") },
	{ "ro_RO", N_("Romanian") },
	{ "ru_RU", N_("Russian") },
	{ "es_ES", N_("Spanish") },
	{ "sh_SH" , N_("S-Chinese") },
    { "sw_SW", N_("Swedish") },
	{ "tc_TC", N_("T-Chinese") },
	{ "tr_TR", N_("Turkish") },
	{ "", "" },
};

#define DIRENTRY_SIZE 16

#if defined(_MSC_VER)
#pragma pack(1)
#endif

struct romdir{
	char fileName[10];
	u16 extInfoSize;
	u32 fileSize;
#if defined(_MSC_VER)
};		
#pragma pack()				//+22
#else
} __attribute__((packed));
#endif

const char *LabelAuthors = { N_(
	"PCSX2, a PS2 emulator\n\n"
	"Active Devs: Arcum42, Refraction,\n"
	"drk||raziel, cottonvibes, gigaherz,\n"
	"rama, Jake.Stine, saqib, Tmkk\n"
	"\n"
	"Inactive devs: Alexey silinov, Aumatt,\n"
	"Florin, goldfinger, Linuzappz, loser,\n"
	"Nachbrenner, shadow, Zerofrog\n"
	"\n"
	"Betatesting: Bositman, ChaosCode,\n"
	"CKemu, crushtest, GeneralPlot,\n"
	"Krakatos, Parotaku, Rudy_X\n"
	"\n"
	"Webmasters: CKemu, Falcon4ever"
	)
};

const char *LabelGreets = { N_(
	"Contributors: Hiryu and Sjeep for libcvd (the iso parsing and\n"
	"filesystem driver code), nneeve, pseudonym\n"
	"\n"
	"Plugin Specialists: ChickenLiver (Lilypad), Efp (efp),\n"
	"Gabest (Gsdx, Cdvdolio, Xpad)\n"
	"\n"
	"Special thanks to: black_wd, Belmont, BGome, _Demo_, Dreamtime,\n"
	"F|RES, MrBrown, razorblade, Seta-san, Skarmeth"
	)
};

// ------------------------------------------------------------------------
// Force DevAssert to *not* inline for devel/debug builds (allows using breakpoints to trap
// assertions), and force it to inline for release builds (optimizes it out completely since
// IsDevBuild is false).  Since Devel builds typically aren't enabled with Global Optimization/
// LTCG, this currently isn't even necessary.  But might as well, in case we decide at a later
// date to re-enable LTCG for devel.
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
int IsBIOS(const char *filename, char *description)
{
	char ROMVER[14+1];
	FILE *fp;
	unsigned int fileOffset=0, found=FALSE;
	struct romdir rd;

	string Bios( Path::Combine( Config.BiosDir, filename ) );

	int biosFileSize = Path::getFileSize( Bios );
	if( biosFileSize <= 0) return FALSE;	

	fp = fopen(Bios.c_str(), "rb");
	if (fp == NULL) return FALSE;

	while ((ftell(fp)<512*1024) && (fread(&rd, DIRENTRY_SIZE, 1, fp)==1))
		if (strcmp(rd.fileName, "RESET") == 0)
			break; /* found romdir */

	if ((strcmp(rd.fileName, "RESET") != 0) || (rd.fileSize == 0)) {
		fclose(fp);
		return FALSE;	//Unable to locate ROMDIR structure in file or a ioprpXXX.img
	}

	while(strlen(rd.fileName) > 0)
	{
		if (strcmp(rd.fileName, "ROMVER") == 0)	// found romver
		{
			uint filepos = ftell(fp);
			fseek(fp, fileOffset, SEEK_SET);
			if (fread(&ROMVER, 14, 1, fp) == 0) break;
			fseek(fp, filepos, SEEK_SET);//go back
			
			const char zonefail[2] = { ROMVER[4], '\0' };	// the default "zone" (unknown code)
			const char* zone = zonefail;

			switch(ROMVER[4])
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

			sprintf(description, "%s v%c%c.%c%c(%c%c/%c%c/%c%c%c%c) %s", zone,
				ROMVER[0], ROMVER[1],	// ver major
				ROMVER[2], ROMVER[3],	// ver minor
				ROMVER[12], ROMVER[13],	// day
				ROMVER[10], ROMVER[11],	// month
				ROMVER[6], ROMVER[7], ROMVER[8], ROMVER[9],	// year!
				(ROMVER[5]=='C') ? "Console" : (ROMVER[5]=='D') ? "Devel" : ""
			);
			found = TRUE;
		}

		if ((rd.fileSize % 0x10)==0)
			fileOffset += rd.fileSize;
		else
			fileOffset += (rd.fileSize + 0x10) & 0xfffffff0;

		if (fread(&rd, DIRENTRY_SIZE, 1, fp)==0) break;
	}
	fileOffset-=((rd.fileSize + 0x10) & 0xfffffff0) - rd.fileSize;

	fclose(fp);
	
	if (found)
	{
		char percent[6];

		if ( biosFileSize < (int)fileOffset)
		{
			sprintf(percent, " %d%%", biosFileSize*100/(int)fileOffset);
			strcat(description, percent);//we force users to have correct bioses,
											//not that lame scph10000 of 513KB ;-)
		}
		return TRUE;
	}

	return FALSE;	//fail quietly
}

int GetPS2ElfName(char *name){
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
	while (pos && *pos && pos<=&buffer[255] 
		&& (*pos<'A' || (*pos>'Z' && *pos<'a') || *pos>'z'))
		pos++;
	if (!pos || *pos==0)
		return 0;

	sscanf(pos, "%s", name);

	if (strncmp("cdrom0:\\", name, 8) == 0) {
		strncpy(CdromId, name+8, 11); CdromId[11] = 0;
	}
	
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

void SaveGSState(const string& file)
{
	if( g_SaveGSStream ) return;

	Console::WriteLn( _("Saving GS State...") );
	Console::WriteLn( "\t%hs", params file.c_str() );

	g_fGSSave = new gzSavingState( file );
	
	g_SaveGSStream = 1;
	g_nLeftGSFrames = 2;

	g_fGSSave->Freeze( g_nLeftGSFrames );
}

void LoadGSState(const string& file)
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
		if( !Path::isRooted( file ) )
		{
			string strfile( Path::Combine( SSTATES_DIR, file ) );
			f = new gzLoadingState( strfile.c_str() );

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

char *ParseLang(char *id) {
	int i=0;

	while (sLangs[i].id[0] != 0) {
		if (!strcmp(id, sLangs[i].id))
			return _(sLangs[i].name);
		i++;
	}

	return id;
}

char* mystrlwr( char* string )
{
	assert( string != NULL );
	while ( 0 != ( *string++ = (char)tolower( *string ) ) );
    return string;
}

static string GetGSStateFilename()
{
	string gsText;
	ssprintf( gsText, "/%8.8X.%d.gs", ElfCRC, StatesC);
	return Path::Combine( SSTATES_DIR, gsText );
}

void CycleFrameLimit(int dir)
{
	const char* limitMsg;
	u32 newOptions;
	u32 curFrameLimit = Config.Options & PCSX2_FRAMELIMIT_MASK;
	u32 newFrameLimit = 0;
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
				// cpu state is intact so just display a passive msg to console.

				Console::Error( _( "Error > Could not save state to slot %d" ), params StatesC );
				Console::Error( ex.cMessage() );
			}
			break;

		case 2:
			if( keymod->shift )
				StatesC = (StatesC+NUM_STATES-1) % NUM_STATES;
			else
				StatesC = (StatesC+1) % NUM_STATES;

			Console::Notice( _( " > Selected savestate slot %d" ), params StatesC);

			if( GSchangeSaveState != NULL )
				GSchangeSaveState(StatesC, SaveState::GetFilename(StatesC).c_str());
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
				Console::Notice( _("Saveslot %d cannot be loaded; slot does not exist (file not found)"), params StatesC );
			}
			catch( Exception::RuntimeError& ex )
			{
				// This is the bad one.  Chances are the cpu has been reset, so emulation has
				// to be aborted.  Sorry user!  We'll give you some info for your trouble:

				Console::Error( _("An error occured while trying to load saveslot %d"), params StatesC );
				Console::Error( ex.cMessage() );
				Msgbox::Alert(
					"Pcsx2 encountered an error while trying to load the savestate\n"
					"and emulation had to be aborted." );

				ClosePlugins( true );

				throw Exception::CpuStateShutdown(
					"Saveslot load failed; PS2 emulated state had to be shut down." );	// let the GUI handle the error "gracefully"
			}
			break;

		case 4:
			CycleFrameLimit(keymod->shift ? -1 : 1);
			break;

		// note: VK_F5-VK_F7 are reserved for GS
		case 8:
			GSmakeSnapshot( SNAPSHOTS_DIR "/" );
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
				string Text;
				if( strgametitle[0] != 0 ) 
				{
					// only take the first two words
					char name[256], *tok;
					string gsText;

					tok = strtok(strgametitle, " ");
					sprintf(name, "%s_", mystrlwr(tok));
					
					tok = strtok(NULL, " ");
					if( tok != NULL ) strcat(name, tok);

					ssprintf( gsText, "%s.%d.gs", name, StatesC);
					Text = Path::Combine( SSTATES_DIR, gsText );
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
