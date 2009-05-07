/*  ZeroPAD - author: zerofrog(@gmail.com)
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

#define JOYSTICK_SUPPORT
#ifdef JOYSTICK_SUPPORT
#include "joystick.h"
#endif

#include "zeropad.h"

extern "C"
{
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

Display *GSdsp;
static pthread_spinlock_t s_mutexStatus;
static u32 s_keyPress[2], s_keyRelease[2]; // thread safe

extern GtkWidget *Conf, *s_devicecombo;
extern string s_strIniPath;

static const char* s_pGuiKeyMap[] = 
{ 
	"L2", "R2", "L1", "R1",
	"Triangle", "Circle", "Cross", "Square",
	"Select", "L3", "R3", "Start",
	"Up", "Right", "Down", "Left",
	"Lx", "Rx", "Ly", "Ry"
};

string GetLabelFromButton(const char* buttonname)
{
	string label = "e";
	label += buttonname;
	return label;
}

s32  _PADopen(void *pDsp)
{
	GSdsp = *(Display**)pDsp;
	pthread_spin_init(&s_mutexStatus, PTHREAD_PROCESS_PRIVATE);
	s_keyPress[0] = s_keyPress[1] = 0;
	s_keyRelease[0] = s_keyRelease[1] = 0;
	XAutoRepeatOff(GSdsp);

#ifdef JOYSTICK_SUPPORT
	JoystickInfo::EnumerateJoysticks(s_vjoysticks);
#endif

	return 0;
}

void _PADclose()
{
	pthread_spin_destroy(&s_mutexStatus);
	XAutoRepeatOn(GSdsp);

	vector<JoystickInfo*>::iterator it = s_vjoysticks.begin();
	
	// Delete everything in the vector vjoysticks.
	while (it != s_vjoysticks.end())
	{
		delete *it;
		it ++;
	}
	
	s_vjoysticks.clear();
}

void _PADupdate(int pad)
{
	pthread_spin_lock(&s_mutexStatus);
	status[pad] |= s_keyRelease[pad];
	status[pad] &= ~s_keyPress[pad];
	s_keyRelease[pad] = 0;
	s_keyPress[pad] = 0;
	pthread_spin_unlock(&s_mutexStatus);
}

int _GetJoystickIdFromPAD(int pad)
{
	// select the right joystick id
	int joyid = -1;
	
	for (int p = 0; p < PADSUBKEYS; p++)
	{
		for (int i = 0; i < PADKEYS; ++i)
		{
			if (IS_JOYSTICK(conf.keys[(PadEnum[pad][p])][i]) || IS_JOYBUTTONS(conf.keys[(PadEnum[pad][p])][i]))
			{
				joyid = PAD_GETJOYID(conf.keys[(PadEnum[pad][p])][i]);
				return joyid;
			}
		}
	}
	return joyid;
}

void CALLBACK PADupdate(int pad)
{
	int i;
	XEvent E;
	int keyPress = 0, keyRelease = 0;
	KeySym key;

	// keyboard input
	while (XPending(GSdsp) > 0)
	{
		XNextEvent(GSdsp, &E);
		switch (E.type)
		{
			case KeyPress:
				key = XLookupKeysym((XKeyEvent *) & E, 0);

#ifdef ANALOG_CONTROLS_HACK
				switch (key)
				{
					case KEY_PAD_LX_LEFT:
						g_lanalog[pad].x = DEF_VALUE / 256;
						if (conf.options&PADOPTION_REVERTLX) g_lanalog[pad].x = -g_lanalog[pad].x;
						g_lanalog[pad].x += 0x80;
						break;
					case KEY_PAD_LX_RIGHT:
						g_lanalog[pad].x = -DEF_VALUE / 256;
						if (conf.options&PADOPTION_REVERTLX) g_lanalog[pad].x = -g_lanalog[pad].x;
						g_lanalog[pad].x += 0x80;
						break;
					case KEY_PAD_LY_UP:
						g_lanalog[pad].y = DEF_VALUE / 256;
						if (conf.options&PADOPTION_REVERTLY) g_lanalog[pad].y = -g_lanalog[pad].y;
						g_lanalog[pad].y += 0x80;
						break;
					case KEY_PAD_LY_DOWN:
						g_lanalog[pad].y = -DEF_VALUE / 256;
						if (conf.options&PADOPTION_REVERTLY) g_lanalog[pad].y = -g_lanalog[pad].y;
						g_lanalog[pad].y += 0x80;
						break;
					case KEY_PAD_RX_LEFT:
						g_ranalog[pad].x = DEF_VALUE / 256;
						if (conf.options&PADOPTION_REVERTRX) g_ranalog[pad].x = -g_ranalog[pad].x;
						g_ranalog[pad].x += 0x80;
						break;
					case KEY_PAD_RX_RIGHT:
						g_ranalog[pad].x = -DEF_VALUE / 256;
						if (conf.options&PADOPTION_REVERTRX) g_ranalog[pad].x = -g_ranalog[pad].x;
						g_ranalog[pad].x += 0x80;
						break;
					case KEY_PAD_RY_UP:
						g_ranalog[pad].y = DEF_VALUE / 256;
						if (conf.options&PADOPTION_REVERTRY) g_ranalog[pad].y = -g_ranalog[pad].y;
						g_ranalog[pad].y += 0x80;
						break;
					case KEY_PAD_RY_DOWN:
						g_ranalog[pad].y = -DEF_VALUE / 256;
						if (conf.options&PADOPTION_REVERTRY) g_ranalog[pad].y = -g_ranalog[pad].y;
						g_ranalog[pad].y += 0x80;
						break;
				}
#endif

				i = FindKey(key, pad);
				if (i != -1) {
					keyPress |= (1 << i);
					keyRelease &= ~(1 << i);
				}
				event.evt = KEYPRESS;
				event.key = key;
				break;
				
			case KeyRelease:
				key = XLookupKeysym((XKeyEvent *) & E, 0);

#ifdef ANALOG_CONTROLS_HACK
				switch (key)
				{
					case KEY_PAD_LX_LEFT:
						g_lanalog[pad].x = 0x80;
						break;
					case KEY_PAD_LY_UP:
						g_lanalog[pad].y = 0x80;
						break;
					case KEY_PAD_RX_LEFT:
						g_ranalog[pad].x = 0x80;
						break;
					case KEY_PAD_RY_UP:
						g_ranalog[pad].y = 0x80;
						break;
					case KEY_PAD_LX_RIGHT:
						g_lanalog[pad].x = 0x80;
						break;
					case KEY_PAD_LY_DOWN:
						g_lanalog[pad].y = 0x80;
						break;
					case KEY_PAD_RX_RIGHT:
						g_ranalog[pad].x = 0x80;
						break;
					case KEY_PAD_RY_DOWN:
						g_ranalog[pad].y = 0x80;
						break;
				}
#endif

				i = FindKey(key, pad);
				if (i != -1) {
					keyPress &= ~(1 << i); 
					keyRelease |= (1 << i);
				}
				event.evt = KEYRELEASE;
				event.key = key;
				break;

			case FocusIn:
				XAutoRepeatOff(GSdsp);
				break;

			case FocusOut:
				XAutoRepeatOn(GSdsp);
				break;
		}
	}

	// joystick info
#ifdef JOYSTICK_SUPPORT
	
	SDL_JoystickUpdate();
	
	for (int i = 0; i < PADKEYS; i++)
	{
		int key = conf.keys[PadEnum[pad][0]][i];
		JoystickInfo* pjoy = NULL;

		if (IS_JOYBUTTONS(key))
		{
			int joyid = PAD_GETJOYID(key);
			
			if (joyid >= 0 && joyid < (int)s_vjoysticks.size())
			{
				pjoy = s_vjoysticks[joyid];
				
				if (SDL_JoystickGetButton((pjoy)->GetJoy(), PAD_GETJOYBUTTON(key)))
					status[(pjoy)->GetPAD()] &= ~(1 << i); // pressed
				else
					status[(pjoy)->GetPAD()] |= (1 << i); // pressed
			}
		}
		else if (IS_JOYSTICK(key))
		{
			int joyid = PAD_GETJOYID(key);
			if (joyid >= 0 && joyid < (int)s_vjoysticks.size())
			{

				pjoy = s_vjoysticks[joyid];
				int value = SDL_JoystickGetAxis((pjoy)->GetJoy(), PAD_GETJOYSTICK_AXIS(key));
				int pad = (pjoy)->GetPAD();
				switch (i)
				{
					case PAD_LX:
						if (abs(value) > (pjoy)->GetDeadzone(value))
						{
							g_lanalog[pad].x = value / 256;
							if (conf.options & PADOPTION_REVERTLX) g_lanalog[pad].x = -g_lanalog[pad].x;
							g_lanalog[pad].x += 0x80;
						}
						else 
						{
							g_lanalog[pad].x = 0x80;
						}
						break;
					case PAD_LY:
						if (abs(value) > (pjoy)->GetDeadzone(value))
						{
							g_lanalog[pad].y = value / 256;
							if (conf.options & PADOPTION_REVERTLY) g_lanalog[pad].y = -g_lanalog[pad].y;
							g_lanalog[pad].y += 0x80;
						}
						else
						{
							g_lanalog[pad].y = 0x80;
						}
						break;
					case PAD_RX:
						if (abs(value) > (pjoy)->GetDeadzone(value))
						{
							g_ranalog[pad].x = value / 256;
							if (conf.options & PADOPTION_REVERTRX) g_ranalog[pad].x = -g_ranalog[pad].x;
							g_ranalog[pad].x += 0x80;
						}
						else 
						{
							g_ranalog[pad].x = 0x80;
						}
						break;
					case PAD_RY:
						if (abs(value) > (pjoy)->GetDeadzone(value))
						{
							g_ranalog[pad].y = value / 256;
							if (conf.options&PADOPTION_REVERTRY) g_ranalog[pad].y = -g_ranalog[pad].y;
							g_ranalog[pad].y += 0x80;
						}
						else 
						{
							g_ranalog[pad].y = 0x80;
						}
						break;
				}
			}
		}
		else if (IS_POV(key))
		{
			int joyid = PAD_GETJOYID(key);
			if (joyid >= 0 && joyid < (int)s_vjoysticks.size())
			{
				pjoy = s_vjoysticks[joyid];

				int value = SDL_JoystickGetAxis((pjoy)->GetJoy(), PAD_GETJOYSTICK_AXIS(key));
				int pad = (pjoy)->GetPAD();
				
				if (PAD_GETPOVSIGN(key) && (value < -2048))
					status[pad] &= ~(1 << i);
				else if (!PAD_GETPOVSIGN(key) && (value > 2048))
					status[pad] &= ~(1 << i);
				else
					status[pad] |= (1 << i);
			}
		}
	}
#endif

	pthread_spin_lock(&s_mutexStatus);
	s_keyPress[pad] |= keyPress;
	s_keyPress[pad] &= ~keyRelease;
	s_keyRelease[pad] |= keyRelease;
	s_keyRelease[pad] &= ~keyPress;
	pthread_spin_unlock(&s_mutexStatus);
}

void UpdateConf(int pad)
{
	s_selectedpad = pad;

	int i;
	GtkWidget *Btn;
	for (i = 0; i < ArraySize(s_pGuiKeyMap); i++)
	{

		if (s_pGuiKeyMap[i] == NULL)
			continue;

		Btn = lookup_widget(Conf, GetLabelFromButton(s_pGuiKeyMap[i]).c_str());
		if (Btn == NULL)
		{
			printf("ZeroPAD: cannot find key %s\n", s_pGuiKeyMap[i]);
			continue;
		}

		string tmp;
		if (IS_KEYBOARD(conf.keys[pad][i]))
		{
			char* pstr = XKeysymToString(PAD_GETKEY(conf.keys[pad][i]));
			if (pstr != NULL) tmp = pstr;
		}
		else if (IS_JOYBUTTONS(conf.keys[pad][i]))
		{
			tmp.resize(20);
			sprintf(&tmp[0], "JBut %d", PAD_GETJOYBUTTON(conf.keys[pad][i]));
		}
		else if (IS_JOYSTICK(conf.keys[pad][i]))
		{
			tmp.resize(20);
			sprintf(&tmp[0], "JAxis %d", PAD_GETJOYSTICK_AXIS(conf.keys[pad][i]));
		}
		else if (IS_POV(conf.keys[pad][i]))
		{
			tmp.resize(20);
			sprintf(&tmp[0], "JPOV %d%s", PAD_GETJOYSTICK_AXIS(conf.keys[pad][i]), PAD_GETPOVSIGN(conf.keys[pad][i]) ? "-" : "+");
		}

		if (tmp.size() > 0)
		{
			gtk_entry_set_text(GTK_ENTRY(Btn), tmp.c_str());
		}
		else
			gtk_entry_set_text(GTK_ENTRY(Btn), "Unknown");

		gtk_object_set_user_data(GTK_OBJECT(Btn), (void*)(PADKEYS*pad + i));
	}

	// check bounds
	int joyid = _GetJoystickIdFromPAD(pad);
	
	if (joyid < 0 || joyid >= (int)s_vjoysticks.size())
	{
		// get first unused joystick
		for (joyid = 0; joyid < s_vjoysticks.size(); ++joyid)
		{
			if (s_vjoysticks[joyid]->GetPAD() < 0) break;
		}
	}

	if (joyid >= 0 && joyid < (int)s_vjoysticks.size())
		gtk_combo_box_set_active(GTK_COMBO_BOX(s_devicecombo), joyid); // select the combo
	else 
		gtk_combo_box_set_active(GTK_COMBO_BOX(s_devicecombo), s_vjoysticks.size()); // no gamepad

	int padopts = conf.options >> (16 * pad);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reverselx")), padopts&PADOPTION_REVERTLX);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reversely")), padopts&PADOPTION_REVERTLY);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reverserx")), padopts&PADOPTION_REVERTRX);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reversery")), padopts&PADOPTION_REVERTRY);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "forcefeedback")), padopts&PADOPTION_FORCEFEEDBACK);
}

void OnConf_Key(GtkButton *button, gpointer user_data)
{
	GdkEvent *ev;
	GtkWidget* label = lookup_widget(Conf, GetLabelFromButton(gtk_button_get_label(button)).c_str());
	if (label == NULL)
	{
		printf("couldn't find correct label\n");
		return;
	}

	int id = (int)(uptr)gtk_object_get_user_data(GTK_OBJECT(label));
	int pad = id / PADKEYS;
	int key = id % PADKEYS;
	unsigned long *pkey = &conf.keys[pad][key];

	// save the states
#ifdef JOYSTICK_SUPPORT
	vector<JoystickInfo*>::iterator itjoy = s_vjoysticks.begin();

	SDL_JoystickUpdate();
	
	// Save everything in the vector s_vjoysticks.
	while (itjoy != s_vjoysticks.end())
	{
		(*itjoy)->SaveState();
		itjoy++;
	}
#endif

	for (;;)
	{
		ev = gdk_event_get();
		if (ev != NULL)
		{
			if (ev->type == GDK_KEY_PRESS)
			{
				*pkey = ev->key.keyval;

				char* tmp = XKeysymToString(*pkey);
				if (tmp != NULL)
					gtk_entry_set_text(GTK_ENTRY(label), tmp);
				else
					gtk_entry_set_text(GTK_ENTRY(label), "Unknown");
				return;
			}
		}

#ifdef JOYSTICK_SUPPORT
		itjoy = s_vjoysticks.begin();
		
		SDL_JoystickUpdate();
		
		while (itjoy != s_vjoysticks.end())
		{
			// MAKE sure to look for changes in the state!!
			for (int i = 0; i < (*itjoy)->GetNumButtons(); ++i)
			{
				int but = SDL_JoystickGetButton((*itjoy)->GetJoy(), i);
				if (but != (*itjoy)->GetButtonState(i))
				{
					if (!but)    // released, we don't really want this
					{
						(*itjoy)->SetButtonState(i, 0);
						break;
					}

					*pkey = PAD_JOYBUTTON((*itjoy)->GetId(), i);
					char str[32];
					sprintf(str, "JBut %d", i);
					gtk_entry_set_text(GTK_ENTRY(label), str);
					return;
				}
			}

			for (int i = 0; i < (*itjoy)->GetNumAxes(); ++i)
			{
				int value = SDL_JoystickGetAxis((*itjoy)->GetJoy(), i);

				if (value != (*itjoy)->GetAxisState(i))
				{

					if (abs(value) <= (*itjoy)->GetAxisState(i))  // we don't want this
					{
						// released, we don't really want this
						(*itjoy)->SetButtonState(i, value);
						break;
					}


					if (abs(value) > 0x3fff)
					{
						if (key < 16)    // POV
						{
							*pkey = PAD_POV((*itjoy)->GetId(), value < 0, i);
							char str[32];
							sprintf(str, "JPOV %d%s", i, value < 0 ? "-" : "+");
							gtk_entry_set_text(GTK_ENTRY(label), str);
							return;
						}
						else   // axis
						{
							*pkey = PAD_JOYSTICK((*itjoy)->GetId(), i);
							char str[32];
							sprintf(str, "JAxis %d", i);
							gtk_entry_set_text(GTK_ENTRY(label), str);
							return;
						}
					}
				}
			}
			
			
			/*for (int i = 0; i < (*itjoy)->GetNumHats(); ++i)
			{
				int value = SDL_JoystickGetHat((*itjoy)->GetJoy(), i);

				if (value != (*itjoy)->GetAxisState(i))
				{
					*pkey = PAD_HAT((*itjoy)->GetId(), i);
					char str[32];
					sprintf(str, "JHat %d", i);
					gtk_entry_set_text(GTK_ENTRY(label), str);
					return;
				}
			}*/
			
			itjoy++;
		}
