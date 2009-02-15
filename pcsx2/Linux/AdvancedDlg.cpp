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
 
GtkWidget *AdvDlg;
 
void setAdvancedOptions()
{
	if (!cpucaps.hasStreamingSIMD2Extensions)
	{
		// SSE1 cpus do not support Denormals Are Zero flag.

		Config.sseMXCSR &= ~FLAG_DENORMAL_ZERO;
		Config.sseVUMXCSR &= ~FLAG_DENORMAL_ZERO;
	}

	switch ((Config.sseMXCSR & 0x6000) >> 13)
	{
		case 0:
			set_checked(AdvDlg, "radio_EE_Round_Near", TRUE);
			break;
		case 1:
			set_checked(AdvDlg, "radio_EE_Round_Negative", TRUE);
			break;
		case 2:
			set_checked(AdvDlg, "radio_EE_Round_Positive", TRUE);
			break;
		case 3:
			set_checked(AdvDlg, "radio_EE_Round_Zero", TRUE);
			break;
	}

	switch ((Config.sseVUMXCSR & 0x6000) >> 13)
	{
		case 0:
			set_checked(AdvDlg, "radio_VU_Round_Near", TRUE);
			break;
		case 1:
			set_checked(AdvDlg, "radio_VU_Round_Negative", TRUE);
			break;
		case 2:
			set_checked(AdvDlg, "radio_VU_Round_Positive", TRUE);
			break;
		case 3:
			set_checked(AdvDlg, "radio_VU_Round_Zero", TRUE);
			break;
	}


	switch (Config.eeOptions)
	{
		case FLAG_EE_CLAMP_NONE:
			set_checked(AdvDlg, "radio_EE_Clamp_None", TRUE);
			break;
		case FLAG_EE_CLAMP_NORMAL:
			set_checked(AdvDlg, "radio_EE_Clamp_Normal", TRUE);
			break;
		case FLAG_EE_CLAMP_EXTRA_PRESERVE:
			set_checked(AdvDlg, "radio_EE_Clamp_Extra_Preserve", TRUE);
			break;
	}

	switch (Config.vuOptions)
	{
		case FLAG_VU_CLAMP_NONE:
			set_checked(AdvDlg, "radio_VU_Clamp_None", TRUE);
			break;
		case FLAG_VU_CLAMP_NORMAL:
			set_checked(AdvDlg, "radio_VU_Clamp_Normal", TRUE);
			break;
		case FLAG_VU_CLAMP_EXTRA:
			set_checked(AdvDlg, "radio_VU_Clamp_Extra", TRUE);
			break;
		case FLAG_VU_CLAMP_EXTRA_PRESERVE:
			set_checked(AdvDlg, "radio_VU_Clamp_Extra_Preserve", TRUE);
			break;
	}
	set_checked(AdvDlg, "check_EE_Flush_Zero", (Config.sseMXCSR & FLAG_FLUSH_ZERO) ? TRUE : FALSE);
	set_checked(AdvDlg, "check_EE_Denormal_Zero", (Config.sseMXCSR & FLAG_DENORMAL_ZERO) ? TRUE : FALSE);

	set_checked(AdvDlg, "check_VU_Flush_Zero", (Config.sseVUMXCSR & FLAG_FLUSH_ZERO) ? TRUE : FALSE);
	set_checked(AdvDlg, "check_VU_Denormal_Zero", (Config.sseVUMXCSR & FLAG_DENORMAL_ZERO) ? TRUE : FALSE);
}

void on_Advanced(GtkMenuItem *menuitem, gpointer user_data)
{
	AdvDlg = create_AdvDlg();

	setAdvancedOptions();

	gtk_widget_show_all(AdvDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void on_Advanced_Defaults(GtkButton *button, gpointer user_data)
{
	Config.sseMXCSR = DEFAULT_sseMXCSR;
	Config.sseVUMXCSR = DEFAULT_sseVUMXCSR;
	Config.eeOptions = DEFAULT_eeOptions;
	Config.vuOptions = DEFAULT_vuOptions;

	setAdvancedOptions();
}

void on_Advanced_OK(GtkButton *button, gpointer user_data)
{
	Config.sseMXCSR &= 0x1fbf;
	Config.sseVUMXCSR &= 0x1fbf;
	Config.eeOptions = 0;
	Config.vuOptions = 0;

	Config.sseMXCSR |= is_checked(AdvDlg, "radio_EE_Round_Near") ? FLAG_ROUND_NEAR : 0;
	Config.sseMXCSR |= is_checked(AdvDlg, "radio_EE_Round_Negative") ? FLAG_ROUND_NEGATIVE : 0;
	Config.sseMXCSR |= is_checked(AdvDlg, "radio_EE_Round_Positive") ? FLAG_ROUND_POSITIVE : 0;
	Config.sseMXCSR |= is_checked(AdvDlg, "radio_EE_Round_Zero") ? FLAG_ROUND_ZERO : 0;

	Config.sseMXCSR |= is_checked(AdvDlg, "check_EE_Denormal_Zero") ? FLAG_DENORMAL_ZERO : 0;
	Config.sseMXCSR |= is_checked(AdvDlg, "check_EE_Flush_Zero") ? FLAG_FLUSH_ZERO : 0;

	Config.sseVUMXCSR |= is_checked(AdvDlg, "radio_VU_Round_Near") ? FLAG_ROUND_NEAR : 0;
	Config.sseVUMXCSR |= is_checked(AdvDlg, "radio_VU_Round_Negative") ? FLAG_ROUND_NEGATIVE : 0;
	Config.sseVUMXCSR |= is_checked(AdvDlg, "radio_VU_Round_Positive") ? FLAG_ROUND_POSITIVE : 0;
	Config.sseVUMXCSR |= is_checked(AdvDlg, "radio_VU_Round_Zero") ? FLAG_ROUND_ZERO : 0;

	Config.sseVUMXCSR |= is_checked(AdvDlg, "check_VU_Denormal_Zero") ? FLAG_DENORMAL_ZERO : 0;
	Config.sseVUMXCSR |= is_checked(AdvDlg, "check_VU_Flush_Zero") ? FLAG_FLUSH_ZERO : 0;

	Config.eeOptions |= is_checked(AdvDlg, "radio_EE_Clamp_None") ? FLAG_EE_CLAMP_NONE : 0;
	Config.eeOptions |= is_checked(AdvDlg, "radio_EE_Clamp_Normal") ? FLAG_EE_CLAMP_NORMAL : 0;
	Config.eeOptions |= is_checked(AdvDlg, "radio_EE_Clamp_Extra_Preserve") ? FLAG_EE_CLAMP_EXTRA_PRESERVE : 0;

	Config.vuOptions |= is_checked(AdvDlg, "radio_VU_Clamp_None") ? FLAG_VU_CLAMP_NONE : 0;
	Config.vuOptions |= is_checked(AdvDlg, "radio_VU_Clamp_Normal") ? FLAG_VU_CLAMP_NORMAL : 0;
	Config.vuOptions |= is_checked(AdvDlg, "radio_VU_Clamp_Extra") ? FLAG_VU_CLAMP_EXTRA : 0;
	Config.vuOptions |= is_checked(AdvDlg, "radio_VU_Clamp_Extra_Preserve") ? FLAG_VU_CLAMP_EXTRA_PRESERVE : 0;

	SetCPUState(Config.sseMXCSR, Config.sseVUMXCSR);
	SaveConfig();

	gtk_widget_destroy(AdvDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}
