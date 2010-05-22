/*  ZeroPAD - author: zerofrog(@gmail.com)
 *  Copyright (C) 2006-2007
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
 */

 #include <string.h>
#include <gtk/gtk.h>
#include <pthread.h>

#include "joystick.h"

#include "zeropad.h"
#include "linux.h"

extern "C"
{
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

extern string s_strIniPath;
GtkWidget *Conf = NULL, *About = NULL;
GtkWidget *s_devicecombo = NULL;
extern void UpdateConf(int pad);

void SaveConfig()
{
	int i, j;
	FILE *f;
	
	const std::string iniFile(s_strIniPath + "zeropad.ini");
	f = fopen(iniFile.c_str(), "w");
	if (f == NULL)
	{
		printf("ZeroPAD: failed to save ini %s\n", s_strIniPath.c_str());
		return;
	}

	for (j = 0; j < 2 * PADSUBKEYS; j++)
	{
		for (i = 0; i < PADKEYS; i++)
		{
			fprintf(f, "[%d][%d] = 0x%lx\n", j, i, conf.keys[j][i]);
		}
	}
	fprintf(f, "log = %d\n", conf.log);
	fprintf(f, "options = %d\n", conf.options);
	fclose(f);
}

void LoadConfig()
{
	FILE *f;
	char str[256];
	int i, j;

	memset(&conf, 0, sizeof(conf));
	conf.keys[0][0] = XK_a;			// L2
	conf.keys[0][1] = XK_semicolon;	// R2
	conf.keys[0][2] = XK_w;			// L1
	conf.keys[0][3] = XK_p;			// R1
	conf.keys[0][4] = XK_i;			// TRIANGLE
	conf.keys[0][5] = XK_l;			// CIRCLE
	conf.keys[0][6] = XK_k;			// CROSS
	conf.keys[0][7] = XK_j;			// SQUARE
	conf.keys[0][8] = XK_v;			// SELECT
	conf.keys[0][11] = XK_n; 			// START
	conf.keys[0][12] = XK_e;			// UP
	conf.keys[0][13] = XK_f;			// RIGHT
	conf.keys[0][14] = XK_d;			// DOWN
	conf.keys[0][15] = XK_s;			// LEFT
	conf.log = 0;

	const std::string iniFile(s_strIniPath + "zeropad.ini");
	f = fopen(iniFile.c_str(), "r");
	if (f == NULL)
	{
		printf("ZeroPAD: failed to load ini %s\n", s_strIniPath.c_str());
		SaveConfig(); //save and return
		return;
	}

	for (j = 0; j < 2 * PADSUBKEYS; j++)
	{
		for (i = 0; i < PADKEYS; i++)
		{
			sprintf(str, "[%d][%d] = 0x%%x\n", j, i);
			if (fscanf(f, str, &conf.keys[j][i]) == 0) conf.keys[j][i] = 0;
		}
	}
	fscanf(f, "log = %d\n", &conf.log);
	fscanf(f, "options = %d\n", &conf.options);
	fclose(f);
}

GtkWidget *MsgDlg;

void OnMsg_Ok()
{
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

// GUI event handlers
void on_joydevicescombo_changed(GtkComboBox *combobox, gpointer user_data)
{
	int joyid = gtk_combo_box_get_active(combobox);

	// unassign every joystick with this pad
	for (int i = 0; i < (int)s_vjoysticks.size(); ++i)
	{
		if (s_vjoysticks[i]->GetPAD() == s_selectedpad) s_vjoysticks[i]->Assign(-1);
	}

	if (joyid >= 0 && joyid < (int)s_vjoysticks.size()) s_vjoysticks[joyid]->Assign(s_selectedpad);
}

void on_checkbutton_reverselx_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	int mask = PADOPTION_REVERTLX << (16 * s_selectedpad);

	if (gtk_toggle_button_get_active(togglebutton))
		conf.options |= mask;
	else
		conf.options &= ~mask;
}

void on_checkbutton_reversely_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	int mask = PADOPTION_REVERTLY << (16 * s_selectedpad);

	if (gtk_toggle_button_get_active(togglebutton))
		conf.options |= mask;
	else
		conf.options &= ~mask;
}

void on_checkbutton_reverserx_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	int mask = PADOPTION_REVERTRX << (16 * s_selectedpad);
	if (gtk_toggle_button_get_active(togglebutton))
		conf.options |= mask;
	else
		conf.options &= ~mask;
}

void on_checkbutton_reversery_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	int mask = PADOPTION_REVERTRY << (16 * s_selectedpad);

	if (gtk_toggle_button_get_active(togglebutton))
		conf.options |= mask;
	else
		conf.options &= ~mask;
}

void on_forcefeedback_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	int mask = PADOPTION_REVERTLX << (16 * s_selectedpad);

	if (gtk_toggle_button_get_active(togglebutton))
	{
		conf.options |= mask;

		int joyid = gtk_combo_box_get_active(GTK_COMBO_BOX(s_devicecombo));

		if (joyid >= 0 && joyid < (int)s_vjoysticks.size()) s_vjoysticks[joyid]->TestForce();
	}
	else
	{
		conf.options &= ~mask;
	}
}

void SysMessage(char *fmt, ...)
{
	GtkWidget *Ok, *Txt;
	GtkWidget *Box, *Box1;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

	MsgDlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "ZeroPad");
	gtk_container_set_border_width(GTK_CONTAINER(MsgDlg), 5);

	Box = gtk_vbox_new(5, 0);
	gtk_container_add(GTK_CONTAINER(MsgDlg), Box);
	gtk_widget_show(Box);

	Txt = gtk_label_new(msg);

	gtk_box_pack_start(GTK_BOX(Box), Txt, FALSE, FALSE, 5);
	gtk_widget_show(Txt);

	Box1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(Box), Box1, FALSE, FALSE, 0);
	gtk_widget_show(Box1);

	Ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnMsg_Ok), NULL);
	gtk_container_add(GTK_CONTAINER(Box1), Ok);
	GTK_WIDGET_SET_FLAGS(Ok, GTK_CAN_DEFAULT);
	gtk_widget_show(Ok);

	gtk_widget_show(MsgDlg);

	gtk_main();
}

void OnConf_Pad1(GtkButton *button, gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) UpdateConf(0);
}

void OnConf_Pad2(GtkButton *button, gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) UpdateConf(1);
}

void OnConf_Pad3(GtkButton *button, gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) UpdateConf(2);
}

void OnConf_Pad4(GtkButton *button, gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) UpdateConf(3);
}

void OnConf_Ok(GtkButton *button, gpointer user_data)
{
//	conf.analog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Analog));
	SaveConfig();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(Conf);
	gtk_main_quit();
	LoadConfig(); // load previous config
}

void OnAbout_Ok(GtkDialog *About, gint response_id, gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(About));
	gtk_main_quit();
}

void CALLBACK PADabout()
{

	About = create_About();

	gtk_widget_show_all(About);
	gtk_main();
}

s32 CALLBACK PADtest()
{
	return 0;
}
