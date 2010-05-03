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

#ifndef __PSXSIO2_H__
#define __PSXSIO2_H__


#define BUFSIZE	8448

//from sio2man.c

struct SIO2_packet {
    unsigned int recvVal1; // 0x00
    unsigned int sendArray1[4]; // 0x04-0x10
    unsigned int sendArray2[4]; // 0x14-0x20

    unsigned int recvVal2; // 0x24

    unsigned int sendArray3[16]; // 0x28-0x64

    unsigned int recvVal3; // 0x68

    int sendSize; // 0x6C
    int recvSize; // 0x70

    unsigned char *sendBuf; // 0x74
    unsigned char *recvBuf; // 0x78

    unsigned int dmacAddress1;
    unsigned int dmacSize1;
    unsigned int dmacCount1;
    unsigned int dmacAddress2;
    unsigned int dmacSize2;
    unsigned int dmacCount2;
};

struct sio2Struct {
	struct SIO2_packet packet;
	u32 ctrl;
	u32 intr;
	u32 _8278, _827C;
	int recvIndex;
	u32 hackedRecv;
	int cmdport;
	int cmdlength;	//length of a command sent to a port
					//is less_equal than the dma send size
	u8 buf[BUFSIZE];
};

extern sio2Struct sio2;

void sio2Reset();

u32  sio2_getRecv1();
u32  sio2_getRecv2();
u32  sio2_getRecv3();
void sio2_setSend1(u32 index, u32 value);	//0->3
u32  sio2_getSend1(u32 index);				//0->3
void sio2_setSend2(u32 index, u32 value);	//0->3
u32  sio2_getSend2(u32 index);				//0->3
void sio2_setSend3(u32 index, u32 value);	//0->15
u32  sio2_getSend3(u32 index);				//0->15

void sio2_setCtrl(u32 value);
u32  sio2_getCtrl();
void sio2_setIntr(u32 value);
u32  sio2_getIntr();
void sio2_set8278(u32 value);
u32  sio2_get8278();
void sio2_set827C(u32 value);
u32  sio2_get827C();

void sio2_serialIn(u8 value);
void sio2_fifoIn(u8 value);
u8   sio2_fifoOut();

void psxDma11(u32 madr, u32 bcr, u32 chcr);
void psxDma12(u32 madr, u32 bcr, u32 chcr);

void psxDMA11Interrupt();
void psxDMA12Interrupt();

#endif /* __PSXSIO2_H__ */

