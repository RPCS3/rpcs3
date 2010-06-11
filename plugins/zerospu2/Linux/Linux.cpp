/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
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

#include "Linux.h"
#include "zerospu2.h"

extern char *libraryName;

GtkWidget *ConfDlg;

void __forceinline SysMessage(const char *fmt, ...)
{
    va_list list;
    char msg[512];

    va_start(list, fmt);
    vsprintf(msg, fmt, list);
    va_end(list);

    if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

    GtkWidget *dialog;
    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_OK,
                                     "%s", msg);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

void SaveConfig()
{
	string iniFile( s_strIniPath + "zerospu2.ini" );

	FILE* f = fopen(iniFile.c_str(),"w");
	if (f == NULL)
	{
		ERROR_LOG("Failed to open %s\n", iniFile.c_str());
		return;
	}

	fprintf(f, "log = %d\n", conf.Log);
	//fprintf(f, "options = %d\n", conf.options);

	fprintf(f, "realtime = %d\n", (conf.options & OPTION_REALTIME));
	fprintf(f, "timestretch = %d\n", (conf.options & OPTION_TIMESTRETCH));
	fprintf(f, "recording = %d\n", (conf.options & OPTION_RECORDING));
	fprintf(f, "mute = %d\n", (conf.options & OPTION_MUTE));

	fclose(f);
}

void LoadConfig()
{
	int temp;

	memset(&conf, 0, sizeof(conf));

	string iniFile( s_strIniPath + "zerospu2.ini" );

	FILE* f = fopen(iniFile.c_str(), "r");
	if (f == NULL)
	{
		ERROR_LOG("Failed to open %s\n", iniFile.c_str());
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

void DisplayDialog()
{
    int return_value;

    GtkWidget *dialog;
    GtkWidget *main_box;
    GtkWidget *time_scaling_check, *real_time_check, *recording_check, *mute_check, *logging_check;

	LoadConfig();
	
    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons (
		"ZeroSPU2 Config",
		NULL, /* parent window*/
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
		NULL);

    time_scaling_check = gtk_check_button_new_with_label("Time Scaling (recommended)");
    gtk_widget_set_tooltip_text(time_scaling_check, "Slows down or speeds up sound with respect to the game's real speed.\nEnabling this produces higher quality sound with less cracking, but can reduce speed.");
    
    real_time_check = gtk_check_button_new_with_label("Real Time Mode");
    gtk_widget_set_tooltip_text(real_time_check, "Tries to reduce delays in music as much as possible.\nUse when a game is already fast, and needs sound tightly syncronized. (like in DDR, Guitar Hero, & Guitaroo Man)");
    
    recording_check = gtk_check_button_new_with_label("Recording");
    gtk_widget_set_tooltip_text(recording_check, "Saves the raw 16 bit stereo wave data to zerospu2.wav. Timed to ps2 time.");
    
    mute_check = gtk_check_button_new_with_label("Mute");
    gtk_widget_set_tooltip_text(mute_check, "ZeroSPU2 will not output sound (fast).");
    
    logging_check = gtk_check_button_new_with_label("Enable logging");
    gtk_widget_set_tooltip_text(logging_check, "For development use only.");

    main_box = gtk_vbox_new(false, 5);

	gtk_container_add(GTK_CONTAINER(main_box), time_scaling_check);
	gtk_container_add(GTK_CONTAINER(main_box), real_time_check);
	gtk_container_add(GTK_CONTAINER(main_box), recording_check);
	gtk_container_add(GTK_CONTAINER(main_box), mute_check);
	gtk_container_add(GTK_CONTAINER(main_box), logging_check);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_scaling_check), (conf.options & OPTION_TIMESTRETCH));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(real_time_check), (conf.options & OPTION_REALTIME));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(recording_check), (conf.options & OPTION_RECORDING));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mute_check), (conf.options & OPTION_MUTE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(logging_check), (conf.Log));

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_box);
    gtk_widget_show_all (dialog);

    return_value = gtk_dialog_run (GTK_DIALOG (dialog));

    if (return_value == GTK_RESPONSE_ACCEPT)
    {
		conf.options = 0;

		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(time_scaling_check)))
			conf.options |= OPTION_TIMESTRETCH;
			
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(real_time_check)))
			conf.options |= OPTION_REALTIME;
			
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recording_check)))
			conf.options |= OPTION_RECORDING;
			
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mute_check)))
			conf.options |= OPTION_MUTE;
			
    	conf.Log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(logging_check));
    	SaveConfig();
    }

    gtk_widget_destroy (dialog);
}

void CALLBACK SPU2configure()
{
	LOG_CALLBACK("SPU2configure()\n");
	DisplayDialog();
}

void CALLBACK SPU2about()
{
	LOG_CALLBACK("SPU2about()\n");
	SysMessage("%s %d.%d\ndeveloper: zerofrog", libraryName, SPU2_VERSION, SPU2_BUILD);
}

