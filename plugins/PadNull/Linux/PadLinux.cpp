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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <gdk/gdkx.h>
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
	
    GtkScrolledWindow *win;

    win = *(GtkScrolledWindow**) pDsp;

	if (GTK_IS_WIDGET(win))
	{
	    // Since we have a GtkScrolledWindow, for now we'll grab whatever display
	    // comes along instead. Later, we can fiddle with this, but I'm not sure the
	    // best way to get a Display* out of a GtkScrolledWindow. A GtkWindow I might
	    // be able to manage... --arcum42
        GSdsp = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
	}
	else
	{
        GSdsp = *(Display**)pDsp;
	}
	
	XAutoRepeatOff(GSdsp);

	return 0;
}

void  _PADClose()
{
	XAutoRepeatOn(GSdsp);
}
