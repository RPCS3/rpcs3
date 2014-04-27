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
#include "Linux.h"
#include "GLWin.h"

#include <map>

extern u32 THR_KeyEvent; // value for passing out key events beetwen threads
extern bool THR_bShift;
extern bool THR_bCtrl;

static map<string, confOptsStruct> mapConfOpts;
static gameHacks tempHacks;

void CALLBACK GSkeyEvent(keyEvent *ev)
{
	static bool bAlt = false;

	switch (ev->evt)
	{
		case KEYPRESS:
			switch (ev->key)
			{
				case XK_F5:
				case XK_F6:
				case XK_F7:
				case XK_F9:
					THR_KeyEvent = ev->key;
					break;

				case XK_Escape:
                    if (conf.fullscreen()) GLWin.ToggleFullscreen();
					break;

				case XK_Shift_L:
				case XK_Shift_R:
					THR_bShift = true;
					break;

				case XK_Control_L:
				case XK_Control_R:
					THR_bCtrl = true;
					break;

				case XK_Alt_L:
				case XK_Alt_R:
					bAlt = true;
					break;

                case XK_Return:
                    if (bAlt)
                        GLWin.ToggleFullscreen();
			}
			break;

		case KEYRELEASE:
			switch (ev->key)
			{
				case XK_Shift_L:
				case XK_Shift_R:
					THR_bShift = false;
					break;

				case XK_Control_L:
				case XK_Control_R:
					THR_bCtrl = false;
					break;

				case XK_Alt_L:
				case XK_Alt_R:
					bAlt = false;
					break;
			}
	}
}

void add_map_entry(u32 option, const char *key, const char *desc)
{
	confOpts.value = option;
	confOpts.desc = desc;
	mapConfOpts[key] = confOpts;
}

