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

//////////////////////////
// Joystick definitions //
//////////////////////////

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
		case SDL_HAT_CENTERED: return "SDL_HAT_CENTERED";
		case SDL_HAT_UP: return "SDL_HAT_UP";
		case SDL_HAT_RIGHT: return "SDL_HAT_RIGHT";
		case SDL_HAT_DOWN: return "SDL_HAT_DOWN";
		case SDL_HAT_LEFT: return "SDL_HAT_LEFT";
		case SDL_HAT_RIGHTUP: return "SDL_HAT_RIGHTUP";
		case SDL_HAT_RIGHTDOWN: return "SDL_HAT_RIGHTDOWN";
		case SDL_HAT_LEFTUP: return "SDL_HAT_LEFTUP";
		case SDL_HAT_LEFTDOWN: return "SDL_HAT_LEFTDOWN";
		default: return "Unknown";
	}

	return "Unknown";
}

bool JoystickIdWithinBounds(int joyid)
{
	return ((joyid >= 0) && (joyid < (int)s_vjoysticks.size()));
}

// opens handles to all possible joysticks
void JoystickInfo::EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks)
{

	if (!s_bSDLInit)
	{
#if SDL_VERSION_ATLEAST(1,3,0)
		// SDL in 3rdparty wrap X11 call. In order to get x11 symbols loaded
		// video must be loaded too.
		// Example of X11 symbol are XAutoRepeatOn/XAutoRepeatOff
		if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_VIDEO|SDL_INIT_HAPTIC) < 0) return;
#else
		if (SDL_Init(SDL_INIT_JOYSTICK) < 0) return;
#endif
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
		vjoysticks[i]->Init(i);
	}
}

void JoystickInfo::InitHapticEffect()
{
#if SDL_VERSION_ATLEAST(1,3,0)
	if (haptic == NULL) return;

#if 0
	additional field of the effect
    /* Trigger */
    Uint16 button;      /**< Button that triggers the effect. */
    Uint16 interval;    /**< How soon it can be triggered again after button. */

	// periodic parameter
    Sint16 offset;      /**< Mean value of the wave. */
    Uint16 phase;       /**< Horizontal shift given by hundredth of a cycle. */
#endif

	/*******************************************************************/
	/* Effect small													   */
	/*******************************************************************/
	haptic_effect_data[0].type = SDL_HAPTIC_SQUARE;

	// Direction of the effect SDL_HapticDirection
    haptic_effect_data[0].periodic.direction.type = SDL_HAPTIC_POLAR; // Polar coordinates
    haptic_effect_data[0].periodic.direction.dir[0] = 18000; // Force comes from south

	// periodic parameter
    haptic_effect_data[0].periodic.period = 1000; // 1000 ms
    haptic_effect_data[0].periodic.magnitude = 2000; // 2000/32767 strength

	// Replay
    haptic_effect_data[0].periodic.length = 2000; // 2 seconds long
    haptic_effect_data[0].periodic.delay  = 0;	   // start 0 second after the upload

	// enveloppe
    haptic_effect_data[0].periodic.attack_length = 500;// Takes 0.5 second to get max strength
	haptic_effect_data[0].periodic.attack_level = 0;   // start at 0
    haptic_effect_data[0].periodic.fade_length = 500;	// Takes 0.5 second to fade away
	haptic_effect_data[0].periodic.fade_level = 0;    	// finish at 0

	/*******************************************************************/
	/* Effect big													   */
	/*******************************************************************/
	haptic_effect_data[1].type = SDL_HAPTIC_TRIANGLE;

	// Direction of the effect SDL_HapticDirection
    haptic_effect_data[1].periodic.direction.type = SDL_HAPTIC_POLAR; // Polar coordinates
    haptic_effect_data[1].periodic.direction.dir[0] = 18000; // Force comes from south

	// periodic parameter
    haptic_effect_data[1].periodic.period = 1000; // 1000 ms
    haptic_effect_data[1].periodic.magnitude = 2000; // 2000/32767 strength

	// Replay
    haptic_effect_data[1].periodic.length = 2000; // 2 seconds long
    haptic_effect_data[1].periodic.delay  = 0;	   // start 0 second after the upload

	// enveloppe
    haptic_effect_data[1].periodic.attack_length = 500;// Takes 0.5 second to get max strength
	haptic_effect_data[1].periodic.attack_level = 0;   // start at 0
    haptic_effect_data[1].periodic.fade_length = 500;	// Takes 0.5 second to fade away
	haptic_effect_data[1].periodic.fade_level = 0;    	// finish at 0

	/*******************************************************************/
	/* Upload effect to the device									   */
	/*******************************************************************/
	for (int i = 0 ; i < 2 ; i++)
		haptic_effect_id[i] = SDL_HapticNewEffect(haptic, &haptic_effect_data[i]);
#endif
}


