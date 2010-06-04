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

#include "linux.h"
#include <gdk/gdkx.h>

Display *GSdsp;

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

extern string KeyName(int pad, int key);

s32  _PADopen(void *pDsp)
{
    GtkScrolledWindow *win;

    win = *(GtkScrolledWindow**) pDsp;
    PAD_LOG("It is a '%s'", GTK_OBJECT_TYPE_NAME(&win));

	if (GTK_IS_WIDGET(win))
	{
	    // Since we have a GtkScrolledWindow, for now we'll grab whatever display
	    // comes along instead. Later, we can fiddle with this, but I'm not sure the
	    // best way to get a Display* out of a GtkScrolledWindow. A GtkWindow I might
	    // be able to manage... --arcum42
        GSdsp = GDK_DISPLAY();
	}
	else
	{
        GSdsp = *(Display**)pDsp;
	}

    SetAutoRepeat(false);
	return 0;
}

void _PADclose()
{
	SetAutoRepeat(true);

	vector<JoystickInfo*>::iterator it = s_vjoysticks.begin();

	// Delete everything in the vector vjoysticks.
	while (it != s_vjoysticks.end())
	{
		delete *it;
		it ++;
	}

	s_vjoysticks.clear();
}

int _GetJoystickIdFromPAD(int pad)
{
	// select the right joystick id
	u32 joyid = -1;

	for (int p = 0; p < MAX_SUB_KEYS; p++)
	{
		for (int i = 0; i < MAX_KEYS; ++i)
		{
			KeyType k = type_of_key(PadEnum[pad][p],i);

			if (k == PAD_JOYSTICK || k == PAD_JOYBUTTONS)
			{
				joyid = key_to_joystick_id(PadEnum[pad][p],i);
				return joyid;
			}
		}
	}

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

EXPORT_C_(void) PADupdate(int pad)
{
	// Poll keyboard.
	PollForKeyboardInput(pad);

	// joystick info
	SDL_JoystickUpdate();

	for (int i = 0; i < MAX_KEYS; i++)
	{
		int cpad = PadEnum[pad][0];

		if (JoystickIdWithinBounds(key_to_joystick_id(cpad, i)))
		{
			JoystickInfo* pjoy = s_vjoysticks[key_to_joystick_id(cpad, i)];
			int pad = (pjoy)->GetPAD();

			switch (type_of_key(cpad, i))
			{
				case PAD_JOYBUTTONS:
				{
					int value = SDL_JoystickGetButton((pjoy)->GetJoy(), key_to_button(cpad, i));

					if (value)
						clear_bit(status[pad], i); // released
					else
						set_bit(status[pad], i); // pressed
					break;
				}
			case PAD_HAT:
				{
					int value = SDL_JoystickGetHat((pjoy)->GetJoy(), key_to_axis(cpad, i));

					if (key_to_hat_dir(cpad, i) == value)
					{
						clear_bit(status[pad], i);
						//PAD_LOG("Registered %s\n", HatName(value), i);
						//PAD_LOG("%s\n", KeyName(cpad, i).c_str());
					}
					else
					{
						set_bit(status[pad], i);
					}
					break;
				}
			case PAD_POV:
				{
					int value = SDL_JoystickGetAxis((pjoy)->GetJoy(), key_to_axis(cpad, i));

					PAD_LOG("%s: %d (%d)\n", KeyName(cpad, i).c_str(), value, key_to_pov_sign(cpad, i));
					if (key_to_pov_sign(cpad, i) && (value < -2048))
					{
						//PAD_LOG("%s Released+.\n", KeyName(cpad, i).c_str());
						clear_bit(status[pad], i);
					}
					else if (!key_to_pov_sign(cpad, i) && (value > 2048))
					{
						//PAD_LOG("%s Released-\n", KeyName(cpad, i).c_str());
						clear_bit(status[pad], i);
					}
					else
					{
						//PAD_LOG("%s Pressed.\n", KeyName(cpad, i).c_str());
						set_bit(status[pad], i);
					}
					break;
				}
				case PAD_JOYSTICK:
				{
					int value = SDL_JoystickGetAxis((pjoy)->GetJoy(), key_to_axis(cpad, i));

					switch (i)
					{
						case PAD_LX:
						case PAD_LY:
						case PAD_RX:
						case PAD_RY:
							if (abs(value) > (pjoy)->GetDeadzone(value))
								Analog::ConfigurePad(pad, i, value);
							else
								Analog::ResetPad(pad, i);
							break;
					}
					break;
				}
			default: break;
			}
		}
	}
}

void UpdateConf(int pad)
{
	initLogging();
	s_selectedpad = pad;
	init_tree_view();
	int i;
	GtkWidget *Btn;
	for (i = 0; i < ArraySize(s_pGuiKeyMap); i++)
	{

		if (s_pGuiKeyMap[i] == NULL) continue;

		Btn = lookup_widget(Conf, s_pGuiKeyMap[i]);
		if (Btn == NULL)
		{
			PAD_LOG("OnePAD: cannot find key %s\n", s_pGuiKeyMap[i]);
			continue;
		}

		gtk_object_set_user_data(GTK_OBJECT(Btn), (void*)(MAX_KEYS * pad + i));
	}

	// check bounds
	int joyid = _GetJoystickIdFromPAD(pad);

	if (JoystickIdWithinBounds(joyid))
		gtk_combo_box_set_active(GTK_COMBO_BOX(s_devicecombo), joyid); // select the combo
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(s_devicecombo), s_vjoysticks.size()); // no gamepad

	int padopts = conf.options >> (16 * pad);

	set_checked(Conf, "checkbutton_reverselx", padopts & PADOPTION_REVERTLX);
	set_checked(Conf, "checkbutton_reversely", padopts & PADOPTION_REVERTLY);
	set_checked(Conf, "checkbutton_reverserx", padopts & PADOPTION_REVERTRX);
	set_checked(Conf, "checkbutton_reversery", padopts & PADOPTION_REVERTRY);
	set_checked(Conf, "forcefeedback", padopts & PADOPTION_FORCEFEEDBACK);
}