void CreateGameHackTable(GtkWidget *treeview, gameHacks hacks)
{
	char descbuf[255];
	bool itval;
	GtkCellRenderer *treerend;
	GtkListStore *treestore;//Gets typecast as GtkTreeModel as needed.
	GtkTreeIter treeiter;
	GtkTreeViewColumn *treecol;

	//--------- Let's build a treeview for our advanced options! --------//
	treestore = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);

	//setup columns in treeview
	//COLUMN 0 is the checkboxes
	treecol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(treecol, "Select");
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), treecol);
	treerend = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(treecol, treerend, true);
	gtk_tree_view_column_add_attribute(treecol, treerend, "active", 0);//link 'active' attrib to first column of model
	g_object_set(treerend, "activatable", true, NULL);//set 'activatable' attrib true by default for all rows regardless of model.
	g_signal_connect(treerend, "toggled", (GCallback) OnToggle_advopts, treestore);//set a global callback, we also pass a reference to our treestore.

	//COLUMN 1 is the text descriptions
	treecol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(treecol, "Description");
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), treecol);
	treerend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(treecol, treerend, true);
	gtk_tree_view_column_add_attribute(treecol, treerend, "text", 1);//link 'text' attrib to second column of model

	//setup the model with all our rows of option data
	mapConfOpts.clear();

	add_map_entry(GAME_TEXTURETARGS, "00000001", "Tex Target checking - 00000001\nLego Racers");
	add_map_entry(GAME_AUTORESET, "00000002", "Auto reset targs - 00000002\nUse when game is slow and toggling AA fixes it. Samurai Warriors. (Automatically on for Shadow Hearts)");
	add_map_entry(GAME_INTERLACE2X, "00000004", "Interlace 2X - 00000004\nFixes 2x bigger screen. Gradius 3.");
	//GAME_TEXAHACK (still implemented)
	add_map_entry(GAME_NOTARGETRESOLVE, "00000010", "No target resolves - 00000010\nStops all resolving of targets.  Try this first for really slow games. (Automatically on for Dark Cloud 1.)");
	add_map_entry(GAME_EXACTCOLOR, "00000020", "Exact color testing - 00000020\nFixes overbright or shadow/black artifacts. Crash 'n Burn.");
	//add_map_entry(GAME_NOCOLORCLAMP, "00000040", "No color clamping - 00000040\nSpeeds up games, but might be too bright or too dim.");
	//GAME_FFXHACK
	add_map_entry(GAME_NOALPHAFAIL, "00000100", "Alpha Fail hack - 00000100\nRemove vertical stripes or other coloring artifacts. Breaks Persona 4 and MGS3. (Automatically on for Sonic Unleashed, Shadow the Hedgehog, & Ghost in the Shell.)");
	add_map_entry(GAME_NODEPTHUPDATE, "00000200", "Disable depth updates - 00000200");
	add_map_entry(GAME_QUICKRESOLVE1, "00000400", "Resolve Hack #1 - 00000400\n Speeds some games. Kingdom Hearts.");
	add_map_entry(GAME_NOQUICKRESOLVE, "00000800", "Resolve Hack #2 - 00000800\nShadow Hearts, Urbz. Destroys FFX.");
	add_map_entry(GAME_NOTARGETCLUT, "00001000", "No target CLUT - 00001000\nResident Evil 4, or foggy scenes.");
	add_map_entry(GAME_NOSTENCIL, "00002000", "Disable stencil buffer - 00002000\nUsually safe to do for simple scenes. Harvest Moon.");
	//GAME_VSSHACKOFF (still implemented)
	add_map_entry(GAME_NODEPTHRESOLVE, "00008000", "No depth resolve - 00008000\nMight give z buffer artifacts.");
	add_map_entry(GAME_FULL16BITRES, "00010000", "Full 16 bit resolution - 00010000\nUse when half the screen is missing.");
	add_map_entry(GAME_RESOLVEPROMOTED, "00020000", "Resolve Hack #3 - 00020000\nNeopets");
	add_map_entry(GAME_FASTUPDATE, "00040000", "Fast Update - 00040000\n Speeds some games. Needed for Sonic Unleashed. Okami.");
	add_map_entry(GAME_NOALPHATEST, "00080000", "Disable alpha testing - 00080000");
	add_map_entry(GAME_DISABLEMRTDEPTH, "00100000", "Enable Multiple RTs - 00100000");
	//GAME_32BITTARGS
	//GAME_PATH3HACK
	//GAME_DOPARALLELCTX
	add_map_entry(GAME_XENOSPECHACK, "01000000", "Specular Highlights - 01000000\nMakes graphics faster by removing highlights. (Automatically on for Xenosaga, Okami, & Okage.)");
	//add_map_entry(GAME_PARTIALPOINTERS, "02000000", "Partial targets - 02000000");
	add_map_entry(GAME_PARTIALDEPTH, "04000000", "Partial depth - 04000000");
	//GAME_REGETHACK (commented out in code)
	add_map_entry(GAME_GUSTHACK, "10000000", "Gust fix - 10000000. Makes gust games cleaner and faster. (Automatically on for most Gust games)");
	add_map_entry(GAME_NOLOGZ, "20000000", "No logarithmic Z - 20000000. Could decrease number of Z-artifacts.");
	add_map_entry(GAME_AUTOSKIPDRAW, "40000000", "Remove blur effect on some games\nSlow games.");

	for (map<string, confOptsStruct>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it)
	{
		gtk_list_store_append(treestore, &treeiter);//new row
		itval = (hacks._u32 & it->second.value) ? true : false;
		
		if (conf.def_hacks._u32 & it->second.value)
		{
			snprintf(descbuf, 254, "*%s", it->second.desc);
		}
		else
		{
			snprintf(descbuf, 254, "%s", it->second.desc);
		}
		
		gtk_list_store_set(treestore, &treeiter, 0, itval, 1, descbuf, -1);
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(treestore));//NB: store is cast as tree model.

	g_object_unref(treestore);//allow model to be destroyed when the tree is destroyed.

	//don't select/highlight rows
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_NONE);
	//------treeview done -------//
}

void SaveGameHackTable(GtkWidget *treeview, gameHacks& hacks)
{
	GtkTreeModel *treemodel;
	GtkTreeIter treeiter;
	gboolean treeoptval;

	//------- get advanced options from the treeview model -------//
	treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gtk_tree_model_get_iter_first(treemodel, &treeiter);

	hacks._u32 = 0;

	for (map<string, confOptsStruct>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it)
	{
		treeoptval = false;
		gtk_tree_model_get(treemodel, &treeiter, 0, &treeoptval, -1);

		if (treeoptval) hacks._u32 |= it->second.value;

		gtk_tree_model_iter_next(treemodel, &treeiter);
	}

	//---------- done getting advanced options ---------//
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

void DisplayAdvancedDialog()
{
	GtkWidget *dialog;
	
	GtkWidget *advanced_frame, *advanced_box;
	GtkWidget *advanced_scroll;
	GtkWidget *tree;
				 
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "ZZOgl PG Advanced Config");
	// A good value for the heigh will be 1000 instead of 800 but I'm afraid that some people still uses small screen...
	gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 800);
	gtk_window_set_modal(GTK_WINDOW(dialog), true);
	
	advanced_box = gtk_vbox_new(false, 5);
	advanced_frame = gtk_frame_new("Advanced Settings:");
	gtk_container_add(GTK_CONTAINER(advanced_frame), advanced_box);

	tree = gtk_tree_view_new();

	CreateGameHackTable(tree, tempHacks);

	advanced_scroll = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(advanced_scroll), tree);
	
	gtk_box_pack_start(GTK_BOX(advanced_box), advanced_scroll, true, true, 2);
	
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), advanced_frame);

	gtk_widget_show_all(dialog);

	gtk_dialog_run(GTK_DIALOG(dialog));
	SaveGameHackTable(tree, tempHacks);
	gtk_widget_destroy(dialog);
}

