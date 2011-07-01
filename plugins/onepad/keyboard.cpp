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

 /*
  * Theoretically, this header is for anything to do with keyboard input.
  * Pragmatically, event handing's going in here too.
  */
  
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "keyboard.h"

#ifndef __LINUX__
char* KeysymToChar(int keysym)
{
	LPWORD temp;

	ToAscii((UINT) keysym, NULL, NULL, temp, NULL);
	return (char*)temp;
}
#endif

void SetAutoRepeat(bool autorep)
{
 #ifdef __LINUX__
    if (toggleAutoRepeat)
    {
        if (autorep)
            XAutoRepeatOn(GSdsp);
        else
            XAutoRepeatOff(GSdsp);
    }
#endif
}

#ifdef __LINUX__
static bool s_grab_input = false;
static bool s_Shift = false;
static unsigned int  s_previous_mouse_x = 0;
static unsigned int  s_previous_mouse_y = 0;
void AnalyzeKeyEvent(int pad, keyEvent &evt)
{
	KeySym key = (KeySym)evt.key;
	int index = get_keyboard_key(pad, key);

	switch (evt.evt)
	{
		case KeyPress:
			// Shift F12 is not yet use by pcsx2. So keep it to grab/ungrab input
			// I found it very handy vs the automatic fullscreen detection
			// 1/ Does not need to detect full-screen
			// 2/ Can use a debugger in full-screen
			// 3/ Can grab input in window without the need of a pixelated full-screen
			if (key == XK_Shift_R || key == XK_Shift_L) s_Shift = true;
			if (key == XK_F12 && s_Shift) {
				if(!s_grab_input) {
					s_grab_input = true;
					XGrabPointer(GSdsp, GSwin, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, GSwin, None, CurrentTime);
					XGrabKeyboard(GSdsp, GSwin, True, GrabModeAsync, GrabModeAsync, CurrentTime);
				} else {
					s_grab_input = false;
					XUngrabPointer(GSdsp, CurrentTime);
					XUngrabKeyboard(GSdsp, CurrentTime);
				}
			}

			// Analog controls.
			if (IsAnalogKey(index))
			{
				switch (index)
				{
					case PAD_R_LEFT:
					case PAD_R_UP:
					case PAD_L_LEFT:
					case PAD_L_UP:
						key_status->press(pad, index, -MAX_ANALOG_VALUE);
						break;
					case PAD_R_RIGHT:
					case PAD_R_DOWN:
					case PAD_L_RIGHT:
					case PAD_L_DOWN:
						key_status->press(pad, index, MAX_ANALOG_VALUE);
						break;
				}
			} else if (index != -1)
				key_status->press(pad, index);

			//PAD_LOG("Key pressed:%d\n", index);

			event.evt = KEYPRESS;
			event.key = key;
			break;

		case KeyRelease:
			if (key == XK_Shift_R || key == XK_Shift_L) s_Shift = false;

			if (index != -1)
				key_status->release(pad, index);

			event.evt = KEYRELEASE;
			event.key = key;
			break;

		case FocusIn:
			XAutoRepeatOff(GSdsp);
			break;

		case FocusOut:
			XAutoRepeatOn(GSdsp);
			break;

		case ButtonPress:
			if (index != -1)
				key_status->press(pad, index);
			break;

		case ButtonRelease:
			if (index != -1)
				key_status->release(pad, index);
			break;

		case MotionNotify:
			// FIXME: How to handle when the mouse does not move, no event generated!!!
			// 1/ small move == no move. Cons : can not do small movement
			// 2/ use a watchdog timer thread
			// 3/ ??? idea welcome ;)
			if (conf->options & ((PADOPTION_MOUSE_L|PADOPTION_MOUSE_R) << 16 * pad )) {
				unsigned int pad_x;
				unsigned int pad_y;
				// Note when both PADOPTION_MOUSE_R and PADOPTION_MOUSE_L are set, take only the right one
				if (conf->options & (PADOPTION_MOUSE_R << 16 * pad)) {
					pad_x = PAD_R_RIGHT;
					pad_y = PAD_R_UP;
				} else {
					pad_x = PAD_L_RIGHT;
					pad_y = PAD_L_UP;
				}

				unsigned x = evt.key & 0xFFFF;
				unsigned int value = abs(s_previous_mouse_x - x) * conf->sensibility;

				if (x == 0)
					key_status->press(pad, pad_x, -MAX_ANALOG_VALUE);
				else if (x == 0xFFFF)
					key_status->press(pad, pad_x, MAX_ANALOG_VALUE);
				else if (x < (s_previous_mouse_x -2))
					key_status->press(pad, pad_x, -value);
				else if (x > (s_previous_mouse_x +2))
					key_status->press(pad, pad_x, value);
				else
					key_status->release(pad, pad_x);


				unsigned y = evt.key >> 16;
				value = abs(s_previous_mouse_y - y) * conf->sensibility;

				if (y == 0)
					key_status->press(pad, pad_y, -MAX_ANALOG_VALUE);
				else if (y == 0xFFFF)
					key_status->press(pad, pad_y, MAX_ANALOG_VALUE);
				else if (y < (s_previous_mouse_y -2))
					key_status->press(pad, pad_y, -value);
				else if (y > (s_previous_mouse_y +2))
					key_status->press(pad, pad_y, value);
				else
					key_status->release(pad, pad_y);

				s_previous_mouse_x = x;
				s_previous_mouse_y = y;
			}

			break;
	}
}

