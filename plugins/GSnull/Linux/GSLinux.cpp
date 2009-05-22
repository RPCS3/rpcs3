/*  GSnull
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

#include "GSLinux.h"

Display *display;
int screen;

int GSOpenWindow(void *pDsp, char *Title)
{
	display = XOpenDisplay(0);
	screen = DefaultScreen(display);

	if( pDsp != NULL )
		*(Display**)pDsp = display;
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
	switch(ev->evt) {
		case KEYPRESS:
			switch(ev->key) {
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
			switch(ev->key) {
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
