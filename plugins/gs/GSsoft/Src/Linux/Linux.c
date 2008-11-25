/*  GSsoft 
 *  Copyright (C) 2002-2004  GSsoft Team
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <dlfcn.h>

#include "GS.h"
#include "interface.h"
#include "support.h"
#include "Linux.h"
#include "Rec.h"

GtkWidget *MsgDlg;

void OnMsg_Ok() {
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

void SysMessage(char *fmt, ...) {
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
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "GSsoft Msg");
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


void CALLBACK GSkeyEvent(keyEvent *ev) {
	switch (ev->event) {
		case KEYPRESS:
			switch (ev->key) {
				case XK_Page_Up:
					if (conf.fps) fpspos++; break;
				case XK_Page_Down:
					if (conf.fps) fpspos--; break;
				case XK_End:
					if (conf.fps) fpspress = 1; break;
				case XK_Delete:
					conf.fps = 1 - conf.fps;
					break;
			}
			break;
	}
}

GtkWidget *Conf;
GtkWidget *Logging;
GtkWidget *ComboFRes;
GtkWidget *ComboWRes;
GtkWidget *ComboCacheSize;
GtkWidget *ComboCodec;
GtkWidget *ComboFilters;
GList *fresl;
GList *wresl;
GList *cachesizel;
GList *codecl;
GList *filtersl;

void OnConf_Ok() {
	GtkWidget *Btn;
	char *str;
	int i;

	Btn = lookup_widget(Conf, "GtkCheckButton_Fullscreen");
	conf.fullscreen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));

	str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ComboFRes)->entry));
	sscanf(str, "%dx%d", &conf.fmode.width, &conf.fmode.height);

	str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ComboWRes)->entry));
	sscanf(str, "%dx%d", &conf.wmode.width, &conf.wmode.height);

	Btn = lookup_widget(Conf, "GtkCheckButton_Fps");
	conf.fps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));

	Btn = lookup_widget(Conf, "GtkCheckButton_Frameskip");
	conf.frameskip = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));

	Btn = lookup_widget(Conf, "GtkCheckButton_Record");
	conf.record = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));

	Btn = lookup_widget(Conf, "GtkCheckButton_Cache");
	conf.cache = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));

	str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ComboCacheSize)->entry));
	sscanf(str, "%d", &conf.cachesize);

	str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ComboCodec)->entry));
	for (i=0; ; i++) {
		if (codeclist[i] == NULL) break;
		if (strcmp(str, codeclist[i]) == 0) { conf.codec = i; break; }
	}

	str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ComboFilters)->entry));
	for (i=0; ; i++) {
		if (filterslist[i] == NULL) break;
		if (strcmp(str, filterslist[i]) == 0) { conf.filter = i; break; }
	}

	SaveConfig();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel() {
	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Logging() {
#ifdef GS_LOG
	GtkWidget *Btn;

	Logging = create_Logging();

	Btn = lookup_widget(Logging, "GtkCheckButton_Log");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), conf.log);

	gtk_widget_show_all(Logging);
	gtk_main();
#endif
}

void OnLogging_Ok() {
#ifdef GS_LOG
	GtkWidget *Btn;

	Btn = lookup_widget(Logging, "GtkCheckButton_Log");
	conf.log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));

	SaveConfig();

	gtk_widget_destroy(Logging);
	gtk_main_quit();
#endif
}

void OnLogging_Cancel() {
	gtk_widget_destroy(Logging);
	gtk_main_quit();
}

char mname[64][32];

void CALLBACK GSconfigure() {
	GtkWidget *Btn;
	char name[32];
	int nmodes, i;

	LoadConfig();

	Conf = create_Config();

	fresl = NULL;
	ComboFRes = lookup_widget(Conf, "GtkCombo_FRes");
	nmodes = DXgetModes();
	for (i=0; i<nmodes; i++) {
		sprintf(mname[i], "%dx%d", modes[i].width, modes[i].height);
		fresl = g_list_append(fresl, mname[i]);	
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(ComboFRes), fresl);
	sprintf(name, "%dx%d", conf.fmode.width, conf.fmode.height);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ComboFRes)->entry), name);

	wresl = NULL;
	ComboWRes = lookup_widget(Conf, "GtkCombo_WRes");
	wresl = g_list_append(wresl, "320x240");
	wresl = g_list_append(wresl, "512x384");
	wresl = g_list_append(wresl, "640x480");
	wresl = g_list_append(wresl, "800x600");
	gtk_combo_set_popdown_strings(GTK_COMBO(ComboWRes), wresl);
	sprintf(name, "%dx%d", conf.wmode.width, conf.wmode.height);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ComboWRes)->entry), name);

	Btn = lookup_widget(Conf, "GtkCheckButton_Fullscreen");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), conf.fullscreen);

	Btn = lookup_widget(Conf, "GtkCheckButton_Fps");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), conf.fps);

	Btn = lookup_widget(Conf, "GtkCheckButton_Frameskip");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), conf.frameskip);

	Btn = lookup_widget(Conf, "GtkCheckButton_Record");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), conf.record);

	Btn = lookup_widget(Conf, "GtkCheckButton_Cache");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), conf.cache);

	cachesizel = NULL;
	ComboCacheSize = lookup_widget(Conf, "GtkCombo_CacheSize");
	cachesizel = g_list_append(cachesizel, "64");	
	cachesizel = g_list_append(cachesizel, "128");	
	cachesizel = g_list_append(cachesizel, "256");	
	gtk_combo_set_popdown_strings(GTK_COMBO(ComboCacheSize), cachesizel);
	sprintf(name, "%d", conf.cachesize);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ComboCacheSize)->entry), name);

	codecl = NULL;
	ComboCodec = lookup_widget(Conf, "GtkCombo_Codec");
	for (i=0; ; i++) {
		if (codeclist[i] == NULL) break;
		codecl = g_list_append(codecl, codeclist[i]);	
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(ComboCodec), codecl);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ComboCodec)->entry), codeclist[conf.codec]);

	if (recExist() == -1) {
		Btn = lookup_widget(Conf, "GtkFrame_Rec");
		gtk_widget_set_sensitive(Btn, FALSE);
	}

	filtersl = NULL;
	ComboFilters = lookup_widget(Conf, "GtkCombo_Filters");
	for (i=0; ; i++) {
		if (filterslist[i] == NULL) break;
		filtersl = g_list_append(filtersl, filterslist[i]);	
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(ComboFilters), filtersl);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ComboFilters)->entry), filterslist[conf.filter]);

#ifndef GS_LOG
	Btn = lookup_widget(Conf, "GtkButton_Logging");
	gtk_widget_set_sensitive(Btn, FALSE);
#endif

	gtk_widget_show_all(Conf);
	gtk_main();
}

GtkWidget *About;

void OnAbout_Ok() {
	gtk_widget_destroy(About);
	gtk_main_quit();
}

void CALLBACK GSabout() {

	About = create_About();

	gtk_widget_show_all(About);
	gtk_main();
}

s32 CALLBACK GStest() {
	return 0;
}

void *SysLoadLibrary(char *lib) {
	return dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
}

void *SysLoadSym(void *lib, char *sym) {
	void *ret = dlsym(lib, sym);
	if (ret == NULL) printf("null: %s\n", sym);
	return dlsym(lib, sym);
}

char *SysLibError() {
	return dlerror();
}

void SysCloseLibrary(void *lib) {
	dlclose(lib);
}

