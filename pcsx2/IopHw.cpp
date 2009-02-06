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

#include "PsxCommon.h"
#include "Misc.h"
#include "iR5900.h"

// NOTE: Any modifications to read/write fns should also go into their const counterparts
// found in iPsxHw.cpp.

void psxHwReset() {
/*	if (Config.Sio) psxHu32(0x1070) |= 0x80;
	if (Config.SpuIrq) psxHu32(0x1070) |= 0x200;*/

	memzero_ptr<0x10000>(psxH);

//	mdecInit(); //intialize mdec decoder
	cdrReset();
	cdvdReset();
	psxRcntInit();
	sioInit();
//	sio2Reset();
}

u8 psxHwRead8(u32 add) {
	u8 hard;

	if (add >= 0x1f801600 && add < 0x1f801700) {
		return USBread8(add);
	}

	switch (add) {
		case 0x1f801040: hard = sioRead8();break; 
      //  case 0x1f801050: hard = serial_read8(); break;//for use of serial port ignore for now

		case 0x1f80146e: // DEV9_R_REV
			return DEV9read8(add);

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
			hard = psxHu8(add);
			return hard;
#endif

		case 0x1f801800: hard = cdrRead0(); break;
		case 0x1f801801: hard = cdrRead1(); break;
		case 0x1f801802: hard = cdrRead2(); break;
		case 0x1f801803: hard = cdrRead3(); break;

		case 0x1f803100: // PS/EE/IOP conf related
			hard = 0x10; // Dram 2M
			break;

		case 0x1F808264:
			hard = sio2_fifoOut();//sio2 serial data feed/fifo_out
			PSXHW_LOG("SIO2 read8 DATAOUT %08X\n", hard);
			return hard;

		default:
			hard = psxHu8(add); 
			PSXHW_LOG("*Unknown 8bit read at address %lx\n", add);
			return hard;
	}
	
	PSXHW_LOG("*Known 8bit read at address %lx value %x\n", add, hard);
	return hard;
}

u16 psxHwRead16(u32 add) {
	u16 hard;

	if (add >= 0x1f801600 && add < 0x1f801700) {
		return USBread16(add);
	}

	switch (add) {

		case 0x1f801070: PSXHW_LOG("IREG 16bit read %x\n", psxHu16(0x1070));
			return psxHu16(0x1070);
		case 0x1f801074: PSXHW_LOG("IMASK 16bit read %x\n", psxHu16(0x1074));
			return psxHu16(0x1074);

		case 0x1f801040:
			hard = sioRead8();
			hard|= sioRead8() << 8;
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
			return hard;
		case 0x1f801044:
			hard = sio.StatReg;
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
			return hard;
		case 0x1f801048:
			hard = sio.ModeReg;
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
			return hard;
		case 0x1f80104a:
			hard = sio.CtrlReg;
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
			return hard;
		case 0x1f80104e:
			hard = sio.BaudReg;
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
			return hard;

		//Serial port stuff not support now ;P
	 // case 0x1f801050: hard = serial_read16(); break;
	 //	case 0x1f801054: hard = serial_status_read(); break;
	 //	case 0x1f80105a: hard = serial_control_read(); break;
	 //	case 0x1f80105e: hard = serial_baud_read(); break;
	
		case 0x1f801100:
			hard = (u16)psxRcntRcount16(0);
			PSXCNT_LOG("T0 count read16: %x\n", hard);
			return hard;
		case 0x1f801104:
			hard = psxCounters[0].mode;
            psxCounters[0].mode &= ~0x1800;
			psxCounters[0].mode |= 0x400;
			PSXCNT_LOG("T0 mode read16: %x\n", hard);
			return hard;
		case 0x1f801108:
			hard = psxCounters[0].target;
			PSXCNT_LOG("T0 target read16: %x\n", hard);
			return hard;
		case 0x1f801110:
			hard = (u16)psxRcntRcount16(1);
			PSXCNT_LOG("T1 count read16: %x\n", hard);
			return hard;
		case 0x1f801114:
			hard = psxCounters[1].mode;
			psxCounters[1].mode &= ~0x1800;
			psxCounters[1].mode |= 0x400;
			PSXCNT_LOG("T1 mode read16: %x\n", hard);
			return hard;
		case 0x1f801118:
			hard = psxCounters[1].target;
			PSXCNT_LOG("T1 target read16: %x\n", hard);
			return hard;
		case 0x1f801120:
			hard = (u16)psxRcntRcount16(2);
			PSXCNT_LOG("T2 count read16: %x\n", hard);
			return hard;
		case 0x1f801124:
			hard = psxCounters[2].mode;
			psxCounters[2].mode &= ~0x1800;
			psxCounters[2].mode |= 0x400;
			PSXCNT_LOG("T2 mode read16: %x\n", hard);
			return hard;
		case 0x1f801128:
			hard = psxCounters[2].target;
			PSXCNT_LOG("T2 target read16: %x\n", hard);
			return hard;

		case 0x1f80146e: // DEV9_R_REV
			return DEV9read16(add);

		case 0x1f801480:
			hard = (u16)psxRcntRcount32(3);
			PSXCNT_LOG("T3 count read16: %lx\n", hard);
			return hard;
		case 0x1f801484:
			hard = psxCounters[3].mode;
			psxCounters[3].mode &= ~0x1800;
			psxCounters[3].mode |= 0x400;
			PSXCNT_LOG("T3 mode read16: %lx\n", hard);
			return hard;
		case 0x1f801488:
			hard = psxCounters[3].target;
			PSXCNT_LOG("T3 target read16: %lx\n", hard);
			return hard;
		case 0x1f801490:
			hard = (u16)psxRcntRcount32(4);
			PSXCNT_LOG("T4 count read16: %lx\n", hard);
			return hard;
		case 0x1f801494:
			hard = psxCounters[4].mode;
			psxCounters[4].mode &= ~0x1800;
			psxCounters[4].mode |= 0x400;
			PSXCNT_LOG("T4 mode read16: %lx\n", hard);
			return hard;
		case 0x1f801498:
			hard = psxCounters[4].target;
			PSXCNT_LOG("T4 target read16: %lx\n", hard);
			return hard;
		case 0x1f8014a0:
			hard = (u16)psxRcntRcount32(5);
			PSXCNT_LOG("T5 count read16: %lx\n", hard);
			return hard;
		case 0x1f8014a4:
			hard = psxCounters[5].mode;
			psxCounters[5].mode &= ~0x1800;
			psxCounters[5].mode |= 0x400;
			PSXCNT_LOG("T5 mode read16: %lx\n", hard);
			return hard;
		case 0x1f8014a8:
			hard = psxCounters[5].target;
			PSXCNT_LOG("T5 target read16: %lx\n", hard);
			return hard;

		case 0x1f801504:
			hard = psxHu16(0x1504);
			PSXHW_LOG("DMA7 BCR_size 16bit read %lx\n", hard);
			return hard;
		case 0x1f801506:
			hard = psxHu16(0x1506);
			PSXHW_LOG("DMA7 BCR_count 16bit read %lx\n", hard);
			return hard;
		//case 0x1f802030: hard =   //int_2000????
		//case 0x1f802040: hard =//dip switches...??

		default:
			if (add>=0x1f801c00 && add<0x1f801e00) {
            	hard = SPU2read(add);
			} else {
				hard = psxHu16(add); 
				PSXHW_LOG("*Unknown 16bit read at address %lx\n", add);
			}
            return hard;
	}
	

	PSXHW_LOG("*Known 16bit read at address %lx value %x\n", add, hard);
	return hard;
}

