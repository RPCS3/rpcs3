/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
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
 
 // Modified by arcum42@gmail.com

#include <gtk/gtk.h>

extern "C" {
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

#include "zerospu2.h"
#include "Linux.h"

extern char *libraryName;
extern string s_strIniPath;

// This is a bit ugly. I'll see if I can work out a better way to do this later.
int SetupSound()
{	
#ifdef ZEROSPU2_OSS
	return OSSSetupSound();
#else
	return AlsaSetupSound();
#endif
}
void RemoveSound()
{
#ifdef ZEROSPU2_OSS
	OSSRemoveSound();
#else
	AlsaRemoveSound();
#endif
}
int SoundGetBytesBuffered()
{
#ifdef ZEROSPU2_OSS
	return OSSSoundGetBytesBuffered();
#else
	return AlsaSoundGetBytesBuffered();
#endif
}
void SoundFeedVoiceData(unsigned char* pSound,long lBytes)
{
#ifdef ZEROSPU2_OSS
	OSSSoundFeedVoiceData(pSound, lBytes);
#else
	AlsaSoundFeedVoiceData(pSound, lBytes);
#endif
}

GtkWidget *MsgDlg, *ConfDlg;

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

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;
    
	MsgDlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "SPU2null Msg");
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

void CALLBACK SPU2configure() 
{
	char strcurdir[256];
	getcwd(strcurdir, 256);
	s_strIniPath = strcurdir;
	s_strIniPath += "/inis/zerospu2.ini";
	
	LOG_CALLBACK("SPU2configure()\n");
	ConfDlg = create_Config();
	LoadConfig();
	set_checked(ConfDlg, "timescalingbutton", (conf.options & OPTION_TIMESTRETCH));
	set_checked(ConfDlg, "realtimebutton", (conf.options & OPTION_REALTIME));
	set_checked(ConfDlg, "recordingbutton", (conf.options & OPTION_RECORDING));
	set_checked(ConfDlg, "mutebutton", (conf.options & OPTION_MUTE));
	set_checked(ConfDlg, "loggingbutton", (conf.Log));
	
	gtk_widget_show_all(ConfDlg);
	gtk_main();
}


void on_Conf_Ok (GtkButton *button, gpointer user_data)
{
	conf.options = 0;
	
	if (is_checked(ConfDlg, "realtimebutton")) 
		conf.options |= OPTION_REALTIME;
	if (is_checked(ConfDlg, "timescalingbutton")) 
		conf.options |= OPTION_TIMESTRETCH;
	if (is_checked(ConfDlg, "recordingbutton"))
		conf.options |= OPTION_RECORDING;
	if (is_checked(ConfDlg, "mutebutton"))
		conf.options |= OPTION_MUTE;
	
	conf.Log = is_checked(ConfDlg, "loggingbutton");
	
	SaveConfig();
	gtk_widget_destroy(ConfDlg);
	gtk_main_quit();
}


void on_Conf_Cancel (GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(ConfDlg);
	gtk_main_quit();

}

void CALLBACK SPU2about() 
{
	LOG_CALLBACK("SPU2about()\n");
	SysMessage("%s %d.%d\ndeveloper: zerofrog", libraryName, SPU2_VERSION, SPU2_BUILD);
}

void SaveConfig() 
{
	FILE *f;
	char cfg[255];

	strcpy(cfg, s_strIniPath.c_str());
	
	f = fopen(cfg,"w");
	if (f == NULL) 
	{
		ERROR_LOG("Failed to open %s\n", s_strIniPath.c_str());
		return;
	}
	
	fprintf(f, "log = %d\n", conf.Log);
	//fprintf(f, "options = %d\n", conf.options);
	
	fprintf(f, "realtime = %d\n", is_checked(ConfDlg, "realtimebutton"));
	fprintf(f, "timestretch = %d\n", is_checked(ConfDlg, "timescalingbutton"));
	fprintf(f, "recording = %d\n", is_checked(ConfDlg, "recordingbutton"));
	fprintf(f, "mute = %d\n", is_checked(ConfDlg, "mutebutton"));
	
	fclose(f);
}

void LoadConfig() 
{
	FILE *f;
	char cfg[255];
	int temp;

	memset(&conf, 0, sizeof(conf));

	strcpy(cfg, s_strIniPath.c_str());
	
	f = fopen(cfg, "r");
	if (f == NULL) 
	{
		ERROR_LOG("Failed to open %s\n", s_strIniPath.c_str());
		conf.Log = 0;
		conf.options = 0;
		SaveConfig();//save and return
		return;
	}
	
	fscanf(f, "log = %d\n", &conf.Log);
	
	fscanf(f, "realtime = %d\n", &temp);
	if (temp) conf.options |= OPTION_REALTIME;
	
	fscanf(f, "timestretch = %d\n", &temp);
	if (temp) conf.options |= OPTION_TIMESTRETCH;
	
	fscanf(f, "recording = %d\n", &temp);
	if (temp) conf.options |= OPTION_RECORDING;
	
	fscanf(f, "mute = %d\n", &temp);
	if (temp) conf.options |= OPTION_MUTE;
	
	fscanf(f, "options = %d\n", &conf.options);
	
	fclose(f);
}