void DisplayDialog()
{
	int return_value;

	GtkWidget *dialog;
	GtkWidget *main_frame, *main_box;

	GtkWidget *option_frame, *option_box;
	GtkWidget *log_check, *dis_hacks_check;
	GtkWidget *int_label, *int_box, *int_holder;
	GtkWidget *bilinear_label, *bilinear_box, *bilinear_holder;
	GtkWidget *aa_label, *aa_box, *aa_holder;
	GtkWidget *snap_label, *snap_box, *snap_holder;
	GtkWidget *fullscreen_label, *widescreen_check;

	
	GtkWidget *advanced_button;
	GtkWidget *separator;
	GtkWidget *skipdraw_label, *skipdraw_text, *skipdraw_holder, *warning_label;
	
	if (!(conf.loaded())) LoadConfig();

	/* Create the widgets */
	dialog = gtk_dialog_new_with_buttons(
				 "ZZOgl PG Config",
				 NULL, /* parent window*/
				 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
				 GTK_STOCK_CANCEL,
				 GTK_RESPONSE_REJECT,
				 GTK_STOCK_OK,
				 GTK_RESPONSE_ACCEPT,
				 NULL);

	log_check = gtk_check_button_new_with_label("Logging");
	gtk_widget_set_tooltip_text(log_check, "Used for Debugging.");

	int_label = gtk_label_new("Interlacing:");
	int_box = gtk_combo_box_new_text();

	gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "No Interlacing");
	gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "Interlace 0");
	gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "Interlace 1");
	gtk_combo_box_set_active(GTK_COMBO_BOX(int_box), conf.interlace);
	gtk_widget_set_tooltip_text(int_box, "Toggled by pressing F5 when running.");
	int_holder = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(int_holder), int_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(int_holder), int_box, false, false, 2);

	bilinear_label = gtk_label_new("Bilinear Filtering:");
	bilinear_box = gtk_combo_box_new_text();
	
	gtk_combo_box_append_text(GTK_COMBO_BOX(bilinear_box), "Off");
	gtk_combo_box_append_text(GTK_COMBO_BOX(bilinear_box), "Normal");
	gtk_combo_box_append_text(GTK_COMBO_BOX(bilinear_box), "Forced");
	gtk_combo_box_set_active(GTK_COMBO_BOX(bilinear_box), conf.bilinear);
	gtk_widget_set_tooltip_text(bilinear_box, "Best quality is off. Turn on for speed. Toggled by pressing Shift + F5 when running.");
	bilinear_holder = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(bilinear_holder), bilinear_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(bilinear_holder), bilinear_box, false, false, 2);
	
	aa_label = gtk_label_new("Anti-Aliasing:");
	aa_box = gtk_combo_box_new_text();

	gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "1X (None)");
	gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "2X");
	gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "4X");
	gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "8X");
	gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "16X");
	gtk_combo_box_set_active(GTK_COMBO_BOX(aa_box), conf.aa);
	gtk_widget_set_tooltip_text(aa_box, "Toggled by pressing F6 when running.");
	aa_holder = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(aa_holder), aa_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(aa_holder), aa_box, false, false, 2);
	
	snap_label = gtk_label_new("Snapshot format:");
	snap_box = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(snap_box), "JPEG");
	gtk_combo_box_append_text(GTK_COMBO_BOX(snap_box), "TIFF");
	gtk_combo_box_set_active(GTK_COMBO_BOX(snap_box), conf.zz_options.tga_snap);
	snap_holder = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(snap_holder), snap_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(snap_holder), snap_box, false, false, 2);

	widescreen_check = gtk_check_button_new_with_label("Widescreen");
	gtk_widget_set_tooltip_text(widescreen_check, "Force a 4:3 ration when disabled");
	fullscreen_label = gtk_label_new("Press Alt-Enter for Fullscreen.");
	gtk_label_set_single_line_mode(GTK_LABEL(fullscreen_label), false);
	  
	advanced_button = gtk_button_new_with_label("Advanced...");

	dis_hacks_check = gtk_check_button_new_with_label("Disable Automatic Hacks");
	gtk_widget_set_tooltip_text(dis_hacks_check, "Used for testing how useful hacks that are on automatically are.");
	
