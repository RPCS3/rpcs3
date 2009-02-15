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
#include "iR5900.h"

extern int g_pbufi;
extern s8 g_pbuf[1024];

#define CONSTREAD8_CALL(name) { \
	iFlushCall(0); \
	CALLFunc((uptr)name); \
	if( sign ) MOVSX32R8toR(EAX, EAX); \
	else MOVZX32R8toR(EAX, EAX); \
} \

static u32 s_16 = 0x10;

int psxHwConstRead8(u32 x86reg, u32 add, u32 sign) {
	
	if (add >= 0x1f801600 && add < 0x1f801700) {
		PUSH32I(add);
		CONSTREAD8_CALL(USBread8);
		// since calling from different dll, esp already changed
		return 1;
	}

	switch (add) {
		case 0x1f801040:
			CONSTREAD8_CALL(sioRead8);
			return 1;
      //  case 0x1f801050: hard = serial_read8(); break;//for use of serial port ignore for now

#ifdef PCSX2_DEVBUILD
		case 0x1f801100:
		case 0x1f801104:
		case 0x1f801108:
		case 0x1f801110:
		case 0x1f801114:
		case 0x1f801118:
		case 0x1f801120:
		case 0x1f801124:
		case 0x1f801128:
		case 0x1f801480:
		case 0x1f801484:
		case 0x1f801488:
		case 0x1f801490:
		case 0x1f801494:
		case 0x1f801498:
		case 0x1f8014a0:
		case 0x1f8014a4:
		case 0x1f8014a8:
			SysPrintf("8bit counter read %x\n", add);
			_eeReadConstMem8(x86reg, (uptr)&psxH[(add) & 0xffff], sign);
			return 0;
#endif

		case 0x1f80146e: // DEV9_R_REV
			PUSH32I(add);
			CONSTREAD8_CALL(DEV9read8);
			return 1;

		case 0x1f801800: CONSTREAD8_CALL(cdrRead0); return 1;
		case 0x1f801801: CONSTREAD8_CALL(cdrRead1); return 1;
		case 0x1f801802: CONSTREAD8_CALL(cdrRead2); return 1;
		case 0x1f801803: CONSTREAD8_CALL(cdrRead3); return 1;

		case 0x1f803100: // PS/EE/IOP conf related
			if( IS_XMMREG(x86reg) ) SSEX_MOVD_M32_to_XMM(x86reg&0xf, (uptr)&s_16);
			MMXONLY(else if( IS_MMXREG(x86reg) ) MOVDMtoMMX(x86reg&0xf, (uptr)&s_16);)
			else MOV32ItoR(x86reg, 0x10);
			return 0;

		case 0x1F808264: //sio2 serial data feed/fifo_out
			CONSTREAD8_CALL(sio2_fifoOut);
			return 1;

		default:
			_eeReadConstMem8(x86reg, (uptr)&psxH[(add) & 0xffff], sign);
			return 0;
	}
}

#define CONSTREAD16_CALL(name) { \
	iFlushCall(0); \
	CALLFunc((uptr)name); \
	if( sign ) MOVSX32R16toR(EAX, EAX); \
	else MOVZX32R16toR(EAX, EAX); \
} \

void psxConstReadCounterMode16(int x86reg, int index, int sign)
{
	if( IS_MMXREG(x86reg) ) {
		MMXONLY(MOV16MtoR(ECX, (uptr)&psxCounters[index].mode);
		MOVDMtoMMX(x86reg&0xf, (uptr)&psxCounters[index].mode - 2);)
	}
	else {
		if( sign ) MOVSX32M16toR(ECX, (uptr)&psxCounters[index].mode);
		else MOVZX32M16toR(ECX, (uptr)&psxCounters[index].mode);

		MOV32RtoR(x86reg, ECX);
	}

	AND16ItoR(ECX, ~0x1800);
	OR16ItoR(ECX, 0x400);
	MOV16RtoM((uptr)&psxCounters[index].mode, ECX);
}