u32 psxHwRead32(u32 add) {
	u32 hard;

	if (add >= 0x1f801600 && add < 0x1f801700) {
		return USBread32(add);
	}
	if (add >= 0x1f808400 && add <= 0x1f808550) {//the size is a complete guess..
		return FWread32(add);
	}

	switch (add) {
		case 0x1f801040:
			hard = sioRead8();
			hard|= sioRead8() << 8;
			hard|= sioRead8() << 16;
			hard|= sioRead8() << 24;
			PAD_LOG("sio read32 ;ret = %lx\n", hard);
			return hard;
			
	//	case 0x1f801050: hard = serial_read32(); break;//serial port
		case 0x1f801060:
			PSXHW_LOG("RAM size read %lx\n", psxHu32(0x1060));
			return psxHu32(0x1060);
		case 0x1f801070: PSXHW_LOG("IREG 32bit read %x\n", psxHu32(0x1070));
			return psxHu32(0x1070);
		case 0x1f801074: PSXHW_LOG("IMASK 32bit read %x\n", psxHu32(0x1074));
			return psxHu32(0x1074);
		case 0x1f801078:
			PSXHW_LOG("ICTRL 32bit read %x\n", psxHu32(0x1078));
			hard = psxHu32(0x1078);
			psxHu32(0x1078) = 0;
			return hard;

/*		case 0x1f801810:
//			hard = GPU_readData();
			PSXHW_LOG("GPU DATA 32bit read %lx\n", hard);
			return hard;*/
/*		case 0x1f801814:
			hard = GPU_readStatus();
			PSXHW_LOG("GPU STATUS 32bit read %lx\n", hard);
			return hard;
*/
/*		case 0x1f801820: hard = mdecRead0(); break;
		case 0x1f801824: hard = mdecRead1(); break;
*/

		case 0x1f8010a0:
			PSXHW_LOG("DMA2 MADR 32bit read %lx\n", psxHu32(0x10a0));
			return HW_DMA2_MADR;
		case 0x1f8010a4:
			PSXHW_LOG("DMA2 BCR 32bit read %lx\n", psxHu32(0x10a4));
			return HW_DMA2_BCR;
		case 0x1f8010a8:
			PSXHW_LOG("DMA2 CHCR 32bit read %lx\n", psxHu32(0x10a8));
			return HW_DMA2_CHCR;

		case 0x1f8010b0:
			PSXHW_LOG("DMA3 MADR 32bit read %lx\n", psxHu32(0x10b0));
			return HW_DMA3_MADR;
		case 0x1f8010b4:
			PSXHW_LOG("DMA3 BCR 32bit read %lx\n", psxHu32(0x10b4));
			return HW_DMA3_BCR;
		case 0x1f8010b8:
			PSXHW_LOG("DMA3 CHCR 32bit read %lx\n", psxHu32(0x10b8));
			return HW_DMA3_CHCR;

		case 0x1f801520:
			PSXHW_LOG("DMA9 MADR 32bit read %lx\n", HW_DMA9_MADR);
			return HW_DMA9_MADR;
		case 0x1f801524:
			PSXHW_LOG("DMA9 BCR 32bit read %lx\n", HW_DMA9_BCR);
			return HW_DMA9_BCR;
		case 0x1f801528:
			PSXHW_LOG("DMA9 CHCR 32bit read %lx\n", HW_DMA9_CHCR);
			return HW_DMA9_CHCR;
		case 0x1f80152C:
			PSXHW_LOG("DMA9 TADR 32bit read %lx\n", HW_DMA9_TADR);
			return HW_DMA9_TADR;

		case 0x1f801530:
			PSXHW_LOG("DMA10 MADR 32bit read %lx\n", HW_DMA10_MADR);
			return HW_DMA10_MADR;
		case 0x1f801534:
			PSXHW_LOG("DMA10 BCR 32bit read %lx\n", HW_DMA10_BCR);
			return HW_DMA10_BCR;
		case 0x1f801538:
			PSXHW_LOG("DMA10 CHCR 32bit read %lx\n", HW_DMA10_CHCR);
			return HW_DMA10_CHCR;

//		case 0x1f8010f0:  PSXHW_LOG("DMA PCR 32bit read " << psxHu32(0x10f0));
//			return HW_DMA_PCR; // dma rest channel

		case 0x1f8010f4:
			PSXHW_LOG("DMA ICR 32bit read %lx\n", HW_DMA_ICR);
			return HW_DMA_ICR;

//SSBus registers
		case 0x1f801000:
			hard = psxHu32(0x1000);
			PSXHW_LOG("SSBUS <spd_addr> 32bit read %lx\n", hard);
			return hard;
		case 0x1f801004:
			hard = psxHu32(0x1004);
			PSXHW_LOG("SSBUS <pio_addr> 32bit read %lx\n", hard);
			return hard;
		case 0x1f801008:
			hard = psxHu32(0x1008);
			PSXHW_LOG("SSBUS <spd_delay> 32bit read %lx\n", hard);
			return hard;
		case 0x1f80100C:
			hard = psxHu32(0x100C);
			PSXHW_LOG("SSBUS dev1_delay 32bit read %lx\n", hard);
			return hard;
		case 0x1f801010:
			hard = psxHu32(0x1010);
			PSXHW_LOG("SSBUS rom_delay 32bit read %lx\n", hard);
			return hard;
		case 0x1f801014:
			hard = psxHu32(0x1014);
			PSXHW_LOG("SSBUS spu_delay 32bit read %lx\n", hard);
			return hard;
		case 0x1f801018:
			hard = psxHu32(0x1018);
			PSXHW_LOG("SSBUS dev5_delay 32bit read %lx\n", hard);
			return hard;
		case 0x1f80101C:
			hard = psxHu32(0x101C);
			PSXHW_LOG("SSBUS <pio_delay> 32bit read %lx\n", hard);
			return hard;
		case 0x1f801020:
			hard = psxHu32(0x1020);
			PSXHW_LOG("SSBUS com_delay 32bit read %lx\n", hard);
			return hard;
		case 0x1f801400:
			hard = psxHu32(0x1400);
			PSXHW_LOG("SSBUS dev1_addr 32bit read %lx\n", hard);
			return hard;
		case 0x1f801404:
			hard = psxHu32(0x1404);
			PSXHW_LOG("SSBUS spu_addr 32bit read %lx\n", hard);
			return hard;
		case 0x1f801408:
			hard = psxHu32(0x1408);
			PSXHW_LOG("SSBUS dev5_addr 32bit read %lx\n", hard);
			return hard;
		case 0x1f80140C:
			hard = psxHu32(0x140C);
			PSXHW_LOG("SSBUS spu1_addr 32bit read %lx\n", hard);
			return hard;
		case 0x1f801410:
			hard = psxHu32(0x1410);
			PSXHW_LOG("SSBUS <dev9_addr3> 32bit read %lx\n", hard);
			return hard;
		case 0x1f801414:
			hard = psxHu32(0x1414);
			PSXHW_LOG("SSBUS spu1_delay 32bit read %lx\n", hard);
			return hard;
		case 0x1f801418:
			hard = psxHu32(0x1418);
			PSXHW_LOG("SSBUS <dev9_delay2> 32bit read %lx\n", hard);
			return hard;
		case 0x1f80141C:
			hard = psxHu32(0x141C);
			PSXHW_LOG("SSBUS <dev9_delay3> 32bit read %lx\n", hard);
			return hard;
		case 0x1f801420:
			hard = psxHu32(0x1420);
			PSXHW_LOG("SSBUS <dev9_delay1> 32bit read %lx\n", hard);
			return hard;

		case 0x1f8010f0:
			PSXHW_LOG("DMA PCR 32bit read %lx\n", HW_DMA_PCR);
			return HW_DMA_PCR;

		case 0x1f8010c8:
			PSXHW_LOG("DMA4 CHCR 32bit read %lx\n", HW_DMA4_CHCR);
		 return HW_DMA4_CHCR;       // DMA4 chcr (SPU DMA)
			
		// time for rootcounters :)
		case 0x1f801100:
			hard = (u16)psxRcntRcount16(0);
			PSXCNT_LOG("T0 count read32: %lx\n", hard);
			return hard;
		case 0x1f801104:
			hard = (u16)psxCounters[0].mode;
			PSXCNT_LOG("T0 mode read32: %lx\n", hard);
			return hard;
		case 0x1f801108:
			hard = psxCounters[0].target;
			PSXCNT_LOG("T0 target read32: %lx\n", hard);
			return hard;
		case 0x1f801110:
			hard = (u16)psxRcntRcount16(1);
			PSXCNT_LOG("T1 count read32: %lx\n", hard);
			return hard;
		case 0x1f801114:
			hard = (u16)psxCounters[1].mode;
			PSXCNT_LOG("T1 mode read32: %lx\n", hard);
			return hard;
		case 0x1f801118:
			hard = psxCounters[1].target;
			PSXCNT_LOG("T1 target read32: %lx\n", hard);
			return hard;
		case 0x1f801120:
			hard = (u16)psxRcntRcount16(2);
			PSXCNT_LOG("T2 count read32: %lx\n", hard);
			return hard;
		case 0x1f801124:
			hard = (u16)psxCounters[2].mode;
			PSXCNT_LOG("T2 mode read32: %lx\n", hard);
			return hard;
		case 0x1f801128:
			hard = psxCounters[2].target;
			PSXCNT_LOG("T2 target read32: %lx\n", hard);
			return hard;

		case 0x1f801480:
			hard = (u32)psxRcntRcount32(3);
			PSXCNT_LOG("T3 count read32: %lx\n", hard);
			return hard;
		case 0x1f801484:
			hard = (u16)psxCounters[3].mode;
			PSXCNT_LOG("T3 mode read32: %lx\n", hard);
			return hard;
		case 0x1f801488:
			hard = psxCounters[3].target;
			PSXCNT_LOG("T3 target read32: %lx\n", hard);
			return hard;
		case 0x1f801490:
			hard = (u32)psxRcntRcount32(4);
			PSXCNT_LOG("T4 count read32: %lx\n", hard);
			return hard;
		case 0x1f801494:
			hard = (u16)psxCounters[4].mode;
			PSXCNT_LOG("T4 mode read32: %lx\n", hard);
			return hard;
		case 0x1f801498:
			hard = psxCounters[4].target;
			PSXCNT_LOG("T4 target read32: %lx\n", hard);
			return hard;
		case 0x1f8014a0:
			hard = (u32)psxRcntRcount32(5);
			PSXCNT_LOG("T5 count read32: %lx\n", hard);
			return hard;
		case 0x1f8014a4:
			hard = (u16)psxCounters[5].mode;
			PSXCNT_LOG("T5 mode read32: %lx\n", hard);
			return hard;
		case 0x1f8014a8:
			hard = psxCounters[5].target;
			PSXCNT_LOG("T5 target read32: %lx\n", hard);
			return hard;

		case 0x1f801450:
			hard = psxHu32(add);
			PSXHW_LOG("%08X ICFG 32bit read %x\n", psxRegs.pc, hard);
			return hard;


		case 0x1F8010C0:
			HW_DMA4_MADR = SPU2ReadMemAddr(0);
			return HW_DMA4_MADR;

		case 0x1f801500:
			HW_DMA7_MADR = SPU2ReadMemAddr(1);
			PSXHW_LOG("DMA7 MADR 32bit read %lx\n", HW_DMA7_MADR);
			return HW_DMA7_MADR;  // DMA7 madr
		case 0x1f801504:
			PSXHW_LOG("DMA7 BCR 32bit read %lx\n", HW_DMA7_BCR);
			return HW_DMA7_BCR; // DMA7 bcr

		case 0x1f801508:
			PSXHW_LOG("DMA7 CHCR 32bit read %lx\n", HW_DMA7_CHCR);
			return HW_DMA7_CHCR;         // DMA7 chcr (SPU2)		

		case 0x1f801570:
			hard = psxHu32(0x1570);
			PSXHW_LOG("DMA PCR2 32bit read %lx\n", hard);
			return hard;
		case 0x1f801574:
			PSXHW_LOG("DMA ICR2 32bit read %lx\n", HW_DMA_ICR2);
			return HW_DMA_ICR2;

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
			hard=sio2_getSend3((add-0x1F808200)/4);
			PSXHW_LOG("SIO2 read param[%d] (%lx)\n", (add-0x1F808200)/4, hard);
			return hard;

		case 0x1F808240:
		case 0x1F808248:
		case 0x1F808250:
		case 0x1F80825C:
			hard=sio2_getSend1((add-0x1F808240)/8);
			PSXHW_LOG("SIO2 read send1[%d] (%lx)\n", (add-0x1F808240)/8, hard);
			return hard;
		
		case 0x1F808244:
		case 0x1F80824C:
		case 0x1F808254:
		case 0x1F808258:
			hard=sio2_getSend2((add-0x1F808244)/8);
			PSXHW_LOG("SIO2 read send2[%d] (%lx)\n", (add-0x1F808244)/8, hard);
			return hard;

		case 0x1F808268:
			hard=sio2_getCtrl();
			PSXHW_LOG("SIO2 read CTRL (%lx)\n", hard);
			return hard;

		case 0x1F80826C:
			hard=sio2_getRecv1();
			PSXHW_LOG("SIO2 read Recv1 (%lx)\n", hard);
			return hard;

		case 0x1F808270:
			hard=sio2_getRecv2();
			PSXHW_LOG("SIO2 read Recv2 (%lx)\n", hard);
			return hard;

		case 0x1F808274:
			hard=sio2_getRecv3();
			PSXHW_LOG("SIO2 read Recv3 (%lx)\n", hard);
			return hard;

		case 0x1F808278:
			hard=sio2_get8278();
			PSXHW_LOG("SIO2 read [8278] (%lx)\n", hard);
			return hard;

		case 0x1F80827C:
			hard=sio2_get827C();
			PSXHW_LOG("SIO2 read [827C] (%lx)\n", hard);
			return hard;

		case 0x1F808280:
			hard=sio2_getIntr();
			PSXHW_LOG("SIO2 read INTR (%lx)\n", hard);
			return hard;

		default:
			hard = psxHu32(add); 
			PSXHW_LOG("*Unknown 32bit read at address %lx: %lx\n", add, hard);
			return hard;
	}
	PSXHW_LOG("*Known 32bit read at address %lx: %lx\n", add, hard);
	return hard;
}

