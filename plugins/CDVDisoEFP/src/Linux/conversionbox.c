/*  conversionbox.c
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
#include <sys/types.h> // off64_t

#include <gtk/gtkbutton.h> // gtk_button_new_with_label()
#include <gtk/gtkcombobox.h> // gtk_combo_box_new()
#include <gtk/gtkcheckbutton.h> // gtk_check_button_new()
#include <gtk/gtkcontainer.h> // gtk_container_add()
#include <gtk/gtkentry.h> // gtk_entry_new()
#include <gtk/gtkfilesel.h> // gtk_file_selection_set_filename()
#include <gtk/gtkhbbox.h> // gtk_hbutton_box_new()
#include <gtk/gtkhbox.h> // gtk_hbox_new()
#include <gtk/gtkhseparator.h> // gtk_hseparator_new()
#include <gtk/gtklabel.h> // gtk_label_new()
#include <gtk/gtkmain.h> // gtk_main_iteration()
#include <gtk/gtktogglebutton.h> // gtk_toggle_button_get_active()
#include <gtk/gtkvbox.h> // gtk_vbox_new()
#include <gtk/gtkwindow.h> // gtk_window_new()

#include "isofile.h"
#include "isocompress.h" // compressdesc[]
#include "imagetype.h" // imagedata[]
#include "multifile.h" // multinames[]
#include "toc.h"
#include "progressbox.h"
#include "messagebox.h"
#include "selectionbox.h"
#include "mainbox.h"
#include "conversionbox.h"


struct ConversionBoxData conversionbox;


void ConversionBoxDestroy() {
  if(conversionbox.window != NULL) {
    gtk_widget_destroy(conversionbox.window);
    conversionbox.window = NULL;
    conversionbox.file = NULL;
    conversionbox.selectbutton = NULL;
    conversionbox.filedesc = NULL;
    conversionbox.compress = NULL;
    conversionbox.multi = NULL;
    conversionbox.okbutton = NULL;
    conversionbox.cancelbutton = NULL;
  } // ENDIF- Do we have a Main Window still?
} // END ConversionBoxDestroy()


void ConversionBoxUnfocus() {
  gtk_widget_set_sensitive(conversionbox.file, FALSE);
  gtk_widget_set_sensitive(conversionbox.selectbutton, FALSE);
  gtk_widget_set_sensitive(conversionbox.compress, FALSE);
  gtk_widget_set_sensitive(conversionbox.multi, FALSE);
  gtk_widget_set_sensitive(conversionbox.okbutton, FALSE);
  gtk_widget_set_sensitive(conversionbox.cancelbutton, FALSE);
  // gtk_window_iconify(GTK_WINDOW(conversionbox.window));
  gtk_widget_hide(conversionbox.window);
} // END ConversionBoxUnfocus()


gint ConversionBoxFileEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  int returnval;
  char templine[256];
  struct IsoFile *tempfile;

  returnval = IsIsoFile(gtk_entry_get_text(GTK_ENTRY(conversionbox.file)));
  if(returnval == -1) {
    gtk_label_set_text(GTK_LABEL(conversionbox.filedesc), "File Type: ---");
    return(TRUE);
  } // ENDIF- Not a name of any sort?

  if(returnval == -2) {
    gtk_label_set_text(GTK_LABEL(conversionbox.filedesc),
                       "File Type: Not a file");
    return(TRUE);
  } // ENDIF- Not a regular file?

  if(returnval == -3) {
    gtk_label_set_text(GTK_LABEL(conversionbox.filedesc),
                       "File Type: Not a valid image file");
    return(TRUE);
  } // ENDIF- Not a regular file?

  if(returnval == -4) {
    gtk_label_set_text(GTK_LABEL(conversionbox.filedesc),
                       "File Type: Missing Table File (will rebuild)");
    return(TRUE);
  } // ENDIF- Not a regular file?

  tempfile = IsoFileOpenForRead(gtk_entry_get_text(GTK_ENTRY(conversionbox.file)));
  sprintf(templine, "File Type: %s%s%s",
          multinames[tempfile->multi],
          tempfile->imagename,
          compressdesc[tempfile->compress]);
  gtk_label_set_text(GTK_LABEL(conversionbox.filedesc), templine);
  tempfile = IsoFileClose(tempfile);
  return(TRUE);
} // END ConversionBoxFileEvent()


void ConversionBoxRefocus() {
  GdkEvent event;

  ConversionBoxFileEvent(NULL, event, NULL);

  gtk_widget_set_sensitive(conversionbox.file, TRUE);
  gtk_widget_set_sensitive(conversionbox.selectbutton, TRUE);
  gtk_widget_set_sensitive(conversionbox.compress, TRUE);
  gtk_widget_set_sensitive(conversionbox.multi, TRUE);
  gtk_widget_set_sensitive(conversionbox.okbutton, TRUE);
  gtk_widget_set_sensitive(conversionbox.cancelbutton, TRUE);
  gtk_window_set_focus(GTK_WINDOW(conversionbox.window), conversionbox.file);
  gtk_widget_show_all(conversionbox.window);
  gtk_window_deiconify(GTK_WINDOW(conversionbox.window));
} // END ConversionBoxRefocus()


gint ConversionBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  gtk_widget_hide(conversionbox.window);
  MainBoxRefocus();
  return(TRUE);
} // END ConversionBoxCancelEvent()


gint ConversionBoxOKEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  char templine[256];
  char tempblock[2352];
  const char *filename;
  int compressmethod;
  int multi;
  struct IsoFile *fromfile;
  struct IsoFile *tofile;
  int i;
  off64_t endsector;
  int stop;
  int retval;

  ConversionBoxUnfocus();

  filename = gtk_entry_get_text(GTK_ENTRY(conversionbox.file));
  if(IsIsoFile(filename) < 0) {
    filename = NULL;
    MessageBoxShow("Not a valid file", 3);
    return(TRUE);
  } // ENDIF- Not an Iso File? Stop early.

  compressmethod = gtk_combo_box_get_active(GTK_COMBO_BOX(conversionbox.compress));
  if(compressmethod > 0)  compressmethod += 2;
  multi = 0;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(conversionbox.multi)) == TRUE)
    multi = 1;

  fromfile = NULL;
  fromfile = IsoFileOpenForRead(filename);
  if(fromfile == NULL) {
    filename = NULL;
    MessageBoxShow("Cannot opening the source file", 3);
    return(TRUE);
  } // ENDIF- Not an Iso File? Stop early.

  if((compressmethod == fromfile->compress) &&
     (multi == fromfile->multi)) {
    fromfile = IsoFileClose(fromfile);
    filename = NULL;
    MessageBoxShow("Compress/Multifile methods match - no need to convert", 3);
    return(TRUE);
  } // ENDIF- Not an Iso File? Stop early.

  tofile = IsoFileOpenForWrite(filename,
                               GetImageTypeConvertTo(fromfile->imagetype),
                               multi,
                               compressmethod);
  if(tofile == NULL) {
    fromfile = IsoFileClose(fromfile);
    filename = NULL;
    MessageBoxShow("Cannot create the new file", 3);
    return(TRUE);
  } // ENDIF- Not an Iso File? Stop early.

  if(fromfile->multi == 1) {
    i = 0;
    while((i < 10) && 
          (IsoFileSeek(fromfile, fromfile->multisectorend[i] + 1) == 0))  i++;
    endsector = fromfile->multisectorend[fromfile->multiend];
  } else {
    endsector = fromfile->filesectorsize;
  } // ENDIF- Get ending sector from multifile? (Or single file?)
  IsoFileSeek(fromfile, 0);

  // Open Progress Bar
  sprintf(templine, "%s: %s%s -> %s%s",
          filename,
          multinames[fromfile->multi],
          compressdesc[fromfile->compress],
          multinames[tofile->multi],
          compressdesc[tofile->compress]);
  ProgressBoxStart(templine, endsector);

  tofile->cdvdtype = fromfile->cdvdtype;
  for(i = 0; i < 2048; i++)  tofile->toc[i] = fromfile->toc[i];

  stop = 0;
  mainbox.stop = 0;
  progressbox.stop = 0;
  while((stop == 0) && (tofile->sectorpos < endsector)) {
    retval = IsoFileRead(fromfile, tempblock);
    if(retval < 0) {
      MessageBoxShow("Trouble reading source file", 3);
      stop = 1;
    } else {
      retval = IsoFileWrite(tofile, tempblock);
      if(retval < 0) {
        MessageBoxShow("Trouble writing new file", 3);
        stop = 1;
      } // ENDIF- Trouble writing out the next block?
    } // ENDIF- Trouble reading in the next block?

    ProgressBoxTick(tofile->sectorpos);
    while(gtk_events_pending())  gtk_main_iteration();

    if(mainbox.stop != 0)  stop = 2;
    if(progressbox.stop != 0)  stop = 2;
  } // ENDWHILE- Not stopped for some reason...

  ProgressBoxStop();

  if(stop == 0) {
    if(tofile->multi == 1)  tofile->name[tofile->multipos] = '0'; // First file
    strcpy(templine, tofile->name);

    // fromfile = IsoFileCloseAndDelete(fromfile);
    fromfile = IsoFileClose(fromfile);

    IsoSaveTOC(tofile);
    tofile = IsoFileClose(tofile);
    gtk_entry_set_text(GTK_ENTRY(mainbox.file), templine);

  } else {
    fromfile = IsoFileClose(fromfile);
    tofile = IsoFileCloseAndDelete(tofile);
  } // ENDIF- Did we succeed in the transfer?

  if(stop != 1)  ConversionBoxRefocus();
  if(stop == 0)  ConversionBoxCancelEvent(widget, event, data);
  return(TRUE);
} // END ConversionBoxOKEvent()


gint ConversionBoxBrowseEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  ConversionBoxUnfocus();

  // Transfer file name to file selection
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(selectionbox.window),
                                  gtk_entry_get_text(GTK_ENTRY(conversionbox.file)));
  selectionbox.wherefrom = 3; // From the Conversion Window
  SelectionBoxRefocus();  
  return(TRUE);
} // END ConversionBoxBrowseEvent()


void ConversionBoxDisplay() {
  GtkWidget *item;
  GtkWidget *hbox1;
  GtkWidget *vbox1;
  int nameptr;

  conversionbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(conversionbox.window), 5);
  gtk_window_set_title(GTK_WINDOW(conversionbox.window), "CDVDisoEFP File Conversion");
  gtk_window_set_position(GTK_WINDOW(conversionbox.window), GTK_WIN_POS_CENTER);

  g_signal_connect(G_OBJECT(conversionbox.window), "delete_event",
                   G_CALLBACK(ConversionBoxCancelEvent), NULL);

  vbox1 = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(conversionbox.window), vbox1);
  gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
  gtk_widget_show(vbox1);

  hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_label_new("Iso File:");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  conversionbox.file = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox1), conversionbox.file, TRUE, TRUE, 0);
  gtk_widget_show(conversionbox.file);
  g_signal_connect(G_OBJECT(conversionbox.file), "changed",
                   G_CALLBACK(ConversionBoxFileEvent), NULL);

  conversionbox.selectbutton = gtk_button_new_with_label("Browse");
  gtk_box_pack_start(GTK_BOX(hbox1), conversionbox.selectbutton, FALSE, FALSE, 0);
  gtk_widget_show(conversionbox.selectbutton);
  g_signal_connect(G_OBJECT(conversionbox.selectbutton), "clicked",
                   G_CALLBACK(ConversionBoxBrowseEvent), NULL);
  hbox1 = NULL;

  conversionbox.filedesc = gtk_label_new("File Type: ---");
  gtk_box_pack_start(GTK_BOX(vbox1), conversionbox.filedesc, FALSE, FALSE, 0);
  gtk_widget_show(conversionbox.filedesc);
  // ConversionBoxFileEvent(NULL, 0, NULL); // Work out compromise later...

  item = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox1), item, TRUE, TRUE, 0);
  gtk_widget_show(item);
  item = NULL;

  hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_label_new("Change Compression To:");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  conversionbox.compress = gtk_combo_box_new_text();
  gtk_box_pack_start(GTK_BOX(hbox1), conversionbox.compress, FALSE, FALSE, 0);
  nameptr = 0;
  while(compressnames[nameptr] != NULL) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(conversionbox.compress),
                              compressnames[nameptr]);
    nameptr++;
  } // ENDWHILE- loading compression types into combo box
  gtk_combo_box_set_active(GTK_COMBO_BOX(conversionbox.compress), 0); // Temp Line
  gtk_widget_show(conversionbox.compress);
  hbox1 = NULL;

  hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_label_new("Multiple Files (all under 2 GB):");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  conversionbox.multi = gtk_check_button_new();
  gtk_box_pack_start(GTK_BOX(hbox1), conversionbox.multi, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(conversionbox.multi), FALSE);
  gtk_widget_show(conversionbox.multi);
  hbox1 = NULL;

  hbox1 = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  conversionbox.okbutton = gtk_button_new_with_label("Change File");
  gtk_box_pack_start(GTK_BOX(hbox1), conversionbox.okbutton, TRUE, TRUE, 0);
  gtk_widget_show(conversionbox.okbutton);
  g_signal_connect(G_OBJECT(conversionbox.okbutton), "clicked",
                   G_CALLBACK(ConversionBoxOKEvent), NULL);

  conversionbox.cancelbutton = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hbox1), conversionbox.cancelbutton, TRUE, TRUE, 0);
  gtk_widget_show(conversionbox.cancelbutton);
  g_signal_connect(G_OBJECT(conversionbox.cancelbutton), "clicked",
                   G_CALLBACK(ConversionBoxCancelEvent), NULL);
  hbox1 = NULL;
  vbox1 = NULL;
} // END ConversionBoxDisplay()
