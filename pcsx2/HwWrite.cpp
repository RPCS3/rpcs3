/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
#include "Hardware.h"
#include "Gif_Unit.h"

#include "ps2/HwInternal.h"
#include "ps2/eeHwTraceLog.inl"

using namespace R5900;

// Shift the middle 8 bits (bits 4-12) into the lower 8 bits.
// This helps the compiler optimize the switch statement into a lookup table. :)

#define HELPSWITCH(m) (((m)>>4) & 0xff)
#define mcase(src) case HELPSWITCH(src)

template< uint page > void __fastcall _hwWrite8(u32 mem, u8 value);
template< uint page > void __fastcall _hwWrite16(u32 mem, u8 value);
template< uint page > void __fastcall _hwWrite128(u32 mem, u8 value);


template<uint page>
void __fastcall _hwWrite32( u32 mem, u32 value )
{
	pxAssume( (mem & 0x03) == 0 );

	// Notes:
	// All unknown registers on the EE are "reserved" as discarded writes and indeterminate
	// reads.  Bus error is only generated for registers outside the first 16k of mapped
	// register space (which is handled by the VTLB mapping, so no need for checks here).

	switch (page)
	{
		case 0x00:	if (!rcntWrite32<0x00>(mem, value)) return;	break;
		case 0x01:	if (!rcntWrite32<0x01>(mem, value)) return;	break;

		case 0x02:
			if (!ipuWrite32(mem, value)) return;
		break;

		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		{
			// [Ps2Confirm] Direct FIFO read/write behavior.  We need to create a test that writes
			// data to one of the FIFOs and determine the result.  I'm not quite sure offhand a good
			// way to do that --air
			// Current assumption is that 32-bit and 64-bit writes likely do 128-bit zero-filled
			// writes (upper 96 bits are 0, lower 32 bits are effective).

			u128 zerofill = u128::From32(0);
			zerofill._u32[(mem >> 2) & 0x03] = value;

			DevCon.WriteLn( Color_Cyan, "Writing 32-bit FIFO data (zero-extended to 128 bits)" );
			_hwWrite128<page>(mem & ~0x0f, &zerofill);
		}
		return;

		case 0x03:
			if (mem >= EEMemoryMap::VIF0_Start)
			{
				if(mem >= EEMemoryMap::VIF1_Start)
				{
					if (!vifWrite32<1>(mem, value)) return;
				}
				else
				{
					if (!vifWrite32<0>(mem, value)) return;
				}
			}
			else iswitch(mem)
			{
				icase(GIF_CTRL)
				{
					// Not exactly sure what RST needs to do
					gifRegs.ctrl.write(value & 9);
					if (gifRegs.ctrl.RST) {
						GUNIT_LOG("GIF CTRL - Reset");
						gifUnit.Reset(true); // Should it reset gsSIGNAL?
						//gifUnit.ResetRegs();
					}
					gifRegs.stat.PSE = gifRegs.ctrl.PSE;
					return;
				}

				icase(GIF_MODE)
				{
					// need to set GIF_MODE (hamster ball)
					gifRegs.mode.write(value);
					gifRegs.stat.M3R = gifRegs.mode.M3R;
					gifRegs.stat.IMT = gifRegs.mode.IMT;
					return;
				}
			}
		break;

		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
			if (!dmacWrite32<page>(mem, value)) return;
		break;
		
		case 0x0f:
		{
			switch( HELPSWITCH(mem) )
			{
				mcase(INTC_STAT):
					psHu32(INTC_STAT) &= ~value;
					//cpuTestINTCInts();
				return;

				mcase(INTC_MASK):
					psHu32(INTC_MASK) ^= (u16)value;
					cpuTestINTCInts();
				return;

				mcase(SIO_TXFIFO):
				{
					u8* woot = (u8*)&value;
					// [Ps2Confirm] What happens when we write 32 bit values to SIO_TXFIFO?
					// If it works like the IOP, then all 32 bits are written to the FIFO in
					// order.  PCSX2 up to this point simply ignored non-8bit writes to this port.
					_hwWrite8<0x0f>(SIO_TXFIFO, woot[0]);
					_hwWrite8<0x0f>(SIO_TXFIFO, woot[1]);
					_hwWrite8<0x0f>(SIO_TXFIFO, woot[2]);
					_hwWrite8<0x0f>(SIO_TXFIFO, woot[3]);
				}
				return;

				mcase(SBUS_F200):
					// Performs a standard psHu32 assignment (which is the default action anyway).
					//psHu32(mem) = value;
				break;

				mcase(SBUS_F220):
					psHu32(mem) |= value;
				return;

				mcase(SBUS_F230):
					psHu32(mem) &= ~value;
				return;

				mcase(SBUS_F240):
					if(!(value & 0x100))
						psHu32(mem) &= ~0x100;
					else
						psHu32(mem) |= 0x100;
				return;

				mcase(SBUS_F260):
					psHu32(mem) = 0;
				return;

				mcase(MCH_RICM)://MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5
					if ((((value >> 16) & 0xFFF) == 0x21) && (((value >> 6) & 0xF) == 1) && (((psHu32(0xf440) >> 7) & 1) == 0))//INIT & SRP=0
						rdram_sdevid = 0;	// if SIO repeater is cleared, reset sdevid
					psHu32(mem) = value & ~0x80000000;	//kill the busy bit
				return;

				mcase(MCH_DRD):
					// Performs a standard psHu32 assignment (which is the default action anyway).
					//psHu32(mem) = value;
				break;

				mcase(DMAC_ENABLEW):
					if (!dmacWrite32<0x0f>(DMAC_ENABLEW, value)) return;

				//mcase(SIO_ISR):
				//mcase(0x1000f410):
				// Mystery Regs!  No one knows!?
				// (unhandled so fall through to default)

			}
		}	
		break;
	}
	
	psHu32(mem) = value;
}

