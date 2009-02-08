/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include "Common.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "IopMem.h"

// The full suite of hardware APIs:
#include "IPU/IPU.h"
#include "GS.h"
#include "Counters.h"
#include "Vif.h"
#include "VifDma.h"
#include "SPR.h"
#include "Sif.h"


extern int rdram_devices;	// put 8 for TOOL and 2 for PS2 and PSX
extern int rdram_sdevid;
extern char sio_buffer[1024];
extern int sio_count;

int hwConstRead8(u32 x86reg, u32 mem, u32 sign)
{
	if( mem >= 0x10000000 && mem < 0x10008000 )
		DevCon::WriteLn("hwRead8 to %x", params mem);

	if ((mem & 0xffffff0f) == 0x1000f200) {
		if(mem == 0x1000f260) {
			MMXONLY(if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
			else)
                XOR32RtoR(x86reg, x86reg);
			return 0;
		}
		else if(mem == 0x1000F240) {

			_eeReadConstMem8(x86reg, (uptr)&PS2MEM_HW[(mem) & 0xffff], sign);
			//psHu32(mem) &= ~0x4000;
			return 0;
		}
	}
	
	if (mem < 0x10010000)
	{
		_eeReadConstMem8(x86reg, (uptr)&PS2MEM_HW[(mem) & 0xffff], sign);
	}
	else {
		MMXONLY(if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
		else )
            XOR32RtoR(x86reg, x86reg);
	}

	return 0;
}

#define CONSTREAD16_CALL(name) { \
	iFlushCall(0); \
	CALLFunc((uptr)name); \
	if( sign ) MOVSX32R16toR(EAX, EAX); \
	else MOVZX32R16toR(EAX, EAX); \
} \

static u32 s_regreads[3] = {0x010200000, 0xbfff0000, 0xF0000102};
int hwConstRead16(u32 x86reg, u32 mem, u32 sign)
{
	if( mem >= 0x10002000 && mem < 0x10008000 )
		DevCon::WriteLn("hwRead16 to %x", params mem);

	if( mem >= 0x10000000 && mem < 0x10002000 )
		EECNT_LOG("cnt read to %x\n", params mem);

	switch (mem) {
		case 0x10000000:
			PUSH32I(0);
			CONSTREAD16_CALL(rcntRcount);
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10000010:
			_eeReadConstMem16(x86reg, (uptr)&counters[0].mode, sign);
			return 0;
		case 0x10000020:
			_eeReadConstMem16(x86reg, (uptr)&counters[0].mode, sign);
			return 0;
		case 0x10000030:
			_eeReadConstMem16(x86reg, (uptr)&counters[0].hold, sign);
			return 0;

		case 0x10000800:
			PUSH32I(1);
			CONSTREAD16_CALL(rcntRcount);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x10000810:
			_eeReadConstMem16(x86reg, (uptr)&counters[1].mode, sign);
			return 0;

		case 0x10000820:
			_eeReadConstMem16(x86reg, (uptr)&counters[1].target, sign);
			return 0;

		case 0x10000830:
			_eeReadConstMem16(x86reg, (uptr)&counters[1].hold, sign);
			return 0;

		case 0x10001000:
			PUSH32I(2);
			CONSTREAD16_CALL(rcntRcount);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x10001010:
			_eeReadConstMem16(x86reg, (uptr)&counters[2].mode, sign);
			return 0;

		case 0x10001020:
			_eeReadConstMem16(x86reg, (uptr)&counters[2].target, sign);
			return 0;

		case 0x10001800:
			PUSH32I(3);
			CONSTREAD16_CALL(rcntRcount);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x10001810:
			_eeReadConstMem16(x86reg, (uptr)&counters[3].mode, sign);
			return 0;

		case 0x10001820:
			_eeReadConstMem16(x86reg, (uptr)&counters[3].target, sign);
			return 0;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				if(mem == 0x1000f260) {
					MMXONLY(if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
					else )
                        XOR32RtoR(x86reg, x86reg);
					return 0;
				}
				else if(mem == 0x1000F240) {

					MMXONLY(if( IS_MMXREG(x86reg) ) {
						MOVDMtoMMX(x86reg&0xf, (uptr)&PS2MEM_HW[(mem) & 0xffff] - 2);
						PORMtoR(x86reg&0xf, (uptr)&s_regreads[0]);
						PANDMtoR(x86reg&0xf, (uptr)&s_regreads[1]);
					}
					else )
                    {
						if( sign ) MOVSX32M16toR(x86reg, (uptr)&PS2MEM_HW[(mem) & 0xffff]);
						else MOVZX32M16toR(x86reg, (uptr)&PS2MEM_HW[(mem) & 0xffff]);

						OR32ItoR(x86reg, 0x0102);
						AND32ItoR(x86reg, ~0x4000);
					}
					return 0;
				}
			}
			if (mem < 0x10010000) {
				_eeReadConstMem16(x86reg, (uptr)&PS2MEM_HW[(mem) & 0xffff], sign);
			}
			else {
				MMXONLY(if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
				else )
                    XOR32RtoR(x86reg, x86reg);
			}
			
			return 0;
	}
}