int psxHwConstRead16(u32 x86reg, u32 add, u32 sign) {
	if (add >= 0x1f801600 && add < 0x1f801700) {
		PUSH32I(add);
		CONSTREAD16_CALL(USBread16);
		return 1;
	}

	switch (add) {

		case 0x1f801040:
			iFlushCall(0);
			CALLFunc((uptr)sioRead8);
            PUSHR(EAX);
			CALLFunc((uptr)sioRead8);
			POPR(ECX);
			AND32ItoR(ECX, 0xff);
			SHL32ItoR(EAX, 8);
			OR32RtoR(EAX, ECX);
			if( sign ) MOVSX32R16toR(EAX, EAX);
			else MOVZX32R16toR(EAX, EAX);
			return 1;

		case 0x1f801044:
			_eeReadConstMem16(x86reg, (uptr)&sio.StatReg, sign);
			return 0;

		case 0x1f801048:
			_eeReadConstMem16(x86reg, (uptr)&sio.ModeReg, sign);
			return 0;

		case 0x1f80104a:
			_eeReadConstMem16(x86reg, (uptr)&sio.CtrlReg, sign);
			return 0;

		case 0x1f80104e:
			_eeReadConstMem16(x86reg, (uptr)&sio.BaudReg, sign);
			return 0;

		// counters[0]
		case 0x1f801100:
			PUSH32I(0);
			CONSTREAD16_CALL(psxRcntRcount16);
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x1f801104:
			psxConstReadCounterMode16(x86reg, 0, sign);
			return 0;

		case 0x1f801108:
			_eeReadConstMem16(x86reg, (uptr)&psxCounters[0].target, sign);
			return 0;

		// counters[1]
		case 0x1f801110:
			PUSH32I(1);
			CONSTREAD16_CALL(psxRcntRcount16);
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x1f801114:
			psxConstReadCounterMode16(x86reg, 1, sign);
			return 0;

		case 0x1f801118:
			_eeReadConstMem16(x86reg, (uptr)&psxCounters[1].target, sign);
			return 0;

		// counters[2]
		case 0x1f801120:
			PUSH32I(2);
			CONSTREAD16_CALL(psxRcntRcount16);
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x1f801124:
			psxConstReadCounterMode16(x86reg, 2, sign);
			return 0;

		case 0x1f801128:
			_eeReadConstMem16(x86reg, (uptr)&psxCounters[2].target, sign);
			return 0;

		case 0x1f80146e: // DEV9_R_REV
			PUSH32I(add);
			CONSTREAD16_CALL(DEV9read16);
			return 1;

		// counters[3]
		case 0x1f801480:
			PUSH32I(3);
			CONSTREAD16_CALL(psxRcntRcount32);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x1f801484:
			psxConstReadCounterMode16(x86reg, 3, sign);
			return 0;

		case 0x1f801488:
			_eeReadConstMem16(x86reg, (uptr)&psxCounters[3].target, sign);
			return 0;

		// counters[4]
		case 0x1f801490:
			PUSH32I(4);
			CONSTREAD16_CALL(psxRcntRcount32);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x1f801494:
			psxConstReadCounterMode16(x86reg, 4, sign);
			return 0;
			
		case 0x1f801498:
			_eeReadConstMem16(x86reg, (uptr)&psxCounters[4].target, sign);
			return 0;

		// counters[5]
		case 0x1f8014a0:
			PUSH32I(5);
			CONSTREAD16_CALL(psxRcntRcount32);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x1f8014a4:
			psxConstReadCounterMode16(x86reg, 5, sign);
			return 0;

		case 0x1f8014a8:
			_eeReadConstMem16(x86reg, (uptr)&psxCounters[5].target, sign);
			return 0;

		default:			
			if (add>=0x1f801c00 && add<0x1f801e00) {
			
				PUSH32I(add);
				CONSTREAD16_CALL(SPU2read);
				return 1;
			} else {
				_eeReadConstMem16(x86reg, (uptr)&psxH[(add) & 0xffff], sign);
				return 0;
			}
	}
}

void psxConstReadCounterMode32(int x86reg, int index)
{
	if( IS_MMXREG(x86reg) ) {
		MMXONLY(MOV16MtoR(ECX, (uptr)&psxCounters[index].mode);
		MOVDMtoMMX(x86reg&0xf, (uptr)&psxCounters[index].mode);)
	}
	else {
		MOVZX32M16toR(ECX, (uptr)&psxCounters[index].mode);
		MOV32RtoR(x86reg, ECX);
	}

	//AND16ItoR(ECX, ~0x1800);
	//OR16ItoR(ECX, 0x400);
	//MOV16RtoM((uptr)&psxCounters[index].mode, ECX);
}

