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
#include "IopCommon.h"
#include "sio_internal.h"

sio2Struct sio2;

/*
w [8268]=0x3bc		sio2_start/sio2man
r [8270]		padman_start/padman
	padman->7480[00]=bit4;
	padman->7480[13]=bit5;
  packetExchange(&703F8);
w [8268]|=0x0C;
........
w [8268]|=0x01;

only recv2 & dataout influences padman
*/

//		0xBF808200,0xBF808204,0xBF808208,0xBF80820C,
//		0xBF808210,0xBF808214,0xBF808218,0xBF80821C,	packet->sendArray3
//		0xBF808220,0xBF808224,0xBF808228,0xBF80822C,	call12/13_s/getparams
//		0xBF808230,0xBF808234,0xBF808238,0xBF80823C,

//		0xBF808240,0xBF808248,0xBF808250,0xBF808258,	packet->sendArray1/call_7/8
//		0xBF808244,0xBF80824C,0xBF808254,0xBF80825C,	packet->sendArray2/call_9/10

//		0xBF808260,										serial data/fifo in/out s/get8260_datain/out packet->sendbuf(nomem!)
//                            0xBF808268,				ctrl s/get8268_ctrl

//                                       0xBF80826C,	packet->recv1/2/3 get826C_recv1, get8270_recv2, get8274_recv3
//		0xBF808270,0xBF808274,

//                            0xBF808278,0xBF80827C,	s/get8278, s/get827C
//		0xBF808280										interrupt related s/get8280_intr


void sio2Reset() {
	DevCon::Status( "Sio2 Reset" );
	memzero_obj(sio2);
	sio2.packet.recvVal1 = 0x1D100; // Nothing is connected at start
}

u32 sio2_getRecv1() {
	PAD_LOG("Reading Recv1 = %x",sio2.packet.recvVal1);

	return sio2.packet.recvVal1;
}

u32 sio2_getRecv2() {
	PAD_LOG("Reading Recv2 = %x",0xF);

	return 0xf;
}//0, 0x10, 0x20, 0x10 | 0x20; bits 4 & 5

u32 sio2_getRecv3() { 
	if(sio2.packet.recvVal3 == 0x8C || sio2.packet.recvVal3 == 0x8b ||
	   sio2.packet.recvVal3 == 0x83)
	{
		PAD_LOG("Reading Recv3 = %x",sio2.packet.recvVal3);

		sio.packetsize = sio2.packet.recvVal3;
		sio2.packet.recvVal3 = 0; // Reset
		return sio.packetsize;
	}
	else
	{
		PAD_LOG("Reading Recv3 = %x",sio.packetsize << 16);

		return sio.packetsize << 16;
	}
}

void sio2_setSend1(u32 index, u32 value){sio2.packet.sendArray1[index]=value;}	//0->3
u32 sio2_getSend1(u32 index){return sio2.packet.sendArray1[index];}				//0->3
void sio2_setSend2(u32 index, u32 value){sio2.packet.sendArray2[index]=value;}	//0->3
u32 sio2_getSend2(u32 index){return sio2.packet.sendArray2[index];}				//0->3

void sio2_setSend3(u32 index, u32 value) 
{
//	int i;
	sio2.packet.sendArray3[index]=value;
//	if (index==15){
//		for (i=0; i<4; i++){PAD_LOG("0x%08X ", sio2.packet.sendArray1[i]);}PAD_LOG("\n");
//		for (i=0; i<4; i++){PAD_LOG("0x%08X ", sio2.packet.sendArray2[i]);}PAD_LOG("\n");
//		for (i=0; i<8; i++){PAD_LOG("0x%08X ", sio2.packet.sendArray3[i]);}PAD_LOG("\n");
//		for (  ; i<16; i++){PAD_LOG("0x%08X ", sio2.packet.sendArray3[i]);}PAD_LOG("\n");
	PAD_LOG("[%d] : 0x%08X", index,sio2.packet.sendArray3[index]);
//	}
}	//0->15

u32 sio2_getSend3(u32 index) {return sio2.packet.sendArray3[index];}				//0->15

void sio2_setCtrl(u32 value){
	sio2.ctrl=value;
	if (sio2.ctrl & 1){	//recv packet
		//handle data that had been sent

		iopIntcIrq( 17 );
		//SBUS
		sio2.recvIndex=0;
		sio2.ctrl &= ~1;
	} else { // send packet
		//clean up
		sio2.packet.sendSize=0;	//reset size
		sio2.cmdport=0;
		sio2.cmdlength=0;
		sioWriteCtrl16(SIO_RESET);
	}
}
u32 sio2_getCtrl(){return sio2.ctrl;}

void sio2_setIntr(u32 value){sio2.intr=value;}
u32 sio2_getIntr(){
	return sio2.intr;
}

void sio2_set8278(u32 value){sio2._8278=value;}
u32 sio2_get8278(){return sio2._8278;}
void sio2_set827C(u32 value){sio2._827C=value;}
u32 sio2_get827C(){return sio2._827C;}