void PollForX11KeyboardInput(int pad)
{
	keyEvent evt;
	XEvent E;
	XButtonEvent* BE;

	// Keyboard input send by PCSX2
	while (!ev_fifo.empty()) {
		AnalyzeKeyEvent(pad, ev_fifo.front());
		pthread_spin_lock(&mutex_KeyEvent);
		ev_fifo.pop();
		pthread_spin_unlock(&mutex_KeyEvent);
	}

	// keyboard input
	while (XPending(GSdsp) > 0)
	{
		XNextEvent(GSdsp, &E);
		evt.evt = E.type;
		evt.key = (int)XLookupKeysym((XKeyEvent *) & E, 0);
		// Change the format of the structure to be compatible with GSOpen2
		// mode (event come from pcsx2 not X)
		BE = (XButtonEvent*)&E;
		switch (evt.evt) {
			case MotionNotify: evt.key = (BE->x & 0xFFFF) | (BE->y << 16); break;
			case ButtonRelease:
			case ButtonPress: evt.key = BE->button; break;
			default: break;
		}
		AnalyzeKeyEvent(pad, evt);
	}
}

bool PollX11KeyboardMouseEvent(u32 &pkey)
{
	GdkEvent *ev = gdk_event_get();

	if (ev != NULL)
	{
		if (ev->type == GDK_KEY_PRESS) {

			if (ev->key.keyval == GDK_Escape)
				pkey = 0;
			else
				pkey = ev->key.keyval;

			return true;
		} else if(ev->type == GDK_BUTTON_PRESS) {
			pkey = ev->button.button;
			return true;
		}
	}

	return false;
}

#else
LRESULT WINAPI PADwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool lbutton = false, rbutton = false;
	key_status->keyboard_state_acces(cpad);

	switch (msg)
	{
		case WM_KEYDOWN:
			if (lParam & 0x40000000) return TRUE;

			for (int pad = 0; pad < 2; ++pad)
			{
				for (int i = 0; i < MAX_KEYS; i++)
				{
					if (wParam ==  get_key(pad, i))
					{
						key_status->press(pad, i);
						break;
					}
				}
			}

			event.evt = KEYPRESS;
			event.key = wParam;
			break;

		case WM_KEYUP:
			for (int pad = 0; pad < 2; ++pad)
			{
				for (int i = 0; i < MAX_KEYS; i++)
				{
					if (wParam == get_key(pad,i))
					{
						key_status->release(pad, i);
						break;
					}
				}
			}


			event.evt = KEYRELEASE;
			event.key = wParam;
			break;

		case WM_DESTROY:
		case WM_QUIT:
			event.evt = KEYPRESS;
			event.key = VK_ESCAPE;
			return GSwndProc(hWnd, msg, wParam, lParam);

		default:
			return GSwndProc(hWnd, msg, wParam, lParam);
	}

	for (int pad = 0; pad < 2; ++pad)
		key_status->commit_status(pad);

	return TRUE;
}
#endif
