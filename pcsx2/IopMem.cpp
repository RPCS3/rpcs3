/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include "PrecompiledHeader.h"

#include "PsxCommon.h"
#include "VU.h"
#include "iCore.h"
#include "Hw.h"
#include "iR3000A.h"

int g_psxWriteOk=1;
static u32 writectrl;

#ifdef PCSX2_VIRTUAL_MEM
void psxMemAlloc()
{
	// In VirtualMemory land all mem taken care by memAlloc
}

void psxMemReset()
{
	memzero_ptr<Ps2MemSize::IopRam>(psxM);
}

void psxMemShutdown()
{
}

u8 iopMemRead8(u32 mem)
{
	u32 t = (mem >> 16) & 0x1fff;
	
	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				return psxHu8(mem);
			else
				return psxHwRead8(mem);
			break;

#ifdef _DEBUG
		case 0x1d00: assert(0);
#endif

		case 0x1f40:
			mem &= 0x1fffffff;
			return psxHw4Read8(mem);

		case 0x1000: return DEV9read8(mem & 0x1FFFFFFF);

		default:
			assert( g_psxWriteOk );
			return *(u8*)PSXM(mem);
	}
}

u16 iopMemRead16(u32 mem)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				return psxHu16(mem);
			else
				return psxHwRead16(mem);
			break;

		case 0x1d00:
			SIF_LOG("Sif reg read %x value %x\n", mem, psxHu16(mem));
			switch(mem & 0xF0)
			{
				case 0x40: return psHu16(0x1000F240) | 0x0002;
				case 0x60: return 0;
				default: return *(u16*)(PS2MEM_HW+0xf200+(mem&0xf0));
			}
			break;

		case 0x1f90:
			return SPU2read(mem & 0x1FFFFFFF);
		case 0x1000:
			return DEV9read16(mem & 0x1FFFFFFF);

		default:
			assert( g_psxWriteOk );
			return *(u16*)PSXM(mem);
	}
}

u32 iopMemRead32(u32 mem)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				return psxHu32(mem);
			else
				return psxHwRead32(mem);
			break;

		case 0x1d00:
			SIF_LOG("Sif reg read %x value %x\n", mem, psxHu32(mem));
			switch(mem & 0xF0)
			{
				case 0x40: return psHu32(0x1000F240) | 0xF0000002;
				case 0x60: return 0;
				default: return *(u32*)(PS2MEM_HW+0xf200+(mem&0xf0));
			}
			break;

		case 0x1fff: return g_psxWriteOk;
		case 0x1000:
			return DEV9read32(mem & 0x1FFFFFFF);

		default:
			//assert(g_psxWriteOk);
			if( mem == 0xfffe0130 )
				return writectrl;
			else if( mem == 0xffffffff )
				return writectrl;
			else if( g_psxWriteOk )
				return *(u32*)PSXM(mem);
			else return 0;
	}
}

void iopMemWrite8(u32 mem, u8 value)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				psxHu8(mem) = value;
			else
				psxHwWrite8(mem, value);
			break;

		case 0x1f40:
			mem&= 0x1fffffff;
			psxHw4Write8(mem, value);
			break;

		case 0x1d00:
			SysPrintf("sw8 [0x%08X]=0x%08X\n", mem, value);
			*(u8*)(PS2MEM_HW+0xf200+(mem&0xff)) = value;
			break;

		case 0x1000:
			DEV9write8(mem & 0x1fffffff, value);
			return;

		default:
			assert(g_psxWriteOk);
			*(u8  *)PSXM(mem) = value;
			psxCpu->Clear(mem&~3, 1);
			break;
	}
}

