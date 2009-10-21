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
#include "IopCommon.h"

u8 *psxM = NULL;
u8 *psxP = NULL;
u8 *psxH = NULL;	// standard hardware registers (0x000->0x3ff is the scratchpad)
u8 *psxS = NULL;	// 'undocumented' SIF communication registers

uptr *psxMemWLUT = NULL;
const uptr *psxMemRLUT = NULL;

static u8* m_psxAllMem = NULL;
static const uint m_psxMemSize =
	Ps2MemSize::IopRam +
	Ps2MemSize::IopHardware +
	0x00010000 +		// psxP
	0x00000100 ;		// psxS

void psxMemAlloc()
{
	if( m_psxAllMem == NULL )
		m_psxAllMem = vtlb_malloc( m_psxMemSize, 4096 );

	if( m_psxAllMem == NULL)
		throw Exception::OutOfMemory( "psxMemAlloc > failed allocating memory for the IOP processor." );

	u8* curpos = m_psxAllMem;
	psxM = curpos; curpos += Ps2MemSize::IopRam;
	psxP = curpos; curpos += 0x00010000; 
	psxH = curpos; curpos += Ps2MemSize::IopHardware;
	psxS = curpos; //curpos += 0x00010000;

	psxMemWLUT = (uptr*)_aligned_malloc(0x2000 * sizeof(uptr) * 2, 16);
	psxMemRLUT = psxMemWLUT + 0x2000; //(uptr*)_aligned_malloc(0x10000 * sizeof(uptr),16);
}