int hwContRead32_f440()
{
    if ((psHu32(0xf430) >> 6) & 0xF)
        return 0;
	else
		switch ((psHu32(0xf430)>>16) & 0xFFF){//MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5
		case 0x21://INIT
        {
			int ret = 0x1F * (rdram_sdevid < rdram_devices);
            rdram_sdevid += (rdram_sdevid < rdram_devices);
            return ret;
        }
		case 0x23://CNFGA
			return 0x0D0D;	//PVER=3 | MVER=16 | DBL=1 | REFBIT=5
		case 0x24://CNFGB
			//0x0110 for PSX  SVER=0 | CORG=8(5x9x7) | SPT=1 | DEVTYP=0 | BYTE=0
			return 0x0090;	//SVER=0 | CORG=4(5x9x6) | SPT=1 | DEVTYP=0 | BYTE=0
		case 0x40://DEVID
			return psHu32(0xf430) & 0x1F;	// =SDEV
	}

    return 0;
}

int hwConstRead32(u32 x86reg, u32 mem)
{
	//IPU regs
	if ((mem>=0x10002000) && (mem<0x10003000)) {
		//return ipuConstRead32(x86reg, mem);
		iFlushCall(0);
		PUSH32I( mem );
		CALLFunc( (uptr)ipuRead32 );
	}

	switch (mem) {
		case 0x10000000:
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((uptr)rcntRcount);
			EECNT_LOG("Counter 0 count read = %x\n", rcntRcount(0));
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10000010:
			_eeReadConstMem32(x86reg, (uptr)&counters[0].mode);
			EECNT_LOG("Counter 0 mode read = %x\n", counters[0].modeval);
			return 0;
		case 0x10000020:
			_eeReadConstMem32(x86reg, (uptr)&counters[0].target);
			EECNT_LOG("Counter 0 target read = %x\n", counters[0].target);
			return 0;
		case 0x10000030:
			_eeReadConstMem32(x86reg, (uptr)&counters[0].hold);
			return 0;

		case 0x10000800:
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((uptr)rcntRcount);
			EECNT_LOG("Counter 1 count read = %x\n", rcntRcount(1));
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10000810:
			_eeReadConstMem32(x86reg, (uptr)&counters[1].mode);
			EECNT_LOG("Counter 1 mode read = %x\n", counters[1].modeval);
			return 0;
		case 0x10000820:
			_eeReadConstMem32(x86reg, (uptr)&counters[1].target);
			EECNT_LOG("Counter 1 target read = %x\n", counters[1].target);
			return 0;
		case 0x10000830:
			_eeReadConstMem32(x86reg, (uptr)&counters[1].hold);
			return 0;

		case 0x10001000:
			iFlushCall(0);
			PUSH32I(2);
			CALLFunc((uptr)rcntRcount);
			EECNT_LOG("Counter 2 count read = %x\n", rcntRcount(2));
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10001010:
			_eeReadConstMem32(x86reg, (uptr)&counters[2].mode);
			EECNT_LOG("Counter 2 mode read = %x\n", counters[2].modeval);
			return 0;
		case 0x10001020:
			_eeReadConstMem32(x86reg, (uptr)&counters[2].target);
			EECNT_LOG("Counter 2 target read = %x\n", counters[2].target);
			return 0;
		case 0x10001030:
			// fixme: Counters[2].hold and Counters[3].hold are never assigned values
			// anywhere in Pcsx2.
			_eeReadConstMem32(x86reg, (uptr)&counters[2].hold);
			return 0;

		case 0x10001800:
			iFlushCall(0);
			PUSH32I(3);
			CALLFunc((uptr)rcntRcount);
			EECNT_LOG("Counter 3 count read = %x\n", rcntRcount(3));
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10001810:
			_eeReadConstMem32(x86reg, (uptr)&counters[3].mode);
			EECNT_LOG("Counter 3 mode read = %x\n", counters[3].modeval);
			return 0;
		case 0x10001820:
			_eeReadConstMem32(x86reg, (uptr)&counters[3].target);
			EECNT_LOG("Counter 3 target read = %x\n", counters[3].target);
			return 0;
		case 0x10001830:
			// fixme: Counters[2].hold and Counters[3].hold are never assigned values
			// anywhere in Pcsx2.
			_eeReadConstMem32(x86reg, (uptr)&counters[3].hold);
			return 0;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			if( IS_XMMREG(x86reg) ) SSEX_PXOR_XMM_to_XMM(x86reg&0xf, x86reg&0xf);
			MMXONLY(else if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);)
			else XOR32RtoR(x86reg, x86reg);
			return 0;

		case 0x1000f440:
            iFlushCall(0);
            CALLFunc((uptr)hwContRead32_f440);
            return 1;

		case 0x1000f520: // DMAC_ENABLER
			_eeReadConstMem32(x86reg, (uptr)&PS2MEM_HW[0xf590]);
			return 0;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				if(mem == 0x1000f260) {
					if( IS_XMMREG(x86reg) ) SSEX_PXOR_XMM_to_XMM(x86reg&0xf, x86reg&0xf);
					MMXONLY(else if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);)
					else XOR32RtoR(x86reg, x86reg);
					return 0;
				}
				else if(mem == 0x1000F240) {

					if( IS_XMMREG(x86reg) ) {
						SSEX_MOVD_M32_to_XMM(x86reg&0xf, (uptr)&PS2MEM_HW[(mem) & 0xffff]);
						SSEX_POR_M128_to_XMM(x86reg&0xf, (uptr)&s_regreads[2]);
					}
					MMXONLY(else if( IS_MMXREG(x86reg) ) {
						MOVDMtoMMX(x86reg&0xf, (uptr)&PS2MEM_HW[(mem) & 0xffff]);
						PORMtoR(x86reg&0xf, (uptr)&s_regreads[2]);
					})
					else {
						MOV32MtoR(x86reg, (uptr)&PS2MEM_HW[(mem) & 0xffff]);
						OR32ItoR(x86reg, 0xF0000102);
					}
					return 0;
				}
			}
			
			if (mem < 0x10010000) {
				_eeReadConstMem32(x86reg, (uptr)&PS2MEM_HW[(mem) & 0xffff]);
			}
			else {
				if( IS_XMMREG(x86reg) ) SSEX_PXOR_XMM_to_XMM(x86reg&0xf, x86reg&0xf);
				MMXONLY(else if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);)
				else XOR32RtoR(x86reg, x86reg);
			}

			return 0;
	}
}

