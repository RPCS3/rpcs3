/*  messagebox.c
 *  Copyright (C) 2002-2005  PCSX2 Team
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
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#include <stddef.h> // NULL

#include <gtk/gtkbutton.h> // gtk_button_new_with_label()
#include <gtk/gtkhbbox.h> // gtk_hbutton_box_new()
#include <gtk/gtklabel.h> // gtk_label_new()
#include <gtk/gtkmain.h> // gtk_init(), gtk_main(), gtk_main_quit()
#include <gtk/gtkvbox.h> // gtk_vbox_new()
#include <gtk/gtkwindow.h> // gtk_window_new()

#include "mainbox.h"
#include "devicebox.h"
#include "conversionbox.h"
#include "messagebox.h"


struct MessageBoxData messagebox;


void MessageBoxDestroy() {
  if(messagebox.window != NULL) {
    gtk_widget_destroy(messagebox.window);
    messagebox.window = NULL;
    messagebox.desc = NULL;
  } // ENDIF- Do we have a Main Window still?
} // END MessageBoxDestroy()


void MessageBoxShow(char *message, int wherefrom) {
  gtk_label_set_text(GTK_LABEL(messagebox.desc), message);
  messagebox.wherefrom = wherefrom;

  gtk_widget_show_all(messagebox.window);
  gtk_window_deiconify(GTK_WINDOW(messagebox.window));
} // END MessageBox()


gint MessageBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  gtk_widget_hide(messagebox.window);

  switch(messagebox.wherefrom) {
    case 1:
      MainBoxRefocus();
      break;
    case 2:
      DeviceBoxRefocus();
      break;
    case 3:
      ConversionBoxRefocus();
      break;
  } // ENDSWITCH- Whose window do I get to re-focus when this is done?

  return(TRUE);
} // END MessageBoxCancelEvent()


void MessageBoxDisplay() {
  GtkWidget *item;
  GtkWidget *hbox1;
  GtkWidget *vbox1;

  messagebox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(messagebox.window), 5);
  gtk_window_set_title(GTK_WINDOW(messagebox.window), "CDVDisoEFP");
  gtk_window_set_position(GTK_WINDOW(messagebox.window), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable(GTK_WINDOW(messagebox.window), FALSE);

  g_signal_connect(G_OBJECT(messagebox.window), "delete_event",
                   G_CALLBACK(MessageBoxCancelEvent), NULL);

  vbox1 = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(messagebox.window), vbox1);
  gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
  gtk_widget_show(vbox1);

  messagebox.desc = gtk_label_new("Hi There!\r\nHow are you doing?");
  gtk_box_pack_start(GTK_BOX(vbox1), messagebox.desc, FALSE, FALSE, 0);
  gtk_widget_show(messagebox.desc);

  hbox1 = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hbox1), item, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS(item, GTK_CAN_DEFAULT);
  gtk_widget_show(item);
  g_signal_connect(G_OBJECT(item), "clicked",
                   G_CALLBACK(MessageBoxCancelEvent), NULL);
  item = NULL;
  hbox1 = NULL;
  vbox1 = NULL;
} // END MessageBoxDisplay()
