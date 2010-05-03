/*  GSnull
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GS.h"
#include "GSLinux.h"

Display *display;
int screen;
GtkScrolledWindow *win;

int GSOpenWindow(void *pDsp, char *Title)
{
	display = XOpenDisplay(0);
	screen = DefaultScreen(display);

	if (pDsp != NULL)
		*(Display**)pDsp = display;
	else
		return -1;

	return 0;
}

int GSOpenWindow2(void *pDsp, u32 flags)
{
    GtkWidget *widget;
	if (pDsp != NULL)
		win = *(GtkScrolledWindow**)pDsp;
	else
		return -1;

	return 0;
}

void GSCloseWindow()
{
	XCloseDisplay(display);
}

void GSProcessMessages()
{
	if ( GSKeyEvent )
		{
		int myKeyEvent = GSKeyEvent;
		bool myShift = GSShift;
		GSKeyEvent = 0;

		switch ( myKeyEvent )
		{
			case XK_F5:
			 	OnKeyboardF5(myShift);
				break;
			case XK_F6:
				OnKeyboardF6(myShift);
				break;
			case XK_F7:
				OnKeyboardF7(myShift);
				break;
			case XK_F9:
				OnKeyboardF9(myShift);
				break;
		}
	}
}


void HandleKeyEvent(keyEvent *ev)
{
	switch(ev->evt)
	{
		case KEYPRESS:
			switch(ev->key)
			{
				case XK_F5:
				case XK_F6:
				case XK_F7:
				case XK_F9:
					GSKeyEvent = ev->key ;
					break;
				case XK_Escape:
					break;
				case XK_Shift_L:
				case XK_Shift_R:
					GSShift = true;
					break;
				case XK_Alt_L:
				case XK_Alt_R:
					GSAlt = true;
					break;
			}
			break;
		case KEYRELEASE:
			switch(ev->key)
			{
				case XK_Shift_L:
				case XK_Shift_R:
					GSShift = false;
					break;
				case XK_Alt_L:
				case XK_Alt_R:
					GSAlt = false;
					break;
			}
	}
}
