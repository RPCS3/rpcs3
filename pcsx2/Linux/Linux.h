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

#ifndef __LINUX_H__
#define __LINUX_H__

#include "PrecompiledHeader.h"
#include "Paths.h"
#include "Common.h"
#include "HostGui.h"

//For CpuDlg
#include "Counters.h"

// For AdvancedDialog
#include "x86/ix86/ix86.h"

#include <dirent.h>
#include <dlfcn.h>

#include <X11/keysym.h>
#include <gtk/gtk.h>
#include <glib/gthread.h>
#include <glib.h>

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

extern void SaveConfig();
extern int LoadConfig();
extern void SysRestorableReset();

extern int Pcsx2Configure();

extern GtkWidget *MainWindow;

char cfgfile[g_MaxPath];

#define is_checked(main_widget, widget_name) (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name))))
#define set_checked(main_widget,widget_name, state) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)), state)

#define set_flag(v, flag, value) if (value == TRUE) v |= flag; else v &= flag;
#define get_flag(v,flag) ((v & (1 << flag)) != 0)

char ee_log_names[17][32] =
{
	"Cpu Log",
	"Mem Log",
	"HW Log",
	"Dma Log",
	"Bios Log",
	"Elf Log",
	"Fpu Log",
	"MMI Log",
	"VU0 Log",
	"Cop0 Log",
	"Vif Log",
	"SPR Log",
	"GIF Log",
	"Sif Log",
	"IPU Log",
	"VU Micro Log",
	"RPC Log"
};

char iop_log_names[9][32] =
{
	"IOP Log",
	"Mem Log",
	"HW Log",
	"Bios Log",
	"Dma Log",
	"Pad Log",
	"Gte Log",
	"Cdr Log",
	"GPU Log"
};

char vu_stealing_labels[5][256] = 
{
	"0: No speedup.",
	"1: Slight speedup, should work with most games.",
	"2: Moderate speedup, should work with most games with minor problems.",
	"3: Large speedup, may break many games and make others skip frames.",
	"4: Very large speedup, will break games in interesting ways."
};

char ee_cycle_labels[3][256] = 
{
	"Default Cycle Rate: Most compatible option - recommended for everyone with high-end machines.",
	"x1.5 Cycle Rate: Moderate speedup, and works well with most games.",
	"x2 Cycle Rate: Big speedup! Works well with many games."
};
//Tri-Ace - IDC_GAMEFIX2
#define FLAG_VU_ADD_SUB 0x1
// Persona3/4  - IDC_GAMEFIX4
#define FLAG_VU_CLIP 0x2

// Digimon Rumble Arena - IDC_GAMEFIX3
#define FLAG_FPU_Compare 0x4
//Tales of Destiny - IDC_GAMEFIX5
#define FLAG_FPU_MUL 0x8
//Fatal Frame
#define FLAG_DMAExec 0x10

#define FLAG_VU_NO_OVERFLOW 0x2
#define FLAG_VU_EXTRA_OVERFLOW 0x40

#define FLAG_FPU_NO_OVERFLOW 0x800
#define FLAG_FPU_EXTRA_OVERFLOW 0x1000

#define FLAG_EE_2_SYNC 0x1
#define FLAG_IOP_2_SYNC 0x10
#define FLAG_TRIPLE_SYNC 0x20
#define FLAG_ESC 0x400

#define FLAG_ROUND_NEAR 0x0000
#define FLAG_ROUND_NEGATIVE 0x2000
#define FLAG_ROUND_POSITIVE 0x4000
#define FLAG_ROUND_ZERO 0x6000

#define FLAG_FLUSH_ZERO 0x8000
#define FLAG_DENORMAL_ZERO 0x0040

#define FLAG_VU_CLAMP_NONE 0x0
#define FLAG_VU_CLAMP_NORMAL 0x1
#define FLAG_VU_CLAMP_EXTRA 0x3
#define FLAG_VU_CLAMP_EXTRA_PRESERVE 0x7

#define FLAG_EE_CLAMP_NONE 0x0
#define FLAG_EE_CLAMP_NORMAL 0x1
#define FLAG_EE_CLAMP_EXTRA_PRESERVE 0x3
#define FLAG_EE_CLAMP_FULL 0x7

#endif /* __LINUX_H__ */
