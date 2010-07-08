/*  device.h
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */
#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <time.h> // time_t
#ifndef __LINUX__
#ifdef __linux__
#define __LINUX__
#endif /* __linux__ */
#endif /* No __LINUX__ */
#define CDVDdefs
#include "../PS2Edefs.h"

// #define VERBOSE_FUNCTION_DEVICE
// #define VERBOSE_WARNINGS
#define VERBOSE_DISC_TYPE
#define VERBOSE_DISC_INFO

// Device Data

extern int devicehandle;
extern s32 devicecapability; // Need to export?
extern time_t lasttime;
extern s32 traystatus;
extern s32 disctype;
extern u8 tocbuffer[];

// Device Functions
extern void DeviceInit();
extern void InitDisc();
extern s32 DiscInserted();
extern s32 DeviceOpen();
extern void DeviceClose();
extern s32 DeviceReadTrack(u32 lsn, int mode, u8 *buffer);
extern s32 DeviceBufferOffset();
extern s32 DeviceGetTD(u8 track, cdvdTD *cdvdtd);
extern s32 DeviceGetDiskType();
extern s32 DeviceTrayStatus();
extern s32 DeviceTrayOpen();
extern s32 DeviceTrayClose();

#endif /* __DEVICE_H__ */
