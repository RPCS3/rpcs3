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

#include <gtk/gtkbutton.h> // gtk_button_new_with_label()
#include <gtk/gtkcheckbutton.h> // gtk_check_button_new_with_label()
#include <gtk/gtkcontainer.h> // gtk_container_add()
#include <gtk/gtkentry.h> // gtk_entry_new()
#include <gtk/gtkfilesel.h> // gtk_file_selection_set_filename()
#include <gtk/gtkhbbox.h> // gtk_hbutton_box_new()
#include <gtk/gtkhbox.h> // gtk_hbox_new()
#include <gtk/gtklabel.h> // gtk_label_new()
#include <gtk/gtkmain.h> // gtk_init(), gtk_main(), gtk_main_quit()
#include <gtk/gtktogglebutton.h> // gtk_toggle_button_set_active(), (_get_)
#include <gtk/gtkvbox.h> // gtk_vbox_new()
#include <gtk/gtkwindow.h> // gtk_window_new()

#include "conf.h"
#include "isofile.h"
#include "isocompress.h" // compressdesc[]
// #include "imagetype.h" // imagedata[].name
#include "multifile.h" // multinames[]
#include "tablerebuild.h" // IsoTableRebuild()
#include "devicebox.h"
#include "conversionbox.h"
#include "progressbox.h"
#include "messagebox.h"
#include "selectionbox.h"
#include "mainbox.h"


struct MainBoxData mainbox;


void MainBoxDestroy() {
  if(mainbox.window != NULL) {
    gtk_widget_destroy(mainbox.window);
    mainbox.window = NULL;
    mainbox.file = NULL;
    mainbox.selectbutton = NULL;
    mainbox.desc = NULL;
    mainbox.startcheck = NULL;
    mainbox.restartcheck = NULL;
    mainbox.okbutton = NULL;
    mainbox.devbutton = NULL;
    mainbox.convbutton = NULL;
  } // ENDIF- Do we have a Main Window still?
} // END MainBoxDestroy()


void MainBoxUnfocus() {
  gtk_widget_set_sensitive(mainbox.file, FALSE);
  gtk_widget_set_sensitive(mainbox.selectbutton, FALSE);
  gtk_widget_set_sensitive(mainbox.startcheck, FALSE);
  gtk_widget_set_sensitive(mainbox.restartcheck, FALSE);
  gtk_widget_set_sensitive(mainbox.okbutton, FALSE);
  gtk_widget_set_sensitive(mainbox.devbutton, FALSE);
  gtk_widget_set_sensitive(mainbox.convbutton, FALSE);
  gtk_window_iconify(GTK_WINDOW(mainbox.window));
} // END MainBoxUnfocus()


gint MainBoxFileEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  int returnval;
  char templine[256];
  struct IsoFile *tempfile;

  returnval = IsIsoFile(gtk_entry_get_text(GTK_ENTRY(mainbox.file)));
  if(returnval == -1) {
    gtk_label_set_text(GTK_LABEL(mainbox.desc), "File Type: ---");
    return(TRUE);
  } // ENDIF- Not a name of any sort?

  if(returnval == -2) {
    gtk_label_set_text(GTK_LABEL(mainbox.desc), "File Type: Not a file");
    return(TRUE);
  } // ENDIF- Not a regular file?

  if(returnval == -3) {
    gtk_label_set_text(GTK_LABEL(mainbox.desc), "File Type: Not a valid image file");
    return(TRUE);
  } // ENDIF- Not an Image file?

  if(returnval == -4) {
    gtk_label_set_text(GTK_LABEL(mainbox.desc), "File Type: Missing Table File (will rebuild)");
    return(TRUE);
  } // ENDIF- Missing Compression seek table?

  tempfile = IsoFileOpenForRead(gtk_entry_get_text(GTK_ENTRY(mainbox.file)));
  sprintf(templine, "File Type: %s%s%s",
          multinames[tempfile->multi],
          tempfile->imagename,
          compressdesc[tempfile->compress]);
  gtk_label_set_text(GTK_LABEL(mainbox.desc), templine);
  tempfile = IsoFileClose(tempfile);
  return(TRUE);
} // END MainBoxFileEvent()


