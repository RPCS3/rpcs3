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

#include "McdDlgs.h"

void OnConf_Memcards(GtkMenuItem *menuitem, gpointer user_data) 
{
	char file[g_MaxPath], card[g_MaxPath];
	DIR *dir;
	struct dirent *entry;     
	struct stat statinfo;
	GtkWidget *memcombo1, *memcombo2;
	int i = 0;

	MemDlg = create_MemDlg();
	memcombo1 = lookup_widget(MemDlg, "memcard1combo");
	memcombo2 = lookup_widget(MemDlg, "memcard2combo");
	
	getcwd(file, ARRAYSIZE(file)); /* store current dir */
	sprintf( card, "%s/%s", file, MEMCARDS_DIR );
	chdir(card); /* change dirs so that plugins can find their config file*/
	    
	if ((dir = opendir(card)) == NULL)
	{
		Console::Error("ERROR: Could not open directory %s\n", params card);
		return;
	}

	while ((entry = readdir(dir)) != NULL) 
	{
		stat(entry->d_name, &statinfo);
		
		if (S_ISREG(statinfo.st_mode)) 
		{
			char path[g_MaxPath];
			
			sprintf( path, "%s/%s", MEMCARDS_DIR, entry->d_name);
			gtk_combo_box_append_text(GTK_COMBO_BOX(memcombo1), entry->d_name);
			gtk_combo_box_append_text(GTK_COMBO_BOX(memcombo2), entry->d_name);
			
			if (strcmp(Config.Mcd1, path) == 0) 
				gtk_combo_box_set_active(GTK_COMBO_BOX(memcombo1), i);
			if (strcmp(Config.Mcd2, path) == 0) 
				gtk_combo_box_set_active(GTK_COMBO_BOX(memcombo2), i);
			
			i++;
		}
	}

	closedir(dir);

	chdir(file);
	gtk_widget_show_all(MemDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
	
}
	
void OnMemcards_Ok(GtkButton *button, gpointer user_data)
{
	
	strcpy(Config.Mcd1, MEMCARDS_DIR "/");
	strcpy(Config.Mcd2, MEMCARDS_DIR "/");
	
	strcat(Config.Mcd1, gtk_combo_box_get_active_text(GTK_COMBO_BOX(lookup_widget(MemDlg, "memcard1combo"))));
	strcat(Config.Mcd2, gtk_combo_box_get_active_text(GTK_COMBO_BOX(lookup_widget(MemDlg, "memcard2combo"))));
	
	SaveConfig();
	
	gtk_widget_destroy(MemDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}