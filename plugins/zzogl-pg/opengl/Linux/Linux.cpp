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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
					if (conf.options & GSOPTION_FULLSCREEN) 
						GSclose();
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
	GtkWidget *treeview;
	GtkTreeModel *treemodel;
	GtkTreeIter treeiter;
	gboolean treeoptval;

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
	conf.negaa = 0;

	conf.options = 0;
	conf.options |= is_checked(Conf, "checkAVI") ? GSOPTION_CAPTUREAVI : 0;
	conf.options |= is_checked(Conf, "checkWireframe") ? GSOPTION_WIREFRAME : 0;
	conf.options |= is_checked(Conf, "checkfullscreen") ? GSOPTION_FULLSCREEN : 0;
	conf.options |= is_checked(Conf, "checkwidescreen") ? GSOPTION_WIDESCREEN : 0;
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

void add_map_entry(u32 option, const char *key, const char *desc)
{
	confOpts.value = option;
	confOpts.desc = desc;
	mapConfOpts[key] = confOpts;
}

void CALLBACK GSconfigure()
{
	char descbuf[255];
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
	set_checked(Conf, "checkwidescreen", (conf.options&GSOPTION_WIDESCREEN)?1:0);
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
	add_map_entry(0x00000001, "00000001", "Tex Target checking - 00000001\nLego Racers");
	add_map_entry(0x00000002, "00000002", "Auto reset targs - 00000002\nShadow Hearts, Samurai Warriors.  Use when game is slow and toggling AA fixes it.");
	add_map_entry(0x00000010, "00000010", "No target resolves - 00000010\nStops all resolving of targets.  Try this first for really slow games. Dark Cloud 1");
	add_map_entry(0x00000020, "00000020", "Exact color testing - 00000020\nFixes overbright or shadow/black artifacts (Crash 'n Burn).");
	add_map_entry(0x00000040, "00000040", "No color clamping - 00000040\nSpeeds up games, but might be too bright or too dim.");
	add_map_entry(0x00000100, "00000100", "Alpha Fail hack - 00000100\nFor Sonic Unleashed, Shadow the Hedgehog, Ghost in the Shell. Remove vertical stripes or other coloring artefacts. Break Persona 4 and MGS3");
	add_map_entry(0x00000200, "00000200", "Disable depth updates - 00000200");
	add_map_entry(0x00000400, "00000400", "Resolve Hack #1 - 00000400\nKingdom Hearts.  Speeds some games.");
	add_map_entry(0x00000800, "00000800", "Resolve Hack #2 - 00000800\nShadow Hearts, Urbz. Destroy FFX");
	add_map_entry(0x00001000, "00001000", "No target CLUT - 00001000\nResident Evil 4, or foggy scenes.");
	add_map_entry(0x00002000, "00002000", "Disable stencil buffer - 00002000\nUsually safe to do for simple scenes. Harvest Moon");
	add_map_entry(0x00008000, "00008000", "No depth resolve - 00008000\nMight give z buffer artifacts.");
	add_map_entry(0x00010000, "00010000", "Full 16 bit resolution - 00010000\nUse when half the screen is missing.");
	add_map_entry(0x00020000, "00020000", "Resolve Hack #3 - 00020000\nNeopets");
	add_map_entry(0x00040000, "00040000", "Fast Update - 00040000\nOkami. Speeds some games. Needs for Sonic Unleashed");
	add_map_entry(0x00080000, "00080000", "Disable alpha testing - 00080000");
	add_map_entry(0x00100000, "00100000", "Enable Multiple RTs - 00100000");
	add_map_entry(0x01000000, "01000000", "Specular Highlights - 01000000\nMakes Xenosaga and Okage graphics faster by removing highlights");
	add_map_entry(0x02000000, "02000000", "Partial targets - 02000000");
	add_map_entry(0x04000000, "04000000", "Partial depth - 04000000");
	add_map_entry(0x10000000, "10000000", "Gust fix, made gustgame more clean and fast - 10000000");
	add_map_entry(0x20000000, "20000000", "No logarithmic Z, could decrease number of Z-artefacts - 20000000");
	add_map_entry(0x00000004, "00000004", "Interlace 2X - 00000004\nFixes 2x bigger screen (Gradius 3).");

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

void SysMessage(const char *fmt, ...) 
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
