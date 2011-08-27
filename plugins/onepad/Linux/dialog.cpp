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
#include "keyboard.h"
#include "onepad.h"
#include <gtk/gtk.h>

void config_key(int pad, int key);
void on_conf_key(GtkButton *button, gpointer user_data);
void on_toggle_option(GtkToggleButton *togglebutton, gpointer user_data);

static int current_pad = 0;
static GtkComboBox *joy_choose_cbox;

const char* s_pGuiKeyMap[] =
{
	"L2", "R2", "L1", "R1",
	"Triangle", "Circle", "Cross", "Square",
	"Select", "L3", "R3", "Start",
	"Up", "Right", "Down", "Left",
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
	COL_KEYSYM,
	NUM_COLS
};
	
class keys_tree
{	
	private:
		GtkTreeStore *treestore;
		GtkTreeModel *model;
		GtkTreeView *view[2];
		bool has_columns;
		int show_pad;
		bool show_keyboard_key[2];
		bool show_joy_key[2];

		void repopulate()
		{
			GtkTreeIter toplevel;

			gtk_tree_store_clear(treestore);

			string pad_value;
			switch(show_pad) {
				case 0: pad_value = "Pad 1"; break;
				case 1: pad_value = "Pad 2"; break;
				default: pad_value = "Invalid"; break;
			}

			// joystick key
			if (show_joy_key[show_pad]) {
				for (int key = 0; key < MAX_KEYS; key++)
				{
					if (get_key(show_pad, key) != 0)
					{
						gtk_tree_store_append(treestore, &toplevel, NULL);
						gtk_tree_store_set(treestore, &toplevel,
								COL_PAD, pad_value.c_str(),
								COL_BUTTON, s_pGuiKeyMap[key],
								COL_KEY, KeyName(show_pad, key).c_str(),
								COL_PAD_NUM, show_pad,
								COL_VALUE, key,
								COL_KEYSYM, 0,
								-1);
					}
				}
			}

			// keyboard/mouse key
			if (show_keyboard_key[show_pad]) {
				map<u32,u32>::iterator it;
				for (it = conf->keysym_map[show_pad].begin(); it != conf->keysym_map[show_pad].end(); ++it) {
					int keysym = it->first;
					int key = it->second;
					gtk_tree_store_append(treestore, &toplevel, NULL);
					gtk_tree_store_set(treestore, &toplevel,
							COL_PAD, pad_value.c_str(),
							COL_BUTTON, s_pGuiKeyMap[key],
							COL_KEY, KeyName(show_pad, key, keysym).c_str(),
							COL_PAD_NUM, show_pad,
							COL_VALUE, key,
							COL_KEYSYM, keysym,
							-1);
				}
			}
		}

