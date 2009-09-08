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
#include "AppConfig.h"
#include "Common.h"
#include "BiosTools.h"

#include "wx/file.h"

#define DIRENTRY_SIZE 16

//////////////////////////////////////////////////////////////////////////////////////////
// romdir structure (packing required!)
//
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

//////////////////////////////////////////////////////////////////////////////////////////

u32 BiosVersion;  //  Used in Memory, Misc, CDVD

// Returns a string message telling the user to consult guides for obtaining a legal BIOS.
// This message is in a function because it's used as part of several dialogs in PCSX2 (there
// are multiple variations on the BIOS and BIOS folder checks).
wxString BIOS_GetMsg_Required()
{
	return pxE( ".Popup:BiosDumpRequired",
		L"PCSX2 requires a PS2 BIOS in order to run.  For legal reasons, you *must* obtain \n"
		L"a BIOS from an actual PS2 unit that you own (borrowing doesn't count).\n"
		L"Please consult the FAQs and Guides for further instructions."
	);
}

// Returns the version information of the bios currently loaded into memory.
static u32 GetBiosVersion()
{
	int i;

	for (i=0; i<512*1024; i++)
	{
		if( strncmp( (char*)&psRu8(i), "RESET", 5 ) == 0 )
			break; /* found romdir */
	}
	if (i == 512*1024) return -1;

	const romdir* rd = (romdir*)&psRu8(i);
	uint fileOffset = 0;

	while(strlen(rd->fileName) > 0)
	{
		if (strcmp(rd->fileName, "ROMVER") == 0)
		{
			char vermaj[8];
			char vermin[8];
			u32 version;

			const s8 *ROMVER = &psRs8(fileOffset);

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

// Attempts to load a BIOS rom sub-component, by trying multiple combinations of base
// filename and extension.  The bios specified in the user's configuration is used as
// the base.
//
// Parameters:
//   ext - extension of the sub-component to load.  Valid options are rom1, rom2, AND erom.
//
static void loadBiosRom( const wxChar *ext, u8 *dest, long maxSize )
{
	wxString Bios1;
	long filesize;

	// Try first a basic extension concatenation (normally results in something like name.bin.rom1)
	const wxString Bios( g_Conf->FullpathToBios() );
	Bios1.Printf( L"%s.%s", Bios.c_str(), ext);

	if( (filesize=Path::GetFileSize( Bios1 ) ) <= 0 )
	{
		// Try the name properly extensioned next (name.rom1)
		Bios1 = Path::ReplaceExtension( Bios, ext );
		if( (filesize=Path::GetFileSize( Bios1 ) ) <= 0 )
		{
			Console::Notice( "Load Bios Warning: %s not found (this is not an error!)", wxString(ext).ToAscii().data() );
			return;
		}
	}

	// if we made it this far, we have a successful file found:

	wxFile fp( Bios1 );
	fp.Read( dest, min( maxSize, filesize ) );
}

// Loads the configured bios rom file into PS2 memory.  PS2 memory must be allocated prior to
// this method being called.
//
// Remarks:
//   This function does not fail if rom1, rom2, or erom files are missing, since none are
//   explicitly required for most emulation tasks.
//
// Exceptions:
//   FileNotFound - Thrown if the primary bios file (usually .bin) is not found.
//
void LoadBIOS()
{
	DevAssert( PS2MEM_ROM != NULL, "PS2 system memory has not been initialized yet." );
	
	wxString Bios( g_Conf->FullpathToBios() );

	long filesize = Path::GetFileSize( Bios );
	if( filesize > 0 )
	{
		wxFile fp( Bios.c_str() );
		fp.Read( PS2MEM_ROM, min( (long)Ps2MemSize::Rom, filesize ) );
	}
	else
	{
		// Translated: Bios file not found or not specified ... A bios is required for Pcsx2 to run!
		throw Exception::FileNotFound( Bios,
			L"Configured Bios file does not exist",
			
			_("The configured BIOS file does not exist, or no BIOS has been configured.\n\n") +
			BIOS_GetMsg_Required()
		);
	}

	BiosVersion = GetBiosVersion();
	Console::Status("Bios Version %d.%d", BiosVersion >> 8, BiosVersion & 0xff);

	//injectIRX("host.irx");	//not fully tested; still buggy

	loadBiosRom( L"rom1", PS2MEM_ROM1, Ps2MemSize::Rom1 );
	loadBiosRom( L"rom2", PS2MEM_ROM2, Ps2MemSize::Rom2 );
	loadBiosRom( L"erom", PS2MEM_EROM, Ps2MemSize::ERom );
}

bool IsBIOS(const wxString& filename, wxString& description)
{
	uint fileOffset=0;
	romdir rd;

	wxFileName Bios( g_Conf->Folders.Bios + filename );
	wxFile fp( Bios.GetFullPath().c_str() );

	if( !fp.IsOpened() ) return FALSE;

	int biosFileSize = fp.Length();
	if( biosFileSize < 1024*4096) return FALSE;		// valid bios must be at least 4mb.

	while( (fp.Tell() < 512*1024) && (fp.Read( &rd, DIRENTRY_SIZE ) == DIRENTRY_SIZE) )
	{
		if (strcmp(rd.fileName, "RESET") == 0)
			break;	// found romdir
	}

	if ((strcmp(rd.fileName, "RESET") != 0) || (rd.fileSize == 0))
		return FALSE;	//Unable to locate ROMDIR structure in file or a ioprpXXX.img

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
				case 'T': zone = "T10K";	break;
				case 'X': zone = "Test";	break;
				case 'J': zone = "Japan";	break;
				case 'A': zone = "USA";		break;
				case 'E': zone = "Europe";	break;
				case 'H': zone = "HK";		break;
				case 'P': zone = "Free";	break;
				case 'C': zone = "China";	break;
			}

			const wxString romver( wxString::FromAscii(aROMVER) );

			description.Printf( L"%-7s v%c%c.%c%c(%c%c/%c%c/%c%c%c%c)  %s",
				wxString::FromAscii(zone).c_str(),
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
	fileOffset -= ((rd.fileSize + 0x10) & 0xfffffff0) - rd.fileSize;

	if (found)
	{
		if ( biosFileSize < (int)fileOffset)
		{
			description += wxsFormat( L" %d%%", ((biosFileSize*100) / (int)fileOffset) );
			// we force users to have correct bioses,
			// not that lame scph10000 of 513KB ;-)
		}
		return true;
	}

	return false;	//fail quietly
}
