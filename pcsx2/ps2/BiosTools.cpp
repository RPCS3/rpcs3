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
#include "Common.h"
#include "BiosTools.h"

#include "Utilities/pxStreams.h"
#include "wx/ffile.h"


// FIXME: Temporary hack until we remove dependence on Pcsx2App.
#include "AppConfig.h"
#include "wx/mstream.h"
#include "wx/wfstream.h"

#define DIRENTRY_SIZE 16

// --------------------------------------------------------------------------------------
// romdir structure (packing required!)
// --------------------------------------------------------------------------------------
//
#if defined(_MSC_VER)
#	pragma pack(1)
#endif

struct romdir
{
	char fileName[10];
	u16 extInfoSize;
	u32 fileSize;
} __packed;			// +16

#ifdef _MSC_VER
#	pragma pack()
#endif

C_ASSERT( sizeof(romdir) == DIRENTRY_SIZE );

u32 BiosVersion;
u32 BiosChecksum;
wxString BiosDescription;


// --------------------------------------------------------------------------------------
//  Exception::BiosLoadFailed  (implementations)
// --------------------------------------------------------------------------------------
Exception::BiosLoadFailed::BiosLoadFailed( const wxString& filename )
{
	StreamName = filename;
}

// This method throws a BadStream exception if the bios information chould not be obtained.
//  (indicating that the file is invalid, incomplete, corrupted, or plain naughty).
static void LoadBiosVersion( pxInputStream& fp, u32& version, wxString& description, wxString& zoneStr )
{
	uint i;
	romdir rd;

	for (i=0; i<512*1024; i++)
	{
		fp.Read( rd );
		if (strncmp( rd.fileName, "RESET", 5 ) == 0)
			break; /* found romdir */
	}

	if (i == 512*1024)
	{
		throw Exception::BadStream( fp.GetStreamName() )
			.SetDiagMsg(L"BIOS version check failed: 'RESET' tag could not be found.")
			.SetUserMsg(_("The selected BIOS file is not a valid PS2 BIOS.  Please re-configure."));
	}

	uint fileOffset = 0;

	while(strlen(rd.fileName) > 0)
	{
		if (strcmp(rd.fileName, "ROMVER") == 0)
		{
			char romver[14+1];		// ascii version loaded from disk.

			wxFileOffset filetablepos = fp.Tell();
			fp.Seek( fileOffset );
			fp.Read( &romver, 14 );
			fp.Seek( filetablepos );	//go back

			romver[14] = 0;

			const char zonefail[2] = { romver[4], '\0' };	// the default "zone" (unknown code)
			const char* zone = zonefail;

			switch(romver[4])
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

			char vermaj[3] = { romver[0], romver[1], 0 };
			char vermin[3] = { romver[2], romver[3], 0 };

			FastFormatUnicode result;
			result.Write( "%-7s v%s.%s(%c%c/%c%c/%c%c%c%c)  %s",
				zone,
				vermaj, vermin,
				romver[12], romver[13],	// day
				romver[10], romver[11],	// month
				romver[6], romver[7], romver[8], romver[9],	// year!
				(romver[5]=='C') ? "Console" : (romver[5]=='D') ? "Devel" : ""
			);

			version = strtol(vermaj, (char**)NULL, 0) << 8;
			version|= strtol(vermin, (char**)NULL, 0);

			Console.WriteLn(L"Bios Found: %s", result.c_str());

			description = result.c_str();
			zoneStr = fromUTF8(zone);
		}

		if ((rd.fileSize % 0x10) == 0)
			fileOffset += rd.fileSize;
		else
			fileOffset += (rd.fileSize + 0x10) & 0xfffffff0;

		fp.Read( rd );
	}

	fileOffset -= ((rd.fileSize + 0x10) & 0xfffffff0) - rd.fileSize;

	if (description.IsEmpty())
		throw Exception::BadStream( fp.GetStreamName() )
			.SetDiagMsg(L"BIOS version check failed: 'ROMDIR' tag could not be found.")
			.SetUserMsg(_("The selected BIOS file is not a valid PS2 BIOS.  Please re-configure."));
	
	wxFileOffset fileSize = fp.Length();
	if (fileSize < (int)fileOffset)
	{
		description += pxsFmt( L" %d%%", ((fileSize*100) / (int)fileOffset) );
		// we force users to have correct bioses,
		// not that lame scph10000 of 513KB ;-)
	}
}

static void LoadBiosVersion( pxInputStream& fp, u32& version, wxString& description )
{
	wxString zoneStr;
	LoadBiosVersion( fp,version, description, zoneStr );
}

template< size_t _size >
void ChecksumIt( u32& result, const u8 (&srcdata)[_size] )
{
	pxAssume( (_size & 3) == 0 );
	for( size_t i=0; i<_size/4; ++i )
		result ^= ((u32*)srcdata)[i];
}

