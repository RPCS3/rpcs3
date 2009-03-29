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
#include "MemoryCard.h"

_sio sio;

static const u8 cardh[4] = { 0xFF, 0xFF, 0x5a, 0x5d };

// Memory Card Specs : Sector size etc.
static const mc_command_0x26_tag mc_command_0x26= {'+', 512, 16, 0x4000, 0x52, 0x5A};

static int m_PostSavestateCards[2] = { 0, 0 };

// SIO Inline'd IRQs : Calls the SIO interrupt handlers directly instead of
// feeding them through the IOP's branch test. (see SIO.H for details)

#ifdef SIO_INLINE_IRQS
#define SIO_INT() sioInterrupt()
#define SIO_FORCEINLINE
#else
__forceinline void SIO_INT()
{
	if( !(psxRegs.interrupt & (1<<IopEvt_SIO)) )
		PSX_INT(IopEvt_SIO, 64 ); // PSXCLK/250000);
}
#define SIO_FORCEINLINE __forceinline
#endif

// Currently only check if pad wants mtap to be active.
// Could lets PCSX2 have its own options, if anyone ever
// wants to add support for using the extra memcard slots.
static bool IsMtapPresent( uint port ) {
	switch(port) {
		case 1: return 0 != PAD1queryMtap(port);
		case 2: return 0 != PAD2queryMtap(port);
	}
	return 0;
}

static void _ReadMcd(u8 *data, u32 adr, int size) {
	MemoryCard::Read(sio.GetMemcardIndex(), data, adr, size);
}

static void _SaveMcd(const u8 *data, u32 adr, int size) {
	MemoryCard::Save(sio.GetMemcardIndex(), data, adr, size);
}

static void _EraseMCDBlock(u32 adr) {
	MemoryCard::Erase(sio.GetMemcardIndex(), adr);
}

u8 sio_xor(u8 *buf, uint length){
	u8 i, x;

	for (x=0, i=0; i<length; i++)	x ^= buf[i];
	return x & 0xFF;
}

void sioInit()
{
	memzero_obj(sio);
	memzero_obj(m_PostSavestateCards);

	// Transfer(?) Ready and the Buffer is Empty
	sio.StatReg = TX_RDY | TX_EMPTY;
	sio.packetsize = 0;
	sio.terminator =0x55; // Command terminator 'U'

	MemoryCard::Init();
}

void psxSIOShutdown()
{
	MemoryCard::Shutdown();
}

u8 sioRead8() {
	u8 ret = 0xFF;

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
		//PAD_LOG("sio read8 ;ret = %x", ret);
	return ret;
}

