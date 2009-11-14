/*  OnePAD - author: arcum42(@gmail.com)
 *  Copyright (C) 2009
 *
 *  Based on ZeroPAD, author zerofrog@gmail.com
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

#include "onepad.h"
#include "linux.h"

extern "C"
{
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

extern string s_strIniPath;
GtkWidget *Conf, *About, *MsgDlg;
GtkWidget *s_devicecombo;

GtkTreeStore *treestore;
GtkTreeModel *model;  

bool has_columns = false;

extern string KeyName(int pad, int key);
extern const char* s_pGuiKeyMap[];
extern void UpdateConf(int pad);

enum 
{
	COL_PAD = 0,
	COL_BUTTON,
	COL_KEY,
	COL_VALUE,
	NUM_COLS
};


void populate_tree_view()
{
	GtkTreeIter toplevel;
	
	gtk_tree_store_clear(treestore);
	
	for (int pad = 0; pad < 2 * MAX_SUB_KEYS; pad++)
	{
		for (int key = 0; key < MAX_KEYS; key++)
		{
			if (get_key(pad, key) != 0)
			{
				gtk_tree_store_append(treestore, &toplevel, NULL);
				gtk_tree_store_set(treestore, &toplevel,
					COL_PAD, pad,
					COL_BUTTON, s_pGuiKeyMap[key],
					COL_KEY, KeyName(pad, key).c_str(),
					COL_VALUE, key,
				-1);
			}
		}
	}	
}

void create_a_column(GtkWidget *view, const char *name, int num, bool visible)
{
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *col;  
	
	col = gtk_tree_view_column_new();  
	gtk_tree_view_column_set_title(col, name);

	/* pack tree view column into tree view */
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_text_new();

	/* pack cell renderer into tree view column */
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	/* connect 'text' property of the cell renderer to
	*  model column that contains the first name */
	gtk_tree_view_column_add_attribute(col, renderer, "text", num);
	gtk_tree_view_column_set_visible(col, visible);
}

void create_columns(GtkWidget *view)
{
	if (!has_columns)
	{
		create_a_column(view, "Pad #", COL_PAD, true);
		create_a_column(view, "Pad Button", COL_BUTTON, true);
		create_a_column(view, "Key Value", COL_KEY, true);
		create_a_column(view, "Internal", COL_VALUE, false);
		has_columns = true;
	}
}

void init_tree_view()
{
	GtkWidget *view; 

	view = lookup_widget(Conf,"padtreeview");
	
	treestore = gtk_tree_store_new(NUM_COLS,
                                 G_TYPE_UINT,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
				 G_TYPE_UINT);

	populate_tree_view();
	create_columns(view);
	
	model = GTK_TREE_MODEL(treestore);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

	g_object_unref(model); /* destroy model automatically with view */

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
                              GTK_SELECTION_SINGLE);

}

void destroy_tree_view()
{
	has_columns = false;
	//g_object_unref(treestore);
}

/*void add_tree_row(int pad, int key)
{
	GtkTreeIter toplevel;

	gtk_tree_store_append(treestore, &toplevel, NULL);
	gtk_tree_store_set(treestore, &toplevel,
		COL_PAD, pad,
		COL_BUTTON, s_pGuiKeyMap[key],
		COL_KEY, KeyName(pad, key).c_str(),
		COL_VALUE, key,
	-1);
}

void delete_tree_row()
{
}*/

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

void on_Clear(GtkButton *button, gpointer user_data)
{
	clearPAD();
	init_tree_view();
}

void on_Remove(GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkWidget *view; 

	view = lookup_widget(Conf,"padtreeview");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		int pad;
		int key;

		gtk_tree_model_get(model, &iter, COL_PAD, &pad, COL_VALUE, &key,-1);
		set_key(pad, key, 0);
		init_tree_view();
	}
	else
	{
		// Not selected.
	}
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
	destroy_tree_view();
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

EXPORT_C_(void) PADabout()
{

	About = create_About();

	gtk_widget_show_all(About);
	gtk_main();
}

EXPORT_C_(s32) PADtest()
{
	return 0;
}
