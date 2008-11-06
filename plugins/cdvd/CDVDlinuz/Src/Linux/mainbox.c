/*  mainbox.c
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
#include <string.h> // strcpy()
#include <sys/stat.h> // stat()
#include <sys/types.h> // stat()
#include <unistd.h> // stat()

#include <gtk/gtkbutton.h> // gtk_button_new_with_label()
#include <gtk/gtkcontainer.h> // gtk_container_add()
#include <gtk/gtkentry.h> // gtk_entry_new()
#include <gtk/gtkhbbox.h> // gtk_hbutton_box_new()
#include <gtk/gtkhbox.h> // gtk_hbox_new()
#include <gtk/gtklabel.h> // gtk_label_new()
#include <gtk/gtkmain.h> // gtk_init(), gtk_main(), gtk_main_quit()
#include <gtk/gtkvbox.h> // gtk_vbox_new()
#include <gtk/gtkwindow.h> // gtk_window_new()

#include "conf.h"
// #include "logfile.h"
#include "device.h" // DeviceOpen(), DeviceClose()
#include "mainbox.h"


struct MainBoxData mainbox;


void MainBoxDestroy() {
  if(mainbox.window != NULL) {
    gtk_widget_destroy(mainbox.window);
    mainbox.window = NULL;
    mainbox.device = NULL;
    mainbox.desc = NULL;
  } // ENDIF- Do we have a Main Window still?
} // END MainBoxDestroy()


void MainBoxUnfocus() {
  gtk_widget_set_sensitive(mainbox.device, FALSE);
  gtk_window_iconify(GTK_WINDOW(mainbox.window));
} // END MainBoxUnfocus()


gint MainBoxDeviceEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  struct stat filestat;
  int retval;

  retval = stat(gtk_entry_get_text(GTK_ENTRY(mainbox.device)), &filestat);
  if(retval == -1) {
    gtk_label_set_text(GTK_LABEL(mainbox.desc), "Device Type: ---");
    return(TRUE);
  } // ENDIF- Not a name of any sort?

  if(S_ISDIR(filestat.st_mode) != 0) {
    gtk_label_set_text(GTK_LABEL(mainbox.desc), "Device Type: Not a device");
    return(TRUE);
  } // ENDIF- Not a regular file?

  gtk_label_set_text(GTK_LABEL(mainbox.desc), "Device Type: Device Likely");
  return(TRUE);
} // END MainBoxFileEvent()


void MainBoxRefocus() {
  GdkEvent event;

  MainBoxDeviceEvent(NULL, event, NULL);

  gtk_widget_set_sensitive(mainbox.device, TRUE);
  gtk_window_set_focus(GTK_WINDOW(mainbox.window), mainbox.device);
  gtk_window_deiconify(GTK_WINDOW(mainbox.window));
} // END MainBoxRefocus()


gint MainBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  MainBoxDestroy();

  gtk_main_quit();
  return(TRUE);
} // END MainBoxCancelEvent()


gint MainBoxOKEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  const char *tempdevice;
  int retval;

  MainBoxUnfocus();

  tempdevice = gtk_entry_get_text(GTK_ENTRY(mainbox.device));
  strcpy(conf.devicename, tempdevice); // Temporarily put in new device name
  tempdevice = NULL;
  if(*(conf.devicename) != 0) {
    retval = DeviceOpen(); // Test by opening the device.
    DeviceClose(); // Failed or not, close it.
    if(retval != 0) {
      MainBoxRefocus();
      return(TRUE);
    } // ENDIF- Not an ISO file? Message and Stop here.
  } // ENDIF- Is there an ISO file to check out?

  SaveConf();

  MainBoxCancelEvent(widget, event, data);
  return(TRUE);
} // END MainBoxOKEvent()


void MainBoxDisplay() {
  GtkWidget *item;
  GtkWidget *hbox1;
  GtkWidget *vbox1;

  mainbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(mainbox.window), 5);
  gtk_window_set_title(GTK_WINDOW(mainbox.window), "CDVDlinuz Configuration");
  gtk_window_set_position(GTK_WINDOW(mainbox.window), GTK_WIN_POS_CENTER);

  g_signal_connect(G_OBJECT(mainbox.window), "delete_event",
                   G_CALLBACK(MainBoxCancelEvent), NULL);

  vbox1 = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(mainbox.window), vbox1);
  gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
  gtk_widget_show(vbox1);

  hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_label_new("CD/DVD Device:");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  mainbox.device = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox1), mainbox.device, TRUE, TRUE, 0);
  gtk_widget_show(mainbox.device);
  g_signal_connect(G_OBJECT(mainbox.device), "changed",
                   G_CALLBACK(MainBoxDeviceEvent), NULL);
  hbox1 = NULL;

  mainbox.desc = gtk_label_new("File Type: ---");
  gtk_box_pack_start(GTK_BOX(vbox1), mainbox.desc, FALSE, FALSE, 0);
  gtk_widget_show(mainbox.desc);

  hbox1 = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_button_new_with_label("Ok");
  gtk_box_pack_start(GTK_BOX(hbox1), item, TRUE, TRUE, 0);
  gtk_widget_show(item);
  g_signal_connect(G_OBJECT(item), "clicked",
                   G_CALLBACK(MainBoxOKEvent), NULL);

  item = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hbox1), item, TRUE, TRUE, 0);
  gtk_widget_show(item);
  g_signal_connect(G_OBJECT(item), "clicked",
                   G_CALLBACK(MainBoxCancelEvent), NULL);
  item = NULL;
  hbox1 = NULL;
  vbox1 = NULL;

  // We held off setting the name until now... so description would show.
  gtk_entry_set_text(GTK_ENTRY(mainbox.device), conf.devicename);
} // END MainBoxDisplay()