#endif
	}
}

void CALLBACK PADconfigure()
{
	char strcurdir[256];
	getcwd(strcurdir, 256);
	s_strIniPath = strcurdir;
	s_strIniPath += "/inis/zeropad.ini";

	LoadConfig();

	Conf = create_Conf();

	// recreate
#ifdef JOYSTICK_SUPPORT
	JoystickInfo::EnumerateJoysticks(s_vjoysticks);
#endif

	s_devicecombo = lookup_widget(Conf, "joydevicescombo");

	// fill the combo
	char str[255];
	vector<JoystickInfo*>::iterator it = s_vjoysticks.begin();
	
	// Delete everything in the vector vjoysticks.
	while (it != s_vjoysticks.end())
	{
		sprintf(str, "%d: %s - but: %d, axes: %d, pov: %d", (*it)->GetId(), (*it)->GetName().c_str(),
		        (*it)->GetNumButtons(), (*it)->GetNumAxes(), (*it)->GetNumPOV()/*, (*it)->GetNumHats()*/); // ,hats: %d
		gtk_combo_box_append_text(GTK_COMBO_BOX(s_devicecombo), str);
		it++;
	}
	
	gtk_combo_box_append_text(GTK_COMBO_BOX(s_devicecombo), "No Gamepad");

	UpdateConf(0);

	gtk_widget_show_all(Conf);
	gtk_main();
}
