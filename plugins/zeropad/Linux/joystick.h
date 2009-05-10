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
 
 #ifndef __JOYSTICK_H__
 #define __JOYSTICK_H__
 
#include <SDL/SDL.h>
#include "zeropad.h"

// holds all joystick info
class JoystickInfo
{
	public:
		JoystickInfo();
		~JoystickInfo()
		{
			Destroy();
		}

		void Destroy();
		// opens handles to all possible joysticks
		static void EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks);

		bool Init(int id, bool bStartThread = true); // opens a handle and gets information
		void Assign(int pad); // assigns a joystick to a pad

		void TestForce();

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
		
		int GetNumPOV()
		{
			return numpov;
		}
		
		int GetId()
		{
			return _id;
		}
		
		int GetPAD()
		{
			return pad;
		}
		
		int GetDeadzone(int axis)
		{
			return deadzone;
		}

		void SaveState();
		
		int GetButtonState(int i)
		{
			return vbutstate[i];
		}
		
		int GetAxisState(int i)
		{
			return vaxisstate[i];
		}
		
		int GetPOVState(int i)
		{
			//PAD_LOG("Getting POV State of %d.\n", i);
			return vpovstate[i];
		}
		
		void SetButtonState(int i, int state)
		{
			vbutstate[i] = state;
		}
		
		void SetAxisState(int i, int value)
		{
			vaxisstate[i] = value;
		}
		
		void SetPOVState(int i, int value)
		{
			//PAD_LOG("We should set %d to %d.\n", i, value);
			vpovstate[i] = value;
		}
		
		SDL_Joystick* GetJoy()
		{
			return joy;
		}

	private:

		string devname; // pretty device name
		int _id;
		int numbuttons, numaxes, numpov;
		int axisrange, deadzone;
		int pad;

		vector<int> vbutstate, vaxisstate, vpovstate;

		SDL_Joystick* joy;
};


extern int s_selectedpad;
extern vector<JoystickInfo*> s_vjoysticks;
#endif
