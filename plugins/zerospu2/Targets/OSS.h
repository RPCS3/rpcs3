/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
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

#ifndef OSS_H_INCLUDED
#define OSS_H_INCLUDED

#define OSS_MODE_STEREO	1

extern int OSSSetupSound();
extern void OSSRemoveSound();
extern int OSSSoundGetBytesBuffered();
extern void OSSSoundFeedVoiceData(unsigned char* pSound,long lBytes);

static SoundCallbacks OSSCmds =
{
	(intFunction)OSSSetupSound,
	(voidFunction)OSSRemoveSound,
	(intFunction)OSSSoundGetBytesBuffered,
	(soundFeedFunction)OSSSoundFeedVoiceData
};

#endif // OSS_H_INCLUDED
