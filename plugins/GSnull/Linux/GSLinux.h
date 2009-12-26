/*  GSnull
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#ifndef __GSLINUX_H__
#define __GSLINUX_H__

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

extern int GSOpenWindow(void *pDsp, char *Title);
extern int GSOpenWindow2(void *pDsp, u32 flags);
extern void GSCloseWindow();
extern void GSProcessMessages();
extern void HandleKeyEvent(keyEvent *ev);

#endif
