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

#include "ps2/HwInternal.h"
#include "ps2/eeHwTraceLog.inl"

using namespace R5900;

static __fi void IntCHackCheck()
{
	// Sanity check: To protect from accidentally "rewinding" the cyclecount
	// on the few times nextBranchCycle can be behind our current cycle.
	s32 diff = g_nextEventCycle - cpuRegs.cycle;
	if( diff > 0 ) cpuRegs.cycle = g_nextEventCycle;
}

static const uint HwF_VerboseConLog	= 1<<0;
static const uint HwF_IntcStatHack	= 1<<1;	// used for Reads only.

template< uint page > void __fastcall _hwRead128(u32 mem, mem128_t* result );

template< uint page, bool intcstathack >
mem32_t __fastcall _hwRead32(u32 mem)
{
	pxAssume( (mem & 0x03) == 0 );

	switch( page )
	{
		case 0x00:	return rcntRead32<0x00>( mem );
		case 0x01:	return rcntRead32<0x01>( mem );
		
		case 0x02:	return ipuRead32( mem );

		case 0x03:
			if (mem >= EEMemoryMap::VIF0_Start)
			{
				if(mem >= EEMemoryMap::VIF1_Start)
					return vifRead32<1>(mem);
				else
					return vifRead32<0>(mem);
			}
			return dmacRead32<0x03>( mem );
		
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		{
			// [Ps2Confirm] Reading from FIFOs using non-128 bit reads is a complete mystery.
			// No game is known to attempt such a thing (yay!), so probably nothing for us to
			// worry about.  Chances are, though, doing so is "legal" and yields some sort
			// of reproducible behavior.  Candidate for real hardware testing.
			
			// Current assumption: Reads 128 bits and discards the unused portion.

			DevCon.WriteLn( Color_Cyan, "Reading 32-bit FIFO data" );

			u128 out128;
			_hwRead128<page>(mem & ~0x0f, &out128);
			return out128._u32[(mem >> 2) & 0x3];
		}
		break;

		case 0x0f:
		{
			// INTC_STAT shortcut for heavy spinning.
			// Performance Note: Visual Studio handles this best if we just manually check for it here,
			// outside the context of the switch statement below.  This is likely fixed by PGO also,
			// but it's an easy enough conditional to account for anyways.

			if (mem == INTC_STAT)
			{
				if (intcstathack) IntCHackCheck();
				return psHu32(INTC_STAT);
			}

			//if ((mem & 0x1000f200) == 0x1000f200)
			//	Console.Error("SBUS");

			switch( mem )
			{
				case SIO_ISR:
				case SBUS_F260:
				case 0x1000f410:
				case MCH_RICM:
					return 0;

				case SBUS_F240:
					return psHu32(SBUS_F240) | 0xF0000102;

				case MCH_DRD:
					if( !((psHu32(MCH_RICM) >> 6) & 0xF) )
					{
						switch ((psHu32(MCH_RICM)>>16) & 0xFFF)
						{
							//MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5

							case 0x21://INIT
								if(rdram_sdevid < rdram_devices)
								{
									rdram_sdevid++;
									return 0x1F;
								}
							return 0;

							case 0x23://CNFGA
								return 0x0D0D;	//PVER=3 | MVER=16 | DBL=1 | REFBIT=5

							case 0x24://CNFGB
								//0x0110 for PSX  SVER=0 | CORG=8(5x9x7) | SPT=1 | DEVTYP=0 | BYTE=0
								return 0x0090;	//SVER=0 | CORG=4(5x9x6) | SPT=1 | DEVTYP=0 | BYTE=0

							case 0x40://DEVID
								return psHu32(MCH_RICM) & 0x1F;	// =SDEV
						}
					}
				return 0;
			}
		}
		break;
	}
	//Hack for Transformers and Test Drive Unlimited to simulate filling the VIF FIFO
	//It actually stalls VIF a few QW before the end of the transfer, so we need to pretend its all gone
	//else itll take aaaaaaaaages to boot.
	if(mem == (D1_CHCR + 0x10) && CHECK_VIFFIFOHACK) 
		return psHu32(mem) + (vif1ch.qwc * 16);

	return psHu32(mem);
}

template< uint page >
mem32_t __fastcall hwRead32(u32 mem)
{
	mem32_t retval = _hwRead32<page,false>(mem);
	eeHwTraceLog( mem, retval, true );
	return retval;
}

mem32_t __fastcall hwRead32_page_0F_INTC_HACK(u32 mem)
{
	mem32_t retval = _hwRead32<0x0f,true>(mem);
	eeHwTraceLog( mem, retval, true );
	return retval;
}

// --------------------------------------------------------------------------------------
//  hwRead8 / hwRead16 / hwRead64 / hwRead128
// --------------------------------------------------------------------------------------

template< uint page >
mem8_t __fastcall _hwRead8(u32 mem)
{
	u32 ret32 = _hwRead32<page, false>(mem & ~0x03);
	return ((u8*)&ret32)[mem & 0x03];
}

