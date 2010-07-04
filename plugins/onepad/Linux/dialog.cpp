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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "joystick.h"
#include "onepad.h"
#include <gtk/gtk.h>

extern bool PollX11Keyboard(char* &temp, u32 &pkey);
extern string KeyName(int pad, int key);

void config_key(int pad, int key);
void on_conf_key(GtkButton *button, gpointer user_data);

int current_pad = 0;
int current_joystick = -1;
GtkWidget *rev_lx_check, *rev_ly_check, *force_feedback_check, *rev_rx_check, *rev_ry_check;

const char* s_pGuiKeyMap[] =
{
	"L2", "R2", "L1", "R1",
	"Triangle", "Circle", "Cross", "Square",
	"Select", "L3", "R3", "Start",
	"Up", "Right", "Down", "Left",
	"Lx", "Rx", "Ly", "Ry",
	"L_Up", "L_Right", "L_Down", "L_Left",
	"R_Up", "R_Right", "R_Down", "R_Left"
};

enum
{
	COL_PAD = 0,
	COL_BUTTON,
	COL_KEY,
	COL_PAD_NUM,
	COL_VALUE,
	NUM_COLS
};

class keys_tree
{	
	private:
		GtkTreeStore *treestore;
		GtkTreeModel *model;
		GtkTreeView *view;
		bool has_columns;
		
		void repopulate()
		{
			GtkTreeIter toplevel;

			gtk_tree_store_clear(treestore);

			for (int pad = 0; pad < 2 * MAX_SUB_KEYS; pad++)
			{
				for (int key = 0; key < MAX_KEYS; key++)
				{
					if (get_key(pad, key) != 0)
					{
						string pad_value;
						switch(pad)
						{
							case 0: pad_value = "Pad 1"; break;
							case 2: pad_value = "Pad 1"; break;
							case 1: pad_value = "Pad 2"; break;
							case 3: pad_value = "Pad 2"; break;
							default: pad_value = "Invalid"; break;
						}
						gtk_tree_store_append(treestore, &toplevel, NULL);
						gtk_tree_store_set(treestore, &toplevel,
							COL_PAD, pad_value.c_str(),
							COL_BUTTON, s_pGuiKeyMap[key],
							COL_KEY, KeyName(pad, key).c_str(),
							COL_PAD_NUM, pad,
							COL_VALUE, key,
						-1);
					}
				}
			}
		}

		void create_a_column(const char *name, int num, bool visible)
		{
			GtkCellRenderer     *renderer;
			GtkTreeViewColumn   *col;

			col = gtk_tree_view_column_new();
			gtk_tree_view_column_set_title(col, name);

			/* pack tree view column into tree view */
			gtk_tree_view_append_column(view, col);

			renderer = gtk_cell_renderer_text_new();

			/* pack cell renderer into tree view column */
			gtk_tree_view_column_pack_start(col, renderer, TRUE);

			/* connect 'text' property of the cell renderer to
			*  model column that contains the first name */
			gtk_tree_view_column_add_attribute(col, renderer, "text", num);
			gtk_tree_view_column_set_visible(col, visible);
		}

		void create_columns()
		{
			if (!has_columns)
			{
				create_a_column("Pad #", COL_PAD, true);
				create_a_column("Pad Button", COL_BUTTON, true);
				create_a_column("Key Value", COL_KEY, true);
				create_a_column("Pad Num", COL_PAD_NUM, false);
				create_a_column("Internal", COL_VALUE, false);
				has_columns = true;
			}
		}
	public:
		GtkWidget *view_widget()
		{
			return GTK_WIDGET(view);
		}
	