int GetLabelId(GtkWidget *label)
{
	if (label == NULL)
	{
		PAD_LOG("couldn't find correct label\n");
		return -1;
	}

	return (int)(uptr)gtk_object_get_user_data(GTK_OBJECT(label));
}

void OnConf_Key(GtkButton *button, gpointer user_data)
{
	bool captured = false;

	const char* buttonname = gtk_widget_get_name(GTK_WIDGET(button));
	GtkWidget* label =  lookup_widget(Conf, buttonname);

	int id = GetLabelId(label);

	if (id == -1) return;

	int pad = id / MAX_KEYS;
	int key = id % MAX_KEYS;

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

	init_tree_view();
}

void populate_joysticks()
{
	// recreate
	JoystickInfo::EnumerateJoysticks(s_vjoysticks);

	s_devicecombo = lookup_widget(Conf, "joydevicescombo");

	// fill the combo
	char str[255];
	vector<JoystickInfo*>::iterator it = s_vjoysticks.begin();

	// Delete everything in the vector vjoysticks.
	while (it != s_vjoysticks.end())
	{
		sprintf(str, "%d: %s - but: %d, axes: %d, hats: %d", (*it)->GetId(), (*it)->GetName().c_str(),
		        (*it)->GetNumButtons(), (*it)->GetNumAxes(), (*it)->GetNumHats());
		gtk_combo_box_append_text(GTK_COMBO_BOX(s_devicecombo), str);
		it++;
	}

	gtk_combo_box_append_text(GTK_COMBO_BOX(s_devicecombo), "No Gamepad");

	UpdateConf(0);
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
}

typedef struct
{
	GtkWidget *widget;
	void put(char* lbl, GtkFixed *fix, int x, int y)
	{
		widget = gtk_button_new_with_label(lbl);
		gtk_fixed_put(fix, widget, x, y);
		gtk_widget_set_size_request(widget, 64, 24);
	}
} dialog_buttons;