static u32 s_tempsio;
int psxHwConstRead32(u32 x86reg, u32 add) {
	if (add >= 0x1f801600 && add < 0x1f801700) {
		iFlushCall(0);
		PUSH32I(add);
		CALLFunc((uptr)USBread32);
		return 1;
	}
	if (add >= 0x1f808400 && add <= 0x1f808550) {//the size is a complete guess..
		iFlushCall(0);
		PUSH32I(add);
		CALLFunc((uptr)FWread32);
		return 1;
	}

	switch (add) {
		case 0x1f801040:
			iFlushCall(0);
			CALLFunc((uptr)sioRead8);
			AND32ItoR(EAX, 0xff);
			MOV32RtoM((uptr)&s_tempsio, EAX);
			CALLFunc((uptr)sioRead8);
			AND32ItoR(EAX, 0xff);
			SHL32ItoR(EAX, 8);
			OR32RtoM((uptr)&s_tempsio, EAX);

			// 3rd
			CALLFunc((uptr)sioRead8);
			AND32ItoR(EAX, 0xff);
			SHL32ItoR(EAX, 16);
			OR32RtoM((uptr)&s_tempsio, EAX);

			// 4th
			CALLFunc((uptr)sioRead8);
			SHL32ItoR(EAX, 24);
			OR32MtoR(EAX, (uptr)&s_tempsio);
			return 1;
			
		//case 0x1f801050: hard = serial_read32(); break;//serial port
		case 0x1f801078:
			PSXHW_LOG("ICTRL 32bit read %x\n", psxHu32(0x1078));
			_eeReadConstMem32(x86reg, (uptr)&psxH[add&0xffff]);
			MOV32ItoM((uptr)&psxH[add&0xffff], 0);
			return 0;
		
			// counters[0]
		case 0x1f801100:
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((uptr)psxRcntRcount16);
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x1f801104:
			psxConstReadCounterMode32(x86reg, 0);
			return 0;

		case 0x1f801108:
			_eeReadConstMem32(x86reg, (uptr)&psxCounters[0].target);
			return 0;

		// counters[1]
		case 0x1f801110:
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((uptr)psxRcntRcount16);
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x1f801114:
			psxConstReadCounterMode32(x86reg, 1);
			return 0;

		case 0x1f801118:
			_eeReadConstMem32(x86reg, (uptr)&psxCounters[1].target);
			return 0;

		// counters[2]
		case 0x1f801120:
			iFlushCall(0);
			PUSH32I(2);
			CALLFunc((uptr)psxRcntRcount16);
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x1f801124:
			psxConstReadCounterMode32(x86reg, 2);
			return 0;

		case 0x1f801128:
			_eeReadConstMem32(x86reg, (uptr)&psxCounters[2].target);
			return 0;

		// counters[3]
		case 0x1f801480:
			iFlushCall(0);
			PUSH32I(3);
			CALLFunc((uptr)psxRcntRcount32);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x1f801484:
			psxConstReadCounterMode32(x86reg, 3);
			return 0;

		case 0x1f801488:
			_eeReadConstMem32(x86reg, (uptr)&psxCounters[3].target);
			return 0;

		// counters[4]
		case 0x1f801490:
			iFlushCall(0);
			PUSH32I(4);
			CALLFunc((uptr)psxRcntRcount32);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x1f801494:
			psxConstReadCounterMode32(x86reg, 4);
			return 0;
			
		case 0x1f801498:
			_eeReadConstMem32(x86reg, (uptr)&psxCounters[4].target);
			return 0;

		// counters[5]
		case 0x1f8014a0:
			iFlushCall(0);
			PUSH32I(5);
			CALLFunc((uptr)psxRcntRcount32);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x1f8014a4:
			psxConstReadCounterMode32(x86reg, 5);
			return 0;

		case 0x1f8014a8:
			_eeReadConstMem32(x86reg, (uptr)&psxCounters[5].target);
			return 0;

		case 0x1F808200:
		case 0x1F808204:
		case 0x1F808208:
		case 0x1F80820C:
		case 0x1F808210:
		case 0x1F808214:
		case 0x1F808218:
		case 0x1F80821C:
		case 0x1F808220:
		case 0x1F808224:
		case 0x1F808228:
		case 0x1F80822C:
		case 0x1F808230:
		case 0x1F808234:
		case 0x1F808238:
		case 0x1F80823C:
			iFlushCall(0);
			PUSH32I((add-0x1F808200)/4);
			CALLFunc((uptr)sio2_getSend3);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x1F808240:
		case 0x1F808248:
		case 0x1F808250:
		case 0x1F80825C:
			iFlushCall(0);
			PUSH32I((add-0x1F808240)/8);
			CALLFunc((uptr)sio2_getSend1);
			ADD32ItoR(ESP, 4);
			return 1;
		
		case 0x1F808244:
		case 0x1F80824C:
		case 0x1F808254:
		case 0x1F808258:
			iFlushCall(0);
			PUSH32I((add-0x1F808244)/8);
			CALLFunc((uptr)sio2_getSend2);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x1F808268:
			iFlushCall(0);
			CALLFunc((uptr)sio2_getCtrl);
			return 1;
			
		case 0x1F80826C:
			iFlushCall(0);
			CALLFunc((uptr)sio2_getRecv1);
			return 1;

		case 0x1F808270:
			iFlushCall(0);
			CALLFunc((uptr)sio2_getRecv2);
			return 1;

		case 0x1F808274:
			iFlushCall(0);
			CALLFunc((uptr)sio2_getRecv3);
			return 1;

		case 0x1F808278:
			iFlushCall(0);
			CALLFunc((uptr)sio2_get8278);
			return 1;

		case 0x1F80827C:
			iFlushCall(0);
			CALLFunc((uptr)sio2_get827C);
			return 1;

		case 0x1F808280:
			iFlushCall(0);
			CALLFunc((uptr)sio2_getIntr);
			return 1;

		case 0x1F801C00:
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((uptr)SPU2ReadMemAddr);
			return 1;

		case 0x1F801500:
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((uptr)SPU2ReadMemAddr);
			return 1;

		default:
			_eeReadConstMem32(x86reg, (uptr)&psxH[(add) & 0xffff]);
			return 0;
	}
}

