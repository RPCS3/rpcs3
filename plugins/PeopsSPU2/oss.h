/***************************************************************************
                          oss_sound.h  -  description
                             -------------------
    begin                : Wed Dec 8 1999
    copyright            : (C) 1999 by Marcin "Duddie" Dudar
    email                : duddie@psemu.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _OSS_SOUND_H
#define _OSS_SOUND_H

#ifdef OSS_MEM_DEF
#define OSS_MEM_EXTERN
#else
#define OSS_MEM_EXTERN extern
#endif

OSS_MEM_EXTERN int sound_buffer_size;

#define OSS_MODE_STEREO	1
#define OSS_MODE_MONO		0

#define OSS_SPEED_44100	44100

void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

#endif // _OSS_SOUND_H
