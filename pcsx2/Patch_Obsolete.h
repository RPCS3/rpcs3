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

// Old Patching functions that are currently not used
// so out-sourced them to this header file that isn't referenced...

// Obsolete code used by old cheats-gui code...
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

void ResetPatch( void )
{
	patchnumber = 0;
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
