/*  devicebox.c
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

#ifndef __LINUX__
#ifdef __linux__
#define __LINUX__
#endif /* __linux__ */
#endif /* No __LINUX__ */
#define CDVDdefs
#include "PS2Edefs.h"

#include "conf.h"
#include "device.h"
#include "isofile.h"
#include "isocompress.h" // compressdesc[]
// #include "imagetype.h" // imagedata[].name
#include "multifile.h" // multinames[]
#include "toc.h"
#include "progressbox.h"
#include "messagebox.h"
#include "selectionbox.h"
#include "mainbox.h"
#include "devicebox.h"


struct DeviceBoxData devicebox;


void DeviceBoxDestroy() {
  if(devicebox.window != NULL) {
    gtk_widget_destroy(devicebox.window);
    devicebox.window = NULL;
    devicebox.device = NULL;
    devicebox.devicedesc = NULL;
    devicebox.file = NULL;
    devicebox.selectbutton = NULL;
    devicebox.filedesc = NULL;
    devicebox.compress = NULL;
    devicebox.multi = NULL;
    devicebox.okbutton = NULL;
    devicebox.cancelbutton = NULL;
  } // ENDIF- Do we have a Main Window still?
} // END DeviceBoxDestroy()


void DeviceBoxUnfocus() {
  gtk_widget_set_sensitive(devicebox.device, FALSE);
  gtk_widget_set_sensitive(devicebox.file, FALSE);
  gtk_widget_set_sensitive(devicebox.selectbutton, FALSE);
  gtk_widget_set_sensitive(devicebox.compress, FALSE);
  gtk_widget_set_sensitive(devicebox.multi, FALSE);
  gtk_widget_set_sensitive(devicebox.okbutton, FALSE);
  gtk_widget_set_sensitive(devicebox.cancelbutton, FALSE);
  gtk_widget_hide(devicebox.window);
  // gtk_window_iconify(GTK_WINDOW(devicebox.window));
} // END DeviceBoxUnfocus()


gint DeviceBoxDeviceEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  struct stat filestat;
  int returnval;

  returnval = stat(gtk_entry_get_text(GTK_ENTRY(devicebox.device)), &filestat);
  if(returnval == -1) {
    gtk_label_set_text(GTK_LABEL(devicebox.devicedesc), "Device Type: ---");
    return(TRUE);
  } // ENDIF- Not a name of any sort?

  if(S_ISDIR(filestat.st_mode) != 0) {
    gtk_label_set_text(GTK_LABEL(devicebox.devicedesc), "Device Type: Not a device");
    return(TRUE);
  } // ENDIF- Not a regular file?

  gtk_label_set_text(GTK_LABEL(devicebox.devicedesc), "Device Type: Device Likely");
  return(TRUE);
} // END DeviceBoxDeviceEvent()


gint DeviceBoxFileEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  int returnval;
  char templine[256];
  struct IsoFile *tempfile;

  returnval = IsIsoFile(gtk_entry_get_text(GTK_ENTRY(devicebox.file)));
  if(returnval == -1) {
    gtk_label_set_text(GTK_LABEL(devicebox.filedesc), "File Type: ---");
    return(TRUE);
  } // ENDIF- Not a name of any sort?

  if(returnval == -2) {
    gtk_label_set_text(GTK_LABEL(devicebox.filedesc), "File Type: Not a file");
    return(TRUE);
  } // ENDIF- Not a regular file?

  if(returnval == -3) {
    gtk_label_set_text(GTK_LABEL(devicebox.filedesc), "File Type: Not a valid image file");
    return(TRUE);
  } // ENDIF- Not an image file?

  if(returnval == -4) {
    gtk_label_set_text(GTK_LABEL(devicebox.filedesc), "File Type: Missing Table File (will rebuild)");
    return(TRUE);
  } // ENDIF- Not a regular file?

  tempfile = IsoFileOpenForRead(gtk_entry_get_text(GTK_ENTRY(devicebox.file)));
  sprintf(templine, "File Type: %s%s%s",
          multinames[tempfile->multi],
          tempfile->imagename,
          compressdesc[tempfile->compress]);
  gtk_label_set_text(GTK_LABEL(devicebox.filedesc), templine);
  tempfile = IsoFileClose(tempfile);
  return(TRUE);
} // END DeviceBoxFileEvent()


void DeviceBoxRefocus() {
  GdkEvent event;

  DeviceBoxDeviceEvent(NULL, event, NULL);
  DeviceBoxFileEvent(NULL, event, NULL);

  gtk_widget_set_sensitive(devicebox.device, TRUE);
  gtk_widget_set_sensitive(devicebox.file, TRUE);
  gtk_widget_set_sensitive(devicebox.selectbutton, TRUE);
  gtk_widget_set_sensitive(devicebox.compress, TRUE);
  gtk_widget_set_sensitive(devicebox.multi, TRUE);
  gtk_widget_set_sensitive(devicebox.okbutton, TRUE);
  gtk_widget_set_sensitive(devicebox.cancelbutton, TRUE);
  gtk_window_set_focus(GTK_WINDOW(devicebox.window), devicebox.file);
  gtk_widget_show_all(devicebox.window);
  gtk_window_deiconify(GTK_WINDOW(devicebox.window));
} // END DeviceBoxRefocus()


