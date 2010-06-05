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

extern string KeyName(int pad, int key);

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

EXPORT_C_(void) PADabout()
{
	SysMessage("OnePad is a rewrite of Zerofrog's ZeroPad, done by arcum42.");
}

EXPORT_C_(s32) PADtest()
{
	return 0;
}

s32  _PADopen(void *pDsp)
{
    GtkScrolledWindow *win;

    win = *(GtkScrolledWindow**) pDsp;

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


EXPORT_C_(void) PADconfigure()
{
	LoadConfig();

	DisplayDialog();
	return;
}