template<uint page>
void __fastcall hwWrite32( u32 mem, u32 value )
{
	eeHwTraceLog( mem, value, false );
	_hwWrite32<page>( mem, value );
}

// --------------------------------------------------------------------------------------
//  hwWrite8 / hwWrite16 / hwWrite64 / hwWrite128
// --------------------------------------------------------------------------------------

template< uint page >
void __fastcall _hwWrite8(u32 mem, u8 value)
{
	iswitch (mem)
	icase(SIO_TXFIFO)
	{
		static bool iggy_newline = false;
		static char sio_buffer[1024];
		static int sio_count;

		if (value == '\r')
		{
			iggy_newline = true;
			sio_buffer[sio_count++] = '\n';
		}
		else if (!iggy_newline || (value != '\n'))
		{
			iggy_newline = false;
			sio_buffer[sio_count++] = value;
		}

		if ((sio_count == ArraySize(sio_buffer)-1) || (sio_buffer[sio_count-1] == '\n'))
		{
			sio_buffer[sio_count] = 0;
			eeConLog( ShiftJIS_ConvertString(sio_buffer) );
			sio_count = 0;
		}
		return;
	}

	u32 merged = _hwRead32<page,false>(mem & ~0x03);
	((u8*)&merged)[mem & 0x3] = value;

	_hwWrite32<page>(mem & ~0x03, merged);
}

template< uint page >
void __fastcall hwWrite8(u32 mem, u8 value)
{
	eeHwTraceLog( mem, value, false );
	_hwWrite8<page>(mem, value);
}

template< uint page >
void __fastcall _hwWrite16(u32 mem, u16 value)
{
	pxAssume( (mem & 0x01) == 0 );

	u32 merged = _hwRead32<page,false>(mem & ~0x03);
	((u16*)&merged)[(mem>>1) & 0x1] = value;

	hwWrite32<page>(mem & ~0x03, merged);
}

template< uint page >
void __fastcall hwWrite16(u32 mem, u16 value)
{
	eeHwTraceLog( mem, value, false );
	_hwWrite16<page>(mem, value);
}