#ifdef ZEROGS_DEVBUILD
	separator = gtk_hseparator_new();
	skipdraw_label = gtk_label_new("Skipdraw:");
	skipdraw_text = gtk_entry_new();
	warning_label = gtk_label_new("Experimental!!");
	char buf[5];
	sprintf(buf, "%d", conf.SkipDraw);
	gtk_entry_set_text(GTK_ENTRY(skipdraw_text), buf);
	skipdraw_holder = gtk_hbox_new(false, 5);
	
	gtk_box_pack_start(GTK_BOX(skipdraw_holder), skipdraw_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(skipdraw_holder), skipdraw_text, false, false, 2);
#endif
	
	main_box = gtk_hbox_new(false, 5);
	main_frame = gtk_frame_new("ZZOgl PG Config");

	gtk_container_add(GTK_CONTAINER(main_frame), main_box);

	option_box = gtk_vbox_new(false, 5);
	option_frame = gtk_frame_new("");
	gtk_container_add(GTK_CONTAINER(option_frame), option_box);
	gtk_frame_set_shadow_type(GTK_FRAME(option_frame), GTK_SHADOW_NONE);

	gtk_box_pack_start(GTK_BOX(option_box), log_check, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), bilinear_holder, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), int_holder, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), aa_holder, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), snap_holder, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), widescreen_check, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), advanced_button, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), dis_hacks_check, false, false, 2);
	
#ifdef ZEROGS_DEVBUILD
	gtk_box_pack_start(GTK_BOX(option_box), separator, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), warning_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), skipdraw_holder, false, false, 2);
#endif

	gtk_box_pack_start(GTK_BOX(option_box), fullscreen_label, false, false, 2);
	
	gtk_box_pack_start(GTK_BOX(main_box), option_frame, false, false, 2);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(log_check), conf.log);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widescreen_check), (conf.widescreen()));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dis_hacks_check), (conf.disableHacks));

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_frame);
	g_signal_connect_swapped(GTK_OBJECT (advanced_button), "clicked", G_CALLBACK(DisplayAdvancedDialog), advanced_button);
	tempHacks = conf.hacks;
	gtk_widget_show_all(dialog);

	return_value = gtk_dialog_run(GTK_DIALOG(dialog));

	if (return_value == GTK_RESPONSE_ACCEPT)
	{
		ZZOptions fake_options;

		if (gtk_combo_box_get_active(GTK_COMBO_BOX(int_box)) != -1)
			conf.interlace = gtk_combo_box_get_active(GTK_COMBO_BOX(int_box));

		if (gtk_combo_box_get_active(GTK_COMBO_BOX(aa_box)) != -1)
			conf.aa = gtk_combo_box_get_active(GTK_COMBO_BOX(aa_box));
			
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(bilinear_box)) != -1)
			conf.bilinear = gtk_combo_box_get_active(GTK_COMBO_BOX(bilinear_box));

		conf.log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(log_check));
		fake_options.widescreen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widescreen_check));
		fake_options.tga_snap = gtk_combo_box_get_active(GTK_COMBO_BOX(snap_box));
		
#ifdef ZEROGS_DEVBUILD
		conf.SkipDraw = atoi((char*)gtk_entry_get_text(GTK_ENTRY(skipdraw_text)));
#endif
	
		conf.zz_options = fake_options;
		conf.hacks = tempHacks;
		
		conf.disableHacks = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dis_hacks_check));
			
		GSsetGameCRC(g_LastCRC, conf.hacks._u32);

		SaveConfig();
	}

	gtk_widget_destroy(dialog);
}

void CALLBACK GSconfigure()
{
	char strcurdir[256];
	getcwd(strcurdir, 256);

	if (!(conf.loaded())) LoadConfig();

	DisplayDialog();
}

void SysMessage(const char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(NULL,
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_INFO,
									GTK_BUTTONS_OK,
									"%s", msg);

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

void CALLBACK GSabout()
{
	SysMessage("ZZOgl PG: by Zeydlitz (PG version worked on by arcum42, gregory, and the pcsx2 development team). Based off of ZeroGS, by zerofrog.");
}

s32 CALLBACK GStest()
{
	return 0;
}

void *SysLoadLibrary(char *lib)
{
	return dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
}

void *SysLoadSym(void *lib, char *sym)
{
	void *ret = dlsym(lib, sym);

	if (ret == NULL) ZZLog::Debug_Log("null: %s", sym);

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