void MainBoxRefocus() {
  GdkEvent event;

  MainBoxFileEvent(NULL, event, NULL);

  gtk_widget_set_sensitive(mainbox.file, TRUE);
  gtk_widget_set_sensitive(mainbox.selectbutton, TRUE);
  gtk_widget_set_sensitive(mainbox.startcheck, TRUE);
  gtk_widget_set_sensitive(mainbox.restartcheck, TRUE);
  gtk_widget_set_sensitive(mainbox.okbutton, TRUE);
  gtk_widget_set_sensitive(mainbox.devbutton, TRUE);
  gtk_widget_set_sensitive(mainbox.convbutton, TRUE);
  gtk_window_set_focus(GTK_WINDOW(mainbox.window), mainbox.file);
  gtk_window_deiconify(GTK_WINDOW(mainbox.window));
} // END MainBoxRefocus()


gint MainBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  mainbox.stop = 1; // Halt all long processess...

  MessageBoxDestroy();
  SelectionBoxDestroy();
  ProgressBoxDestroy();
  ConversionBoxDestroy();
  DeviceBoxDestroy();
  MainBoxDestroy();

  gtk_main_quit();
  return(TRUE);
} // END MainBoxCancelEvent()


gint MainBoxOKEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  const char *tempisoname;

  MainBoxUnfocus();

  tempisoname = gtk_entry_get_text(GTK_ENTRY(mainbox.file));
  if(*(tempisoname) != 0) {
    if(IsIsoFile(tempisoname) == -4) {
      IsoTableRebuild(tempisoname);
      MainBoxRefocus();
      return(TRUE);
    } // ENDIF- Do we need to rebuild an image file's index before using it?

    if(IsIsoFile(tempisoname) < 0) {
      tempisoname = NULL;
      MainBoxRefocus();
      MessageBoxShow("Not a Valid Image File.", 1);
      return(TRUE);
    } // ENDIF- Not an ISO file? Message and Stop here.
  } // ENDIF- Is there an ISO file to check out?

  strcpy(conf.isoname, tempisoname);
  tempisoname = NULL;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mainbox.startcheck)) == FALSE) {
    conf.startconfigure = 0; // FALSE
  } else {
    conf.startconfigure = 1; // TRUE
  } // ENDIF- Was this check button turned off?
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mainbox.restartcheck)) == FALSE) {
    conf.restartconfigure = 0; // FALSE
  } else {
    conf.restartconfigure = 1; // TRUE
  } // ENDIF- Was this check button turned off?

  SaveConf();

  MainBoxCancelEvent(widget, event, data);
  return(TRUE);
} // END MainBoxOKEvent()


gint MainBoxBrowseEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  MainBoxUnfocus();

  gtk_file_selection_set_filename(GTK_FILE_SELECTION(selectionbox.window),
                                  gtk_entry_get_text(GTK_ENTRY(mainbox.file)));
  selectionbox.wherefrom = 1; // From the Main Window
  SelectionBoxRefocus();  
  return(TRUE);
} // END MainBoxBrowseEvent()


gint MainBoxDeviceEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  MainBoxUnfocus();

  gtk_entry_set_text(GTK_ENTRY(devicebox.file),
                     gtk_entry_get_text(GTK_ENTRY(mainbox.file)));
  gtk_window_set_focus(GTK_WINDOW(devicebox.window), devicebox.file);
  gtk_widget_show_all(devicebox.window);
  return(TRUE);
} // END MainBoxBrowseEvent()


gint MainBoxConversionEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  MainBoxUnfocus();

  gtk_entry_set_text(GTK_ENTRY(conversionbox.file),
                     gtk_entry_get_text(GTK_ENTRY(mainbox.file)));
  gtk_window_set_focus(GTK_WINDOW(conversionbox.window), conversionbox.file);
  gtk_widget_show_all(conversionbox.window);
  return(TRUE);
} // END MainBoxBrowseEvent()


