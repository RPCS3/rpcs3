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
#include "IopCommon.h"

#include "Sio.h"
#include "sio_internal.h"

_sio sio;

static const u8 cardh[4] = { 0xFF, 0xFF, 0x5a, 0x5d };

// Memory Card Specs for standard Sony 8mb carts:
//    Flags (magic sio '+' thingie!), Sector size, eraseBlockSize (in pages), card size (in pages), xor checksum (superblock?), terminator (unused?).
static const mc_command_0x26_tag mc_sizeinfo_8mb= {'+', 512, 16, 0x4000, 0x52, 0x5A};

// Ejection timeout management belongs in the MemoryCardFile plugin, except the plugin
// interface is not yet complete.

//Reinsert the card after auto-eject: after max tries or after min tries + XXX milliseconds, whichever comes first.
//E.g. if the game polls the card 100 times/sec and max tries=100, then after 1 second it will see the card as inserted (ms timeout not reached).
//E.g. if the game polls the card 1 time/sec, then it will see the card ejected 4 times, and on the 5th it will see it as inserted (4 secs from the initial access).
//(A 'try' in this context is the game accessing SIO)
static const int   FORCED_MCD_EJECTION_MIN_TRIES =2;
static const int   FORCED_MCD_EJECTION_MAX_TRIES =128;
static const float FORCED_MCD_EJECTION_MAX_MS_AFTER_MIN_TRIES =2800; 

static const int FORCED_MCD_EJECTION_NUM_SLOTS =2;
static int			m_ForceEjectionTimeout[FORCED_MCD_EJECTION_NUM_SLOTS];
static wxDateTime	m_ForceEjection_minTriesTimestamp[FORCED_MCD_EJECTION_NUM_SLOTS];

wxString GetTimeMsStr(){
	wxDateTime unow=wxDateTime::UNow();
	wxString res;
	res.Printf(L"%s.%03d", unow.Format(L"%H:%M:%S").c_str(), (int)unow.GetMillisecond() );
	return res;
}
void _SetForceMcdEjectTimeoutNow(int port, int slot, int mcdEjectTimeoutInSioAccesses)
{
	m_ForceEjectionTimeout[port]=mcdEjectTimeoutInSioAccesses;
}

//allow timeout also for the mcd manager panel
void SetForceMcdEjectTimeoutNow()
{
	const int slot=0;
	int port=0;
	for (port=0; port<2; port++)
		_SetForceMcdEjectTimeoutNow(port, slot, FORCED_MCD_EJECTION_MAX_TRIES);
}


// SIO Inline'd IRQs : Calls the SIO interrupt handlers directly instead of
// feeding them through the IOP's branch test. (see SIO.H for details)

#ifdef SIO_INLINE_IRQS
#define SIO_INT() sioInterrupt()
#define SIO_FORCEINLINE __fi
#else
__fi void SIO_INT()
{
	if( !(psxRegs.interrupt & (1<<IopEvt_SIO)) )
		PSX_INT(IopEvt_SIO, 64 ); // PSXCLK/250000);
}
#define SIO_FORCEINLINE __fi
#endif

// Currently only check if pad wants mtap to be active.
// Could lets PCSX2 have its own options, if anyone ever
// wants to add support for using the extra memcard slots.
static bool IsMtapPresent( uint port )
{
	return EmuConfig.MultitapEnabled( port );
	//return (0 != PADqueryMtap(port+1));
}

static void _ReadMcd(u8 *data, u32 adr, int size)
{
	SysPlugins.McdRead(
		sio.GetMemcardIndex(), sio.activeMemcardSlot[sio.GetMemcardIndex()],
		data, adr, size
	);
}

static void _SaveMcd(const u8 *data, u32 adr, int size)
{
	SysPlugins.McdSave(
		sio.GetMemcardIndex(), sio.activeMemcardSlot[sio.GetMemcardIndex()],
		data, adr, size
	);
}

static void _EraseMCDBlock(u32 adr)
{
	SysPlugins.McdEraseBlock( sio.GetMemcardIndex(), sio.activeMemcardSlot[sio.GetMemcardIndex()], adr );
}

static u8 sio_xor( const u8 *buf, uint length )
{
	u8 i, x;
	for (x=0, i=0; i<length; i++) x ^= buf[i];
	return x;
}

