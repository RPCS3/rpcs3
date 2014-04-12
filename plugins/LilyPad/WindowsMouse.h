/*  LilyPad - Pad plugin for PS2 Emulator
 *  Copyright (C) 2002-2014  PCSX2 Dev Team/ChickenLiver
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU Lesser General Public License as published by the Free
 *  Software Found- ation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with PCSX2.  If not, see <http://www.gnu.org/licenses/>.
 */

// Shared functionality for WM and RAW keyboards.
class WindowsMouse : public Device {
public:
	// Used by GetMouseCapture()/ReleaseMouseCapture()
	// Static because can have multiple raw mice active at once,
	// and only get/release capture once.
	static POINT origCursorPos;
	static POINT center;

	static void GetMouseCapture(HWND hWnd);
	static void WindowResized(HWND hWnd);
	static void ReleaseMouseCapture();

	// hWheel variable lets me display no horizontal wheel for raw input, just to make it clear
	// that it's not supported.
	WindowsMouse(DeviceAPI api, int hWheel, wchar_t *displayName, wchar_t *instanceID=0, wchar_t *deviceID=0);
	wchar_t *GetPhysicalControlName(PhysicalControl *control);
	// State is 0 for up, 1 for down.
	void UpdateButton(unsigned int button, int state);
	// 0/1 are x/y. 2 is vert wheel, 3 is horiz wheel.
	// Delta is in my micro units.  change of (1<<16) is 1 full unit, with
	// the default sensitivity.
	void UpdateAxis(unsigned int axis, int delta);
};