		void init()
		{
			has_columns = false;
			view = GTK_TREE_VIEW(gtk_tree_view_new());
			treestore = gtk_tree_store_new(NUM_COLS,
										G_TYPE_STRING,
										G_TYPE_STRING,
										G_TYPE_STRING,
										G_TYPE_UINT,
										G_TYPE_UINT);

			model = GTK_TREE_MODEL(treestore);
			gtk_tree_view_set_model(view, model);
			g_object_unref(model); /* destroy model automatically with view */
			gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view), GTK_SELECTION_SINGLE);
		}
		
		void update()
		{
			create_columns();
			repopulate();
		}
		
		void clear_all()
		{
			clearPAD();
			update();
		}
		
		bool get_selected(int &pad, int &key)
		{
			GtkTreeSelection *selection;
			GtkTreeIter iter;

			selection = gtk_tree_view_get_selection(view);
			if (gtk_tree_selection_get_selected(selection, &model, &iter))
			{
				gtk_tree_model_get(model, &iter, COL_PAD_NUM, &pad, COL_VALUE, &key,-1);
				return true;
			}
			return false;
		}
		
		void remove_selected()
		{
			int key, pad;
			if (get_selected(pad, key))
			{
				set_key(pad, key, 0);
				update();
			}
		}
		void modify_selected()
		{
			int key, pad;
			if (get_selected(pad, key))
			{
				config_key(pad,key);
				update();
			}
		}
};
keys_tree *fir;

int _GetJoystickIdFromPAD(int pad)
{
	// select the right joystick id
	u32 joyid = -1;

//	for (int p = 0; p < MAX_SUB_KEYS; p++)
//	{
//		for (int i = 0; i < MAX_KEYS; ++i)
//		{
//			KeyType k = type_of_key(PadEnum[pad][p],i);
//
//			if (k == PAD_JOYSTICK || k == PAD_JOYBUTTONS)
//			{
//				joyid = key_to_joystick_id(PadEnum[pad][p],i);
//				return joyid;
//			}
//		}
//	}

	if (!JoystickIdWithinBounds(joyid))
	{
		// get first unused joystick
		for (joyid = 0; joyid < s_vjoysticks.size(); ++joyid)
		{
			if (s_vjoysticks[joyid]->GetPAD() < 0) break;
		}
	}

	return joyid;
}

int Get_Current_Joystick()
{
	// check bounds
	int joyid = _GetJoystickIdFromPAD(0);

	if (JoystickIdWithinBounds(joyid))
		return joyid + 1; // select the combo
	else
		return 0; //s_vjoysticks.size(); // no gamepad
}

void populate_new_joysticks(GtkComboBox *box)
{
	char str[255];
	JoystickInfo::EnumerateJoysticks(s_vjoysticks);

	gtk_combo_box_append_text(box, "No Gamepad");
	
	vector<JoystickInfo*>::iterator it = s_vjoysticks.begin();

	// Delete everything in the vector vjoysticks.
	while (it != s_vjoysticks.end())
	{
		sprintf(str, "%d: %s - but: %d, axes: %d, hats: %d", (*it)->GetId(), (*it)->GetName().c_str(),
		        (*it)->GetNumButtons(), (*it)->GetNumAxes(), (*it)->GetNumHats());
		gtk_combo_box_append_text(box, str);
		it++;
	}
    current_joystick = Get_Current_Joystick();
}

typedef struct
{
	GtkWidget *widget;
	int index;
	void put(const char* lbl, int i, GtkFixed *fix, int x, int y)
	{
		widget = gtk_button_new_with_label(lbl);
		gtk_fixed_put(fix, widget, x, y);
		gtk_widget_set_size_request(widget, 64, 24);
		index = i;
		g_signal_connect(GTK_OBJECT (widget), "clicked", G_CALLBACK(on_conf_key), this);
	}
} dialog_buttons;

