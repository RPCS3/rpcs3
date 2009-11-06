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

IniPatch Patch[ MAX_PATCH ];

u32 SkipCount = 0, IterationCount = 0;
u32 IterationIncrement = 0, ValueIncrement = 0;
u32 PrevCheatType = 0, PrevCheatAddr = 0,LastType = 0;

int g_ZeroGSOptions = 0, patchnumber;

wxString strgametitle;

struct PatchTextTable
{
	const char *text;
	int code;
	PATCHTABLEFUNC func;
};

static const PatchTextTable commands[] =
{
	{ "comment", 1, PatchFunc::comment },
	{ "gametitle", 2, PatchFunc::gametitle },
	{ "patch", 3, PatchFunc::patch },
	{ "fastmemory", 4, NULL }, // enable for faster but bugger mem (mvc2 is faster)
	{ "roundmode", 5, PatchFunc::roundmode }, // changes rounding mode for floating point
											// syntax: roundmode=X,Y
											// possible values for X,Y: NEAR, DOWN, UP, CHOP
											// X - EE rounding mode (default is NEAR)
											// Y - VU rounding mode (default is CHOP)
	{ "zerogs", 6, PatchFunc::zerogs }, // zerogs=hex
	{ "vunanmode",8, NULL },
	{ "ffxhack",9, NULL},
	{ "xkickdelay",10, NULL},
	{ "", 0, NULL }
};

static const PatchTextTable dataType[] =
{
	{ "byte", 1, NULL },
	{ "short", 2, NULL },
	{ "word", 3, NULL },
	{ "double", 4, NULL },
	{ "extended", 5, NULL },
	{ "", 0, NULL }
};

static const PatchTextTable cpuCore[] =
{
	{ "EE", 1, NULL },
	{ "IOP", 2, NULL },
	{ "", 0, NULL }
};

static int PatchTableExecute( char* text1, char* text2, const PatchTextTable * Table )
{
	int i = 0;

	while (Table[i].text[0])
	{
		if (!strcmp(Table[i].text, text1))
		{
			if (Table[i].func) Table[i].func(text1, text2);
			break;
		}
		i++;
	}

	return Table[i].code;
}

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
		case 0x3040:
		{                                                          // vvvvvvvv  00000000  Inc
			u32 mem = memRead32(PrevCheatAddr);
			memWrite32(PrevCheatAddr, mem + (p->addr));
			PrevCheatType = 0;
			break;
		}

		case 0x3050:
		{                                                         // vvvvvvvv  00000000  Dec
			u32 mem = memRead32(PrevCheatAddr);
			memWrite32(PrevCheatAddr, mem - (p->addr));
			PrevCheatType = 0;
			break;
		}

		case 0x4000:							  // vvvvvvvv  iiiiiiii
			for(u32 i = 0; i < IterationCount; i++)
			{
				memWrite32((u32)(PrevCheatAddr + (i * IterationIncrement)),(u32)(p->addr + ((u32)p->data * i)));
			}
			PrevCheatType = 0;
			break;

		case 0x5000:							  // dddddddd  iiiiiiii
			for(u32 i = 0; i < IterationCount; i++)
			{
				u8 mem = memRead8(PrevCheatAddr + i);
				memWrite8(((u32)p->data) + i, mem);
			}
			PrevCheatType = 0;
			break;

		case 0x6000:							  // 000Xnnnn  iiiiiiii
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

		case 0x6001:							// 000Xnnnn  iiiiiiii
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
		    // We might be able to substitute:
		    // if (((u32)p->data & 0xC0000000) == 0x00000000)
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

// This routine is for executing the commands of the ini file.
void inifile_command( char* cmd )
{
	int code;
	char command[ 256 ];
	char parameter[ 256 ];

	// extract param part (after '=')
	char* pEqual = strchr( cmd, '=' );

	if ( ! pEqual ) pEqual = cmd+strlen(cmd); // fastmemory doesn't have =

	memzero( command );
	memzero( parameter );

	strncpy( command, cmd, pEqual - cmd );
	strncpy( parameter, pEqual + 1, sizeof( parameter ) );

	inifile_trim( command );
	inifile_trim( parameter );

	code = PatchTableExecute( command, parameter, commands );
}

#define  USE_CRAZY_BASHIT_INSANE_TRIM
#ifdef USE_CRAZY_BASHIT_INSANE_TRIM

void inifile_trim( char* buffer )
{
	char* pInit = buffer;
	char* pEnd = NULL;

	while ((*pInit == ' ') || (*pInit == '\t')) //skip space
	{
		pInit++;
	}

	if ((pInit[0] == '/') && (pInit[1] == '/')) //remove comment
	{
		buffer[0] = '\0';
		return;
	}

	pEnd = pInit + strlen(pInit) - 1;
	if ( pEnd <= pInit )
	{
		buffer[0] = '\0';
		return;
	}

	while ((*pEnd == '\r') || (*pEnd == '\n') || (*pEnd == ' ') || (*pEnd == '\t'))
	{
		pEnd--;
	}

	if (pEnd <= pInit)
	{
		buffer[0] = '\0';
		return;
	}

	memmove( buffer, pInit, pEnd - pInit + 1 );
	buffer[ pEnd - pInit + 1 ] = '\0';
}

#else

// New version of trim (untested), which I coded but can't use yet because the
// rest of Patch needs to be more wxString-involved first.