// Attempts to load a BIOS rom sub-component, by trying multiple combinations of base
// filename and extension.  The bios specified in the user's configuration is used as
// the base.
//
// Parameters:
//   ext - extension of the sub-component to load.  Valid options are rom1, rom2, AND erom.
//
template< size_t _size >
static void LoadExtraRom( const wxChar* ext, u8 (&dest)[_size] )
{
	wxString Bios1;
	s64 filesize;

	// Try first a basic extension concatenation (normally results in something like name.bin.rom1)
	const wxString Bios( g_Conf->FullpathToBios() );
	Bios1.Printf( L"%s.%s", Bios.c_str(), ext);

	try
	{
		if( (filesize=Path::GetFileSize( Bios1 ) ) <= 0 )
		{
			// Try the name properly extensioned next (name.rom1)
			Bios1 = Path::ReplaceExtension( Bios, ext );
			if( (filesize=Path::GetFileSize( Bios1 ) ) <= 0 )
			{
				Console.WriteLn( Color_Gray, L"BIOS %s module not found, skipping...", ext );
				return;
			}
		}

		wxFile fp( Bios1 );
		fp.Read( dest, std::min<s64>( _size, filesize ) );
		
		// Checksum for ROM1, ROM2, EROM?  Rama says no, Gigaherz says yes.  I'm not sure either way.  --air
		//ChecksumIt( BiosChecksum, dest );
	}
	catch (Exception::BadStream& ex)
	{
		// If any of the secondary roms fail,its a non-critical error.
		// Log it, but don't make a big stink.  99% of games and stuff will
		// still work fine.

		Console.Warning(L"BIOS Warning: %s could not be read (permission denied?)", ext);
		Console.Indent().WriteLn(L"Details: %s", ex.FormatDiagnosticMessage());
		Console.Indent().WriteLn(L"File size: %llu", filesize);
	}
}

// Loads the configured bios rom file into PS2 memory.  PS2 memory must be allocated prior to
// this method being called.
//
// Remarks:
//   This function does not fail if rom1, rom2, or erom files are missing, since none are
//   explicitly required for most emulation tasks.
//
// Exceptions:
//   BadStream - Thrown if the primary bios file (usually .bin) is not found, corrupted, etc.
//
void LoadBIOS()
{
	pxAssertDev( eeMem->ROM != NULL, "PS2 system memory has not been initialized yet." );

	try
	{
		wxString Bios( g_Conf->FullpathToBios() );
		if( !g_Conf->BaseFilenames.Bios.IsOk() || g_Conf->BaseFilenames.Bios.IsDir() )
			throw Exception::FileNotFound( Bios )
				.SetDiagMsg(L"BIOS has not been configured, or the configuration has been corrupted.")
				.SetUserMsg(_("The PS2 BIOS could not be loaded.  The BIOS has not been configured, or the configuration has been corrupted.  Please re-configure."));

		s64 filesize = Path::GetFileSize( Bios );
		if( filesize <= 0 )
		{
			throw Exception::FileNotFound( Bios )
				.SetDiagMsg(L"Configured BIOS file does not exist, or has a file size of zero.")
				.SetUserMsg(_("The configured BIOS file does not exist.  Please re-configure."));
		}

		BiosChecksum = 0;

		wxString biosZone;
		wxFFile fp( Bios );
		fp.Read( eeMem->ROM, std::min<s64>( Ps2MemSize::Rom, filesize ) );

		ChecksumIt( BiosChecksum, eeMem->ROM );

		pxInputStream memfp( Bios, new wxMemoryInputStream( eeMem->ROM, sizeof(eeMem->ROM) ) );
		LoadBiosVersion( memfp, BiosVersion, BiosDescription, biosZone );
		
		Console.SetTitle( pxsFmt( L"Running BIOS (%s v%u.%u)",
			biosZone.c_str(), BiosVersion >> 8, BiosVersion & 0xff
		));

		//injectIRX("host.irx");	//not fully tested; still buggy

		LoadExtraRom( L"rom1", eeMem->ROM1 );
		LoadExtraRom( L"rom2", eeMem->ROM2 );
		LoadExtraRom( L"erom", eeMem->EROM );
	}
	catch (Exception::BadStream& ex)
	{
		// Rethrow as a Bios Load Failure, so that the user interface handling the exceptions
		// can respond to it appropriately.
		throw Exception::BiosLoadFailed( ex.StreamName )
			.SetDiagMsg( ex.DiagMsg() )
			.SetUserMsg( ex.UserMsg() );
	}
}

bool IsBIOS(const wxString& filename, wxString& description)
{
	wxFileName Bios( g_Conf->Folders.Bios + filename );
	pxInputStream inway( filename, new wxFFileInputStream( filename ) );
	
	if (!inway.IsOk()) return false;
	if (inway.Length() < 512*1024) return false;

	try {
		u32 version;
		LoadBiosVersion( inway, version, description );
		return true;
	} catch( Exception::BadStream& ) { }

	return false;	// fail quietly
}
