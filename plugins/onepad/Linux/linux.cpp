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

EXPORT_C_(void) PADconfigure()
{
	LoadConfig();

	Conf = create_Conf();

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

	gtk_widget_show_all(Conf);
	gtk_main();
}