// Note!  Resetting the IOP's memory state is dependent on having *all* psx memory allocated,
// which is performed by MemInit and PsxMemInit()
void psxMemReset()
{
	jASSUME( psxMemWLUT != NULL );
	jASSUME( m_psxAllMem != NULL );

	DbgCon.Status( "psxMemReset > Resetting core memory!" );

	memzero_ptr<0x2000 * sizeof(uptr) * 2>( psxMemWLUT );	// clears both allocations, RLUT and WLUT
	memzero_ptr<m_psxMemSize>( m_psxAllMem );

	// Trick!  We're accessing RLUT here through WLUT, since it's the non-const pointer.
	// So the ones with a 0x2000 prefixed are RLUT tables.
	
	// Map IOP main memory, which is Read/Write, and mirrored three times
	// at 0x0, 0x8000, and 0xa000:
	for (int i=0; i<0x0080; i++)
	{
		psxMemWLUT[i + 0x0000] = (uptr)&psxM[(i & 0x1f) << 16];

		// RLUTs, accessed through WLUT.
		psxMemWLUT[i + 0x2000] = (uptr)&psxM[(i & 0x1f) << 16];
	}

	// A few single-page allocations for things we store in special locations.
	psxMemWLUT[0x2000 + 0x1f00] = (uptr)psxP;
	psxMemWLUT[0x2000 + 0x1f80] = (uptr)psxH;
	//psxMemWLUT[0x1bf80] = (uptr)psxH;

	psxMemWLUT[0x1f00] = (uptr)psxP;
	psxMemWLUT[0x1f80] = (uptr)psxH;
	//psxMemWLUT[0xbf80] = (uptr)psxH;

	// Read-only memory areas, so don't map WLUT for these...
	for (int i=0; i<0x0040; i++)
	{
		psxMemWLUT[i + 0x2000 + 0x1fc0] = (uptr)&PS2MEM_ROM[i << 16];
	}

	for (int i=0; i<0x0004; i++)
	{
		psxMemWLUT[i + 0x2000 + 0x1e00] = (uptr)&PS2MEM_ROM1[i << 16];
	}

	// sif!! (which is read only? (air))
	psxMemWLUT[0x2000 + 0x1d00] = (uptr)psxS;
	//psxMemWLUT[0x1bd00] = (uptr)psxS;

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

u8 __fastcall iopMemRead8(u32 mem)
{
	mem &= 0x1fffffff;
	u32 t = mem >> 16;

	if (t == 0x1f80)
	{
		switch( mem & 0xf000 )
		{
			case 0x1000: return IopMemory::iopHwRead8_Page1(mem);
			case 0x3000: return IopMemory::iopHwRead8_Page3(mem);
			case 0x8000: return IopMemory::iopHwRead8_Page8(mem);

			// code for regression testing -- selectively enable these to help narrow out
			// which register became buggy with the new Hw handlers.
			//case 0x1000: return psxHwRead8(mem);
			//case 0x3000: return psxHwRead8(mem);
			//case 0x8000: return psxHwRead8(mem);
			
			default:
				return psxHu8(mem);
		}
	}
	else if (t == 0x1f40)
	{
		return psxHw4Read8(mem);
	}
	else
	{
		const u8* p = (const u8*)(psxMemRLUT[mem >> 16]);
		if (p != NULL)
		{
			return *(const u8 *)(p + (mem & 0xffff));
		}
		else
		{
			if (t == 0x1000)
				return DEV9read8(mem);
			PSXMEM_LOG("err lb %8.8lx", mem);
			return 0;
		}
	}
}

u16 __fastcall iopMemRead16(u32 mem)
{
	mem &= 0x1fffffff;
	u32 t = mem >> 16;

	if (t == 0x1f80)
	{
		switch( mem & 0xf000 )
		{
			case 0x1000: return IopMemory::iopHwRead16_Page1(mem);
			case 0x3000: return IopMemory::iopHwRead16_Page3(mem);
			case 0x8000: return IopMemory::iopHwRead16_Page8(mem);

			// code for regression testing -- selectively enable these to help narrow out
			// which register became buggy with the new Hw handlers.
			//case 0x1000: return psxHwRead16(mem);
			//case 0x3000: return psxHwRead16(mem);
			//case 0x8000: return psxHwRead16(mem);
			
			default:
				return psxHu16(mem);
		}
	}
	else
	{
		const u8* p = (const u8*)(psxMemRLUT[mem >> 16]);
		if (p != NULL)
		{
			if (t == 0x1d00) 
			{
				u16 ret;
				switch(mem & 0xF0)
				{
				case 0x00:
					ret= psHu16(SBUS_F200);
					break;
				case 0x10:
					ret= psHu16(SBUS_F210);
					break;
				case 0x40:
					ret= psHu16(SBUS_F240) | 0x0002;
					break;
				case 0x60:
					ret = 0;
					break;
				default:
					ret = psxHu16(mem);
					break;
				}
				SIF_LOG("Sif reg read %x value %x", mem, ret);
				return ret;
			}
			return *(const u16 *)(p + (mem & 0xffff));
		}
		else
		{
			if (t == 0x1F90)
				return SPU2read(mem);
			if (t == 0x1000)
				return DEV9read16(mem);
			PSXMEM_LOG("err lh %8.8lx", mem);
			return 0;
		}
	}
}

u32 __fastcall iopMemRead32(u32 mem)
{
	mem &= 0x1fffffff;
	u32 t = mem >> 16;

	if (t == 0x1f80)
	{
		switch( mem & 0xf000 )
		{
			case 0x1000: return IopMemory::iopHwRead32_Page1(mem);
			case 0x3000: return IopMemory::iopHwRead32_Page3(mem);
			case 0x8000: return IopMemory::iopHwRead32_Page8(mem);

			// code for regression testing -- selectively enable these to help narrow out
			// which register became buggy with the new Hw handlers.
			//case 0x1000: return psxHwRead32(mem);
			//case 0x3000: return psxHwRead32(mem);
			//case 0x8000: return psxHwRead32(mem);
			
			default:
				return psxHu32(mem);
		}
	} else
	{
		//see also Hw.c
		const u8* p = (const u8*)(psxMemRLUT[mem >> 16]);
		if (p != NULL)
		{
			if (t == 0x1d00)
			{
				u32 ret;
				switch(mem & 0xF0)
				{
				case 0x00:
					ret= psHu32(SBUS_F200);
					break;
				case 0x10:
					ret= psHu32(SBUS_F210);
					break;
				case 0x20:
					ret= psHu32(SBUS_F220);
					break;
				case 0x30:	// EE Side
					ret= psHu32(SBUS_F230);
					break;
				case 0x40:
					ret= psHu32(SBUS_F240) | 0xF0000002;
					break;
				case 0x60:
					ret = 0;
					break;
				default:
					ret = psxHu32(mem);
					break;
				}
				SIF_LOG("Sif reg read %x value %x", mem, ret);
				return ret;
			}
			return *(const u32 *)(p + (mem & 0xffff));
		}
		else
		{		
			if (t == 0x1000)
				return DEV9read32(mem);
			return 0;
		}
	}
}

void __fastcall iopMemWrite8(u32 mem, u8 value)
{
	mem &= 0x1fffffff;
	u32 t = mem >> 16;
	
	if (t == 0x1f80)
	{
		switch( mem & 0xf000 )
		{
			// Regression testing: selectively pass ranges of registers to new or old
			// handlers.  Helps narrow out which area of registers is erroring out.
			/*case 0x1000:
				if( mem >= 0x1f801000 )
					psxHwWrite8( mem, value );
				else
					IopMemory::iopHwWrite8_Page1(mem,value);
			break;*/

			case 0x1000: IopMemory::iopHwWrite8_Page1(mem,value); break;
			case 0x3000: IopMemory::iopHwWrite8_Page3(mem,value); break;
			case 0x8000: IopMemory::iopHwWrite8_Page8(mem,value); break;

			// code for regression testing -- selectively enable these to help narrow out
			// which register became buggy with the new Hw handlers.
			//case 0x1000: psxHwWrite8(mem,value); break;
			//case 0x3000: psxHwWrite8(mem,value); break;
			//case 0x8000: psxHwWrite8(mem,value); break;
			
			default:
				psxHu8(mem) = value;
			break;
		}
	}
	else if (t == 0x1f40)
	{
		psxHw4Write8(mem, value);
	}
	else
	{
		u8* p = (u8 *)(psxMemWLUT[mem >> 16]);
		if (p != NULL && !(psxRegs.CP0.n.Status & 0x10000) )
		{
			*(u8  *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		}
		else
		{
			if (t == 0x1d00) 
			{
				Console.WriteLn("sw8 [0x%08X]=0x%08X", mem, value);
				psxSu8(mem) = value; 
				return;
			}
			if (t == 0x1000) 
			{
				DEV9write8(mem, value); return;
			}
			PSXMEM_LOG("err sb %8.8lx = %x", mem, value);
		}
	}
}

void __fastcall iopMemWrite16(u32 mem, u16 value)
{
	mem &= 0x1fffffff;
	u32 t = mem >> 16;

	if (t == 0x1f80)
	{
		switch( mem & 0xf000 )
		{
			// Regression testing: selectively pass ranges of registers to new or old
			// handlers.  Helps narrow out which area of registers is erroring out.
			/*case 0x1000:
				if( mem >= 0x1f801000 )
					psxHwWrite16( mem, value );
				else
					IopMemory::iopHwWrite16_Page1(mem,value);
			break;*/

			case 0x1000: IopMemory::iopHwWrite16_Page1(mem,value); break;
			case 0x3000: IopMemory::iopHwWrite16_Page3(mem,value); break;
			case 0x8000: IopMemory::iopHwWrite16_Page8(mem,value); break;

			//case 0x1000: psxHwWrite16(mem,value); break;
			//case 0x3000: psxHwWrite16(mem,value); break;
			//case 0x8000: psxHwWrite16(mem,value); break;
			
			default:
				psxHu16(mem) = value;
			break;
		}
	} else
	{
		u8* p = (u8 *)(psxMemWLUT[mem >> 16]);
		if (p != NULL && !(psxRegs.CP0.n.Status & 0x10000) )
		{
			if( t==0x1D00 ) Console.WriteLn("sw16 [0x%08X]=0x%08X", mem, value);
			*(u16 *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		}
		else
		{
			if (t == 0x1d00)
			{
				switch (mem & 0xf0)
				{
					case 0x10:
						// write to ps2 mem
						psHu16(SBUS_F210) = value;
						return;
					case 0x40:
					{
						u32 temp = value & 0xF0;
						// write to ps2 mem
						if(value & 0x20 || value & 0x80)
						{
							psHu16(SBUS_F240) &= ~0xF000;
							psHu16(SBUS_F240) |= 0x2000;
						}

						
						if(psHu16(SBUS_F240) & temp) 
							psHu16(SBUS_F240) &= ~temp;
						else 
							psHu16(SBUS_F240) |= temp;
						return;
					}
					case 0x60:
						psHu32(SBUS_F260) = 0;
						return;

				}
				psxSu16(mem) = value; return;
			}
			if (t == 0x1F90) {
				SPU2write(mem, value); return;
			}
			if (t == 0x1000) {
				DEV9write16(mem, value); return;
			}
			PSXMEM_LOG("err sh %8.8lx = %x", mem, value);
		}
	}
}

void __fastcall iopMemWrite32(u32 mem, u32 value)
{
	mem &= 0x1fffffff;
	u32 t = mem >> 16;
	
	if (t == 0x1f80)
	{
		switch( mem & 0xf000 )
		{
			// Regression testing: selectively pass ranges of registers to new or old
			// handlers.  Helps narrow out which area of registers is erroring out.
			/*case 0x1000:
				if( mem >= 0x1f801528 )
					psxHwWrite32( mem, value );
				else
					IopMemory::iopHwWrite32_Page1(mem,value);
			break;*/

			case 0x1000: IopMemory::iopHwWrite32_Page1(mem,value); break;
			case 0x3000: IopMemory::iopHwWrite32_Page3(mem,value); break;
			case 0x8000: IopMemory::iopHwWrite32_Page8(mem,value); break;

			//case 0x1000: psxHwWrite32(mem,value); break;
			//case 0x3000: psxHwWrite32(mem,value); break;
			//case 0x8000: psxHwWrite32(mem,value); break;
			
			default:
				psxHu32(mem) = value;
			break;
		}
	} else
	{
		//see also Hw.c
		u8* p = (u8 *)(psxMemWLUT[mem >> 16]);
		if( p != NULL && !(psxRegs.CP0.n.Status & 0x10000) )
		{
			*(u32 *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		}
		else 
		{
			if (t == 0x1d00)
			{
				MEM_LOG("iop Sif reg write %x value %x", mem, value);
				switch (mem & 0xf0)
				{
					case 0x00:		// EE write path (EE/IOP readable)
						return;		// this is the IOP, so read-only (do nothing)

					case 0x10:		// IOP write path (EE/IOP readable)
						psHu32(SBUS_F210) = value;
						return;

					case 0x20:		// Bits cleared when written from IOP.
						psHu32(SBUS_F220) &= ~value;
						return;

					case 0x30:		// bits set when written from IOP
						psHu32(SBUS_F230) |= value;
						return;

					case 0x40:		// Control Register
					{
						u32 temp = value & 0xF0;
						if (value & 0x20 || value & 0x80)
						{
							psHu32(SBUS_F240) &= ~0xF000;
							psHu32(SBUS_F240) |= 0x2000;
						}

						
						if (psHu32(SBUS_F240) & temp)
							psHu32(SBUS_F240) &= ~temp;
						else
							psHu32(SBUS_F240) |= temp;
						return;
					}

					case 0x60:
						psHu32(SBUS_F260) = 0;
					return;
				}
				psxSu32(mem) = value; 

				// wtf?  why were we writing to the EE's sif space?  Commenting this out doesn't
				// break any of my games, and should be more correct, but I guess we'll see.  --air
				//*(u32*)(PS2MEM_HW+0xf200+(mem&0xf0)) = value;
				return; 
			}
			else if (t == 0x1000)
			{
				DEV9write32(mem, value); return;
			}
		}
	}
}
