/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <float.h>

#include "Common.h"
#include "R5900OpcodeTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "iVUzerorec.h"

#ifdef _DEBUG
extern u32 vudump;
#endif

namespace VU1micro
{
	void recAlloc()
	{
		SuperVUAlloc(1);
	}

	void __fastcall recClear( u32 Addr, u32 Size )
	{
		assert( (Addr&7) == 0 );
		SuperVUClear(Addr, Size, 1); // Size should be a multiple of 8 bytes!
	}

	void recShutdown()
	{
		SuperVUDestroy( 1 );
	}

	// commented out because I'm not sure it actually works anymore with SuperVU (air)
	/*static void iVU1DumpBlock()
	{
		FILE *f;
		char filename[ g_MaxPath ];
		u32 *mem;
		u32 i;

	#ifdef _WIN32
		CreateDirectory("dumps", NULL);
		sprintf_s( filename, g_MaxPath, "dumps\\vu%.4X.txt", VU1.VI[ REG_TPC ].UL );
	#else
		mkdir("dumps", 0755);
		sprintf( filename, "dumps/vu%.4X.txt", VU1.VI[ REG_TPC ].UL );
	#endif
		SysPrintf( "dump1 %x => %x (%s)\n", VU1.VI[ REG_TPC ].UL, pc, filename );

		f = fopen( filename, "wb" );
		for ( i = VU1.VI[REG_TPC].UL; i < pc; i += 8 ) {
			char* pstr;
			mem = (u32*)&VU1.Micro[i];

			pstr = disVU1MicroUF( mem[1], i+4 );
			fprintf(f, "%x: %-40s ",  i, pstr);
			
			pstr = disVU1MicroLF( mem[0], i );
			fprintf(f, "%s\n",  pstr);
		}
		fclose( f );
	}*/

	static void recReset()
	{
		SuperVUReset(1);

		// these shouldn't be needed, but shouldn't hurt anything either.
		x86FpuState = FPU_STATE;
		iCWstate = 0;
	}

	static void recStep()
	{
	}

	static void recExecuteBlock(void)
	{
	#ifdef _DEBUG
		static u32 vuprogcount = 0;
		vuprogcount++;
		if( vudump & 8 ) __Log("start vu1: %x %x\n", VU1.VI[ REG_TPC ].UL, vuprogcount);
	#endif

		if((VU0.VI[REG_VPU_STAT].UL & 0x100) == 0){
			//SysPrintf("Execute block VU1, VU1 not busy\n");
			return;
		}
		
		if (VU1.VI[REG_TPC].UL >= VU1.maxmicro)
		{
			Console::Error("VU1 memory overflow!!: %x", params  VU1.VI[REG_TPC].UL);
			/*VU0.VI[REG_VPU_STAT].UL&= ~0x100;
			VU1.cycle++;
			return;*/
		}

		assert( (VU1.VI[ REG_TPC ].UL&7) == 0 );

		FreezeXMMRegs(1);
		do { // while loop needed since not always will return finished
			SuperVUExecuteProgram(VU1.VI[ REG_TPC ].UL & 0x3fff, 1);
		} while( VU0.VI[ REG_VPU_STAT ].UL&0x100 );
		FreezeXMMRegs(0);
	}
}

using namespace VU1micro;

const VUmicroCpu recVU1 = 
{
	recReset
,	recStep
,	recExecuteBlock
,	recClear
};