#define CONSTWRITE_CALL(name) { \
	_recPushReg(mmreg); \
	iFlushCall(0); \
	CALLFunc((uptr)name); \
	ADD32ItoR(ESP, 4); \
} \

void Write8PrintBuffer(u8 value)
{
	if (value == '\r') return;
	if (value == '\n' || g_pbufi >= 1023) {
		g_pbuf[g_pbufi++] = 0; g_pbufi = 0;
		SysPrintf("%s\n", g_pbuf); return;
	}
	g_pbuf[g_pbufi++] = value;
}

void psxHwConstWrite8(u32 add, int mmreg)
{
	if (add >= 0x1f801600 && add < 0x1f801700) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(add);
		CALLFunc((uptr)USBwrite8);
		return;
	}

	switch (add) {
		case 0x1f801040:
			CONSTWRITE_CALL(sioWrite8); break;
		//case 0x1f801050: serial_write8(value); break;//serial port
		case 0x1f801100:
		case 0x1f801104:
		case 0x1f801108:
		case 0x1f801110:
		case 0x1f801114:
		case 0x1f801118:
		case 0x1f801120:
		case 0x1f801124:
		case 0x1f801128:
		case 0x1f801480:
		case 0x1f801484:
		case 0x1f801488:
		case 0x1f801490:
		case 0x1f801494:
		case 0x1f801498:
		case 0x1f8014a0:
		case 0x1f8014a4:
		case 0x1f8014a8:
			SysPrintf("8bit counter write %x\n", add);
			_eeWriteConstMem8((uptr)&psxH[(add) & 0xffff], mmreg);
			return;
		case 0x1f801800: CONSTWRITE_CALL(cdrWrite0); break;
		case 0x1f801801: CONSTWRITE_CALL(cdrWrite1); break;
		case 0x1f801802: CONSTWRITE_CALL(cdrWrite2); break;
		case 0x1f801803: CONSTWRITE_CALL(cdrWrite3); break;
		case 0x1f80380c: CONSTWRITE_CALL(Write8PrintBuffer); break;
		case 0x1F808260: CONSTWRITE_CALL(sio2_serialIn); break;

		default:
			_eeWriteConstMem8((uptr)&psxH[(add) & 0xffff], mmreg);
			return;
	}
}

