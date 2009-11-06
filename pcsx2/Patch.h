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
#ifndef __PATCH_H__
#define __PATCH_H__

#include "Pcsx2Defs.h"

#define MAX_PATCH 1024

enum patch_cpu_type {
	NO_CPU,
	CPU_EE,
	CPU_IOP
};

enum patch_data_type {
	NO_TYPE,
	BYTE_T,
	SHORT_T,
	WORD_T,
	DOUBLE_T,
	EXTENDED_T
};

typedef void (*PATCHTABLEFUNC)( char * text1, char * text2 );

struct IniPatch
{
	int enabled;
	int group;
	patch_data_type type;
	patch_cpu_type cpu;
	int placetopatch;
	u32 addr;
	u64 data;
};


namespace PatchFunc
{
    void comment( char* text1, char* text2 );
    void gametitle( char* text1, char* text2 );
    void patch( char* text1, char* text2 );
    void roundmode( char* text1, char* text2 );
    void zerogs( char* text1, char* text2 );
}

void inifile_read( const char* name );
void inifile_command( char* cmd );
void inifile_trim( char* buffer );

int AddPatch(int Mode, int Place, int Address, int Size, u64 data);
void ApplyPatch( int place = 1);
void ResetPatch( void );

extern int g_ZeroGSOptions;

void SetRoundMode(SSE_RoundMode ee, SSE_RoundMode vu);

#endif /* __PATCH_H__ */

