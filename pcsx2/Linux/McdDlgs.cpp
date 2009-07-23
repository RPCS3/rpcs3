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
	string file;
	char card[g_MaxPath];
	DIR *dir;
	struct dirent *entry;     
	struct stat statinfo;
	GtkWidget *memcombo[2];
	int i = 0, j = 0;
	bool active[2] ={false, false};

	MemDlg = create_MemDlg();
	
	memcombo[0] = lookup_widget(MemDlg, "memcard1combo");
	memcombo[1] = lookup_widget(MemDlg, "memcard2combo");
	
	set_checked(MemDlg, "check_enable_mcd1", Config.Mcd[0].Enabled);
	set_checked(MemDlg, "check_enable_mcd2", Config.Mcd[1].Enabled);
	set_checked(MemDlg, "check_eject_mcds", Config.McdEnableEject);
	
	file = Path::GetCurrentDirectory();/* store current dir */
	sprintf(card, "%s/%s", file.c_str(), MEMCARDS_DIR );
	Path::ChangeDirectory(string(card));/* change dirs so that plugins can find their config file*/
	
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
			
			sprintf(path, "%s/%s/%s", MAIN_DIR, MEMCARDS_DIR, entry->d_name);
			
			for (j = 0; j < 2; j++)
			{
				gtk_combo_box_append_text(GTK_COMBO_BOX(memcombo[j]), entry->d_name);
				
				if (strcmp(Config.Mcd[j].Filename, path) == 0) 
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(memcombo[j]), i);
					active[j] = true;
				}
			}
			i++;
		}
	}
	
	closedir(dir);
	Path::ChangeDirectory(file);
	gtk_widget_show_all(MemDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
	
}
	
void OnMemcards_Ok(GtkButton *button, gpointer user_data)
{
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(lookup_widget(MemDlg, "memcard1combo"))) != -1)
		sprintf(Config.Mcd[0].Filename, "%s/%s/%s", MAIN_DIR, MEMCARDS_DIR, 
		gtk_combo_box_get_active_text(GTK_COMBO_BOX(lookup_widget(MemDlg, "memcard1combo"))));
	else
		sprintf(Config.Mcd[0].Filename, "%s/%s/%s", MAIN_DIR, MEMCARDS_DIR, DEFAULT_MEMCARD1);
	
	
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(lookup_widget(MemDlg, "memcard2combo"))) != -1)
		sprintf(Config.Mcd[1].Filename, "%s/%s/%s", MAIN_DIR, MEMCARDS_DIR, 
		gtk_combo_box_get_active_text(GTK_COMBO_BOX(lookup_widget(MemDlg, "memcard2combo"))));
	else
		sprintf(Config.Mcd[1].Filename, "%s/%s/%s", MAIN_DIR, MEMCARDS_DIR, DEFAULT_MEMCARD2);
	
	Config.Mcd[0].Enabled = is_checked(MemDlg, "check_enable_mcd1");
	Config.Mcd[1].Enabled = is_checked(MemDlg, "check_enable_mcd2");
	Config.McdEnableEject = is_checked(MemDlg, "check_eject_mcds");
	
	SaveConfig();
	
	gtk_widget_destroy(MemDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}