void psxHwConstWrite16(u32 add, int mmreg) {
	if (add >= 0x1f801600 && add < 0x1f801700) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(add);
		CALLFunc((uptr)USBwrite16);
		return;
	}

	switch (add) {
		case 0x1f801040:
			_recPushReg(mmreg);
			iFlushCall(0);
			CALLFunc((uptr)sioWrite8);
			ADD32ItoR(ESP, 1);
			CALLFunc((uptr)sioWrite8);
			ADD32ItoR(ESP, 3);
			return;
		case 0x1f801044:
			return;
		case 0x1f801048:
			_eeWriteConstMem16((uptr)&sio.ModeReg, mmreg);
			return;
		case 0x1f80104a: // control register
			CONSTWRITE_CALL(sioWriteCtrl16);
			return;
		case 0x1f80104e: // baudrate register
			_eeWriteConstMem16((uptr)&sio.BaudReg, mmreg);
			return;

		case 0x1f801070:
			_eeWriteConstMem16OP((uptr)&psxHu32(0x1070), mmreg, 0);		// AND operation
			return;

		case 0x1f801074:
			_eeWriteConstMem16((uptr)&psxHu32(0x1074), mmreg);
			iFlushCall(0);
			CALLFunc( (uptr)&iopTestIntc );
			return;

		case 0x1f801078:
			//According to pSXAuthor this allways becomes 1 on write, but MHPB won't boot if value is not writen ;p
			_eeWriteConstMem16((uptr)&psxHu32(0x1078), mmreg);
			iFlushCall(0);
			CALLFunc( (uptr)&iopTestIntc );
			return;

		// counters[0]
		case 0x1f801100:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((uptr)psxRcntWcount16);
			ADD32ItoR(ESP, 8);
			return;
		case 0x1f801104:
			CONSTWRITE_CALL(psxRcnt0Wmode);
			return;
		case 0x1f801108:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((uptr)psxRcntWtarget16);
			ADD32ItoR(ESP, 8);
			return;

		// counters[1]
		case 0x1f801110:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((uptr)psxRcntWcount16);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f801114:
			CONSTWRITE_CALL(psxRcnt1Wmode);
			return;

		case 0x1f801118:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((uptr)psxRcntWtarget16);
			ADD32ItoR(ESP, 8);
			return;

		// counters[2]
		case 0x1f801120:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(2);
			CALLFunc((uptr)psxRcntWcount16);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f801124:
			CONSTWRITE_CALL(psxRcnt2Wmode);
			return;

		case 0x1f801128:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(2);
			CALLFunc((uptr)psxRcntWtarget16);
			ADD32ItoR(ESP, 8);
			return;

		// counters[3]
		case 0x1f801480:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(3);
			CALLFunc((uptr)psxRcntWcount32);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f801484:
			CONSTWRITE_CALL(psxRcnt3Wmode);
			return;

		case 0x1f801488:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(3);
			CALLFunc((uptr)psxRcntWtarget32);
			ADD32ItoR(ESP, 8);
			return;

		// counters[4]
		case 0x1f801490:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(4);
			CALLFunc((uptr)psxRcntWcount32);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f801494:
			CONSTWRITE_CALL(psxRcnt4Wmode);
			return;

		case 0x1f801498:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(4);
			CALLFunc((uptr)psxRcntWtarget32);
			ADD32ItoR(ESP, 8);
			return;

		// counters[5]
		case 0x1f8014a0:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(5);
			CALLFunc((uptr)psxRcntWcount32);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f8014a4:
			CONSTWRITE_CALL(psxRcnt5Wmode);
			return;

		case 0x1f8014a8:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(5);
			CALLFunc((uptr)psxRcntWtarget32);
			ADD32ItoR(ESP, 8);
			return;

		default:
			if (add>=0x1f801c00 && add<0x1f801e00) {
				_recPushReg(mmreg);
				iFlushCall(0);
				PUSH32I(add);
            	CALLFunc((uptr)SPU2write);
				// leave esp alone
				return;
			}

			_eeWriteConstMem16((uptr)&psxH[(add) & 0xffff], mmreg);
			return;
	}
}