void hwConstRead64(u32 mem, int mmreg) {
	if ((mem>=0x10002000) && (mem<0x10003000)) {
		ipuConstRead64(mem, mmreg);
		return;
	}

	if( IS_XMMREG(mmreg) ) SSE_MOVLPS_M64_to_XMM(mmreg&0xff, (uptr)PSM(mem));
	else {
        MMXONLY(MOVQMtoR(mmreg, (uptr)PSM(mem));
		SetMMXstate();)
	}
}

PCSX2_ALIGNED16(u32 s_TempFIFO[4]);
void hwConstRead128(u32 mem, int xmmreg) {

	// fixme : This needs to be updated to use the new paged FIFO accessors.
	/*if (mem >= 0x10004000 && mem < 0x10008000) {
		iFlushCall(0);
		PUSH32I((uptr)&s_TempFIFO[0]);
		PUSH32I(mem);
		CALLFunc((uptr)ReadFIFO);
		ADD32ItoR(ESP, 8);
		_eeReadConstMem128( xmmreg, (uptr)&s_TempFIFO[0]);
		return;
	}*/

	_eeReadConstMem128( xmmreg, (uptr)PSM(mem));
}

// when writing imm
static void recDmaExecI8(void (*name)(), u32 mem, int mmreg)
{
	MOV8ItoM((uptr)&PS2MEM_HW[(mem) & 0xffff], g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]);
	if( g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 1 ) {
		TEST8ItoM((uptr)&PS2MEM_HW[DMAC_CTRL&0xffff], 1);
		j8Ptr[6] = JZ8(0);
		CALLFunc((uptr)name);
		x86SetJ8( j8Ptr[6] );
	}
}

static void recDmaExec8(void (*name)(), u32 mem, int mmreg)
{
	// Flushcall Note : DMA transfers are almost always "involved" operations 
	// that use memcpys and/or threading.  Freeing all XMM and MMX regs is the
	// best option.

	iFlushCall(FLUSH_NOCONST);
	if( IS_EECONSTREG(mmreg) ) {
		recDmaExecI8(name, mem, mmreg);
	}
	else {
		_eeMoveMMREGtoR(EAX, mmreg);
		_eeWriteConstMem8((uptr)&PS2MEM_HW[(mem) & 0xffff], mmreg);

		TEST8ItoR(EAX, 1);
		j8Ptr[5] = JZ8(0);
		TEST8ItoM((uptr)&PS2MEM_HW[DMAC_CTRL&0xffff], 1);
		j8Ptr[6] = JZ8(0);

		CALLFunc((uptr)name);

		x86SetJ8( j8Ptr[5] );
		x86SetJ8( j8Ptr[6] );
	}
}

static void __fastcall PrintDebug(u8 value)
{
	// Note: This is where the EE's diagonstic messages originate from (like the ones that
	// start with hash # marks)

	if (value == '\n') {
		sio_buffer[sio_count] = 0;
		Console::WriteLn( Color_Cyan, sio_buffer );
		sio_count = 0;
	} else {
		if (sio_count < 1023) {
			sio_buffer[sio_count++] = value;
		}
	}
}

// fixme: this would be more optimal as a C++ template (with bit as the template parameter)
template< uint bit >
static void ConstWrite_ExecTimer( uptr func, u8 index, int mmreg)
{
	if( bit != 32 )
	{
		if( !IS_EECONSTREG(mmreg) )
		{
			if( bit == 8 ) MOVZX32R8toR(mmreg&0xf, mmreg&0xf);
			else if( bit == 16 ) MOVZX32R16toR(mmreg&0xf, mmreg&0xf);
		}
	}

	// FlushCall Note : All counter functions are short and sweet, full flush not needed.

	_recPushReg(mmreg);
	iFlushCall(0);
	PUSH32I(index);
	CALLFunc(func);
	ADD32ItoR(ESP, 8);
}

