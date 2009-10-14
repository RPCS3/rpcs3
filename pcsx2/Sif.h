/*  PCSX2 - PS2 Emulator for PCs
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

#ifndef __SIF_H__
#define __SIF_H__

#define FIFO_SIF0_W 128
#define FIFO_SIF1_W 128

struct sifData
{
	s32 data;
	s32 words;
	s32 count;
	s32 addr;
};

struct _sif0
{
	u32 fifoData[FIFO_SIF0_W];
	s32 fifoReadPos;
	s32 fifoWritePos;
	s32 fifoSize;
	s32 chain;
	s32 end;
	s32 tagMode;
	s32 counter;
	struct sifData sifData;
};

struct _sif1
{
	u32 fifoData[FIFO_SIF1_W];
	s32 fifoReadPos;
	s32 fifoWritePos;
	s32 fifoSize;
	s32 chain;
	s32 end;
	s32 tagMode;
	s32 counter;
	struct sifData sifData;
};
extern DMACh *sif0ch;
extern DMACh *sif1ch;
extern DMACh *sif2ch;

extern void sifInit();
extern void SIF0Dma();
extern void SIF1Dma();
extern void dmaSIF0();
extern void dmaSIF1();
extern void dmaSIF2();
extern void sif1Interrupt();
extern void sif0Interrupt();
extern void EEsif1Interrupt();
extern void EEsif0Interrupt();
extern int  EEsif2Interrupt();
int sifFreeze(gzFile f, int Mode);


#endif /* __SIF_H__ */
