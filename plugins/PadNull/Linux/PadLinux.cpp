/*  PadNull
 *  Copyright (C) 2004-2009 PCSX2 Team
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

#include "PadLinux.h"

Display *GSdsp;
int autoRepeatMode;

void _PadUpdate(int pad)
{
	XEvent evt;
	KeySym key;

	// keyboard input
	while (XPending(GSdsp) > 0)
	{
		XNextEvent(GSdsp, &evt);
		switch (evt.type)
		{
			case KeyPress:
				key = XLookupKeysym((XKeyEvent *) &evt, 0);

				// Add code to check if it's one of the keys we configured here on a real pda plugin..

				event.evt = KEYPRESS;
				event.key = key;
				break;

			case KeyRelease:
				key = XLookupKeysym((XKeyEvent *) &evt, 0);

				// Add code to check if it's one of the keys we configured here on a real pda plugin..

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
}

s32  _PADOpen(void *pDsp)
{
	GSdsp = *(Display**)pDsp;
	XAutoRepeatOff(GSdsp);

	return 0;
}

void  _PADClose()
{
	XAutoRepeatOn(GSdsp);
}
