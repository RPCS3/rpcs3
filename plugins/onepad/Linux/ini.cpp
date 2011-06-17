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

#include <string.h>
#include <gtk/gtk.h>

#include "joystick.h"
#include "keyboard.h"
#include "onepad.h"
#include "linux.h"

extern std::string s_strIniPath;

string KeyName(int pad, int key, int keysym)
{
	string tmp;
	tmp.resize(28);

	if (keysym) {
		if (keysym < 10) {
			// mouse
			switch (keysym) {
				case 1: sprintf(&tmp[0], "Mouse Left"); break;
				case 2: sprintf(&tmp[0], "Mouse Middle"); break;
				case 3: sprintf(&tmp[0], "Mouse Right"); break;
				default: // Use only number for extra button
						sprintf(&tmp[0], "Mouse %d", keysym);
			}
		} else {
			// keyboard
			char* pstr = XKeysymToString(keysym);
			if (pstr != NULL) tmp = pstr;
		}
	} else {
		// joystick
		KeyType k = type_of_joykey(pad, key);
		switch (k)
		{
			case PAD_JOYBUTTONS:
				{
					int button = key_to_button(pad, key);
					sprintf(&tmp[0], "JBut %d", button);
					break;
				}
			case PAD_AXIS:
				{
					if (key_to_axis_type(pad,key))
						sprintf(&tmp[0], "JAxis %d Full", key_to_axis(pad, key));
					else
						sprintf(&tmp[0], "JAxis %d Half%s", key_to_axis(pad, key), key_to_axis_sign(pad, key) ? "-" : "+");
					break;
				}
			case PAD_HAT:
				{
					int axis = key_to_axis(pad, key);
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
			default: break;
		}
	}

	return tmp;
}

void DefaultKeyboardValues()
{
	set_keyboad_key(0, XK_a, PAD_L2);
	set_keyboad_key(0, XK_semicolon, PAD_R2);
	set_keyboad_key(0, XK_w, PAD_L1);
	set_keyboad_key(0, XK_p, PAD_R1);
	set_keyboad_key(0, XK_i, PAD_TRIANGLE);
	set_keyboad_key(0, XK_l, PAD_CIRCLE);
	set_keyboad_key(0, XK_k, PAD_CROSS);
	set_keyboad_key(0, XK_j, PAD_SQUARE);
	set_keyboad_key(0, XK_v, PAD_SELECT);
	set_keyboad_key(0, XK_n, PAD_START);
	set_keyboad_key(0, XK_e, PAD_UP);
	set_keyboad_key(0, XK_f, PAD_RIGHT);
	set_keyboad_key(0, XK_d, PAD_DOWN);
	set_keyboad_key(0, XK_s, PAD_LEFT);
}

void SaveConfig()
{
	FILE *f;

	const std::string iniFile(s_strIniPath + "OnePAD.ini");
	f = fopen(iniFile.c_str(), "w");
	if (f == NULL)
	{
		printf("OnePAD: failed to save ini %s\n", iniFile.c_str());
		return;
	}

	fprintf(f, "log = %d\n", conf->log);
	fprintf(f, "options = %d\n", conf->options);
	fprintf(f, "mouse_sensibility = %d\n", conf->sensibility);
	fprintf(f, "joy_pad_map = %d\n", conf->joyid_map);

	for (int pad = 0; pad < 2; pad++)
	{
		for (int key = 0; key < MAX_KEYS; key++)
		{
			fprintf(f, "[%d][%d] = 0x%x\n", pad, key, get_key(pad,key));
		}
	}

	map<u32,u32>::iterator it;
	for (int pad = 0; pad < 2 ; pad++)
		for (it = conf->keysym_map[pad].begin(); it != conf->keysym_map[pad].end(); ++it)
				fprintf(f, "PAD %d:KEYSYM 0x%x = %d\n", pad, it->first, it->second);

	fclose(f);
}

void LoadConfig()
{
	FILE *f;
	char str[256];
	bool have_user_setting = false;

	if (!conf)
		conf = new PADconf;

	conf->init();

	const std::string iniFile(s_strIniPath + "OnePAD.ini");
	f = fopen(iniFile.c_str(), "r");
	if (f == NULL)
	{
		printf("OnePAD: failed to load ini %s\n", iniFile.c_str());
		SaveConfig(); //save and return
		return;
	}

	u32 value;
	if (fscanf(f, "log = %d\n", &value) == 0) return;
	conf->log = value;
	if (fscanf(f, "options = %d\n", &value) == 0) return;
	conf->options = value;
	if (fscanf(f, "mouse_sensibility = %d\n", &value) == 0) return;
	conf->sensibility = value;
	if (fscanf(f, "joy_pad_map = %d\n", &value) == 0) return;
	conf->joyid_map = value;

	for (int pad = 0; pad < 2; pad++)
	{
		for (int key = 0; key < MAX_KEYS; key++)
		{
			sprintf(str, "[%d][%d] = 0x%%x\n", pad, key);
			u32 temp = 0;

			if (fscanf(f, str, &temp) == 0) temp = 0;
			set_key(pad, key, temp);
			if (temp && pad == 0) have_user_setting = true;
		}
	}

	u32 pad;
	u32 keysym;
	u32 index;
	while( fscanf(f, "PAD %d:KEYSYM 0x%x = %d\n", &pad, &keysym, &index) != EOF ) {
		set_keyboad_key(pad, keysym, index);
		if(pad == 0) have_user_setting = true;
	}

	fclose(f);

	if (!have_user_setting) DefaultKeyboardValues();
}
