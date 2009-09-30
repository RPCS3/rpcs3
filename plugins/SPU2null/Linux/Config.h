/*  SPU2null
 *  Copyright (C) 2002-2005  SPU2null Team
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
#include <gtk/gtk.h>
#include <string>
using namespace std;

extern void SaveConf();
extern void LoadConf();
extern string s_strIniPath;
extern GtkWidget *MsgDlg;
extern void OnMsg_Ok();

#define is_checked(main_widget, widget_name) (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name))))
#define set_checked(main_widget,widget_name, state) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)), state)