int g_pbufi;
s8 g_pbuf[1024];

#define DmaExec(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR & (8 << (n * 4))) { \
		psxDma##n(HW_DMA##n##_MADR, HW_DMA##n##_BCR, HW_DMA##n##_CHCR); \
	} \
}

void psxHwWrite8(u32 add, u8 value) {
	if (add >= 0x1f801600 && add < 0x1f801700) {
		USBwrite8(add, value); return;
	}
	if((add & 0xf) == 0xa) SysPrintf("8bit write (possible chcr set) %x value %x\n", add, value);
	switch (add) {
		case 0x1f801040:
            sioWrite8(value); 
            break;
	//	case 0x1f801050: serial_write8(value); break;//serial port

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
			psxHu8(add) = value;
			return;
		case 0x1f801450:
			if (value) { PSXHW_LOG("%08X ICFG 8bit write %lx\n", psxRegs.pc, value); }
			psxHu8(0x1450) = value;
			return;

		case 0x1f801800: cdrWrite0(value); break;
		case 0x1f801801: cdrWrite1(value); break;
		case 0x1f801802: cdrWrite2(value); break;
		case 0x1f801803: cdrWrite3(value); break;

		case 0x1f80380c:
			if (value == '\r') break;
			if (value == '\n' || g_pbufi >= 1023) {
				g_pbuf[g_pbufi++] = 0; g_pbufi = 0;
				SysPrintf("%s\n", g_pbuf);
			}
			else g_pbuf[g_pbufi++] = value;
            psxHu8(add) = value;
            return;

		case 0x1F808260:
			PSXHW_LOG("SIO2 write8 DATAIN <- %08X\n", value);
			sio2_serialIn(value);return;//serial data feed/fifo

		default:
			psxHu8(add) = value;
			PSXHW_LOG("*Unknown 8bit write at address %lx value %x\n", add, value);
			return;
	}
	psxHu8(add) = value;
	PSXHW_LOG("*Known 8bit write at address %lx value %x\n", add, value);
}

