/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
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

#ifndef __SOCKS_H__
#define __SOCKS_H__

long sockOpen(char *Device);
void sockClose();
long sockSendData(void *pData, int Size);
long sockRecvData(void *pData, int Size);
long sockGetDevicesNum();
char *sockGetDevice(int index);
char *sockGetDeviceDesc(int index);

#endif /* __SOCKS_H__*/