#define CONSTWRITE_TIMERS(bit) \
	case 0x10000000: ConstWrite_ExecTimer<bit>((uptr)&rcntWcount, 0, mmreg); break; \
	case 0x10000010: ConstWrite_ExecTimer<bit>((uptr)&rcntWmode, 0, mmreg); break; \
	case 0x10000020: ConstWrite_ExecTimer<bit>((uptr)&rcntWtarget, 0, mmreg); break; \
	case 0x10000030: ConstWrite_ExecTimer<bit>((uptr)&rcntWhold, 0, mmreg); break; \
	\
	case 0x10000800: ConstWrite_ExecTimer<bit>((uptr)&rcntWcount, 1, mmreg); break; \
	case 0x10000810: ConstWrite_ExecTimer<bit>((uptr)&rcntWmode, 1, mmreg); break; \
	case 0x10000820: ConstWrite_ExecTimer<bit>((uptr)&rcntWtarget, 1, mmreg); break; \
	case 0x10000830: ConstWrite_ExecTimer<bit>((uptr)&rcntWhold, 1, mmreg); break; \
	\
	case 0x10001000: ConstWrite_ExecTimer<bit>((uptr)&rcntWcount, 2, mmreg); break; \
	case 0x10001010: ConstWrite_ExecTimer<bit>((uptr)&rcntWmode, 2, mmreg); break; \
	case 0x10001020: ConstWrite_ExecTimer<bit>((uptr)&rcntWtarget, 2, mmreg); break; \
	\
	case 0x10001800: ConstWrite_ExecTimer<bit>((uptr)&rcntWcount, 3, mmreg); break; \
	case 0x10001810: ConstWrite_ExecTimer<bit>((uptr)&rcntWmode, 3, mmreg); break; \
	case 0x10001820: ConstWrite_ExecTimer<bit>((uptr)&rcntWtarget, 3, mmreg); break; \

void hwConstWrite8(u32 mem, int mmreg)
{
	switch (mem) {
		CONSTWRITE_TIMERS(8)

		case 0x1000f180:
			// Yay fastcall!
			_eeMoveMMREGtoR( ECX, mmreg );
			iFlushCall(0);
			CALLFunc((uptr)PrintDebug);
			break;

		case 0x10008001: // dma0 - vif0
			recDmaExec8(dmaVIF0, mem, mmreg);
			break;

		case 0x10009001: // dma1 - vif1
			recDmaExec8(dmaVIF1, mem, mmreg);
			break;

		case 0x1000a001: // dma2 - gif
			recDmaExec8(dmaGIF, mem, mmreg);
			break;

		case 0x1000b001: // dma3 - fromIPU
			recDmaExec8(dmaIPU0, mem, mmreg);
			break;

		case 0x1000b401: // dma4 - toIPU
			recDmaExec8(dmaIPU1, mem, mmreg);
			break;

		case 0x1000c001: // dma5 - sif0
			//if (value == 0) psxSu32(0x30) = 0x40000;
			recDmaExec8(dmaSIF0, mem, mmreg);
			break;

		case 0x1000c401: // dma6 - sif1
			recDmaExec8(dmaSIF1, mem, mmreg);
			break;

		case 0x1000c801: // dma7 - sif2
			recDmaExec8(dmaSIF2, mem, mmreg);
			break;

		case 0x1000d001: // dma8 - fromSPR
			recDmaExec8(dmaSPR0, mem, mmreg);
			break;

		case 0x1000d401: // dma9 - toSPR
			recDmaExec8(dmaSPR1, mem, mmreg);
			break;

		case 0x1000f592: // DMAC_ENABLEW
			_eeWriteConstMem8( (uptr)&PS2MEM_HW[0xf522], mmreg );
			_eeWriteConstMem8( (uptr)&PS2MEM_HW[0xf592], mmreg );			
			break;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					_eeWriteConstMem8( (uptr)&PS2MEM_HW[mem&0xffff], mmreg);
					break;
				case 0x40:
					if( IS_EECONSTREG(mmreg) ) {
						if( !(g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 0x100) ) {
							AND32ItoM( (uptr)&PS2MEM_HW[mem&0xfffc], ~0x100);
						}
					}
					else {
						_eeMoveMMREGtoR(EAX, mmreg);
						TEST16ItoR(EAX, 0x100);
						j8Ptr[5] = JNZ8(0);
						AND32ItoM( (uptr)&PS2MEM_HW[mem&0xfffc], ~0x100);
						x86SetJ8(j8Ptr[5]);
					}
					break;
				}
				return;
			}
			assert( (mem&0xff0f) != 0xf200 );

			switch(mem&~3) {
				case 0x1000f130:
				case 0x1000f410:
				case 0x1000f430:
					break;
				default:
					_eeWriteConstMem8((uptr)&PS2MEM_HW[mem&0xffff], mmreg);
			}

			break;
	}
}