#define recDmaExec(n) { \
	iFlushCall(0); \
	if( n > 6 ) TEST32ItoM((uptr)&HW_DMA_PCR2, 8 << (((n<<2)-28)&0x1f)); \
	else 		TEST32ItoM((uptr)&HW_DMA_PCR,  8 << (((n<<2))&0x1f)); \
	j8Ptr[5] = JZ8(0); \
	MOV32MtoR(EAX, (uptr)&HW_DMA##n##_CHCR); \
	TEST32ItoR(EAX, 0x01000000); \
	j8Ptr[6] = JZ8(0); \
	\
    _callFunctionArg3((uptr)psxDma##n, MEM_MEMORYTAG, MEM_MEMORYTAG, MEM_X86TAG, (uptr)&HW_DMA##n##_MADR, (uptr)&HW_DMA##n##_BCR, EAX); \
	\
	x86SetJ8( j8Ptr[5] ); \
	x86SetJ8( j8Ptr[6] ); \
} \

#define CONSTWRITE_CALL32(name) { \
	iFlushCall(0); \
	_recPushReg(mmreg); \
	CALLFunc((uptr)name); \
	ADD32ItoR(ESP, 4); \
} \

void psxHwConstWrite32(u32 add, int mmreg)
{
	if (add >= 0x1f801600 && add < 0x1f801700) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(add);
		CALLFunc((uptr)USBwrite32);
		return;
	}
	if (add >= 0x1f808400 && add <= 0x1f808550) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(add);
		CALLFunc((uptr)FWwrite32);
		return;
	}

	switch (add) {
	    case 0x1f801040:
			_recPushReg(mmreg);
			iFlushCall(0);
			CALLFunc((uptr)sioWrite8);
			ADD32ItoR(ESP, 1);
			CALLFunc((uptr)sioWrite8);
			ADD32ItoR(ESP, 1);
			CALLFunc((uptr)sioWrite8);
			ADD32ItoR(ESP, 1);
			CALLFunc((uptr)sioWrite8);
			ADD32ItoR(ESP, 1);
			return;

		case 0x1f801070:
			_eeWriteConstMem32OP((uptr)&psxHu32(0x1070), mmreg, 0); // and
			return;

		case 0x1f801074:
			_eeWriteConstMem32((uptr)&psxHu32(0x1074), mmreg);
			iFlushCall(0);
			CALLFunc( (uptr)&iopTestIntc );
			return;

		case 0x1f801078:
			//According to pSXAuthor this allways becomes 1 on write, but MHPB won't boot if value is not writen ;p
			_eeWriteConstMem32((uptr)&psxHu32(0x1078), mmreg);
			iFlushCall(0);
			CALLFunc( (uptr)&iopTestIntc );
			return;

//		case 0x1f801088:
//			HW_DMA0_CHCR = value;        // DMA0 chcr (MDEC in DMA)
////			DmaExec(0);
//			return;

//		case 0x1f801098:
//			HW_DMA1_CHCR = value;        // DMA1 chcr (MDEC out DMA)
////			DmaExec(1);
//			return;
		
		case 0x1f8010a8:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(2);
			return;

		case 0x1f8010b8:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(3);
			return;

		case 0x1f8010c8:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(4);
			return;

		case 0x1f8010e8:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(6);
			return;

		case 0x1f801508:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(7);
			return;

		case 0x1f801518:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(8);
			return;

		case 0x1f801528:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(9);
			return;

		case 0x1f801538:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(10);
			return;

		case 0x1f801548:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(11);
			return;

		case 0x1f801558:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			recDmaExec(12);
			return;

		case 0x1f8010f4:
		case 0x1f801574:
		{
			// u32 tmp = (~value) & HW_DMA_ICR;
			_eeMoveMMREGtoR(EAX, mmreg);
			MOV32RtoR(ECX, EAX);
			NOT32R(ECX);
			AND32MtoR(ECX, (uptr)&psxH[(add) & 0xffff]);

			// HW_DMA_ICR = ((tmp ^ value) & 0xffffff) ^ tmp;
			XOR32RtoR(EAX, ECX);
			AND32ItoR(EAX, 0xffffff);
			XOR32RtoR(EAX, ECX);
			MOV32RtoM((uptr)&psxH[(add) & 0xffff], EAX);
			return;
		}

		// counters[0]
		case 0x1f801100:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((uptr)psxRcntWcount16);
			ADD32ItoR(ESP, 8);
			return;
		case 0x1f801104:
			CONSTWRITE_CALL32(psxRcnt0Wmode);
			return;
		case 0x1f801108:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((uptr)psxRcntWtarget16);
			ADD32ItoR(ESP, 8);
			return;

		// counters[1]
		case 0x1f801110:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((uptr)psxRcntWcount16);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f801114:
			CONSTWRITE_CALL32(psxRcnt1Wmode);
			return;

		case 0x1f801118:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((uptr)psxRcntWtarget16);
			ADD32ItoR(ESP, 8);
			return;

		// counters[2]
		case 0x1f801120:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(2);
			CALLFunc((uptr)psxRcntWcount16);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f801124:
			CONSTWRITE_CALL32(psxRcnt2Wmode);
			return;

		case 0x1f801128:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(2);
			CALLFunc((uptr)psxRcntWtarget16);
			ADD32ItoR(ESP, 8);
			return;

		// counters[3]
		case 0x1f801480:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(3);
			CALLFunc((uptr)psxRcntWcount32);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f801484:
			CONSTWRITE_CALL32(psxRcnt3Wmode);
			return;

		case 0x1f801488:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(3);
			CALLFunc((uptr)psxRcntWtarget32);
			ADD32ItoR(ESP, 8);
			return;

		// counters[4]
		case 0x1f801490:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(4);
			CALLFunc((uptr)psxRcntWcount32);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f801494:
			CONSTWRITE_CALL32(psxRcnt4Wmode);
			return;

		case 0x1f801498:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(4);
			CALLFunc((uptr)psxRcntWtarget32);
			ADD32ItoR(ESP, 8);
			return;

		// counters[5]
		case 0x1f8014a0:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(5);
			CALLFunc((uptr)psxRcntWcount32);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f8014a4:
			CONSTWRITE_CALL32(psxRcnt5Wmode);
			return;

		case 0x1f8014a8:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(5);
			CALLFunc((uptr)psxRcntWtarget32);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1f8014c0:
			SysPrintf("RTC_HOLDMODE 32bit write\n");
			break;

		case 0x1F808200:
		case 0x1F808204:
		case 0x1F808208:
		case 0x1F80820C:
		case 0x1F808210:
		case 0x1F808214:
		case 0x1F808218:
		case 0x1F80821C:
		case 0x1F808220:
		case 0x1F808224:
		case 0x1F808228:
		case 0x1F80822C:
		case 0x1F808230:
		case 0x1F808234:
		case 0x1F808238:
		case 0x1F80823C:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I((add-0x1F808200)/4);
			CALLFunc((uptr)sio2_setSend3);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1F808240:
		case 0x1F808248:
		case 0x1F808250:
		case 0x1F808258:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I((add-0x1F808240)/8);
			CALLFunc((uptr)sio2_setSend1);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1F808244:
		case 0x1F80824C:
		case 0x1F808254:
		case 0x1F80825C:
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I((add-0x1F808244)/8);
			CALLFunc((uptr)sio2_setSend2);
			ADD32ItoR(ESP, 8);
			return;

		case 0x1F808268: CONSTWRITE_CALL32(sio2_setCtrl); return;
		case 0x1F808278: CONSTWRITE_CALL32(sio2_set8278);	return;
		case 0x1F80827C: CONSTWRITE_CALL32(sio2_set827C);	return;
		case 0x1F808280: CONSTWRITE_CALL32(sio2_setIntr);	return;
		
		case 0x1F8010C0:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((uptr)SPU2WriteMemAddr);
			return;

		case 0x1F801500:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			_recPushReg(mmreg);
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((uptr)SPU2WriteMemAddr);
			return;
		default:
			_eeWriteConstMem32((uptr)&psxH[(add) & 0xffff], mmreg);
			return;
	}
}

