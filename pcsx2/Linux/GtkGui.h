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
 
 #ifndef __GTKGUI_H__
#define __GTKGUI_H__


#include "PrecompiledHeader.h"
#include "Linux.h"

#ifdef __cplusplus
extern "C" {
#endif
	
#include "support.h"
#include "callbacks.h"
#include "interface.h"
	
#ifdef __cplusplus
}
#endif

bool applychanges = FALSE;
bool configuringplug = FALSE;
bool UseGui = TRUE;

 MemoryAlloc<u8>* g_RecoveryState = NULL;
bool g_GameInProgress = false;	// Set TRUE if a game is actively running.

//static bool AccBreak = false;
static bool m_ReturnToGame = false;		// set to exit the RunGui message pump


int efile = 0;
char elfname[g_MaxPath];
int Slots[5] = { -1, -1, -1, -1, -1 };

GtkWidget *CpuDlg;

// Functions Callbacks
void OnFile_LoadElf(GtkMenuItem *menuitem, gpointer user_data);
void OnFile_Exit(GtkMenuItem *menuitem, gpointer user_data);
void OnEmu_Run(GtkMenuItem *menuitem, gpointer user_data);
void OnEmu_Reset(GtkMenuItem *menuitem, gpointer user_data);
void OnEmu_Arguments(GtkMenuItem *menuitem, gpointer user_data);
void OnLanguage(GtkMenuItem *menuitem, gpointer user_data);
void OnHelp_Help();
void OnHelp_About(GtkMenuItem *menuitem, gpointer user_data);
void ExecuteCpu();

void StartGui();
void pcsx2_exit();
GtkWidget *MainWindow;
GtkWidget *pStatusBar = NULL, *Status_Box;
GtkWidget *CmdLine;	//2002-09-28 (Florin)
GtkWidget *widgetCmdLine;
GtkWidget *LogDlg;
GtkWidget *FileSel;
GtkWidget *AboutDlg, *about_version , *about_authors, *about_greets;
	
void init_widgets();

GtkAccelGroup *AccelGroup;

GtkWidget *GameFixDlg, *SpeedHacksDlg, *AdvDlg;
	
#endif

