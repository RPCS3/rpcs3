/***************************************************************************
                          alsa.h  -  description
                             -------------------
    begin                : Sat Mar 01 2003
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _ALSA_SOUND_H
#define _ALSA_SOUND_H

#ifdef ALSA_MEM_DEF
#define ALSA_MEM_EXTERN
#else
#define ALSA_MEM_EXTERN extern
#endif

ALSA_MEM_EXTERN int sound_buffer_size;

#endif // _ALSA_SOUND_H
