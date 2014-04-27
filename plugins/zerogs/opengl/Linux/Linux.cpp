/*  ZeroGS
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <dlfcn.h>

#include "GS.h"

extern "C" {
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

#include "Linux.h"
#include "zerogs.h"

#include <map>

GtkWidget *Conf, *Logging, *About;
GList *fresl, *wresl, *cachesizel, *codecl, *filtersl;

static int prevbilinearfilter;

struct confOptsStruct
{
	int value;
	const char *desc;
} confOpts;

static map<string, confOptsStruct> mapConfOpts;

extern void OnKeyboardF5(int);
extern void OnKeyboardF6(int);
extern void OnKeyboardF7(int);
extern void OnKeyboardF9(int);

void CALLBACK GSkeyEvent(keyEvent *ev)
{
	//static bool bShift = false;
	static bool bAlt = false;

	switch(ev->evt) {
		case KEYPRESS:
			switch(ev->key) {
				case XK_F5:
				case XK_F6:
				case XK_F7:
				case XK_F9:
					THR_KeyEvent = ev->key ;
					break;
				case XK_Escape:
					break;
				case XK_Shift_L:
				case XK_Shift_R:
					//bShift = true;
					THR_bShift = true;
					break;
				case XK_Alt_L:
				case XK_Alt_R:
					bAlt = true;
					break;
				}
			break;
		case KEYRELEASE:
			switch(ev->key) {
				case XK_Shift_L:
				case XK_Shift_R:
					//bShift = false;
					THR_bShift = false;
					break;
				case XK_Alt_L:
				case XK_Alt_R:
					bAlt = false;
					break;
			}
	}
}

void OnConf_Ok(GtkButton *button, gpointer user_data)
{
	GtkWidget *Btn;
	GtkWidget *treeview;
	GtkTreeModel *treemodel;
	GtkTreeIter treeiter;
	gboolean treeoptval;
	gchar *gbuf;
	char *str;
	int i;

	conf.bilinear = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkBilinear")));
	// restore
	if (conf.bilinear && prevbilinearfilter)
		conf.bilinear = prevbilinearfilter;

	//conf.mrtdepth = 1;//IsDlgButtonChecked(hW, IDC_CONFIG_DEPTHWRITE);

	if is_checked(Conf, "radioAANone")
		conf.aa = 0;
	else if is_checked(Conf, "radioAA2X")
		conf.aa = 1;
	else if is_checked(Conf, "radioAA4X")
		conf.aa = 2;
	else if is_checked(Conf, "radioAA8X")
		conf.aa = 3;
	else
		conf.aa = 4;

	conf.options = 0;
	conf.options |= is_checked(Conf, "checkAVI") ? GSOPTION_CAPTUREAVI : 0;
	conf.options |= is_checked(Conf, "checkWireframe") ? GSOPTION_WIREFRAME : 0;
	conf.options |= is_checked(Conf, "checkfullscreen") ? GSOPTION_FULLSCREEN : 0;
	conf.options |= is_checked(Conf, "checkTGA") ? GSOPTION_TGASNAP : 0;

	if is_checked(Conf, "radiointerlace0")
		conf.interlace = 0;
	else if is_checked(Conf, "radiointerlace1")
		conf.interlace = 1;
	else
		conf.interlace = 2;

	//------- get advanced options from the treeview model -------//
	treeview = lookup_widget(Conf,"treeview1");
	treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gtk_tree_model_get_iter_first(treemodel, &treeiter);

	conf.gamesettings = 0;
	for(map<string, confOptsStruct>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it)
	{
		treeoptval = FALSE;
		gtk_tree_model_get(treemodel, &treeiter, 0, &treeoptval, -1);

		if(treeoptval) conf.gamesettings |= it->second.value;

		gtk_tree_model_iter_next(treemodel,&treeiter);
	}

	GSsetGameCRC(0, conf.gamesettings);
	//---------- done getting advanced options ---------//

	if is_checked(Conf, "radioSize640")
		conf.options |= GSOPTION_WIN640;
	else if is_checked(Conf, "radioSize800")
		conf.options |= GSOPTION_WIN800;
	else if is_checked(Conf, "radioSize1024")
		conf.options |= GSOPTION_WIN1024;
	else if is_checked(Conf, "radioSize1280")
		conf.options |= GSOPTION_WIN1280;

	SaveConfig();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void CALLBACK GSconfigure()
{
	char name[32];
	char descbuf[255];
	int nmodes, i;
	bool itval;
	GtkWidget *treeview;
	GtkCellRenderer *treerend;
	GtkListStore *treestore;//Gets typecast as GtkTreeModel as needed.
	GtkTreeIter treeiter;
	GtkTreeViewColumn *treecol;

	char strcurdir[256];
	getcwd(strcurdir, 256);

	if (!(conf.options & GSOPTION_LOADED)) LoadConfig();
	Conf = create_Config();

	// fixme; Need to check "checkInterlace" as well.
	if (conf.interlace == 0)
		set_checked(Conf, "radiointerlace0", true);
	else if (conf.interlace == 1)
		set_checked(Conf, "radiointerlace1", true);
	else
		set_checked(Conf, "radionointerlace", true);

	set_checked(Conf, "checkBilinear", !!conf.bilinear);
	//set_checked(Conf, "checkbutton6", conf.mrtdepth);
	set_checked(Conf, "radioAANone", (conf.aa==0));
	set_checked(Conf, "radioAA2X",     (conf.aa==1));
	set_checked(Conf, "radioAA4X",     (conf.aa==2));
	set_checked(Conf, "radioAA8X",     (conf.aa==3));
	set_checked(Conf, "radioAA16X",   (conf.aa==4));
	set_checked(Conf, "checkWireframe", (conf.options&GSOPTION_WIREFRAME)?1:0);
	set_checked(Conf, "checkAVI", (conf.options&GSOPTION_CAPTUREAVI)?1:0);
	set_checked(Conf, "checkfullscreen", (conf.options&GSOPTION_FULLSCREEN)?1:0);
	set_checked(Conf, "checkTGA", (conf.options&GSOPTION_TGASNAP)?1:0);

	set_checked(Conf, "radioSize640", ((conf.options&GSOPTION_WINDIMS)>>4)==0);
	set_checked(Conf, "radioSize800", ((conf.options&GSOPTION_WINDIMS)>>4)==1);
	set_checked(Conf, "radioSize1024", ((conf.options&GSOPTION_WINDIMS)>>4)==2);
	set_checked(Conf, "radioSize1280", ((conf.options&GSOPTION_WINDIMS)>>4)==3);

	prevbilinearfilter = conf.bilinear;

	//--------- Let's build a treeview for our advanced options! --------//
	treeview = lookup_widget(Conf,"treeview1");
	treestore = gtk_list_store_new(2,G_TYPE_BOOLEAN, G_TYPE_STRING);

	//setup columns in treeview
	//COLUMN 0 is the checkboxes
	treecol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(treecol, "Select");
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), treecol);
	treerend = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(treecol, treerend, TRUE);
	gtk_tree_view_column_add_attribute(treecol, treerend, "active", 0);//link 'active' attrib to first column of model
	g_object_set(treerend, "activatable", TRUE, NULL);//set 'activatable' attrib true by default for all rows regardless of model.
	g_signal_connect(treerend, "toggled", (GCallback) OnToggle_advopts, treestore);//set a global callback, we also pass a reference to our treestore.

	//COLUMN 1 is the text descriptions
	treecol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(treecol, "Description");
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), treecol);
	treerend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(treecol, treerend, TRUE);
	gtk_tree_view_column_add_attribute(treecol, treerend, "text", 1);//link 'text' attrib to second column of model

	//setup the model with all our rows of option data
	mapConfOpts.clear();
	confOpts.value = 0x00000001;
	confOpts.desc = "Tex Target checking - 00000001\nLego Racers";
	mapConfOpts["00000001"] = confOpts;
	confOpts.value = 0x00000002;
	confOpts.desc = "Auto reset targs - 00000002\nShadow Hearts, Samurai Warriors.  Use when game is slow and toggling AA fixes it.";
	mapConfOpts["00000002"] = confOpts;
	confOpts.value = 0x00000004;
	confOpts.desc = "Interlace 2X - 00000004\nFixes 2x bigger screen (Gradius 3).";
	mapConfOpts["00000004"] = confOpts;
	confOpts.value = 0x00000008;
	confOpts.desc = "Text Alpha hack - 00000008\nNightmare Before Christmas.";
	mapConfOpts["00000008"] = confOpts;
	confOpts.value = 0x00000010;
	confOpts.desc = "No target resolves - 00000010\nStops all resolving of targets.  Try this first for really slow games.";
	mapConfOpts["00000010"] = confOpts;
	confOpts.value = 0x00000020;
	confOpts.desc = "Exact color testing - 00000020\nFixes overbright or shadow/black artifacts (Crash 'n Burn).";
	mapConfOpts["00000020"] = confOpts;
	confOpts.value = 0x00000040;
	confOpts.desc = "No color clamping - 00000040\nSpeeds up games, but might be too bright or too dim.";
	mapConfOpts["00000040"] = confOpts;
//	confOpts.value = 0x00000080;
//	confOpts.desc = "FFX hack - 00000080\nShows missing geometry.";
//	mapConfOpts["00000080"] = confOpts;
	confOpts.value = 0x00000200;
	confOpts.desc = "Disable depth updates - 00000200";
	mapConfOpts["00000200"] = confOpts;
	confOpts.value = 0x00000400;
	confOpts.desc = "Resolve Hack #1 - 00000400\nKingdom Hearts.  Speeds some games.";
	mapConfOpts["00000400"] = confOpts;
	confOpts.value = 0x00000800;
	confOpts.desc = "Resolve Hack #2 - 00000800\nShadow Hearts, Urbz.";
	mapConfOpts["00000800"] = confOpts;
	confOpts.value = 0x00001000;
	confOpts.desc = "No target CLUT - 00001000\nResident Evil 4, or foggy scenes.";
	mapConfOpts["00001000"] = confOpts;
	confOpts.value = 0x00002000;
	confOpts.desc = "Disable stencil buffer - 00002000\nUsually safe to do for simple scenes.";
	mapConfOpts["00002000"] = confOpts;
//	confOpts.value = 0x00004000;
//	confOpts.desc = "No vertical stripes - 00004000\nTry when there's a lot of garbage on screen.";
//	mapConfOpts["00004000"] = confOpts;
	confOpts.value = 0x00008000;
	confOpts.desc = "No depth resolve - 00008000\nMight give z buffer artifacts.";
	mapConfOpts["00008000"] = confOpts;
	confOpts.value = 0x00010000;
	confOpts.desc = "Full 16 bit resolution - 00010000\nUse when half the screen is missing.";
	mapConfOpts["00010000"] = confOpts;
	confOpts.value = 0x00020000;
	confOpts.desc = "Resolve Hack #3 - 00020000\nNeopets";
	mapConfOpts["00020000"] = confOpts;
	confOpts.value = 0x00040000;
	confOpts.desc = "Fast Update - 00040000\nOkami.  Speeds some games.";
	mapConfOpts["00040000"] = confOpts;
	confOpts.value = 0x00080000;
	confOpts.desc = "Disable alpha testing - 00080000";
	mapConfOpts["00080000"] = confOpts;
	confOpts.value = 0x00100000;
	confOpts.desc = "Disable Multiple RTs - 00100000";
	mapConfOpts["00100000"] = confOpts;
	confOpts.value = 0x00200000;
	confOpts.desc = "32 bit render targets - 00200000";
	mapConfOpts["00200000"] = confOpts;

	confOpts.value = 0x00400000;
	confOpts.desc = "Path 3 Hack - 00400000";
	mapConfOpts["00400000"] = confOpts;
	confOpts.value = 0x00800000;
	confOpts.desc = "Parallelize Contexts - 00800000 (Might speed things up, xenosaga is faster)";
	mapConfOpts["00800000"] = confOpts;
	confOpts.value = 0x01000000;
	confOpts.desc = "Specular Highlights - 01000000\nMakes xenosaga graphics faster by removing highlights";
	mapConfOpts["01000000"] = confOpts;

	for(map<string, confOptsStruct>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it)
	{
		gtk_list_store_append(treestore, &treeiter);//new row
		itval = (conf.gamesettings&it->second.value)?TRUE:FALSE;
		snprintf(descbuf, 254, "%s", it->second.desc);
		gtk_list_store_set(treestore, &treeiter, 0, itval, 1, descbuf, -1);
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(treestore));//NB: store is cast as tree model.
	g_object_unref(treestore);//allow model to be destroyed when the tree is destroyed.

	//don't select/highlight rows
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_NONE);
	//------treeview done -------//

	//Let's do it!
	gtk_widget_show_all(Conf);
	gtk_main();
}

void OnToggle_advopts(GtkCellRendererToggle *cell, gchar *path, gpointer user_data)
{
	GtkTreeIter treeiter;
	gboolean val;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(user_data), &treeiter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(user_data), &treeiter, 0, &val, -1);
	val = !val;
	gtk_list_store_set(GTK_LIST_STORE(user_data), &treeiter, 0, val, -1);

}

void OnAbout_Ok(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(About);
	gtk_main_quit();
}

void CALLBACK GSabout()
{
	About = create_About();

	gtk_widget_show_all(About);
	gtk_main();
}

s32 CALLBACK GStest()
{
	return 0;
}

GtkWidget *MsgDlg;

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

void *SysLoadLibrary(char *lib)
{
	return dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
}

void *SysLoadSym(void *lib, char *sym)
{
	void *ret = dlsym(lib, sym);
	if (ret == NULL) printf("null: %s\n", sym);
	return dlsym(lib, sym);
}

char *SysLibError()
{
	return dlerror();
}

void SysCloseLibrary(void *lib)
{
	dlclose(lib);
}
