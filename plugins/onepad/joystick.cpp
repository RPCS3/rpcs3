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

void JoystickInfo::UpdateReleaseState()
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

bool JoystickIdWithinBounds(int joyid)
{
	return ((joyid >= 0) && (joyid < (int)s_vjoysticks.size()));
}

// opens handles to all possible joysticks
void JoystickInfo::EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks)
{

	if (!s_bSDLInit)
	{
#if SDL_MAJOR_VERSION >= 2
		// Tell SDL to catch event even if the windows isn't focussed
		SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
		// SDL in 3rdparty wrap X11 call. In order to get x11 symbols loaded
		// video must be loaded too.
		// Example of X11 symbol are XAutoRepeatOn/XAutoRepeatOff
		if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_VIDEO|SDL_INIT_HAPTIC|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS) < 0) return;
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
#if SDL_MAJOR_VERSION >= 2
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
	/* Effect big & small    										   */
	/*******************************************************************/
	for (int new_effect = 0; new_effect < 2 ; new_effect++) {

		// Direction of the effect SDL_HapticDirection
		haptic_effect_data[new_effect].periodic.direction.type = SDL_HAPTIC_POLAR; // Polar coordinates
		haptic_effect_data[new_effect].periodic.direction.dir[0] = 18000; // Force comes from south

		// periodic parameter
		haptic_effect_data[new_effect].periodic.period = 20; // 20 ms
		haptic_effect_data[new_effect].periodic.magnitude = 2000; // 2000/32767 strength

		// Replay
		haptic_effect_data[new_effect].periodic.length = 60; // 60 ms long
		haptic_effect_data[new_effect].periodic.delay  = 0;	   // start 0 second after the upload

		// enveloppe
		haptic_effect_data[new_effect].periodic.attack_length = 5;// Takes 5 ms to get max strength
		haptic_effect_data[new_effect].periodic.attack_level = 0;   // start at 0
		haptic_effect_data[new_effect].periodic.fade_length = 5;	// Takes 5 ms to fade away
		haptic_effect_data[new_effect].periodic.fade_level = 0;    	// finish at 0
	}

	/*******************************************************************/
	/* Effect small													   */
	/*******************************************************************/
	haptic_effect_data[0].type = SDL_HAPTIC_LEFTRIGHT;

	/*******************************************************************/
	/* Effect big													   */
	/*******************************************************************/
	haptic_effect_data[1].type = SDL_HAPTIC_TRIANGLE;

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

#if SDL_MAJOR_VERSION >= 2
	int joyid = conf->get_joyid(pad);
	if (!JoystickIdWithinBounds(joyid)) return;
	JoystickInfo* pjoy = s_vjoysticks[joyid];

	if (pjoy->haptic == NULL) return;
	if (pjoy->haptic_effect_id[type] < 0) return;

	// FIXME: might need to multiply force
	pjoy->haptic_effect_data[type].periodic.magnitude = force * conf->ff_intensity ; // force/32767 strength
	// Upload the new effect
	SDL_HapticUpdateEffect(pjoy->haptic, pjoy->haptic_effect_id[type], &pjoy->haptic_effect_data[type]);

	// run the effect once
	SDL_HapticRunEffect( pjoy->haptic, pjoy->haptic_effect_id[type], 1 );
#endif
}

void JoystickInfo::Destroy()
{
	if (joy != NULL)
	{
#if SDL_MAJOR_VERSION >= 2
		// Haptic must be closed before the joystick
		if (haptic != NULL) {
			SDL_HapticClose(haptic);
			haptic = NULL;
		}
#endif

#if SDL_MAJOR_VERSION >= 2
		if (joy) SDL_JoystickClose(joy);
#else
		if (SDL_JoystickOpened(_id)) SDL_JoystickClose(joy);
#endif
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
#if SDL_MAJOR_VERSION >= 2
	devname = SDL_JoystickName(joy);
#else
	devname = SDL_JoystickName(id);
#endif

	vaxisstate.resize(numaxes);
	vbuttonstate.resize(numbuttons);
	vhatstate.resize(numhats);

	// Sixaxis, dualshock3 hack
	// Most buttons are actually axes due to analog pressure support. Only the first 4 buttons
	// are digital (select, start, l3, r3). To avoid conflict just forget the others.
	// Keep the 4 hat buttons too (usb driver). (left pressure does not work with recent kernel). Moreover the pressure
	// work sometime on half axis neg others time in fulll axis. So better keep them as button for the moment
	u32 found_hack = devname.find(string("PLAYSTATION(R)3"));
	if (found_hack != string::npos) {
		numbuttons = 4;
		// Enable this hack is bluetooth too. It avoid to restart the onepad gui
		numbuttons += 4;
	}

#if SDL_MAJOR_VERSION >= 2
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
			// Pressure sensitive button are detected as both button (digital) and axes (analog). So better
			// drop the button to emulate the pressure sensiblity of the ds2 :)
			// Trick: detect the release of the button. It avoid all races condition between axes and buttons :)
			// If the button support pressure it will be detected as an axis when it is pressed.
			if (but) {
				SetButtonState(i, but);
				return false;
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
		// Sixaxis, dualshock3 hack
		u32 found_hack = devname.find(string("PLAYSTATION(R)3"));
		if (found_hack != string::npos) {
			// The analog mode of the hat button is quite erratic. Values can be in half- axis
			// or full axis... So better keep them as button for the moment -- gregory
			if (i >= 8 && i <= 11 && (conf->options & PADOPTION_SIXAXIS_USB))
				continue;
			// Disable accelerometer
			if ((i >= 4 && i <= 6))
				continue;
		}

		s32 value = SDL_JoystickGetAxis(GetJoy(), i);
		s32 old_value = GetAxisState(i);

		if (abs(value - old_value) < 0x1000)
			continue;

		if (value != old_value)
		{
			PAD_LOG("Change in joystick %d: %d.\n", i, value);
			// There are several kinds of axes
			// Half+: 0 (release) -> 32768
			// Half-: 0 (release) -> -32768
			// Full (like dualshock 3): -32768 (release) ->32768
			const s32 full_axis_ceil = -0x6FFF;
			const s32 half_axis_ceil = 0x1FFF;

			// Normally, old_value contains the release state so it can be used to detect the types of axis.
			bool is_full_axis = (old_value < full_axis_ceil) ? true : false;

			if ((!is_full_axis && abs(value) <= half_axis_ceil)
					|| (is_full_axis && value <= full_axis_ceil))  // we don't want this
			{
				continue;
			}

			if ((!is_full_axis && abs(value) > half_axis_ceil)
					|| (is_full_axis && value > full_axis_ceil)) 
			{
				bool sign = (value < 0);
				pkey = axis_to_key(is_full_axis, sign, i);

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