gint DeviceBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  gtk_widget_hide(devicebox.window);
  MainBoxRefocus();
  return(TRUE);
} // END DeviceBoxCancelEvent()


gint DeviceBoxOKEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  char templine[256];
  u8 tempbuffer[2352];
  struct IsoFile *tofile;
  const char *tempdevice;
  s32 retval;
  cdvdTD cdvdtd;
  int stop;
  int compressmethod;
  int multi;
  int imagetype;
  int i;

  DeviceBoxUnfocus();

  tempdevice = gtk_entry_get_text(GTK_ENTRY(devicebox.device));
  strcpy(conf.devicename, tempdevice); // Temporarily put in new device name
  tempdevice = NULL;
  retval = DeviceOpen();
  if(retval != 0) {
    DeviceClose();
    MessageBoxShow("Could not open the device", 2);
    return(TRUE);
  } // ENDIF- Trouble opening device? Abort here.

  DeviceTrayStatus();
  retval = DiscInserted();
  if(retval != 0) {
    DeviceClose();
    MessageBoxShow("No disc in the device\r\nPlease put a disc in and try again.", 2);
    return(TRUE);
  } // ENDIF- Trouble opening device? Abort here.

  retval = DeviceGetTD(0, &cdvdtd); // Fish for Ending Sector
  if(retval < 0) {
    DeviceClose();
    MessageBoxShow("Could not retrieve disc sector size", 2);
    return(TRUE);
  } // ENDIF- Trouble getting disc sector count?

  compressmethod = gtk_combo_box_get_active(GTK_COMBO_BOX(devicebox.compress));
  if(compressmethod > 0)  compressmethod += 2;
  multi = 0;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(devicebox.multi)) == TRUE)
    multi = 1;

  imagetype = 0;
  if((disctype != CDVD_TYPE_PS2DVD) &&
     (disctype != CDVD_TYPE_DVDV))  imagetype = 8;

  tofile = IsoFileOpenForWrite(gtk_entry_get_text(GTK_ENTRY(devicebox.file)),
                               imagetype,
                               multi,
                               compressmethod);
  if(tofile == NULL) {
    DeviceClose();
    MessageBoxShow("Could not create the new ISO file", 2);
    return(TRUE);
  } // ENDIF- Trouble opening the ISO file?

  // Open Progress Bar
  sprintf(templine, "%s -> %s",
          gtk_entry_get_text(GTK_ENTRY(devicebox.device)), tofile->name);
  ProgressBoxStart(templine, (off64_t) cdvdtd.lsn);

  tofile->cdvdtype = disctype;
  for(i = 0; i < 2048; i++)  tofile->toc[i] = tocbuffer[i];

  stop = 0;
  mainbox.stop = 0;
  progressbox.stop = 0;
  while((stop == 0) && (tofile->sectorpos < cdvdtd.lsn)) {
    if(imagetype == 0) {
      retval = DeviceReadTrack((u32) tofile->sectorpos,
                               CDVD_MODE_2048,
                               tempbuffer);
    } else {
      retval = DeviceReadTrack((u32) tofile->sectorpos,
                               CDVD_MODE_2352,
                               tempbuffer);
    } // ENDIF- Are we reading a DVD sector? (Or a CD sector?)
    if(retval < 0) {
      for(i = 0; i < 2352; i++) {
        tempbuffer[i] = 0;
      } // NEXT i- Zeroing the buffer
    } // ENDIF- Trouble reading next block?
    retval = IsoFileWrite(tofile, tempbuffer);
    if(retval < 0) {
      MessageBoxShow("Trouble writing new file", 3);
      stop = 1;
    } // ENDIF- Trouble writing out the next block?

    ProgressBoxTick(tofile->sectorpos);
    while(gtk_events_pending())  gtk_main_iteration();

    if(mainbox.stop != 0)  stop = 2;
    if(progressbox.stop != 0)  stop = 2;
  } // ENDWHILE- No reason found to stop...

  ProgressBoxStop();

  if(stop == 0) {
    if(tofile->multi == 1)  tofile->name[tofile->multipos] = '0'; // First file
    strcpy(templine, tofile->name);
  } // ENDIF- Did we succeed with the transfer?

  DeviceClose();
  if(stop == 0) {
    IsoSaveTOC(tofile);
    tofile = IsoFileClose(tofile);
    gtk_entry_set_text(GTK_ENTRY(mainbox.file), templine);
  } else {
    tofile = IsoFileCloseAndDelete(tofile);
  } // ENDIF- (Failed to complete writing file? Get rid of the garbage files.)

  if(stop != 1)  DeviceBoxRefocus();
  if(stop == 0)  DeviceBoxCancelEvent(widget, event, data);
  return(TRUE);
} // END DeviceBoxOKEvent()