void psxHwWrite16(u32 add, u16 value) {
	if (add >= 0x1f801600 && add < 0x1f801700) {
		USBwrite16(add, value); return;
	}

	if((add & 0xf) == 0x9) DevCon::WriteLn("16bit write (possible chcr set) %x value %x", params add, value);

	switch (add) {
		case 0x1f801040:
			sioWrite8((u8)value);
			sioWrite8((u8)(value>>8));
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
			return;
		case 0x1f801044:
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
			return;
		case 0x1f801048:
			sio.ModeReg = value;
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
			return;
		case 0x1f80104a: // control register
			sioWriteCtrl16(value);
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
			return;
		case 0x1f80104e: // baudrate register
			sio.BaudReg = value;
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
			return;

		//serial port ;P
	//  case 0x1f801050: serial_write16(value); break;
	//	case 0x1f80105a: serial_control_write(value);break;
	//	case 0x1f80105e: serial_baud_write(value); break;
	//	case 0x1f801054: serial_status_write(value); break;

		case 0x1f801070: 
			PSXHW_LOG("IREG 16bit write %x\n", value);
//			if (Config.Sio) psxHu16(0x1070) |= 0x80;
//			if (Config.SpuIrq) psxHu16(0x1070) |= 0x200;
			psxHu16(0x1070) &= value;
			return;

		case 0x1f801074:
			PSXHW_LOG("IMASK 16bit write %x\n", value);
			psxHu16(0x1074) = value;
			iopTestIntc();
			return;

		case 0x1f801078:	// see the 32-bit version for notes!
			PSXHW_LOG("ICTRL 16bit write %x\n", value);
			psxHu16(0x1078) = value;
			iopTestIntc();
			return;

		case 0x1f8010c4:
			PSXHW_LOG("DMA4 BCR_size 16bit write %lx\n", value);
			psxHu16(0x10c4) = value;
			return; // DMA4 bcr_size

		case 0x1f8010c6:
			PSXHW_LOG("DMA4 BCR_count 16bit write %lx\n", value);
			psxHu16(0x10c6) = value; return; // DMA4 bcr_count

		case 0x1f801100:
			PSXCNT_LOG("COUNTER 0 COUNT 16bit write %x\n", value);
			psxRcntWcount16(0, value); return;
		case 0x1f801104:
			PSXCNT_LOG("COUNTER 0 MODE 16bit write %x\n", value);
			psxRcnt0Wmode(value); return;
		case 0x1f801108:
			PSXCNT_LOG("COUNTER 0 TARGET 16bit write %x\n", value);
			psxRcntWtarget16(0, value); return;

		case 0x1f801110:
			PSXCNT_LOG("COUNTER 1 COUNT 16bit write %x\n", value);
			psxRcntWcount16(1, value); return;
		case 0x1f801114:
			PSXCNT_LOG("COUNTER 1 MODE 16bit write %x\n", value);
			psxRcnt1Wmode(value); return;
		case 0x1f801118:
			PSXCNT_LOG("COUNTER 1 TARGET 16bit write %x\n", value);
			psxRcntWtarget16(1, value); return;

		case 0x1f801120:
			PSXCNT_LOG("COUNTER 2 COUNT 16bit write %x\n", value);
			psxRcntWcount16(2, value); return;
		case 0x1f801124:
			PSXCNT_LOG("COUNTER 2 MODE 16bit write %x\n", value);
			psxRcnt2Wmode(value); return;
		case 0x1f801128:
			PSXCNT_LOG("COUNTER 2 TARGET 16bit write %x\n", value);
			psxRcntWtarget16(2, value); return;

		case 0x1f801450:
			if (value) { PSXHW_LOG("%08X ICFG 16bit write %lx\n", psxRegs.pc, value); }
			psxHu16(0x1450) = value/* & (~0x8)*/;
			return;

		case 0x1f801480:
			PSXCNT_LOG("COUNTER 3 COUNT 16bit write %lx\n", value);
			psxRcntWcount32(3, value); return;
		case 0x1f801484:
			PSXCNT_LOG("COUNTER 3 MODE 16bit write %lx\n", value);
			psxRcnt3Wmode(value); return;
		case 0x1f801488:
			PSXCNT_LOG("COUNTER 3 TARGET 16bit write %lx\n", value);
			psxRcntWtarget32(3, value); return;

		case 0x1f801490:
			PSXCNT_LOG("COUNTER 4 COUNT 16bit write %lx\n", value);
			psxRcntWcount32(4, value); return;
		case 0x1f801494:
			PSXCNT_LOG("COUNTER 4 MODE 16bit write %lx\n", value);
			psxRcnt4Wmode(value); return;
		case 0x1f801498:
			PSXCNT_LOG("COUNTER 4 TARGET 16bit write %lx\n", value);
			psxRcntWtarget32(4, value); return;

		case 0x1f8014a0:
			PSXCNT_LOG("COUNTER 5 COUNT 16bit write %lx\n", value);
			psxRcntWcount32(5, value); return;
		case 0x1f8014a4:
			PSXCNT_LOG("COUNTER 5 MODE 16bit write %lx\n", value);
			psxRcnt5Wmode(value); return;
		case 0x1f8014a8:
			PSXCNT_LOG("COUNTER 5 TARGET 16bit write %lx\n", value);
			psxRcntWtarget32(5, value); return;

		case 0x1f801504:
			psxHu16(0x1504) = value;
			PSXHW_LOG("DMA7 BCR_size 16bit write %lx\n", value);
			return;
		case 0x1f801506:
			psxHu16(0x1506) = value;
			PSXHW_LOG("DMA7 BCR_count 16bit write %lx\n", value);
			return;
		default:
			if (add>=0x1f801c00 && add<0x1f801e00) {
				SPU2write(add, value);
				return;
			}

			psxHu16(add) = value;
			PSXHW_LOG("*Unknown 16bit write at address %lx value %x\n", add, value);
			return;
	}
	psxHu16(add) = value;
	PSXHW_LOG("*Known 16bit write at address %lx value %x\n", add, value);
}

