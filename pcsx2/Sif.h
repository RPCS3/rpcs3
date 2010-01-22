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

#define FIFO_SIF_W 128

struct sifData
{
	s32 data;
	s32 words;
	s32 count;
	s32 addr;
};

struct sifFifo
{
	u32 data[FIFO_SIF_W];
	s32 readPos;
	s32 writePos;
	s32 size;
	
	void write(u32 *from, int words)
	{
		const int wP0 = min((FIFO_SIF_W - writePos), words);
		const int wP1 = words - wP0;

		memcpy(&data[writePos], from, wP0 << 2);
		memcpy(&data[0], &from[wP0], wP1 << 2);

		writePos = (writePos + words) & (FIFO_SIF_W - 1);
		size += words;
		SIF_LOG("  SIF + %d = %d (pos=%d)", words, size, writePos);
	}
	
	void read(u32 *to, int words)
	{
		const int wP0 = min((FIFO_SIF_W - readPos), words);
		const int wP1 = words - wP0;

		memcpy(to, &data[readPos], wP0 << 2);
		memcpy(&to[wP0], &data[0], wP1 << 2);

		readPos = (readPos + words) & (FIFO_SIF_W - 1);
		size -= words;
		SIF_LOG("  SIF - %d = %d (pos=%d)", words, size, readPos);
	}
};

struct _sif
{
	sifFifo fifo;
	s32 chain;
	s32 end;
	s32 tagMode;
	s32 counter;
	struct sifData sifData;
};
extern DMACh *sif0ch, *sif1ch, *sif2ch;

extern void sifInit();

extern void SIF0Dma();
extern void SIF1Dma();

extern void dmaSIF0();
extern void dmaSIF1();
extern void dmaSIF2();

extern void EEsif0Interrupt();
extern void EEsif1Interrupt();

extern void sif0Interrupt();
extern void sif1Interrupt();


#endif /* __SIF_H__ */
