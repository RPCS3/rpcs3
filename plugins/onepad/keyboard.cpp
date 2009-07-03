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
 
 /*
  * Theoretically, this header is for anything to do with keyboard input.
  * Pragmatically, event handing's going in here too.
  */
  
 #include "keyboard.h"
 

__forceinline int FindKey(int key, int pad)
{
	for (int p = 0; p < PADSUBKEYS; p++)
		for (int i = 0; i < PADKEYS; i++)
			if (key ==  get_key(PadEnum[pad][p], i)) return i;
	return -1;
}

char* KeysymToChar(int keysym)
{
 #ifdef __LINUX__
	return XKeysymToString(keysym);
#else
	LPWORD temp;

	ToAscii((UINT) keysym, NULL, NULL, temp, NULL);
	return (char*)temp;
	#endif
}

void PollForKeyboardInput(int pad)
{
 #ifdef __LINUX__
	PollForX11KeyboardInput(pad);
#endif
}

void SetAutoRepeat(bool autorep)
{
 #ifdef __LINUX__
	if (autorep)
		XAutoRepeatOn(GSdsp);
	else
		XAutoRepeatOff(GSdsp);
#endif
}

 #ifdef __LINUX__
void PollForX11KeyboardInput(int pad)
{
	XEvent E;
	KeySym key;
	int keyPress = 0, keyRelease = 0;
	int i;
	
	// keyboard input
	while (XPending(GSdsp) > 0)
	{
		XNextEvent(GSdsp, &E);
		switch (E.type)
		{
			case KeyPress:
				key = XLookupKeysym((XKeyEvent *) & E, 0);

				i = FindKey(key, pad);
			
				// Analog controls.
				if ((i > PAD_RY) && (i <= PAD_R_LEFT))
				{
				switch (i)
				{
					case PAD_R_LEFT:
					case PAD_R_UP:
					case PAD_L_LEFT:
					case PAD_L_UP:
						Analog::ConfigurePad(Analog::AnalogToPad(i), pad, DEF_VALUE);
						break;
					case PAD_R_RIGHT:
					case PAD_R_DOWN:
					case PAD_L_RIGHT:
					case PAD_L_DOWN:
						Analog::ConfigurePad(Analog::AnalogToPad(i), pad, -DEF_VALUE);
						break;
				}
				i += 0xff00;
				}
				
				if (i != -1) 
				{
					clear_bit(keyRelease, i); 
					set_bit(keyPress, i);
				}
				//PAD_LOG("Key pressed:%d\n", i);

				event.evt = KEYPRESS;
				event.key = key;
				break;
				
			case KeyRelease:
				key = XLookupKeysym((XKeyEvent *) & E, 0);

				i = FindKey(key, pad);
			
				// Analog Controls.
				if ((i > PAD_RY) && (i <= PAD_R_LEFT))
				{
					Analog::ResetPad(Analog::AnalogToPad(i), pad);
					i += 0xff00;
				}
				
				if (i != -1) 
				{
					clear_bit(keyPress, i); 
					set_bit(keyRelease, i);
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

	UpdateKeys(pad, keyPress, keyRelease);
}

bool PollX11Keyboard(char* &temp, u32 &pkey)
{
	GdkEvent *ev = gdk_event_get();
	
	if (ev != NULL)
	{
		if (ev->type == GDK_KEY_PRESS)
		{
			
			if (ev->key.keyval == GDK_Escape) 
			{
				temp = "Unknown";
				pkey = NULL;
			}
			else
			{
				temp = KeysymToChar(ev->key.keyval);
				pkey = ev->key.keyval;
			}
			
			return true;
		}
	}
	
	return false;
}
#else
LRESULT WINAPI PADwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int keyPress[2] = {0}, keyRelease[2] = {0};
	static bool lbutton = false, rbutton = false;

	switch (msg)
	{
		case WM_KEYDOWN:
			if (lParam & 0x40000000) return TRUE;

			for (int pad = 0; pad < 2; ++pad)
			{
				for (int i = 0; i < PADKEYS; i++)
				{
					if (wParam ==  get_key(pad, i))
					{
						set_bit(keyPress[pad], i);
						clear_bit(keyRelease[pad], i);
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
				for (int i = 0; i < PADKEYS; i++)
				{
					if (wParam == get_key(pad,i))
					{
						set_bit(keyRelease[pad], i);
						clear_bit(keyPress[pad], i);
						break;
					}
				}
			}


			event.evt = KEYRELEASE;
			event.key = wParam;
			break;

		/*case WM_LBUTTONDOWN:
			lbutton = true;
			break;

		case WM_LBUTTONUP:
			g_lanalog[0].x = 0x80;
			g_lanalog[0].y = 0x80;
			g_lanalog[1].x = 0x80;
			g_lanalog[1].y = 0x80;
			lbutton = false;
			break;

		case WM_RBUTTONDOWN:
			rbutton = true;
			break;

		case WM_RBUTTONUP:
			g_ranalog[0].x = 0x80;
			g_ranalog[0].y = 0x80;
			g_ranalog[1].x = 0x80;
			g_ranalog[1].y = 0x80;
			rbutton = false;
			break;

		case WM_MOUSEMOVE:
			if (lbutton)
			{
				g_lanalog[0].x = LOWORD(lParam) & 254;
				g_lanalog[0].y = HIWORD(lParam) & 254;
				g_lanalog[1].x = LOWORD(lParam) & 254;
				g_lanalog[1].y = HIWORD(lParam) & 254;
			}
			if (rbutton)
			{
				g_ranalog[0].x = LOWORD(lParam) & 254;
				g_ranalog[0].y = HIWORD(lParam) & 254;
				g_ranalog[1].x = LOWORD(lParam) & 254;
				g_ranalog[1].y = HIWORD(lParam) & 254;
			}
			break;*/

		case WM_DESTROY:
		case WM_QUIT:
			event.evt = KEYPRESS;
			event.key = VK_ESCAPE;
			return GSwndProc(hWnd, msg, wParam, lParam);

		default:
			return GSwndProc(hWnd, msg, wParam, lParam);
	}

	for (int pad = 0; pad < 2; ++pad)
	{
		UpdateKeys(pad, keyPress[pad], keyRelease[pad]);
	}

	return TRUE;
}
#endif
