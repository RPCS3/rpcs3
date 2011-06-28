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
#include "keyboard.h"

#include <string.h>
#include <gtk/gtk.h>
#include "linux.h"
#include <gdk/gdkx.h>

Display *GSdsp;
Window	GSwin;

void SysMessage(const char *fmt, ...)
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
	GSdsp = *(Display**)pDsp;
	GSwin = (Window)*(((u32*)pDsp)+1);

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

void PollForJoystickInput(int cpad)
{
	int joyid = conf->get_joyid(cpad);
	if (!JoystickIdWithinBounds(joyid)) return;

	SDL_JoystickUpdate();
	for (int i = 0; i < MAX_KEYS; i++)
	{
		JoystickInfo* pjoy = s_vjoysticks[joyid];

		switch (type_of_joykey(cpad, i))
		{
			case PAD_JOYBUTTONS:
				{

					int value = SDL_JoystickGetButton((pjoy)->GetJoy(), key_to_button(cpad, i));
					if (value)
						key_status->press(cpad, i);
					else
						key_status->release(cpad, i);

					break;
				}
			case PAD_HAT:
				{
					int value = SDL_JoystickGetHat((pjoy)->GetJoy(), key_to_axis(cpad, i));

					// key_to_hat_dir and SDL_JoystickGetHat are a 4 bits bitmap, one for each directions. Only 1 bit can be high for
					// key_to_hat_dir. SDL_JoystickGetHat handles diagonal too (2 bits) so you must check the intersection
					// '&' not only equality '=='. -- Gregory
					if (key_to_hat_dir(cpad, i) & value)
						key_status->press(cpad, i);
					else
						key_status->release(cpad, i);

					break;
				}
			case PAD_AXIS:
				{
					int value = pjoy->GetAxisFromKey(cpad, i);
					bool sign = key_to_axis_sign(cpad, i);
					bool full_axis = key_to_axis_type(cpad, i);

					if (IsAnalogKey(i)) {
						if (abs(value) > pjoy->GetDeadzone())
							key_status->press(cpad, i, value);
						else
							key_status->release(cpad, i);

					} else {
						if (full_axis) {
							value += 0x8000;
							if (value > pjoy->GetDeadzone())
								key_status->press(cpad, i, min(value/256 , 0xFF));
							else
								key_status->release(cpad, i);

						} else {
							if (sign && (-value > pjoy->GetDeadzone()))
								key_status->press(cpad, i, min(-value /128, 0xFF));
							else if (!sign && (value > pjoy->GetDeadzone()))
								key_status->press(cpad, i, min(value /128, 0xFF));
							else
								key_status->release(cpad, i);
						}
					}
				}
			default: break;
		}
	}
}

EXPORT_C_(void) PADupdate(int pad)
{
	// Actually PADupdate is always call with pad == 0. So you need to update both
	// pads -- Gregory
	for (int cpad = 0; cpad < 2; cpad++) {
		// Poll keyboard/mouse event
		key_status->keyboard_state_acces(cpad);
		PollForX11KeyboardInput(cpad);

		// Get joystick state
		key_status->joystick_state_acces(cpad);
		PollForJoystickInput(cpad);

		key_status->commit_status(cpad);
	}
}

EXPORT_C_(void) PADconfigure()
{
	LoadConfig();

	DisplayDialog();
	return;
}
