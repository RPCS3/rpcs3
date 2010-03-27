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

void add_map_entry(u32 option, const char *key, const char *desc)
{
	confOpts.value = option;
	confOpts.desc = desc;
	mapConfOpts[key] = confOpts;
}

void CreateGameHackTable(GtkWidget *treeview)
{
	char descbuf[255];
	bool itval;
	GtkCellRenderer *treerend;
	GtkListStore *treestore;//Gets typecast as GtkTreeModel as needed.
	GtkTreeIter treeiter;
	GtkTreeViewColumn *treecol;
	
	//--------- Let's build a treeview for our advanced options! --------//
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
}

void SaveGameHackTable(GtkWidget *treeview)
{
	GtkTreeModel *treemodel;
	GtkTreeIter treeiter;
	gboolean treeoptval;
	
	//------- get advanced options from the treeview model -------//
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

void DisplayDialog()
{
    int return_value;
    
    GtkWidget *dialog;
    GtkWidget *main_frame, *main_box;
    
    GtkWidget *option_frame, *option_box;
    GtkWidget *int_label, *int_box;
    GtkWidget *bilinear_check, *bilinear_label;
    GtkWidget *aa_label, *aa_box;
    GtkWidget *wireframe_check, *avi_check;
    GtkWidget *snap_label, *snap_box;
    GtkWidget *size_label, *size_box;
    GtkWidget *fullscreen_check, *widescreen_check;
    
    GtkWidget *advanced_frame, *advanced_box;
    GtkWidget *advanced_scroll;
    GtkWidget *tree;
	
	if (!(conf.options & GSOPTION_LOADED)) LoadConfig();
	
    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons (
		"ZZOgl PG Config",
		NULL, /* parent window*/
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
		GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
		NULL);
	
    int_label = gtk_label_new ("Interlacing: (F5 to toggle)");
    int_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "No Interlacing");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "Interlace 0");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "Interlace 1");
    gtk_combo_box_set_active(GTK_COMBO_BOX(int_box), conf.interlace);
    
    
    bilinear_check = gtk_check_button_new_with_label("Bilinear Filtering (Shift + F5)");
    bilinear_label = gtk_label_new ("Best quality is off. Turn on for speed.");
    
    aa_label = gtk_label_new ("Anti-Aliasing for Higher Quality(F6)");
    aa_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "1X - No Anti-Aliasing");
    gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "2X - Anti-Aliasing x 2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "4X - Anti-Aliasing x 4");
    gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "8X - Anti-Aliasing x 8");
    gtk_combo_box_append_text(GTK_COMBO_BOX(aa_box), "8X - Anti-Aliasing x 8");
    gtk_combo_box_set_active(GTK_COMBO_BOX(aa_box), conf.aa);
    
    wireframe_check = gtk_check_button_new_with_label("Wireframe Rendering(Shift + F6)");
    avi_check = gtk_check_button_new_with_label("Capture Avi (as zerogs.avi)(F7)");
    
    snap_label = gtk_label_new ("Snapshot format:");
    snap_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(snap_box), "JPEG");
    gtk_combo_box_append_text(GTK_COMBO_BOX(snap_box), "TIFF");
    gtk_combo_box_set_active(GTK_COMBO_BOX(snap_box), conf.options&GSOPTION_TGASNAP);
    
    fullscreen_check = gtk_check_button_new_with_label("Fullscreen (Alt + Enter)");
	widescreen_check = gtk_check_button_new_with_label("Widescreen");
	
    size_label = gtk_label_new ("Default Window Size: (no speed impact)");
    size_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(size_box), "640x480");
    gtk_combo_box_append_text(GTK_COMBO_BOX(size_box), "800x600");
    gtk_combo_box_append_text(GTK_COMBO_BOX(size_box), "1024x768");
    gtk_combo_box_append_text(GTK_COMBO_BOX(size_box), "1280x960");
    gtk_combo_box_set_active(GTK_COMBO_BOX(size_box), (conf.options&GSOPTION_WINDIMS)>>4);
    
    main_box = gtk_hbox_new(false, 5);
    main_frame = gtk_frame_new ("ZZOgl PG Config");
    gtk_container_add (GTK_CONTAINER(main_frame), main_box);
    
    option_box = gtk_vbox_new(false, 5);
    option_frame = gtk_frame_new ("");
    gtk_container_add (GTK_CONTAINER(option_frame), option_box);
    
    advanced_box = gtk_vbox_new(false, 5);
    advanced_frame = gtk_frame_new ("Advanced Settings:");
    gtk_container_add (GTK_CONTAINER(advanced_frame), advanced_box);
    
	tree = gtk_tree_view_new();
	CreateGameHackTable(tree);
	advanced_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(advanced_scroll), tree);
	
	gtk_box_pack_start(GTK_BOX(option_box), int_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), int_box, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), bilinear_check, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), bilinear_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), aa_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), aa_box, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), wireframe_check, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), avi_check, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), snap_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), snap_box, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), fullscreen_check, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), widescreen_check, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), size_label, false, false, 2);
	gtk_box_pack_start(GTK_BOX(option_box), size_box, false, false, 2);
	
	gtk_box_pack_start(GTK_BOX(advanced_box), advanced_scroll, true, true, 2);
	
	gtk_box_pack_start(GTK_BOX(main_box), option_frame, false, false, 2);
	gtk_box_pack_start(GTK_BOX(main_box), advanced_frame, true, true, 2);
	
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bilinear_check), conf.bilinear);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wireframe_check), (conf.options & GSOPTION_WIREFRAME));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(avi_check), (conf.options & GSOPTION_CAPTUREAVI));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fullscreen_check), (conf.options & GSOPTION_FULLSCREEN));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widescreen_check), (conf.options & GSOPTION_WIDESCREEN));
    
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_frame);
    gtk_widget_show_all (dialog);
    
    return_value = gtk_dialog_run (GTK_DIALOG (dialog));
    
    if (return_value == GTK_RESPONSE_ACCEPT)
    {
    	int fake_options = 0;
    	SaveGameHackTable(tree);
    	
    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(int_box)) != -1) 
			conf.interlace = gtk_combo_box_get_active(GTK_COMBO_BOX(int_box));

    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(aa_box)) != -1) 
			conf.aa = gtk_combo_box_get_active(GTK_COMBO_BOX(aa_box));
			
		conf.negaa = 0;	
			
		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(size_box)))
		{
			case 0: fake_options |= GSOPTION_WIN640; break;
			case 1: fake_options |= GSOPTION_WIN800; break;
			case 2: fake_options |= GSOPTION_WIN1024; break;
			case 3: fake_options |= GSOPTION_WIN1280; break;
		}
		
		conf.bilinear = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bilinear_check));
		
    	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wireframe_check))) 
			fake_options |= GSOPTION_WIREFRAME;
			
    	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(avi_check))) 
			fake_options |= GSOPTION_CAPTUREAVI;
			
    	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fullscreen_check))) 
			fake_options |= GSOPTION_FULLSCREEN;
			
    	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widescreen_check))) 
			fake_options |= GSOPTION_WIDESCREEN;
			
    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(snap_box)) == 1) 
			fake_options |= GSOPTION_TGASNAP;

		conf.options = fake_options;
		SaveConfig();
    }
    
    gtk_widget_destroy (dialog);
}

void CALLBACK GSconfigure()
{
	char strcurdir[256];
	getcwd(strcurdir, 256);
	
	if (!(conf.options & GSOPTION_LOADED)) LoadConfig();
	
	DisplayDialog();
}

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

void CALLBACK GSabout() 
{
	SysMessage("ZZOgl PG: by Zeydlitz (PG version worked on by arcum42). Based off of ZeroGS, by zerofrog.");
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
