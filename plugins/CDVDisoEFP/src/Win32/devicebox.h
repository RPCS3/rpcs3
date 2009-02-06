/*  devicebox.h
 *  Copyright (C) 2002-2005  PCSX2 Team
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
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#ifndef DEVICEBOX_H
#define DEVICEBOX_H


#include <windows.h> // HWND


extern HWND deviceboxwindow;

extern void DeviceBoxDestroy();
// extern void DeviceBoxUnfocus();
extern void DeviceBoxRefocus();
extern void DeviceBoxDisplay();
extern BOOL CALLBACK DeviceBoxCallback(HWND window,
                                       UINT msg,
                                       WPARAM param,
                                       LPARAM param2);


#endif /* DEVICEBOX_H */
