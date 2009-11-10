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

#define _PC_	// disables MIPS opcode macros.

#include "IopCommon.h"
#include "Patch.h"

#include <wx/textfile.h>

IniPatch Patch[ MAX_PATCH ];

u32 SkipCount = 0, IterationCount = 0;
u32 IterationIncrement = 0, ValueIncrement = 0;
u32 PrevCheatType = 0, PrevCheatAddr = 0,LastType = 0;

int g_ZeroGSOptions = 0, patchnumber;

wxString strgametitle;

struct PatchTextTable
{
	int				code;
	const wxChar*	text;
	PATCHTABLEFUNC*	func;
};

static const PatchTextTable commands[] =
{
	{ 1, L"comment", PatchFunc::comment },
	{ 2, L"gametitle", PatchFunc::gametitle },
	{ 3, L"patch", PatchFunc::patch },
	{ 4, L"fastmemory", NULL }, // enable for faster but bugger mem (mvc2 is faster)
	{ 5, L"roundmode", PatchFunc::roundmode }, // changes rounding mode for floating point
											// syntax: roundmode=X,Y
											// possible values for X,Y: NEAR, DOWN, UP, CHOP
											// X - EE rounding mode (default is NEAR)
											// Y - VU rounding mode (default is CHOP)
	{ 6, L"zerogs", PatchFunc::zerogs }, // zerogs=hex
	{ 7, L"vunanmode", NULL },
	{ 8, L"ffxhack", NULL},			// *obsolete*
	{ 9, L"xkickdelay", NULL},

	{ 0, wxEmptyString, NULL }		// Array Terminator
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

void writeCheat()
{
	switch (LastType)
	{
		case 0x0:
			memWrite8(PrevCheatAddr,IterationIncrement & 0xFF);
			break;
		case 0x1:
			memWrite16(PrevCheatAddr,IterationIncrement & 0xFFFF);
			break;
		case 0x2:
			memWrite32(PrevCheatAddr,IterationIncrement);
			break;
		default:
			break;
	}
}

void handle_extended_t( IniPatch *p)
{
	if (SkipCount > 0)
	{
		SkipCount--;
	}
	else switch (PrevCheatType)
	{
		case 0x3040: // vvvvvvvv  00000000  Inc
		{
			u32 mem = memRead32(PrevCheatAddr);
			memWrite32(PrevCheatAddr, mem + (p->addr));
			PrevCheatType = 0;
			break;
		}

		case 0x3050: // vvvvvvvv  00000000  Dec
		{
			u32 mem = memRead32(PrevCheatAddr);
			memWrite32(PrevCheatAddr, mem - (p->addr));
			PrevCheatType = 0;
			break;
		}

		case 0x4000: // vvvvvvvv  iiiiiiii
			for(u32 i = 0; i < IterationCount; i++)
			{
				memWrite32((u32)(PrevCheatAddr + (i * IterationIncrement)),(u32)(p->addr + ((u32)p->data * i)));
			}
			PrevCheatType = 0;
			break;

		case 0x5000: // dddddddd  iiiiiiii
			for(u32 i = 0; i < IterationCount; i++)
			{
				u8 mem = memRead8(PrevCheatAddr + i);
				memWrite8(((u32)p->data) + i, mem);
			}
			PrevCheatType = 0;
			break;

		case 0x6000: // 000Xnnnn  iiiiiiii
			if (IterationIncrement == 0x0)
			{
				//LastType = ((u32)p->addr & 0x000F0000) >> 16;
				u32 mem = memRead32(PrevCheatAddr);
				if ((u32)p->addr < 0x100)
				{
					LastType = 0x0;
					PrevCheatAddr = mem + ((u32)p->addr);
				}
				else if ((u32)p->addr < 0x1000)
				{
					LastType = 0x1;
					PrevCheatAddr = mem + ((u32)p->addr * 2);
				}
				else
				{
					LastType = 0x2;
					PrevCheatAddr = mem + ((u32)p->addr * 4);
				}

				// Check if needed to read another pointer
				PrevCheatType = 0;
				if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0)
				{
				    switch(LastType)
				    {
				        case 0x0:
                            memWrite8(PrevCheatAddr,(u8)p->data & 0xFF);
                            break;
				        case 0x1:
                            memWrite16(PrevCheatAddr,(u16)p->data & 0x0FFFF);
                            break;
				        case 0x2:
                            memWrite32(PrevCheatAddr,(u32)p->data);
                            break;
				        default:
                            break;
				    }
				}
			}
			else
			{
				// Get Number of pointers
				if (((u32)p->addr & 0x0000FFFF) == 0)
					IterationCount = 1;
				else
					IterationCount = (u32)p->addr & 0x0000FFFF;

				// Read first pointer
				LastType = ((u32)p->addr & 0x000F0000) >> 16;
				u32 mem = memRead32(PrevCheatAddr);

				PrevCheatAddr = mem + (u32)p->data;
				IterationCount--;

				// Check if needed to read another pointer
				if (IterationCount == 0)
				{
					PrevCheatType = 0;
					if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0) writeCheat();
				}
				else
				{
					if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0)
						PrevCheatType = 0;
					else
						PrevCheatType = 0x6001;
				}
			}
			break;

		case 0x6001: // 000Xnnnn  iiiiiiii
		{
			// Read first pointer
			u32 mem = memRead32(PrevCheatAddr & 0x0FFFFFFF);

			PrevCheatAddr = mem + (u32)p->addr;
			IterationCount--;

			// Check if needed to read another pointer
			if (IterationCount == 0)
			{
				PrevCheatType = 0;
				if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0) writeCheat();
			}
			else
			{
				mem = memRead32(PrevCheatAddr);

				PrevCheatAddr = mem + (u32)p->data;
				IterationCount--;
				if (IterationCount == 0)
				{
					PrevCheatType = 0;
					if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0) writeCheat();
				}
			}
		}
		break;

		default:
            if ((p->addr & 0xF0000000) == 0x00000000) // 0aaaaaaa 0000000vv
            {
                memWrite8(p->addr & 0x0FFFFFFF, (u8)p->data & 0x000000FF);
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xF0000000) == 0x10000000) // 0aaaaaaa 0000vvvv
            {
                memWrite16(p->addr & 0x0FFFFFFF, (u16)p->data & 0x0000FFFF);
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xF0000000) == 0x20000000) // 0aaaaaaa vvvvvvvv
            {
                memWrite32(p->addr & 0x0FFFFFFF, (u32)p->data);
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30000000) // 300000vv 0aaaaaaa  Inc
            {
                u8 mem = memRead8((u32)p->data);
                memWrite8((u32)p->data, mem + (p->addr & 0x000000FF));
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30100000) // 301000vv 0aaaaaaa  Dec
            {
                u8 mem = memRead8((u32)p->data);
                memWrite8((u32)p->data, mem - (p->addr & 0x000000FF));
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30200000) // 3020vvvv 0aaaaaaa Inc
            {
                u16 mem = memRead16((u32)p->data);
                memWrite16((u32)p->data, mem + (p->addr & 0x0000FFFF));
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30300000) // 3030vvvv 0aaaaaaa Dec
            {
                u16 mem = memRead16((u32)p->data);
                memWrite16((u32)p->data, mem - (p->addr & 0x0000FFFF));
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30400000) // 30400000 0aaaaaaa Inc	+ Another line
            {
                PrevCheatType = 0x3040;
                PrevCheatAddr = (u32)p->data;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30500000) // 30500000 0aaaaaaa Inc	+ Another line
            {
                PrevCheatType = 0x3050;
                PrevCheatAddr = (u32)p->data;
            }
            else if ((p->addr & 0xF0000000) == 0x40000000) // 4aaaaaaa nnnnssss + Another line
            {
                IterationCount = ((u32)p->data & 0xFFFF0000) / 0x10000;
                IterationIncrement = ((u32)p->data & 0x0000FFFF) * 4;
                PrevCheatAddr = (u32)p->addr & 0x0FFFFFFF;
                PrevCheatType = 0x4000;
            }
            else if ((p->addr & 0xF0000000) == 0x50000000) // 5sssssss nnnnnnnn + Another line
            {
                PrevCheatAddr = (u32)p->addr & 0x0FFFFFFF;
                IterationCount = ((u32)p->data);
                PrevCheatType = 0x5000;
            }
            else if ((p->addr & 0xF0000000) == 0x60000000) // 6aaaaaaa 000000vv + Another line/s
            {
                PrevCheatAddr = (u32)p->addr & 0x0FFFFFFF;
                IterationIncrement = ((u32)p->data);
                IterationCount = 0;
                PrevCheatType = 0x6000;
            }
            else if ((p->addr & 0xF0000000) == 0x70000000)
            {
                if ((p->data & 0x00F00000) == 0x00000000) // 7aaaaaaa 000000vv
                {
                    u8 mem = memRead8((u32)p->addr & 0x0FFFFFFF);
                    memWrite8((u32)p->addr & 0x0FFFFFFF,(u8)(mem | (p->data & 0x000000FF)));
                }
                else if ((p->data & 0x00F00000) == 0x00100000) // 7aaaaaaa 0010vvvv
                {
                    u16 mem = memRead16((u32)p->addr & 0x0FFFFFFF);
                    memWrite16((u32)p->addr & 0x0FFFFFFF,(u16)(mem | (p->data & 0x0000FFFF)));
                }
                else if ((p->data & 0x00F00000) == 0x00200000) // 7aaaaaaa 002000vv
                {
                    u8 mem = memRead8((u32)p->addr&0x0FFFFFFF);
                    memWrite8((u32)p->addr & 0x0FFFFFFF,(u8)(mem & (p->data & 0x000000FF)));
                }
                else if ((p->data & 0x00F00000) == 0x00300000) // 7aaaaaaa 0030vvvv
                {
                    u16 mem = memRead16((u32)p->addr & 0x0FFFFFFF);
                    memWrite16((u32)p->addr & 0x0FFFFFFF,(u16)(mem & (p->data & 0x0000FFFF)));
                }
                else if ((p->data & 0x00F00000) == 0x00400000) // 7aaaaaaa 004000vv
                {
                    u8 mem = memRead8((u32)p->addr & 0x0FFFFFFF);
                    memWrite8((u32)p->addr & 0x0FFFFFFF,(u8)(mem ^ (p->data & 0x000000FF)));
                }
                else if ((p->data & 0x00F00000) == 0x00500000) // 7aaaaaaa 0050vvvv
                {
                    u16 mem = memRead16((u32)p->addr & 0x0FFFFFFF);
                    memWrite16((u32)p->addr & 0x0FFFFFFF,(u16)(mem ^ (p->data & 0x0000FFFF)));
                }
            }
            else if (p->addr < 0xE0000000)
            {
                if ((((u32)p->data & 0xFFFF0000) == 0x00000000) ||
                    (((u32)p->data & 0xFFFF0000) == 0x00100000) ||
                    (((u32)p->data & 0xFFFF0000) == 0x00200000) ||
                    (((u32)p->data & 0xFFFF0000) == 0x00300000))
                {
                    u16 mem = memRead16((u32)p->addr & 0x0FFFFFFF);
                    if (mem != (0x0000FFFF & (u32)p->data)) SkipCount = 1;
                    PrevCheatType = 0;
                }
            }
            else if (p->addr < 0xF0000000)
            {
                if (((u32)p->data & 0xC0000000) == 0x00000000)
                if ((((u32)p->data & 0xF0000000) == 0x00000000) ||
                    (((u32)p->data & 0xF0000000) == 0x10000000) ||
                    (((u32)p->data & 0xF0000000) == 0x20000000) ||
                    (((u32)p->data & 0xF0000000) == 0x30000000))
                {
                    u16 mem = memRead16((u32)p->data & 0x0FFFFFFF);
                    if (mem != (0x0000FFFF & (u32)p->addr)) SkipCount = ((u32)p->addr & 0xFFF0000) / 0x10000;
                    PrevCheatType = 0;
                }
            }
	}
}

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
void inifile_command( const wxString& cmd )
{
	ParsedAssignmentString set( cmd );

	// Is this really what we want to be doing here? Seems like just leaving it empty/blank
	// would make more sense... --air
    if (set.rvalue.IsEmpty()) set.rvalue = set.lvalue;

    int code = PatchTableExecute( set, commands );
}

