/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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
 
GtkWidget *GameFixDlg, *SpeedHacksDlg;
 
 void on_Game_Fixes(GtkMenuItem *menuitem, gpointer user_data)
{
	GameFixDlg = create_GameFixDlg();
	
	set_checked(GameFixDlg, "check_VU_Add_Sub", (Config.GameFixes & FLAG_VU_ADD_SUB));
	set_checked(GameFixDlg, "check_VU_Clip", (Config.GameFixes & FLAG_VU_CLIP));

	set_checked(GameFixDlg, "check_FPU_Compare", (Config.GameFixes & FLAG_FPU_Compare));
	set_checked(GameFixDlg, "check_FPU_Mul", (Config.GameFixes & FLAG_FPU_MUL));
	
	gtk_widget_show_all(GameFixDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void on_Game_Fix_OK(GtkButton *button, gpointer user_data)
{

	Config.GameFixes = 0;
	
	Config.GameFixes |= is_checked(GameFixDlg, "check_VU_Add_Sub") ? FLAG_VU_ADD_SUB : 0;
	Config.GameFixes |= is_checked(GameFixDlg, "check_VU_Clip") ? FLAG_VU_CLIP : 0;
	
	Config.GameFixes |= is_checked(GameFixDlg, "check_FPU_Compare") ? FLAG_FPU_Compare : 0;
	Config.GameFixes |= is_checked(GameFixDlg, "check_FPU_Mul") ? FLAG_FPU_MUL : 0;

	SaveConfig();
	
	gtk_widget_destroy(GameFixDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void on_Speed_Hacks(GtkMenuItem *menuitem, gpointer user_data)
{
	SpeedHacksDlg = create_SpeedHacksDlg();

	switch (CHECK_EE_CYCLERATE)
	{
		case 0:
			set_checked(SpeedHacksDlg, "check_default_cycle_rate", true);
			break;
		case 1:
			set_checked(SpeedHacksDlg, "check_1_5_cycle_rate", true);
			break;
		case 2:
			set_checked(SpeedHacksDlg, "check_2_cycle_rate", true);
			break;
		case 3:
			set_checked(SpeedHacksDlg, "check_3_cycle_rate", true);
			break;
		default:
			set_checked(SpeedHacksDlg, "check_default_cycle_rate", true);
			break;
	}

	set_checked(SpeedHacksDlg, "check_iop_cycle_rate", CHECK_IOP_CYCLERATE);
	set_checked(SpeedHacksDlg, "check_wait_cycles_sync_hack", CHECK_WAITCYCLE_HACK);
	set_checked(SpeedHacksDlg, "check_intc_sync_hack", CHECK_INTC_STAT_HACK);
	set_checked(SpeedHacksDlg, "check_ESC_hack", CHECK_ESCAPE_HACK);

	gtk_range_set_value(GTK_RANGE(lookup_widget(SpeedHacksDlg, "VUCycleHackScale")), Config.VUCycleHack);	
	gtk_widget_show_all(SpeedHacksDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void on_Speed_Hack_OK(GtkButton *button, gpointer user_data)
{
	Config.Hacks = 0;

	if is_checked(SpeedHacksDlg, "check_default_cycle_rate")
		Config.Hacks = 0;
	else if is_checked(SpeedHacksDlg, "check_1_5_cycle_rate")
		Config.Hacks = 1;
	else if is_checked(SpeedHacksDlg, "check_2_cycle_rate")
		Config.Hacks = 2;
	else if is_checked(SpeedHacksDlg, "check_3_cycle_rate")
		Config.Hacks = 3;

	Config.Hacks |= is_checked(SpeedHacksDlg, "check_iop_cycle_rate") << 3;
	Config.Hacks |= is_checked(SpeedHacksDlg, "check_wait_cycles_sync_hack") << 4;
	Config.Hacks |= is_checked(SpeedHacksDlg, "check_intc_sync_hack") << 5;
	Config.Hacks |= is_checked(SpeedHacksDlg, "check_ESC_hack") << 10;

	Config.VUCycleHack = gtk_range_get_value(GTK_RANGE(lookup_widget(SpeedHacksDlg, "VUCycleHackScale")));	
	SaveConfig();

	gtk_widget_destroy(SpeedHacksDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}
