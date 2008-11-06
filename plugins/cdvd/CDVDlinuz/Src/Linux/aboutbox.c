/*  aboutbox.c
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
#include <stdio.h> // sprintf()

#include <gtk/gtkbutton.h> // gtk_button_new_with_label()
#include <gtk/gtkcontainer.h> // gtk_container_add()
#include <gtk/gtkhbbox.h> // gtk_hbutton_box_new()
#include <gtk/gtklabel.h> // gtk_label_new()
#include <gtk/gtkmain.h> // gtk_init(), gtk_main(), gtk_main_quit()
#include <gtk/gtkvbox.h> // gtk_vbox_new()
#include <gtk/gtkwindow.h> // gtk_window_new()

#include "version.h"
#include "aboutbox.h"


struct AboutBoxData aboutbox;


gint AboutBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  if(aboutbox.window != NULL) {
    gtk_widget_destroy(aboutbox.window);
    aboutbox.window = NULL;
  } // ENDIF- Do we have an About Box still?

  gtk_main_quit();
  return(TRUE);
} // END AboutBoxCancelEvent()


void AboutBoxDisplay() {
  GtkWidget *item;
  GtkWidget *container;
  GtkWidget *vbox1;
  char templine[256];

  aboutbox.window = NULL;
  aboutbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(aboutbox.window), 5);
  gtk_window_set_title(GTK_WINDOW(aboutbox.window), "About CDVDlinuz");
  gtk_window_set_position(GTK_WINDOW(aboutbox.window), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(aboutbox.window), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(aboutbox.window), FALSE);

  g_signal_connect(G_OBJECT(aboutbox.window), "delete_event",
                   G_CALLBACK(AboutBoxCancelEvent), NULL);

  vbox1 = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(aboutbox.window), vbox1);
  gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
  gtk_widget_show(vbox1);

  sprintf(templine, "%s v%i.%i", libname, revision, build);
  item = gtk_label_new(templine);
  gtk_box_pack_start(GTK_BOX(vbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  item = gtk_label_new("Current Author: efp");
  gtk_box_pack_start(GTK_BOX(vbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  item = gtk_label_new("Original code by: linuzappz & shadow");
  gtk_box_pack_start(GTK_BOX(vbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  container = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox1), container, TRUE, TRUE, 0);
  gtk_widget_show(container);

  item = gtk_button_new_with_label("Ok");
  gtk_container_add(GTK_CONTAINER(container), item);
  GTK_WIDGET_SET_FLAGS(item, GTK_CAN_DEFAULT);
  gtk_widget_show(item);

  g_signal_connect(G_OBJECT(item), "clicked",
                   G_CALLBACK(AboutBoxCancelEvent), NULL);
  item = NULL;
  container = NULL;
  vbox1 = NULL;

  gtk_widget_show(aboutbox.window);
  gtk_main();
} // END AboutDisplay()