void config_key(int pad, int key)
{
	bool captured = false;

	// save the joystick states
	UpdateJoysticks();

	while (!captured)
	{
		vector<JoystickInfo*>::iterator itjoy;
		char *tmp;

		u32 pkey = get_key(pad, key);
		if (PollX11Keyboard(tmp, pkey))
		{
			set_key(pad, key, pkey);
			PAD_LOG("%s\n", KeyName(pad, key).c_str());
			captured = true;
			break;
		}

		SDL_JoystickUpdate();

		itjoy = s_vjoysticks.begin();
		while ((itjoy != s_vjoysticks.end()) && (!captured))
		{
			int button_id, direction;

			pkey = get_key(pad, key);
			if ((*itjoy)->PollButtons(button_id, pkey))
			{
				set_key(pad, key, pkey);
				PAD_LOG("%s\n", KeyName(pad, key).c_str());
				captured = true;
				break;
			}

			bool sign = false;
			bool pov = (!((key == PAD_RY) || (key == PAD_LY) || (key == PAD_RX) || (key == PAD_LX)));

			int axis_id;

			if (pov)
			{
				if ((*itjoy)->PollPOV(axis_id, sign, pkey))
				{
					set_key(pad, key, pkey);
					PAD_LOG("%s\n", KeyName(pad, key).c_str());
					captured = true;
					break;
				}
			}
			else
			{
				if ((*itjoy)->PollAxes(axis_id, pkey))
				{
					set_key(pad, key, pkey);
					PAD_LOG("%s\n", KeyName(pad, key).c_str());
					captured = true;
					break;
				}
			}

			if ((*itjoy)->PollHats(axis_id, direction, pkey))
			{
				set_key(pad, key, pkey);
				PAD_LOG("%s\n", KeyName(pad, key).c_str());
				captured = true;
				break;
			}
			itjoy++;
		}
	}
}

void on_conf_key(GtkButton *button, gpointer user_data)
{
	dialog_buttons *btn = (dialog_buttons *)user_data;
	int key = btn->index;

	if (key == -1) return;
	
	config_key(current_pad, key);
	fir->update();
}

void on_remove_clicked(GtkButton *button, gpointer user_data)
{
	fir->remove_selected();
}

void on_clear_clicked(GtkButton *button, gpointer user_data)
{
	fir->clear_all();
}

void on_modify_clicked(GtkButton *button, gpointer user_data)
{
	fir->modify_selected();
}

void joy_changed(GtkComboBox *box, gpointer user_data)
{
	int joyid = gtk_combo_box_get_active(box);

	// unassign every joystick with this pad
	for (int i = 0; i < (int)s_vjoysticks.size(); ++i)
	{
		if (s_vjoysticks[i]->GetPAD() == s_selectedpad) s_vjoysticks[i]->Assign(-1);
	}

	if (joyid >= 0 && joyid < (int)s_vjoysticks.size()) s_vjoysticks[joyid]->Assign(current_pad);
}

void pad_changed(GtkComboBox *box, gpointer user_data)
{
	int temp = gtk_combo_box_get_active(box);
	if (temp >= 0) current_pad = temp;
	fir->update();
    int options = (conf.options >> (16 * current_pad));
	
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rev_lx_check), (options & PADOPTION_REVERSELX));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rev_ly_check), (options & PADOPTION_REVERSELY));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rev_rx_check), (options & PADOPTION_REVERSERX));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rev_ry_check), (options & PADOPTION_REVERSERY));
}

void update_option(int option, bool value)
{
    int mask = (option << (16 * current_pad));
    
    if (value)
		conf.options |= mask;
	else
		conf.options &= ~mask;
}

void on_refresh(GtkComboBox *box, gpointer user_data)
{
    GtkComboBox *joy_choose_cbox = (GtkComboBox*)user_data;
    
    if (current_joystick < 0) current_joystick = Get_Current_Joystick();
    
    for(int i=0; i <= (int)s_vjoysticks.size(); i++)
    {
    	gtk_combo_box_remove_text(joy_choose_cbox, 0);
    }
    populate_new_joysticks(joy_choose_cbox);
    if (gtk_combo_box_get_active(joy_choose_cbox) != current_joystick)
		gtk_combo_box_set_active(joy_choose_cbox, current_joystick);
}

void on_rev_lx(GtkComboBox *box, gpointer user_data)
{
	update_option(PADOPTION_REVERSELX, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rev_lx_check)));
}

void on_rev_ly(GtkComboBox *box, gpointer user_data)
{
	update_option(PADOPTION_REVERSELY, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rev_ly_check)));
}

void on_rev_rx(GtkComboBox *box, gpointer user_data)
{
	update_option(PADOPTION_REVERSERX, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rev_rx_check)));
}