int psxHw4ConstRead8(u32 x86reg, u32 add, u32 sign) {
	switch (add) {
		case 0x1f402004: CONSTREAD8_CALL((uptr)cdvdRead04); return 1;
		case 0x1f402005: CONSTREAD8_CALL((uptr)cdvdRead05); return 1;
		case 0x1f402006: CONSTREAD8_CALL((uptr)cdvdRead06); return 1;
		case 0x1f402007: CONSTREAD8_CALL((uptr)cdvdRead07); return 1;
		case 0x1f402008: CONSTREAD8_CALL((uptr)cdvdRead08); return 1;
		case 0x1f40200A: CONSTREAD8_CALL((uptr)cdvdRead0A); return 1;
		case 0x1f40200B: CONSTREAD8_CALL((uptr)cdvdRead0B); return 1;
		case 0x1f40200C: CONSTREAD8_CALL((uptr)cdvdRead0C); return 1;
		case 0x1f40200D: CONSTREAD8_CALL((uptr)cdvdRead0D); return 1;
		case 0x1f40200E: CONSTREAD8_CALL((uptr)cdvdRead0E); return 1;
		case 0x1f40200F: CONSTREAD8_CALL((uptr)cdvdRead0F); return 1;
		case 0x1f402013: CONSTREAD8_CALL((uptr)cdvdRead13); return 1;
		case 0x1f402015: CONSTREAD8_CALL((uptr)cdvdRead15); return 1;
		case 0x1f402016: CONSTREAD8_CALL((uptr)cdvdRead16); return 1;
		case 0x1f402017: CONSTREAD8_CALL((uptr)cdvdRead17); return 1;
		case 0x1f402018: CONSTREAD8_CALL((uptr)cdvdRead18); return 1;
		case 0x1f402020: CONSTREAD8_CALL((uptr)cdvdRead20); return 1;
		case 0x1f402021: CONSTREAD8_CALL((uptr)cdvdRead21); return 1;
		case 0x1f402022: CONSTREAD8_CALL((uptr)cdvdRead22); return 1;
		case 0x1f402023: CONSTREAD8_CALL((uptr)cdvdRead23); return 1;
		case 0x1f402024: CONSTREAD8_CALL((uptr)cdvdRead24); return 1;
		case 0x1f402028: CONSTREAD8_CALL((uptr)cdvdRead28); return 1;
		case 0x1f402029: CONSTREAD8_CALL((uptr)cdvdRead29); return 1;
		case 0x1f40202A: CONSTREAD8_CALL((uptr)cdvdRead2A); return 1;
		case 0x1f40202B: CONSTREAD8_CALL((uptr)cdvdRead2B); return 1;
		case 0x1f40202C: CONSTREAD8_CALL((uptr)cdvdRead2C); return 1;
		case 0x1f402030: CONSTREAD8_CALL((uptr)cdvdRead30); return 1;
		case 0x1f402031: CONSTREAD8_CALL((uptr)cdvdRead31); return 1;
		case 0x1f402032: CONSTREAD8_CALL((uptr)cdvdRead32); return 1;
		case 0x1f402033: CONSTREAD8_CALL((uptr)cdvdRead33); return 1;
		case 0x1f402034: CONSTREAD8_CALL((uptr)cdvdRead34); return 1;
		case 0x1f402038: CONSTREAD8_CALL((uptr)cdvdRead38); return 1;
		case 0x1f402039: CONSTREAD8_CALL((uptr)cdvdRead39); return 1;
		case 0x1f40203A: CONSTREAD8_CALL((uptr)cdvdRead3A); return 1;
		default:
			Console::Notice("*Unknown 8bit read at address %lx", params add);
			XOR32RtoR(x86reg, x86reg);
			return 0;
	}
}

