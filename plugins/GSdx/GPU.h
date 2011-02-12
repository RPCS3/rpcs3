/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#pragma pack(push, 1)

#include "GS.h"

enum
{
	GPU_POLYGON = 1,
	GPU_LINE = 2,
	GPU_SPRITE = 3,
};

REG32_(GPUReg, STATUS)
	uint32 TX:4;
	uint32 TY:1;
	uint32 ABR:2;
	uint32 TP:2;
	uint32 DTD:1;
	uint32 DFE:1;
	uint32 MD:1;
	uint32 ME:1;
	uint32 _PAD0:3;
	uint32 WIDTH1:1;
	uint32 WIDTH0:2;
	uint32 HEIGHT:1;
	uint32 ISPAL:1;
	uint32 ISRGB24:1;
	uint32 ISINTER:1;
	uint32 DEN:1;
	uint32 _PAD1:2;
	uint32 IDLE:1;
	uint32 IMG:1;
	uint32 COM:1;
	uint32 DMA:2;
	uint32 LCF:1;
	/*
	uint32 TX:4;
	uint32 TY:1;
	uint32 ABR:2;
	uint32 TP:2;
	uint32 DTD:1;
	uint32 DFE:1;
	uint32 PBW:1;
	uint32 PBC:1;
	uint32 _PAD0:3;
	uint32 HRES2:1;
	uint32 HRES1:2;
	uint32 VRES:1;
	uint32 ISPAL:1;
	uint32 ISRGB24:1;
	uint32 ISINTER:1;
	uint32 ISSTOP:1;
	uint32 _PAD1:1;
	uint32 DMARDY:1;
	uint32 IDIDLE:1;
	uint32 DATARDY:1;
	uint32 ISEMPTY:1;
	uint32 TMODE:2;
	uint32 ODE:1;
	*/
REG_END

REG32_(GPUReg, PACKET)
	uint32 _PAD:24;
	uint32 OPTION:5;
	uint32 TYPE:3;
REG_END

REG32_(GPUReg, PRIM)
	uint32 VTX:24;
	uint32 TGE:1;
	uint32 ABE:1;
	uint32 TME:1;
	uint32 _PAD2:1;
	uint32 IIP:1;
	uint32 TYPE:3;
REG_END

REG32_(GPUReg, POLYGON)
	uint32 _PAD:24;
	uint32 TGE:1;
	uint32 ABE:1;
	uint32 TME:1;
	uint32 VTX:1;
	uint32 IIP:1;
	uint32 TYPE:3;
REG_END

REG32_(GPUReg, LINE)
	uint32 _PAD:24;
	uint32 ZERO1:1;
	uint32 ABE:1;
	uint32 ZERO2:1;
	uint32 PLL:1;
	uint32 IIP:1;
	uint32 TYPE:3;
REG_END

REG32_(GPUReg, SPRITE)
	uint32 _PAD:24;
	uint32 ZERO:1;
	uint32 ABE:1;
	uint32 TME:1;
	uint32 SIZE:2;
	uint32 TYPE:3;
REG_END

REG32_(GPUReg, RESET)
	uint32 _PAD:32;
REG_END

REG32_(GPUReg, DEN)
	uint32 DEN:1;
	uint32 _PAD:31;
REG_END

REG32_(GPUReg, DMA)
	uint32 DMA:2;
	uint32 _PAD:30;
REG_END

REG32_(GPUReg, DAREA)
	uint32 X:10;
	uint32 Y:9;
	uint32 _PAD:13;
REG_END

REG32_(GPUReg, DHRANGE)
	uint32 X1:12;
	uint32 X2:12;
	uint32 _PAD:8;
REG_END

REG32_(GPUReg, DVRANGE)
	uint32 Y1:10;
	uint32 Y2:11;
	uint32 _PAD:11;
REG_END

REG32_(GPUReg, DMODE)
	uint32 WIDTH0:2;
	uint32 HEIGHT:1;
	uint32 ISPAL:1;
	uint32 ISRGB24:1;
	uint32 ISINTER:1;
	uint32 WIDTH1:1;
	uint32 REVERSE:1;
	uint32 _PAD:24;
REG_END

REG32_(GPUReg, GPUINFO)
	uint32 PARAM:24;
	uint32 _PAD:8;
REG_END

REG32_(GPUReg, MODE)
	uint32 TX:4;
	uint32 TY:1;
	uint32 ABR:2;
	uint32 TP:2;
	uint32 DTD:1;
	uint32 DFE:1;
	uint32 _PAD:21;
REG_END

REG32_(GPUReg, MASK)
	uint32 MD:1;
	uint32 ME:1;
	uint32 _PAD:30;
REG_END

REG32_(GPUReg, DRAREA)
	uint32 X:10;
	uint32 Y:10;
	uint32 _PAD:12;
REG_END

REG32_(GPUReg, DROFF)
	int32 X:11;
	int32 Y:11;
	int32 _PAD:10;
REG_END

REG32_(GPUReg, RGB)
	uint32 R:8;
	uint32 G:8;
	uint32 B:8;
	uint32 _PAD:8;
REG_END

REG32_(GPUReg, XY)
	int32 X:11;
	int32 _PAD1:5;
	int32 Y:11;
	int32 _PAD2:5;
REG_END

REG32_(GPUReg, UV)
	uint32 U:8;
	uint32 V:8;
	uint32 _PAD:16;
REG_END

REG32_(GPUReg, TWIN)
	uint32 TWW:5;
	uint32 TWH:5;
	uint32 TWX:5;
	uint32 TWY:5;
	uint32 _PAD:12;
REG_END

REG32_(GPUReg, CLUT)
	uint32 _PAD1:16;
	uint32 X:6;
	uint32 Y:9;
	uint32 _PAD2:1;
REG_END

REG32_SET(GPUReg)
	GPURegSTATUS STATUS;
	GPURegPACKET PACKET;
	GPURegPRIM PRIM;
	GPURegPOLYGON POLYGON;
	GPURegLINE LINE;
	GPURegSPRITE SPRITE;
	GPURegRESET RESET;
	GPURegDEN DEN;
	GPURegDMA DMA;
	GPURegDAREA DAREA;
	GPURegDHRANGE DHRANGE;
	GPURegDVRANGE DVRANGE;
	GPURegDMODE DMODE;
	GPURegGPUINFO GPUINFO;
	GPURegMODE MODE;
	GPURegMASK MASK;
	GPURegDRAREA DRAREA;
	GPURegDROFF DROFF;
	GPURegRGB RGB;
	GPURegXY XY;
	GPURegUV UV;
	GPURegTWIN TWIN;
	GPURegCLUT CLUT;
REG_SET_END

struct GPUFreezeData
{
	uint32 version; // == 1
	uint32 status;
	uint32 control[256];
	uint16 vram[1024 * 1024];
};

#pragma pack(pop)

