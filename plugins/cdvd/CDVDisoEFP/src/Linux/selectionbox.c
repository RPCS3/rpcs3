/*  selectionbox.c
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

#include <glib-object.h> // g_signal_connect()
#include <gtk/gtkentry.h> // gtk_entry_set_text()
#include <gtk/gtkfilesel.h> // gtk_file_selection_new()

#include "devicebox.h"
#include "conversionbox.h"
#include "selectionbox.h"
#include "mainbox.h"


struct SelectionBoxData selectionbox;


void SelectionBoxDestroy() {
  if(selectionbox.window != NULL) {
    gtk_widget_destroy(selectionbox.window);
    selectionbox.window = NULL;
  } // ENDIF- Do we have a Main Window still?
} // END SelectionBoxDestroy()


void SelectionBoxRefresh() {
} // END SelectionBoxRefresh()


void SelectionBoxUnfocus() {
  gtk_widget_hide(selectionbox.window);
} // END SelectionBoxUnfocus()


void SelectionBoxRefocus() {
  gtk_widget_show(selectionbox.window);
} // END SelectionBoxRefocus()


gint SelectionBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  SelectionBoxUnfocus();
  switch(selectionbox.wherefrom) {
    case 1:
      gtk_entry_set_text(GTK_ENTRY(mainbox.file),
                         gtk_file_selection_get_filename(GTK_FILE_SELECTION(selectionbox.window)));
      MainBoxRefocus();
      break;
    case 2:
      gtk_entry_set_text(GTK_ENTRY(devicebox.file),
                         gtk_file_selection_get_filename(GTK_FILE_SELECTION(selectionbox.window)));
      DeviceBoxRefocus();
      break;
    case 3:
      gtk_entry_set_text(GTK_ENTRY(conversionbox.file),
                         gtk_file_selection_get_filename(GTK_FILE_SELECTION(selectionbox.window)));
      ConversionBoxRefocus();
      break;
  } // ENDSWITCH wherefrom- What Box called us?
  return(TRUE);
} // END SelectionBoxCancelEvent()


gint SelectionBoxOKEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  // Validate listed file(?)

  // Transfer file name to calling window?

  SelectionBoxCancelEvent(widget, event, data);
  return(TRUE);
} // END SelectionBoxOKEvent()


void SelectionBoxDisplay() {
  selectionbox.window = gtk_file_selection_new("Select an ISO file");
  g_signal_connect(G_OBJECT(selectionbox.window), "delete_event",
                   G_CALLBACK(SelectionBoxCancelEvent), NULL);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(selectionbox.window)->ok_button), 
                   "clicked", G_CALLBACK(SelectionBoxOKEvent), NULL);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(selectionbox.window)->cancel_button),
                   "clicked", G_CALLBACK(SelectionBoxCancelEvent), NULL);

  selectionbox.wherefrom = 0; // Called by no one... yet.
} // END SelectionBoxDisplay()
