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

#ifndef __DEBUGDLG_H__
#define __DEBUGDLG_H__

#include "Linux.h"
#ifdef __cplusplus
extern "C"
{
#endif

#include "support.h"
#include "callbacks.h"
#include "interface.h"

#ifdef __cplusplus
}
#endif
#include "R3000A.h"
#include "IopMem.h"


GtkWidget *ListDV;
GtkListStore *ListDVModel;
GtkWidget *SetPCDlg, *SetPCEntry;
GtkWidget *SetBPADlg, *SetBPAEntry;
GtkWidget *SetBPCDlg, *SetBPCEntry;
GtkWidget *DumpCDlg, *DumpCTEntry, *DumpCFEntry;
GtkWidget *DumpRDlg, *DumpRTEntry, *DumpRFEntry;
GtkWidget *MemWriteDlg, *MemEntry, *DataEntry;
GtkAdjustment *DebugAdj;

extern int efile;
extern char elfname[g_MaxPath];

int DebugMode; // 0 - EE | 1 - IOP
static u32 dPC, dBPA = -1, dBPC = -1;
static char nullAddr[g_MaxPath];

GtkWidget *DebugWnd;

#endif // __DEBUGDLG_H__
