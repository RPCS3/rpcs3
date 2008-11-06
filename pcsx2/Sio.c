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
#include <stdlib.h>
#include <sys/stat.h>

#include "PsxCommon.h"

#include "Paths.h"

#ifdef _WIN32
#pragma warning(disable:4244)
#endif

FILE * MemoryCard1, * MemoryCard2;

const unsigned char cardh[4] = { 0xFF, 0xFF, 0x5a, 0x5d };
// Memory Card Specs : Sector size etc.
struct mc_command_0x26_tag mc_command_0x26= {'+', 512, 16, 0x4000, 0x52, 0x5A};
#define SIO_INT() PSX_INT(16, PSXCLK/250000); /*270;*/

void _ReadMcd(char *data, u32 adr, int size) {
	ReadMcd(sio.CtrlReg&0x2000?2:1, data, adr, size);
}

void _SaveMcd(char *data, u32 adr, int size) {
	SaveMcd(sio.CtrlReg&0x2000?2:1, data, adr, size);
}

void _EraseMCDBlock(u32 adr) {
	EraseMcd(sio.CtrlReg&0x2000?2:1, adr);
}

unsigned char xor(unsigned char *buf, unsigned int length){
	register unsigned char i, x;

	for (x=0, i=0; i<length; i++)	x ^= buf[i];
	return x & 0xFF;
}

int sioInit() {
	memset(&sio, 0, sizeof(sio));
	
	MemoryCard1 = LoadMcd(1);
	MemoryCard2 = LoadMcd(2);

	// Transfer(?) Ready and the Buffer is Empty
	sio.StatReg = TX_RDY | TX_EMPTY;
	sio.packetsize = 0;
	sio.terminator =0x55; // Command terminator 'U'

	return 0;
}

void psxSIOShutdown()
{
	if(MemoryCard1) fclose(MemoryCard1);
	if(MemoryCard2) fclose(MemoryCard2);
}

unsigned char sioRead8() {
	unsigned char ret = 0xFF;

	if (sio.StatReg & RX_RDY) {
		ret = sio.buf[sio.parp];
		if (sio.parp == sio.bufcount) {
			sio.StatReg &= ~RX_RDY;		// Receive is not Ready now?
			sio.StatReg |= TX_EMPTY;	// Buffer is Empty

			if (sio.padst == 2) sio.padst = 0;
			/*if (sio.mcdst == 1) {
				sio.mcdst = 99;
				sio.StatReg&= ~TX_EMPTY;
				sio.StatReg|= RX_RDY;
			}*/
		}
	}

#ifdef PAD_LOG
		//PAD_LOG("sio read8 ;ret = %x\n", ret);
#endif
	return ret;
}