void iopMemWrite16(u32 mem, u16 value)
{
	u32 t = (mem >> 16) & 0x1fff;
	switch(t) {
		case 0x1600:
			//HACK: DEV9 VM crash fix
			break;
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				psxHu16(mem) = value;
			else
				psxHwWrite16(mem, value);
			break;

		case 0x1d00:
			switch (mem & 0xf0) {
				case 0x10:
					// write to ps2 mem
					psHu16(0x1000F210) = value;
					return;
				case 0x40:
				{
					u32 temp = value & 0xF0;
					// write to ps2 mem
					if(value & 0x20 || value & 0x80)
					{
						psHu16(0x1000F240) &= ~0xF000;
						psHu16(0x1000F240) |= 0x2000;
					}
						
					if(psHu16(0x1000F240) & temp) psHu16(0x1000F240) &= ~temp;
					else psHu16(0x1000F240) |= temp;
					return;
				}
				case 0x60:
					psHu32(0x1000F260) = 0;
					return;
				default:
					assert(0);
			}
			return;

		case 0x1f90:
			SPU2write(mem & 0x1FFFFFFF, value); return;
			
		case 0x1000:
			DEV9write16(mem & 0x1fffffff, value); return;
		default:
			assert( g_psxWriteOk );
			*(u16 *)PSXM(mem) = value;
			psxCpu->Clear(mem&~3, 1);
			break;
	}
}

void iopMemWrite32(u32 mem, u32 value)
{
	u32 t = (mem >> 16) & 0x1fff;
	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				psxHu32(mem) = value;
			else
				psxHwWrite32(mem, value);
			break;

		case 0x1d00:
			switch (mem & 0xf0) {
				case 0x10:
					// write to ps2 mem
					psHu32(0x1000F210) = value;
					return;
				case 0x20:
					// write to ps2 mem
					psHu32(0x1000F220) &= ~value;
					return;
				case 0x30:
					// write to ps2 mem
					psHu32(0x1000F230) |= value;
					return;
				case 0x40:
				{
					u32 temp = value & 0xF0;
					// write to ps2 mem
					if(value & 0x20 || value & 0x80)
					{
						psHu32(0x1000F240) &= ~0xF000;
						psHu32(0x1000F240) |= 0x2000;
					}

					
					if(psHu32(0x1000F240) & temp) psHu32(0x1000F240) &= ~temp;
					else psHu32(0x1000F240) |= temp;
					return;
				}
				case 0x60:
					psHu32(0x1000F260) = 0;
					return;

				default:
					*(u32*)(PS2MEM_HW+0xf200+(mem&0xf0)) = value;
			}
				
			return;

		case 0x1000:
			DEV9write32(mem & 0x1fffffff, value);
			return;

		case 0x1ffe:
			if( mem == 0xfffe0130 ) {
				writectrl = value;
				switch (value) {
					case 0x800: case 0x804:
					case 0xc00: case 0xc04:
					case 0xcc0: case 0xcc4:
					case 0x0c4:
						g_psxWriteOk = 0;
						//PSXMEM_LOG("writectrl: writenot ok\n");
						break;
					case 0x1e988:
					case 0x1edd8:
						g_psxWriteOk = 1;
						//PSXMEM_LOG("writectrl: write ok\n");
						break;
					default:
						PSXMEM_LOG("unk %8.8lx = %x\n", mem, value);
						break;
				}
			}
			break;

		default:
			
			if( g_psxWriteOk ) {
				*(u32 *)PSXM(mem) = value;
				psxCpu->Clear(mem&~3, 1);
			}

			break;
	}
}

#else

u8 *psxM = NULL;
u8 *psxP = NULL;
u8 *psxH = NULL;
u8 *psxS = NULL;

uptr *psxMemWLUT = NULL;
const uptr *psxMemRLUT = NULL;

static u8* m_psxAllMem = NULL;
static const uint m_psxMemSize =
	Ps2MemSize::IopRam +
	Ps2MemSize::IopHardware +
	0x00010000 +		// psxP
	0x00010000 ;		// psxS

void psxMemAlloc()
{
	if( m_psxAllMem == NULL )
		m_psxAllMem = vtlb_malloc( m_psxMemSize, 4096, 0x21000000 );

	if( m_psxAllMem == NULL)
		throw Exception::OutOfMemory( "psxMemAlloc > failed allocating memory for the IOP processor." );

	u8* curpos = m_psxAllMem;
	psxM = curpos; curpos += Ps2MemSize::IopRam;
	psxP = curpos; curpos += 0x00010000; 
	psxH = curpos; curpos += Ps2MemSize::IopHardware;
	psxS = curpos; //curpos += 0x00010000;

	psxMemWLUT = (uptr*)_aligned_malloc(0x10000 * sizeof(uptr) * 2, 16);
	psxMemRLUT = psxMemWLUT + 0x10000; //(uptr*)_aligned_malloc(0x10000 * sizeof(uptr),16);
}