		void create_a_column(const char *name, int num, bool visible)
		{
			for (int i = 0; i < 2; i++) {
				GtkCellRenderer     *renderer;
				GtkTreeViewColumn   *col;

				col = gtk_tree_view_column_new();
				gtk_tree_view_column_set_title(col, name);

				/* pack tree view column into tree view */
				gtk_tree_view_append_column(view[i], col);

				renderer = gtk_cell_renderer_text_new();

				/* pack cell renderer into tree view column */
				gtk_tree_view_column_pack_start(col, renderer, TRUE);

				/* connect 'text' property of the cell renderer to
				 *  model column that contains the first name */
				gtk_tree_view_column_add_attribute(col, renderer, "text", num);
				gtk_tree_view_column_set_visible(col, visible);
			}
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
				create_a_column("Keysym", COL_KEYSYM, false);
				has_columns = true;
			}
		}
	public:
		GtkWidget *view_widget(int i)
		{
			return GTK_WIDGET(view[i]);
		}

		void init()
		{
			show_pad = 0;
			has_columns = false;
			for (int i = 0; i < 2; i++)
				show_keyboard_key[i] = show_joy_key[i] = true;

			treestore = gtk_tree_store_new(NUM_COLS,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_UINT,
					G_TYPE_UINT,
					G_TYPE_UINT);

			model = GTK_TREE_MODEL(treestore);

			for (int i = 0; i < 2; i++) {
				view[i] = GTK_TREE_VIEW(gtk_tree_view_new());
				gtk_tree_view_set_model(view[i], model);
				gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view[i]), GTK_SELECTION_SINGLE);
			}
			g_object_unref(model); /* destroy model automatically with view */
		}

		void set_show_pad(int pad) { show_pad = pad; }

		void set_show_joy(bool flag) { show_joy_key[show_pad] = flag; }

		void set_show_key(bool flag) { show_keyboard_key[show_pad] = flag; }

		void update()
		{
			create_columns();
			repopulate();
		}

		void clear_all()
		{
			clearPAD(show_pad);
			update();
		}

		bool get_selected(int &pad, int &key, int &keysym)
		{
			GtkTreeSelection *selection;
			GtkTreeIter iter;

			selection = gtk_tree_view_get_selection(view[show_pad&1]);
			if (gtk_tree_selection_get_selected(selection, &model, &iter))
			{
				gtk_tree_model_get(model, &iter, COL_PAD_NUM, &pad, COL_VALUE, &key, COL_KEYSYM, &keysym,-1);
				return true;
			}
			return false;
		}

		void remove_selected()
		{
			int key, pad, keysym;
			if (get_selected(pad, key, keysym))
			{
				if (keysym)
					conf->keysym_map[pad].erase(keysym);
				else
					set_key(pad, key, 0);
				update();
			}
		}

		void modify_selected()
		{
			int key, pad, keysym;
			if (get_selected(pad, key, keysym))
			{
				remove_selected();
				config_key(pad,key);
				update();
			}
		}
};
keys_tree *key_tree_manager;

void populate_new_joysticks(GtkComboBox *box)
{
	char str[255];
	JoystickInfo::EnumerateJoysticks(s_vjoysticks);

	gtk_combo_box_append_text(box, "Keyboard/mouse only");
	
	vector<JoystickInfo*>::iterator it = s_vjoysticks.begin();

	// Get everything in the vector vjoysticks.
	while (it != s_vjoysticks.end())
	{
		sprintf(str, "Keyboard/mouse and %s - but: %d, axes: %d, hats: %d", (*it)->GetName().c_str(),
		        (*it)->GetNumButtons(), (*it)->GetNumAxes(), (*it)->GetNumHats());
		gtk_combo_box_append_text(box, str);
		it++;
	}
}

void set_current_joy()
{
	u32 joyid = conf->get_joyid(current_pad);
	if (JoystickIdWithinBounds(joyid))
		// 0 is special case for no gamepad. So you must increase of 1.
		gtk_combo_box_set_active(joy_choose_cbox, joyid+1);
	else
		gtk_combo_box_set_active(joy_choose_cbox, 0);
}

typedef struct
{
	GtkWidget *widget;
	int index;
	void put(const char* lbl, int i, GtkFixed *fix, int x, int y)
	{
		widget = gtk_button_new_with_label(lbl);
		index = i;

		gtk_fixed_put(fix, widget, x, y);
		gtk_widget_set_size_request(widget, 64, 24);
		g_signal_connect(GTK_OBJECT (widget), "clicked", G_CALLBACK(on_conf_key), this);
	}
} dialog_buttons;

typedef struct
{
	GtkWidget *widget;
	unsigned int mask;
	void create(GtkWidget* area, const char* label, u32 x, u32 y, u32 mask_value)
	{
		widget = gtk_check_button_new_with_label(label);
		mask = mask_value;

		gtk_fixed_put(GTK_FIXED(area), widget, x, y);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), mask & conf->options);
		g_signal_connect(GTK_OBJECT (widget), "toggled", G_CALLBACK(on_toggle_option), this);
	}
} dialog_checkbox;

