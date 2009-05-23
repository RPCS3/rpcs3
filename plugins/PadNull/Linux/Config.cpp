/*  FWnull
 *  Copyright (C) 2004-2009 PCSX2 Team
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <string>
using namespace std;

#include "Pad.h"
#include "Config.h"

 #ifdef __cplusplus
extern "C"
{
#endif

#include "support.h"
#include "callbacks.h"
#include "interface.h"

#ifdef __cplusplus
}
#endif

GtkWidget *MsgDlg, *About, *Conf;
extern string s_strIniPath;

void OnMsg_Ok() 
{
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

void SysMessage(char *fmt, ...) 
{
	GtkWidget *Ok,*Txt;
	GtkWidget *Box,*Box1;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg) - 1] == '\n') msg[strlen(msg)-1] = 0;

	MsgDlg = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "FireWire Msg");
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
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnMsg_Ok), NULL);
	gtk_container_add(GTK_CONTAINER(Box1), Ok);
	GTK_WIDGET_SET_FLAGS(Ok, GTK_CAN_DEFAULT);
	gtk_widget_show(Ok);

	gtk_widget_show(MsgDlg);	

	gtk_main();
}
 
void OnAbout_Ok(GtkButton *button, gpointer user_data) 
{
	gtk_widget_destroy(About);
	gtk_main_quit();
}

EXPORT_C_(void) PADabout() 
{
	About = create_About();
	gtk_widget_show_all(About);
	gtk_main();
}

void OnConf_Ok(GtkButton *button, gpointer user_data) 
{
	conf.Log = is_checked(Conf, "check_logging");
	SaveConfig();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel(GtkButton *button, gpointer user_data) 
{
	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

EXPORT_C_(void) PADconfigure() 
{
	LoadConfig();
	Conf = create_Config();

	set_checked(Conf, "check_logging", conf.Log);
	gtk_widget_show_all(Conf);
	gtk_main();
}

void LoadConfig() 
{
	FILE *f;
	char cfg[255];

	strcpy(cfg, s_strIniPath.c_str());
	f = fopen(cfg, "r");
	if (f == NULL) 
	{
		printf("failed to open %s\n", s_strIniPath.c_str());
		SaveConfig();//save and return
		return;
	}
	fscanf(f, "logging = %hhx\n", &conf.Log);
	//fscanf(f, "options = %hhx\n", &confOptions);
	fclose(f);
}

void SaveConfig() 
{
	FILE *f;
	char cfg[255];

	strcpy(cfg, s_strIniPath.c_str());
	f = fopen(cfg,"w");
	if (f == NULL) 
	{
		printf("failed to open '%s'\n", s_strIniPath.c_str());
		return;
	}
	fprintf(f, "logging = %hhx\n", conf.Log);
	//fprintf(f, "options = %hhx\n", confOptions);
	fclose(f);
}