void DisplayDialog()
{
    int return_value;

    GtkWidget *dialog;
    GtkWidget *main_frame, *main_box;

    GtkWidget *pad_choose_frame, *pad_choose_box;
    GtkWidget *pad_choose_cbox;
    
    GtkWidget *joy_choose_frame, *joy_choose_box;
    GtkWidget *joy_choose_cbox;
    
    GtkWidget *keys_frame, *keys_box;
    
    GtkWidget *keys_tree_frame, *keys_tree_box;
    GtkWidget *keys_tree_view;
    GtkWidget *keys_tree_clear_btn, *keys_tree_remove_btn;
    GtkWidget *keys_btn_box, *keys_btn_frame;
    
    GtkWidget *keys_static_frame, *keys_static_box;
    GtkWidget *keys_static_area;
    dialog_buttons btn[29];
    GtkWidget *rev_lx_check, *rev_ly_check, *force_feedback_check, *rev_rx_check, *rev_ry_check;
	
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

    pad_choose_cbox = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(pad_choose_cbox), "Pad 1");
    gtk_combo_box_append_text(GTK_COMBO_BOX(pad_choose_cbox), "Pad 2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(pad_choose_cbox), "Pad 1 (alt)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(pad_choose_cbox), "Pad 2 (alt)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(pad_choose_cbox), 0);
    
    joy_choose_cbox = gtk_combo_box_new_text ();
    populate_new_joysticks(GTK_COMBO_BOX(joy_choose_cbox));
    gtk_combo_box_set_active(GTK_COMBO_BOX(joy_choose_cbox), Get_Current_Joystick());

    keys_tree_view = gtk_tree_view_new();
	keys_tree_clear_btn = gtk_button_new_with_label("Clear All");
	keys_tree_remove_btn = gtk_button_new_with_label("Remove");
    
    main_box = gtk_vbox_new(false, 5);
    main_frame = gtk_frame_new ("Onepad Config");
    gtk_container_add (GTK_CONTAINER(main_frame), main_box);

    pad_choose_box = gtk_vbox_new(false, 5);
    pad_choose_frame = gtk_frame_new ("Choose a Pad to modify:");
    gtk_container_add (GTK_CONTAINER(pad_choose_frame), pad_choose_box);

    joy_choose_box = gtk_vbox_new(false, 5);
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
	btn[0].put("L1", GTK_FIXED(keys_static_area), static_offset + 64, 32);
	btn[1].put("L2", GTK_FIXED(keys_static_area), static_offset + 64, 8);
	btn[2].put("L3", GTK_FIXED(keys_static_area), static_offset + 200, 8);
	btn[3].put("R1", GTK_FIXED(keys_static_area), static_offset + 392, 32);
	btn[4].put("R2", GTK_FIXED(keys_static_area), static_offset + 392, 8);
	btn[5].put("R3", GTK_FIXED(keys_static_area), static_offset + 272, 8);
	
	btn[6].put("Select", GTK_FIXED(keys_static_area), static_offset + 200, 48);
	btn[7].put("Start", GTK_FIXED(keys_static_area), static_offset + 272, 48);
	
	// Arrow pad
	btn[8].put("Up", GTK_FIXED(keys_static_area), static_offset + 64, 80);
	btn[9].put("Down", GTK_FIXED(keys_static_area), static_offset + 64, 128);
	btn[10].put("Left", GTK_FIXED(keys_static_area), static_offset + 0, 104);
	btn[11].put("Right", GTK_FIXED(keys_static_area), static_offset + 128, 104);
	
	btn[12].put("Analog", GTK_FIXED(keys_static_area), static_offset + 232, 104);
	
	btn[13].put("Triangle", GTK_FIXED(keys_static_area), static_offset + 392, 80);
	btn[14].put("Square", GTK_FIXED(keys_static_area), static_offset + 328, 104);
	btn[15].put("Circle", GTK_FIXED(keys_static_area), static_offset + 456, 104);
	btn[16].put("Cross", GTK_FIXED(keys_static_area), static_offset + 392,128);
	
	// Joy 1
	btn[17].put("Up", GTK_FIXED(keys_static_area), static_offset + 64, 240);
	btn[18].put("Down", GTK_FIXED(keys_static_area), static_offset + 64, 312);
	btn[19].put("Left", GTK_FIXED(keys_static_area), static_offset + 0, 272);
	btn[20].put("Right", GTK_FIXED(keys_static_area), static_offset + 128, 272);
	btn[21].put("LX", GTK_FIXED(keys_static_area), static_offset + 64, 264);
	btn[22].put("LY", GTK_FIXED(keys_static_area), static_offset + 64, 288);
	
	// Joy 2
	btn[23].put("Up", GTK_FIXED(keys_static_area), static_offset + 392, 240);
	btn[24].put("Down", GTK_FIXED(keys_static_area), static_offset + 392, 312);
	btn[25].put("Left", GTK_FIXED(keys_static_area), static_offset + 328, 272);
	btn[26].put("Right", GTK_FIXED(keys_static_area), static_offset + 456, 272);
	btn[27].put("RX", GTK_FIXED(keys_static_area), static_offset + 392, 264);
	btn[28].put("RY", GTK_FIXED(keys_static_area), static_offset + 392, 288);
    
    keys_box = gtk_hbox_new(false, 5);
    keys_frame = gtk_frame_new ("Key Settings");
    gtk_container_add (GTK_CONTAINER(keys_frame), keys_box);
    
	gtk_box_pack_start (GTK_BOX (keys_tree_box), keys_tree_view, true, true, 0);
	gtk_box_pack_end (GTK_BOX (keys_btn_box), keys_tree_clear_btn, false, false, 0);
	gtk_box_pack_end (GTK_BOX (keys_btn_box), keys_tree_remove_btn, false, false, 0);
    
	gtk_container_add(GTK_CONTAINER(pad_choose_box), pad_choose_cbox);
	gtk_container_add(GTK_CONTAINER(joy_choose_box), joy_choose_cbox);
	gtk_container_add(GTK_CONTAINER(keys_tree_box), keys_btn_frame);
	gtk_box_pack_start (GTK_BOX (keys_box), keys_tree_frame, true, true, 0);
	gtk_container_add(GTK_CONTAINER(keys_box), keys_static_area);

	gtk_container_add(GTK_CONTAINER(main_box), pad_choose_frame);
	gtk_container_add(GTK_CONTAINER(main_box), joy_choose_frame);
	gtk_container_add(GTK_CONTAINER(main_box), keys_frame);

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_frame);
    gtk_widget_show_all (dialog);

    return_value = gtk_dialog_run (GTK_DIALOG (dialog));

    if (return_value == GTK_RESPONSE_ACCEPT)
    {
//		DebugEnabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(debug_check));
//    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(int_box)) != -1)
//    		Interpolation = gtk_combo_box_get_active(GTK_COMBO_BOX(int_box));
//
//    	EffectsDisabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(effects_check));
//
//    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(reverb_box)) != -1)
//			ReverbBoost = gtk_combo_box_get_active(GTK_COMBO_BOX(reverb_box));
//
//    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(mod_box)) != 1)
//			OutputModule = 0;
//		else
//			OutputModule = FindOutputModuleById( PortaudioOut->GetIdent() );
//
//    	SndOutLatencyMS = gtk_range_get_value(GTK_RANGE(latency_slide));
//    	
//    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(sync_box)) != -1)
//			SynchMode = gtk_combo_box_get_active(GTK_COMBO_BOX(sync_box));
    }

    gtk_widget_destroy (dialog);
}

EXPORT_C_(void) PADconfigure()
{
	LoadConfig();

	//DisplayDialog();
	//return;
	Conf = create_Conf();
	
	populate_joysticks();

	gtk_widget_show_all(Conf);
	gtk_main();
}