void config_key(int pad, int key)
{
	bool captured = false;
	u32 key_pressed = 0;

	// save the joystick states
	UpdateJoysticks();

	while (!captured)
	{
		vector<JoystickInfo*>::iterator itjoy;

		if (PollX11KeyboardMouseEvent(key_pressed))
		{
			// special case for keyboard/mouse to handle multiple keys
			// Note: key_pressed == 0 when ESC is hit to abort the capture
			if (key_pressed > 0)
				set_keyboad_key(pad, key_pressed, key);

			captured = true;
			break;
		}

		SDL_JoystickUpdate();

		itjoy = s_vjoysticks.begin();
		while ((itjoy != s_vjoysticks.end()) && (!captured))
		{
			if ((*itjoy)->PollButtons(key_pressed)) {
				set_key(pad, key, key_pressed);
				captured = true;
				break;
			}

			if ((*itjoy)->PollAxes(key_pressed)) {
				set_key(pad, key, key_pressed);
				captured = true;
				break;
			}

			if ((*itjoy)->PollHats(key_pressed)) {
				set_key(pad, key, key_pressed);
				captured = true;
				break;
			}
			itjoy++;
		}
	}

	PAD_LOG("%s\n", KeyName(pad, key).c_str());
}

void on_conf_key(GtkButton *button, gpointer user_data)
{
	dialog_buttons *btn = (dialog_buttons *)user_data;
	int key = btn->index;

	if (key == -1) return;
	
	config_key(current_pad, key);
	key_tree_manager->update();
}

void on_remove_clicked(GtkButton *button, gpointer user_data)
{
	key_tree_manager->remove_selected();
}

void on_clear_clicked(GtkButton *button, gpointer user_data)
{
	key_tree_manager->clear_all();
}

void on_modify_clicked(GtkButton *button, gpointer user_data)
{
	key_tree_manager->modify_selected();
}

void on_view_joy_clicked(GtkToggleButton *togglebutton, gpointer user_data)
{
	key_tree_manager->set_show_joy(gtk_toggle_button_get_active(togglebutton));
	key_tree_manager->update();
}

void on_view_key_clicked(GtkToggleButton *togglebutton, gpointer user_data)
{
	key_tree_manager->set_show_key(gtk_toggle_button_get_active(togglebutton));
	key_tree_manager->update();
}

void on_toggle_option(GtkToggleButton *togglebutton, gpointer user_data)
{
	dialog_checkbox *checkbox = (dialog_checkbox*)user_data;

	if (gtk_toggle_button_get_active(togglebutton))
		conf->options |= checkbox->mask;
	else
		conf->options &= ~checkbox->mask;
}

void joy_changed(GtkComboBox *box, gpointer user_data)
{
	int joyid = gtk_combo_box_get_active(box);
	// Note position 0 of the combo box is no gamepad
	joyid--;

	// FIXME: might be a good idea to ask the user first ;)
	// FIXME: do you need to clean the previous key association. or what the user want
	// 1 : keep previous joy
	// 2 : use new joy with old key binding
	// 3 : use new joy with fresh key binding

	// unassign every joystick with this pad
	for (int i = 0; i < 2; ++i)
		if (joyid == conf->get_joyid(i)) conf->set_joyid(i, -1);

	conf->set_joyid(current_pad, joyid);
}

void pad_changed(GtkNotebook *notebook, GtkNotebookPage *notebook_page, int page, void *data)
{
	current_pad = page;
	key_tree_manager->set_show_pad(current_pad&1);
	key_tree_manager->update();
	
	// update joy
	set_current_joy();
}

//void on_forcefeedback_toggled(GtkToggleButton *togglebutton, gpointer user_data)
//{
//	int mask = PADOPTION_REVERSELX << (16 * s_selectedpad);
//
//	if (gtk_toggle_button_get_active(togglebutton))
//	{
//		conf->options |= mask;
//
//		u32 joyid = conf->get_joyid(current_pad);
//		if (JoystickIdWithinBounds(joyid)) s_vjoysticks[joyid]->TestForce();
//	}
//	else
//	{
//		conf->options &= ~mask;
//	}
//}

struct button_positions
{
	const char* label;
	u32 x,y;
};

// Warning position is important and must match the order of the gamePadValues structure
button_positions b_pos[MAX_KEYS] =
{
	{ "L2", 64, 8},
	{ "R2", 392, 8},
	{ "L1", 64, 32},
	{ "R1", 392, 32},
	
	{ "Triangle", 392, 80},
	{ "Circle", 456, 104},
	{ "Cross", 392, 128},
	{ "Square", 328, 104},
	
	{ "Select", 200, 48},
	{ "L3", 64, 224},
	{ "R3", 392, 224},
	{ "Start", 272, 48},
	
	// Arrow pad
	{ "Up", 64, 80},
	{ "Right", 128, 104},
	{ "Down", 64, 128},
	{ "Left", 0, 104},
	
	// Left Analog joystick
	{ "Up", 64, 200},
	{ "Right", 128, 224},
	{ "Down", 64, 248},
	{ "Left", 0, 224},
	
	// Right Analog joystick
	{ "Up", 392, 200},
	{ "Right", 456, 224},
	{ "Down", 392, 248},
	{ "Left", 328, 224}
};