void SIO_CommandWrite(u8 value,int way) {
	PAD_LOG("sio write8 %x", value);

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
		case 3:
			// No pad connected.
			sio.parp++;
			sio.bufcount = 6;
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
				PAD_LOG("RESET MEMORY CARD");

				sio.bufcount =  8;
				memset8_obj<0xff>(sio.buf);
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio.mcdst = 99;
				sio2.packet.recvVal3 = 0x8c;
				break;
			case 0x12: // RESET
				sio.bufcount =  8;
				memset8_obj<0xff>(sio.buf);
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio.mcdst = 99;

				sio2.packet.recvVal3 = 0x8c;
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x81: // COMMIT
				sio.bufcount =  8;
				memset8_obj<0xff>(sio.buf);
				sio.mcdst = 99;
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio2.packet.recvVal3 = 0x8c;
				if(value == 0x81) {
					if(sio.mc_command==0x42)
						sio2.packet.recvVal1 = 0x1600; // Writing
					else if(sio.mc_command==0x43) sio2.packet.recvVal1 = 0x1700; // Reading
				}
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x21:
			case 0x22:
			case 0x23: // SECTOR SET
                sio.bufcount =  8; sio.mcdst = 99; sio.sector=0; sio.k=0;
				memset8_obj<0xff>(sio.buf);
				sio2.packet.recvVal3 = 0x8c;
				sio.buf[8]=sio.terminator;
				sio.buf[7]='+';
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x24:
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x25:
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x26:
				sio.bufcount = 12; sio.mcdst = 99; sio2.packet.recvVal3 = 0x83;
				memset8_obj<0xff>(sio.buf);
				memcpy(&sio.buf[2], &mc_command_0x26, sizeof(mc_command_0x26));
				sio.buf[12]=sio.terminator;
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x27:
			case 0x28:
			case 0xBF:
				sio.bufcount =  4; sio.mcdst = 99; sio2.packet.recvVal3 = 0x8b;
				memset8_obj<0xff>(sio.buf);
				sio.buf[4]=sio.terminator;
				sio.buf[3]='+';
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x42: // WRITE
			case 0x43: // READ
			case 0x82:
				if(value==0x82 && sio.lastsector==sio.sector) sio.mode = 2;
				if(value==0x42) sio.mode = 0;
				if(value==0x43) sio.lastsector = sio.sector; // Reading

				sio.bufcount =133; sio.mcdst = 99;
				memset8_obj<0xff>(sio.buf);
				sio.buf[133]=sio.terminator;
				sio.buf[132]='+';
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0xf0:
			case 0xf1:
			case 0xf2:
				sio.mcdst = 99;
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0xf3:
			case 0xf7:
				sio.bufcount = 4; sio.mcdst = 99;
				memset8_obj<0xff>(sio.buf);
				sio.buf[4]=sio.terminator;
				sio.buf[3]='+';
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x52:
				sio.rdwr = 1; memset8_obj<0xff>(sio.buf);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			case 0x57:
				sio.rdwr = 2; memset8_obj<0xff>(sio.buf);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';
				MEMCARDS_LOG("MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
				break;
			default:
				sio.mcdst = 0;
				memset8_obj<0xff>(sio.buf);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';
				MEMCARDS_LOG("Unknown MC(%d) command 0x%02X", sio.GetMemcardIndex()+1, value);
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
					if (sio_xor((u8 *)&sio.sector, 4) == value)
						MEMCARDS_LOG("MC(%d) SET PAGE sio.sector 0x%04X",
									sio.GetMemcardIndex()+1, sio.sector);
					else
						MEMCARDS_LOG("MC(%d) SET PAGE XOR value ERROR 0x%02X != ^0x%02X",
							sio.GetMemcardIndex()+1, value, sio_xor((u8 *)&sio.sector, 4));
				}
				break;

			// SET_TERMINATOR; reads the new terminator code
			case 0x27:
				if(sio.parp==2)	{
					sio.terminator = value;
					sio.buf[4] = value;
				MEMCARDS_LOG("MC(%d) SET TERMINATOR command 0x%02X", sio.GetMemcardIndex()+1, value);

				}
				break;

			// GET_TERMINATOR; puts in position 3 the current terminator code and in 4 the default one
			//                                                                  depending on the param
			case 0x28:
				if(sio.parp == 2) {
					sio.buf[2] = '+';
					sio.buf[3] = sio.terminator;

					//if(value == 0) sio.buf[4] = 0xFF;
					sio.buf[4] = 0x55;
				MEMCARDS_LOG("MC(%d) GET TERMINATOR command 0x%02X", sio.GetMemcardIndex()+1, value);
				}
				break;
			// WRITE DATA
			case 0x42:
				if (sio.parp==2) {
					sio.bufcount=5+value;
					memset8_obj<0xff>(sio.buf);
					sio.buf[sio.bufcount-1]='+';
					sio.buf[sio.bufcount]=sio.terminator;
				MEMCARDS_LOG("MC(%d) WRITE command 0x%02X\n\n\n\n", sio.GetMemcardIndex()+1, value);
				}
				else
				if ((sio.parp>2) && (sio.parp<sio.bufcount-2)) {
					sio.buf[sio.parp]=value;
				//MEMCARDS_LOG("MC(%d) WRITING 0x%02X", sio.GetMemcardIndex()+1, value);
				} else
				if (sio.parp==sio.bufcount-2) {
					if (sio_xor(&sio.buf[3], sio.bufcount-5)==value) {
                        _SaveMcd(&sio.buf[3], (512+16)*sio.sector+sio.k, sio.bufcount-5);
						sio.buf[sio.bufcount-1]=value;
						sio.k+=sio.bufcount-5;
					}else {
						MEMCARDS_LOG("MC(%d) write XOR value error 0x%02X != ^0x%02X",
							sio.GetMemcardIndex()+1, value, sio_xor(&sio.buf[3], sio.bufcount-5));
					}
				}
				break;
			// READ DATA
			case 0x43:
				if (sio.parp==2){
					//int i;
					sio.bufcount=value+5;
					sio.buf[3]='+';
				MEMCARDS_LOG("MC(%d) READ command 0x%02X", sio.GetMemcardIndex()+1, value);
					_ReadMcd(&sio.buf[4], (512+16)*sio.sector+sio.k, value);
					/*if(sio.mode==2)
					{
						int j;
						for(j=0; j < value; j++)
							sio.buf[4+j] = ~sio.buf[4+j];
					}*/

					sio.k+=value;
					sio.buf[sio.bufcount-1]=sio_xor(&sio.buf[4], value);
					sio.buf[sio.bufcount]=sio.terminator;
				}
				break;
			// INTERNAL ERASE
			case 0x82:
				if(sio.parp==2) {
					sio.buf[2]='+';
					sio.buf[3]=sio.terminator;
					//if (sio.k != 0 || (sio.sector & 0xf) != 0)
					//	Console::Notice("saving : odd position for erase.");

					_EraseMCDBlock((512+16)*(sio.sector&~0xf));

				/*	memset(sio.buf, -1, 256);
					_SaveMcd(sio.buf, (512+16)*sio.sector, 256);
					_SaveMcd(sio.buf, (512+16)*sio.sector+256, 256);
					_SaveMcd(sio.buf, (512+16)*sio.sector+512, 16);
					sio.buf[2]='+';
					sio.buf[3]=sio.terminator;*/
					//sio.buf[sio.bufcount] = sio.terminator;
				MEMCARDS_LOG("MC(%d) INTERNAL ERASE command 0x%02X", sio.GetMemcardIndex()+1, value);
				}
				break;
			// CARD AUTHENTICATION CHECKS
			case 0xF0:
				if (sio.parp==2)
				{
					MEMCARDS_LOG("MC(%d) CARD AUTH :0x%02X", sio.GetMemcardIndex()+1, value);
					switch(value){
					case  1:
					case  2:
					case  4:
					case 15:
					case 17:
					case 19:
						sio.bufcount=13;
						memset8_obj<0xff>(sio.buf);
						sio.buf[12] = 0; // Xor value of data from index 4 to 11
						sio.buf[3]='+';
						sio.buf[13] = sio.terminator;
						break;
					case  6:
					case  7:
					case 11:
						sio.bufcount=13;
						memset8_obj<0xff>(sio.buf);
						sio.buf[12]='+';
						sio.buf[13] = sio.terminator;
						break;
					default:
						sio.bufcount=4;
						memset8_obj<0xff>(sio.buf);
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
			case 0x12:
				// Query number of pads supported.
				sio.buf[3] = 4;
				sio.mtapst = 2;
				sio.bufcount = 5;
				break;
			case 0x13:
				// Query number of memcards supported.
				sio.buf[3] = 4;
				sio.mtapst = 2;
				sio.bufcount = 5;
				break;
			case 0x21:
				// Set pad slot.
				sio.mtapst = 0x21;
				sio.bufcount = 6; // No idea why this is 6, saved from old code.
				break;
			case 0x22:
				// Set memcard slot.
				sio.mtapst = 0x22;
				sio.bufcount = 6; // No idea why this is 6, saved from old code.
				break;
			}
			// These were taken from old code.  No idea if they're needed.
			// Don't seem to break anything, at least.
			sio.buf[sio.bufcount-1]='+';
			sio.buf[sio.bufcount]='Z';
			return;
		case 0x2:
			sio.packetsize++;
			sio.parp++;
            if (sio.bufcount<=sio.parp)	sio.mcdst = 0;
			SIO_INT();
			return;
		case 0x21:
			// Set pad slot.
			sio.packetsize++;
			sio.parp++;
			sio.mtapst = 2;
			switch (sio.CtrlReg&0x2002) {
				case 0x0002:
					// Not sure if these checks are absolutely needed, but
					// prefer to be safe.
					if (IsMtapPresent(1))
						sio.activePadSlot[0] = value;
					break;
				case 0x2002:
					if (IsMtapPresent(2))
						sio.activePadSlot[1] = value;
					break;
			}
			SIO_INT();
			return;
		case 0x22:
			// Set memcard slot.
			sio.packetsize++;
			sio.parp++;
			sio.mtapst = 2;
			switch (sio.CtrlReg&0x2002) {
				case 0x0002:
					// Not sure if these checks are absolutely needed, but
					// prefer to be safe.
					if (IsMtapPresent(1))
						sio.activeMemcardSlot[0] = value;
					break;
				case 0x2002:
					if (IsMtapPresent(2))
						sio.activeMemcardSlot[1] = value;
					break;
			}
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

			sio.bufcount = 2;
			sio.parp = 0;
			sio.padst = 1;
			sio.packetsize = 1;
			sio.count = 0;
			sio2.packet.recvVal1 = 0x1100; // Pad is present

			switch (sio.CtrlReg&0x2002) {
				case 0x0002:
					if (!PAD1setSlot(1, 1+sio.activePadSlot[0])) {
						// Pad is not present.  Don't send poll, just return a bunch of 0's.
						sio2.packet.recvVal1 = 0x1D100;
						sio.padst = 3;
					}
					else {
						sio.buf[0] = PAD1startPoll(1);
					}
					break;
				case 0x2002:
					if (!PAD2setSlot(1, 1+sio.activePadSlot[1])) {
						// Pad is not present.  Don't send poll, just return a bunch of 0's.
						sio2.packet.recvVal1 = 0x1D100;
						sio.padst = 3;
					}
					else {
						sio.buf[0] = PAD2startPoll(2);
					}
					break;
			}

			SIO_INT();
			return;

		case 0x21: // start mtap
			sio.StatReg &= ~TX_EMPTY;	// Now the Buffer is not empty
			sio.StatReg |= RX_RDY;		// Transfer is Ready
			sio.parp = 0;
			sio.packetsize = 1;
			sio.mtapst = 1;
			sio.count = 0;
			sio2.packet.recvVal1 = 0x1D100; // Mtap is not connected :(
			switch (sio.CtrlReg&0x2002) {
				case 0x0002:
					if (!IsMtapPresent(1)) {
						// If "unplug" multitap, set slots to 0.
						sio.activePadSlot[0] = 0;
						sio.activeMemcardSlot[0] = 0;
						break;
					}
					sio.bufcount = 3;
					sio.buf[0] = 0xFF;
					sio.buf[1] = 0x80; // Have no idea if this is correct.  From PSX mtap.
					sio.buf[2] = 0x5A;
					sio2.packet.recvVal1 = 0x1100; // Mtap is connected :)
					break;
				case 0x2002:
					if (!IsMtapPresent(2)) {
						// If "unplug" multitap, set slots to 0.
						sio.activePadSlot[1] = 0;
						sio.activeMemcardSlot[1] = 0;
						break;
					}
					sio.bufcount = 3;
					sio.buf[0] = 0xFF;
					sio.buf[1] = 0x80; // Have no idea if this is correct.  From PSX mtap.
					sio.buf[2] = 0x5A;
					sio2.packet.recvVal1 = 0x1100; // Mtap is connected :)
					break;
			}
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
		{
			sio.StatReg &= ~TX_EMPTY;
			sio.StatReg |= RX_RDY;
			memcpy(sio.buf, cardh, 4);
			sio.parp = 0;
			sio.bufcount = 8;
			sio.mcdst = 1;
			sio.packetsize = 1;
			sio.rdwr = 0;
			sio.count = 0;

			// Memcard presence reporting!
			// In addition to checking the presence of MemoryCard1/2 file handles, we check a
			// variable which force-disables all memory cards after a savestate recovery.  This
			// is needed to inform certain games to clear their cached memorycard indexes.

			// Note:
			//	0x01100 means Memcard is present
			//  0x1D100 means Memcard is missing.

			const int mcidx = sio.GetMemcardIndex();

			if( sio.activeMemcardSlot[mcidx] )
			{
				// Might want to more agressively declare a card's non-existence here.
				// As non-zero slots always report a failure, and have to read
				// the FAT before writing, think this should be fine.
				sio2.packet.recvVal1 = 0x1D100;
				PAD_LOG( "START MEMCARD[%d][%d] - Only one memcard supported per slot - reported as missing.", sio.GetMemcardIndex(), sio.activeMemcardSlot[mcidx]);
			}
			else if( m_PostSavestateCards[mcidx] )
			{
				m_PostSavestateCards[mcidx]--;
				sio2.packet.recvVal1 = 0x1D100;
				PAD_LOG( "START MEMCARD[%d] - post-savestate ejection - reported as missing!", sio.GetMemcardIndex() );
			}
			else
			{
				sio2.packet.recvVal1 = MemoryCard::IsPresent( sio.GetMemcardIndex() ) ? 0x1100 : 0x1D100;
				PAD_LOG("START MEMCARD [%d] - %s",
					sio.GetMemcardIndex(), MemoryCard::IsPresent( sio.GetMemcardIndex() ) ? "Present" : "Missing" );
			}

			SIO_INT();
		}
		return;
	}
}