void SIO_CommandWrite(u8 value,int way) {
#ifdef PAD_LOG
	PAD_LOG("sio write8 %x\n", value);
#endif

	// PAD COMMANDS
	switch (sio.padst) {
		case 1: SIO_INT();
			if ((value&0x40) == 0x40) {
				sio.padst = 2; sio.parp = 1;
				switch (sio.CtrlReg&0x2002) {
					case 0x0002:
						sio.packetsize ++;	// Total packet size sent
						sio.buf[sio.parp] = PAD1poll(value);
						break;
					case 0x2002:
						sio.packetsize ++;	// Total packet size sent
						sio.buf[sio.parp] = PAD2poll(value);
						break;
				}
				if (!(sio.buf[sio.parp] & 0x0f)) {
					sio.bufcount = 2 + 32;
				} else {
					sio.bufcount = 2 + (sio.buf[sio.parp] & 0x0f) * 2;
				}
			}
			else sio.padst = 0;
			return;
		case 2:
			sio.parp++;
			switch (sio.CtrlReg&0x2002) {
				case 0x0002: sio.packetsize ++; sio.buf[sio.parp] = PAD1poll(value); break;
				case 0x2002: sio.packetsize ++; sio.buf[sio.parp] = PAD2poll(value); break;
			}
			if (sio.parp == sio.bufcount) { sio.padst = 0; return; }
			SIO_INT();
			return;
	}

	// MEMORY CARD COMMANDS
	switch (sio.mcdst) {
		case 1:
			sio.packetsize++;
			SIO_INT();
			if (sio.rdwr) { sio.parp++; return; }
			sio.parp = 1;
			switch (value) {
			case 0x11: // RESET
				PAD_LOG("RESET MEMORY CARD\n");
				sio.bufcount =  8; 
				memset(sio.buf, 0xFF, 256);
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio.mcdst = 99; 
				sio2.packet.recvVal3 = 0x8c;
				break;
			case 0x12: // RESET
				sio.bufcount =  8; 
				memset(sio.buf, 0xFF, 256);
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio.mcdst = 99; 

				sio2.packet.recvVal3 = 0x8c;
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			case 0x81: // COMMIT
				sio.bufcount =  8; 
				memset(sio.buf, 0xFF, 256);
				sio.mcdst = 99; 
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio2.packet.recvVal3 = 0x8c;
				if(value == 0x81) {
					if(sio.mc_command==0x42)
						sio2.packet.recvVal1 = 0x1600; // Writing
					else if(sio.mc_command==0x43) sio2.packet.recvVal1 = 0x1700; // Reading
				}
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			case 0x21: 
			case 0x22: 
			case 0x23: // SECTOR SET
                sio.bufcount =  8; sio.mcdst = 99; sio.sector=0; sio.k=0;
				memset(sio.buf, 0xFF, 256);
				sio2.packet.recvVal3 = 0x8c; 
				sio.buf[8]=sio.terminator;
				sio.buf[7]='+';
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif

				break;
			case 0x24:											break;
			case 0x25:											break;
			case 0x26: 
				sio.bufcount = 12; sio.mcdst = 99; sio2.packet.recvVal3 = 0x83;	
				memset(sio.buf, 0xFF, 256);
				memcpy(&sio.buf[2], &mc_command_0x26, sizeof(mc_command_0x26));
				sio.buf[12]=sio.terminator;
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			case 0x27: 
			case 0x28: 
			case 0xBF:
				sio.bufcount =  4; sio.mcdst = 99; sio2.packet.recvVal3 = 0x8b;
				memset(sio.buf, 0xFF, 256);
				sio.buf[4]=sio.terminator;
				sio.buf[3]='+';
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			case 0x42: // WRITE
			case 0x43: // READ
			case 0x82:
				if(value==0x82 && sio.lastsector==sio.sector) sio.mode = 2;
				if(value==0x42) sio.mode = 0;
				if(value==0x43) sio.lastsector = sio.sector; // Reading 
				
				sio.bufcount =133; sio.mcdst = 99;
				memset(sio.buf, 0xFF, 256);
				sio.buf[133]=sio.terminator;
				sio.buf[132]='+';
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			case 0xf0:
			case 0xf1:
			case 0xf2:				  
				sio.mcdst = 99;	
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			case 0xf3:
			case 0xf7: 
				sio.bufcount = 4; sio.mcdst = 99;
				memset(sio.buf, 0xFF, 256);
				sio.buf[4]=sio.terminator;
				sio.buf[3]='+';
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			case 0x52:
				sio.rdwr = 1; memset(sio.buf, 0xFF, 256);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			case 0x57:
				sio.rdwr = 2; memset(sio.buf, 0xFF, 256);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';	
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				break;
			default:
				sio.mcdst = 0;
				memset(sio.buf, 0xFF, 256);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';	
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("Unknown MC(%d) command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif	
			}
			sio.mc_command=value;
			return;
		// FURTHER PROCESSING OF THE MEMORY CARD COMMANDS
		case 99:
			sio.packetsize++;
			sio.parp++;
			switch(sio.mc_command)
			{
			// SET_ERASE_PAGE; the next erase commands will *clear* data starting with the page set here
			case 0x21:
			// SET_WRITE_PAGE; the next write commands will commit data starting with the page set here
			case 0x22:
			// SET_READ_PAGE; the next read commands will return data starting with the page set here
			case 0x23:
                if (sio.parp==2)sio.sector|=(value & 0xFF)<< 0;
				if (sio.parp==3)sio.sector|=(value & 0xFF)<< 8;
				if (sio.parp==4)sio.sector|=(value & 0xFF)<<16;
				if (sio.parp==5)sio.sector|=(value & 0xFF)<<24;
				if (sio.parp==6)
				{
#ifdef MEMCARDS_LOG
                 MEMCARDS_LOG("MC(%d) SET PAGE sio.sector 0x%04X\n",
								((sio.CtrlReg&0x2000)>>13)+1, sio.sector);
#endif
				}
				break;

			// SET_TERMINATOR; reads the new terminator code
			case 0x27:
				if(sio.parp==2)	{
					sio.terminator = value;
					sio.buf[4] = value;
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) SET TERMINATOR command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif

				}
				break;

			// GET_TERMINATOR; puts in position 3 the current terminator code and in 4 the default one
			//                                                                  depending on the param
			case 0x28:
				if(sio.parp == 2) {
					sio.buf[2] = '+';
					sio.buf[3] = sio.terminator;
					
					if(value == 0) sio.buf[4] = 0xFF;
					else sio.buf[4] = 0x55;
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) GET TERMINATOR command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				}	
				break;
			// WRITE DATA
			case 0x42:
				if (sio.parp==2) {
					sio.bufcount=5+value;
					memset(sio.buf, 0xFF, 256);
					sio.buf[sio.bufcount-1]='+';
					sio.buf[sio.bufcount]=sio.terminator;
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) WRITE command 0x%02X\n\n\n\n\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				} else
				if ((sio.parp>2) && (sio.parp<sio.bufcount-2)) {
					sio.buf[sio.parp]=value;
#ifdef MEMCARDS_LOG
				//MEMCARDS_LOG("MC(%d) WRITING 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				} else
				if (sio.parp==sio.bufcount-2) {
					if (xor(&sio.buf[3], sio.bufcount-5)==value) {
                        _SaveMcd(&sio.buf[3], (512+16)*sio.sector+sio.k, sio.bufcount-5);
						sio.buf[sio.bufcount-1]=value;
						sio.k+=sio.bufcount-5;
					}else {
#ifdef MEMCARDS_LOG
						MEMCARDS_LOG("MC(%d) write XOR value error 0x%02X != ^0x%02X\n",
							((sio.CtrlReg&0x2000)>>13)+1, value, xor(&sio.buf[3], sio.bufcount-5));
#endif
					}
				}
				break;
			// READ DATA
			case 0x43:
				if (sio.parp==2){
					//int i;
					sio.bufcount=value+5;
					sio.buf[3]='+';
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) READ command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
					_ReadMcd(&sio.buf[4], (512+16)*sio.sector+sio.k, value);
					if(sio.mode==2) 
					{
						int j;
						for(j=0; j < value; j++)
							sio.buf[4+j] = ~sio.buf[4+j];
					}

					sio.k+=value;
					sio.buf[sio.bufcount-1]=xor(&sio.buf[4], value);
					sio.buf[sio.bufcount]=sio.terminator;
				}
				break;
			// INTERNAL ERASE
			case 0x82:
				if(sio.parp==2) {
					sio.buf[2]='+';
					sio.buf[3]=sio.terminator;
				/*	memset(sio.buf, -1, 256);
					_SaveMcd(sio.buf, (512+16)*sio.sector, 256);
					_SaveMcd(sio.buf, (512+16)*sio.sector+256, 256);
					_SaveMcd(sio.buf, (512+16)*sio.sector+512, 16);
					sio.buf[2]='+';
				*/	sio.buf[3]=sio.terminator;
					//sio.buf[sio.bufcount] = sio.terminator;
#ifdef MEMCARDS_LOG
				MEMCARDS_LOG("MC(%d) INTERNAL ERASE command 0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
				}
				break;
			// CARD AUTHENTICATION CHECKS
			case 0xF0:
				if (sio.parp==2)
				{
#ifdef MEMCARDS_LOG
					MEMCARDS_LOG("MC(%d) CARD AUTH :0x%02X\n", ((sio.CtrlReg&0x2000)>>13)+1, value);
#endif
					switch(value){
					case  1:
					case  2:
					case  4:
					case 15:
					case 17:
					case 19:
						sio.bufcount=13;
						memset(sio.buf, 0xFF, 256);
						sio.buf[12] = 0; // Xor value of data from index 4 to 11
						sio.buf[3]='+';
						sio.buf[13] = sio.terminator;
						break;
					case  6:
					case  7:
					case 11:
						sio.bufcount=13;
						memset(sio.buf, 0xFF, 256);
						sio.buf[12]='+';
						sio.buf[13] = sio.terminator;
						break;
					default:
						sio.bufcount=4;
						memset(sio.buf, 0xFF, 256);
						sio.buf[3]='+';
						sio.buf[4] = sio.terminator;
					}
				}
				break;
			}
			if (sio.bufcount<=sio.parp)	sio.mcdst = 0;
			return;
	}

	switch (sio.mtapst) 
	{
		case 0x1:
			sio.packetsize++;
			sio.parp = 1;
			SIO_INT();
			switch(value) {
			case 0x12:  sio.mtapst = 2; sio.bufcount = 5;	break;
			case 0x13:  sio.mtapst = 2; sio.bufcount = 5;	break;
			case 0x21:	sio.mtapst = 2; sio.bufcount = 6;	break;
			}
			sio.buf[sio.bufcount]='Z';
			sio.buf[sio.bufcount-1]='+';
			return;
		case 0x2:
			sio.packetsize++;
			sio.parp++;
            if (sio.bufcount<=sio.parp)	sio.mcdst = 0;
			SIO_INT();
			return;
	}

	
	if(sio.count == 1 || way == 0) InitializeSIO(value);
}

