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
	numpov = SDL_JoystickNumHats(joy);
	devname = SDL_JoystickName(id);
	
	vaxisstate.resize(numaxes);
	vbutstate.resize(numbuttons);
	vpovstate.resize(numpov);

	//PAD_LOG("There are %d buttons, %d axises, and %d povs.\n", numbuttons, numaxes, numpov);
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
	for (int i = 0; i < numpov; ++i)
		SetPOVState(i, SDL_JoystickGetHat(joy, i));
}

void JoystickInfo::TestForce()
{
}