void sioWrite8(u8 value)
{
	SIO_CommandWrite(value,0);
}

void SIODMAWrite(u8 value)
{
	SIO_CommandWrite(value,1);
}

void sioWriteCtrl16(u16 value) {
	sio.CtrlReg = value & ~RESET_ERR;
	if (value & RESET_ERR) sio.StatReg &= ~IRQ;
	if ((sio.CtrlReg & SIO_RESET) || (!sio.CtrlReg))
	{
		sio.mtapst = 0; sio.padst = 0; sio.mcdst = 0; sio.parp = 0;
		sio.StatReg = TX_RDY | TX_EMPTY;
		psxRegs.interrupt &= ~(1<<IopEvt_SIO);
	}
}

void SIO_FORCEINLINE sioInterrupt() {
	PAD_LOG("Sio Interrupt");
	sio.StatReg|= IRQ;
	psxHu32(0x1070)|=0x80;
}

// Signals the sio to eject the specified memory card.
// Called from the memory card configuration when a user changes memory cards.
void sioEjectCard( uint mcdId )
{
	jASSUME( mcdId < 2 );
	m_PostSavestateCards[mcdId] = 64;

	// Reload the new memory card:

	MemoryCard::Unload( mcdId );
	MemoryCard::Init();
}

void SaveState::sioFreeze()
{
	// CRCs for memory cards.
	u64 m_mcdCRCs[2];

	FreezeTag( "sio" );
	if (GetVersion() == 0) {
		FreezeMem( &sio, sizeof( sio ) - 4);
		memset(sio.activePadSlot, 0, sizeof(sio.activePadSlot));
		memset(sio.activeMemcardSlot, 0, sizeof(sio.activeMemcardSlot));
	}
	else {
	    Freeze( sio );
	}

	if( IsSaving() )
	{
		for( int i=0; i<2; ++i )
			m_mcdCRCs[i] = MemoryCard::GetCRC( i );
	}
	Freeze( m_mcdCRCs );

	if( IsLoading() && Config.McdEnableEject )
	{
		// Note: TOTA works with values as low as 20 here.
		// It "times out" with values around 1800 (forces user to check the memcard
		// twice to find it).  Other games could be different. :|

		// At 64: Disgaea 1 and 2, and Grandia 2 end up displaying a quick "no memcard!"
		// notice before finding the memorycard and re-enumerating it.  A very minor
		// annoyance, but no breakages.

		// GuitarHero will break completely with almost any value here, by design, because
		// it has a "rule" that the memcard should never be ejected during a song.  So by
		// ejecting it, the game freezes (which is actually good emulation, but annoying!)

		for( int i=0; i<2; ++i )
		{
			u64 newCRC = MemoryCard::GetCRC( i );
			if( newCRC != m_mcdCRCs[i] )
			{
				m_PostSavestateCards[i] = 64;
				m_mcdCRCs[i] = newCRC;
			}
		}
	}
}

