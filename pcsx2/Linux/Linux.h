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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include <sys/time.h>
#include <pthread.h>

#include <X11/keysym.h>
#include <gtk/gtk.h>
#include <glib/gthread.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkdialog.h>
#include <glib.h>

#include "PrecompiledHeader.h"
#include "Paths.h"
#include "Common.h"

#include "Counters.h"
#include "VUmicro.h"
#include "Plugins.h"
#include "x86/ix86/ix86.h"
#include "x86/iR5900.h"

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

/* Misc.c */
extern void vu0Shutdown();
extern void vu1Shutdown();
extern void SaveConfig();

extern bool UseGui;

extern int efile;
extern int g_SaveGSStream;
extern int g_ZeroGSOptions;
extern void FixCPUState(void);

extern void InitLanguages();
extern char *GetLanguageNext();
extern void CloseLanguages();
extern void ChangeLanguage(char *lang);
extern void StartGui();
extern void RunGui();
extern int Pcsx2Configure();
extern GtkWidget *CpuDlg;

extern int LoadConfig();
extern void SaveConfig();

extern void init_widgets();
extern MemoryAlloc<u8>* g_RecoveryState;
extern void SysRestorableReset();
extern void SysDetect();
void SignalExit(int sig);
extern void RunExecute(const char* elf_file, bool use_bios = false);
extern void ExecuteCpu();

extern bool g_ReturnToGui;			// set to exit the execution of the emulator and return control to the GUI
extern bool g_EmulationInProgress;	// Set TRUE if a game is actively running (set to false on reset)

typedef struct
{
	char lang[g_MaxPath];
} _langs;

typedef enum
{
	GS,
	PAD1,
	PAD2,
	SPU,
	CDVD,
	DEV9,
	USB,
	FW,
	BIOS
} plugin_types;

extern GtkWidget *MainWindow;
extern bool applychanges;
extern bool configuringplug;

GtkWidget *check_eerec, *check_vu0rec, *check_vu1rec;
GtkWidget *check_mtgs , *check_cpu_dc;
GtkWidget *check_console , *check_patches;
GtkWidget *radio_normal_limit, *radio_limit_limit, *radio_fs_limit, *radio_vuskip_limit;

_langs *langs;
unsigned int langsMax;

char cfgfile[g_MaxPath];

/* Hacks */

int Config_hacks_backup;

#define is_checked(main_widget, widget_name) (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name))))
#define set_checked(main_widget,widget_name, state) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)), state)

#define set_flag(v, flag, value) if (value == TRUE) v |= flag; else v &= flag;
#define get_flag(v,flag) ((v & (1 << flag)) != 0)

/*static __forceinline void print_flags(char *name, u32 num, char *flag_names[16])
{
	int i;

	DevCon::WriteLn("%s:", name);

	if (flag_names != NULL)
	{
		for(i=0; i<=15; i++)
			DevCon::WriteLn("%s %x: %x", params flag_names[i], (1<<i), get_flag(num, i));
	}
	else
	{
		for(i=0; i<=15; i++)
			DevCon::WriteLn("%x: %x", params (1<<i), get_flag(num, i));
	}
} */

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

#define FLAG_VU_ADD_SUB 0x1
#define FLAG_VU_CLIP 0x2
#define FLAG_FPU_CLAMP 0x4
#define FLAG_VU_BRANCH 0x8
#define FLAG_AVOID_DELAY_HANDLING 0x10

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

#endif /* __LINUX_H__ */