// Note!  Resetting the IOP's memory state is dependent on having *all* psx memory allocated,
// which is performed by MemInit and PsxMemInit()
void psxMemReset()
{
	jASSUME( psxMemWLUT != NULL );
	jASSUME( m_psxAllMem != NULL );

	DbgCon::Status( "psxMemReset > Resetting core memory!" );

	memzero_ptr<0x10000 * sizeof(uptr) * 2>( psxMemWLUT );	// clears both allocations, RLUT and WLUT
	memzero_ptr<m_psxMemSize>( m_psxAllMem );

	// Trick!  We're accessing RLUT here through WLUT, since it's the non-const pointer.
	// So the ones with a 1 prefixed (ala 0x18000, etc) are RLUT tables.
	for (int i=0; i<0x0080; i++)
	{
		psxMemWLUT[i + 0x0000] = (uptr)&psxM[(i & 0x1f) << 16];
		psxMemWLUT[i + 0x8000] = (uptr)&psxM[(i & 0x1f) << 16];
		psxMemWLUT[i + 0xa000] = (uptr)&psxM[(i & 0x1f) << 16];

		// RLUTs, accessed through WLUT.
		psxMemWLUT[i + 0x10000] = (uptr)&psxM[(i & 0x1f) << 16];
		psxMemWLUT[i + 0x18000] = (uptr)&psxM[(i & 0x1f) << 16];
		psxMemWLUT[i + 0x1a000] = (uptr)&psxM[(i & 0x1f) << 16];
	}

	// A few single-page allocations...
	psxMemWLUT[0x11f00] = (uptr)psxP;
	psxMemWLUT[0x11f80] = (uptr)psxH;
	psxMemWLUT[0x1bf80] = (uptr)psxH;

	psxMemWLUT[0x1f00] = (uptr)psxP;
	psxMemWLUT[0x1f80] = (uptr)psxH;
	psxMemWLUT[0xbf80] = (uptr)psxH;

	// Read-only memory areas, so don't map WLUT for these...
	for (int i=0; i<0x0040; i++)
	{
		psxMemWLUT[i + 0x11fc0] = (uptr)&PS2MEM_ROM[i << 16];
		psxMemWLUT[i + 0x19fc0] = (uptr)&PS2MEM_ROM[i << 16];
		psxMemWLUT[i + 0x1bfc0] = (uptr)&PS2MEM_ROM[i << 16];
	}

	for (int i=0; i<0x0004; i++)
	{
		psxMemWLUT[i + 0x11e00] = (uptr)&PS2MEM_ROM1[i << 16];
		psxMemWLUT[i + 0x19e00] = (uptr)&PS2MEM_ROM1[i << 16];
		psxMemWLUT[i + 0x1be00] = (uptr)&PS2MEM_ROM1[i << 16];
	}

	// Scratchpad! (which is read only? (air))
	psxMemWLUT[0x11d00] = (uptr)psxS;
	psxMemWLUT[0x1bd00] = (uptr)psxS;

	// why isn't scratchpad read/write? (air)
	//for (i=0; i<0x0001; i++) psxMemWLUT[i + 0x1d00] = (uptr)&psxS[i << 16];
	//for (i=0; i<0x0001; i++) psxMemWLUT[i + 0xbd00] = (uptr)&psxS[i << 16];

	// this one looks like an old hack for some special write-only memory area,
	// but leaving it in for reference (air)
	//for (i=0; i<0x0008; i++) psxMemWLUT[i + 0xbfc0] = (uptr)&psR[i << 16];
}

void psxMemShutdown()
{
	vtlb_free( m_psxAllMem, m_psxMemSize );
	m_psxAllMem = NULL;

	psxM = psxP = psxH = psxS = NULL;

	safe_aligned_free(psxMemWLUT);
	psxMemRLUT = NULL;
}

u8 iopMemRead8(u32 mem) {
	const u8* p;
	u32 t;

	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			return psxHu8(mem);
		else
			return psxHwRead8(mem);
	} else
	if (t == 0x1f40) {
		mem&= 0x1fffffff;
		return psxHw4Read8(mem);
	} else {
		p = (const u8*)(psxMemRLUT[mem >> 16]);
		if (p != NULL) {
			return *(const u8 *)(p + (mem & 0xffff));
		} else {
			if (t == 0x1000) return DEV9read8(mem & 0x1FFFFFFF);
			PSXMEM_LOG("err lb %8.8lx\n", mem);
			return 0;
		}
	}
}