gint DeviceBoxBrowseEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  DeviceBoxUnfocus();

  // Transfer file name to file selection
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(selectionbox.window),
                                  gtk_entry_get_text(GTK_ENTRY(devicebox.file)));
  selectionbox.wherefrom = 2; // From the Device Window
  SelectionBoxRefocus();  
  return(TRUE);
} // END DeviceBoxBrowseEvent()


void DeviceBoxDisplay() {
  GtkWidget *item;
  GtkWidget *hbox1;
  GtkWidget *vbox1;
  int nameptr;

  devicebox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(devicebox.window), 5);
  gtk_window_set_title(GTK_WINDOW(devicebox.window), "CDVDisoEFP ISO Creation");
  gtk_window_set_position(GTK_WINDOW(devicebox.window), GTK_WIN_POS_CENTER);

  g_signal_connect(G_OBJECT(devicebox.window), "delete_event",
                   G_CALLBACK(DeviceBoxCancelEvent), NULL);

  vbox1 = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(devicebox.window), vbox1);
  gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
  gtk_widget_show(vbox1);

  hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_label_new("Source CD/DVD Device:");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  devicebox.device = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox1), devicebox.device, TRUE, TRUE, 0);
  gtk_widget_show(devicebox.device);
  g_signal_connect(G_OBJECT(devicebox.device), "changed",
                   G_CALLBACK(DeviceBoxDeviceEvent), NULL);
  hbox1 = NULL;

  devicebox.devicedesc = gtk_label_new("Device Type: ---");
  gtk_box_pack_start(GTK_BOX(vbox1), devicebox.devicedesc, FALSE, FALSE, 0);
  gtk_widget_show(devicebox.devicedesc);

  item = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox1), item, TRUE, TRUE, 0);
  gtk_widget_show(item);
  item = NULL;

  hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_label_new("Iso File:");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  devicebox.file = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox1), devicebox.file, TRUE, TRUE, 0);
  gtk_widget_show(devicebox.file);
  g_signal_connect(G_OBJECT(devicebox.file), "changed",
                   G_CALLBACK(DeviceBoxFileEvent), NULL);

  devicebox.selectbutton = gtk_button_new_with_label("Browse");
  gtk_box_pack_start(GTK_BOX(hbox1), devicebox.selectbutton, FALSE, FALSE, 0);
  gtk_widget_show(devicebox.selectbutton);
  g_signal_connect(G_OBJECT(devicebox.selectbutton), "clicked",
                   G_CALLBACK(DeviceBoxBrowseEvent), NULL);
  hbox1 = NULL;

  devicebox.filedesc = gtk_label_new("File Type: ---");
  gtk_box_pack_start(GTK_BOX(vbox1), devicebox.filedesc, FALSE, FALSE, 0);
  gtk_widget_show(devicebox.filedesc);

  hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_label_new("New File Compression:");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  devicebox.compress = gtk_combo_box_new_text();
  gtk_box_pack_start(GTK_BOX(hbox1), devicebox.compress, FALSE, FALSE, 0);
  nameptr = 0;
  while(compressnames[nameptr] != NULL) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(devicebox.compress),
                              compressnames[nameptr]);
    nameptr++;
  } // ENDWHILE- loading compression types into combo box
  gtk_combo_box_set_active(GTK_COMBO_BOX(devicebox.compress), 0); // Temp Line
  gtk_widget_show(devicebox.compress);
  hbox1 = NULL;

  hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_label_new("Multiple Files (all under 2 GB):");
  gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
  gtk_widget_show(item);
  item = NULL;

  devicebox.multi = gtk_check_button_new();
  gtk_box_pack_start(GTK_BOX(hbox1), devicebox.multi, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(devicebox.multi), FALSE);
  gtk_widget_show(devicebox.multi);
  hbox1 = NULL;

  hbox1 = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  devicebox.okbutton = gtk_button_new_with_label("Make File");
  gtk_box_pack_start(GTK_BOX(hbox1), devicebox.okbutton, TRUE, TRUE, 0);
  gtk_widget_show(devicebox.okbutton);
  g_signal_connect(G_OBJECT(devicebox.okbutton), "clicked",
                   G_CALLBACK(DeviceBoxOKEvent), NULL);

  devicebox.cancelbutton = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hbox1), devicebox.cancelbutton, TRUE, TRUE, 0);
  gtk_widget_show(devicebox.cancelbutton);
  g_signal_connect(G_OBJECT(devicebox.cancelbutton), "clicked",
                   G_CALLBACK(DeviceBoxCancelEvent), NULL);
  hbox1 = NULL;
  vbox1 = NULL;

  // Device text not set until now to get the correct description.
  gtk_entry_set_text(GTK_ENTRY(devicebox.device), conf.devicename);

  DeviceInit(); // Initialize device access
} // END DeviceBoxDisplay()
