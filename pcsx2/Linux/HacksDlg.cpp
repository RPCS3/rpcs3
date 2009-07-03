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
	set_checked(GameFixDlg, "check_DMAExec", (Config.GameFixes & FLAG_DMAExec));
	set_checked(GameFixDlg, "check_XGKick", (Config.GameFixes & FLAG_XGKick));
	
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
	Config.GameFixes |= is_checked(GameFixDlg, "check_DMAExec") ? FLAG_DMAExec : 0;
	Config.GameFixes |= is_checked(GameFixDlg, "check_XGKick") ? FLAG_XGKick : 0;

	SaveConfig();
	
	gtk_widget_destroy(GameFixDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void on_vu_slider_changed(GtkRange *range,  gpointer user_data)
{
	int i;
	
	i = gtk_range_get_value(range);
	 gtk_label_set_text(GTK_LABEL(lookup_widget(SpeedHacksDlg,"vu_cycle_stealing_label")),vu_stealing_labels[i]);
}

void on_ee_slider_changed(GtkRange *range,  gpointer user_data)
{
	int i;
	
	i = gtk_range_get_value(range);
	 gtk_label_set_text(GTK_LABEL(lookup_widget(SpeedHacksDlg,"ee_cycle_label")),ee_cycle_labels[i]);
}

void on_Speed_Hacks(GtkMenuItem *menuitem, gpointer user_data)
{
	SpeedHacksDlg = create_SpeedHacksDlg();
	GtkRange *vuScale = GTK_RANGE(lookup_widget(SpeedHacksDlg, "VUCycleHackScale"));
	GtkRange *eeScale = GTK_RANGE(lookup_widget(SpeedHacksDlg, "EECycleHackScale"));
	
	set_checked(SpeedHacksDlg, "check_iop_cycle_rate",			Config.Hacks.IOPCycleDouble);
	set_checked(SpeedHacksDlg, "check_wait_cycles_sync_hack",	Config.Hacks.WaitCycleExt);
	set_checked(SpeedHacksDlg, "check_intc_sync_hack",			Config.Hacks.INTCSTATSlow);
	set_checked(SpeedHacksDlg, "check_idle_loop_fastforward",	Config.Hacks.IdleLoopFF);
	set_checked(SpeedHacksDlg, "check_microvu_flag_hack",		Config.Hacks.vuFlagHack);
	set_checked(SpeedHacksDlg, "check_microvu_min_max_hack",	Config.Hacks.vuMinMax);

	gtk_range_set_value(vuScale, Config.Hacks.VUCycleSteal);	
	on_vu_slider_changed(vuScale,  NULL);
	gtk_range_set_value(eeScale, Config.Hacks.EECycleRate);
	on_ee_slider_changed(eeScale,  NULL);
	
	gtk_widget_show_all(SpeedHacksDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}



void on_Speed_Hack_OK(GtkButton *button, gpointer user_data)
{
	PcsxConfig::Hacks_t newhacks;
	newhacks.EECycleRate = 0;
	
	newhacks.IOPCycleDouble = is_checked(SpeedHacksDlg, "check_iop_cycle_rate");
	newhacks.WaitCycleExt = is_checked(SpeedHacksDlg, "check_wait_cycles_sync_hack");
	newhacks.INTCSTATSlow = is_checked(SpeedHacksDlg, "check_intc_sync_hack");
	newhacks.IdleLoopFF = is_checked(SpeedHacksDlg, "check_idle_loop_fastforward");
	newhacks.vuFlagHack  = is_checked(SpeedHacksDlg, "check_microvu_flag_hack");
	newhacks.vuMinMax = is_checked(SpeedHacksDlg, "check_microvu_min_max_hack");
	
	newhacks.VUCycleSteal = gtk_range_get_value(GTK_RANGE(lookup_widget(SpeedHacksDlg, "VUCycleHackScale")));	
	newhacks.EECycleRate = gtk_range_get_value(GTK_RANGE(lookup_widget(SpeedHacksDlg, "EECycleHackScale")));	
	
	if (memcmp(&newhacks, &Config.Hacks, sizeof(newhacks)))
	{
		SysRestorableReset();
		Config.Hacks = newhacks;
		SaveConfig();
	}
	
	gtk_widget_destroy(SpeedHacksDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}