void InitializeSIO(u8 value)
{
	switch (value) {
		case 0x01: // start pad
			sio.StatReg &= ~TX_EMPTY;	// Now the Buffer is not empty
			sio.StatReg |= RX_RDY;		// Transfer is Ready

			switch (sio.CtrlReg&0x2002) {
				case 0x0002: sio.buf[0] = PAD1startPoll(1); break;
				case 0x2002: sio.buf[0] = PAD2startPoll(2); break;
			}

			sio.bufcount = 2;
			sio.parp = 0;
			sio.padst = 1;
			sio.packetsize = 1;
			sio.count = 0;
			sio2.packet.recvVal1 = 0x1100; // Pad is present
			SIO_INT();
			return;

		case 0x21: // start mtap
			sio.StatReg &= ~TX_EMPTY;	// Now the Buffer is not empty
			sio.StatReg |= RX_RDY;		// Transfer is Ready
			sio.parp = 0;
			sio.packetsize = 1;
			sio.mtapst = 1;
			sio.count = 0;
			sio2.packet.recvVal1 = 0x1D100; // Mtap is not connected :)
			SIO_INT();
			return;

		case 0x61: // start remote control sensor
			sio.StatReg &= ~TX_EMPTY;	// Now the Buffer is not empty
			sio.StatReg |= RX_RDY;		// Transfer is Ready
			sio.parp = 0;
			sio.packetsize = 1;
			sio.count = 0;
			sio2.packet.recvVal1 = 0x1100; // Pad is present
			SIO_INT();
			return;

		case 0x81: // start memcard
			sio.StatReg &= ~TX_EMPTY;
			sio.StatReg |= RX_RDY;
			memcpy(sio.buf, cardh, 4);
			sio.parp = 0;
			sio.bufcount = 8;
			sio.mcdst = 1;
			sio.packetsize = 1;
			sio.rdwr = 0;
			sio2.packet.recvVal1 = 0x1100; // Memcards are present
			sio.count = 0;
			SIO_INT();
			PAD_LOG("START MEMORY CARD\n");
			return;
	}
}