// This routine recieves a file from inifile_read, trims it,
// Then sends the command to be parsed.
void inifile_process(wxTextFile &f1 )
{
    Console.WriteLn("inifile_process");
    for (uint i = 0; i < f1.GetLineCount(); i++)
    {
        inifile_trim(f1[i]);
        if (!f1[i].IsEmpty()) inifile_command(f1[i]);
    }
}

// This routine creates a pnach filename from the games crc,
// loads it, trims the commands, and sends them to be parsed.
void inifile_read(const wxString& name )
{
	wxTextFile f1;
	wxString buffer;

	patchnumber = 0;

	// FIXME : We need to add a 'patches' folder to the AppConfig, and use that instead. --air

	buffer = Path::Combine(L"patches", name + L".pnach");

	if(!f1.Open(buffer) && wxFileName::IsCaseSensitive())
	{
		f1.Open( Path::Combine(L"patches", name.Upper() + L".pnach") );
	}

	if(!f1.IsOpened())
	{
		Console.WriteLn( Color_Gray, "No patch found. Resuming execution without a patch (this is NOT an error)." );
		return;
	}

	Console.WriteLn( Color_Green, "Patch found!");
	inifile_process( f1 );
}

void _ApplyPatch(IniPatch *p)
{
	if (p->enabled == 0) return;

	switch (p->cpu)
	{
		case CPU_EE:
			switch (p->type)
			{
				case BYTE_T:
					memWrite8(p->addr, (u8)p->data);
					break;

				case SHORT_T:
					memWrite16(p->addr, (u16)p->data);
					break;

				case WORD_T:
					memWrite32(p->addr, (u32)p->data);
					break;

				case DOUBLE_T:
					memWrite64(p->addr, &p->data);
					break;

				case EXTENDED_T:
					handle_extended_t(p);
					break;

				default:
					break;
			}
			break;

		case CPU_IOP:
			switch (p->type)
			{
				case BYTE_T:
					iopMemWrite8(p->addr, (u8)p->data);
					break;
				case SHORT_T:
					iopMemWrite16(p->addr, (u16)p->data);
					break;
				case WORD_T:
					iopMemWrite32(p->addr, (u32)p->data);
					break;
				default:
					break;
			}
			break;

		default:
			break;
	}
}

