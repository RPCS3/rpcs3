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

#define _PC_	// disables MIPS opcode macros.

#include "IopCommon.h"
#include "Patch.h"
#include "GameDatabase.h"

#include <wx/textfile.h>
#include <wx/dir.h>
#include <wx/txtstrm.h>
#include <wx/zipstrm.h>

IniPatch Patch[ MAX_PATCH ];
IniPatch Cheat[ MAX_CHEAT ];

int patchnumber = 0;
int cheatnumber = 0;

wxString strgametitle;

struct PatchTextTable
{
	int				code;
	const wxChar*	text;
	PATCHTABLEFUNC*	func;
};

static const PatchTextTable commands_patch[] =
{
	{ 1, L"comment",	PatchFunc::comment },
	{ 2, L"patch",		PatchFunc::patch },
	{ 0, wxEmptyString, NULL } // Array Terminator
};

static const PatchTextTable commands_cheat[] =
{
	{ 1, L"comment",	PatchFunc::comment },
	{ 2, L"patch",		PatchFunc::cheat },
	{ 0, wxEmptyString, NULL } // Array Terminator
};

static const PatchTextTable dataType[] =
{
	{ 1, L"byte", NULL },
	{ 2, L"short", NULL },
	{ 3, L"word", NULL },
	{ 4, L"double", NULL },
	{ 5, L"extended", NULL },
	{ 0, wxEmptyString, NULL }
};

static const PatchTextTable cpuCore[] =
{
	{ 1, L"EE", NULL },
	{ 2, L"IOP", NULL },
	{ 0, wxEmptyString,  NULL }
};

// IniFile Functions.

void inifile_trim( wxString& buffer )
{
	buffer.Trim(false);			// trims left side.

	if( buffer.Length() <= 1 )	// this I'm not sure about... - air
	{
		buffer.Empty();
		return;
	}

	if( buffer.Left( 2 ) == L"//" )
	{
		buffer.Empty();
		return;
	}

	buffer.Trim(true);			// trims right side.
}

static int PatchTableExecute( const ParsedAssignmentString& set, const PatchTextTable * Table )
{
	int i = 0;

	while (Table[i].text[0])
	{
		if (!set.lvalue.Cmp(Table[i].text))
		{
			if (Table[i].func) Table[i].func(set.lvalue, set.rvalue);
			break;
		}
		i++;
	}

	return Table[i].code;
}

// This routine is for executing the commands of the ini file.
void inifile_command(bool isCheat, const wxString& cmd)
{
	ParsedAssignmentString set( cmd );

	// Is this really what we want to be doing here? Seems like just leaving it empty/blank
	// would make more sense... --air
    if (set.rvalue.IsEmpty()) set.rvalue = set.lvalue;

	/*int code = */PatchTableExecute(set, isCheat ? commands_cheat : commands_patch);
}

// This routine receives a string containing patches, trims it,
// Then sends the command to be parsed.
void TrimPatches(wxString& s)
{
	wxStringTokenizer tkn( s, L"\n" );
	
    while(tkn.HasMoreTokens()) {
		inifile_command(0, tkn.GetNextToken());
	}
}

// This routine loads patches from the game database
// Returns number of patches loaded
int InitPatches(const wxString& crc, const Game_Data& game)
{
	bool patchFound = false;
	wxString patch;
	patchnumber = 0;

	if (game.IsOk())
	{
		if (game.sectionExists(L"patches", crc)) {
			patch = game.getSection(L"patches", crc);
			patchFound = true;
		}
		else if (game.keyExists(L"[patches]")) {
			patch = game.getString(L"[patches]");
			patchFound = true;
		}
	}
	
	if (patchFound) TrimPatches(patch);
	
	return patchnumber;
}

void inifile_processString(const wxString& inStr)
{
  wxString str(inStr);
  inifile_trim(str);
  if (!str.IsEmpty()) inifile_command(1, str);
}

// This routine receives a file from inifile_read, trims it,
// Then sends the command to be parsed.
void inifile_process(wxTextFile &f1 )
{
    for (uint i = 0; i < f1.GetLineCount(); i++)
    {
        inifile_processString(f1[i]);
    }
}

void ResetCheatsCount()
{
  cheatnumber = 0;
}

static int LoadCheatsFiles(const wxDirName& folderName, wxString& fileSpec, const wxString& friendlyName)
{
	if (!folderName.Exists()) {
		Console.WriteLn(Color_Red, L"The %s folder ('%s') is inaccessible. Skipping...", friendlyName.c_str(), folderName.ToString().c_str());
		return 0;
	}
	wxDir dir(folderName.ToString());

	int before = cheatnumber;
	wxString buffer;
	wxTextFile f;
	bool found = dir.GetFirst(&buffer, L"*", wxDIR_FILES);
	while (found) {
		if (buffer.Upper().Matches(fileSpec.Upper())) {
			Console.WriteLn(Color_Gray, L"Found %s file: '%s'", friendlyName.c_str(), buffer.c_str());
			int before = cheatnumber;
			f.Open(Path::Combine(dir.GetName(), buffer));
			inifile_process(f);
			f.Close();
			int loaded = cheatnumber - before;
			Console.WriteLn((loaded ? Color_Green : Color_Gray), L"Loaded %d %s from '%s'", loaded, friendlyName.c_str(), buffer.c_str());
		}
		found = dir.GetNext(&buffer);
	}

	return cheatnumber - before;
}