// Flushcall Note : DMA transfers are almost always "involved" operations 
// that use memcpys and/or threading.  Freeing all XMM and MMX regs is the
// best option (removes the need for FreezeXMMRegs()).  But register
// allocation is such a mess right now that we can't do it (yet).

static void recDmaExecI16( void (*name)(), u32 mem, int mmreg )
{
	MOV16ItoM((uptr)&PS2MEM_HW[(mem) & 0xffff], g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]);
	if( g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 0x100 ) {
		TEST8ItoM((uptr)&PS2MEM_HW[DMAC_CTRL&0xffff], 1);
		j8Ptr[6] = JZ8(0);
		CALLFunc((uptr)name);
		x86SetJ8( j8Ptr[6] );
	}
}

static void recDmaExec16(void (*name)(), u32 mem, int mmreg)
{
	iFlushCall(0);

	if( IS_EECONSTREG(mmreg) ) {
		recDmaExecI16(name, mem, mmreg);
	}
	else {
		_eeMoveMMREGtoR(EAX, mmreg);
		_eeWriteConstMem16((uptr)&PS2MEM_HW[(mem) & 0xffff], mmreg);

		TEST16ItoR(EAX, 0x100);
		j8Ptr[5] = JZ8(0);
		TEST8ItoM((uptr)&PS2MEM_HW[DMAC_CTRL&0xffff], 1);
		j8Ptr[6] = JZ8(0);

		CALLFunc((uptr)name);

		x86SetJ8( j8Ptr[5] );
		x86SetJ8( j8Ptr[6] );
	}
}

void hwConstWrite16(u32 mem, int mmreg)
{
	switch(mem) {

		CONSTWRITE_TIMERS(16)

		case 0x10008000: // dma0 - vif0
			recDmaExec16(dmaVIF0, mem, mmreg);
			break;

		case 0x10009000: // dma1 - vif1 - chcr
			recDmaExec16(dmaVIF1, mem, mmreg);
			break;

		case 0x1000a000: // dma2 - gif
			recDmaExec16(dmaGIF, mem, mmreg);
			break;
		case 0x1000b000: // dma3 - fromIPU
			recDmaExec16(dmaIPU0, mem, mmreg);
			break;
		case 0x1000b400: // dma4 - toIPU
			recDmaExec16(dmaIPU1, mem, mmreg);
			break;
		case 0x1000c000: // dma5 - sif0
			//if (value == 0) psxSu32(0x30) = 0x40000;
			recDmaExec16(dmaSIF0, mem, mmreg);
			break;
		case 0x1000c002:
			//?
			break;
		case 0x1000c400: // dma6 - sif1
			recDmaExec16(dmaSIF1, mem, mmreg);
			break;
		case 0x1000c800: // dma7 - sif2
			recDmaExec16(dmaSIF2, mem, mmreg);
			break;
		case 0x1000c802:
			//?
			break;
		case 0x1000d000: // dma8 - fromSPR
			recDmaExec16(dmaSPR0, mem, mmreg);
			break;
		case 0x1000d400: // dma9 - toSPR
			recDmaExec16(dmaSPR1, mem, mmreg);
			break;
		case 0x1000f592: // DMAC_ENABLEW
			_eeWriteConstMem16((uptr)&PS2MEM_HW[0xf522], mmreg);
			_eeWriteConstMem16((uptr)&PS2MEM_HW[0xf592], mmreg);
			break;
		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;
		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					_eeWriteConstMem16((uptr)&PS2MEM_HW[mem&0xffff], mmreg);
					break;
				case 0x20:
					_eeWriteConstMem16OP((uptr)&PS2MEM_HW[mem&0xffff], mmreg, 1);
					break;
				case 0x30:
					if( IS_EECONSTREG(mmreg) ) {
						AND16ItoM((uptr)&PS2MEM_HW[mem&0xffff], ~g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]);
					}
					else {
						NOT32R(mmreg&0xf);
						AND16RtoM((uptr)&PS2MEM_HW[mem&0xffff], mmreg&0xf);
					}
					break;
				case 0x40:
					if( IS_EECONSTREG(mmreg) ) {
						if( !(g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 0x100) ) {
							AND16ItoM((uptr)&PS2MEM_HW[mem&0xffff], ~0x100);
						}
						else {
							OR16ItoM((uptr)&PS2MEM_HW[mem&0xffff], 0x100);
						}
					}
					else {
						_eeMoveMMREGtoR(EAX, mmreg);
						TEST16ItoR(EAX, 0x100);
						j8Ptr[5] = JZ8(0);
						OR16ItoM((uptr)&PS2MEM_HW[mem&0xffff], 0x100);
						j8Ptr[6] = JMP8(0);

						x86SetJ8( j8Ptr[5] );
						AND16ItoM((uptr)&PS2MEM_HW[mem&0xffff], ~0x100);

						x86SetJ8( j8Ptr[6] );
					}

					break;
				case 0x60:
					_eeWriteConstMem16((uptr)&PS2MEM_HW[mem&0xffff], 0);
					break;
				}
				return;
			}

			_eeWriteConstMem16((uptr)&PS2MEM_HW[mem&0xffff], mmreg);
	}
}

