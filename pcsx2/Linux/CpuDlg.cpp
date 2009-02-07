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
 
void OnCpu_Ok(GtkButton *button, gpointer user_data)
{
	u32 newopts = 0;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_EERec"))))
		newopts |= PCSX2_EEREC;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU0rec"))))
		newopts |= PCSX2_VU0REC;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU1rec"))))
		newopts |= PCSX2_VU1REC;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_MTGS"))))
		newopts |= PCSX2_GSMULTITHREAD;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitNormal"))))
		newopts |= PCSX2_FRAMELIMIT_NORMAL;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitLimit"))))
		newopts |= PCSX2_FRAMELIMIT_LIMIT;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitFS"))))
		newopts |= PCSX2_FRAMELIMIT_SKIP;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_VUSkip"))))
		newopts |= PCSX2_FRAMELIMIT_VUSKIP;

	Config.CustomFps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "CustomFPSLimit")));
	Config.CustomFrameSkip = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FrameThreshold")));
	Config.CustomConsecutiveFrames = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesBeforeSkipping")));
	Config.CustomConsecutiveSkip = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesToSkip")));

	if (Config.Options != newopts)
	{
		SysRestorableReset();

		if ((Config.Options&PCSX2_GSMULTITHREAD) ^(newopts&PCSX2_GSMULTITHREAD))
		{
			// Need the MTGS setting to take effect, so close out the plugins:
			PluginsResetGS();

			if (CHECK_MULTIGS)
				Console::Notice("MTGS mode disabled.\n\tEnjoy the fruits of single-threaded simpicity.");
			else
				Console::Notice("MTGS mode enabled.\n\tWelcome to multi-threaded awesomeness. And random crashes.");
		}

		Config.Options = newopts;
	}
	else
		UpdateVSyncRate();

	SaveConfig();

	gtk_widget_destroy(CpuDlg);
	if (MainWindow) gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnConf_Cpu(GtkMenuItem *menuitem, gpointer user_data)
{
	char str[512];

	CpuDlg = create_CpuDlg();
	gtk_window_set_title(GTK_WINDOW(CpuDlg), _("Configuration"));

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_EERec")), !!CHECK_EEREC);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU0rec")), !!CHECK_VU0REC);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU1rec")), !!CHECK_VU1REC);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_MTGS")), !!CHECK_MULTIGS);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitNormal")), CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_NORMAL);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitLimit")), CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_LIMIT);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitFS")), CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_SKIP);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_VUSkip")), CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_VUSKIP);

	sprintf(str, "Cpu Vendor:     %s", cpuinfo.x86ID);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_CpuVendor")), str);
	sprintf(str, "Familly:   %s", cpuinfo.x86Fam);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_Family")), str);
	sprintf(str, "Cpu Speed:   %d MHZ", cpuinfo.cpuspeed);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_CpuSpeed")), str);

	strcpy(str, "Features:    ");
	if (cpucaps.hasMultimediaExtensions) strcat(str, "MMX");
	if (cpucaps.hasStreamingSIMDExtensions) strcat(str, ",SSE");
	if (cpucaps.hasStreamingSIMD2Extensions) strcat(str, ",SSE2");
	if (cpucaps.hasStreamingSIMD3Extensions) strcat(str, ",SSE3");
	if (cpucaps.hasAMD64BitArchitecture) strcat(str, ",x86-64");
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_Features")), str);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "CustomFPSLimit")), (gdouble)Config.CustomFps);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FrameThreshold")), (gdouble)Config.CustomFrameSkip);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesBeforeSkipping")), (gdouble)Config.CustomConsecutiveFrames);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesToSkip")), (gdouble)Config.CustomConsecutiveSkip);

	gtk_widget_show_all(CpuDlg);
	if (MainWindow) gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}