/*  GSsoft
 *  Copyright (C) 2002-2005  GSsoft Team
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
 */

#ifndef __MEM_H__
#define __MEM_H__


void writePixel32(int x, int y, u32 pixel, u32 bp, u32 bw);
void writePixel24(int x, int y, u32 pixel, u32 bp, u32 bw);
void writePixel16(int x, int y, u16 pixel, u32 bp, u32 bw);
void writePixel16S(int x, int y, u16 pixel, u32 bp, u32 bw);
void writePixel8(int x, int y, u8 pixel, u32 bp, u32 bw);
void writePixel8H(int x, int y, u8 pixel, u32 bp, u32 bw);
void writePixel4(int x, int y, u8 pixel, u32 bp, u32 bw);
void writePixel4HL(int x, int y, u8 pixel, u32 bp, u32 bw);
void writePixel4HH(int x, int y, u8 pixel, u32 bp, u32 bw);

u32  readPixel32(int x, int y, u32 bp, u32 bw);
u32  readPixel24(int x, int y, u32 bp, u32 bw);
u16  readPixel16(int x, int y, u32 bp, u32 bw);
u16  readPixel16S(int x, int y, u32 bp, u32 bw);
u8   readPixel8(int x, int y, u32 bp, u32 bw);
u8   readPixel8H(int x, int y, u32 bp, u32 bw);
u8   readPixel4(int x, int y, u32 bp, u32 bw);
u8   readPixel4HL(int x, int y, u32 bp, u32 bw);
u8   readPixel4HH(int x, int y, u32 bp, u32 bw);

void writePixel32Z(int x, int y, u32 pixel, u32 bp, u32 bw);
void writePixel24Z(int x, int y, u32 pixel, u32 bp, u32 bw);
void writePixel16Z(int x, int y, u16 pixel, u32 bp, u32 bw);
void writePixel16SZ(int x, int y, u16 pixel, u32 bp, u32 bw);

u32  readPixel32Z(int x, int y, u32 bp, u32 bw);
u32  readPixel24Z(int x, int y, u32 bp, u32 bw);
u16  readPixel16Z(int x, int y, u32 bp, u32 bw);
u16  readPixel16SZ(int x, int y, u32 bp, u32 bw);

u32  readPixel16to32(int x, int y, u32 bp, u32 bw);
u32  readPixel16Sto32(int x, int y, u32 bp, u32 bw);

u32  readPixel16Zto32(int x, int y, u32 bp, u32 bw);
u32  readPixel16SZto32(int x, int y, u32 bp, u32 bw);

#endif /* __MEM_H__ */