//this is for applying patches directly to memory
void ApplyPatch(int place)
{
	for (int i = 0; i < patchnumber; i++)
	{
	    if (Patch[i].placetopatch == place)
            _ApplyPatch(&Patch[i]);
	}
}

void InitPatch(const wxString& crc)
{
    inifile_read(crc);
    Console.WriteLn("patchnumber: %d", patchnumber);
    ApplyPatch(0);
}

void ResetPatch( void )
{
	patchnumber = 0;
}

int AddPatch(int Mode, int Place, int Address, int Size, u64 data)
{
	if ( patchnumber >= MAX_PATCH )
	{
		Console.Error( "Patch ERROR: Maximum number of patches reached.");
		return -1;
	}

	Patch[patchnumber].placetopatch = Mode;
	Patch[patchnumber].cpu = (patch_cpu_type)Place;
	Patch[patchnumber].addr = Address;
	Patch[patchnumber].type = (patch_data_type)Size;
	Patch[patchnumber].data = data;
	return patchnumber++;
}

void PrintPatch(int i)
{
	Console.WriteLn("Patch[%d]:", i);
	if (Patch[i].enabled == 0)
        Console.WriteLn("Disabled.");
    else
        Console.WriteLn("Enabled.");

    Console.WriteLn("PlaceToPatch:%d", Patch[i].placetopatch);

    switch(Patch[i].cpu)
    {
        case CPU_EE:	Console.WriteLn("Cpu: EE"); break;
        case CPU_IOP:	Console.WriteLn("Cpu: IOP"); break;
        default:		Console.WriteLn("Cpu: None"); break;
    }

    Console.WriteLn("Address: %X", Patch[i].addr);

    switch (Patch[i].type)
    {
        case BYTE_T:		Console.WriteLn("Type: Byte"); break;
        case SHORT_T:		Console.WriteLn("Type: Short"); break;
        case WORD_T:		Console.WriteLn("Type: Word"); break;
        case DOUBLE_T:		Console.WriteLn("Type: Double"); break;
        case EXTENDED_T:	Console.WriteLn("Type: Extended"); break;

        default:			Console.WriteLn("Type: None"); break;
    }

    Console.WriteLn("Data: %I64X", Patch[i].data);
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

    void gametitle( const wxString& text1, const wxString& text2 )
    {
        Console.WriteLn( L"gametitle: " + text2 );
        strgametitle = text2;
        Console.SetTitle( strgametitle );
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

    void patch( const wxString& cmd, const wxString& param )
    {
		// Error Handling Note:  I just throw simple wxStrings here, and then catch them below and
		// format them into more detailed cmd+data+error printouts.  If we want to add user-friendly
		// (translated) messages for display in a popup window then we'll have to upgrade the
		// exception a little bit.

        DevCon.WriteLn(cmd + L" " + param);

		try
		{
			if ( patchnumber >= MAX_PATCH )
				throw wxString( L"Maximum number of patches reached" );
	        
			Patch[patchnumber].enabled = 0;
			PatchPieces pieces( param );

			Patch[patchnumber].placetopatch	= StrToU32(pieces.PlaceToPatch(), 10);
			Patch[patchnumber].cpu			= (patch_cpu_type)PatchTableExecute( pieces.CpuType(), cpuCore );
			Patch[patchnumber].addr			= StrToU32(pieces.MemAddr(), 16);
			Patch[patchnumber].type			= (patch_data_type)PatchTableExecute( pieces.OperandSize(), dataType );
			Patch[patchnumber].data			= StrToU64( pieces.WriteValue(), 16 );

			if (Patch[patchnumber].cpu == 0)
				throw wxsFormat( L"Unrecognized CPU Target: '%s'", pieces.CpuType().c_str() );

			if (Patch[patchnumber].type == 0)
				throw wxsFormat( L"Unrecognized Operand Size: '%s'", pieces.OperandSize().c_str() );

			Patch[patchnumber].enabled = 1;		// omg success!!

			//PrintPatch(patchnumber);
			patchnumber++;
		}
		catch( wxString& exmsg )
		{
			Console.Error( L"(Patch) Error Parsing: %s=%s", cmd.c_str(), param.c_str() );
			Console.Error( L"\t" + exmsg );
		}
    }

    void roundmode( const wxString& cmd, const wxString& param )
    {
        DevCon.WriteLn(cmd + L" " + param);

        int index;
        wxString pText;

        SSE_RoundMode eetype = EmuConfig.Cpu.sseMXCSR.GetRoundMode();
        SSE_RoundMode vutype = EmuConfig.Cpu.sseVUMXCSR.GetRoundMode();

        index = 0;
        pText = param.Lower().BeforeFirst(L',');
        while(pText != wxEmptyString)
        {
            SSE_RoundMode type;

            if (pText.Contains(L"near"))
                type = SSEround_Nearest;
            else if (pText.Contains(L"down"))
                type = SSEround_NegInf;
            else if (pText.Contains(L"up"))
                type = SSEround_PosInf;
            else if (pText.Contains(L"chop"))
                type = SSEround_Chop;
            else
            {
                Console.WriteLn(L"bad argument (" + pText + L") to round mode! skipping...\n");
                break;
            }

            if( index == 0 )
                eetype = type;
            else
                vutype = type;

            if( index == 1 )
                break;

            index++;
            pText = param.AfterFirst(L',');
        }

        SetRoundMode(eetype,vutype);
    }

    void zerogs(const wxString& cmd, const wxString& param)
    {
        DevCon.WriteLn( cmd + L" " + param);
        g_ZeroGSOptions = StrToU32(param, 16);
    }
}

void PrintRoundMode(SSE_RoundMode ee, SSE_RoundMode vu)
{
    switch(ee)
    {
        case SSEround_Nearest: DevCon.WriteLn("EE: Near"); break;
        case SSEround_NegInf: DevCon.WriteLn("EE: Down"); break;
        case SSEround_PosInf: DevCon.WriteLn("EE: Up"); break;
        case SSEround_Chop: DevCon.WriteLn("EE: Chop"); break;
        default: DevCon.WriteLn("EE: ?"); break;
    }

    switch(vu)
    {
        case SSEround_Nearest: DevCon.WriteLn("VU: Near"); break;
        case SSEround_NegInf: DevCon.WriteLn("VU: Down"); break;
        case SSEround_PosInf: DevCon.WriteLn("VU: Up"); break;
        case SSEround_Chop: DevCon.WriteLn("VU: Chop"); break;
        default: DevCon.WriteLn("VU: ?"); break;
    }
}

void SetRoundMode(SSE_RoundMode ee, SSE_RoundMode vu)
{
	SSE_MXCSR mxfpu	= EmuConfig.Cpu.sseMXCSR;
	SSE_MXCSR mxvu	= EmuConfig.Cpu.sseVUMXCSR;

    //PrintRoundMode(ee,vu);
	SetCPUState( mxfpu.SetRoundMode( ee ), mxvu.SetRoundMode( vu ) );
}
