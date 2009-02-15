/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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
#ifndef __PATCH_H__
#define __PATCH_H__

#ifdef _WIN32
#include<windows.h>
#endif

#include "PS2Etypes.h"

//
// Defines
//
#define MAX_PATCH 1024 

#define IFIS(x,str) if(!strnicmp(x,str,sizeof(str)-1))

#define GETNEXT_PARAM() \
	while ( *param && ( *param != ',' ) ) param++; \
	if ( *param ) param++; \
	while ( *param && ( *param == ' ' ) ) param++; \
	if ( *param == 0 ) { Console::Error( _( "Not enough params for inicommand" ) ); return; }
	
//
// Enums
//

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

//
// Typedefs
//
typedef void (*PATCHTABLEFUNC)( char * text1, char * text2 );

struct PatchTextTable
{
	const char *text;
	int code;
	PATCHTABLEFUNC func;
};
	
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

//
// Function prototypes
//
void patchFunc_comment( char * text1, char * text2 );
void patchFunc_gametitle( char * text1, char * text2 );
void patchFunc_patch( char * text1, char * text2 );
void patchFunc_fastmemory( char * text1, char * text2 );
void patchFunc_path3hack( char * text1, char * text2 );
void patchFunc_roundmode( char * text1, char * text2 );
void patchFunc_zerogs( char * text1, char * text2 );
void patchFunc_vunanmode( char * text1, char * text2 );
void patchFunc_ffxhack( char * text1, char * text2 );
void patchFunc_xkickdelay( char * text1, char * text2 );

void inifile_trim( char * buffer );

//
// Variables
//
extern PatchTextTable commands[];
extern PatchTextTable dataType[];
extern PatchTextTable cpuCore[];
extern IniPatch patch[ MAX_PATCH ];
extern int patchnumber;

#ifdef __LINUX__
// Nasty, currently neccessary hack	
extern u32 LinuxsseMXCSR;
extern u32 LinuxsseVUMXCSR;
#endif

void applypatch( int place );
void inifile_read( const char * name );
void inifile_command( char * cmd );
void resetpatch( void );

int AddPatch(int Mode, int Place, int Address, int Size, u64 data);

extern void SetFastMemory(int); // iR5900LoadStore.c

extern int path3hack;
extern int g_FFXHack;
//extern int g_VUGameFixes;
extern int g_ZeroGSOptions;
extern u32 g_sseMXCSR;
extern u32 g_sseVUMXCSR;

void SetRoundMode(u32 ee, u32 vu);
int LoadPatch(const std::string& patchfile);

#endif /* __PATCH_H__ */

