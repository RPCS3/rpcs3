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

s32  _PADopen(void *pDsp)
{
	GSdsp = *(Display**)pDsp;
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
	int joyid = -1;
	
	for (int p = 0; p < PADSUBKEYS; p++)
	{
		for (int i = 0; i < PADKEYS; ++i)
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
	
	for (int i = 0; i < PADKEYS; i++)
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
								Analog::ConfigurePad(i, pad, value);
							else 
								Analog::ResetPad(i, pad);
							break;
					}
				break;
				}
	#ifdef EXPERIMENTAL_POV_CODE
			case PAD_HAT:
				{
					int value = SDL_JoystickGetHat((pjoy)->GetJoy(), key_to_axis(cpad, i));
					
					if ((value != SDL_HAT_CENTERED) && (key_to_hat_dir(cpad, i) == value))
					{
						if 	((value == SDL_HAT_UP) || 
							(value == SDL_HAT_RIGHT) || 
							(value == SDL_HAT_DOWN) || 
							(value == SDL_HAT_LEFT))
						{
							set_bit(status[pad], i);
							PAD_LOG("Registered %s. Set (%d)\n", HatName(value), i);
						}
						else
						{
							clear_bit(status[pad], i);
						}
					}
					else
					{
						clear_bit(status[pad], i);
					}
					break;
				}
	#endif
			case PAD_POV:
				{
					int value = SDL_JoystickGetAxis((pjoy)->GetJoy(), key_to_axis(cpad, i));
					
					if (key_to_pov_sign(cpad, i) && (value < -2048))
						clear_bit(status[pad], i);
					else if (!key_to_pov_sign(cpad, i) && (value > 2048))
						clear_bit(status[pad], i);
					else
						set_bit(status[pad], i);
					break;
				}
			default: break;
			}
		}
	}
}

string KeyName(int pad, int key)
{
	string tmp;
	KeyType k = type_of_key(pad, key);
	
	switch (k)
		{
			case PAD_KEYBOARD:
			{
				char* pstr = KeysymToChar(pad_to_key(pad, key));
				if (pstr != NULL) tmp = pstr;
				break;
			}
			case PAD_JOYBUTTONS:
			{
				int button = key_to_button(pad, key);
				tmp.resize(28);
			
				sprintf(&tmp[0], "JBut %d", button);
				break;
			}
			case PAD_JOYSTICK:
			{
				int axis = key_to_axis(pad, key);
				tmp.resize(28);
			
				sprintf(&tmp[0], "JAxis %d", axis);
				break;
			}
#ifdef EXPERIMENTAL_POV_CODE
			case PAD_HAT:
			{
				int axis = key_to_axis(pad, key);
				tmp.resize(28);
			
				switch(key_to_hat_dir(pad, key))
				{
					case SDL_HAT_UP:
						sprintf(&tmp[0], "JPOVU-%d", axis);
						break;
					
					case SDL_HAT_RIGHT:
						sprintf(&tmp[0], "JPOVR-%d", axis);
						break;
					
					case SDL_HAT_DOWN:
						sprintf(&tmp[0], "JPOVD-%d", axis);
						break;
					
					case SDL_HAT_LEFT:
						sprintf(&tmp[0], "JPOVL-%d", axis);
						break;
				}
				break;
			}
#endif
			case PAD_POV:
			{
				tmp.resize(28);
				sprintf(&tmp[0], "JPOV %d%s", key_to_axis(pad, key), key_to_pov_sign(pad, key) ? "-" : "+");
				break;
			}
			default: break;
		}
	return tmp;
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

		gtk_object_set_user_data(GTK_OBJECT(Btn), (void*)(PADKEYS * pad + i));
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
	
	int pad = id / PADKEYS;
	int key = id % PADKEYS;

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
			captured = true;
			break;
		}
		
		SDL_JoystickUpdate();
		
		itjoy = s_vjoysticks.begin();
		while ((itjoy != s_vjoysticks.end()) && (!captured))
		{
			int jbutton, direction;
			
			pkey = get_key(pad, key);
			if ((*itjoy)->PollButtons(jbutton, pkey))
			{
				set_key(pad, key, pkey);
				captured = true;
				break;
			}

			bool negative = false;
			bool pov = (!((key == PAD_RY) || (key == PAD_LY) || (key == PAD_RX) || (key == PAD_LX)));
			
			if ((*itjoy)->PollAxes(pov, jbutton, negative, pkey))
			{
				set_key(pad, key, pkey);
				captured = true;
				break;
			}
			
#ifdef EXPERIMENTAL_POV_CODE
			if ((*itjoy)->PollHats(jbutton, direction, pkey))
			{
				set_key(pad, key, pkey);
				captured = true;
				break;
			}
#endif
			itjoy++;
		}
	}
	
	init_tree_view();
}

EXPORT_C_(void) PADconfigure()
{
	char strcurdir[256];
	getcwd(strcurdir, 256);
	s_strIniPath = strcurdir;
	s_strIniPath += "/inis/OnePAD.ini";

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