void sio2_serialIn(u8 value){
	u16 ctrl=0x0002;
	if (sio2.packet.sendArray3[sio2.cmdport] && (sio2.cmdlength==0))
	{
		
		sio2.cmdlength=(sio2.packet.sendArray3[sio2.cmdport] >> 8) & 0x1FF;
		ctrl &= ~0x2000;
		ctrl |= (sio2.packet.sendArray3[sio2.cmdport] & 1) << 13;
		//sioWriteCtrl16(SIO_RESET);
		sioWriteCtrl16(ctrl);
		PSXDMA_LOG("sio2_fifoIn: ctrl = %x, cmdlength = %x, cmdport = %d (%x)", ctrl, sio2.cmdlength, sio2.cmdport, sio2.packet.sendArray3[sio2.cmdport]);

		sio2.cmdport++;
	}

	if (sio2.cmdlength) sio2.cmdlength--;
	sioWrite8(value);
	
	if (sio2.packet.sendSize > BUFSIZE) {//asadr
		Console::Notice("*PCSX2*: sendSize >= %d", params BUFSIZE);
	} else {
		sio2.buf[sio2.packet.sendSize] = sioRead8();
		sio2.packet.sendSize++;
	}
}
extern void SIODMAWrite(u8 value);

void sio2_fifoIn(u8 value){
	u16 ctrl=0x0002;
	if (sio2.packet.sendArray3[sio2.cmdport] && (sio2.cmdlength==0))
	{
		
		sio2.cmdlength=(sio2.packet.sendArray3[sio2.cmdport] >> 8) & 0x1FF;
		ctrl &= ~0x2000;
		ctrl |= (sio2.packet.sendArray3[sio2.cmdport] & 1) << 13;
		//sioWriteCtrl16(SIO_RESET);
		sioWriteCtrl16(ctrl);
		PSXDMA_LOG("sio2_fifoIn: ctrl = %x, cmdlength = %x, cmdport = %d (%x)", ctrl, sio2.cmdlength, sio2.cmdport, sio2.packet.sendArray3[sio2.cmdport]);

		sio2.cmdport++;
	}

	if (sio2.cmdlength) sio2.cmdlength--;
	SIODMAWrite(value);
	
	if (sio2.packet.sendSize > BUFSIZE) {//asadr
		Console::WriteLn("*PCSX2*: sendSize >= %d", params BUFSIZE);
	} else {
		sio2.buf[sio2.packet.sendSize] = sioRead8();
		sio2.packet.sendSize++;
	}
}

u8 sio2_fifoOut(){
	if (sio2.recvIndex <= sio2.packet.sendSize){
		//PAD_LOG("READING %x\n",sio2.buf[sio2.recvIndex]);
		return sio2.buf[sio2.recvIndex++];
	} else {
		Console::Error( "*PCSX2*: buffer overrun" );
	}
	return 0; // No Data
}

void SaveState::sio2Freeze()
{
	FreezeTag( "sio2" );
	Freeze(sio2);
}

/////////////////////////////////////////////////
////////////////////////////////////////////  DMA
/////////////////////////////////////////////////

void psxDma11(u32 madr, u32 bcr, u32 chcr) {
	unsigned int i, j;
	int size = (bcr >> 16) * (bcr & 0xffff);
	PSXDMA_LOG("*** DMA 11 - SIO2 in *** %lx addr = %lx size = %lx", chcr, madr, bcr);
	
	if (chcr != 0x01000201) return;

	for(i = 0; i < (bcr >> 16); i++)
	{
		sio.count = 1;
		for(j = 0; j < ((bcr & 0xFFFF) * 4); j++)
		{
			sio2_fifoIn(iopMemRead8(madr));
			madr++;
			if(sio2.packet.sendSize == BUFSIZE)
				goto finished;
		}
	}

finished:
	HW_DMA11_MADR = madr;
	PSX_INT(IopEvt_Dma11,(size>>2));	// Interrupts should always occur at the end
}

void psxDMA11Interrupt()
{
	HW_DMA11_CHCR &= ~0x01000000;
	psxDmaInterrupt2(4);
}

void psxDma12(u32 madr, u32 bcr, u32 chcr) {
	int size = ((bcr >> 16) * (bcr & 0xFFFF)) * 4;
	PSXDMA_LOG("*** DMA 12 - SIO2 out *** %lx addr = %lx size = %lx", chcr, madr, size);

	if (chcr != 0x41000200) return;

	sio2.recvIndex = 0; // Set To start;    saqib

	bcr = size;
	while (bcr > 0) {
		iopMemWrite8( madr, sio2_fifoOut() );
		bcr--; madr++;
		if(sio2.recvIndex == sio2.packet.sendSize) break;
	}
	HW_DMA12_MADR = madr;
	PSX_INT(IopEvt_Dma12,(size>>2));	// Interrupts should always occur at the end
}

void psxDMA12Interrupt()
{
	HW_DMA12_CHCR &= ~0x01000000;
	psxDmaInterrupt2(5);
}