void MainBoxDisplay() {
  GtkWidget *item;
  GtkWidget *hbox1;
  GtkWidget *vbox1;

  mainbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(mainbox.window), 5);
  gtk_window_set_title(GTK_WINDOW(mainbox.window), "CDVDisoEFP Configuration");
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

  item = gtk_label_new("Iso File:");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  mainbox.file = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox1), mainbox.file, TRUE, TRUE, 0);
  gtk_widget_show(mainbox.file);
  g_signal_connect(G_OBJECT(mainbox.file), "changed",
                   G_CALLBACK(MainBoxFileEvent), NULL);

  mainbox.selectbutton = gtk_button_new_with_label("Browse");
  gtk_box_pack_start(GTK_BOX(hbox1), mainbox.selectbutton, FALSE, FALSE, 0);
  gtk_widget_show(mainbox.selectbutton);
  g_signal_connect(G_OBJECT(mainbox.selectbutton), "clicked",
                   G_CALLBACK(MainBoxBrowseEvent), NULL);
  hbox1 = NULL;

  mainbox.desc = gtk_label_new("File Type: ---");
  gtk_box_pack_start(GTK_BOX(vbox1), mainbox.desc, FALSE, FALSE, 0);
  gtk_widget_show(mainbox.desc);

  // hbox1 = gtk_hbox_new(FALSE, 10);
  // gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  // gtk_widget_show(hbox1);

  mainbox.startcheck = gtk_check_button_new_with_label("Show Configure screen when starting emulation");
  gtk_box_pack_start(GTK_BOX(vbox1), mainbox.startcheck, FALSE, FALSE, 0);
  gtk_widget_show(mainbox.startcheck);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mainbox.startcheck), FALSE);
  if(conf.startconfigure != 0) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mainbox.startcheck), TRUE);
  } // ENDIF- Is this box supposed to be checked?

  mainbox.restartcheck = gtk_check_button_new_with_label("Show Configure screen when restarting emulation");
  gtk_box_pack_start(GTK_BOX(vbox1), mainbox.restartcheck, FALSE, FALSE, 0);
  gtk_widget_show(mainbox.restartcheck);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mainbox.restartcheck), FALSE);
  if(conf.restartconfigure != 0) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mainbox.restartcheck), TRUE);
  } // ENDIF- Is this box supposed to be checked?

  hbox1 = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  mainbox.okbutton = gtk_button_new_with_label("Ok");
  gtk_box_pack_start(GTK_BOX(hbox1), mainbox.okbutton, TRUE, TRUE, 0);
  gtk_widget_show(mainbox.okbutton);
  g_signal_connect(G_OBJECT(mainbox.okbutton), "clicked",
                   G_CALLBACK(MainBoxOKEvent), NULL);

  mainbox.devbutton = gtk_button_new_with_label("Get from Disc");
  gtk_box_pack_start(GTK_BOX(hbox1), mainbox.devbutton, TRUE, TRUE, 0);
  gtk_widget_show(mainbox.devbutton);
  g_signal_connect(G_OBJECT(mainbox.devbutton), "clicked",
                   G_CALLBACK(MainBoxDeviceEvent), NULL);

  mainbox.convbutton = gtk_button_new_with_label("Convert");
  gtk_box_pack_start(GTK_BOX(hbox1), mainbox.convbutton, TRUE, TRUE, 0);
  gtk_widget_show(mainbox.convbutton);
  g_signal_connect(G_OBJECT(mainbox.convbutton), "clicked",
                   G_CALLBACK(MainBoxConversionEvent), NULL);

  item = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hbox1), item, TRUE, TRUE, 0);
  gtk_widget_show(item);
  g_signal_connect(G_OBJECT(item), "clicked",
                   G_CALLBACK(MainBoxCancelEvent), NULL);
  item = NULL;
  hbox1 = NULL;
  vbox1 = NULL;

  // We held off setting the name until now... so description would show.
  gtk_entry_set_text(GTK_ENTRY(mainbox.file), conf.isoname);
} // END MainBoxDisplay()
