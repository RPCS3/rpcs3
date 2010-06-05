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

extern std::string s_strIniPath;

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

void DefaultValues()
{
	set_key(0, PAD_L2, XK_a);
	set_key(0, PAD_R2, XK_semicolon);
	set_key(0, PAD_L1, XK_w);
	set_key(0, PAD_R1, XK_p);
	set_key(0, PAD_TRIANGLE, XK_i);
	set_key(0, PAD_CIRCLE, XK_l);
	set_key(0, PAD_CROSS, XK_k);
	set_key(0, PAD_SQUARE, XK_j);
	set_key(0, PAD_SELECT, XK_v);
	set_key(0, PAD_START, XK_n);
	set_key(0, PAD_UP, XK_e);
	set_key(0, PAD_RIGHT, XK_f);
	set_key(0, PAD_DOWN, XK_d);
	set_key(0, PAD_LEFT, XK_s);
}

void SaveConfig()
{
	FILE *f;
	char cfg[255];

	const std::string iniFile(s_strIniPath + "OnePAD.ini");
	f = fopen(iniFile.c_str(), "w");
	if (f == NULL)
	{
		printf("ZeroPAD: failed to save ini %s\n", iniFile.c_str());
		return;
	}

	for (int pad = 0; pad < 2 * MAX_SUB_KEYS; pad++)
	{
		for (int key = 0; key < MAX_KEYS; key++)
		{
			fprintf(f, "[%d][%d] = 0x%lx\n", pad, key, get_key(pad,key));
		}
	}
	fprintf(f, "log = %d\n", conf.log);
	fprintf(f, "options = %d\n", conf.options);
	fclose(f);
}

void LoadConfig()
{
	FILE *f;
	char str[256];

	memset(&conf, 0, sizeof(conf));
	DefaultValues();
	conf.log = 0;

	const std::string iniFile(s_strIniPath + "OnePAD.ini");
	f = fopen(iniFile.c_str(), "r");
	if (f == NULL)
	{
		printf("OnePAD: failed to load ini %s\n", iniFile.c_str());
		SaveConfig(); //save and return
		return;
	}

	for (int pad = 0; pad < 2 * MAX_SUB_KEYS; pad++)
	{
		for (int key = 0; key < MAX_KEYS; key++)
		{
			sprintf(str, "[%d][%d] = 0x%%x\n", pad, key);
			u32 temp;

			if (fscanf(f, str, &temp) == 0) temp = 0;
			set_key(pad, key, temp);
		}
	}
	fscanf(f, "log = %d\n", &conf.log);
	fscanf(f, "options = %d\n", &conf.options);
	fclose(f);
}
