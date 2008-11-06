/*  progressbox.c
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
#include <sys/types.h> // off64_t

#include <gtk/gtkbutton.h> // gtk_button_new_with_label()
#include <gtk/gtkcontainer.h> // gtk_container_add()
#include <gtk/gtkfilesel.h> // gtk_file_selection_set_filename()
#include <gtk/gtkhbbox.h> // gtk_hbutton_box_new()
#include <gtk/gtkhbox.h> // gtk_hbox_new()
#include <gtk/gtklabel.h> // gtk_label_new()
#include <gtk/gtkmain.h> // gtk_main_iteration(), gtk_events_pending()
#include <gtk/gtkprogressbar.h> // gtk_progress_bar_new()
#include <gtk/gtkvbox.h> // gtk_vbox_new()
#include <gtk/gtkwindow.h> // gtk_window_new()

#include "progressbox.h"


struct ProgressBoxData progressbox;
char progressboxline[256];


void ProgressBoxDestroy() {
  if(progressbox.window != NULL) {
    gtk_widget_destroy(progressbox.window);
    progressbox.window = NULL;
    progressbox.desc = NULL;
  } // ENDIF- Do we have a Main Window still?
} // END ProgressBoxDestroy()


void ProgressBoxStart(char *description, off64_t maximum) {
  gtk_label_set_text(GTK_LABEL(progressbox.desc), description);

  progressbox.max = maximum;
  progressbox.gmax = maximum;
  progressbox.lastpct = 100;
  progressbox.stop = 0;

  ProgressBoxTick(0);
  gtk_widget_show_all(progressbox.window);
  gtk_window_deiconify(GTK_WINDOW(progressbox.window));
} // END ProgressBoxStart()


void ProgressBoxTick(off64_t current) {
  gdouble gcur;
  off64_t thispct;

  gcur = current;
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbox.bar),
                                gcur / progressbox.gmax);

  sprintf(progressboxline, "%llu of %llu", current, progressbox.max);
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbox.bar), progressboxline);

  if(progressbox.max >= 100) {
    thispct = current / ( progressbox.max / 100 );
    if(thispct != progressbox.lastpct) {
      sprintf(progressboxline, "%llu%% CDVDisoEFP Progress", thispct);
      gtk_window_set_title(GTK_WINDOW(progressbox.window), progressboxline);
      progressbox.lastpct = thispct;
    } // ENDIF- Change in percentage? (Avoiding title flicker)
  } // ENDIF- Our maximum # over 100? (Avoiding divide-by-zero error)

  while(gtk_events_pending())  gtk_main_iteration(); // Give time for window to redraw.
} // END ProgressBoxTick()


void ProgressBoxStop() {
  gtk_widget_hide(progressbox.window);
  gtk_window_set_title(GTK_WINDOW(progressbox.window), "CDVDisoEFP Progress");
} // END ProgressBoxStop()


gint ProgressBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data) {
  progressbox.stop = 1;

  return(TRUE);
} // END ProgressBoxCancelEvent()


void ProgressBoxDisplay() {
  GtkWidget *item;
  GtkWidget *hbox1;
  GtkWidget *vbox1;

  progressbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(progressbox.window), 5);
  gtk_window_set_title(GTK_WINDOW(progressbox.window), "CDVDisoEFP Progress");
  gtk_window_set_position(GTK_WINDOW(progressbox.window), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable(GTK_WINDOW(progressbox.window), FALSE);

  g_signal_connect(G_OBJECT(progressbox.window), "delete_event",
                   G_CALLBACK(ProgressBoxCancelEvent), NULL);

  vbox1 = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(progressbox.window), vbox1);
  gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
  gtk_widget_show(vbox1);

  progressbox.desc = gtk_label_new("Twiddling Thumbs");
  gtk_box_pack_start(GTK_BOX(vbox1), progressbox.desc, FALSE, FALSE, 0);
  gtk_widget_show(progressbox.desc);

  progressbox.bar = gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox1), progressbox.bar, FALSE, FALSE, 0);
  gtk_widget_show(progressbox.bar);

  hbox1 = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  item = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hbox1), item, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS(item, GTK_CAN_DEFAULT);
  gtk_widget_show(item);
  g_signal_connect(G_OBJECT(item), "clicked",
                   G_CALLBACK(ProgressBoxCancelEvent), NULL);
  item = NULL;
  hbox1 = NULL;
  vbox1 = NULL;
} // END ProgressBoxDisplay()
