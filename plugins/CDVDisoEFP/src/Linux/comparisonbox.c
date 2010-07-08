/*  comparisonbox.c
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#include <stddef.h> // NULL
#include <stdio.h> // sprintf()
#include <string.h> // strcpy()

#include <gtk/gtkbutton.h> // gtk_button_new_with_label()
#include <gtk/gtkcontainer.h> // gtk_container_add()
#include <gtk/gtkentry.h> // gtk_entry_new()
#include <gtk/gtkfilesel.h> // gtk_file_selection_set_filename()
#include <gtk/gtkhbbox.h> // gtk_hbutton_box_new()
#include <gtk/gtkhbox.h> // gtk_hbox_new()
#include <gtk/gtkhseparator.h> // gtk_hseparator_new()
#include <gtk/gtklabel.h> // gtk_label_new()
#include <gtk/gtkmain.h> // gtk_init(), gtk_main(), gtk_main_quit()
#include <gtk/gtkvbox.h> // gtk_vbox_new()
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h> // gtk_window_new()

#include "logfile.h"
#include "conf.h"
#include "isofile.h"
#include "isocompress.h" // compressdesc[]
#include "multifile.h" // multinames[]
#include "tablerebuild.h" // IsoTableRebuild()
#include "progressbox.h"
#include "messagebox.h"


struct MainBoxData
{
	GtkWidget *window; // GtkWindow
	GtkWidget *file1; // GtkEntry
	GtkWidget *desc1; // GtkLabel
	GtkWidget *file2; // GtkEntry
	GtkWidget *desc2; // GtkLabel
	GtkWidget *okbutton; // GtkButton
	// Leaving the Cancel button unblocked... for emergency shutdown

	int stop; // Variable signal to stop long processes
};


struct MainBoxData mainbox;


void MainBoxDestroy()
{
	if (mainbox.window != NULL)
	{
		gtk_widget_destroy(mainbox.window);
		mainbox.window = NULL;
		mainbox.file1 = NULL;
		mainbox.desc1 = NULL;
		mainbox.file2 = NULL;
		mainbox.desc2 = NULL;
		mainbox.okbutton = NULL;
	} // ENDIF- Do we have a Main Window still?
} // END MainBoxDestroy()


void MainBoxUnfocus()
{
	gtk_widget_set_sensitive(mainbox.file1, FALSE);
	gtk_widget_set_sensitive(mainbox.file2, FALSE);
	gtk_widget_set_sensitive(mainbox.okbutton, FALSE);
	gtk_window_iconify(GTK_WINDOW(mainbox.window));
} // END MainBoxUnfocus()


gint MainBoxFile1Event(GtkWidget *widget, GdkEvent event, gpointer data)
{
	int returnval;
	char templine[256];
	struct IsoFile *tempfile;

	returnval = IsIsoFile(gtk_entry_get_text(GTK_ENTRY(mainbox.file1)));
	if (returnval == -1)
	{
		gtk_label_set_text(GTK_LABEL(mainbox.desc1), "File Type: ---");
		return(TRUE);
	} // ENDIF- Not a name of any sort?

	if (returnval == -2)
	{
		gtk_label_set_text(GTK_LABEL(mainbox.desc1), "File Type: Not a file");
		return(TRUE);
	} // ENDIF- Not a regular file?

	if (returnval == -3)
	{
		gtk_label_set_text(GTK_LABEL(mainbox.desc1), "File Type: Not a valid image file");
		return(TRUE);
	} // ENDIF- Not an Image file?

	if (returnval == -4)
	{
		gtk_label_set_text(GTK_LABEL(mainbox.desc1), "File Type: Missing Table File (will rebuild)");
		return(TRUE);
	} // ENDIF- Missing Compression seek table?

	tempfile = IsoFileOpenForRead(gtk_entry_get_text(GTK_ENTRY(mainbox.file1)));
	sprintf(templine, "File Type: %s%s%s",
	        multinames[tempfile->multi],
	        tempfile->imagename,
	        compressdesc[tempfile->compress]);
	gtk_label_set_text(GTK_LABEL(mainbox.desc1), templine);
	tempfile = IsoFileClose(tempfile);
	return(TRUE);
} // END MainBoxFileEvent()


gint MainBoxFile2Event(GtkWidget *widget, GdkEvent event, gpointer data)
{
	int returnval;
	char templine[256];
	struct IsoFile *tempfile;

	returnval = IsIsoFile(gtk_entry_get_text(GTK_ENTRY(mainbox.file2)));
	if (returnval == -1)
	{
		gtk_label_set_text(GTK_LABEL(mainbox.desc2), "File Type: ---");
		return(TRUE);
	} // ENDIF- Not a name of any sort?

	if (returnval == -2)
	{
		gtk_label_set_text(GTK_LABEL(mainbox.desc2), "File Type: Not a file");
		return(TRUE);
	} // ENDIF- Not a regular file?

	if (returnval == -3)
	{
		gtk_label_set_text(GTK_LABEL(mainbox.desc2), "File Type: Not a valid image file");
		return(TRUE);
	} // ENDIF- Not an Image file?

	if (returnval == -4)
	{
		gtk_label_set_text(GTK_LABEL(mainbox.desc2), "File Type: Missing Table File (will rebuild)");
		return(TRUE);
	} // ENDIF- Missing Compression seek table?

	tempfile = IsoFileOpenForRead(gtk_entry_get_text(GTK_ENTRY(mainbox.file2)));
	sprintf(templine, "File Type: %s%s%s",
	        multinames[tempfile->multi],
	        tempfile->imagename,
	        compressdesc[tempfile->compress]);
	gtk_label_set_text(GTK_LABEL(mainbox.desc2), templine);
	tempfile = IsoFileClose(tempfile);
	return(TRUE);
} // END MainBoxFileEvent()


void MainBoxRefocus()
{
	GdkEvent event;

	MainBoxFile1Event(NULL, event, NULL);
	MainBoxFile2Event(NULL, event, NULL);

	gtk_widget_set_sensitive(mainbox.file1, TRUE);
	gtk_widget_set_sensitive(mainbox.file2, TRUE);
	gtk_widget_set_sensitive(mainbox.okbutton, TRUE);
	gtk_window_set_focus(GTK_WINDOW(mainbox.window), mainbox.file1);
	gtk_window_deiconify(GTK_WINDOW(mainbox.window));
} // END MainBoxRefocus()


gint MainBoxCancelEvent(GtkWidget *widget, GdkEvent event, gpointer data)
{
	mainbox.stop = 1; // Halt all long processess...

	MessageBoxDestroy();
	ProgressBoxDestroy();
	MainBoxDestroy();

	gtk_main_quit();
	return(TRUE);
} // END MainBoxCancelEvent()


gint MainBoxOKEvent(GtkWidget *widget, GdkEvent event, gpointer data)
{
	const char *tempisoname1;
	const char *tempisoname2;
	struct IsoFile *tempiso1;
	struct IsoFile *tempiso2;
	int stop;
	off64_t endsector;
	off64_t sector;
	int retval;
	char tempblock1[2448];
	char tempblock2[2448];
	int i;

	MainBoxUnfocus();

	tempisoname1 = gtk_entry_get_text(GTK_ENTRY(mainbox.file1));
	tempisoname2 = gtk_entry_get_text(GTK_ENTRY(mainbox.file2));
	tempiso1 = NULL;
	tempiso2 = NULL;

	tempiso1 = IsoFileOpenForRead(tempisoname1);
	if (tempiso1 == NULL)
	{
		MainBoxRefocus();
		MessageBoxShow("First file is not a Valid Image File.", 0);
		tempisoname1 = NULL;
		tempisoname2 = NULL;
		return(TRUE);
	} // ENDIF- Not an ISO file? Message and Stop here.

	tempiso2 = IsoFileOpenForRead(tempisoname2);
	if (tempiso2 == NULL)
	{
		MainBoxRefocus();
		MessageBoxShow("Second file is not a Valid Image File.", 0);
		tempiso1 = IsoFileClose(tempiso1);
		tempisoname1 = NULL;
		tempisoname2 = NULL;
		return(TRUE);
	} // ENDIF- Not an ISO file? Message and Stop here.

	if (tempiso1->blocksize != tempiso2->blocksize)
	{
		MainBoxRefocus();
		MessageBoxShow("Block sizes in Image files do not match.", 0);
		tempiso1 = IsoFileClose(tempiso1);
		tempiso2 = IsoFileClose(tempiso2);
		tempisoname1 = NULL;
		tempisoname2 = NULL;
		return(TRUE);
	} // ENDIF- Not an ISO file? Message and Stop here.

	if (tempiso1->multi == 1)
	{
		i = 0;
		while ((i < 10) &&
		        (IsoFileSeek(tempiso1, tempiso1->multisectorend[i] + 1) == 0))  i++;
		endsector = tempiso1->multisectorend[tempiso1->multiend];
	}
	else
	{
		endsector = tempiso1->filesectorsize;
	} // ENDIF- Get ending sector from multifile? (Or single file?)
	IsoFileSeek(tempiso1, 0);

	if (tempiso2->multi == 1)
	{
		i = 0;
		while ((i < 10) &&
		        (IsoFileSeek(tempiso2, tempiso2->multisectorend[i] + 1) == 0))  i++;
		sector = tempiso2->multisectorend[tempiso2->multiend];
	}
	else
	{
		sector = tempiso2->filesectorsize;
	} // ENDIF- Get ending sector from multifile? (Or single file?)
	IsoFileSeek(tempiso2, 0);
	if (sector != endsector)
	{
		MainBoxRefocus();
		MessageBoxShow("Number of blocks in Image files do not match.", 0);
		tempiso1 = IsoFileClose(tempiso1);
		tempiso2 = IsoFileClose(tempiso2);
		tempisoname1 = NULL;
		tempisoname2 = NULL;
		return(TRUE);
	} // ENDIF- Number of blocks don't match? Say so.

	sprintf(tempblock1, "%s == %s ?", tempisoname1, tempisoname2);
	ProgressBoxStart(tempblock1, endsector);

	stop = 0;
	mainbox.stop = 0;
	progressbox.stop = 0;
	while ((stop == 0) && (tempiso1->sectorpos < endsector))
	{
		retval = IsoFileRead(tempiso1, tempblock1);
		if (retval < 0)
		{
			MainBoxRefocus();
			MessageBoxShow("Trouble reading first file.", 0);
			stop = 1;
		}
		else
		{
			retval = IsoFileRead(tempiso2, tempblock2);
			if (retval < 0)
			{
				MainBoxRefocus();
				MessageBoxShow("Trouble reading second file.", 0);
				stop = 1;
			}
			else
			{
				i = 0;
				while ((i < tempiso1->blocksize) && (tempblock1[i] == tempblock2[i])) i++;
				if (i < tempiso1->blocksize)
				{
					MainBoxRefocus();
					MessageBoxShow("Trouble reading second file.", 0);
					stop = 1;
				} // ENDIF- Sectors don't match? Say so.
			} // ENDIF- Trouble reading second file?
		} // ENDIF- Trouble reading first file?

		ProgressBoxTick(tempiso1->sectorpos);
		while (gtk_events_pending())  gtk_main_iteration();

		if (mainbox.stop != 0)  stop = 2;
		if (progressbox.stop != 0)  stop = 2;
	} // ENDWHILE- Comparing two files... sector by sector

	if (stop == 0)
	{
		MainBoxRefocus();
		MessageBoxShow("Images Match.", 0);
	} // ENDIF- Everything checked out? Say so.
	tempiso1 = IsoFileClose(tempiso1);
	tempiso2 = IsoFileClose(tempiso2);
	tempisoname1 = NULL;
	tempisoname2 = NULL;
	return(TRUE);
} // END MainBoxOKEvent()


void MainBoxDisplay()
{
	GtkWidget *item;
	GtkWidget *hbox1;
	GtkWidget *vbox1;

	mainbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox.window), 5);
	gtk_window_set_title(GTK_WINDOW(mainbox.window), "CDVDisoEFP Comparsion");
	gtk_window_set_position(GTK_WINDOW(mainbox.window), GTK_WIN_POS_CENTER);
	// gtk_window_set_resizable(GTK_WINDOW(mainbox.window), FALSE);

	g_signal_connect(G_OBJECT(mainbox.window), "delete_event",
	                 G_CALLBACK(MainBoxCancelEvent), NULL);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(mainbox.window), vbox1);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
	gtk_widget_show(vbox1);

	hbox1 = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
	gtk_widget_show(hbox1);

	item = gtk_label_new("First Iso File:");
	gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
	gtk_widget_show(item);
	item = NULL;

	mainbox.file1 = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox1), mainbox.file1, TRUE, TRUE, 0);
	gtk_widget_show(mainbox.file1);
	g_signal_connect(G_OBJECT(mainbox.file1), "changed",
	                 G_CALLBACK(MainBoxFile1Event), NULL);
	hbox1 = NULL;

	mainbox.desc1 = gtk_label_new("File Type: ---");
	gtk_box_pack_start(GTK_BOX(vbox1), mainbox.desc1, FALSE, FALSE, 0);
	gtk_widget_show(mainbox.desc1);

	item = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1), item, TRUE, TRUE, 0);
	gtk_widget_show(item);
	item = NULL;

	hbox1 = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
	gtk_widget_show(hbox1);

	item = gtk_label_new("Second Iso File:");
	gtk_box_pack_start(GTK_BOX(hbox1), item, FALSE, FALSE, 0);
	gtk_widget_show(item);
	item = NULL;

	mainbox.file2 = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox1), mainbox.file2, TRUE, TRUE, 0);
	gtk_widget_show(mainbox.file2);
	g_signal_connect(G_OBJECT(mainbox.file2), "changed",
	                 G_CALLBACK(MainBoxFile2Event), NULL);
	hbox1 = NULL;

	mainbox.desc2 = gtk_label_new("File Type: ---");
	gtk_box_pack_start(GTK_BOX(vbox1), mainbox.desc2, FALSE, FALSE, 0);
	gtk_widget_show(mainbox.desc2);

	hbox1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
	gtk_widget_show(hbox1);

	mainbox.okbutton = gtk_button_new_with_label("Compare");
	gtk_box_pack_start(GTK_BOX(hbox1), mainbox.okbutton, TRUE, TRUE, 0);
	gtk_widget_show(mainbox.okbutton);
	g_signal_connect(G_OBJECT(mainbox.okbutton), "clicked",
	                 G_CALLBACK(MainBoxOKEvent), NULL);

	item = gtk_button_new_with_label("Exit");
	gtk_box_pack_start(GTK_BOX(hbox1), item, TRUE, TRUE, 0);
	gtk_widget_show(item);
	g_signal_connect(G_OBJECT(item), "clicked",
	                 G_CALLBACK(MainBoxCancelEvent), NULL);
	item = NULL;
	hbox1 = NULL;
	vbox1 = NULL;

	// We held off setting the name until now... so description would show.
	gtk_entry_set_text(GTK_ENTRY(mainbox.file1), conf.isoname);
	gtk_entry_set_text(GTK_ENTRY(mainbox.file2), conf.isoname);
} // END MainBoxDisplay()


int main(int argc, char *argv[])
{
	gtk_init(NULL, NULL);

	OpenLog();
	InitConf();
	LoadConf();
	MainBoxDisplay();
	ProgressBoxDisplay();
	MessageBoxDisplay();

	gtk_widget_show_all(mainbox.window);
	gtk_main();
	CloseLog();
	return(0);
} // END main()