void on_rev_ry(GtkComboBox *box, gpointer user_data)
{
	update_option(PADOPTION_REVERSERY, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rev_ry_check)));
}

//void on_forcefeedback_toggled(GtkToggleButton *togglebutton, gpointer user_data)
//{
//	int mask = PADOPTION_REVERSELX << (16 * s_selectedpad);
//
//	if (gtk_toggle_button_get_active(togglebutton))
//	{
//		conf.options |= mask;
//
//		int joyid = gtk_combo_box_get_active(GTK_COMBO_BOX(s_devicecombo));
//
//		if (joyid >= 0 && joyid < (int)s_vjoysticks.size()) s_vjoysticks[joyid]->TestForce();
//	}
//	else
//	{
//		conf.options &= ~mask;
//	}
//}

void DisplayDialog()
{
    int return_value;

    GtkWidget *dialog;
    GtkWidget *main_frame, *main_box;

    GtkWidget *pad_choose_frame, *pad_choose_box;
    GtkComboBox *pad_choose_cbox;
    
    GtkWidget *joy_choose_frame, *joy_choose_box;
    GtkComboBox *joy_choose_cbox;
    //GtkWidget *joy_refresh;
    
    GtkWidget *keys_frame, *keys_box;
    
    GtkWidget *keys_tree_frame, *keys_tree_box;
    GtkWidget *keys_tree_clear_btn, *keys_tree_remove_btn, *keys_tree_modify_btn;
    GtkWidget *keys_btn_box, *keys_btn_frame;
    
    GtkWidget *keys_static_frame, *keys_static_box;
    GtkWidget *keys_static_area;
    dialog_buttons btn[29];
    
	LoadConfig();
    current_pad = 0;
	fir = new keys_tree;
	fir->init();
	
    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons (
		"OnePAD Config",
		NULL, /* parent window*/
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
		NULL);

    pad_choose_cbox = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(pad_choose_cbox, "Pad 1");
    gtk_combo_box_append_text(pad_choose_cbox, "Pad 2");
    gtk_combo_box_append_text(pad_choose_cbox, "Pad 1 (alt)");
    gtk_combo_box_append_text(pad_choose_cbox, "Pad 2 (alt)");
    gtk_combo_box_set_active(pad_choose_cbox, current_pad);
	g_signal_connect(GTK_OBJECT (pad_choose_cbox), "changed", G_CALLBACK(pad_changed), NULL);
    
    if (current_joystick == -1) current_joystick = Get_Current_Joystick();
    joy_choose_cbox = GTK_COMBO_BOX(gtk_combo_box_new_text());
    populate_new_joysticks(joy_choose_cbox);
    gtk_combo_box_set_active(joy_choose_cbox, current_joystick);
	g_signal_connect(GTK_OBJECT (joy_choose_cbox), "changed", G_CALLBACK(joy_changed), NULL);
	
	//joy_refresh = gtk_button_new_with_label("Refresh");
	//g_signal_connect(GTK_OBJECT (joy_refresh), "clicked", G_CALLBACK(on_refresh), joy_choose_cbox);
	//gtk_widget_set_size_request(joy_refresh, 64, 24);
	
	keys_tree_clear_btn = gtk_button_new_with_label("Clear All");
	g_signal_connect(GTK_OBJECT (keys_tree_clear_btn), "clicked", G_CALLBACK(on_clear_clicked), NULL);
	gtk_widget_set_size_request(keys_tree_clear_btn, 64, 24);
	
	keys_tree_remove_btn = gtk_button_new_with_label("Remove");
	g_signal_connect(GTK_OBJECT (keys_tree_remove_btn), "clicked", G_CALLBACK(on_remove_clicked), NULL);
    gtk_widget_set_size_request(keys_tree_remove_btn, 64, 24);
    
	keys_tree_modify_btn = gtk_button_new_with_label("Modify");
	g_signal_connect(GTK_OBJECT (keys_tree_modify_btn), "clicked", G_CALLBACK(on_modify_clicked), NULL);
    gtk_widget_set_size_request(keys_tree_modify_btn, 64, 24);
		
    main_box = gtk_vbox_new(false, 5);
    main_frame = gtk_frame_new ("Onepad Config");
    gtk_container_add (GTK_CONTAINER(main_frame), main_box);

    pad_choose_box = gtk_vbox_new(false, 5);
    pad_choose_frame = gtk_frame_new ("Choose a Pad to modify:");
    gtk_container_add (GTK_CONTAINER(pad_choose_frame), pad_choose_box);

    joy_choose_box = gtk_hbox_new(false, 5);
    joy_choose_frame = gtk_frame_new ("Joystick to use for this pad");
    gtk_container_add (GTK_CONTAINER(joy_choose_frame), joy_choose_box);

    keys_btn_box = gtk_hbox_new(false, 5);
    keys_btn_frame = gtk_frame_new ("");
    gtk_container_add (GTK_CONTAINER(keys_btn_frame), keys_btn_box);
    
    keys_tree_box = gtk_vbox_new(false, 5);
    keys_tree_frame = gtk_frame_new ("");
    gtk_container_add (GTK_CONTAINER(keys_tree_frame), keys_tree_box);
    
    keys_static_box = gtk_hbox_new(false, 5);
    keys_static_frame = gtk_frame_new ("");
    gtk_container_add (GTK_CONTAINER(keys_static_frame), keys_static_box);
    
	keys_static_area = gtk_fixed_new();
	
	u32 static_offset = 0; //320
	btn[0].put("L2", 0, GTK_FIXED(keys_static_area), static_offset + 64, 8);
	btn[1].put("R2", 1, GTK_FIXED(keys_static_area), static_offset + 392, 8);
	btn[2].put("L1", 2, GTK_FIXED(keys_static_area), static_offset + 64, 32);
	btn[3].put("R1", 3, GTK_FIXED(keys_static_area), static_offset + 392, 32);
	
	btn[4].put("Triangle", 4, GTK_FIXED(keys_static_area), static_offset + 392, 80);
	btn[5].put("Circle", 5, GTK_FIXED(keys_static_area), static_offset + 456, 104);
	btn[6].put("Cross", 6, GTK_FIXED(keys_static_area), static_offset + 392,128);
	btn[7].put("Square", 7, GTK_FIXED(keys_static_area), static_offset + 328, 104);
	
	btn[8].put("Select", 8, GTK_FIXED(keys_static_area), static_offset + 200, 48);
	btn[9].put("L3", 9, GTK_FIXED(keys_static_area), static_offset + 200, 8);
	btn[10].put("R3", 10, GTK_FIXED(keys_static_area), static_offset + 272, 8);
	btn[11].put("Start", 11, GTK_FIXED(keys_static_area), static_offset + 272, 48);
	
	// Arrow pad
	btn[12].put("Up", 12, GTK_FIXED(keys_static_area), static_offset + 64, 80);
	btn[13].put("Right", 13, GTK_FIXED(keys_static_area), static_offset + 128, 104);
	btn[14].put("Down", 14, GTK_FIXED(keys_static_area), static_offset + 64, 128);
	btn[15].put("Left", 15, GTK_FIXED(keys_static_area), static_offset + 0, 104);
	
	//btn[xx].put("Analog", GTK_FIXED(keys_static_area), static_offset + 232, 104);
	
	btn[16].put("Lx", 16, GTK_FIXED(keys_static_area), static_offset + 64, 264);
	btn[17].put("Rx", 17, GTK_FIXED(keys_static_area), static_offset + 392, 264);
	btn[18].put("Ly", 18, GTK_FIXED(keys_static_area), static_offset + 64, 288);
	btn[19].put("Ry", 19, GTK_FIXED(keys_static_area), static_offset + 392, 288);
	
	// Left Joystick
	btn[20].put("Up", 20, GTK_FIXED(keys_static_area), static_offset + 64, 240);
	btn[21].put("Right", 21, GTK_FIXED(keys_static_area), static_offset + 128, 272);
	btn[22].put("Down", 22, GTK_FIXED(keys_static_area), static_offset + 64, 312);
	btn[23].put("Left", 23, GTK_FIXED(keys_static_area), static_offset + 0, 272);
	
	// Right Joystick
	btn[24].put("Up", 24, GTK_FIXED(keys_static_area), static_offset + 392, 240);
	btn[25].put("Right", 25, GTK_FIXED(keys_static_area), static_offset + 456, 272);
	btn[26].put("Down", 26, GTK_FIXED(keys_static_area), static_offset + 392, 312);
	btn[27].put("Left", 27, GTK_FIXED(keys_static_area), static_offset + 328, 272);
    
    int options = (conf.options >> (16 * current_pad));
    rev_lx_check = gtk_check_button_new_with_label("Reverse Lx");
	gtk_fixed_put(GTK_FIXED(keys_static_area), rev_lx_check, static_offset + 40, 344);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rev_lx_check), (options & PADOPTION_REVERSELX));
	g_signal_connect(GTK_OBJECT (rev_lx_check), "toggled", G_CALLBACK(on_rev_lx), NULL);
	
    rev_ly_check = gtk_check_button_new_with_label("Reverse Ly");
	gtk_fixed_put(GTK_FIXED(keys_static_area), rev_ly_check, static_offset + 40, 368);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rev_ly_check), (options & PADOPTION_REVERSELY));
	g_signal_connect(GTK_OBJECT (rev_ly_check), "toggled", G_CALLBACK(on_rev_ly), NULL);
	
    rev_rx_check = gtk_check_button_new_with_label("Reverse Rx");
	gtk_fixed_put(GTK_FIXED(keys_static_area), rev_rx_check, static_offset + 368, 344);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rev_rx_check), (options & PADOPTION_REVERSERX));
	g_signal_connect(GTK_OBJECT (rev_rx_check), "toggled", G_CALLBACK(on_rev_rx), NULL);
	
    rev_ry_check = gtk_check_button_new_with_label("Reverse Ry");
	gtk_fixed_put(GTK_FIXED(keys_static_area), rev_ry_check, static_offset + 368, 368);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rev_ry_check), (options & PADOPTION_REVERSERY));
	g_signal_connect(GTK_OBJECT (rev_ry_check), "toggled", G_CALLBACK(on_rev_ry), NULL);
    
    keys_box = gtk_hbox_new(false, 5);
    keys_frame = gtk_frame_new ("Key Settings");
    gtk_container_add (GTK_CONTAINER(keys_frame), keys_box);
    
	gtk_box_pack_start (GTK_BOX (keys_tree_box), fir->view_widget(), true, true, 0);
	gtk_box_pack_end (GTK_BOX (keys_btn_box), keys_tree_clear_btn, false, false, 0);
	gtk_box_pack_end (GTK_BOX (keys_btn_box), keys_tree_remove_btn, false, false, 0);
	gtk_box_pack_end (GTK_BOX (keys_btn_box), keys_tree_modify_btn, false, false, 0);
    
	gtk_container_add(GTK_CONTAINER(pad_choose_box), GTK_WIDGET(pad_choose_cbox));
	gtk_container_add(GTK_CONTAINER(joy_choose_box), GTK_WIDGET(joy_choose_cbox));
	//gtk_container_add(GTK_CONTAINER(joy_choose_box), joy_refresh);
	gtk_container_add(GTK_CONTAINER(keys_tree_box), keys_btn_frame);
	gtk_box_pack_start (GTK_BOX (keys_box), keys_tree_frame, true, true, 0);
	gtk_container_add(GTK_CONTAINER(keys_box), keys_static_area);

	gtk_container_add(GTK_CONTAINER(main_box), pad_choose_frame);
	gtk_container_add(GTK_CONTAINER(main_box), joy_choose_frame);
	gtk_container_add(GTK_CONTAINER(main_box), keys_frame);

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_frame);
    
    fir->update();
    
    gtk_widget_show_all (dialog);

    return_value = gtk_dialog_run (GTK_DIALOG (dialog));
    if (return_value == GTK_RESPONSE_ACCEPT) SaveConfig();
	
	LoadConfig();
	delete fir;
    gtk_widget_destroy (dialog);
}