void sioWrite8(unsigned char value)
{
	SIO_CommandWrite(value,0);
}

void SIODMAWrite(u8 value)
{
	SIO_CommandWrite(value,1);
}

void sioWriteCtrl16(unsigned short value) {
	sio.CtrlReg = value & ~RESET_ERR;
	if (value & RESET_ERR) sio.StatReg &= ~IRQ;
	if ((sio.CtrlReg & SIO_RESET) || (!sio.CtrlReg)) {
		sio.mtapst = 0; sio.padst = 0; sio.mcdst = 0; sio.parp = 0;
		sio.StatReg = TX_RDY | TX_EMPTY;
		psxRegs.interrupt&= ~(1<<16);
	}
}

void  sioInterrupt() {
#ifdef PAD_LOG
	PAD_LOG("Sio Interrupt\n");
#endif
	sio.StatReg|= IRQ;
	psxHu32(0x1070)|=0x80;
	psxRegs.interrupt&= ~(1 << 16);
}

extern u32 dwCurSaveStateVer;
int sioFreeze(gzFile f, int Mode) {

    int savesize = sizeof(sio);
    if( Mode == 0 && dwCurSaveStateVer == 0x7a30000e )
        savesize -= 4;
    sio.count = 0;
    gzfreeze(&sio, savesize);

	return 0;
}