// Warning position is important and must match the order of the PadOptions structure
#define CHECK_NBR 8
button_positions check_pos[CHECK_NBR] =
{
	{ "Enable force feedback", 40, 400},
	{ "Reverse Lx", 40, 304},
	{ "Reverse Ly", 40, 328},
	{ "Reverse Rx", 368, 304},
	{ "Reverse Ry", 368, 328},
	{ "Use mouse for left analog joy", 40, 352},
	{ "Use mouse for right analog joy", 368, 352},
	{ "Hack: Sixaxis/DS3 plugged in USB", 368, 400}
};

GtkWidget *create_notebook_page_dialog(int page, dialog_buttons btn[MAX_KEYS], dialog_checkbox checkbox[CHECK_NBR])
{
    GtkWidget *main_box;
    GtkWidget *joy_choose_frame, *joy_choose_box;
    
    GtkWidget *keys_frame, *keys_box;
    
    GtkWidget *keys_tree_box, *keys_tree_scroll;
    GtkWidget *keys_tree_clear_btn, *keys_tree_remove_btn, *keys_tree_modify_btn, *keys_tree_show_key_btn, *keys_tree_show_joy_btn;
    GtkWidget *keys_btn_box, *keys_filter_box;
    
    GtkWidget *keys_static_frame, *keys_static_box;
    GtkWidget *keys_static_area;
	
    joy_choose_cbox = GTK_COMBO_BOX(gtk_combo_box_new_text());
    populate_new_joysticks(joy_choose_cbox);
	set_current_joy();
	g_signal_connect(GTK_OBJECT (joy_choose_cbox), "changed", G_CALLBACK(joy_changed), NULL);
    
    keys_tree_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(keys_tree_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER(keys_tree_scroll), key_tree_manager->view_widget(page));
    gtk_widget_set_size_request(keys_tree_scroll, 300, 600);
    
	keys_tree_clear_btn = gtk_button_new_with_label("Clear All");
	g_signal_connect(GTK_OBJECT (keys_tree_clear_btn), "clicked", G_CALLBACK(on_clear_clicked), NULL);
	gtk_widget_set_size_request(keys_tree_clear_btn, 70, 24);
	
	keys_tree_remove_btn = gtk_button_new_with_label("Remove");
	g_signal_connect(GTK_OBJECT (keys_tree_remove_btn), "clicked", G_CALLBACK(on_remove_clicked), NULL);
    gtk_widget_set_size_request(keys_tree_remove_btn, 70, 24);
    
	keys_tree_modify_btn = gtk_button_new_with_label("Modify");
	g_signal_connect(GTK_OBJECT (keys_tree_modify_btn), "clicked", G_CALLBACK(on_modify_clicked), NULL);
    gtk_widget_set_size_request(keys_tree_modify_btn, 70, 24);

	keys_tree_show_joy_btn =  gtk_check_button_new_with_label("Show joy");
	g_signal_connect(GTK_OBJECT (keys_tree_show_joy_btn), "toggled", G_CALLBACK(on_view_joy_clicked), NULL);
    gtk_widget_set_size_request(keys_tree_show_joy_btn, 100, 24);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keys_tree_show_joy_btn), true);

	keys_tree_show_key_btn = gtk_check_button_new_with_label("Show key");
	g_signal_connect(GTK_OBJECT (keys_tree_show_key_btn), "toggled", G_CALLBACK(on_view_key_clicked), NULL);
    gtk_widget_set_size_request(keys_tree_show_key_btn, 100, 24);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keys_tree_show_key_btn), true);

    joy_choose_box = gtk_hbox_new(false, 5);
    joy_choose_frame = gtk_frame_new ("Joystick to use for this pad");
    gtk_container_add (GTK_CONTAINER(joy_choose_frame), joy_choose_box);

    keys_btn_box = gtk_hbox_new(false, 5);
	keys_filter_box = gtk_hbox_new(false, 5);
    keys_tree_box = gtk_vbox_new(false, 5);
    
    keys_static_box = gtk_hbox_new(false, 5);
    keys_static_frame = gtk_frame_new ("");
    gtk_container_add (GTK_CONTAINER(keys_static_frame), keys_static_box);
    
	keys_static_area = gtk_fixed_new();
	
	for(int i = 0; i < MAX_KEYS; i++)
	{
		btn[i].put(b_pos[i].label, i, GTK_FIXED(keys_static_area), b_pos[i].x, b_pos[i].y);
	}
    
	u32 mask = 1 << (16*page);
	for(int i = 0; i < CHECK_NBR; i++) {
		checkbox[i].create(keys_static_area, check_pos[i].label, check_pos[i].x, check_pos[i].y, mask);
		mask = mask << 1;
	}

    keys_box = gtk_hbox_new(false, 5);
    keys_frame = gtk_frame_new ("Key Settings");
    gtk_container_add (GTK_CONTAINER(keys_frame), keys_box);
    
	gtk_box_pack_start (GTK_BOX (keys_tree_box), keys_tree_scroll, true, true, 0);
	gtk_box_pack_start (GTK_BOX (keys_tree_box), keys_btn_box, false, false, 0);
	gtk_box_pack_start (GTK_BOX (keys_tree_box), keys_filter_box, false, false, 0);

	gtk_box_pack_start (GTK_BOX (keys_filter_box), keys_tree_show_joy_btn, false, false, 0);
	gtk_box_pack_start (GTK_BOX (keys_filter_box), keys_tree_show_key_btn, false, false, 0);

	gtk_box_pack_start (GTK_BOX (keys_btn_box), keys_tree_clear_btn, false, false, 0);
	gtk_box_pack_start (GTK_BOX (keys_btn_box), keys_tree_remove_btn, false, false, 0);
	gtk_box_pack_start (GTK_BOX (keys_btn_box), keys_tree_modify_btn, false, false, 0);
    
	gtk_container_add(GTK_CONTAINER(joy_choose_box), GTK_WIDGET(joy_choose_cbox));
	gtk_box_pack_start(GTK_BOX (keys_box), keys_tree_box, false, false, 0);
	gtk_container_add(GTK_CONTAINER(keys_box), keys_static_area);

    main_box = gtk_vbox_new(false, 5);
	gtk_container_add(GTK_CONTAINER(main_box), joy_choose_frame);
	gtk_container_add(GTK_CONTAINER(main_box), keys_frame);

	return main_box;
}

