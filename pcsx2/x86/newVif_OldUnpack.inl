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

// Old Vif Unpack Code
// Only here for testing/reference
// If newVif is defined and newVif1 isn't, vif1 will use this code
// same goes for vif0...
template void VIFunpack<0>(u32 *data, vifCode *v, u32 size);
template void VIFunpack<1>(u32 *data, vifCode *v, u32 size);
template<const u32 VIFdmanum> void VIFunpack(u32 *data, vifCode *v, u32 size) {
	//if (!VIFdmanum) DevCon.WriteLn("vif#%d, size = %d [%x]", VIFdmanum, size, data);
	UNPACKFUNCTYPE func;
	const VIFUnpackFuncTable *ft;
	VURegs * VU;
	u8 *cdata = (u8*)data;
	u32 tempsize = 0;
	const u32 memlimit = vif_size(VIFdmanum);

	if (VIFdmanum == 0) {
		VU = &VU0;
		vifRegs = vif0Regs;
		vifMaskRegs = g_vif0Masks;
		vif = &vif0;
		vifRow = g_vifmask.Row0;
	}
	else {
		VU = &VU1;
		vifRegs = vif1Regs;
		vifMaskRegs = g_vif1Masks;
		vif = &vif1;
		vifRow = g_vifmask.Row1;
	}

	u32 *dest      = (u32*)(VU->Mem + v->addr);
	u32 unpackType = v->cmd & 0xf;

	ft     = &VIFfuncTable[ unpackType ];
	func   = vif->usn ? ft->funcU : ft->funcS;
	size <<= 2;

	if (vifRegs->cycle.cl >= vifRegs->cycle.wl) { // skipping write
		if (v->addr >= memlimit) {
			DevCon.Warning("Overflown at the start");
			v->addr &= (memlimit - 1);
			dest = (u32*)(VU->Mem + v->addr);
		}

		size = min(size, (int)vifRegs->num * ft->gsize); //size will always be the same or smaller

		tempsize = v->addr + ((((vifRegs->num-1) / vifRegs->cycle.wl) *
			 (vifRegs->cycle.cl - vifRegs->cycle.wl)) * 16) + (vifRegs->num * 16);

		//Sanity Check (memory overflow)
		if (tempsize > memlimit) {
			if (((vifRegs->cycle.cl != vifRegs->cycle.wl) &&
			   ((memlimit + (vifRegs->cycle.cl - vifRegs->cycle.wl) * 16) == tempsize))) {
				//It's a red herring, so ignore it! SSE unpacks will be much quicker.
				DevCon.WriteLn("what!!!!!!!!!");
				//tempsize = 0;
				tempsize = size;
				size = 0;
			}
			else {
				DevCon.Warning("VIF%x Unpack ending %x > %x", VIFdmanum, tempsize, VIFdmanum ? 0x4000 : 0x1000);
				tempsize = size;
				size = 0;
			}
		}
		else {
			tempsize = size;
			size = 0;
		}
		if (tempsize) {
			int incdest = ((vifRegs->cycle.cl - vifRegs->cycle.wl) << 2) + 4;
			size = 0;
			int addrstart = v->addr;
			//if((tempsize >> 2) != v->size) DevCon.Warning("split when size != tagsize");

			VIFUNPACK_LOG("sorting tempsize :p, size %d, vifnum %d, addr %x", tempsize, vifRegs->num, v->addr);

			while ((tempsize >= ft->gsize) && (vifRegs->num > 0)) {
				if(v->addr >= memlimit) {
					DevCon.Warning("Mem limit overflow");
					v->addr &= (memlimit - 1);
					dest = (u32*)(VU->Mem + v->addr);
				}

				func(dest, (u32*)cdata, ft->qsize);
				cdata    += ft->gsize;
				tempsize -= ft->gsize;

				vifRegs->num--;
				vif->cl++;
				
				if (vif->cl == vifRegs->cycle.wl) {
					dest    += incdest;
					v->addr +=(incdest * 4);
					vif->cl = 0;
				}
				else {
					dest    += 4;
					v->addr += 16;
				}
			}
			if (v->addr >= memlimit) {
                v->addr &=(memlimit - 1);
                dest = (u32*)(VU->Mem + v->addr);
            }
			v->addr = addrstart;
			if(tempsize > 0) size = tempsize;
		}
		
		if (size >= ft->dsize && vifRegs->num > 0) { //Else write what we do have
			DevCon.Warning("huh!!!!!!!!!!!!!!!!!!!!!!");
			VIF_LOG("warning, end with size = %d", size);
			// unpack one qword
			//v->addr += (size / ft->dsize) * 4;
			func(dest, (u32*)cdata, size / ft->dsize);
			size = 0;
			VIFUNPACK_LOG("leftover done, size %d, vifnum %d, addr %x", size, vifRegs->num, v->addr);
		}
	}
	else { // filling write
		if(vifRegs->cycle.cl > 0) // Quicker and avoids zero division :P
			if((u32)(((size / ft->gsize) / vifRegs->cycle.cl) * vifRegs->cycle.wl) < vifRegs->num)
			DevCon.Warning("Filling write warning! %x < %x and CL = %x WL = %x", (size / ft->gsize), vifRegs->num, vifRegs->cycle.cl, vifRegs->cycle.wl);

		DevCon.Warning("filling write %d cl %d, wl %d mask %x mode %x unpacktype %x addr %x", vifRegs->num, vifRegs->cycle.cl, vifRegs->cycle.wl, vifRegs->mask, vifRegs->mode, unpackType, vif->tag.addr);
		while (vifRegs->num > 0) {
			if (vif->cl == vifRegs->cycle.wl) {
				vif->cl = 0;
			}
			// unpack one qword
			if (vif->cl < vifRegs->cycle.cl) { 
				if(size < ft->gsize) { DevCon.WriteLn("Out of Filling write data!"); break; }
				func(dest, (u32*)cdata, ft->qsize);
				cdata += ft->gsize;
				size  -= ft->gsize;
				vif->cl++;
				vifRegs->num--;
				if (vif->cl == vifRegs->cycle.wl) {
					vif->cl = 0;
				}
			}
			else {
				func(dest, (u32*)cdata, ft->qsize);
				v->addr += 16;
				vifRegs->num--;
				vif->cl++;
			}
			dest += 4;
			if (vifRegs->num == 0) break;
		}
	}
}
