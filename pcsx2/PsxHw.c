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

#include <stdio.h>
#include <string.h>

#include "PsxCommon.h"
#include "Misc.h"
#include "iR5900.h"


#ifdef _WIN32
#pragma warning(disable:4244)
#endif

// NOTE: Any modifications to read/write fns should also go into their const counterparts

void psxHwReset() {
/*	if (Config.Sio) psxHu32(0x1070) |= 0x80;
	if (Config.SpuIrq) psxHu32(0x1070) |= 0x200;*/

	memset(psxH, 0, 0x10000);

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
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read8 DATAOUT %08X\n", hard);
#endif
			return hard;

		default:
			hard = psxHu8(add); 
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unkwnown 8bit read at address %lx\n", add);
#endif
			return hard;
	}
	
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 8bit read at address %lx value %x\n", add, hard);
#endif
	return hard;
}

u16 psxHwRead16(u32 add) {
	u16 hard;

	if (add >= 0x1f801600 && add < 0x1f801700) {
		return USBread16(add);
	}

	switch (add) {
#ifdef PSXHW_LOG
		case 0x1f801070: PSXHW_LOG("IREG 16bit read %x\n", psxHu16(0x1070));
			return psxHu16(0x1070);
#endif
#ifdef PSXHW_LOG
		case 0x1f801074: PSXHW_LOG("IMASK 16bit read %x\n", psxHu16(0x1074));
			return psxHu16(0x1074);
#endif

		case 0x1f801040:
			hard = sioRead8();
			hard|= sioRead8() << 8;
#ifdef PAD_LOG
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
#endif
			return hard;
		case 0x1f801044:
			hard = sio.StatReg;
#ifdef PAD_LOG
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
#endif
			return hard;
		case 0x1f801048:
			hard = sio.ModeReg;
#ifdef PAD_LOG
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
#endif
			return hard;
		case 0x1f80104a:
			hard = sio.CtrlReg;
#ifdef PAD_LOG
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
#endif
			return hard;
		case 0x1f80104e:
			hard = sio.BaudReg;
#ifdef PAD_LOG
			PAD_LOG("sio read16 %lx; ret = %x\n", add&0xf, hard);
#endif
			return hard;

		//Serial port stuff not support now ;P
	 // case 0x1f801050: hard = serial_read16(); break;
	 //	case 0x1f801054: hard = serial_status_read(); break;
	 //	case 0x1f80105a: hard = serial_control_read(); break;
	 //	case 0x1f80105e: hard = serial_baud_read(); break;
	
		case 0x1f801100:
			hard = (u16)psxRcntRcount16(0);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T0 count read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801104:
			hard = psxCounters[0].mode;
            psxCounters[0].mode &= ~0x1800;
			psxCounters[0].mode |= 0x400;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T0 mode read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801108:
			hard = psxCounters[0].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T0 target read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801110:
			hard = (u16)psxRcntRcount16(1);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T1 count read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801114:
			hard = psxCounters[1].mode;
			psxCounters[1].mode &= ~0x1800;
			psxCounters[1].mode |= 0x400;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T1 mode read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801118:
			hard = psxCounters[1].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T1 target read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801120:
			hard = (u16)psxRcntRcount16(2);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T2 count read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801124:
			hard = psxCounters[2].mode;
			psxCounters[2].mode &= ~0x1800;
			psxCounters[2].mode |= 0x400;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T2 mode read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801128:
			hard = psxCounters[2].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T2 target read16: %x\n", hard);
#endif
			return hard;

		case 0x1f80146e: // DEV9_R_REV
			return DEV9read16(add);

		case 0x1f801480:
			hard = (u16)psxRcntRcount32(3);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T3 count read16: %lx\n", hard);
#endif
			return hard;
		case 0x1f801484:
			hard = psxCounters[3].mode;
			psxCounters[3].mode &= ~0x1800;
			psxCounters[3].mode |= 0x400;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T3 mode read16: %lx\n", hard);
#endif
			return hard;
		case 0x1f801488:
			hard = psxCounters[3].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T3 target read16: %lx\n", hard);
#endif
			return hard;
		case 0x1f801490:
			hard = (u16)psxRcntRcount32(4);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T4 count read16: %lx\n", hard);
#endif
			return hard;
		case 0x1f801494:
			hard = psxCounters[4].mode;
			psxCounters[4].mode &= ~0x1800;
			psxCounters[4].mode |= 0x400;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T4 mode read16: %lx\n", hard);
#endif
			return hard;
		case 0x1f801498:
			hard = psxCounters[4].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T4 target read16: %lx\n", hard);
#endif
			return hard;
		case 0x1f8014a0:
			hard = (u16)psxRcntRcount32(5);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T5 count read16: %lx\n", hard);
#endif
			return hard;
		case 0x1f8014a4:
			hard = psxCounters[5].mode;
			psxCounters[5].mode &= ~0x1800;
			psxCounters[5].mode |= 0x400;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T5 mode read16: %lx\n", hard);
#endif
			return hard;
		case 0x1f8014a8:
			hard = psxCounters[5].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T5 target read16: %lx\n", hard);
#endif
			return hard;

		case 0x1f801504:
			hard = psxHu16(0x1504);
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA7 BCR_size 16bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801506:
			hard = psxHu16(0x1506);
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA7 BCR_count 16bit read %lx\n", hard);
#endif
			return hard;
		//case 0x1f802030: hard =   //int_2000????
		//case 0x1f802040: hard =//dip switches...??

		default:
			if (add>=0x1f801c00 && add<0x1f801e00) {
            	hard = SPU2read(add);
			} else {
				hard = psxHu16(add); 
#ifdef PSXHW_LOG
				PSXHW_LOG("*Unkwnown 16bit read at address %lx\n", add);
#endif
			}
            return hard;
	}
	
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 16bit read at address %lx value %x\n", add, hard);
#endif
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
#ifdef PAD_LOG
			PAD_LOG("sio read32 ;ret = %lx\n", hard);
#endif
			return hard;
			
	//	case 0x1f801050: hard = serial_read32(); break;//serial port
#ifdef PSXHW_LOG
		case 0x1f801060:
			PSXHW_LOG("RAM size read %lx\n", psxHu32(0x1060));
			return psxHu32(0x1060);
#endif
#ifdef PSXHW_LOG
		case 0x1f801070: PSXHW_LOG("IREG 32bit read %x\n", psxHu32(0x1070));
			return psxHu32(0x1070);
#endif
#ifdef PSXHW_LOG
		case 0x1f801074: PSXHW_LOG("IMASK 32bit read %x\n", psxHu32(0x1074));
			return psxHu32(0x1074);
#endif
		case 0x1f801078:
#ifdef PSXHW_LOG
			PSXHW_LOG("ICTRL 32bit read %x\n", psxHu32(0x1078));
#endif
			hard = psxHu32(0x1078);
			psxHu32(0x1078) = 0;
			return hard;

/*		case 0x1f801810:
//			hard = GPU_readData();
#ifdef PSXHW_LOG
			PSXHW_LOG("GPU DATA 32bit read %lx\n", hard);
#endif
			return hard;*/
/*		case 0x1f801814:
			hard = GPU_readStatus();
#ifdef PSXHW_LOG
			PSXHW_LOG("GPU STATUS 32bit read %lx\n", hard);
#endif
			return hard;
*/
/*		case 0x1f801820: hard = mdecRead0(); break;
		case 0x1f801824: hard = mdecRead1(); break;
*/
#ifdef PSXHW_LOG
		case 0x1f8010a0:
			PSXHW_LOG("DMA2 MADR 32bit read %lx\n", psxHu32(0x10a0));
			return HW_DMA2_MADR;
		case 0x1f8010a4:
			PSXHW_LOG("DMA2 BCR 32bit read %lx\n", psxHu32(0x10a4));
			return HW_DMA2_BCR;
		case 0x1f8010a8:
			PSXHW_LOG("DMA2 CHCR 32bit read %lx\n", psxHu32(0x10a8));
			return HW_DMA2_CHCR;
#endif

#ifdef PSXHW_LOG
		case 0x1f8010b0:
			PSXHW_LOG("DMA3 MADR 32bit read %lx\n", psxHu32(0x10b0));
			return HW_DMA3_MADR;
		case 0x1f8010b4:
			PSXHW_LOG("DMA3 BCR 32bit read %lx\n", psxHu32(0x10b4));
			return HW_DMA3_BCR;
		case 0x1f8010b8:
			PSXHW_LOG("DMA3 CHCR 32bit read %lx\n", psxHu32(0x10b8));
			return HW_DMA3_CHCR;
#endif

#ifdef PSXHW_LOG
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
#endif

#ifdef PSXHW_LOG
		case 0x1f801530:
			PSXHW_LOG("DMA10 MADR 32bit read %lx\n", HW_DMA10_MADR);
			return HW_DMA10_MADR;
		case 0x1f801534:
			PSXHW_LOG("DMA10 BCR 32bit read %lx\n", HW_DMA10_BCR);
			return HW_DMA10_BCR;
		case 0x1f801538:
			PSXHW_LOG("DMA10 CHCR 32bit read %lx\n", HW_DMA10_CHCR);
			return HW_DMA10_CHCR;
#endif

//		case 0x1f8010f0:  PSXHW_LOG("DMA PCR 32bit read " << psxHu32(0x10f0));
//			return HW_DMA_PCR; // dma rest channel
#ifdef PSXHW_LOG
		case 0x1f8010f4:
			PSXHW_LOG("DMA ICR 32bit read %lx\n", HW_DMA_ICR);
			return HW_DMA_ICR;
#endif
//SSBus registers
		case 0x1f801000:
			hard = psxHu32(0x1000);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <spd_addr> 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801004:
			hard = psxHu32(0x1004);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <pio_addr> 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801008:
			hard = psxHu32(0x1008);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <spd_delay> 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f80100C:
			hard = psxHu32(0x100C);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS dev1_delay 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801010:
			hard = psxHu32(0x1010);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS rom_delay 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801014:
			hard = psxHu32(0x1014);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS spu_delay 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801018:
			hard = psxHu32(0x1018);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS dev5_delay 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f80101C:
			hard = psxHu32(0x101C);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <pio_delay> 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801020:
			hard = psxHu32(0x1020);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS com_delay 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801400:
			hard = psxHu32(0x1400);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS dev1_addr 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801404:
			hard = psxHu32(0x1404);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS spu_addr 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801408:
			hard = psxHu32(0x1408);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS dev5_addr 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f80140C:
			hard = psxHu32(0x140C);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS spu1_addr 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801410:
			hard = psxHu32(0x1410);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <dev9_addr3> 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801414:
			hard = psxHu32(0x1414);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS spu1_delay 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801418:
			hard = psxHu32(0x1418);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <dev9_delay2> 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f80141C:
			hard = psxHu32(0x141C);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <dev9_delay3> 32bit read %lx\n", hard);
#endif
			return hard;
		case 0x1f801420:
			hard = psxHu32(0x1420);
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <dev9_delay1> 32bit read %lx\n", hard);
#endif
			return hard;

		case 0x1f8010f0:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA PCR 32bit read %lx\n", HW_DMA_PCR);
#endif
			return HW_DMA_PCR;

		case 0x1f8010c8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA4 CHCR 32bit read %lx\n", HW_DMA4_CHCR);
#endif
		 return HW_DMA4_CHCR;       // DMA4 chcr (SPU DMA)
			
		// time for rootcounters :)
		case 0x1f801100:
			hard = (u16)psxRcntRcount16(0);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T0 count read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801104:
			hard = (u16)psxCounters[0].mode;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T0 mode read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801108:
			hard = psxCounters[0].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T0 target read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801110:
			hard = (u16)psxRcntRcount16(1);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T1 count read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801114:
			hard = (u16)psxCounters[1].mode;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T1 mode read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801118:
			hard = psxCounters[1].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T1 target read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801120:
			hard = (u16)psxRcntRcount16(2);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T2 count read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801124:
			hard = (u16)psxCounters[2].mode;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T2 mode read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801128:
			hard = psxCounters[2].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T2 target read32: %lx\n", hard);
#endif
			return hard;

		case 0x1f801480:
			hard = (u32)psxRcntRcount32(3);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T3 count read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801484:
			hard = (u16)psxCounters[3].mode;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T3 mode read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801488:
			hard = psxCounters[3].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T3 target read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801490:
			hard = (u32)psxRcntRcount32(4);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T4 count read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801494:
			hard = (u16)psxCounters[4].mode;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T4 mode read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f801498:
			hard = psxCounters[4].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T4 target read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f8014a0:
			hard = (u32)psxRcntRcount32(5);
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T5 count read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f8014a4:
			hard = (u16)psxCounters[5].mode;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T5 mode read32: %lx\n", hard);
#endif
			return hard;
		case 0x1f8014a8:
			hard = psxCounters[5].target;
#ifdef PSXCNT_LOG
			PSXCNT_LOG("T5 target read32: %lx\n", hard);
#endif
			return hard;

		case 0x1f801450:
			hard = psxHu32(add);
#ifdef PSXHW_LOG
			PSXHW_LOG("%08X ICFG 32bit read %x\n", psxRegs.pc, hard);
#endif
			return hard;


		case 0x1F8010C0:
			HW_DMA4_MADR = SPU2ReadMemAddr(0);
			return HW_DMA4_MADR;

		case 0x1f801500:
			HW_DMA7_MADR = SPU2ReadMemAddr(1);
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA7 MADR 32bit read %lx\n", HW_DMA7_MADR);
#endif
			return HW_DMA7_MADR;  // DMA7 madr
		case 0x1f801504:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA7 BCR 32bit read %lx\n", HW_DMA7_BCR);
#endif
			return HW_DMA7_BCR; // DMA7 bcr

		case 0x1f801508:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA7 CHCR 32bit read %lx\n", HW_DMA7_CHCR);
#endif
			return HW_DMA7_CHCR;         // DMA7 chcr (SPU2)		

		case 0x1f801570:
			hard = psxHu32(0x1570);
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA PCR2 32bit read %lx\n", hard);
#endif
			return hard;
#ifdef PSXHW_LOG
		case 0x1f801574:
			PSXHW_LOG("DMA ICR2 32bit read %lx\n", HW_DMA_ICR2);
			return HW_DMA_ICR2;
#endif

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
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read param[%d] (%lx)\n", (add-0x1F808200)/4, hard);
#endif
			return hard;

		case 0x1F808240:
		case 0x1F808248:
		case 0x1F808250:
		case 0x1F80825C:
			hard=sio2_getSend1((add-0x1F808240)/8);
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read send1[%d] (%lx)\n", (add-0x1F808240)/8, hard);
#endif
			return hard;
		
		case 0x1F808244:
		case 0x1F80824C:
		case 0x1F808254:
		case 0x1F808258:
			hard=sio2_getSend2((add-0x1F808244)/8);
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read send2[%d] (%lx)\n", (add-0x1F808244)/8, hard);
#endif
			return hard;

		case 0x1F808268:
			hard=sio2_getCtrl();
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read CTRL (%lx)\n", hard);
#endif
			return hard;

		case 0x1F80826C:
			hard=sio2_getRecv1();
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read Recv1 (%lx)\n", hard);
#endif
			return hard;

		case 0x1F808270:
			hard=sio2_getRecv2();
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read Recv2 (%lx)\n", hard);
#endif
			return hard;

		case 0x1F808274:
			hard=sio2_getRecv3();
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read Recv3 (%lx)\n", hard);
#endif
			return hard;

		case 0x1F808278:
			hard=sio2_get8278();
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read [8278] (%lx)\n", hard);
#endif
			return hard;

		case 0x1F80827C:
			hard=sio2_get827C();
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read [827C] (%lx)\n", hard);
#endif
			return hard;

		case 0x1F808280:
			hard=sio2_getIntr();
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 read INTR (%lx)\n", hard);
#endif
			return hard;

		default:
			hard = psxHu32(add); 
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unknown 32bit read at address %lx: %lx\n", add, hard);
#endif
			return hard;
	}
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 32bit read at address %lx: %lx\n", add, hard);
#endif
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
#ifdef PCSX2_DEVBUILD
	if((add & 0xf) == 0xa) SysPrintf("8bit write (possible chcr set) %x value %x\n", add, value);
