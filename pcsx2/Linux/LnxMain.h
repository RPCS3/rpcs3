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

#ifndef __LNXMAIN_H__
#define __LNXMAIN_H__

#include "Linux.h"
#include "LnxMain.h"
#include "HostGui.h"

// For Linux ISO loading
#include "CDVD/CDVDisoReader.h"

extern bool applychanges;
extern bool Slots[5];

extern bool ParseCommandLine(int argc, char *argv[], char **file);
extern void MemoryCard_Init();

void OnLanguage(GtkMenuItem *menuitem, gpointer user_data);
void InitLanguages();
char *GetLanguageNext();
void CloseLanguages();

void StartGui();
void pcsx2_exit();
extern bool SysInit();
extern void SysClose();

GtkWidget *MainWindow, *Status_Box;
GtkWidget *pStatusBar = NULL;
GtkWidget *CmdLine, *widgetCmdLine;
GtkWidget *LogDlg, *MemDlg;
GtkAccelGroup *AccelGroup;

typedef struct
{
	char lang[g_MaxPath];
} _langs;

_langs *langs;
unsigned int langsMax;

const char* phelpmsg =
    "\tpcsx2 [options] [file]\n\n"
    "-cfg [file] {configuration file}\n"
    "-efile [efile] {0 - reset, 1 - runcd (default), 2 - loadelf}\n"
    "-help {display this help file}\n"
    "-nogui {Don't use gui when launching}\n"
    "-loadgs [file} {Loads a gsstate}\n"
    "\n"
#ifdef PCSX2_DEVBUILD
    "Testing Options: \n"
    "\t-frame [frame] {game will run up to this frame before exiting}\n"
    "\t-image [name] {path and base name of image (do not include the .ext)}\n"
    "\t-jpg {save images to jpg format}\n"
    "\t-log [name] {log path to save log file in}\n"
    "\t-logopt [hex] {log options in hex (see debug.h) }\n"
    "\t-numimages [num] {after hitting frame, this many images will be captures every 20 frames}\n"
    "\t-test {Triggers testing mode (only for dev builds)}\n"
    "\n"
#endif
    "Load Plugins:\n"
    "\t-cdvd [libpath] {specify the library load path of the CDVD plugin}\n"
    "\t-gs [libpath] {specify the library load path of the GS plugin}\n"
    "-pad [tsxcal] {specify to hold down on the triangle, square, circle, x, start, select buttons}\n"
    "\t-spu [libpath] {specify the library load path of the SPU2 plugin}\n"
    "\n";

#endif