// This routine loads cheats from a zip file
// Returns number of cheats loaded
// Note: Should be called after InitPatches()
// Note: only load cheats from the root folder of the zip
int LoadCheatsFromZip(wxString gameCRC, const wxString& cheatsArchiveFilename) {
  gameCRC.MakeUpper();

  int before = cheatnumber;

  std::auto_ptr<wxZipEntry> entry;
  wxFFileInputStream in(cheatsArchiveFilename);
  wxZipInputStream zip(in);
  while (entry.reset(zip.GetNextEntry()), entry.get() != NULL)
  {
    wxString name = entry->GetName();
    name.MakeUpper();
    if (name.Find(gameCRC) == 0 && name.Find(L".PNACH") == name.Length()-6) {
      Console.WriteLn(Color_Gray, L"Loading patch '%s' from archive '%s'",
                      entry->GetName().c_str(), cheatsArchiveFilename.c_str());
      wxTextInputStream pnach(zip);
      while (!zip.Eof()) {
        inifile_processString(pnach.ReadLine());
      }
    }
  }

  return cheatnumber - before;
}


// This routine loads cheats from *.pnach files
// Returns number of cheats loaded
// Note: Should be called after InitPatches()
int LoadCheats(wxString name, const wxDirName& folderName, const wxString& friendlyName)
{
	int loaded = 0;

	wxString filespec = name + L"*.pnach";
	loaded += LoadCheatsFiles(folderName, filespec, friendlyName);

	Console.WriteLn((loaded ? Color_Green : Color_Gray), L"Overall %d %s loaded", loaded, friendlyName.c_str());
	return loaded;
}

static u32 StrToU32(const wxString& str, int base = 10)
{
    unsigned long l;
    str.ToULong(&l, base);
    return l;
}

static u64 StrToU64(const wxString& str, int base = 10)
{
    wxULongLong_t l;
    str.ToULongLong(&l, base);
    return l;
}

// PatchFunc Functions.
namespace PatchFunc
{
    void comment( const wxString& text1, const wxString& text2 )
    {
        Console.WriteLn( L"comment: " + text2 );
    }

    struct PatchPieces
    {
		wxArrayString m_pieces;

		PatchPieces( const wxString& param )
		{
			SplitString( m_pieces, param, L"," );
			if( m_pieces.Count() < 5 )
				throw wxsFormat( L"Expected 5 data parameters; only found %d", m_pieces.Count() );
		}

		const wxString& PlaceToPatch() const	{ return m_pieces[0]; }
		const wxString& CpuType() const			{ return m_pieces[1]; }
		const wxString& MemAddr() const			{ return m_pieces[2]; }
		const wxString& OperandSize() const		{ return m_pieces[3]; }
		const wxString& WriteValue() const		{ return m_pieces[4]; }
    };

	template<bool isCheat> 
	void patchHelper(const wxString& cmd, const wxString& param) {
		// Error Handling Note:  I just throw simple wxStrings here, and then catch them below and
		// format them into more detailed cmd+data+error printouts.  If we want to add user-friendly
		// (translated) messages for display in a popup window then we'll have to upgrade the
		// exception a little bit.

        DevCon.WriteLn(cmd + L" " + param);

		try
		{
			if (isCheat && cheatnumber >= MAX_CHEAT)
				throw wxString( L"Maximum number of cheats reached" );
			if(!isCheat && patchnumber >= MAX_PATCH)
				throw wxString( L"Maximum number of patches reached" );

			IniPatch& iPatch = isCheat ? Cheat[cheatnumber] : Patch[patchnumber];
			PatchPieces pieces(param);

			iPatch.enabled = 0;

			iPatch.placetopatch	= StrToU32(pieces.PlaceToPatch(), 10);
			iPatch.cpu			= (patch_cpu_type)PatchTableExecute(pieces.CpuType(), cpuCore);
			iPatch.addr			= StrToU32(pieces.MemAddr(), 16);
			iPatch.type			= (patch_data_type)PatchTableExecute(pieces.OperandSize(), dataType);
			iPatch.data			= StrToU64(pieces.WriteValue(), 16);

			if (iPatch.cpu  == 0)
				throw wxsFormat(L"Unrecognized CPU Target: '%s'", pieces.CpuType().c_str());

			if (iPatch.type == 0)
				throw wxsFormat(L"Unrecognized Operand Size: '%s'", pieces.OperandSize().c_str());

			iPatch.enabled = 1; // omg success!!

			if (isCheat) cheatnumber++;
			else		 patchnumber++;
		}
		catch( wxString& exmsg )
		{
			Console.Error(L"(Patch) Error Parsing: %s=%s", cmd.c_str(), param.c_str());
			Console.Indent().Error( exmsg );
		}
	}
	void patch(const wxString& cmd, const wxString& param) { patchHelper<0>(cmd, param); }
	void cheat(const wxString& cmd, const wxString& param) { patchHelper<1>(cmd, param); }
}

// This is for applying patches directly to memory
void ApplyPatch(int place)
{
	for (int i = 0; i < patchnumber; i++)
	{
	    if (Patch[i].placetopatch == place)
            _ApplyPatch(&Patch[i]);
	}
}

// This is for applying cheats directly to memory
void ApplyCheat(int place)
{
	for (int i = 0; i < cheatnumber; i++)
	{
	    if (Cheat[i].placetopatch == place)
            _ApplyPatch(&Cheat[i]);
	}
}