// when writing an Imm

static void recDmaExecI( void (*name)(), u32 mem, int mmreg )
{
	u32 c = g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0];
    /* Keep the old tag if in chain mode and hw doesnt set it*/
    if( (c & 0xc) == 0x4 && (c&0xffff0000) == 0 ) {
        MOV16ItoM((uptr)&PS2MEM_HW[(mem) & 0xffff], c);
    }
	else MOV32ItoM((uptr)&PS2MEM_HW[(mem) & 0xffff], c);
	if( c & 0x100 ) {
		TEST8ItoM((uptr)&PS2MEM_HW[DMAC_CTRL&0xffff], 1);
		j8Ptr[6] = JZ8(0);
		CALLFunc((uptr)name);
		x86SetJ8( j8Ptr[6] );
	}
}

static void recDmaExec( void (*name)(), u32 mem, int mmreg )
{

	iFlushCall(0);

	if( IS_EECONSTREG(mmreg) ) {
		recDmaExecI(name, mem, mmreg);
	}
	else {

		// fixme: This is a lot of code to be injecting into the recompiler
		// for every DMA transfer.  It might actually be more efficient to
		// set this up as a C function call instead (depends on how often
		// the register is written without actually starting a DMA xfer).

		_eeMoveMMREGtoR(EAX, mmreg);
        TEST32ItoR(EAX, 0xffff0000);
        j8Ptr[6] = JNZ8(0);
        MOV32RtoR(ECX, EAX);
        AND32ItoR(ECX, 0xc);
        CMP32ItoR(ECX, 4);
        j8Ptr[7] = JNE8(0);
        if( IS_XMMREG(mmreg) || IS_MMXREG(mmreg) ) {
			MOV16RtoM((uptr)&PS2MEM_HW[(mem) & 0xffff], EAX);
		}
		else {
			_eeWriteConstMem16((uptr)&PS2MEM_HW[(mem) & 0xffff], mmreg);
		}
        j8Ptr[8] = JMP8(0);
        x86SetJ8(j8Ptr[6]);
        x86SetJ8(j8Ptr[7]);
        _eeWriteConstMem32((uptr)&PS2MEM_HW[(mem) & 0xffff], mmreg);
        x86SetJ8(j8Ptr[8]);

        TEST16ItoR(EAX, 0x100);
		j8Ptr[5] = JZ8(0);
		TEST32ItoM((uptr)&PS2MEM_HW[DMAC_CTRL&0xffff], 1);
		j8Ptr[6] = JZ8(0);

		CALLFunc((uptr)name);
		x86SetJ8( j8Ptr[5] );
		x86SetJ8( j8Ptr[6] );
	}
}


