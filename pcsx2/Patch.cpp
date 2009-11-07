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
	wxString text;
	int code;
	PATCHTABLEFUNC func;
};

static const PatchTextTable commands[] =
{
	{ L"comment", 1, PatchFunc::comment },
	{ L"gametitle", 2, PatchFunc::gametitle },
	{ L"patch", 3, PatchFunc::patch },
	{ L"fastmemory", 4, NULL }, // enable for faster but bugger mem (mvc2 is faster)
	{ L"roundmode", 5, PatchFunc::roundmode }, // changes rounding mode for floating point
											// syntax: roundmode=X,Y
											// possible values for X,Y: NEAR, DOWN, UP, CHOP
											// X - EE rounding mode (default is NEAR)
											// Y - VU rounding mode (default is CHOP)
	{ L"zerogs", 6, PatchFunc::zerogs }, // zerogs=hex
	{ L"vunanmode",8, NULL },
	{ L"ffxhack",9, NULL},
	{ L"xkickdelay",10, NULL},
	{ wxEmptyString, 0, NULL }
};

static const PatchTextTable dataType[] =
{
	{ L"byte", 1, NULL },
	{ L"short", 2, NULL },
	{ L"word", 3, NULL },
	{ L"double", 4, NULL },
	{ L"extended", 5, NULL },
	{ wxEmptyString, 0, NULL }
};

static const PatchTextTable cpuCore[] =
{
	{ L"EE", 1, NULL },
	{ L"IOP", 2, NULL },
	{ wxEmptyString, 0, NULL }
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

// New version of trim (untested), which I coded but can't use yet because the
// rest of Patch needs to be more wxString-involved first. --Air

// And now it is... --Arcum42
void inifile_trim( wxString& buffer )
{
	buffer.Trim( false );		// trims left side.

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

static int PatchTableExecute( wxString text1, wxString text2, const PatchTextTable * Table )
{
	int i = 0;

	while (Table[i].text[0])
	{
		if (!text1.Cmp(Table[i].text))
		{
			if (Table[i].func) Table[i].func(text1, text2);
			break;
		}
		i++;
	}

	return Table[i].code;
}

// This routine is for executing the commands of the ini file.
void inifile_command( wxString cmd )
{
	int code;

	wxString command;
	wxString parameter;

	// extract param part (after '=')
    wxString pEqual = cmd.AfterFirst(L'=');
    if (pEqual.IsEmpty()) pEqual = cmd;

	command = cmd.BeforeFirst(L'=');
	parameter = pEqual;

	inifile_trim( command );
	inifile_trim( parameter );

    code = PatchTableExecute( command, parameter, commands );
}

// This routine recieves a file from inifile_read, trims it,
// Then sends the command to be parsed.
void inifile_process(wxTextFile &f1 )
{
    Console.WriteLn("inifile_process");
    for (int i = 0; i < f1.GetLineCount(); i++)
    {
        inifile_trim(f1[i]);
        if (!f1[i].IsEmpty()) inifile_command(f1[i]);
    }
}

// This routine creates a pnach filename from the games crc,
// loads it, trims the commands, and sends them to be parsed.
void inifile_read(wxString name )
{
	wxTextFile f1;
	wxString buffer;

	patchnumber = 0;

	buffer = Path::Combine(L"patches", name + L".pnach");

	if(!f1.Open(buffer) && wxFileName::IsCaseSensitive())
	{
		name.MakeUpper();
		f1.Open( Path::Combine(L"patches", name + L".pnach") );
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

void InitPatch(wxString crc)
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
        case CPU_EE: Console.WriteLn("Cpu: EE"); break;
        case CPU_IOP: Console.WriteLn("Cpu: IOP"); break;
        default: Console.WriteLn("Cpu: None"); break;
    }

    Console.WriteLn("Address: %X", Patch[i].addr);

    switch (Patch[i].type)
    {
        case BYTE_T: Console.WriteLn("Type: Byte"); break;
        case SHORT_T: Console.WriteLn("Type: Short"); break;
        case WORD_T: Console.WriteLn("Type: Word"); break;
        case DOUBLE_T: Console.WriteLn("Type: Double"); break;
        case EXTENDED_T: Console.WriteLn("Type: Extended"); break;
        default: Console.WriteLn("Type: None"); break;
    }

    Console.WriteLn("Data: %I64X", Patch[i].data);
}

u32 StrToU32(wxString str, int base = 10)
{
    long l;

    str.ToLong(&l, base);

    return l;
}

u64 StrToU64(wxString str, int base = 10)
{
    long long l;

    str.ToLongLong(&l, base);

    return l;
}

// PatchFunc Functions.
namespace PatchFunc
{
    void comment( wxString text1, wxString text2 )
    {
        Console.WriteLn( L"comment: " + text2 );
    }

    void gametitle( wxString text1, wxString text2 )
    {
        Console.WriteLn( L"gametitle: " + text2 );
        strgametitle = text2;
        Console.SetTitle( strgametitle );
    }

    void patch( wxString cmd, wxString param )
    {
        DevCon.WriteLn(cmd + L" " + param);
        wxString pText;

        if ( patchnumber >= MAX_PATCH )
        {
            // TODO : Use wxLogError for this, once we have full unicode compliance on cmd/params vars.
            //wxLogError( L"Patch ERROR: Maximum number of patches reached: %s=%s", cmd, param );
            Console.Error( L"Patch ERROR: Maximum number of patches reached: " + cmd +L"=" + param );
            return;
        }

        // I've just sort of hacked this in place for the moment.
        // Using SafeList is probably better, but I'll leave that to Air...
        //SafeList<wxString> pieces;
        //SplitString( pieces, param, L"," );

        Patch[patchnumber].placetopatch = StrToU32(param.BeforeFirst(L','), 10);
        param = param.AfterFirst(L',');
        pText = param.BeforeFirst(L',');

        inifile_trim( pText );
        Patch[patchnumber].cpu = (patch_cpu_type)PatchTableExecute( pText, wxEmptyString, cpuCore );

        if (Patch[patchnumber].cpu == 0)
        {
            Console.Error( L"Unrecognized patch '" + cmd + L"'" );
            return;
        }

        param = param.AfterFirst(L',');
        pText = param.BeforeFirst(L',');
        inifile_trim( pText );
        Patch[patchnumber].addr = StrToU32(pText, 16);

        param = param.AfterFirst(L',');
        pText = param.BeforeFirst(L',');
        inifile_trim( pText );
        Patch[patchnumber].type = (patch_data_type)PatchTableExecute( pText, wxEmptyString, dataType );

        if ( Patch[patchnumber].type == 0 )
        {
            Console.Error( L"Unrecognized patch '" + cmd + L"'" );
            return;
        }

        param = param.AfterFirst(L',');
        pText = param.BeforeFirst(L',');
        inifile_trim( pText );

        Patch[patchnumber].data = StrToU64(pText, 16);
        Patch[patchnumber].enabled = 1;

        //PrintPatch(patchnumber);
        patchnumber++;
    }

    void roundmode( wxString cmd, wxString param )
    {
        DevCon.WriteLn(cmd + L" " + param);

        int index;
        wxString pText;

        SSE_RoundMode eetype = EmuConfig.Cpu.sseMXCSR.GetRoundMode();
        SSE_RoundMode vutype = EmuConfig.Cpu.sseVUMXCSR.GetRoundMode();

        index = 0;
        param.MakeLower();
        pText = param.BeforeFirst(L',');
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

    void zerogs(wxString cmd, wxString param)
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
