/*  CDVDiso
 *  Copyright (C) 2002-2004  CDVDiso Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <zlib.h>
#include <bzlib.h>
#include <gtk/gtk.h>

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

#include "CDVDiso.h"

void SaveConf();
void LoadConf();
extern void SysMessageLoc(char *fmt, ...);

extern char *LibName;

extern const u8 revision;
extern const u8 build;

extern GtkWidget *AboutDlg, *ConfDlg, *MsgDlg, *FileSel;
extern GtkWidget *Edit, *CdEdit;
extern bool stop;

extern GtkWidget *Method,*Progress;
extern GtkWidget *BtnCompress, *BtnDecompress;
extern GtkWidget *BtnCreate, *BtnCreateZ;

extern GList *methodlist;

// Make it easier to check and set checkmarks in the gui
#define is_checked(main_widget, widget_name) (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)))) 
#define set_checked(main_widget,widget_name, state) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)), state)


extern void OnFileSel(GtkButton *button, gpointer user_data);
extern void OnStop(GtkButton *button, gpointer user_data);
extern void OnCompress(GtkButton *button, gpointer user_data);
extern void OnDecompress(GtkButton *button, gpointer user_data);
extern void OnCreate(GtkButton *button, gpointer user_data);
extern void OnCreateZ(GtkButton *button, gpointer user_data);
extern void OnOk(GtkButton *button, gpointer user_data);
extern void OnCancel(GtkButton *button, gpointer user_data);