void hwConstWrite32(u32 mem, int mmreg)
{
	//IPU regs
	if ((mem>=0x10002000) && (mem<0x10003000)) {
    	//psHu32(mem) = value;
		ipuConstWrite32(mem, mmreg);
		return;
	}

	if ((mem>=0x10003800) && (mem<0x10003c00)) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(mem);
		CALLFunc((uptr)vif0Write32);
		ADD32ItoR(ESP, 8);
		return;
	}
	if ((mem>=0x10003c00) && (mem<0x10004000)) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(mem);
		CALLFunc((uptr)vif1Write32);
		ADD32ItoR(ESP, 8);
		return;
	}

	switch (mem) {

		CONSTWRITE_TIMERS(32)

		case GIF_CTRL:

			_eeMoveMMREGtoR(EAX, mmreg);

			iFlushCall(0);
			TEST8ItoR(EAX, 1);
			j8Ptr[5] = JZ8(0);

			// reset GS
			CALLFunc((uptr)gsGIFReset);
			j8Ptr[6] = JMP8(0);

			x86SetJ8( j8Ptr[5] );
			AND32I8toR(EAX, 8);
			MOV32RtoM((uptr)&PS2MEM_HW[mem&0xffff], EAX);

			TEST16ItoR(EAX, 8);
			j8Ptr[5] = JZ8(0);
			OR8ItoM((uptr)&PS2MEM_HW[GIF_STAT&0xffff], 8);
			j8Ptr[7] = JMP8(0);

			x86SetJ8( j8Ptr[5] );
			AND8ItoM((uptr)&PS2MEM_HW[GIF_STAT&0xffff], ~8);
			x86SetJ8( j8Ptr[6] );
			x86SetJ8( j8Ptr[7] );
			return;

		case GIF_MODE:
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem32((uptr)&PS2MEM_HW[mem&0xffff], mmreg);
			AND8ItoM((uptr)&PS2MEM_HW[GIF_STAT&0xffff], ~5);
			AND8ItoR(EAX, 5);
			OR8RtoM((uptr)&PS2MEM_HW[GIF_STAT&0xffff], EAX);
			return;

		case GIF_STAT: // stat is readonly
			return;

		case 0x10008000: // dma0 - vif0
			recDmaExec(dmaVIF0, mem, mmreg);
			break;

		case 0x10009000: // dma1 - vif1 - chcr
			recDmaExec(dmaVIF1, mem, mmreg);
			break;

		case 0x1000a000: // dma2 - gif
			recDmaExec(dmaGIF, mem, mmreg);
			break;

		case 0x1000b000: // dma3 - fromIPU
			recDmaExec(dmaIPU0, mem, mmreg);
			break;
		case 0x1000b400: // dma4 - toIPU
			recDmaExec(dmaIPU1, mem, mmreg);
			break;
		case 0x1000c000: // dma5 - sif0
			//if (value == 0) psxSu32(0x30) = 0x40000;
			recDmaExec(dmaSIF0, mem, mmreg);
			break;

		case 0x1000c400: // dma6 - sif1
			recDmaExec(dmaSIF1, mem, mmreg);
			break;

		case 0x1000c800: // dma7 - sif2
			recDmaExec(dmaSIF2, mem, mmreg);
			break;

		case 0x1000d000: // dma8 - fromSPR
			recDmaExec(dmaSPR0, mem, mmreg);
			break;

		case 0x1000d400: // dma9 - toSPR
			recDmaExec(dmaSPR1, mem, mmreg);
			break;

		case 0x1000e010: // DMAC_STAT
			_eeMoveMMREGtoR(EAX, mmreg);
			iFlushCall(0);
			MOV32RtoR(ECX, EAX);
			SHR32ItoR(EAX, 16);
			NOT32R(ECX);
			XOR16RtoM((uptr)&PS2MEM_HW[0xe012], EAX);
			AND16RtoM((uptr)&PS2MEM_HW[0xe010], ECX);

			CALLFunc((uptr)cpuTestDMACInts);
			break;

		case 0x1000f000: // INTC_STAT
			_eeWriteConstMem32OP((uptr)&PS2MEM_HW[0xf000], mmreg, 2);
			CALLFunc((uptr)cpuTestINTCInts);
			break;

		case 0x1000f010: // INTC_MASK
			_eeMoveMMREGtoR(EAX, mmreg);
			iFlushCall(0);
			XOR16RtoM((uptr)&PS2MEM_HW[0xf010], EAX);
			CALLFunc((uptr)cpuTestINTCInts);
			break;

        case 0x1000f130:
		case 0x1000f410:
			break;

        case 0x1000f430://MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5

            //if ((((value >> 16) & 0xFFF) == 0x21) && (((value >> 6) & 0xF) == 1) && (((psHu32(0xf440) >> 7) & 1) == 0))//INIT & SRP=0
            //    rdram_sdevid = 0
            _eeMoveMMREGtoR(EAX, mmreg);
            MOV32RtoR(EDX, EAX);
            MOV32RtoR(ECX, EAX);
            SHR32ItoR(EAX, 6);
            SHR32ItoR(EDX, 16);
            AND32ItoR(EAX, 0xf);
            AND32ItoR(EDX, 0xfff);
            CMP32ItoR(EAX, 1);
            j8Ptr[5] = JNE8(0);
            CMP32ItoR(EDX, 0x21);
            j8Ptr[6] = JNE8(0);

            TEST32ItoM((uptr)&psHu32(0xf440), 0x80);
            j8Ptr[7] = JNZ8(0);

            // if SIO repeater is cleared, reset sdevid
            MOV32ItoM((uptr)&rdram_sdevid, 0);
			
            //kill the busy bit
            x86SetJ8(j8Ptr[5]);
            x86SetJ8(j8Ptr[6]);
            x86SetJ8(j8Ptr[7]);
            AND32ItoR(ECX, ~0x80000000);
            MOV32RtoM((uptr)&psHu32(0xf430), ECX);
			break;

		case 0x1000f440://MCH_DRD:
            _eeWriteConstMem32((uptr)&PS2MEM_HW[0xf440], mmreg);
			break;

		case 0x1000f590: // DMAC_ENABLEW
			_eeWriteConstMem32((uptr)&PS2MEM_HW[0xf520], mmreg);
			_eeWriteConstMem32((uptr)&PS2MEM_HW[0xf590], mmreg);
			return;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					_eeWriteConstMem32((uptr)&PS2MEM_HW[mem&0xffff], mmreg);
					break;
				case 0x20:
					_eeWriteConstMem32OP((uptr)&PS2MEM_HW[mem&0xffff], mmreg, 1);
					break;
				case 0x30:
					_eeWriteConstMem32OP((uptr)&PS2MEM_HW[mem&0xffff], mmreg, 2);
					break;
				case 0x40:
					if( IS_EECONSTREG(mmreg) ) {
						if( !(g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 0x100) ) {
							AND32ItoM( (uptr)&PS2MEM_HW[mem&0xfffc], ~0x100);
						}
						else {
							OR32ItoM((uptr)&PS2MEM_HW[mem&0xffff], 0x100);
						}
					}
					else {
						_eeMoveMMREGtoR(EAX, mmreg);
						TEST32ItoR(EAX, 0x100);
						j8Ptr[5] = JZ8(0);
						OR32ItoM((uptr)&PS2MEM_HW[mem&0xffff], 0x100);
						j8Ptr[6] = JMP8(0);

						x86SetJ8( j8Ptr[5] );
						AND32ItoM((uptr)&PS2MEM_HW[mem&0xffff], ~0x100);

						x86SetJ8( j8Ptr[6] );
					}

					break;
				case 0x60:
					MOV32ItoM((uptr)&PS2MEM_HW[mem&0xffff], 0);
					break;
				}
				return;
			}

			_eeWriteConstMem32((uptr)&PS2MEM_HW[mem&0xffff], mmreg);
		break;
	}
}