template< typename T >
static void apply_xor( u8& dest, const T& src )
{
	u8* buf = (u8*)&src;
	for (uint x=0; x<sizeof(src); x++) dest ^= buf[x];
}

void sioInit()
{
	memzero(sio);
	memzero(m_ForceEjectionTimeout);

	// Transfer(?) Ready and the Buffer is Empty
	sio.StatReg = TX_RDY | TX_EMPTY;
	sio.packetsize = 0;
	sio.terminator = 0x55; // Command terminator 'U'
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
						sio.buf[sio.parp] = PADpoll(value);
						break;
					case 0x2002:
						sio.packetsize ++;	// Total packet size sent
						sio.buf[sio.parp] = PADpoll(value);
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
				case 0x0002: sio.packetsize ++; sio.buf[sio.parp] = PADpoll(value); break;
				case 0x2002: sio.packetsize ++; sio.buf[sio.parp] = PADpoll(value); break;
			}
			if (sio.parp == sio.bufcount) { sio.padst = 0; return; }
			SIO_INT();
			return;
		case 3:
			// No pad connected.
			sio.parp++;
			if (sio.parp == sio.bufcount) { sio.padst = 0; return; }
			SIO_INT();
			return;
	}

	// MEMORY CARD COMMANDS
	switch (sio.mcdst) {
		case 1:
		{
			sio.packetsize++;
			SIO_INT();
			if (sio.rdwr) { sio.parp++; return; }
			sio.parp = 1;

			const char* log_cmdname = "";

			switch (value) {
			case 0x11: // RESET
				log_cmdname = "Reset1";

				sio.bufcount =  8;
				memset8<0xff>(sio.buf);
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio.mcdst = 99;
				sio2.packet.recvVal3 = 0x8c;
				break;

			// FIXME : Why are there two identical cases for resetting the
			// memorycard(s)?  there doesn't appear to be anything dealing with
			// card slots here.  --air
			case 0x12: // RESET
				log_cmdname = "Reset2";
				sio.bufcount =  8;
				memset8<0xff>(sio.buf);
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio.mcdst = 99;
				sio2.packet.recvVal3 = 0x8c;
				break;

			case 0x81: // COMMIT
				log_cmdname = "Commit";
				sio.bufcount =  8;
				memset8<0xff>(sio.buf);
				sio.mcdst = 99;
				sio.buf[3] = sio.terminator;
				sio.buf[2] = '+';
				sio2.packet.recvVal3 = 0x8c;
				if(value == 0x81) {
					if(sio.mc_command==0x42)
						sio2.packet.recvVal1 = 0x1600; // Writing
					else if(sio.mc_command==0x43) sio2.packet.recvVal1 = 0x1700; // Reading
				}
				break;
			case 0x21:
			case 0x22:
			case 0x23: // SECTOR SET
				log_cmdname = "SetSector";
                sio.bufcount =  8; sio.mcdst = 99; sio.sector=0; sio.k=0;
				memset8<0xff>(sio.buf);
				sio2.packet.recvVal3 = 0x8c;
				sio.buf[8]=sio.terminator;
				sio.buf[7]='+';
				break;

			case 0x24: break;
			case 0x25: break;

			case 0x26:
			{
				log_cmdname = "GetInfo";

				const uint port = sio.GetMemcardIndex();
				const uint slot = sio.activeMemcardSlot[port];

				mc_command_0x26_tag cmd = mc_sizeinfo_8mb;
				PS2E_McdSizeInfo info;
				
				info.SectorSize			= cmd.sectorSize;
				info.EraseBlockSizeInSectors			= cmd.eraseBlocks;
				info.McdSizeInSectors	= cmd.mcdSizeInSectors;
				
				SysPlugins.McdGetSizeInfo( port, slot, info );
				pxAssumeDev( cmd.mcdSizeInSectors >= mc_sizeinfo_8mb.mcdSizeInSectors,
					"Mcd plugin returned an invalid memorycard size: Cards smaller than 8MB are not supported." );
				
				cmd.sectorSize			= info.SectorSize;
				cmd.eraseBlocks			= info.EraseBlockSizeInSectors;
				cmd.mcdSizeInSectors	= info.McdSizeInSectors;
				
				// Recalculate the xor summation
				// This uses a trick of removing the known xor values for a default 8mb memorycard (for which the XOR
				// was calculated), and replacing it with our new values.
				
				apply_xor( cmd.mc_xor, mc_sizeinfo_8mb.sectorSize );
				apply_xor( cmd.mc_xor, mc_sizeinfo_8mb.eraseBlocks );
				apply_xor( cmd.mc_xor, mc_sizeinfo_8mb.mcdSizeInSectors );

				apply_xor( cmd.mc_xor, cmd.sectorSize );
				apply_xor( cmd.mc_xor, cmd.eraseBlocks );
				apply_xor( cmd.mc_xor, cmd.mcdSizeInSectors );
				
				sio.bufcount = 12; sio.mcdst = 99; sio2.packet.recvVal3 = 0x83;
				memset8<0xff>(sio.buf);
				memcpy_fast(&sio.buf[2], &cmd, sizeof(cmd));
				sio.buf[12]=sio.terminator;
			}
			break;

			case 0x27:
			case 0x28:
			case 0xBF:
				log_cmdname = "NotSure";	// FIXME !!
				sio.bufcount =  4; sio.mcdst = 99; sio2.packet.recvVal3 = 0x8b;
				memset8<0xff>(sio.buf);
				sio.buf[4]=sio.terminator;
				sio.buf[3]='+';
				break;

			// FIXME ?
			// sio.lastsector and sio.mode are unused.

			case 0x42: // WRITE
				log_cmdname = "Write";
				//sio.mode = 0;
			goto __doReadWrite;

			case 0x43: // READ
				log_cmdname = "Read";
				//sio.lastsector = sio.sector; // Reading
			goto __doReadWrite;

			case 0x82:
				log_cmdname = "Read(?)"; // FIXME !!
				//if(sio.lastsector==sio.sector) sio.mode = 2;

			__doReadWrite:
				sio.bufcount =133; sio.mcdst = 99;
				memset8<0xff>(sio.buf);
				sio.buf[133]=sio.terminator;
				sio.buf[132]='+';
			break;
			

			case 0xf0:
			case 0xf1:
			case 0xf2:
				log_cmdname = "NoClue"; // FIXME !!
				sio.mcdst = 99;
				break;

			case 0xf3:
			case 0xf7:
				log_cmdname = "NoClueHereEither"; // FIXME !!
				sio.bufcount = 4; sio.mcdst = 99;
				memset8<0xff>(sio.buf);
				sio.buf[4]=sio.terminator;
				sio.buf[3]='+';
				break;

			case 0x52:
				log_cmdname = "FixMe"; // FIXME !!
				sio.rdwr = 1; memset8<0xff>(sio.buf);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';
				break;
			case 0x57:
				log_cmdname = "FixMe"; // FIXME !!
				sio.rdwr = 2; memset8<0xff>(sio.buf);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';
				break;
			default:
				log_cmdname = "Unknown";
				sio.mcdst = 0;
				memset8<0xff>(sio.buf);
				sio.buf[sio.bufcount]=sio.terminator; sio.buf[sio.bufcount-1]='+';
			}
			MEMCARDS_LOG("MC(%d) command 0x%02X [%s]", sio.GetMemcardIndex()+1, value, log_cmdname);
			sio.mc_command = value;
		}
		return;		// END CASE 1.

		// FURTHER PROCESSING OF THE MEMORY CARD COMMANDS
		case 99:
		{
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
						MEMCARDS_LOG("MC(%d) SET PAGE sio.sector, sector=0x%04X", sio.GetMemcardIndex()+1, sio.sector);
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
					MEMCARDS_LOG("MC(%d) SET TERMINATOR command, value=0x%02X", sio.GetMemcardIndex()+1, value);

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
					MEMCARDS_LOG("MC(%d) GET TERMINATOR command, value=0x%02X", sio.GetMemcardIndex()+1, value);
				}
				break;
			// WRITE DATA
			case 0x42:
				if (sio.parp==2) {
					sio.bufcount=5+value;
					memset8<0xff>(sio.buf);
					sio.buf[sio.bufcount-1]='+';
					sio.buf[sio.bufcount]=sio.terminator;
					MEMCARDS_LOG("MC(%d) WRITE command, size=0x%02X", sio.GetMemcardIndex()+1, value);
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
					} else {
						MEMCARDS_LOG("MC(%d) write XOR value error 0x%02X != ^0x%02X",
							sio.GetMemcardIndex()+1, value, sio_xor(&sio.buf[3], sio.bufcount-5));
					}
				}
				break;
			// READ DATA
			case 0x43:
				if (sio.parp==2)
				{
					//int i;
					sio.bufcount=value+5;
					sio.buf[3]='+';
					MEMCARDS_LOG("MC(%d) READ command, size=0x%02X", sio.GetMemcardIndex()+1, value);
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
				if(sio.parp==2)
				{
					sio.buf[2]='+';
					sio.buf[3]=sio.terminator;
					//if (sio.k != 0 || (sio.sector & 0xf) != 0)
					//	Console.Warning("saving : odd position for erase.");

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
						memset8<0xff>(sio.buf);
						sio.buf[12] = 0; // Xor value of data from index 4 to 11
						sio.buf[3]='+';
						sio.buf[13] = sio.terminator;
						break;
					case  6:
					case  7:
					case 11:
						sio.bufcount=13;
						memset8<0xff>(sio.buf);
						sio.buf[12]='+';
						sio.buf[13] = sio.terminator;
						break;
					default:
						sio.bufcount=4;
						memset8<0xff>(sio.buf);
						sio.buf[3]='+';
						sio.buf[4] = sio.terminator;
					}
				}
				break;
			}
			if (sio.bufcount<=sio.parp)	sio.mcdst = 0;
		}
		return;		// END CASE 99.
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
				sio.mtapst = value;
				sio.bufcount = 6; // No idea why this is 6, saved from old code.
				break;
			case 0x22:
				// Set memcard slot.
				sio.mtapst = value;
				sio.bufcount = 6; // No idea why this is 6, saved from old code.
				break;
			}
			// Commented out values are from original code.  They break multitap in bios.
			sio.buf[sio.bufcount-1]=0;//'+';
			sio.buf[sio.bufcount]=0;//'Z';
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
			if (sio.CtrlReg & 2)
			{
				int port = sio.GetMultitapPort();
				if (IsMtapPresent(port))
					sio.activePadSlot[port] = value;
			}
			SIO_INT();
			return;
		case 0x22:
			// Set memcard slot.
			sio.packetsize++;
			sio.parp++;
			sio.mtapst = 2;
			if (sio.CtrlReg & 2)
			{
				int port = sio.GetMultitapPort();
				if (IsMtapPresent(port))
					sio.activeMemcardSlot[port] = value;
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

			sio.bufcount = 4; // Default size, when no pad connected.
			sio.parp = 0;
			sio.padst = 1;
			sio.packetsize = 1;
			sio.count = 0;
			sio2.packet.recvVal1 = 0x1100; // Pad is present

			if( (sio.CtrlReg & 2) == 2 )
			{
				int padslot = (sio.CtrlReg>>12) & 2;	// move 0x2000 bitmask into leftmost bits
				if( padslot != 1 )
				{
					padslot >>= 1;	// transform 0/2 to be 0/1 values

					if (!PADsetSlot(padslot+1, 1+sio.activePadSlot[padslot]) && sio.activePadSlot[padslot])
					{
						// Pad is not present.  Don't send poll, just return a bunch of 0's.
						sio2.packet.recvVal1 = 0x1D100;
						sio.padst = 3;
					}
					else {
						sio.buf[0] = PADstartPoll(padslot+1);
					}
				}
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
			if (sio.CtrlReg & 2) // No idea if this test is needed.  Pads use it, memcards don't.
			{
				int port = sio.GetMultitapPort();
				if (!IsMtapPresent(port))
				{
					// If "unplug" multitap mid game, set active slots to 0.
					sio.activePadSlot[port] = 0;
					sio.activeMemcardSlot[port] = 0;
				}
				else
				{
					sio.bufcount = 3;
					sio.buf[0] = 0xFF;
					sio.buf[1] = 0x80; // Have no idea if this is correct.  From PSX mtap.
					sio.buf[2] = 0x5A;
					sio2.packet.recvVal1 = 0x1100; // Mtap is connected :)
				}
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
			// Note:
			//	0x01100 means Memcard is present
			//  0x1D100 means Memcard is missing.

			const uint port = sio.GetMemcardIndex();
			const uint slot = sio.activeMemcardSlot[port];

			// forced ejection logic.  Technically belongs in the McdIsPresent handler for
			// the plugin, once the memorycard plugin system is completed.
			//  (ejection is only supported for the default non-multitap cards at this time)

			bool forceEject = false;
			if( slot == 0 && m_ForceEjectionTimeout[port]>0 )
			{
				if( m_ForceEjectionTimeout[port] == FORCED_MCD_EJECTION_MAX_TRIES && SysPlugins.McdIsPresent( port, slot ) )
					Console.Warning( L"[%s] Auto-ejecting memcard [port:%d, slot:%d]", GetTimeMsStr().c_str(), port, slot );

				--m_ForceEjectionTimeout[port];
				forceEject = true;

				int numTimesAccessed = FORCED_MCD_EJECTION_MAX_TRIES - m_ForceEjectionTimeout[port];
				if ( numTimesAccessed == FORCED_MCD_EJECTION_MIN_TRIES )
				{//minimum tries reached. start counting millisec timeout.
					m_ForceEjection_minTriesTimestamp[port] = wxDateTime::UNow();
				}

				if ( numTimesAccessed > FORCED_MCD_EJECTION_MIN_TRIES )
				{
					wxTimeSpan delta = wxDateTime::UNow().Subtract( m_ForceEjection_minTriesTimestamp[port] );
					if ( delta.GetMilliseconds() >= FORCED_MCD_EJECTION_MAX_MS_AFTER_MIN_TRIES )
					{
						Console.Warning( L"[%s] Auto-eject: Timeout reached after mcd was accessed %d times [port:%d, slot:%d]", GetTimeMsStr().c_str(), numTimesAccessed, port, slot);
						m_ForceEjectionTimeout[port] = 0;	//Done. on next sio access the card will be seen as inserted.
					}
				}

				if( m_ForceEjectionTimeout[port] == 0 && SysPlugins.McdIsPresent( port, slot ))
					Console.Warning( L"[%s] Re-inserting auto-ejected memcard [port:%d, slot:%d]", GetTimeMsStr().c_str(), port, slot);
			}
			
			if( !forceEject && SysPlugins.McdIsPresent( port, slot ) )
			{
				sio2.packet.recvVal1 = 0x1100;
				PAD_LOG("START MEMCARD [port:%d, slot:%d] - Present", port, slot );
			}
			else
			{
				sio2.packet.recvVal1 = 0x1D100;
				PAD_LOG("START MEMCARD [port:%d, slot:%d] - Missing", port, slot );
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

void SaveStateBase::sioFreeze()
{
	// CRCs for memory cards.
	u64 m_mcdCRCs[2][8];

	FreezeTag( "sio" );
    Freeze( sio );

	// TODO : This stuff should all be moved to the memorycard plugin eventually,
	// but that requires adding memorycard plugin to the savestate, and I'm not in
	// the mood to do that (let's plan it for 0.9.8) --air

	// Note: The Ejection system only works for the default non-multitap MemoryCards
	// only.  This is because it could become very (very!) slow to do a full CRC check
	// on multiple 32 or 64 meg carts.  I have chosen to save 

	if( IsSaving() )
	{
		for( uint port=0; port<2; ++port )
		//for( uint slot=0; slot<4; ++slot )
		{
			const uint slot = 0;		// see above comment about multitap slowness
			m_mcdCRCs[port][slot] = SysPlugins.McdGetCRC( port, slot );
		}
	}

	Freeze( m_mcdCRCs );

	if( IsLoading() && EmuConfig.McdEnableEjection )
	{
		// Notes on the ForceEjectionTimeout:
		//  * TOTA works with values as low as 20 here.
		//    It "times out" with values around 1800 (forces user to check the memcard
		//    twice to find it).  Other games could be different. :|
		//
		//  * At 64: Disgaea 1 and 2, and Grandia 2 end up displaying a quick "no memcard!"
		//    notice before finding the memorycard and re-enumerating it.  A very minor
		//    annoyance, but no breakages.

		//  * GuitarHero will break completely with almost any value here, by design, because
		//    it has a "rule" that the memcard should never be ejected during a song.  So by
		//    ejecting it, the game freezes (which is actually good emulation, but annoying!)

		for( uint port=0; port<2; ++port )
		//for( int slot=0; slot<4; ++slot )
		{
			const uint slot = 0;		// see above comment about multitap slowness
			u64 newCRC = SysPlugins.McdGetCRC( port, slot );
			if( newCRC != m_mcdCRCs[port][slot] )
			{
				//m_mcdCRCs[port][slot] = newCRC;
				//m_ForceEjectionTimeout[port] = 128;
				_SetForceMcdEjectTimeoutNow(port, slot, FORCED_MCD_EJECTION_MAX_TRIES); 
			}
		}
	}
}

