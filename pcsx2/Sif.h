/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#ifndef __SIF_H__
#define __SIF_H__

#include "Common.h"

struct sifData{
        int     data,
                words,
                count,
                addr;
};

int eeSifTransfer;

DMACh *sif0ch;
DMACh *sif1ch;
DMACh *sif2ch;

int  sifInit();
void SIF0Dma();
void SIF1Dma();
void dmaSIF0();
void dmaSIF1();
void dmaSIF2();
void  sif1Interrupt();
void  sif0Interrupt();
void  EEsif1Interrupt();
void  EEsif0Interrupt();
int  EEsif2Interrupt();
int  sifFreeze(gzFile f, int Mode);


#endif /* __SIF_H__ */