void psxHw4ConstWrite8(u32 add, int mmreg) {
	switch (add) {
		case 0x1f402004: CONSTWRITE_CALL(cdvdWrite04); return;
		case 0x1f402005: CONSTWRITE_CALL(cdvdWrite05); return;
		case 0x1f402006: CONSTWRITE_CALL(cdvdWrite06); return;
		case 0x1f402007: CONSTWRITE_CALL(cdvdWrite07); return;
		case 0x1f402008: CONSTWRITE_CALL(cdvdWrite08); return;
		case 0x1f40200A: CONSTWRITE_CALL(cdvdWrite0A); return;
		case 0x1f40200F: CONSTWRITE_CALL(cdvdWrite0F); return;
		case 0x1f402014: CONSTWRITE_CALL(cdvdWrite14); return;
		case 0x1f402016: 
			MMXONLY(_freeMMXregs();)
			CONSTWRITE_CALL(cdvdWrite16);
			return;
		case 0x1f402017: CONSTWRITE_CALL(cdvdWrite17); return;
		case 0x1f402018: CONSTWRITE_CALL(cdvdWrite18); return;
		case 0x1f40203A: CONSTWRITE_CALL(cdvdWrite3A); return;
		default:
			Console::Notice("*Unknown 8bit write at address %lx", params add);
			return;
	}
}