#define DmaExec2(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR2 & (8 << ((n-7) * 4))) { \
		psxDma##n(HW_DMA##n##_MADR, HW_DMA##n##_BCR, HW_DMA##n##_CHCR); \
	} \
}

void psxHwWrite32(u32 add, u32 value) {
	if (add >= 0x1f801600 && add < 0x1f801700) {
		USBwrite32(add, value); return;
	}
	if (add >= 0x1f808400 && add <= 0x1f808550) {
		FWwrite32(add, value); return;
	}
	switch (add) {
	    case 0x1f801040:
			sioWrite8((u8)value);
			sioWrite8((u8)((value&0xff) >>  8));
			sioWrite8((u8)((value&0xff) >> 16));
			sioWrite8((u8)((value&0xff) >> 24));
			PAD_LOG("sio write32 %lx\n", value);
			return;
	//	case 0x1f801050: serial_write32(value); break;//serial port
		case 0x1f801060:
			PSXHW_LOG("RAM size write %lx\n", value);
			psxHu32(add) = value;
			return; // Ram size

//------------------------------------------------------------------
		case 0x1f801070: 
			PSXHW_LOG("IREG 32bit write %lx\n", value);
//			if (Config.Sio) psxHu32(0x1070) |= 0x80;
//			if (Config.SpuIrq) psxHu32(0x1070) |= 0x200;
			psxHu32(0x1070) &= value;
			return;

		case 0x1f801074:
			PSXHW_LOG("IMASK 32bit write %lx\n", value);
			psxHu32(0x1074) = value;
			iopTestIntc();
			return;

		case 0x1f801078: 
			PSXHW_LOG("ICTRL 32bit write %lx\n", value);
			psxHu32(0x1078) = value; //1;	//According to pSXAuthor this allways becomes 1 on write, but MHPB won't boot if value is not writen ;p
			iopTestIntc();
			return;

//------------------------------------------------------------------
		//SSBus registers
		case 0x1f801000:
			psxHu32(0x1000) = value;
			PSXHW_LOG("SSBUS <spd_addr> 32bit write %lx\n", value);
			return;
		case 0x1f801004:
			psxHu32(0x1004) = value;
			PSXHW_LOG("SSBUS <pio_addr> 32bit write %lx\n", value);
			return;
		case 0x1f801008:
			psxHu32(0x1008) = value;
			PSXHW_LOG("SSBUS <spd_delay> 32bit write %lx\n", value);
			return;
		case 0x1f80100C:
			psxHu32(0x100C) = value;
			PSXHW_LOG("SSBUS dev1_delay 32bit write %lx\n", value);
			return;
		case 0x1f801010:
			psxHu32(0x1010) = value;
			PSXHW_LOG("SSBUS rom_delay 32bit write %lx\n", value);
			return;
		case 0x1f801014:
			psxHu32(0x1014) = value;
			PSXHW_LOG("SSBUS spu_delay 32bit write %lx\n", value);
			return;
		case 0x1f801018:
			psxHu32(0x1018) = value;
			PSXHW_LOG("SSBUS dev5_delay 32bit write %lx\n", value);
			return;
		case 0x1f80101C:
			psxHu32(0x101C) = value;
			PSXHW_LOG("SSBUS <pio_delay> 32bit write %lx\n", value);
			return;
		case 0x1f801020:
			psxHu32(0x1020) = value;
			PSXHW_LOG("SSBUS com_delay 32bit write %lx\n", value);
			return;
		case 0x1f801400:
			psxHu32(0x1400) = value;
			PSXHW_LOG("SSBUS dev1_addr 32bit write %lx\n", value);
			return;
		case 0x1f801404:
			psxHu32(0x1404) = value;
			PSXHW_LOG("SSBUS spu_addr 32bit write %lx\n", value);
			return;
		case 0x1f801408:
			psxHu32(0x1408) = value;
			PSXHW_LOG("SSBUS dev5_addr 32bit write %lx\n", value);
			return;
		case 0x1f80140C:
			psxHu32(0x140C) = value;
			PSXHW_LOG("SSBUS spu1_addr 32bit write %lx\n", value);
			return;
		case 0x1f801410:
			psxHu32(0x1410) = value;
			PSXHW_LOG("SSBUS <dev9_addr3> 32bit write %lx\n", value);
			return;
		case 0x1f801414:
			psxHu32(0x1414) = value;
			PSXHW_LOG("SSBUS spu1_delay 32bit write %lx\n", value);
			return;
		case 0x1f801418:
			psxHu32(0x1418) = value;
			PSXHW_LOG("SSBUS <dev9_delay2> 32bit write %lx\n", value);
			return;
		case 0x1f80141C:
			psxHu32(0x141C) = value;
			PSXHW_LOG("SSBUS <dev9_delay3> 32bit write %lx\n", value);
			return;
		case 0x1f801420:
			psxHu32(0x1420) = value;
			PSXHW_LOG("SSBUS <dev9_delay1> 32bit write %lx\n", value);
			return;

//------------------------------------------------------------------
		case 0x1f801080:
			PSXHW_LOG("DMA0 MADR 32bit write %lx\n", value);
			HW_DMA0_MADR = value; return; // DMA0 madr
		case 0x1f801084:
			PSXHW_LOG("DMA0 BCR 32bit write %lx\n", value);
			HW_DMA0_BCR  = value; return; // DMA0 bcr
		case 0x1f801088:
			PSXHW_LOG("DMA0 CHCR 32bit write %lx\n", value);
			HW_DMA0_CHCR = value;        // DMA0 chcr (MDEC in DMA)
//			DmaExec(0);
			return;

//------------------------------------------------------------------
		case 0x1f801090:
			PSXHW_LOG("DMA1 MADR 32bit write %lx\n", value);
			HW_DMA1_MADR = value; return; // DMA1 madr
		case 0x1f801094:
			PSXHW_LOG("DMA1 BCR 32bit write %lx\n", value);
			HW_DMA1_BCR  = value; return; // DMA1 bcr
		case 0x1f801098:
			PSXHW_LOG("DMA1 CHCR 32bit write %lx\n", value);
			HW_DMA1_CHCR = value;        // DMA1 chcr (MDEC out DMA)
//			DmaExec(1);
			return;
		
//------------------------------------------------------------------
		case 0x1f8010a0:
			PSXHW_LOG("DMA2 MADR 32bit write %lx\n", value);
			HW_DMA2_MADR = value; return; // DMA2 madr
		case 0x1f8010a4:
			PSXHW_LOG("DMA2 BCR 32bit write %lx\n", value);
			HW_DMA2_BCR  = value; return; // DMA2 bcr
		case 0x1f8010a8:
			PSXHW_LOG("DMA2 CHCR 32bit write %lx\n", value);
			HW_DMA2_CHCR = value;        // DMA2 chcr (GPU DMA)
			DmaExec(2);
			return;

//------------------------------------------------------------------
		case 0x1f8010b0:
			PSXHW_LOG("DMA3 MADR 32bit write %lx\n", value);
			HW_DMA3_MADR = value; return; // DMA3 madr
		case 0x1f8010b4:
			PSXHW_LOG("DMA3 BCR 32bit write %lx\n", value);
			HW_DMA3_BCR  = value; return; // DMA3 bcr
		case 0x1f8010b8:
			PSXHW_LOG("DMA3 CHCR 32bit write %lx\n", value);
			HW_DMA3_CHCR = value;        // DMA3 chcr (CDROM DMA)
			DmaExec(3);
			
			return;

//------------------------------------------------------------------
		case 0x1f8010c0:
			PSXHW_LOG("DMA4 MADR 32bit write %lx\n", value);
			SPU2WriteMemAddr(0,value);
			HW_DMA4_MADR = value; return; // DMA4 madr
		case 0x1f8010c4:
			PSXHW_LOG("DMA4 BCR 32bit write %lx\n", value);
			HW_DMA4_BCR  = value; return; // DMA4 bcr
		case 0x1f8010c8:
			PSXHW_LOG("DMA4 CHCR 32bit write %lx\n", value);
			HW_DMA4_CHCR = value;        // DMA4 chcr (SPU DMA)
			DmaExec(4);
			return;

//------------------------------------------------------------------
#if 0
		case 0x1f8010d0: break; //DMA5write_madr();
		case 0x1f8010d4: break; //DMA5write_bcr();
		case 0x1f8010d8: break; //DMA5write_chcr(); // Not yet needed??
#endif

		case 0x1f8010e0:
			PSXHW_LOG("DMA6 MADR 32bit write %lx\n", value);
			HW_DMA6_MADR = value; return; // DMA6 madr
		case 0x1f8010e4:
			PSXHW_LOG("DMA6 BCR 32bit write %lx\n", value);
			HW_DMA6_BCR  = value; return; // DMA6 bcr
		case 0x1f8010e8:
			PSXHW_LOG("DMA6 CHCR 32bit write %lx\n", value);
			HW_DMA6_CHCR = value;         // DMA6 chcr (OT clear)
			DmaExec(6);
			return;

//------------------------------------------------------------------
		case 0x1f801500:
			PSXHW_LOG("DMA7 MADR 32bit write %lx\n", value);
			SPU2WriteMemAddr(1,value);
			HW_DMA7_MADR = value; return; // DMA7 madr
		case 0x1f801504:
			PSXHW_LOG("DMA7 BCR 32bit write %lx\n", value);
			HW_DMA7_BCR  = value; return; // DMA7 bcr
		case 0x1f801508:
			PSXHW_LOG("DMA7 CHCR 32bit write %lx\n", value);
			HW_DMA7_CHCR = value;         // DMA7 chcr (SPU2)
			DmaExec2(7);
			return;

//------------------------------------------------------------------
		case 0x1f801510:
			PSXHW_LOG("DMA8 MADR 32bit write %lx\n", value);
			HW_DMA8_MADR = value; return; // DMA8 madr
		case 0x1f801514:
			PSXHW_LOG("DMA8 BCR 32bit write %lx\n", value);
			HW_DMA8_BCR  = value; return; // DMA8 bcr
		case 0x1f801518:
			PSXHW_LOG("DMA8 CHCR 32bit write %lx\n", value);
			HW_DMA8_CHCR = value;         // DMA8 chcr (DEV9)
			DmaExec2(8);
			return;

//------------------------------------------------------------------
		case 0x1f801520:
			PSXHW_LOG("DMA9 MADR 32bit write %lx\n", value);
			HW_DMA9_MADR = value; return; // DMA9 madr
		case 0x1f801524:
			PSXHW_LOG("DMA9 BCR 32bit write %lx\n", value);
			HW_DMA9_BCR  = value; return; // DMA9 bcr
		case 0x1f801528:
			PSXHW_LOG("DMA9 CHCR 32bit write %lx\n", value);
			HW_DMA9_CHCR = value;         // DMA9 chcr (SIF0)
			DmaExec2(9);
			return;
		case 0x1f80152c:
			PSXHW_LOG("DMA9 TADR 32bit write %lx\n", value);
			HW_DMA9_TADR = value; return; // DMA9 tadr

//------------------------------------------------------------------
		case 0x1f801530:
			PSXHW_LOG("DMA10 MADR 32bit write %lx\n", value);
			HW_DMA10_MADR = value; return; // DMA10 madr
		case 0x1f801534:
			PSXHW_LOG("DMA10 BCR 32bit write %lx\n", value);
			HW_DMA10_BCR  = value; return; // DMA10 bcr
		case 0x1f801538:
			PSXHW_LOG("DMA10 CHCR 32bit write %lx\n", value);
			HW_DMA10_CHCR = value;         // DMA10 chcr (SIF1)
			DmaExec2(10);
			return;

//------------------------------------------------------------------
		case 0x1f801540:
			PSXHW_LOG("DMA11 SIO2in MADR 32bit write %lx\n", value);
			HW_DMA11_MADR = value; return;
		
		case 0x1f801544:
			PSXHW_LOG("DMA11 SIO2in BCR 32bit write %lx\n", value);
			HW_DMA11_BCR  = value; return;
		case 0x1f801548:
			PSXHW_LOG("DMA11 SIO2in CHCR 32bit write %lx\n", value);
			HW_DMA11_CHCR = value;         // DMA11 chcr (SIO2 in)
			DmaExec2(11);
			return;

//------------------------------------------------------------------
		case 0x1f801550:
			PSXHW_LOG("DMA12 SIO2out MADR 32bit write %lx\n", value);
			HW_DMA12_MADR = value; return;
		
		case 0x1f801554:
			PSXHW_LOG("DMA12 SIO2out BCR 32bit write %lx\n", value);
			HW_DMA12_BCR  = value; return;
		case 0x1f801558:
			PSXHW_LOG("DMA12 SIO2out CHCR 32bit write %lx\n", value);
			HW_DMA12_CHCR = value;         // DMA12 chcr (SIO2 out)
			DmaExec2(12);
			return;

//------------------------------------------------------------------
		case 0x1f801570:
			psxHu32(0x1570) = value;
			PSXHW_LOG("DMA PCR2 32bit write %lx\n", value);
			return;
		case 0x1f8010f0:
			PSXHW_LOG("DMA PCR 32bit write %lx\n", value);
			HW_DMA_PCR = value;
			return;

		case 0x1f8010f4:
			PSXHW_LOG("DMA ICR 32bit write %lx\n", value);
		{
			u32 tmp = (~value) & HW_DMA_ICR;
			HW_DMA_ICR = ((tmp ^ value) & 0xffffff) ^ tmp;
		}
		return;

		case 0x1f801574:
			PSXHW_LOG("DMA ICR2 32bit write %lx\n", value);
		{
			u32 tmp = (~value) & HW_DMA_ICR2;
			HW_DMA_ICR2 = ((tmp ^ value) & 0xffffff) ^ tmp;
		}
		return;

//------------------------------------------------------------------
/*		case 0x1f801810:
			PSXHW_LOG("GPU DATA 32bit write %lx\n", value);
			GPU_writeData(value); return;
		case 0x1f801814:
			PSXHW_LOG("GPU STATUS 32bit write %lx\n", value);
			GPU_writeStatus(value); return;
*/
/*		case 0x1f801820:
			mdecWrite0(value); break;
		case 0x1f801824:
			mdecWrite1(value); break;
*/
		case 0x1f801100:
			PSXCNT_LOG("COUNTER 0 COUNT 32bit write %lx\n", value);
			psxRcntWcount16(0, value ); return;
		case 0x1f801104:
			PSXCNT_LOG("COUNTER 0 MODE 32bit write %lx\n", value);
			psxRcnt0Wmode(value); return;
		case 0x1f801108:
			PSXCNT_LOG("COUNTER 0 TARGET 32bit write %lx\n", value);
			psxRcntWtarget16(0, value ); return;

		case 0x1f801110:
			PSXCNT_LOG("COUNTER 1 COUNT 32bit write %lx\n", value);
			psxRcntWcount16(1, value ); return;
		case 0x1f801114:
			PSXCNT_LOG("COUNTER 1 MODE 32bit write %lx\n", value);
			psxRcnt1Wmode(value); return;
		case 0x1f801118:
			PSXCNT_LOG("COUNTER 1 TARGET 32bit write %lx\n", value);
			psxRcntWtarget16(1, value ); return;

		case 0x1f801120:
			PSXCNT_LOG("COUNTER 2 COUNT 32bit write %lx\n", value);
			psxRcntWcount16(2, value ); return;
		case 0x1f801124:
			PSXCNT_LOG("COUNTER 2 MODE 32bit write %lx\n", value);
			psxRcnt2Wmode(value); return;
		case 0x1f801128:
			PSXCNT_LOG("COUNTER 2 TARGET 32bit write %lx\n", value);
			psxRcntWtarget16(2, value); return;

		case 0x1f801480:
			PSXCNT_LOG("COUNTER 3 COUNT 32bit write %lx\n", value);
			psxRcntWcount32(3, value); return;
		case 0x1f801484:
			PSXCNT_LOG("COUNTER 3 MODE 32bit write %lx\n", value);
			psxRcnt3Wmode(value); return;
		case 0x1f801488:
			PSXCNT_LOG("COUNTER 3 TARGET 32bit write %lx\n", value);
			psxRcntWtarget32(3, value); return;

		case 0x1f801490:
			PSXCNT_LOG("COUNTER 4 COUNT 32bit write %lx\n", value);
			psxRcntWcount32(4, value); return;
		case 0x1f801494:
			PSXCNT_LOG("COUNTER 4 MODE 32bit write %lx\n", value);
			psxRcnt4Wmode(value); return;
		case 0x1f801498:
			PSXCNT_LOG("COUNTER 4 TARGET 32bit write %lx\n", value);
			psxRcntWtarget32(4, value); return;

		case 0x1f8014a0:
			PSXCNT_LOG("COUNTER 5 COUNT 32bit write %lx\n", value);
			psxRcntWcount32(5, value); return;
		case 0x1f8014a4:
			PSXCNT_LOG("COUNTER 5 MODE 32bit write %lx\n", value);
			psxRcnt5Wmode(value); return;
		case 0x1f8014a8:
			PSXCNT_LOG("COUNTER 5 TARGET 32bit write %lx\n", value);
			psxRcntWtarget32(5, value); return;

//------------------------------------------------------------------
		case 0x1f8014c0:
			PSXHW_LOG("RTC_HOLDMODE 32bit write %lx\n", value);
			Console::Notice("** RTC_HOLDMODE 32bit write %lx", params value);
			break;

		case 0x1f801450:
			if (value) { PSXHW_LOG("%08X ICFG 32bit write %lx\n", psxRegs.pc, value); }
/*			if (value &&
				psxSu32(0x20) == 0x20000 &&
				(psxSu32(0x30) == 0x20000 ||
				 psxSu32(0x30) == 0x40000)) { // don't ask me why :P
				psxSu32(0x20) = 0x10000;
				psxSu32(0x30) = 0x10000;
			}*/
			psxHu32(0x1450) = /*(*/value/* & (~0x8)) | (psxHu32(0x1450) & 0x8)*/;
			return;

//------------------------------------------------------------------
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
			PSXHW_LOG("SIO2 write param[%d] <- %lx\n", (add-0x1F808200)/4, value);
			sio2_setSend3((add-0x1F808200)/4, value);	return;

		case 0x1F808240:
		case 0x1F808248:
		case 0x1F808250:
		case 0x1F808258:
			PSXHW_LOG("SIO2 write send1[%d] <- %lx\n", (add-0x1F808240)/8, value);
			sio2_setSend1((add-0x1F808240)/8, value);	return;

		case 0x1F808244:
		case 0x1F80824C:
		case 0x1F808254:
		case 0x1F80825C:
			PSXHW_LOG("SIO2 write send2[%d] <- %lx\n", (add-0x1F808244)/8, value);
			sio2_setSend2((add-0x1F808244)/8, value);	return;

		case 0x1F808268:
			PSXHW_LOG("SIO2 write CTRL <- %lx\n", value);
			sio2_setCtrl(value);	return;

		case 0x1F808278:
			PSXHW_LOG("SIO2 write [8278] <- %lx\n", value);
			sio2_set8278(value);	return;

		case 0x1F80827C:
			PSXHW_LOG("SIO2 write [827C] <- %lx\n", value);
			sio2_set827C(value);	return;

		case 0x1F808280:
			PSXHW_LOG("SIO2 write INTR <- %lx\n", value);
			sio2_setIntr(value);	return;

//------------------------------------------------------------------
		default:
			psxHu32(add) = value;
			PSXHW_LOG("*Unknown 32bit write at address %lx value %lx\n", add, value);
			return;
	}
	psxHu32(add) = value;
	PSXHW_LOG("*Known 32bit write at address %lx value %lx\n", add, value);
}