template<uint page>
void __fastcall _hwWrite64( u32 mem, const mem64_t* srcval )
{
	pxAssume( (mem & 0x07) == 0 );

	// * Only the IPU has true 64 bit registers.
	// * FIFOs have 128 bit registers that are probably zero-fill.
	// * All other registers likely disregard the upper 32-bits and simply act as normal
	//   32-bit writes.

	switch (page)
	{
		case 0x02:
			if (!ipuWrite64(mem, *srcval)) return;
		break;

		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		{
			DevCon.WriteLn( Color_Cyan, "Writing 64-bit FIFO data (zero-extended to 128 bits)" );

			u128 zerofill = u128::From32(0);
			zerofill._u64[(mem >> 3) & 0x01] = *srcval;
			hwWrite128<page>(mem & ~0x0f, &zerofill);
		}
		return;
		
		default:
			// disregard everything except the lower 32 bits.
			// ... and skip the 64 bit writeback since the 32-bit one will suffice.
			hwWrite32<page>( mem, ((u32*)srcval)[0] );
		return;
	}

	psHu64(mem) = *srcval;
}

template<uint page>
void __fastcall hwWrite64( u32 mem, const mem64_t* srcval )
{
	eeHwTraceLog( mem, *srcval, false );
	_hwWrite64<page>(mem, srcval);
}

template< uint page >
void __fastcall _hwWrite128(u32 mem, const mem128_t* srcval)
{
	pxAssume( (mem & 0x0f) == 0 );

	// FIFOs are the only "legal" 128 bit registers.  Handle them first.
	// all other registers fall back on the 64-bit handler (and from there
	// most of them fall back to the 32-bit handler).

	switch (page)
	{
		case 0x04:
			WriteFIFO_VIF0(srcval);
		return;

		case 0x05:
			WriteFIFO_VIF1(srcval);
		return;

		case 0x06:
			WriteFIFO_GIF(srcval);
		return;

		case 0x07:
			if (mem & 0x10)
			{
				WriteFIFO_IPUin(srcval);
			}
			else
			{
				// [Ps2Confirm] Most likely writes to IPUout will be silently discarded.  A test
				// to confirm such would be easy -- just dump some data to FIFO_IPUout and see
				// if the program causes BUSERR or something on the PS2.

				//WriteFIFO_IPUout(srcval);
			}
				
		return;
	}

	// All upper bits of all non-FIFO 128-bit HW writes are almost certainly disregarded. --air
	hwWrite64<page>(mem, (mem64_t*)srcval);

	//CopyQWC(&psHu128(mem), srcval);
}

template< uint page >
void __fastcall hwWrite128(u32 mem, const mem128_t* srcval)
{
	eeHwTraceLog( mem, *srcval, false );
	_hwWrite128<page>(mem, srcval);
}

#define InstantizeHwWrite(pageidx) \
	template void __fastcall hwWrite8<pageidx>(u32 mem, mem8_t value); \
	template void __fastcall hwWrite16<pageidx>(u32 mem, mem16_t value); \
	template void __fastcall hwWrite32<pageidx>(u32 mem, mem32_t value); \
	template void __fastcall hwWrite64<pageidx>(u32 mem, const mem64_t* srcval); \
	template void __fastcall hwWrite128<pageidx>(u32 mem, const mem128_t* srcval);

InstantizeHwWrite(0x00);	InstantizeHwWrite(0x08);
InstantizeHwWrite(0x01);	InstantizeHwWrite(0x09);
InstantizeHwWrite(0x02);	InstantizeHwWrite(0x0a);
InstantizeHwWrite(0x03);	InstantizeHwWrite(0x0b);
InstantizeHwWrite(0x04);	InstantizeHwWrite(0x0c);
InstantizeHwWrite(0x05);	InstantizeHwWrite(0x0d);
InstantizeHwWrite(0x06);	InstantizeHwWrite(0x0e);
InstantizeHwWrite(0x07);	InstantizeHwWrite(0x0f);