/*******************************************************************
 *******************************************************************
 ***************   MEMORY CARD SPECIFIC FUNTIONS  ******************
 *******************************************************************
 *******************************************************************/
FILE *LoadMcd(int mcd) {
	char str[256];
	FILE *f;

	if (mcd == 1) {
		strcpy(str, Config.Mcd1);
	} else {
		strcpy(str, Config.Mcd2);
	}
	if (*str == 0) sprintf(str, MEMCARDS_DIR "/Mcd00%d.ps2", mcd);
	f = fopen(str, "r+b");
	if (f == NULL) {
		CreateMcd(str);
		f = fopen(str, "r+b");
	}
	if (f == NULL) {
		SysMessage (_("Failed loading MemCard %s"), str); return NULL;
	}

	return f;
}

void SeekMcd(FILE *f, u32 adr) {
	u32 size;

	fseek(f, 0, SEEK_END); size = ftell(f);
	if (size == MCD_SIZE + 64)
		fseek(f, adr + 64, SEEK_SET);
	else if (size == MCD_SIZE + 3904)
		fseek(f, adr + 3904, SEEK_SET);
	else
		fseek(f, adr, SEEK_SET);
}

void ReadMcd(int mcd, char *data, u32 adr, int size) {
	if(mcd == 1)
	{
		if (MemoryCard1 == NULL) {
			memset(data, 0, size);
			return;
		}
		SeekMcd(MemoryCard1, adr);
		fread(data, 1, size, MemoryCard1);
	}
	else
	{
		if (MemoryCard2 == NULL) {
			memset(data, 0, size);
			return;
		}
		SeekMcd(MemoryCard2, adr);
		fread(data, 1, size, MemoryCard2);
	}
}

void SaveMcd(int mcd, char *data, u32 adr, int size) {
	if(mcd == 1)
	{
		SeekMcd(MemoryCard1, adr);
		fwrite(data, 1, size, MemoryCard1);
	}
	else
	{
		SeekMcd(MemoryCard2, adr);
		fwrite(data, 1, size, MemoryCard2);
	}		
}


void EraseMcd(int mcd, u32 adr) {
	u8 data[528*16];
	memset(data,-1,528*16);
	if(mcd == 1)
	{
		SeekMcd(MemoryCard1, adr);
		fwrite(data, 1, 528*16, MemoryCard1);
	}
	else
	{
		SeekMcd(MemoryCard2, adr);
		fwrite(data, 1, 528*16, MemoryCard2);
	}		
}


void CreateMcd(char *mcd) {
	FILE *fp;	
	int i=0, j=0;
	int enc[16] = {0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0,0,0,0};

	fp = fopen(mcd, "wb");
	if (fp == NULL) return;
	for(i=0; i < 16384; i++) 
	{
		for(j=0; j < 512; j++) fputc(0xFF,fp);
		for(j=0; j < 16; j++) fputc(enc[j],fp);
	}
	fclose(fp);
}