u16 iopMemRead16(u32 mem) {
	const u8* p;
	u32 t;

	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			return psxHu16(mem);
		else
			return psxHwRead16(mem);
	} else {
		p = (const u8*)(psxMemRLUT[mem >> 16]);
		if (p != NULL) {
			if (t == 0x1d00) {
				u16 ret;
				switch(mem & 0xF0)
				{
				case 0x00:
					ret= psHu16(0x1000F200);
					break;
				case 0x10:
					ret= psHu16(0x1000F210);
					break;
				case 0x40:
					ret= psHu16(0x1000F240) | 0x0002;
					break;
				case 0x60:
					ret = 0;
					break;
				default:
					ret = psxHu16(mem);
					break;
				}
				SIF_LOG("Sif reg read %x value %x\n", mem, ret);
				return ret;
			}
			return *(const u16 *)(p + (mem & 0xffff));
		} else {
			if (t == 0x1F90)
				return SPU2read(mem & 0x1FFFFFFF);
			if (t == 0x1000) return DEV9read16(mem & 0x1FFFFFFF);
			PSXMEM_LOG("err lh %8.8lx\n", mem);
			return 0;
		}
	}
}

u32 iopMemRead32(u32 mem) {
	const u8* p;
	u32 t;
	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			return psxHu32(mem);
		else
			return psxHwRead32(mem);
	} else {
		//see also Hw.c
		p = (const u8*)(psxMemRLUT[mem >> 16]);
		if (p != NULL) {
			if (t == 0x1d00) {
				u32 ret;
				switch(mem & 0xF0)
				{
				case 0x00:
					ret= psHu32(0x1000F200);
					break;
				case 0x10:
					ret= psHu32(0x1000F210);
					break;
				case 0x20:
					ret= psHu32(0x1000F220);
					break;
				case 0x30:	// EE Side
					ret= psHu32(0x1000F230);
					break;
				case 0x40:
					ret= psHu32(0x1000F240) | 0xF0000002;
					break;
				case 0x60:
					ret = 0;
					break;
				default:
					ret = psxHu32(mem);
					break;
				}
				SIF_LOG("Sif reg read %x value %x\n", mem, ret);
				return ret;
			}
			return *(const u32 *)(p + (mem & 0xffff));
		} else {
			if (t == 0x1000) return DEV9read32(mem & 0x1FFFFFFF);
			
			if (mem != 0xfffe0130) {
				if (g_psxWriteOk) PSXMEM_LOG("err lw %8.8lx\n", mem);
			} else {
				return writectrl;
			}
			return 0;
		}
	}
}

void iopMemWrite8(u32 mem, u8 value) {
	char *p;
	u32 t;

	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			psxHu8(mem) = value;
		else
			psxHwWrite8(mem, value);
	} else
	if (t == 0x1f40) {
		mem&= 0x1fffffff;
		psxHw4Write8(mem, value);
	} else {
		p = (char *)(psxMemWLUT[mem >> 16]);
		if (p != NULL) {
			*(u8  *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		} else {
			if ((t & 0x1FFF)==0x1D00) SysPrintf("sw8 [0x%08X]=0x%08X\n", mem, value);
			if (t == 0x1d00) {
				psxSu8(mem) = value; return;
			}
			if (t == 0x1000) {
				DEV9write8(mem & 0x1fffffff, value); return;
			}
			PSXMEM_LOG("err sb %8.8lx = %x\n", mem, value);
		}
	}
}

