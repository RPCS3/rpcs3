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

#ifndef __COLOR_H__
#define __COLOR_H__

void (*drawPixel) (int, int, u32);
void SETdrawPixel();

void (*drawPixelF) (int, int, u32, u32);
void SETdrawPixelF();

void (*drawPixelZ) (int, int, u32, u32);
u32  (*readPixelZ) (int, int);
void SETdrawPixelZ();

void (*drawPixelZF) (int, int, u32, u32, u32);
void SETdrawPixelZF();

void colSetCol(u32 crgba);
u32  colModulate(u32 texcol);
u32  colHighlight(u32 texcol);
u32  colHighlight2(u32 texcol);

#endif /* __COLOR_H__ */
