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

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "SDL.h"
#if SDL_VERSION_ATLEAST(1,3,0)
#include "SDL_haptic.h"
#endif

#include "onepad.h"
#include "controller.h"

// holds all joystick info
class JoystickInfo
{
	public:
		JoystickInfo() : devname(""), _id(-1), numbuttons(0), numaxes(0), numhats(0), axisrange(0x7fff),
		 deadzone(1500), pad(-1), joy(NULL) {
			 vbuttonstate.clear();
			 vaxisstate.clear();
			 vhatstate.clear();
#if SDL_VERSION_ATLEAST(1,3,0)
			 haptic = NULL;
			 for (int i = 0 ; i < 2 ; i++)
				 haptic_effect_id[i] = -1;
#endif
		 }

		~JoystickInfo()
		{
			Destroy();
		}

		JoystickInfo(const JoystickInfo&);             // copy constructor
		JoystickInfo& operator=(const JoystickInfo&); // assignment
    
		void Destroy();
		// opens handles to all possible joysticks
		static void EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks);

		void InitHapticEffect();
		static void DoHapticEffect(int type, int pad, int force);

		bool Init(int id); // opens a handle and gets information

		void TestForce();

		bool PollButtons(u32 &pkey);
		bool PollAxes(u32 &pkey);
		bool PollHats(u32 &pkey);

		const string& GetName()
		{
			return devname;
		}

		int GetNumButtons()
		{
			return numbuttons;
		}

		int GetNumAxes()
		{
			return numaxes;
		}

		int GetNumHats()
		{
			return numhats;
		}

		int GetPAD()
		{
			return pad;
		}

		int GetDeadzone()
		{
			return deadzone;
		}

		void SaveState();

		int GetButtonState(int i)
		{
			return vbuttonstate[i];
		}

		int GetAxisState(int i)
		{
			return vaxisstate[i];
		}

		int GetHatState(int i)
		{
			//PAD_LOG("Getting POV State of %d.\n", i);
			set_hat_pins(vhatstate[i]);
			return vhatstate[i];
		}

		void SetButtonState(int i, int state)
		{
			vbuttonstate[i] = state;
		}

		void SetAxisState(int i, int value)
		{
			vaxisstate[i] = value;
		}

		void SetHatState(int i, int value)
		{
			//PAD_LOG("We should set %d to %d.\n", i, value);
			set_hat_pins(i);
			vhatstate[i] = value;
		}

		SDL_Joystick* GetJoy()
		{
			return joy;
		}
		int GetAxisFromKey(int pad, int index);
	private:
		string devname; // pretty device name
		int _id;
		int numbuttons, numaxes, numhats;
		int axisrange, deadzone;
		int pad;

		vector<int> vbuttonstate, vaxisstate, vhatstate;

		SDL_Joystick*		joy;
#if SDL_VERSION_ATLEAST(1,3,0)
		SDL_Haptic*   		haptic;
		SDL_HapticEffect	haptic_effect_data[2];
		int   				haptic_effect_id[2];
#endif
};


extern int s_selectedpad;
extern vector<JoystickInfo*> s_vjoysticks;
extern void UpdateJoysticks();
extern const char *HatName(int value);
extern bool JoystickIdWithinBounds(int joyid);
#endif