u8 psxHw4Read8(u32 add) {
	u8 hard;

	switch (add) {
		case 0x1f402004: return cdvdRead04();
		case 0x1f402005: return cdvdRead05();
		case 0x1f402006: return cdvdRead06();
		case 0x1f402007: return cdvdRead07();
		case 0x1f402008: return cdvdRead08();
		case 0x1f40200A: return cdvdRead0A();
		case 0x1f40200B: return cdvdRead0B();
		case 0x1f40200C: return cdvdRead0C();
		case 0x1f40200D: return cdvdRead0D();
		case 0x1f40200E: return cdvdRead0E();
		case 0x1f40200F: return cdvdRead0F();
		case 0x1f402013: return cdvdRead13();
		case 0x1f402015: return cdvdRead15();
		case 0x1f402016: return cdvdRead16();
		case 0x1f402017: return cdvdRead17();
		case 0x1f402018: return cdvdRead18();
		case 0x1f402020: return cdvdRead20();
		case 0x1f402021: return cdvdRead21();
		case 0x1f402022: return cdvdRead22();
		case 0x1f402023: return cdvdRead23();
		case 0x1f402024: return cdvdRead24();
		case 0x1f402028: return cdvdRead28();
		case 0x1f402029: return cdvdRead29();
		case 0x1f40202A: return cdvdRead2A();
		case 0x1f40202B: return cdvdRead2B();
		case 0x1f40202C: return cdvdRead2C();
		case 0x1f402030: return cdvdRead30();
		case 0x1f402031: return cdvdRead31();
		case 0x1f402032: return cdvdRead32();
		case 0x1f402033: return cdvdRead33();
		case 0x1f402034: return cdvdRead34();
		case 0x1f402038: return cdvdRead38();
		case 0x1f402039: return cdvdRead39();
		case 0x1f40203A: return cdvdRead3A();
		default:
			// note: use SysPrintF to notify console since this is a potentially serious
			// emulation problem:
			//PSXHW_LOG("*Unknown 8bit read at address %lx\n", add);
			SysPrintf("*Unknown 8bit read at address %lx\n", add);
			return 0;
	}
	
	PSXHW_LOG("*Known 8bit read at address %lx value %x\n", add, hard);

	return hard;
}

