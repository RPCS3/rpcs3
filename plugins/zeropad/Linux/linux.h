/*  ZeroPAD - author: zerofrog(@gmail.com)
 *  Copyright (C) 2006-2007
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

#include "joystick.h"
#include "keyboard.h"
#include "zeropad.h"

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pthread.h>


extern "C"
{
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

extern GtkWidget *Conf, *s_devicecombo;
extern string s_strIniPath;


#define is_checked(main_widget, widget_name) (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name))))
#define set_checked(main_widget,widget_name, state) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)), state)