void hwConstWrite64(u32 mem, int mmreg)
{
	if ((mem>=0x10002000) && (mem<=0x10002030)) {
		ipuConstWrite64(mem, mmreg);
		return;
	}
	
	if ((mem>=0x10003800) && (mem<0x10003c00)) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(mem);
		CALLFunc((uptr)vif0Write32);
		ADD32ItoR(ESP, 8);
		return;
	}
	if ((mem>=0x10003c00) && (mem<0x10004000)) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(mem);
		CALLFunc((uptr)vif1Write32);
		ADD32ItoR(ESP, 8);
		return;
	}

	switch (mem) {
		case GIF_CTRL:
			_eeMoveMMREGtoR(EAX, mmreg);
			
			iFlushCall(0);
			TEST8ItoR(EAX, 1);
			j8Ptr[5] = JZ8(0);

			// reset GS
			CALLFunc((uptr)gsGIFReset);
			j8Ptr[6] = JMP8(0);

			x86SetJ8( j8Ptr[5] );
			AND32I8toR(EAX, 8);
			MOV32RtoM((uptr)&PS2MEM_HW[mem&0xffff], EAX);

			TEST16ItoR(EAX, 8);
			j8Ptr[5] = JZ8(0);
			OR8ItoM((uptr)&PS2MEM_HW[GIF_STAT&0xffff], 8);
			j8Ptr[7] = JMP8(0);

			x86SetJ8( j8Ptr[5] );
			AND8ItoM((uptr)&PS2MEM_HW[GIF_STAT&0xffff], ~8);
			x86SetJ8( j8Ptr[6] );
			x86SetJ8( j8Ptr[7] );
			return;

		case GIF_MODE:
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem32((uptr)&PS2MEM_HW[mem&0xffff], mmreg);

			AND8ItoM((uptr)&PS2MEM_HW[GIF_STAT&0xffff], ~5);
			AND8ItoR(EAX, 5);
			OR8RtoM((uptr)&PS2MEM_HW[GIF_STAT&0xffff], EAX);
			break;

		case GIF_STAT: // stat is readonly
			return;

		case 0x1000a000: // dma2 - gif
			recDmaExec(dmaGIF, mem, mmreg);
			break;

		case 0x1000e010: // DMAC_STAT
			_eeMoveMMREGtoR(EAX, mmreg);

			iFlushCall(0);
			MOV32RtoR(ECX, EAX);
			SHR32ItoR(EAX, 16);
			NOT32R(ECX);
			XOR16RtoM((uptr)&PS2MEM_HW[0xe012], EAX);
			AND16RtoM((uptr)&PS2MEM_HW[0xe010], ECX);

			CALLFunc((uptr)cpuTestDMACInts);
			break;

		case 0x1000f590: // DMAC_ENABLEW
			_eeWriteConstMem32((uptr)&PS2MEM_HW[0xf520], mmreg);
			_eeWriteConstMem32((uptr)&PS2MEM_HW[0xf590], mmreg);
			break;

		case 0x1000f000: // INTC_STAT
			_eeWriteConstMem32OP((uptr)&PS2MEM_HW[mem&0xffff], mmreg, 2);
			CALLFunc((uptr)cpuTestINTCInts);
			break;

		case 0x1000f010: // INTC_MASK

			_eeMoveMMREGtoR(EAX, mmreg);

			iFlushCall(0);
			XOR16RtoM((uptr)&PS2MEM_HW[0xf010], EAX);
			CALLFunc((uptr)cpuTestINTCInts);			
			break;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;
		default:

			_eeWriteConstMem64((uptr)PSM(mem), mmreg);
			break;
	}
}

void hwConstWrite128(u32 mem, int mmreg)
{
	// fixme : This needs to be updated to use the new paged FIFO accessors.

	/*if (mem >= 0x10004000 && mem < 0x10008000) {
		_eeWriteConstMem128((uptr)&s_TempFIFO[0], mmreg);
		iFlushCall(0);
		PUSH32I((uptr)&s_TempFIFO[0]);
		PUSH32I(mem);
		CALLFunc((uptr)WriteFIFO);
		ADD32ItoR(ESP, 8);
		return;
	}*/

	switch (mem) {
		case 0x1000f590: // DMAC_ENABLEW
			_eeWriteConstMem32((uptr)&PS2MEM_HW[0xf520], mmreg);
			_eeWriteConstMem32((uptr)&PS2MEM_HW[0xf590], mmreg);
			break;
		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		default:

			_eeWriteConstMem128((uptr)&PS2MEM_HW[mem&0xffff], mmreg);
			break;
	}
}