template< uint page >
mem8_t __fastcall hwRead8(u32 mem)
{
	mem8_t ret8 = _hwRead8<0x0f>(mem);
	eeHwTraceLog( mem, ret8, true );
	return ret8;
}

template< uint page >
mem16_t __fastcall _hwRead16(u32 mem)
{
	pxAssume( (mem & 0x01) == 0 );

	u32 ret32 = _hwRead32<page, false>(mem & ~0x03);
	return ((u16*)&ret32)[(mem>>1) & 0x01];
}

template< uint page >
mem16_t __fastcall hwRead16(u32 mem)
{
	u16 ret16 = _hwRead16<page>(mem);
	eeHwTraceLog( mem, ret16, true );
	return ret16;
}

mem16_t __fastcall hwRead16_page_0F_INTC_HACK(u32 mem)
{
	pxAssume( (mem & 0x01) == 0 );

	u32 ret32 = _hwRead32<0x0f, true>(mem & ~0x03);
	u16 ret16 = ((u16*)&ret32)[(mem>>1) & 0x01];

	eeHwTraceLog( mem, ret16, "Read" );
	return ret16;
}

template< uint page >
static void _hwRead64(u32 mem, mem64_t* result )
{
	pxAssume( (mem & 0x07) == 0 );

	switch (page)
	{
		case 0x02:
			*result = ipuRead64(mem);
		return;

		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		{
			// [Ps2Confirm] Reading from FIFOs using non-128 bit reads is a complete mystery.
			// No game is known to attempt such a thing (yay!), so probably nothing for us to
			// worry about.  Chances are, though, doing so is "legal" and yields some sort
			// of reproducible behavior.  Candidate for real hardware testing.
			
			// Current assumption: Reads 128 bits and discards the unused portion.

			uint wordpart = (mem >> 3) & 0x1;
			DevCon.WriteLn( Color_Cyan, "Reading 64-bit FIFO data (%s 64 bits discarded)", wordpart ? "upper" : "lower" );

			u128 out128;
			_hwRead128<page>(mem & ~0x0f, &out128);
			*result = out128._u64[wordpart];
		}
		return;
	}

	*result = _hwRead32<page,false>( mem );
}

template< uint page >
void __fastcall hwRead64(u32 mem, mem64_t* result )
{
	_hwRead64<page>( mem, result );
	eeHwTraceLog( mem, *result, true );
}

template< uint page >
void __fastcall _hwRead128(u32 mem, mem128_t* result )
{
	pxAssume( (mem & 0x0f) == 0 );

	// FIFOs are the only "legal" 128 bit registers, so we Handle them first.
	// All other registers fall back on the 64-bit handler (and from there
	// all non-IPU reads fall back to the 32-bit handler).

	switch (page)
	{
		case 0x05:
			ReadFIFO_VIF1( result );
		break;

		case 0x07:
			if (mem & 0x10)
				ZeroQWC( result );		// IPUin is write-only
			else
				ReadFIFO_IPUout( result );
		break;

		case 0x04:
		case 0x06:
			// VIF0 and GIF are write-only.
			// [Ps2Confirm] Reads from these FIFOs (and IPUin) do one of the following:
			// return zero, leave contents of the dest register unchanged, or in some
			// indeterminate state.  The actual behavior probably isn't important.
			ZeroQWC( result );
		break;
		
		default:
			_hwRead64<page>( mem, &result->lo );
			result->hi = 0;
		break;		
	}
}

template< uint page >
void __fastcall hwRead128(u32 mem, mem128_t* result )
{
	_hwRead128<page>( mem, result );
	eeHwTraceLog( mem, *result, true );
}

#define InstantizeHwRead(pageidx) \
	template mem8_t __fastcall hwRead8<pageidx>(u32 mem); \
	template mem16_t __fastcall hwRead16<pageidx>(u32 mem); \
	template mem32_t __fastcall hwRead32<pageidx>(u32 mem); \
	template void __fastcall hwRead64<pageidx>(u32 mem, mem64_t* result ); \
	template void __fastcall hwRead128<pageidx>(u32 mem, mem128_t* result ); \
	template mem32_t __fastcall _hwRead32<pageidx, false>(u32 mem);

InstantizeHwRead(0x00);	InstantizeHwRead(0x08);
InstantizeHwRead(0x01);	InstantizeHwRead(0x09);
InstantizeHwRead(0x02);	InstantizeHwRead(0x0a);
InstantizeHwRead(0x03);	InstantizeHwRead(0x0b);
InstantizeHwRead(0x04);	InstantizeHwRead(0x0c);
InstantizeHwRead(0x05);	InstantizeHwRead(0x0d);
InstantizeHwRead(0x06);	InstantizeHwRead(0x0e);
InstantizeHwRead(0x07);	InstantizeHwRead(0x0f);