void inifile_trim( wxString& buffer )
{
	buffer.Trim( false );		// trims left side.

	if( buffer.Length() <= 1 )	// this I'm not sure about... - air
	{
		buffer.Clear();
		return;
	}

	if( buffer.Left( 2 ) == "//" )
	{
		buffer.Clear();
		return;
	}

	buffer.Trim(true);			// trims right side.
}

#endif

void inisection_process( FILE * f1 )
{
	char buffer[ 1024 ];
	while( fgets( buffer, sizeof( buffer ), f1 ) )
	{
		inifile_trim( buffer );
		if ( buffer[ 0 ] ) inifile_command( buffer );
	}
}

//this routine is for reading the ini file

void inifile_read( const char* name )
{
	FILE* f1;
	char buffer[ 1024 ];

	patchnumber = 0;

#ifdef _WIN32
	sprintf( buffer, "patches\\%s.pnach", name );
#else
	sprintf( buffer, "patches/%s.pnach", name );
#endif

	f1 = fopen( buffer, "rt" );

#ifndef _WIN32
	if( !f1 )
	{
		 // try all upper case because linux is case sensitive
		 char* pstart = buffer+8;
		 char* pend = buffer+strlen(buffer);
		 while(pstart != pend )
		{
			// stop at the first . since we only want to update the hex
			if( *pstart == '.' ) break;
			*pstart = toupper(*pstart);
			*pstart++;
		 }

		 f1 = fopen(buffer, "rt");
	}
#endif

	if( !f1 )
	{
		Console.WriteLn("No patch found. Resuming execution without a patch (this is NOT an error)." );
		return;
	}

	inisection_process( f1 );
	fclose( f1 );
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
	int i;

	if (place == 0) Console.WriteLn(" patchnumber: %d", patchnumber);

	for (i = 0; i < patchnumber; i++)
	{
	    if (Patch[i].placetopatch == place)
            _ApplyPatch(&Patch[i]);
	}
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

// PatchFunc Functions.
namespace PatchFunc
{
    void comment( char* text1, char* text2 )
    {
        Console.WriteLn( "comment: %s", text2 );
    }

    void gametitle( char* text1, char* text2 )
    {
        Console.WriteLn( "gametitle: %s", text2 );
        strgametitle.FromAscii( text2 );
        Console.SetTitle( strgametitle );
    }

    void patch( char* cmd, char* param )
    {
        char* pText;

        if ( patchnumber >= MAX_PATCH )
        {
            // TODO : Use wxLogError for this, once we have full unicode compliance on cmd/params vars.
            //wxLogError( L"Patch ERROR: Maximum number of patches reached: %s=%s", cmd, param );
            Console.Error( "Patch ERROR: Maximum number of patches reached: %s=%s", cmd, param );
            return;
        }

        //SafeList<wxString> pieces;
        //SplitString( pieces, param, "," );

        pText = strtok( param, "," );
        pText = param;

        Patch[ patchnumber ].placetopatch = strtol( pText, (char **)NULL, 0 );

        pText = strtok( NULL, "," );
        inifile_trim( pText );
        Patch[ patchnumber ].cpu = (patch_cpu_type)PatchTableExecute( pText, NULL, cpuCore );

        if ( Patch[ patchnumber ].cpu == 0 )
        {
            Console.Error( "Unrecognized patch '%s'", pText );
            return;
        }

        pText = strtok( NULL, "," );
        inifile_trim( pText );
        sscanf( pText, "%X", &Patch[ patchnumber ].addr );

        pText = strtok( NULL, "," );
        inifile_trim( pText );
        Patch[ patchnumber ].type = (patch_data_type)PatchTableExecute( pText, NULL, dataType );

        if ( Patch[ patchnumber ].type == 0 )
        {
            Console.Error( "Unrecognized patch '%s'", pText );
            return;
        }

        pText = strtok( NULL, "," );
        inifile_trim( pText );
        sscanf( pText, "%I64X", &Patch[ patchnumber ].data );

        Patch[ patchnumber ].enabled = 1;

        patchnumber++;
    }

    void roundmode( char* cmd, char* param )
    {
        int index;
        char* pText;

        SSE_RoundMode eetype = EmuConfig.Cpu.sseMXCSR.GetRoundMode();
        SSE_RoundMode vutype = EmuConfig.Cpu.sseVUMXCSR.GetRoundMode();

        index = 0;
        pText = strtok( param, ", " );
        while(pText != NULL)
        {
            SSE_RoundMode type;

            if( stricmp(pText, "near") == 0 )
                type = SSEround_Nearest;
            else if( stricmp(pText, "down") == 0 )
                type = SSEround_NegInf;
            else if( stricmp(pText, "up") == 0 )
                type = SSEround_PosInf;
            else if( stricmp(pText, "chop") == 0 )
                type = SSEround_Chop;
            else
            {
                Console.WriteLn("bad argument (%s) to round mode! skipping...\n", pText);
                break;
            }

            if( index == 0 )
                eetype = type;
            else
                vutype = type;

            if( index == 1 )
                break;

            index++;
            pText = strtok(NULL, ", ");
        }

        SetRoundMode(eetype,vutype);
    }

    void zerogs(char* cmd, char* param)
    {
         sscanf(param, "%x", &g_ZeroGSOptions);
    }
}

void SetRoundMode(SSE_RoundMode ee, SSE_RoundMode vu)
{
	SSE_MXCSR mxfpu	= EmuConfig.Cpu.sseMXCSR;
	SSE_MXCSR mxvu	= EmuConfig.Cpu.sseVUMXCSR;

	SetCPUState( mxfpu.SetRoundMode( ee ), mxvu.SetRoundMode( vu ) );
}
