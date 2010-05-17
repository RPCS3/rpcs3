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

typedef void PATCHTABLEFUNC( const wxString& text1, const wxString& text2 );

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
	PATCHTABLEFUNC comment;
    PATCHTABLEFUNC gametitle;
    PATCHTABLEFUNC patch;
    PATCHTABLEFUNC roundmode;
}

int  InitCheats(const wxString& name);
void inifile_command(const wxString& cmd);
void inifile_trim(wxString& buffer);

int  InitPatches(const wxString& name);
int  AddPatch(int Mode, int Place, int Address, int Size, u64 data);
void ApplyPatch( int place = 1);
void ResetPatch( void );

void SetRoundMode(SSE_RoundMode ee, SSE_RoundMode vu);

#endif /* __PATCH_H__ */

