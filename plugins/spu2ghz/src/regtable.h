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
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef _REGTABLE_H_
#define _REGTABLE_H_

#define U16P(x) ((u16*)&(x))

#define PCORE(c,p) \
	U16P(Cores[c].##p)

#define PVCP(c,v,p) \
	PCORE(c,Voices[v].##p)

#define PVC(c,v) \
	PVCP(c,v,VolumeL.Reg_VOL), \
	PVCP(c,v,VolumeR.Reg_VOL), \
	PVCP(c,v,Pitch), \
	PVCP(c,v,ADSR.Reg_ADSR1), \
	PVCP(c,v,ADSR.Reg_ADSR2), \
	PVCP(c,v,ADSR.Value)+1, \
	PVCP(c,v,VolumeL.Value), \
	PVCP(c,v,VolumeR.Value)

#define PVCA(c,v) \
	PVCP(c,v,StartA)+1, \
	PVCP(c,v,StartA), \
	PVCP(c,v,LoopStartA)+1, \
	PVCP(c,v,LoopStartA), \
	PVCP(c,v,NextA)+1, \
	PVCP(c,v,NextA)

#define PRAW(a) \
	((u16*)NULL)

#define PREVB_REG(c,n) \
	PCORE(c,Revb.##n)+1, \
	PCORE(c,Revb.##n)

// Yay!  Global namespace pollution 101!
static const u16 zero=0;

extern u16* regtable[];

#endif