void DisplayDialog()
{
    int return_value;

    GtkWidget *dialog;

	GtkWidget *notebook, *page[2];
	GtkWidget *page_label[2];

	dialog_buttons btn[2][MAX_KEYS];
	dialog_checkbox checkbox[2][CHECK_NBR];

	LoadConfig();
	key_tree_manager = new keys_tree;
	key_tree_manager->init();

    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons (
		"OnePAD Config",
		NULL, /* parent window*/
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
		GTK_STOCK_APPLY,
			GTK_RESPONSE_APPLY,
		GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
		NULL);

	notebook = gtk_notebook_new();

	page_label[0] = gtk_label_new("Pad 1");
	page_label[1] = gtk_label_new("Pad 2");
	for (int i = 0; i < 2; i++) {
		page[i] = create_notebook_page_dialog(i, btn[i], checkbox[i]);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page[i], page_label[i]);
	}
	g_signal_connect(GTK_OBJECT (notebook), "switch-page", G_CALLBACK(pad_changed), NULL);

    gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), notebook);
    
    key_tree_manager->update();
    
    gtk_widget_show_all (dialog);

	do {
		return_value = gtk_dialog_run (GTK_DIALOG (dialog));
		if (return_value == GTK_RESPONSE_APPLY || return_value == GTK_RESPONSE_ACCEPT) {
			SaveConfig();
		}
	} while (return_value == GTK_RESPONSE_APPLY);

	LoadConfig();
	delete key_tree_manager;
    gtk_widget_destroy (dialog);
}
