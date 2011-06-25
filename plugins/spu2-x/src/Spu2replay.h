//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
//

#pragma once

//#define S2R_ENABLE

// s2r dumping
int s2r_open(u32 ticks, char *filename);
void s2r_readreg(u32 ticks,u32 addr);
void s2r_writereg(u32 ticks,u32 addr,s16 value);
void s2r_writedma4(u32 ticks,u16*data,u32 len);
void s2r_writedma7(u32 ticks,u16*data,u32 len);
void s2r_close();

extern bool replay_mode;