void JoystickInfo::DoHapticEffect(int type, int pad, int force)
{
	if (type > 1) return;
	if ( !(conf->options & (PADOPTION_FORCEFEEDBACK << 16 * pad)) ) return;

#if SDL_VERSION_ATLEAST(1,3,0)
	// first search the joy associated to the pad
	vector<JoystickInfo*>::iterator itjoy = s_vjoysticks.begin();
	while (itjoy != s_vjoysticks.end()) {
		if ((*itjoy)->GetPAD() == pad) break;
		itjoy++;
	}

	if (itjoy == s_vjoysticks.end()) return;
	if ((*itjoy)->haptic == NULL) return;
	if ((*itjoy)->haptic_effect_id[type] < 0) return;

	// FIXME: might need to multiply force
	(*itjoy)->haptic_effect_data[type].periodic.magnitude = force; // force/32767 strength
	// Upload the new effect
	SDL_HapticUpdateEffect((*itjoy)->haptic, (*itjoy)->haptic_effect_id[type], &(*itjoy)->haptic_effect_data[type]);

	// run the effect once
	SDL_HapticRunEffect( (*itjoy)->haptic, (*itjoy)->haptic_effect_id[type], 1 );
#endif
}

void JoystickInfo::Destroy()
{
	if (joy != NULL)
	{
#if SDL_VERSION_ATLEAST(1,3,0)
		// Haptic must be closed before the joystick
		if (haptic != NULL) {
			SDL_HapticClose(haptic);
			haptic = NULL;
		}
#endif

		if (SDL_JoystickOpened(_id)) SDL_JoystickClose(joy);
		joy = NULL;
	}
}

bool JoystickInfo::Init(int id)
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

#if SDL_VERSION_ATLEAST(1,3,0)
	if ( haptic == NULL ) {
		if (!SDL_JoystickIsHaptic(joy)) {
			PAD_LOG("Haptic devices not supported!\n");
		} else {
			haptic = SDL_HapticOpenFromJoystick(joy);
			// upload some default effect
			InitHapticEffect();
		}
	}
#endif

	//PAD_LOG("There are %d buttons, %d axises, and %d hats.\n", numbuttons, numaxes, numhats);
	return true;
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

bool JoystickInfo::PollButtons(u32 &pkey)
{
	// MAKE sure to look for changes in the state!!
	for (int i = 0; i < GetNumButtons(); ++i)
	{
		int but = SDL_JoystickGetButton(GetJoy(), i);

		if (but != GetButtonState(i))
		{
			if (!but)    // released, we don't really want this
			{
				continue;
			}
			// Pressure sensitive button are detected as both button (digital) and axe (analog). So better
			// drop the button to emulate the pressure sensiblity of the ds2 :) -- Gregory
			for (int j = 0; j < GetNumAxes(); ++j) {
				int value = SDL_JoystickGetAxis(GetJoy(), j);
				int old_value = GetAxisState(j);
				bool full_axis = (old_value < -0x3FFF) ? true : false;
				if (value != old_value && ((full_axis && value > -0x6FFF ) || (!full_axis && abs(value) > old_value))) {
					return false;
				}

			}

			pkey = button_to_key(i);
			return true;
		}
	}

	return false;
}

bool JoystickInfo::PollAxes(u32 &pkey)
{
	for (int i = 0; i < GetNumAxes(); ++i)
	{
		int value = SDL_JoystickGetAxis(GetJoy(), i);
		int old_value = GetAxisState(i);

		if (value != old_value)
		{
			PAD_LOG("Change in joystick %d: %d.\n", i, value);
			// There is several kinds of axes
			// Half+: 0 (release) -> 32768
			// Half-: 0 (release) -> -32768
			// Full (like dualshock 3): -32768 (release) ->32768
			bool full_axis = (old_value < -0x2FFF) ? true : false;

			if ((!full_axis && abs(value) <= 0x1FFF)
					|| (full_axis && value <= -0x6FFF))  // we don't want this
			{
				continue;
			}

			if ((!full_axis && abs(value) > 0x3FFF)
					|| (full_axis && value > -0x6FFF)) 
			{
				bool sign = (value < 0);
				pkey = axis_to_key(full_axis, sign, i);

				return true;
			}
		}
	}
	return false;
}

bool JoystickInfo::PollHats(u32 &pkey)
{
	for (int i = 0; i < GetNumHats(); ++i)
	{
		int value = SDL_JoystickGetHat(GetJoy(), i);

		if ((value != GetHatState(i)) && (value != SDL_HAT_CENTERED))
		{
			switch (value)
			{
				case SDL_HAT_UP:
				case SDL_HAT_RIGHT:
				case SDL_HAT_DOWN:
				case SDL_HAT_LEFT:
					pkey = hat_to_key(value, i);
					PAD_LOG("Hat Pressed!");
					return true;
				default:
					break;
			}
		}
	}
	return false;
}

int JoystickInfo::GetAxisFromKey(int pad, int index)
{
	return SDL_JoystickGetAxis(GetJoy(), key_to_axis(pad, index));
}