void iopMemWrite16(u32 mem, u16 value) {
	char *p;
	u32 t;

	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			psxHu16(mem) = value;
		else
			psxHwWrite16(mem, value);
	} else {
		p = (char *)(psxMemWLUT[mem >> 16]);
		if (p != NULL) {
			if ((t & 0x1FFF)==0x1D00) SysPrintf("sw16 [0x%08X]=0x%08X\n", mem, value);
			*(u16 *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		} else {
			if (t == 0x1d00) {
					switch (mem & 0xf0) {
						case 0x10:
							// write to ps2 mem
							psHu16(0x1000F210) = value;
							return;
						case 0x40:
						{
							u32 temp = value & 0xF0;
							// write to ps2 mem
							if(value & 0x20 || value & 0x80)
							{
								psHu16(0x1000F240) &= ~0xF000;
								psHu16(0x1000F240) |= 0x2000;
							}

							
							if(psHu16(0x1000F240) & temp) psHu16(0x1000F240) &= ~temp;
							else psHu16(0x1000F240) |= temp;
							return;
						}
						case 0x60:
							psHu32(0x1000F260) = 0;
							return;

					}
				psxSu16(mem) = value; return;
			}
			if (t == 0x1F90) {
				SPU2write(mem & 0x1FFFFFFF, value); return;
			}
			if (t == 0x1000) {
				DEV9write16(mem & 0x1fffffff, value); return;
			}
			PSXMEM_LOG("err sh %8.8lx = %x\n", mem, value);
		}
	}
}

void iopMemWrite32(u32 mem, u32 value) {
	char *p;
	u32 t;
	
	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			psxHu32(mem) = value;
		else
			psxHwWrite32(mem, value);
	} else {
		//see also Hw.c
		p = (char *)(psxMemWLUT[mem >> 16]);
		if (p != NULL) {
			*(u32 *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		} else {
			if (mem != 0xfffe0130) {
				if (t == 0x1d00) {
				MEM_LOG("iop Sif reg write %x value %x\n", mem, value);
					switch (mem & 0xf0) {
						case 0x10:
							// write to ps2 mem
							psHu32(0x1000F210) = value;
							return;
						case 0x20:
							// write to ps2 mem
							psHu32(0x1000F220) &= ~value;
							return;
						case 0x30:
							// write to ps2 mem
							psHu32(0x1000F230) |= value;
							return;
						case 0x40:
						{
							u32 temp = value & 0xF0;
							// write to ps2 mem
							if(value & 0x20 || value & 0x80)
							{
								psHu32(0x1000F240) &= ~0xF000;
								psHu32(0x1000F240) |= 0x2000;
							}

							
							if(psHu32(0x1000F240) & temp) psHu32(0x1000F240) &= ~temp;
							else psHu32(0x1000F240) |= temp;
							return;
						}
						case 0x60:
							psHu32(0x1000F260) = 0;
							return;

					}
					psxSu32(mem) = value; 

					// write to ps2 mem
					if( (mem & 0xf0) != 0x60 )
						*(u32*)(PS2MEM_HW+0xf200+(mem&0xf0)) = value;
					return;   
				}
				if (t == 0x1000) {
					DEV9write32(mem & 0x1fffffff, value); return;
				}

				//if (!g_psxWriteOk) psxCpu->Clear(mem&~3, 1);
				if (g_psxWriteOk) { PSXMEM_LOG("err sw %8.8lx = %x\n", mem, value); }
			} else {
				writectrl = value;
				switch (value) {
					case 0x800: case 0x804:
					case 0xc00: case 0xc04:
					case 0xcc0: case 0xcc4:
					case 0x0c4:
						if (g_psxWriteOk == 0) break;
						g_psxWriteOk = 0;

						// Performance note: Use a for loop instead of memset/memzero
						// This generates *much* more efficient code in this particular case (due to few iterations)
						for (int i=0; i<0x0080; i++)
						{
							psxMemWLUT[i + 0x0000] = 0;
							psxMemWLUT[i + 0x8000] = 0;
							psxMemWLUT[i + 0xa000] = 0;
						}
						//PSXMEM_LOG("writectrl: writenot ok\n");
						break;
					case 0x1e988:
					case 0x1edd8:
						if (g_psxWriteOk == 1) break;
						g_psxWriteOk = 1;
						for (int i=0; i<0x0080; i++)
						{
							psxMemWLUT[i + 0x0000] = (uptr)&psxM[(i & 0x1f) << 16];
							psxMemWLUT[i + 0x8000] = (uptr)&psxM[(i & 0x1f) << 16];
							psxMemWLUT[i + 0xa000] = (uptr)&psxM[(i & 0x1f) << 16];
						}
						//PSXMEM_LOG("writectrl: write ok\n");
						break;
					default:
						PSXMEM_LOG("unk %8.8lx = %x\n", mem, value);
						break;
				}
			}
		}
	}
}

#endif
