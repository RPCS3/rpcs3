/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
 */
 
 #include "Linux.h"

GtkWidget *AboutDlg, *about_version , *about_authors, *about_greets;
 
 void OnHelp_Help()
{
}

void OnHelpAbout_Ok(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(AboutDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnHelp_About(GtkMenuItem *menuitem, gpointer user_data)
{
	char str[g_MaxPath];
	GtkWidget *Label;

	AboutDlg = create_AboutDlg();
	gtk_window_set_title(GTK_WINDOW(AboutDlg), _("About"));

	Label = lookup_widget(AboutDlg, "GtkAbout_LabelVersion");

	// Include the SVN revision
	if (SVN_REV != 0)
		sprintf(str, _("PCSX2 For Linux\nVersion %s %s\n"), PCSX2_VERSION, SVN_REV);
	else
		//Use this instead for a non-svn version
		sprintf(str, _("PCSX2 For Linux\nVersion %s\n"), PCSX2_VERSION);

	gtk_label_set_text(GTK_LABEL(Label), str);

	Label = lookup_widget(AboutDlg, "GtkAbout_LabelAuthors");
	gtk_label_set_text(GTK_LABEL(Label), _(LabelAuthors));

	Label = lookup_widget(AboutDlg, "GtkAbout_LabelGreets");
	gtk_label_set_text(GTK_LABEL(Label), _(LabelGreets));

	gtk_widget_show_all(AboutDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}