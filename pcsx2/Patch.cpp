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
#include "DataBase_Loader.h"
#include <wx/textfile.h>

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
		buffer.Clear();
		return;
	}

	if( buffer.Left( 2 ) == L"//" )
	{
		buffer.Clear();
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

	int code = PatchTableExecute(set, isCheat ? commands_cheat : commands_patch);
}

// This routine receives a string containing patches, trims it,
// Then sends the command to be parsed.
void TrimPatches(string& s)
{
	String_Stream ss(s);
	wxString buff;
    while (!ss.finished()) {
		buff = ss.getLineWX();
		inifile_trim(buff);
		if (!buff.IsEmpty()) inifile_command(0, buff);
	}
}

// This routine loads patches from the game database
// Returns number of patches loaded
int InitPatches(const wxString& name)
{
	bool   patchFound = false;
	string patch;
	string crc  = string(name.ToUTF8().data());
	patchnumber = 0;
	
	if (GameDB && GameDB->gameLoaded()) {
		if (GameDB->keyExists("[patches = " + crc + "]")) {
			patch = GameDB->getString("[patches = " + crc + "]");
			patchFound = true;
		}
		else if (GameDB->keyExists("[patches]")) {
			patch = GameDB->getString("[patches]");
			patchFound = true;
		}
	}

	if (patchFound) {
		Console.WriteLn(Color_Green, "Patch found!");
		TrimPatches(patch);
	}
	else Console.WriteLn(Color_Gray, "No patch found. Resuming execution without a patch (this is NOT an error).");
	
	Console.WriteLn("Patches Loaded: %d", patchnumber);
	return patchnumber;
}

// This routine receives a file from inifile_read, trims it,
// Then sends the command to be parsed.
void inifile_process(wxTextFile &f1 )
{
    for (uint i = 0; i < f1.GetLineCount(); i++)
    {
        inifile_trim(f1[i]);
        if (!f1[i].IsEmpty()) inifile_command(1, f1[i]);
    }
}

// This routine loads cheats from *.pnach files
// Returns number of cheats loaded
// Note: Should be called after InitPatches()
int InitCheats(const wxString& name)
{
	wxTextFile f1;
	wxString buffer;
	cheatnumber = 0;

	// FIXME : We need to add a 'cheats' folder to the AppConfig, and use that instead. --air

	buffer = Path::Combine(L"cheats", name + L".pnach");

	if(!f1.Open(buffer) && wxFileName::IsCaseSensitive())
	{
		f1.Open( Path::Combine(L"cheats", name.Upper() + L".pnach") );
	}

	if(!f1.IsOpened())
	{
		Console.WriteLn( Color_Gray, "No cheats found. Resuming execution without cheats..." );
		return 0;
	}

	Console.WriteLn( Color_Green, "Cheats found!");
	inifile_process( f1 );

	Console.WriteLn("Cheats Loaded: %d", cheatnumber);
	return cheatnumber;
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
