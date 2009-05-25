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
 
#include "joystick.h"

//////////////////////////
// Joystick definitions //
//////////////////////////

int s_selectedpad = 0;
vector<JoystickInfo*> s_vjoysticks;
static u32 s_bSDLInit = false;

void UpdateJoysticks()
{
	vector<JoystickInfo*>::iterator itjoy = s_vjoysticks.begin();

	SDL_JoystickUpdate();
	
	// Save everything in the vector s_vjoysticks.
	while (itjoy != s_vjoysticks.end())
	{
		(*itjoy)->SaveState();
		itjoy++;
	}
}

const char *HatName(int value)
{
	switch(value)
	{
		case SDL_HAT_CENTERED:
			return "SDL_HAT_CENTERED";
		case SDL_HAT_UP:
			return "SDL_HAT_UP";
		case SDL_HAT_RIGHT:
			return "SDL_HAT_RIGHT";
		case SDL_HAT_DOWN:
			return "SDL_HAT_DOWN";
		case SDL_HAT_LEFT:
			return "SDL_HAT_LEFT";
		case SDL_HAT_RIGHTUP:
			return "SDL_HAT_RIGHTUP";
		case SDL_HAT_RIGHTDOWN:
			return "SDL_HAT_RIGHTDOWN";
		case SDL_HAT_LEFTUP:
			return "SDL_HAT_LEFTUP";
		case SDL_HAT_LEFTDOWN:
			return "SDL_HAT_LEFTDOWN";
		default:	
			return "Unknown";
	}
						
	return "Unknown";
}

// opens handles to all possible joysticks
void JoystickInfo::EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks)
{

	if (!s_bSDLInit)
	{
		if (SDL_Init(SDL_INIT_JOYSTICK) < 0) return;
		SDL_JoystickEventState(SDL_QUERY);
		s_bSDLInit = true;
	}
	
	vector<JoystickInfo*>::iterator it = vjoysticks.begin();
	
	// Delete everything in the vector vjoysticks.
	while (it != vjoysticks.end())
	{
		delete *it;
		it ++;
	}
	
	vjoysticks.resize(SDL_NumJoysticks());
	
	for (int i = 0; i < (int)vjoysticks.size(); ++i)
	{
		vjoysticks[i] = new JoystickInfo();
		vjoysticks[i]->Init(i, true);
	}

	// set the pads
	for (int pad = 0; pad < 2; ++pad)
	{
		// select the right joystick id
		int joyid = -1;
		
		for (int i = 0; i < PADKEYS; ++i)
		{
			if (IS_JOYSTICK(conf.keys[pad][i]) || IS_JOYBUTTONS(conf.keys[pad][i]))
			{
				joyid = PAD_GETJOYID(conf.keys[pad][i]);
				break;
			}
		}

		if ((joyid >= 0) && (joyid < (int)s_vjoysticks.size())) s_vjoysticks[joyid]->Assign(pad);
	}

}

JoystickInfo::JoystickInfo()
{
	joy = NULL;
	
	_id = -1;
	pad = -1;
	axisrange = 0x7fff;
	deadzone = 2000;
}

void JoystickInfo::Destroy()
{
	if (joy != NULL)
	{
		if (SDL_JoystickOpened(_id)) SDL_JoystickClose(joy);
		joy = NULL;
	}
}

bool JoystickInfo::Init(int id, bool bStartThread)
{
	Destroy();
	_id = id;

	joy = SDL_JoystickOpen(id);
	if (joy == NULL)
	{
		PAD_LOG("failed to open joystick %d\n", id);
		return false;
	}

	numaxes = SDL_JoystickNumAxes(joy);
	numbuttons = SDL_JoystickNumButtons(joy);
	numhats = SDL_JoystickNumHats(joy);
	devname = SDL_JoystickName(id);
	
	vaxisstate.resize(numaxes);
	vbuttonstate.resize(numbuttons);
	vhatstate.resize(numhats);

	//PAD_LOG("There are %d buttons, %d axises, and %d hats.\n", numbuttons, numaxes, numhats);
	return true;
}

// assigns a joystick to a pad
void JoystickInfo::Assign(int newpad)
{
	if (pad == newpad) return;
	pad = newpad;

	if (pad >= 0)
	{
		for (int i = 0; i < PADKEYS; ++i)
		{
			if (IS_JOYBUTTONS(conf.keys[pad][i]))
			{
				conf.keys[pad][i] = PAD_JOYBUTTON(_id, PAD_GETJOYBUTTON(conf.keys[pad][i]));
			}
			else if (IS_JOYSTICK(conf.keys[pad][i]))
			{
				conf.keys[pad][i] = PAD_JOYSTICK(_id, PAD_GETJOYBUTTON(conf.keys[pad][i]));
			}
		}
	}
}

void JoystickInfo::SaveState()
{
	for (int i = 0; i < numbuttons; ++i)
		SetButtonState(i, SDL_JoystickGetButton(joy, i));
	for (int i = 0; i < numaxes; ++i)
		SetAxisState(i, SDL_JoystickGetAxis(joy, i));
	for (int i = 0; i < numhats; ++i)
		SetHatState(i, SDL_JoystickGetHat(joy, i));
}

void JoystickInfo::TestForce()
{
}

bool JoystickInfo::PollButtons(int &jbutton, u32 &pkey)
{
	// MAKE sure to look for changes in the state!!
	for (int i = 0; i < GetNumButtons(); ++i)
	{
		int but = SDL_JoystickGetButton(GetJoy(), i);
		
		if (but != GetButtonState(i))
		{
			if (!but)    // released, we don't really want this
			{
				SetButtonState(i, 0);
				break;
			}

			pkey = PAD_JOYBUTTON(GetId(), i);
			jbutton = i;
			return true;
		}
	}
	
	return false;	
}

bool JoystickInfo::PollAxes(bool pov, int &jbutton, bool &negative,  u32 &pkey)
{
	for (int i = 0; i < GetNumAxes(); ++i)
	{
		int value = SDL_JoystickGetAxis(GetJoy(), i);

		if (value != GetAxisState(i))
		{
			PAD_LOG("Change in joystick %d: %d.\n", i, value);

			if (abs(value) <= GetAxisState(i))  // we don't want this
			{
				// released, we don't really want this
				SetAxisState(i, value);
				break;
			}

			if (abs(value) > 0x3fff)
			{
				jbutton = i;
				
				if (pov)
				{
					negative = (value < 0);
					pkey = PAD_POV(GetId(), negative, i);
				}
				else   // axis
				{
					pkey = PAD_JOYSTICK(GetId(), i);
				}
				return true;
			}
		}
	}
	return false;
}

bool JoystickInfo::PollHats(int &jbutton, int &dir, u32 &pkey)
{
#ifdef EXPERIMENTAL_POV_CODE
	for (int i = 0; i < GetNumHats(); ++i)
	{
		int value = SDL_JoystickGetHat(GetJoy(), i);

		if (value != SDL_HAT_CENTERED)
		{
			switch (value)
			{
				case SDL_HAT_UP:
				case SDL_HAT_RIGHT:
				case SDL_HAT_DOWN:
				case SDL_HAT_LEFT:
					pkey = PAD_HAT(GetId(), value, i);
					jbutton = i;
					dir = value;
					PAD_LOG("Hat Pressed!");
					return true;
				default:
					break;
			}
		}
	}
#endif
	return false;
}