#endif
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
#ifdef PSXHW_LOG
			if (value) { PSXHW_LOG("%08X ICFG 8bit write %lx\n", psxRegs.pc, value); }
#endif
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
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 write8 DATAIN <- %08X\n", value);
#endif
			sio2_serialIn(value);return;//serial data feed/fifo

		default:
			psxHu8(add) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unknown 8bit write at address %lx value %x\n", add, value);
#endif
			return;
	}
	psxHu8(add) = value;
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 8bit write at address %lx value %x\n", add, value);
#endif
}

void psxHwWrite16(u32 add, u16 value) {
	if (add >= 0x1f801600 && add < 0x1f801700) {
		USBwrite16(add, value); return;
	}
#ifdef PCSX2_DEVBUILD
	if((add & 0xf) == 0x9) SysPrintf("16bit write (possible chcr set) %x value %x\n", add, value);
#endif
	switch (add) {
		case 0x1f801040:
			sioWrite8((u8)value);
			sioWrite8((u8)(value>>8));
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
#endif
			return;
		case 0x1f801044:
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
#endif
			return;
		case 0x1f801048:
			sio.ModeReg = value;
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
#endif
			return;
		case 0x1f80104a: // control register
			sioWriteCtrl16(value);
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
#endif
			return;
		case 0x1f80104e: // baudrate register
			sio.BaudReg = value;
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %lx, %x\n", add&0xf, value);
#endif
			return;

		//serial port ;P
	//  case 0x1f801050: serial_write16(value); break;
	//	case 0x1f80105a: serial_control_write(value);break;
	//	case 0x1f80105e: serial_baud_write(value); break;
	//	case 0x1f801054: serial_status_write(value); break;

		case 0x1f801070: 
#ifdef PSXHW_LOG
			PSXHW_LOG("IREG 16bit write %x\n", value);
#endif
//			if (Config.Sio) psxHu16(0x1070) |= 0x80;
//			if (Config.SpuIrq) psxHu16(0x1070) |= 0x200;
			psxHu16(0x1070) &= value;
			return;
#ifdef PSXHW_LOG
		case 0x1f801074: PSXHW_LOG("IMASK 16bit write %x\n", value);
			psxHu16(0x1074) = value;
			return;
#endif

		case 0x1f8010c4:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA4 BCR_size 16bit write %lx\n", value);
#endif
			psxHu16(0x10c4) = value; return; // DMA4 bcr_size

		case 0x1f8010c6:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA4 BCR_count 16bit write %lx\n", value);
#endif
			psxHu16(0x10c6) = value; return; // DMA4 bcr_count

		case 0x1f801100:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 0 COUNT 16bit write %x\n", value);
#endif
			psxRcntWcount16(0, value); return;
		case 0x1f801104:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 0 MODE 16bit write %x\n", value);
#endif
			psxRcnt0Wmode(value); return;
		case 0x1f801108:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 0 TARGET 16bit write %x\n", value);
#endif
			psxRcntWtarget16(0, value); return;

		case 0x1f801110:
#ifdef PSXHW_LOG
			PSXCNT_LOG("COUNTER 1 COUNT 16bit write %x\n", value);
#endif
			psxRcntWcount16(1, value); return;
		case 0x1f801114:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 1 MODE 16bit write %x\n", value);
#endif
			psxRcnt1Wmode(value); return;
		case 0x1f801118:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 1 TARGET 16bit write %x\n", value);
#endif
			psxRcntWtarget16(1, value); return;

		case 0x1f801120:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 2 COUNT 16bit write %x\n", value);
#endif
			psxRcntWcount16(2, value); return;
		case 0x1f801124:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 2 MODE 16bit write %x\n", value);
#endif
			psxRcnt2Wmode(value); return;
		case 0x1f801128:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 2 TARGET 16bit write %x\n", value);
#endif
			psxRcntWtarget16(2, value); return;

		case 0x1f801450:
#ifdef PSXHW_LOG
			if (value) { PSXHW_LOG("%08X ICFG 16bit write %lx\n", psxRegs.pc, value); }
#endif
			psxHu16(0x1450) = value/* & (~0x8)*/;
			return;

		case 0x1f801480:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 3 COUNT 16bit write %lx\n", value);
#endif
			psxRcntWcount32(3, value); return;
		case 0x1f801484:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 3 MODE 16bit write %lx\n", value);
#endif
			psxRcnt3Wmode(value); return;
		case 0x1f801488:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 3 TARGET 16bit write %lx\n", value);
#endif
			psxRcntWtarget32(3, value); return;

		case 0x1f801490:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 4 COUNT 16bit write %lx\n", value);
#endif
			psxRcntWcount32(4, value); return;
		case 0x1f801494:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 4 MODE 16bit write %lx\n", value);
#endif
			psxRcnt4Wmode(value); return;
		case 0x1f801498:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 4 TARGET 16bit write %lx\n", value);
#endif
			psxRcntWtarget32(4, value); return;

		case 0x1f8014a0:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 5 COUNT 16bit write %lx\n", value);
#endif
			psxRcntWcount32(5, value); return;
		case 0x1f8014a4:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 5 MODE 16bit write %lx\n", value);
#endif
			psxRcnt5Wmode(value); return;
		case 0x1f8014a8:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 5 TARGET 16bit write %lx\n", value);
#endif
			psxRcntWtarget32(5, value); return;

		case 0x1f801504:
			psxHu16(0x1504) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA7 BCR_size 16bit write %lx\n", value);
#endif
			return;
		case 0x1f801506:
			psxHu16(0x1506) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA7 BCR_count 16bit write %lx\n", value);
#endif
			return;
		default:
			if (add>=0x1f801c00 && add<0x1f801e00) {
				SPU2write(add, value);
				return;
			}

			psxHu16(add) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unknown 16bit write at address %lx value %x\n", add, value);
#endif
			return;
	}
	psxHu16(add) = value;
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 16bit write at address %lx value %x\n", add, value);
#endif
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
#ifdef PAD_LOG
			PAD_LOG("sio write32 %lx\n", value);
#endif
			return;
	//	case 0x1f801050: serial_write32(value); break;//serial port
#ifdef PSXHW_LOG
		case 0x1f801060:
			PSXHW_LOG("RAM size write %lx\n", value);
			psxHu32(add) = value;
			return; // Ram size
#endif

		case 0x1f801070: 
#ifdef PSXHW_LOG
			PSXHW_LOG("IREG 32bit write %lx\n", value);
#endif
//			if (Config.Sio) psxHu32(0x1070) |= 0x80;
//			if (Config.SpuIrq) psxHu32(0x1070) |= 0x200;
			psxHu32(0x1070) &= value;
			return;
#ifdef PSXHW_LOG
		case 0x1f801074:
			PSXHW_LOG("IMASK 32bit write %lx\n", value);
			psxHu32(0x1074) = value;
			return;

		case 0x1f801078: 
			PSXHW_LOG("ICTRL 32bit write %lx\n", value);
//			SysPrintf("ICTRL 32bit write %lx\n", value);
			psxHu32(0x1078) = value;
			return;
#endif

		//SSBus registers
		case 0x1f801000:
			psxHu32(0x1000) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <spd_addr> 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801004:
			psxHu32(0x1004) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <pio_addr> 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801008:
			psxHu32(0x1008) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <spd_delay> 32bit write %lx\n", value);
#endif
			return;
		case 0x1f80100C:
			psxHu32(0x100C) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS dev1_delay 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801010:
			psxHu32(0x1010) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS rom_delay 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801014:
			psxHu32(0x1014) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS spu_delay 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801018:
			psxHu32(0x1018) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS dev5_delay 32bit write %lx\n", value);
#endif
			return;
		case 0x1f80101C:
			psxHu32(0x101C) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <pio_delay> 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801020:
			psxHu32(0x1020) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS com_delay 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801400:
			psxHu32(0x1400) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS dev1_addr 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801404:
			psxHu32(0x1404) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS spu_addr 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801408:
			psxHu32(0x1408) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS dev5_addr 32bit write %lx\n", value);
#endif
			return;
		case 0x1f80140C:
			psxHu32(0x140C) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS spu1_addr 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801410:
			psxHu32(0x1410) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <dev9_addr3> 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801414:
			psxHu32(0x1414) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS spu1_delay 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801418:
			psxHu32(0x1418) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <dev9_delay2> 32bit write %lx\n", value);
#endif
			return;
		case 0x1f80141C:
			psxHu32(0x141C) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <dev9_delay3> 32bit write %lx\n", value);
#endif
			return;
		case 0x1f801420:
			psxHu32(0x1420) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("SSBUS <dev9_delay1> 32bit write %lx\n", value);
#endif
			return;
#ifdef PSXHW_LOG
		case 0x1f801080:
			PSXHW_LOG("DMA0 MADR 32bit write %lx\n", value);
			HW_DMA0_MADR = value; return; // DMA0 madr
		case 0x1f801084:
			PSXHW_LOG("DMA0 BCR 32bit write %lx\n", value);
			HW_DMA0_BCR  = value; return; // DMA0 bcr
#endif
		case 0x1f801088:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA0 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA0_CHCR = value;        // DMA0 chcr (MDEC in DMA)
//			DmaExec(0);
			return;

#ifdef PSXHW_LOG
		case 0x1f801090:
			PSXHW_LOG("DMA1 MADR 32bit write %lx\n", value);
			HW_DMA1_MADR = value; return; // DMA1 madr
		case 0x1f801094:
			PSXHW_LOG("DMA1 BCR 32bit write %lx\n", value);
			HW_DMA1_BCR  = value; return; // DMA1 bcr
#endif
		case 0x1f801098:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA1 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA1_CHCR = value;        // DMA1 chcr (MDEC out DMA)
//			DmaExec(1);
			return;
		
#ifdef PSXHW_LOG
		case 0x1f8010a0:
			PSXHW_LOG("DMA2 MADR 32bit write %lx\n", value);
			HW_DMA2_MADR = value; return; // DMA2 madr
		case 0x1f8010a4:
			PSXHW_LOG("DMA2 BCR 32bit write %lx\n", value);
			HW_DMA2_BCR  = value; return; // DMA2 bcr
#endif
		case 0x1f8010a8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA2 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA2_CHCR = value;        // DMA2 chcr (GPU DMA)
			DmaExec(2);
			return;

#ifdef PSXHW_LOG
		case 0x1f8010b0:
			PSXHW_LOG("DMA3 MADR 32bit write %lx\n", value);
			HW_DMA3_MADR = value; return; // DMA3 madr
		case 0x1f8010b4:
			PSXHW_LOG("DMA3 BCR 32bit write %lx\n", value);
			HW_DMA3_BCR  = value; return; // DMA3 bcr
#endif
		case 0x1f8010b8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA3 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA3_CHCR = value;        // DMA3 chcr (CDROM DMA)
			DmaExec(3);
			
			return;

		case 0x1f8010c0:
			PSXHW_LOG("DMA4 MADR 32bit write %lx\n", value);
			SPU2WriteMemAddr(0,value);
			HW_DMA4_MADR = value; return; // DMA4 madr
		case 0x1f8010c4:
			PSXHW_LOG("DMA4 BCR 32bit write %lx\n", value);
			HW_DMA4_BCR  = value; return; // DMA4 bcr
		case 0x1f8010c8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA4 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA4_CHCR = value;        // DMA4 chcr (SPU DMA)
			DmaExec(4);
			return;

#if 0
		case 0x1f8010d0: break; //DMA5write_madr();
		case 0x1f8010d4: break; //DMA5write_bcr();
		case 0x1f8010d8: break; //DMA5write_chcr(); // Not yet needed??
#endif

#ifdef PSXHW_LOG
		case 0x1f8010e0:
			PSXHW_LOG("DMA6 MADR 32bit write %lx\n", value);
			HW_DMA6_MADR = value; return; // DMA6 madr
		case 0x1f8010e4:
			PSXHW_LOG("DMA6 BCR 32bit write %lx\n", value);
			HW_DMA6_BCR  = value; return; // DMA6 bcr
#endif
		case 0x1f8010e8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA6 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA6_CHCR = value;         // DMA6 chcr (OT clear)
			DmaExec(6);
			return;

		case 0x1f801500:
			PSXHW_LOG("DMA7 MADR 32bit write %lx\n", value);
			SPU2WriteMemAddr(1,value);
			HW_DMA7_MADR = value; return; // DMA7 madr
		case 0x1f801504:
			PSXHW_LOG("DMA7 BCR 32bit write %lx\n", value);
			HW_DMA7_BCR  = value; return; // DMA7 bcr
		case 0x1f801508:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA7 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA7_CHCR = value;         // DMA7 chcr (SPU2)
			DmaExec2(7);
			return;

#ifdef PSXHW_LOG
		case 0x1f801510:
			PSXHW_LOG("DMA8 MADR 32bit write %lx\n", value);
			HW_DMA8_MADR = value; return; // DMA8 madr
		case 0x1f801514:
			PSXHW_LOG("DMA8 BCR 32bit write %lx\n", value);
			HW_DMA8_BCR  = value; return; // DMA8 bcr
#endif
		case 0x1f801518:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA8 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA8_CHCR = value;         // DMA8 chcr (DEV9)
			DmaExec2(8);
			return;

#ifdef PSXHW_LOG
		case 0x1f801520:
			PSXHW_LOG("DMA9 MADR 32bit write %lx\n", value);
			HW_DMA9_MADR = value; return; // DMA9 madr
		case 0x1f801524:
			PSXHW_LOG("DMA9 BCR 32bit write %lx\n", value);
			HW_DMA9_BCR  = value; return; // DMA9 bcr
#endif
		case 0x1f801528:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA9 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA9_CHCR = value;         // DMA9 chcr (SIF0)
			DmaExec2(9);
			return;
#ifdef PSXHW_LOG
		case 0x1f80152c:
			PSXHW_LOG("DMA9 TADR 32bit write %lx\n", value);
			HW_DMA9_TADR = value; return; // DMA9 tadr
#endif

#ifdef PSXHW_LOG
		case 0x1f801530:
			PSXHW_LOG("DMA10 MADR 32bit write %lx\n", value);
			HW_DMA10_MADR = value; return; // DMA10 madr
		case 0x1f801534:
			PSXHW_LOG("DMA10 BCR 32bit write %lx\n", value);
			HW_DMA10_BCR  = value; return; // DMA10 bcr
#endif
		case 0x1f801538:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA10 CHCR 32bit write %lx\n", value);
#endif
			HW_DMA10_CHCR = value;         // DMA10 chcr (SIF1)
			DmaExec2(10);
			return;

#ifdef PSXHW_LOG
		case 0x1f801540:
			PSXHW_LOG("DMA11 SIO2in MADR 32bit write %lx\n", value);
			HW_DMA11_MADR = value; return;
		
		case 0x1f801544:
			PSXHW_LOG("DMA11 SIO2in BCR 32bit write %lx\n", value);
			HW_DMA11_BCR  = value; return;
#endif
		case 0x1f801548:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA11 SIO2in CHCR 32bit write %lx\n", value);
#endif
			HW_DMA11_CHCR = value;         // DMA11 chcr (SIO2 in)
			DmaExec2(11);
			return;

#ifdef PSXHW_LOG
		case 0x1f801550:
			PSXHW_LOG("DMA12 SIO2out MADR 32bit write %lx\n", value);
			HW_DMA12_MADR = value; return;
		
		case 0x1f801554:
			PSXHW_LOG("DMA12 SIO2out BCR 32bit write %lx\n", value);
			HW_DMA12_BCR  = value; return;
#endif
		case 0x1f801558:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA12 SIO2out CHCR 32bit write %lx\n", value);
#endif
			HW_DMA12_CHCR = value;         // DMA12 chcr (SIO2 out)
			DmaExec2(12);
			return;

		case 0x1f801570:
			psxHu32(0x1570) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA PCR2 32bit write %lx\n", value);
#endif
			return;
#ifdef PSXHW_LOG
		case 0x1f8010f0:
			PSXHW_LOG("DMA PCR 32bit write %lx\n", value);
			HW_DMA_PCR = value;
			return;
#endif

		case 0x1f8010f4:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA ICR 32bit write %lx\n", value);
#endif
		{
			u32 tmp = (~value) & HW_DMA_ICR;
			HW_DMA_ICR = ((tmp ^ value) & 0xffffff) ^ tmp;
			return;
		}

		case 0x1f801574:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA ICR2 32bit write %lx\n", value);
#endif
		{
			u32 tmp = (~value) & HW_DMA_ICR2;
			HW_DMA_ICR2 = ((tmp ^ value) & 0xffffff) ^ tmp;
			return;
		}

/*		case 0x1f801810:
#ifdef PSXHW_LOG
			PSXHW_LOG("GPU DATA 32bit write %lx\n", value);
#endif
			GPU_writeData(value); return;
		case 0x1f801814:
#ifdef PSXHW_LOG
			PSXHW_LOG("GPU STATUS 32bit write %lx\n", value);
#endif
			GPU_writeStatus(value); return;
*/
/*		case 0x1f801820:
			mdecWrite0(value); break;
		case 0x1f801824:
			mdecWrite1(value); break;
*/
		case 0x1f801100:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 0 COUNT 32bit write %lx\n", value);
#endif
			psxRcntWcount16(0, value ); return;
		case 0x1f801104:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 0 MODE 32bit write %lx\n", value);
#endif
			psxRcnt0Wmode(value); return;
		case 0x1f801108:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 0 TARGET 32bit write %lx\n", value);
#endif
			psxRcntWtarget16(0, value ); return;

		case 0x1f801110:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 1 COUNT 32bit write %lx\n", value);
#endif
			psxRcntWcount16(1, value ); return;
		case 0x1f801114:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 1 MODE 32bit write %lx\n", value);
#endif
			psxRcnt1Wmode(value); return;
		case 0x1f801118:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 1 TARGET 32bit write %lx\n", value);
#endif
			psxRcntWtarget16(1, value ); return;

		case 0x1f801120:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 2 COUNT 32bit write %lx\n", value);
#endif
			psxRcntWcount16(2, value ); return;
		case 0x1f801124:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 2 MODE 32bit write %lx\n", value);
#endif
			psxRcnt2Wmode(value); return;
		case 0x1f801128:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 2 TARGET 32bit write %lx\n", value);
#endif
			psxRcntWtarget16(2, value); return;

		case 0x1f801480:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 3 COUNT 32bit write %lx\n", value);
#endif
			psxRcntWcount32(3, value); return;
		case 0x1f801484:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 3 MODE 32bit write %lx\n", value);
#endif
			psxRcnt3Wmode(value); return;
		case 0x1f801488:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 3 TARGET 32bit write %lx\n", value);
#endif
			psxRcntWtarget32(3, value); return;

		case 0x1f801490:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 4 COUNT 32bit write %lx\n", value);
#endif
			psxRcntWcount32(4, value); return;
		case 0x1f801494:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 4 MODE 32bit write %lx\n", value);
#endif
			psxRcnt4Wmode(value); return;
		case 0x1f801498:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 4 TARGET 32bit write %lx\n", value);
#endif
			psxRcntWtarget32(4, value); return;

		case 0x1f8014a0:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 5 COUNT 32bit write %lx\n", value);
#endif
			psxRcntWcount32(5, value); return;
		case 0x1f8014a4:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 5 MODE 32bit write %lx\n", value);
#endif
			psxRcnt5Wmode(value); return;
		case 0x1f8014a8:
#ifdef PSXCNT_LOG
			PSXCNT_LOG("COUNTER 5 TARGET 32bit write %lx\n", value);
#endif
			psxRcntWtarget32(5, value); return;

		case 0x1f8014c0:
#ifdef PSXHW_LOG
			PSXHW_LOG("RTC_HOLDMODE 32bit write %lx\n", value);
#endif
			SysPrintf("RTC_HOLDMODE 32bit write %lx\n", value);
			break;

		case 0x1f801450:
#ifdef PSXHW_LOG
			if (value) { PSXHW_LOG("%08X ICFG 32bit write %lx\n", psxRegs.pc, value); }
#endif
/*			if (value &&
				psxSu32(0x20) == 0x20000 &&
				(psxSu32(0x30) == 0x20000 ||
				 psxSu32(0x30) == 0x40000)) { // don't ask me why :P
				psxSu32(0x20) = 0x10000;
				psxSu32(0x30) = 0x10000;
			}*/
			psxHu32(0x1450) = /*(*/value/* & (~0x8)) | (psxHu32(0x1450) & 0x8)*/;
			return;

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
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 write param[%d] <- %lx\n", (add-0x1F808200)/4, value);
#endif
			sio2_setSend3((add-0x1F808200)/4, value);	return;

		case 0x1F808240:
		case 0x1F808248:
		case 0x1F808250:
		case 0x1F808258:
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 write send1[%d] <- %lx\n", (add-0x1F808240)/8, value);
#endif
			sio2_setSend1((add-0x1F808240)/8, value);	return;

		case 0x1F808244:
		case 0x1F80824C:
		case 0x1F808254:
		case 0x1F80825C:
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 write send2[%d] <- %lx\n", (add-0x1F808244)/8, value);
#endif
			sio2_setSend2((add-0x1F808244)/8, value);	return;

		case 0x1F808268:
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 write CTRL <- %lx\n", value);
#endif
			sio2_setCtrl(value);	return;

		case 0x1F808278:
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 write [8278] <- %lx\n", value);
#endif
			sio2_set8278(value);	return;

		case 0x1F80827C:
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 write [827C] <- %lx\n", value);
#endif
			sio2_set827C(value);	return;

		case 0x1F808280:
#ifdef PSXHW_LOG
			PSXHW_LOG("SIO2 write INTR <- %lx\n", value);
#endif
			sio2_setIntr(value);	return;

		default:
			psxHu32(add) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unknown 32bit write at address %lx value %lx\n", add, value);
#endif
			return;
	}
	psxHu32(add) = value;
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 32bit write at address %lx value %lx\n", add, value);
#endif
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
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unkwnown 8bit read at address %lx\n", add);
#endif
			SysPrintf("*Unkwnown 8bit read at address %lx\n", add);
			return 0;
	}
	
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 8bit read at address %lx value %x\n", add, hard);
#endif

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
		case 0x1f402016:
			cdvdWrite16(value);
			FreezeMMXRegs(0);
			FreezeXMMRegs(0)
			return;
		case 0x1f402017: cdvdWrite17(value); return;
		case 0x1f402018: cdvdWrite18(value); return;
		case 0x1f40203A: cdvdWrite3A(value); return;
		default:
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unknown 8bit write at address %lx value %x\n", add, value);
#endif
			SysPrintf("*Unknown 8bit write at address %lx value %x\n", add, value);
			return;
	}
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 8bit write at address %lx value %x\n", add, value);
#endif
}

void psxDmaInterrupt(int n) {
	if (HW_DMA_ICR & (1 << (16 + n))) {
		HW_DMA_ICR|= (1 << (24 + n));
		psxRegs.CP0.n.Cause |= 1 << (9 + n);
		psxHu32(0x1070) |= 8;
		
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
		psxHu32(0x1070) |= 8;
	}
}
