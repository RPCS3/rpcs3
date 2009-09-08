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


#include "PrecompiledHeader.h"

#include "Common.h"
#include "Cache.h"

_cacheS pCache[64];

namespace R5900{
namespace Interpreter 
{
#ifdef PCSX2_CACHE_EMU_MEM

int getFreeCache(u32 mem, int mode, int * way) {
	u8 * out;
	u32 paddr;
	u32 taddr[2];
	u8 * t;
	int number;
	int i = (mem >> 6) & 0x3F;
	
	paddr = getMemR(mem);
	taddr[0] = getMemW(pCache[i].tag[0]);
	taddr[1] = getMemW(pCache[i].tag[1]);

	if (taddr[0] == paddr && (pCache[i].tag[0] & 0x20)) 
	{
		*way = 0;
		return i;
	}
	else if(taddr[1] == paddr && (pCache[i].tag[1] & 0x20)) 
	{
		*way = 1;
		return i;
	}

	number = ((pCache[i].tag[0]>>4) & 1) ^ ((pCache[i].tag[1]>>4) & 1);
	
	if(pCache[i].tag[number] & 0x60)	// Valid Dirty
	{
		t = (u8*)(taddr[number]);
		out = (u8*)(t + (mem & 0xFC0));
		((u64*)out)[0] = ((u64*)pCache[i].data[number][0].b8._8)[0];
		((u64*)out)[1] = ((u64*)pCache[i].data[number][0].b8._8)[1];
		((u64*)out)[2] = ((u64*)pCache[i].data[number][1].b8._8)[0];
		((u64*)out)[3] = ((u64*)pCache[i].data[number][1].b8._8)[1];
		((u64*)out)[4] = ((u64*)pCache[i].data[number][2].b8._8)[0];
		((u64*)out)[5] = ((u64*)pCache[i].data[number][2].b8._8)[1];
		((u64*)out)[6] = ((u64*)pCache[i].data[number][3].b8._8)[0];
		((u64*)out)[7] = ((u64*)pCache[i].data[number][3].b8._8)[1];
	}
	
	if(mode == 1)
	{
		pCache[i].tag[number] |= 0x40;	// Set Dirty Bit if mode == write
	}

	pCache[i].tag[number] &= ~(0xFFFFF000);
	pCache[i].tag[number] |= ((mem>>12) & 0xFFFFF) << 12;


	t = (u8 *)paddr; 
	out=  (u8*)(t + (mem & 0xFC0));
	((u64*)pCache[i].data[number][0].b8._8)[0] = ((u64*)out)[0];
	((u64*)pCache[i].data[number][0].b8._8)[1] = ((u64*)out)[1];
	((u64*)pCache[i].data[number][1].b8._8)[0] = ((u64*)out)[2];
	((u64*)pCache[i].data[number][1].b8._8)[1] = ((u64*)out)[3];
	((u64*)pCache[i].data[number][2].b8._8)[0] = ((u64*)out)[4];
	((u64*)pCache[i].data[number][2].b8._8)[1] = ((u64*)out)[5];
	((u64*)pCache[i].data[number][3].b8._8)[0] = ((u64*)out)[6];
	((u64*)pCache[i].data[number][3].b8._8)[1] = ((u64*)out)[7];

	if(pCache[i].tag[number] & 0x10)
		pCache[i].tag[number] &= ~(0x10);
	else 
		pCache[i].tag[number] |= 0x10;
	
	pCache[i].tag[number] |= 0x20;
	*way = number;
	return i;
}

void writeCache8(u32 mem, u8 value) {
	int i, number;

	i = getFreeCache(mem,1,&number);
//	CACHE_LOG("writeCache8 %8.8x adding to %d, way %d, value %x", mem, i,number,value);

	pCache[i].data[number][(mem>>4) & 0x3].b8._8[(mem&0xf)] = value;
}

void writeCache16(u32 mem, u16 value) {
	int i, number;
 
	i = getFreeCache(mem,1,&number);
//	CACHE_LOG("writeCache16 %8.8x adding to %d, way %d, value %x", mem, i,number,value);

	*(u16*)(&pCache[i].data[number][(mem>>4) & 0x3].b8._8[(mem&0xf)]) = value;
}

void writeCache32(u32 mem, u32 value) {
	int i, number;

	i = getFreeCache(mem,1,&number);
//	CACHE_LOG("writeCache32 %8.8x adding to %d, way %d, value %x", mem, i,number,value);
	*(u32*)(&pCache[i].data[number][(mem>>4) & 0x3].b8._8[(mem&0xf)]) = value;
}

void writeCache64(u32 mem, u64 value) {
	int i, number;

	i = getFreeCache(mem,1,&number);
//	CACHE_LOG("writeCache64 %8.8x adding to %d, way %d, value %x", mem, i,number,value);
	*(u64*)(&pCache[i].data[number][(mem>>4) & 0x3].b8._8[(mem&0xf)]) = value;
}

void writeCache128(u32 mem, u64 *value) {
	int i, number;

	i = getFreeCache(mem,1,&number);
//	CACHE_LOG("writeCache128 %8.8x adding to %d", mem, i);
	((u64*)pCache[i].data[number][(mem>>4) & 0x3].b8._8)[0] = value[0];
	((u64*)pCache[i].data[number][(mem>>4) & 0x3].b8._8)[1] = value[1];
}

u8 *readCache(u32 mem) {
	int i, number;

	i = getFreeCache(mem,0,&number);
//	CACHE_LOG("readCache %8.8x from %d, way %d", mem, i,number);

	return pCache[i].data[number][(mem>>4) & 0x3].b8._8;
}

namespace OpcodeImpl
{

extern int Dcache;
void CACHE() {
    u32 addr;
	//if(Dcache == 0) return;
   addr = cpuRegs.GPR.r[_Rs_].UL[0] + _Imm_;
	switch (_Rt_) {
		case 0x1a:
		{
			int index = (addr >> 6) & 0x3F;
			u32 paddr[2];
			int way;
			u32 taddr = getMemR(addr);
			paddr[0] = getMemW(pCache[index].tag[0]);
			paddr[1] = getMemW(pCache[index].tag[1]);

			if(paddr[0] == taddr && (pCache[index].tag[0] & 0x20))
			{
				way = 0;
			}
			else if(paddr[1] == taddr && (pCache[index].tag[1] & 0x20))
			{
				way = 1;
			} 
			else
			{
				return;
			}

			CACHE_LOG("CACHE DHIN addr %x, index %d, way %d, Flags %x",addr,index,way,pCache[index].tag[way] & 0x78);

			pCache[index].tag[way] &= ~(0x6F);
			((u64*)pCache[index].data[way][0].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][0].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][1].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][1].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][2].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][2].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][3].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][3].b8._8)[1] = 0;
			break;
		}
		case 0x18:
		{
			u8 * out;
			int index = (addr >> 6) & 0x3F;
			u32 paddr[2];
			int way;
			u32 taddr = getMemW(addr);
			paddr[0] = getMemW(pCache[index].tag[0]);
			paddr[1] = getMemW(pCache[index].tag[1]);

			if(paddr[0] == taddr && (pCache[index].tag[0] & 0x20))
			{
				way = 0;
			}
			else if(paddr[1] == taddr && (pCache[index].tag[1] & 0x20))
			{
				way = 1;
			} 
			else
			{
				return;
			}

			CACHE_LOG("CACHE DHWBIN addr %x, index %d, way %d, Flags %x",addr,index,way,pCache[index].tag[way] & 0x78);
	
			if(pCache[index].tag[way] & 0x60)	// Valid Dirty
			{
				char * t = (char *)(taddr);//paddr[way]);
				out = (u8*)(t + (addr & 0xFC0));
				((u64*)out)[0] = ((u64*)pCache[index].data[way][0].b8._8)[0];
				((u64*)out)[1] = ((u64*)pCache[index].data[way][0].b8._8)[1];
				((u64*)out)[2] = ((u64*)pCache[index].data[way][1].b8._8)[0];
				((u64*)out)[3] = ((u64*)pCache[index].data[way][1].b8._8)[1];
				((u64*)out)[4] = ((u64*)pCache[index].data[way][2].b8._8)[0];
				((u64*)out)[5] = ((u64*)pCache[index].data[way][2].b8._8)[1];
				((u64*)out)[6] = ((u64*)pCache[index].data[way][3].b8._8)[0];
				((u64*)out)[7] = ((u64*)pCache[index].data[way][3].b8._8)[1];
			}

			pCache[index].tag[way] &= ~(0x6F);
			((u64*)pCache[index].data[way][0].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][0].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][1].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][1].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][2].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][2].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][3].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][3].b8._8)[1] = 0;

			break;
		}
		case 0x1c:
		{
			u8 * out;
			int index = (addr >> 6) & 0x3F;
			u32 paddr[2];
			int way;
			u32 taddr = getMemW(addr);
			paddr[0] = getMemW(pCache[index].tag[0]);
			paddr[1] = getMemW(pCache[index].tag[1]);

			if(paddr[0] == taddr && (pCache[index].tag[0] & 0x20))
			{
				way = 0;
			}
			else if(paddr[1] == taddr && (pCache[index].tag[1] & 0x20))
			{
				way = 1;
			} 
			else
			{
				return;
			}
			CACHE_LOG("CACHE DHWOIN addr %x, index %d, way %d, Flags %x",addr,index,way,pCache[index].tag[way] & 0x78);
			
			if(pCache[index].tag[way] & 0x60)	// Valid Dirty
			{
				char * t = (char *)(taddr);
				out = (u8*)(t + (addr & 0xFC0));
				((u64*)out)[0] = ((u64*)pCache[index].data[way][0].b8._8)[0];
				((u64*)out)[1] = ((u64*)pCache[index].data[way][0].b8._8)[1];
				((u64*)out)[2] = ((u64*)pCache[index].data[way][1].b8._8)[0];
				((u64*)out)[3] = ((u64*)pCache[index].data[way][1].b8._8)[1];
				((u64*)out)[4] = ((u64*)pCache[index].data[way][2].b8._8)[0];
				((u64*)out)[5] = ((u64*)pCache[index].data[way][2].b8._8)[1];
				((u64*)out)[6] = ((u64*)pCache[index].data[way][3].b8._8)[0];
				((u64*)out)[7] = ((u64*)pCache[index].data[way][3].b8._8)[1];
			}

			pCache[index].tag[way] &= ~(0x40);
			break;
		}
		case 0x16:
		{
			int index = (addr >> 6) & 0x3F;
			int way = addr & 0x1;

			CACHE_LOG("CACHE DXIN addr %x, index %d, way %d, flag %x\n",addr,index,way,pCache[index].tag[way] & 0x78);

			pCache[index].tag[way] &= ~(0x6F);

		   ((u64*)pCache[index].data[way][0].b8._8)[0] = 0;
		   ((u64*)pCache[index].data[way][0].b8._8)[1] = 0;
		   ((u64*)pCache[index].data[way][1].b8._8)[0] = 0;
		   ((u64*)pCache[index].data[way][1].b8._8)[1] = 0;
		   ((u64*)pCache[index].data[way][2].b8._8)[0] = 0;
		   ((u64*)pCache[index].data[way][2].b8._8)[1] = 0;
		   ((u64*)pCache[index].data[way][3].b8._8)[0] = 0;
		   ((u64*)pCache[index].data[way][3].b8._8)[1] = 0;
			break;
		}
		case 0x11:
		{
			int index = (addr >> 6) & 0x3F;
			int way = addr & 0x1;
			u8 * out = pCache[index].data[way][(addr>>4) & 0x3].b8._8;
			cpuRegs.CP0.r[28] = *(u32 *)(out+(addr&0xf));

			CACHE_LOG("CACHE DXLDT addr %x, index %d, way %d, DATA %x",addr,index,way,cpuRegs.CP0.r[28]);

			break;
		}
		case 0x10:
		{
			int index = (addr >> 6) & 0x3F;
			int way = addr & 0x1;
			
			cpuRegs.CP0.r[28] = 0;
			cpuRegs.CP0.r[28] = pCache[index].tag[way];

			CACHE_LOG("CACHE DXLTG addr %x, index %d, way %d, DATA %x",addr,index,way,cpuRegs.CP0.r[28]);
			
			break;
		}
		case 0x13:
		{
			int index = (addr >> 6) & 0x3F;
			int way = addr & 0x1;
			//u8 * out = pCache[index].data[way][(addr>>4) & 0x3].b8._8;
			*(u32*)(&pCache[index].data[way][(addr>>4) & 0x3].b8._8[(addr&0xf)]) = cpuRegs.CP0.r[28];

			CACHE_LOG("CACHE DXSDT addr %x, index %d, way %d, DATA %x",addr,index,way,cpuRegs.CP0.r[28]);

			break;
		}
		case 0x12:
		{
			int index = (addr >> 6) & 0x3F;
			int way = addr & 0x1;
			pCache[index].tag[way] = cpuRegs.CP0.r[28];

			CACHE_LOG("CACHE DXSTG addr %x, index %d, way %d, DATA %x",addr,index,way,cpuRegs.CP0.r[28] & 0x6F);

			break;
		}
		case 0x14:
		{

			u8 * out;
			int index = (addr >> 6) & 0x3F;
			int way = addr & 0x1;


			CACHE_LOG("CACHE DXWBIN addr %x, index %d, way %d, Flags %x",addr,index,way,pCache[index].tag[way] & 0x78);

			if(pCache[index].tag[way] & 0x60)	// Dirty
			{
				u32 paddr = getMemW(pCache[index].tag[way]);
				char * t = (char *)(paddr);
				out = (u8*)(t + (addr & 0xFC0));
				((u64*)out)[0] = ((u64*)pCache[index].data[way][0].b8._8)[0];
				((u64*)out)[1] = ((u64*)pCache[index].data[way][0].b8._8)[1];
				((u64*)out)[2] = ((u64*)pCache[index].data[way][1].b8._8)[0];
				((u64*)out)[3] = ((u64*)pCache[index].data[way][1].b8._8)[1];
				((u64*)out)[4] = ((u64*)pCache[index].data[way][2].b8._8)[0];
				((u64*)out)[5] = ((u64*)pCache[index].data[way][2].b8._8)[1];
				((u64*)out)[6] = ((u64*)pCache[index].data[way][3].b8._8)[0];
				((u64*)out)[7] = ((u64*)pCache[index].data[way][3].b8._8)[1];
			}

			pCache[index].tag[way] &= ~(0x6F);
			((u64*)pCache[index].data[way][0].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][0].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][1].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][1].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][2].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][2].b8._8)[1] = 0;
			((u64*)pCache[index].data[way][3].b8._8)[0] = 0;
			((u64*)pCache[index].data[way][3].b8._8)[1] = 0;
			break;
		}
	}
}
}		// end namespace OpcodeImpl
#else

namespace OpcodeImpl
{

void CACHE() {
}
}

#endif

}}