void psxHw4Write8(u32 add, u8 value) {
	
	switch (add) {
		case 0x1f402004: cdvdWrite04(value); return;
		case 0x1f402005: cdvdWrite05(value); return;
		case 0x1f402006: cdvdWrite06(value); return;
		case 0x1f402007: cdvdWrite07(value); return;
		case 0x1f402008: cdvdWrite08(value); return;
		case 0x1f40200A: cdvdWrite0A(value); return;
		case 0x1f40200F: cdvdWrite0F(value); return;
		case 0x1f402014: cdvdWrite14(value); return;
		case 0x1f402016: cdvdWrite16(value); return;
		case 0x1f402017: cdvdWrite17(value); return;
		case 0x1f402018: cdvdWrite18(value); return;
		case 0x1f40203A: cdvdWrite3A(value); return;
		default:
			//PSXHW_LOG("*Unknown 8bit write at address %lx value %x\n", add, value);
			Console::Notice("*Unknown 8bit write at address %lx value %x", params add, value);
			return;
	}
	PSXHW_LOG("*Known 8bit write at address %lx value %x\n", add, value);
}

void psxDmaInterrupt(int n) {
	if (HW_DMA_ICR & (1 << (16 + n))) {
		HW_DMA_ICR|= (1 << (24 + n));
		psxRegs.CP0.n.Cause |= 1 << (9 + n);
		iopIntcIrq( 3 );
	}
}

void psxDmaInterrupt2(int n) {
	if (HW_DMA_ICR2 & (1 << (16 + n))) {
/*		if (HW_DMA_ICR2 & (1 << (24 + n))) {
			SysPrintf("*PCSX2*: HW_DMA_ICR2 n=%d already set\n", n);
		}
		if (psxHu32(0x1070) & 8) {
			SysPrintf("*PCSX2*: psxHu32(0x1070) 8 already set (n=%d)\n", n);
		}*/
		HW_DMA_ICR2|= (1 << (24 + n));
		psxRegs.CP0.n.Cause |= 1 << (16 + n);
		iopIntcIrq( 3 );
	}
}
