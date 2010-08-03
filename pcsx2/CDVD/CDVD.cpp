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
#include "AppConfig.h"

#include <ctype.h>
#include <wx/datetime.h>

#include "CDVD.h"
#include "CDVD_internal.h"
#include "CDVDisoReader.h"

#include "GS.h"			// for gsRegionMode
#include "Elfheader.h"
#include "ps2/BiosTools.h"
#include "GameDatabase.h"

// This typically reflects the Sony-assigned serial code for the Disc, if one exists.
//  (examples:  SLUS-2113, etc).
// If the disc is homebrew then it probably won't have a valid serial; in which case
// this string will be empty.
wxString DiscSerial;

static cdvdStruct cdvd;

static __forceinline void SetResultSize(u8 size)
{
	cdvd.ResultC = size;
	cdvd.ResultP = 0;
	cdvd.sDataIn&=~0x40;
}

static void CDVDREAD_INT(int eCycle)
{
	// Give it an arbitary FAST value. Good for ~5000kb/s in ULE when copying a file from CDVD to HDD
	if (EmuConfig.Speedhacks.fastCDVD) eCycle = 3000;
	PSX_INT(IopEvt_CdvdRead, eCycle);
}

static void CDVD_INT(int eCycle)
{
	if( eCycle == 0 )
		cdvdActionInterrupt();
	else
		PSX_INT(IopEvt_Cdvd, eCycle);
}

// Sets the cdvd IRQ and the reason for the IRQ, and signals the IOP for a branch
// test (which will cause the exception to be handled).
static void cdvdSetIrq( uint id = (1<<Irq_CommandComplete) )
{
	cdvd.PwOff |= id;
	iopIntcIrq( 2 );
	hwIntcIrq(INTC_SBUS);
	psxSetNextBranchDelta( 20 );
}

static int mg_BIToffset(u8 *buffer)
{
	int i, ofs = 0x20;
	for (i=0; i<*(u16*)&buffer[0x1A]; i++)
		ofs+=0x10;

	if (*(u16*)&buffer[0x18] & 1) ofs += buffer[ofs];
	if ((*(u16*)&buffer[0x18] & 0xF000) == 0) ofs += 8;

	return ofs + 0x20;
}

FILE *_cdvdOpenMechaVer()
{
	FILE* fd;

	// get the name of the bios file

	wxFileName mecfile(EmuConfig.BiosFilename);
	mecfile.SetExt( L"mec" );
	const wxCharBuffer file( mecfile.GetFullPath().ToUTF8() );

	// if file doesnt exist, create empty one
	fd = fopen(file, "r+b");
	if (fd == NULL)
	{
		Console.Warning("MEC File Not Found , Creating Blank File");
		fd = fopen(file, "wb");
		if (fd == NULL)
			throw Exception::CannotCreateStream(mecfile.GetFullPath());

		fputc(0x03, fd);
		fputc(0x06, fd);
		fputc(0x02, fd);
		fputc(0x00, fd);
	}
	return fd;
}

s32 cdvdGetMechaVer(u8* ver)
{
	FILE* fd = _cdvdOpenMechaVer();
	if (fd == NULL) return 1;
	fseek(fd, 0, SEEK_SET);
	fread(ver, 1, 4, fd);
	fclose(fd);
	return 0;
}

FILE *_cdvdOpenNVM()
{
	FILE* fd;

	wxFileName nvmfile(EmuConfig.BiosFilename);
	nvmfile.SetExt( L"nvm" );
	const wxCharBuffer file( nvmfile.GetFullPath().ToUTF8() );

	// if file doesn't exist, create empty one
	fd = fopen(file, "r+b");
	if (fd == NULL)
	{
		Console.Warning("NVM File Not Found , Creating Blank File");
		fd = fopen(file, "wb");
		if (fd == NULL)
			throw Exception::CannotCreateStream(nvmfile.GetFullPath());

		for (int i=0; i<1024; i++) fputc(0, fd);
	}
	return fd;
}

//
// the following 'cdvd' functions all return 0 if successful
//

s32 cdvdReadNVM(u8 *dst, int offset, int bytes) {
	FILE* fd = _cdvdOpenNVM();
	if (fd == NULL) return 1;

	fseek(fd, offset, SEEK_SET);
	fread(dst, 1, bytes, fd);
	fclose(fd);

	return 0;
}
s32 cdvdWriteNVM(const u8 *src, int offset, int bytes) {
	FILE* fd = _cdvdOpenNVM();
	if (fd == NULL) return 1;

	fseek(fd, offset, SEEK_SET);
	fwrite(src, 1, bytes, fd);
	fclose(fd);

	return 0;
}

NVMLayout* getNvmLayout(void)
{
	NVMLayout* nvmLayout = NULL;
	s32 nvmIdx;

	for(nvmIdx=0; nvmIdx<NVM_FORMAT_MAX; nvmIdx++)
	{
		if(nvmlayouts[nvmIdx].biosVer <= BiosVersion)
			nvmLayout = &nvmlayouts[nvmIdx];
	}
	return nvmLayout;
}

s32 getNvmData(u8* buffer, s32 offset, s32 size, s32 fmtOffset)
{
	// find the correct bios version
	NVMLayout* nvmLayout = getNvmLayout();
	if (nvmLayout == NULL) return 1;

	// get data from eeprom
	return cdvdReadNVM(buffer, *(s32*)(((u8*)nvmLayout)+fmtOffset) + offset, size);
}
s32 setNvmData(const u8* buffer, s32 offset, s32 size, s32 fmtOffset)
{
	// find the correct bios version
	NVMLayout* nvmLayout = getNvmLayout();
	if (nvmLayout == NULL) return 1;

	// set data in eeprom
	return cdvdWriteNVM(buffer, *(s32*)(((u8*)nvmLayout)+fmtOffset) + offset, size);
}

s32 cdvdReadConsoleID(u8* id)
{
	return getNvmData(id, 0, 8, offsetof(NVMLayout, consoleId));
}
s32 cdvdWriteConsoleID(const u8* id)
{
	return setNvmData(id, 0, 8, offsetof(NVMLayout, consoleId));
}

s32 cdvdReadILinkID(u8* id)
{
	return getNvmData(id, 0, 8, offsetof(NVMLayout, ilinkId));
}
s32 cdvdWriteILinkID(const u8* id)
{
	return setNvmData(id, 0, 8, offsetof(NVMLayout, ilinkId));
}

s32 cdvdReadModelNumber(u8* num, s32 part)
{
	return getNvmData(num, part, 8, offsetof(NVMLayout, modelNum));
}
s32 cdvdWriteModelNumber(const u8* num, s32 part)
{
	return setNvmData(num, part, 8, offsetof(NVMLayout, modelNum));
}

s32 cdvdReadRegionParams(u8* num)
{
	return getNvmData(num, 0, 8, offsetof(NVMLayout,regparams));
}

s32 cdvdWriteRegionParams(const u8* num)
{
	return setNvmData(num, 0, 8, offsetof(NVMLayout,regparams));
}

s32 cdvdReadMAC(u8* num)
{
	return getNvmData(num, 0, 8, offsetof(NVMLayout,mac));
}

s32 cdvdWriteMAC(const u8* num)
{
	return setNvmData(num, 0, 8, offsetof(NVMLayout,mac));
}

s32 cdvdReadConfig(u8* config)
{
	// make sure its in read mode
	if(cdvd.CReadWrite != 0)
	{
		config[0] = 0x80;
		memset(&config[1], 0x00, 15);
		return 1;
	}
	// check if block index is in bounds
	else if(cdvd.CBlockIndex >= cdvd.CNumBlocks)
		return 1;
	else if(
		((cdvd.COffset == 0) && (cdvd.CBlockIndex >= 4))||
		((cdvd.COffset == 1) && (cdvd.CBlockIndex >= 2))||
		((cdvd.COffset == 2) && (cdvd.CBlockIndex >= 7))
		)
	{
		memzero_ptr<16>(config);
		return 0;
	}

	// get config data
	switch (cdvd.COffset)
	{
		case 0:
			return getNvmData(config, (cdvd.CBlockIndex++)*16, 16, offsetof(NVMLayout, config0));
			break;
		case 2:
			return getNvmData(config, (cdvd.CBlockIndex++)*16, 16, offsetof(NVMLayout, config2));
			break;
		default:
			return getNvmData(config, (cdvd.CBlockIndex++)*16, 16, offsetof(NVMLayout, config1));
	}
}
s32 cdvdWriteConfig(const u8* config)
{
	// make sure its in write mode && the block index is in bounds
	if ((cdvd.CReadWrite != 1) || (cdvd.CBlockIndex >= cdvd.CNumBlocks))
		return 1;
	else if(
		((cdvd.COffset == 0) && (cdvd.CBlockIndex >= 4))||
		((cdvd.COffset == 1) && (cdvd.CBlockIndex >= 2))||
		((cdvd.COffset == 2) && (cdvd.CBlockIndex >= 7))
		)
		return 0;

	// get config data
	switch (cdvd.COffset)
	{
		case 0:
			return setNvmData(config, (cdvd.CBlockIndex++)*16, 16, offsetof(NVMLayout, config0));
			break;
		case 2:
			return setNvmData(config, (cdvd.CBlockIndex++)*16, 16, offsetof(NVMLayout, config2));
			break;
		default:
			return setNvmData(config, (cdvd.CBlockIndex++)*16, 16, offsetof(NVMLayout, config1));
	}
}

static MutexRecursive Mutex_NewDiskCB;

// Sets ElfCRC to the CRC of the game bound to the CDVD plugin.
static __forceinline ElfObject* loadElf( const wxString filename )
{
	if (filename.StartsWith(L"host"))
		return new ElfObject(filename.After(':'), Path::GetFileSize(filename.After(':')));

	// Mimic PS2 behavior!
	// Much trial-and-error with changing the ISOFS and BOOT2 contents of an image have shown that
	// the PS2 BIOS performs the peculiar task of *ignoring* the version info from the parsed BOOT2
	// filename *and* the ISOFS, when loading the game's ELF image.  What this means is:
	//
	//   1. a valid PS2 ELF can have any version (ISOFS), and the version need not match the one in SYSTEM.CNF.
	//   2. the version info on the file in the BOOT2 parameter of SYSTEM.CNF can be missing, 10 chars long,
	//      or anything else.  Its all ignored.
	//   3. Games loading their own files do *not* exhibit this behavior; likely due to using newer IOP modules
	//      or lower level filesystem APIs (fortunately that doesn't affect us).
	//
	// FIXME: Properly mimicing this behavior is troublesome since we need to add support for "ignoring"
	// version information when doing file searches.  I'll add this later.  For now, assuming a ;1 should
	// be sufficient (no known games have their ELF binary as anything but version ;1)

	const wxString fixedname( wxStringTokenizer(filename, L';').GetNextToken() + L";1" );

	if( fixedname != filename )
		Console.WriteLn( Color_Blue, "(LoadELF) Non-conforming version suffix detected and replaced." );

	IsoFSCDVD isofs;
	IsoFile file(isofs, filename);
	return new ElfObject(filename, file);
}

static __forceinline void _reloadElfInfo(wxString elfpath)
{
	ScopedPtr<ElfObject> elfptr;

	// Now's a good time to reload the ELF info...
    ScopedLock locker( Mutex_NewDiskCB );

	if (elfpath == LastELF)
		return;

	LastELF = elfpath;

	wxString fname = elfpath.AfterLast('\\');
	if (!fname)
		fname = elfpath.AfterLast('/');
	if (!fname)
		fname = elfpath.AfterLast(':');
	if (fname.Matches(L"????_???.??*"))
		DiscSerial = fname(0,4) + L"-" + fname(5,3) + fname(9,2);

	elfptr = loadElf(elfpath);

	ElfCRC = elfptr->getCRC();
	Console.WriteLn("ELF (%s) CRC = %8.8X", elfpath.ToUTF8().data(), ElfCRC);

	ElfEntry = elfptr->header.e_entry;
	Console.WriteLn("Entry point = 0x%08x", ElfEntry);

	// Note: Do not load game database info here.  This code is generic and called from
	// BIOS key encryption as well as eeloadReplaceOSDSYS.  The first is actually still executing
	// BIOS code, and patches and cheats should not be applied yet.  (they are applied when
	// eeGameStarting is invoked, which is when the VM starts executing the actual game ELF
	// binary).
}

void cdvdReloadElfInfo(wxString elfoverride)
{
	// called from context of executing VM code (recompilers), so we need to trap exceptions
	// and route them through the VM's exception handler.  (needed for non-SEH platforms, such
	// as Linux/GCC)

	try
	{
		if (!elfoverride.IsEmpty())
		{
			_reloadElfInfo(elfoverride);
			return;
		}

		wxString elfpath;
		u32 discType = GetPS2ElfName(elfpath);

		if(discType==1)
		{
			// Is a PS1 disc.
			if (!ENABLE_LOADING_PS1_GAMES)
				Cpu->ThrowException( Exception::RuntimeError()
					.SetDiagMsg(L"PSX game discs are not supported by PCSX2.")
					.SetUserMsg(pxE( "Error:PsxDisc",
						L"Playstation game discs are not supported by PCSX2.  If you want to emulate PSX games "
						L"then you'll have to download a PSX-specific emulator, such as ePSXe or PCSX.")
					)
				);
				//Console.Error( "Playstation1 game discs are not supported by PCSX2." );
		}
		
		// Isn't a disc we recognize?
		if(discType == 0)  return;

		// Recognized and PS2 (BOOT2).  Good job, user.
		_reloadElfInfo(elfpath);
	}
	catch (Exception::FileNotFound& e)
	{
		pxFail( "Not in my back yard!" );
		Cpu->ThrowException(e);
	}
}

static __forceinline s32 StrToS32(const wxString& str, int base = 10)
{
    long l;
    str.ToLong(&l, base);
    return l;
}

void cdvdReadKey(u8 arg0, u16 arg1, u32 arg2, u8* key)
{
	s32 numbers, letters;
	u32 key_0_3;
	u8 key_4, key_14;

    cdvdReloadElfInfo();

	// convert the number characters to a real 32 bit number
	numbers = StrToS32(DiscSerial(5,5));

	// combine the lower 7 bits of each char
	// to make the 4 letters fit into a single u32
	letters =	(s32)((DiscSerial[3]&0x7F)<< 0) |
				(s32)((DiscSerial[2]&0x7F)<< 7) |
				(s32)((DiscSerial[1]&0x7F)<<14) |
				(s32)((DiscSerial[0]&0x7F)<<21);

	// calculate magic numbers
	key_0_3 = ((numbers & 0x1FC00) >> 10) | ((0x01FFFFFF & letters) <<  7);	// numbers = 7F  letters = FFFFFF80
	key_4   = ((numbers & 0x0001F) <<  3) | ((0x0E000000 & letters) >> 25);	// numbers = F8  letters = 07
	key_14  = ((numbers & 0x003E0) >>  2) | 0x04;							// numbers = F8  extra   = 04  unused = 03

	// clear key values
	memzero_ptr<16>(key);

	// store key values
	key[ 0] = (key_0_3&0x000000FF)>> 0;
	key[ 1] = (key_0_3&0x0000FF00)>> 8;
	key[ 2] = (key_0_3&0x00FF0000)>>16;
	key[ 3] = (key_0_3&0xFF000000)>>24;
	key[ 4] = key_4;

    switch (arg2)
    {
        case 75:
            key[14] = key_14;
            key[15] = 0x05;
            break;

//      case 3075:
//          key[15] = 0x01;
//          break;

        case 4246:
            // 0x0001F2F707 = sector 0x0001F2F7  dec 0x07
            key[ 0] = 0x07;
            key[ 1] = 0xF7;
            key[ 2] = 0xF2;
            key[ 3] = 0x01;
            key[ 4] = 0x00;
            key[15] = 0x01;
            break;

        default:
            key[15] = 0x01;
            break;
    }

	Console.WriteLn( "CDVD.KEY = %02X,%02X,%02X,%02X,%02X,%02X,%02X",
		cdvd.Key[0],cdvd.Key[1],cdvd.Key[2],cdvd.Key[3],cdvd.Key[4],cdvd.Key[14],cdvd.Key[15] );
}

s32 cdvdGetToc(void* toc)
{
	s32 ret = CDVD->getTOC(toc);
	if (ret == -1) ret = 0x80;
	return ret;
}

s32 cdvdReadSubQ(s32 lsn, cdvdSubQ* subq)
{
	s32 ret = CDVD->readSubQ(lsn, subq);
	if (ret == -1) ret = 0x80;
	return ret;
}

s32 cdvdCtrlTrayOpen()
{
	Console.Warning("Open virtual disk tray");
	DiscSwapTimerSeconds = cdvd.RTC.second; // remember the PS2 time when this happened
	cdvd.Status = CDVD_STATUS_TRAY_OPEN;
	cdvd.Ready = CDVD_NOTREADY;
	trayState = 1;

	return 0; // needs to be 0 for success according to homebrew test "CDVD"
}

s32 cdvdCtrlTrayClose()
{
	Console.Warning("Close virtual disk tray");
	cdvd.Status = CDVD_STATUS_PAUSE;
	cdvd.Ready = CDVD_READY1;
	trayState = 0;

	return 0; // needs to be 0 for success according to homebrew test "CDVD"
}

// Some legacy function, not used anymore
s32 cdvdGetTrayStatus()
{
	/*s32 ret = CDVD->getTrayStatus();

	if (ret == -1)
		return(CDVD_TRAY_CLOSE);
	else
		return(ret);*/
	return -1;
}

// Note: Is tray status being kept as a var here somewhere?
// Yep, and sceCdTrayReq needs it to detect tray state changes (rama)

//   cdvdNewDiskCB() can update it's status as well...

// Modified by (efp) - 16/01/2006
static __forceinline void cdvdGetDiskType()
{
	cdvd.Type = DoCDVDdetectDiskType();
}

// check whether disc is single or dual layer
// if its dual layer, check what the disctype is and what sector number
// layer1 starts at
//
// args:    gets value for dvd type (0=single layer, 1=ptp, 2=otp)
//          gets value for start lsn of layer1
// returns: 1 if on dual layer disc
//          0 if not on dual layer disc
static s32 cdvdReadDvdDualInfo(s32* dualType, u32* layer1Start)
{
	*dualType = 0;
	*layer1Start = 0;

	return CDVD->getDualInfo(dualType,layer1Start);
}

static uint cdvdBlockReadTime( CDVD_MODE_TYPE mode )
{
	return (PSXCLK * cdvd.BlockSize) / (((mode==MODE_CDROM) ? PSX_CD_READSPEED : PSX_DVD_READSPEED) * cdvd.Speed);
}

void cdvdReset()
{
	memzero(cdvd);

	cdvd.Type = CDVD_TYPE_NODISC;
	cdvd.Spinning = false;

	cdvd.sDataIn = 0x40;
	cdvd.Ready = CDVD_READY2;
	cdvd.Speed = 4;
	cdvd.BlockSize = 2064;
	cdvd.Action = cdvdAction_None;
	cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );

	// CDVD internally uses GMT+9.  If you think the time's wrong, you're wrong.
	// Set up your time zone and winter/summer in the BIOS.  No PS2 BIOS I know of features automatic DST.
	wxDateTime curtime( wxDateTime::GetTimeNow() );
	cdvd.RTC.second = (u8)curtime.GetSecond();
	cdvd.RTC.minute = (u8)curtime.GetMinute();
	cdvd.RTC.hour = (u8)curtime.GetHour(wxDateTime::GMT9);
	cdvd.RTC.day = (u8)curtime.GetDay(wxDateTime::GMT9);
 	cdvd.RTC.month = (u8)curtime.GetMonth(wxDateTime::GMT9) + 1; // WX returns Jan as "0"
 	cdvd.RTC.year = (u8)(curtime.GetYear(wxDateTime::GMT9) - 2000);
}

struct Freeze_v10Compat
{
	u8  Action;
	u32 SeekToSector;
	u32 ReadTime;
	bool Spinning;
};

void SaveStateBase::cdvdFreeze()
{
	FreezeTag( "cdvd" );
	Freeze( cdvd );

	if (IsLoading())
	{
		// Make sure the Cdvd plugin has the expected track loaded into the buffer.
		// If cdvd.Readed is cleared it means we need to load the SeekToSector (ie, a
		// seek is in progress!)

		if( cdvd.Reading )
			cdvd.RErr = DoCDVDreadTrack( cdvd.Readed ? cdvd.Sector : cdvd.SeekToSector, cdvd.ReadMode);
	}
}

static void cdvdDetectDisk()
{
	wxString str;
	cdvd.Type = DoCDVDdetectDiskType();
    cdvdReloadElfInfo();
}

void cdvdNewDiskCB()
{
	ScopedTryLock lock( Mutex_NewDiskCB );
	if( lock.Failed() ) return;
	
	DoCDVDresetDiskTypeCache();
	cdvdDetectDisk();
}

static void mechaDecryptBytes( u32 madr, int size )
{
	int shiftAmount = (cdvd.decSet>>4) & 7;
	int doXor = (cdvd.decSet) & 1;
	int doShift = (cdvd.decSet) & 2;

	u8* curval = iopPhysMem( madr );
	for( int i=0; i<size; ++i, ++curval )
	{
		if( doXor ) *curval ^= cdvd.Key[4];
		if( doShift ) *curval = (*curval >> shiftAmount) | (*curval << (8-shiftAmount) );
	}
}

int cdvdReadSector() {
	s32 bcr;

	CDVD_LOG("SECTOR %d (BCR %x;%x)", cdvd.Sector, HW_DMA3_BCR_H16, HW_DMA3_BCR_L16);

	bcr = (HW_DMA3_BCR_H16 * HW_DMA3_BCR_L16) *4;
	if (bcr < cdvd.BlockSize) {
		CDVD_LOG( "READBLOCK:  bcr < cdvd.BlockSize; %x < %x", bcr, cdvd.BlockSize );
		if (HW_DMA3_CHCR & 0x01000000) {
			HW_DMA3_CHCR &= ~0x01000000;
			psxDmaInterrupt(3);
		}
		return -1;
	}

	// DMAs use physical addresses (air)
	u8* mdest = iopPhysMem( HW_DMA3_MADR );

	// if raw dvd sector 'fill in the blanks'
	if (cdvd.BlockSize == 2064)
	{
		// get info on dvd type and layer1 start
		u32 layer1Start;
		s32 dualType;
		s32 layerNum;
		u32 lsn = cdvd.Sector;

		cdvdReadDvdDualInfo(&dualType, &layer1Start);

		if((dualType == 1) && (lsn >= layer1Start))
		{
			// dual layer ptp disc
			layerNum = 1;
			lsn = lsn - layer1Start + 0x30000;
		}
		else if((dualType == 2) && (lsn >= layer1Start))
		{
			// dual layer otp disc
			layerNum = 1;
			lsn = ~(layer1Start+0x30000 - 1);
		}
		else
		{
			// Assuming the other dualType is 0,
			// single layer disc, or on first layer of dual layer disc.
			layerNum = 0;
			lsn += 0x30000;
		}

		mdest[0] = 0x20 | layerNum;
		mdest[1] = (u8)(lsn >> 16);
		mdest[2] = (u8)(lsn >>  8);
		mdest[3] = (u8)(lsn      );

		// sector IED (not calculated at present)
		mdest[4] = 0;
		mdest[5] = 0;

		// sector CPR_MAI (not calculated at present)
		mdest[6] = 0;
		mdest[7] = 0;
		mdest[8] = 0;
		mdest[9] = 0;
		mdest[10] = 0;
		mdest[11] = 0;

		// normal 2048 bytes of sector data
		memcpy_const(&mdest[12], cdr.Transfer, 2048);

		// 4 bytes of edc (not calculated at present)
		mdest[2060] = 0;
		mdest[2061] = 0;
		mdest[2062] = 0;
		mdest[2063] = 0;
	}
	else
	{
		memcpy_fast( mdest, cdr.Transfer, cdvd.BlockSize);
	}

	// decrypt sector's bytes
	if( cdvd.decSet ) mechaDecryptBytes( HW_DMA3_MADR, cdvd.BlockSize );

	// Added a clear after memory write .. never seemed to be necessary before but *should*
	// be more correct. (air)
	psxCpu->Clear( HW_DMA3_MADR, cdvd.BlockSize/4 );

//	Console.WriteLn("sector %x;%x;%x", PSXMu8(madr+0), PSXMu8(madr+1), PSXMu8(madr+2));

	HW_DMA3_BCR_H16 -= (cdvd.BlockSize / (HW_DMA3_BCR_L16*4));
	HW_DMA3_MADR += cdvd.BlockSize;

	return 0;
}

// inlined due to being referenced in only one place.
__forceinline void cdvdActionInterrupt()
{
	switch( cdvd.Action )
	{
		case cdvdAction_Seek:
			cdvd.Spinning = true;
			cdvd.Ready  = CDVD_READY1; //check (rama)
			cdvd.Sector = cdvd.SeekToSector;
			cdvd.Status = CDVD_STATUS_PAUSE;
		break;

		case cdvdAction_Standby:
			DevCon.Warning("CDVD Standby Call");
			cdvd.Spinning = true; //check (rama)
			cdvd.Ready  = CDVD_READY1; //check (rama)
			cdvd.Sector = cdvd.SeekToSector;
			cdvd.Status = CDVD_STATUS_PAUSE;
		break;

		case cdvdAction_Stop:
			cdvd.Spinning = false;
			cdvd.Ready = CDVD_READY1;
			cdvd.Sector = 0;
			cdvd.Status = CDVD_STATUS_STOP;
		break;

		case cdvdAction_Break:
			// Make sure the cdvd action state is pretty well cleared:
			DevCon.Warning("CDVD Break Call");
			cdvd.Reading = 0;
			cdvd.Readed = 0;
			cdvd.Ready  = CDVD_READY2;		// should be CDVD_READY1 or something else?
			cdvd.Status = CDVD_STATUS_STOP;
			cdvd.RErr = 0;
			cdvd.nCommand = 0;
		break;
	}
	cdvd.Action = cdvdAction_None;

	cdvd.PwOff |= 1<<Irq_CommandComplete;
	psxHu32(0x1070)|= 0x4;
	hwIntcIrq(INTC_SBUS);
}

// inlined due to being referenced in only one place.
__forceinline void cdvdReadInterrupt()
{
	//Console.WriteLn("cdvdReadInterrupt %x %x %x %x %x", cpuRegs.interrupt, cdvd.Readed, cdvd.Reading, cdvd.nSectors, (HW_DMA3_BCR_H16 * HW_DMA3_BCR_L16) *4);

	cdvd.Ready = CDVD_NOTREADY;
	if (!cdvd.Readed)
	{
		// Seeking finished.  Process the track we requested before, and
		// then schedule another CDVD read int for when the block read finishes.

		// NOTE: The first CD track was read when the seek was initiated, so no need
		// to call CDVDReadTrack here.

		cdvd.Spinning = true;
		cdvd.RetryCntP = 0;
		cdvd.Reading = 1;
		cdvd.Readed = 1;
		cdvd.Status = CDVD_STATUS_PAUSE; // check (rama)
		cdvd.Sector = cdvd.SeekToSector;

		CDVD_LOG( "Cdvd Seek Complete > Scheduling block read interrupt at iopcycle=%8.8x.",
			psxRegs.cycle + cdvd.ReadTime );

		CDVDREAD_INT(cdvd.ReadTime);
		return;
	}
	else
	{
		if( cdvd.RErr == 0 )
		{
			while( (cdvd.RErr = DoCDVDgetBuffer(cdr.Transfer)), cdvd.RErr == -2 )
			{
				// not finished yet ... block on the read until it finishes.
				Threading::Sleep( 0 );
				Threading::SpinWait();
			}
		}

		if (cdvd.RErr == -1)
		{
			cdvd.RetryCntP++;

			if (cdvd.RetryCntP <= cdvd.RetryCnt)
			{
				CDVD_LOG( "CDVD read err, retrying... (attempt %d of %d)", cdvd.RetryCntP, cdvd.RetryCnt );
				cdvd.RErr = DoCDVDreadTrack(cdvd.Sector, cdvd.ReadMode);
				CDVDREAD_INT(cdvd.ReadTime);
			}
			else
				Console.Error("CDVD READ ERROR, sector = 0x%08x", cdvd.Sector);

			return;
		}

		cdvd.Reading = false;

		// Any other value besides 0 should be considered invalid here (wtf is that wacky
		// plugin trying to do?)
		pxAssume( cdvd.RErr == 0 );
	}

	if (cdvdReadSector() == -1)
	{
		// This means that the BCR/DMA hasn't finished yet, and rather than fire off the
		// sector-finished notice too early (which might overwrite game data) we delay a
		// bit and try to read the sector again later.
		// An arbitrary delay of some number of cycles probably makes more sense here,
		// but for now it's based on the cdvd.ReadTime value. -- air

		pxAssume((int)cdvd.ReadTime > 0 );
		CDVDREAD_INT(cdvd.ReadTime/4);
		return;
	}

	cdvd.Sector++;

	if (--cdvd.nSectors <= 0)
	{
		cdvd.PwOff |= 1<<Irq_CommandComplete;
		psxHu32(0x1070)|= 0x4;
		hwIntcIrq(INTC_SBUS);

		HW_DMA3_CHCR &= ~0x01000000;
		psxDmaInterrupt(3);
		cdvd.Ready = CDVD_READY2;

		// All done! :D
		return;
	}

	cdvd.RetryCntP = 0;
	cdvd.Reading = 1;
	cdvd.RErr = DoCDVDreadTrack(cdvd.Sector, cdvd.ReadMode);
	CDVDREAD_INT(cdvd.ReadTime);

	return;
}

// Returns the number of IOP cycles until the event completes.
static uint cdvdStartSeek( uint newsector, CDVD_MODE_TYPE mode )
{
	cdvd.SeekToSector = newsector;

	uint delta = abs( (s32)(cdvd.SeekToSector - cdvd.Sector) );
	uint seektime;

	cdvd.Ready = CDVD_NOTREADY;
	cdvd.Reading = 0;
	cdvd.Readed = 0;
	cdvd.Status = CDVD_STATUS_STOP;

	if( !cdvd.Spinning )
	{
		CDVD_LOG( "CdSpinUp > Simulating CdRom Spinup Time, and seek to sector %d", cdvd.SeekToSector );
		seektime = PSXCLK / 3;		// 333ms delay
		cdvd.Spinning = true;
	}
	else if( (tbl_ContigiousSeekDelta[mode] == 0) || (delta >= tbl_ContigiousSeekDelta[mode]) )
	{
		// Select either Full or Fast seek depending on delta:

		if( delta >= tbl_FastSeekDelta[mode] )
		{
			// Full Seek
			CDVD_LOG( "CdSeek Begin > to sector %d, from %d - delta=%d [FULL]", cdvd.SeekToSector, cdvd.Sector, delta );
			seektime = Cdvd_FullSeek_Cycles;
		}
		else
		{
			CDVD_LOG( "CdSeek Begin > to sector %d, from %d - delta=%d [FAST]", cdvd.SeekToSector, cdvd.Sector, delta );
			seektime = Cdvd_FastSeek_Cycles;
		}
	}
	else
	{
		CDVD_LOG( "CdSeek Begin > Contiguous block without seek - delta=%d sectors", delta );

		// seektime is the time it takes to read to the destination block:
		seektime = delta * cdvd.ReadTime;

		if( delta == 0 )
		{
			cdvd.Status = CDVD_STATUS_PAUSE;
			cdvd.Readed = 1; // Note: 1, not 0, as implied by the next comment. Need to look into this. --arcum42
			cdvd.RetryCntP = 0;

			// setting Readed to 0 skips the seek logic, which means the next call to
			// cdvdReadInterrupt will load a block.  So make sure it's properly scheduled
			// based on sector read speeds:

			seektime = cdvd.ReadTime;
		}
	}

	return seektime;
}

u8 monthmap[13] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

void cdvdVsync() {
	cdvd.RTCcount++;
	if (cdvd.RTCcount < ((gsRegionMode == Region_NTSC) ? 60 : 50)) return;
	cdvd.RTCcount = 0;

	if ( cdvd.Status == CDVD_STATUS_TRAY_OPEN )
	{	
		if ( cdvd.RTC.second != DiscSwapTimerSeconds)
		{
			cdvdCtrlTrayClose();
		}
	}

	cdvd.RTC.second++;
	if (cdvd.RTC.second < 60) return;
	cdvd.RTC.second = 0;

	cdvd.RTC.minute++;
	if (cdvd.RTC.minute < 60) return;
	cdvd.RTC.minute = 0;

	cdvd.RTC.hour++;
	if (cdvd.RTC.hour < 24) return;
	cdvd.RTC.hour = 0;

	cdvd.RTC.day++;
	if (cdvd.RTC.day <= (cdvd.RTC.month == 2 && cdvd.RTC.year % 4 == 0 ? 29 : monthmap[cdvd.RTC.month-1])) return;
	cdvd.RTC.day = 1;

	cdvd.RTC.month++;
	if (cdvd.RTC.month <= 12) return;
	cdvd.RTC.month = 1;

	cdvd.RTC.year++;
	if (cdvd.RTC.year < 100) return;
	cdvd.RTC.year = 0;
}

static __forceinline u8 cdvdRead18(void)  // SDATAOUT
{
	u8 ret = 0;

	if (((cdvd.sDataIn & 0x40) == 0) && (cdvd.ResultP < cdvd.ResultC))
	{
		cdvd.ResultP++;
		if (cdvd.ResultP >= cdvd.ResultC) cdvd.sDataIn|= 0x40;
		ret = cdvd.Result[cdvd.ResultP-1];
	}
	CDVD_LOG("cdvdRead18(SDataOut) %x (ResultC=%d, ResultP=%d)", ret, cdvd.ResultC, cdvd.ResultP);

	return ret;
}

u8 cdvdRead(u8 key)
{
	switch (key)
	{
		case 0x04:  // NCOMMAND
			CDVD_LOG("cdvdRead04(NCMD) %x", cdvd.nCommand);
			return cdvd.nCommand;
			break;

		case 0x05: // N-READY
			CDVD_LOG("cdvdRead05(NReady) %x", cdvd.Ready);
			return cdvd.Ready;
			break;

		case 0x06:  // ERROR
			CDVD_LOG("cdvdRead06(Error) %x", cdvd.Error);
			return cdvd.Error;
			break;

		case 0x07:  // BREAK
			CDVD_LOG("cdvdRead07(Break) %x", 0);
			return 0;
			break;

		case 0x08:  // STATUS
			CDVD_LOG("cdvdRead08(Status) %x", cdvd.Status);
			return cdvd.Status;
			break;

		case 0x0A:  // STATUS
			CDVD_LOG("cdvdRead0A(Status) %x", cdvd.Status);
			return cdvd.Status;
			break;

		case 0x0B: // TRAY-STATE (if tray has been opened)
		{
			CDVD_LOG("cdvdRead0B(Tray) %x", trayState);
			return /*tray*/ trayState;
			break;
		}
		case 0x0C: // CRT MINUTE
			CDVD_LOG("cdvdRead0C(Min) %x", itob((u8)(cdvd.Sector/(60*75))));
			return itob((u8)(cdvd.Sector/(60*75)));
			break;

		case 0x0D: // CRT SECOND
			CDVD_LOG("cdvdRead0D(Sec) %x", itob((u8)((cdvd.Sector/75)%60)+2));
			return itob((u8)((cdvd.Sector/75)%60)+2);
			break;

		case 0x0E:  // CRT FRAME
			CDVD_LOG("cdvdRead0E(Frame) %x", itob((u8)(cdvd.Sector%75)));
			return itob((u8)(cdvd.Sector%75));
			break;

		case 0x0F: // TYPE
			CDVD_LOG("cdvdRead0F(Disc Type) %x", cdvd.Type);
			cdvdGetDiskType();
			return cdvd.Type;
			break;

		case 0x13: // UNKNOWN
			CDVD_LOG("cdvdRead13(Unknown) %x", 4);
			return 4;
			break;

		case 0x15: // RSV
			CDVD_LOG("cdvdRead15(RSV)");
			return 0x01; // | 0x80 for ATAPI mode
			break;

		case 0x16: // SCOMMAND
			CDVD_LOG("cdvdRead16(SCMD) %x", cdvd.sCommand);
			return cdvd.sCommand;
			break;

		case 0x17:  // SREADY
			CDVD_LOG("cdvdRead17(SReady) %x", cdvd.sDataIn);
			return cdvd.sDataIn;
			break;

		case 0x18:
			return cdvdRead18();
			break;

		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		{
			int temp = key - 0x20;

			CDVD_LOG("cdvdRead%d(Key%d) %x", key, temp, cdvd.Key[temp]);
			return cdvd.Key[temp];
			break;
		}
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		{
			int temp = key - 0x23;

			CDVD_LOG("cdvdRead%d(Key%d) %x", key, temp, cdvd.Key[temp]);
			return cdvd.Key[temp];
			break;
		}

		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		{
			int temp = key - 0x26;

			CDVD_LOG("cdvdRead%d(Key%d) %x", key, temp, cdvd.Key[temp]);
			return cdvd.Key[temp];
			break;
		}

		case 0x38: 		// valid parts of key data (first and last are valid)
			CDVD_LOG("cdvdRead38(KeysValid) %x", cdvd.Key[15]);

			return cdvd.Key[15];
			break;

		case 0x39:	// KEY-XOR
			CDVD_LOG("cdvdRead39(KeyXor) %x", cdvd.KeyXor);

			return cdvd.KeyXor;
			break;

		case 0x3A: 	// DEC_SET
			CDVD_LOG("cdvdRead3A(DecSet) %x", cdvd.decSet);

			Console.WriteLn("DecSet Read: %02X", cdvd.decSet);
			return cdvd.decSet;
			break;

		default:
			// note: notify the console since this is a potentially serious emulation problem:
			PSXHW_LOG("*Unknown 8bit read at address 0x1f4020%x", key);
			Console.Error( "IOP Unknown 8bit read from addr 0x1f4020%x", key );
			return 0;
			break;
	}
}

static void cdvdWrite04(u8 rt) { // NCOMMAND
	CDVD_LOG("cdvdWrite04: NCMD %s (%x) (ParamP = %x)", nCmdName[rt], rt, cdvd.ParamP);

	cdvd.nCommand = rt;
	cdvd.Status = CDVD_STATUS_STOP; // check (rama)
	cdvd.PwOff = Irq_None;		// good or bad?

	switch (rt) {
		case N_CD_SYNC: // CdSync
		case N_CD_NOP: // CdNop_
			cdvdSetIrq();
		break;

		case N_CD_STANDBY: // CdStandby

			// Seek to sector zero.  The cdvdStartSeek function will simulate
			// spinup times if needed.

			DevCon.Warning( "CdStandby : %d", rt );
			cdvd.Action = cdvdAction_Standby;
			cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );
			CDVD_INT( cdvdStartSeek( 0, MODE_DVDROM ) );
		break;

		case N_CD_STOP: // CdStop
			DevCon.Warning( "CdStop : %d", rt );
			cdvd.Action = cdvdAction_Stop;
			CDVD_INT( PSXCLK / 6 );		// 166ms delay?
		break;

		// from an emulation point of view there is not much need to do anything for this one
		case N_CD_PAUSE: // CdPause
			cdvdSetIrq();
		break;

		case N_CD_SEEK: // CdSeek
			cdvd.Action = cdvdAction_Seek;
			cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );
			CDVD_INT( cdvdStartSeek( *(uint*)(cdvd.Param+0), MODE_DVDROM ) );
		break;

		case N_CD_READ: // CdRead
			// Assign the seek to sector based on cdvd.Param[0]-[3], and the number of  sectors based on cdvd.Param[4]-[7].
			cdvd.SeekToSector =  *(u32*) (cdvd.Param+0);
			cdvd.nSectors =  *(u32*)(cdvd.Param+4);
			cdvd.RetryCnt = (cdvd.Param[8] == 0) ? 0x100 : cdvd.Param[8];
			cdvd.SpindlCtrl = cdvd.Param[9];
			cdvd.Speed = 24;
			switch (cdvd.Param[10]) {
				case 2: cdvd.ReadMode = CDVD_MODE_2340; cdvd.BlockSize = 2340; break;
				case 1: cdvd.ReadMode = CDVD_MODE_2328; cdvd.BlockSize = 2328; break;
				case 0:
				default: cdvd.ReadMode = CDVD_MODE_2048; cdvd.BlockSize = 2048; break;
			}

			CDVD_LOG( "CdRead > startSector=%d, nSectors=%d, RetryCnt=%x, Speed=%x(%x), ReadMode=%x(%x) (1074=%x)",
				cdvd.Sector, cdvd.nSectors, cdvd.RetryCnt, cdvd.Speed, cdvd.Param[9], cdvd.ReadMode, cdvd.Param[10], psxHu32(0x1074));

			if( EmuConfig.CdvdVerboseReads )
				Console.WriteLn("CdRead: Reading Sector %d(%d Blocks of Size %d) at Speed=%dx",
					cdvd.Sector, cdvd.nSectors,cdvd.BlockSize,cdvd.Speed);

			cdvd.ReadTime = cdvdBlockReadTime( MODE_CDROM );
			CDVDREAD_INT( cdvdStartSeek( cdvd.SeekToSector,MODE_CDROM ) );

			// Read-ahead by telling the plugin about the track now.
			// This helps improve performance on actual from-cd emulation
			// (ie, not using the hard drive)
			cdvd.RErr = DoCDVDreadTrack( cdvd.SeekToSector, cdvd.ReadMode );

			// Set the reading block flag.  If a seek is pending then Readed will
			// take priority in the handler anyway.  If the read is contiguous then
			// this'll skip the seek delay.
			cdvd.Reading = 1;
			break;

		case N_CD_READ_CDDA: // CdReadCDDA
		case N_CD_READ_XCDDA: // CdReadXCDDA
			// Assign the seek to sector based on cdvd.Param[0]-[3], and the number of  sectors based on cdvd.Param[4]-[7].
			cdvd.SeekToSector =  *(u32*) (cdvd.Param+0);
			cdvd.nSectors =  *(u32*)(cdvd.Param+4);

			if (cdvd.Param[8] == 0)
				cdvd.RetryCnt = 0x100;
			else
				cdvd.RetryCnt = cdvd.Param[8];

			cdvd.SpindlCtrl = cdvd.Param[9];

			switch (cdvd.Param[9]) {
				case 0x01: cdvd.Speed =  1; break;
				case 0x02: cdvd.Speed =  2; break;
				case 0x03: cdvd.Speed =  4; break;
				case 0x04: cdvd.Speed = 12; break;
				default:   cdvd.Speed = 24; break;
			}

			switch (cdvd.Param[10]) {
				case 1: cdvd.ReadMode = CDVD_MODE_2368; cdvd.BlockSize = 2368; break;
				case 2:
				case 0: cdvd.ReadMode = CDVD_MODE_2352; cdvd.BlockSize = 2352; break;
			}

			CDVD_LOG( "CdReadCDDA > startSector=%d, nSectors=%d, RetryCnt=%x, Speed=%xx(%x), ReadMode=%x(%x) (1074=%x)",
				cdvd.Sector, cdvd.nSectors, cdvd.RetryCnt, cdvd.Speed, cdvd.Param[9], cdvd.ReadMode, cdvd.Param[10], psxHu32(0x1074));

			if( EmuConfig.CdvdVerboseReads )
				Console.WriteLn("CdAudioRead: Reading Sector %d(%d Blocks of Size %d) at Speed=%dx",
					cdvd.Sector, cdvd.nSectors,cdvd.BlockSize,cdvd.Speed);

			cdvd.ReadTime = cdvdBlockReadTime( MODE_CDROM );
			CDVDREAD_INT( cdvdStartSeek( cdvd.SeekToSector, MODE_CDROM ) );

			// Read-ahead by telling the plugin about the track now.
			// This helps improve performance on actual from-cd emulation
			// (ie, not using the hard drive)
			cdvd.RErr = DoCDVDreadTrack( cdvd.SeekToSector, cdvd.ReadMode );

			// Set the reading block flag.  If a seek is pending then Readed will
			// take priority in the handler anyway.  If the read is contiguous then
			// this'll skip the seek delay.
			cdvd.Reading = 1;
		break;

		case N_DVD_READ: // DvdRead
			// Assign the seek to sector based on cdvd.Param[0]-[3], and the number of  sectors based on cdvd.Param[4]-[7].
			cdvd.SeekToSector =  *(u32*) (cdvd.Param+0);
			cdvd.nSectors =  *(u32*)(cdvd.Param+4);

			if (cdvd.Param[8] == 0)
				cdvd.RetryCnt = 0x100;
			else
				cdvd.RetryCnt = cdvd.Param[8];

			cdvd.SpindlCtrl = cdvd.Param[9];
			cdvd.Speed = 4;
			cdvd.ReadMode = CDVD_MODE_2048;
			cdvd.BlockSize = 2064;	// Why oh why was it 2064

			CDVD_LOG( "DvdRead > startSector=%d, nSectors=%d, RetryCnt=%x, Speed=%x(%x), ReadMode=%x(%x) (1074=%x)",
				cdvd.Sector, cdvd.nSectors, cdvd.RetryCnt, cdvd.Speed, cdvd.Param[9], cdvd.ReadMode, cdvd.Param[10], psxHu32(0x1074));

			if( EmuConfig.CdvdVerboseReads )
				Console.WriteLn("DvdRead: Reading Sector %d(%d Blocks of Size %d) at Speed=%dx",
					cdvd.Sector, cdvd.nSectors,cdvd.BlockSize,cdvd.Speed);

			cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );
			CDVDREAD_INT( cdvdStartSeek( cdvd.SeekToSector, MODE_DVDROM ) );

			// Read-ahead by telling the plugin about the track now.
			// This helps improve performance on actual from-cd emulation
			// (ie, not using the hard drive)
			cdvd.RErr = DoCDVDreadTrack( cdvd.SeekToSector, cdvd.ReadMode );

			// Set the reading block flag.  If a seek is pending then Readed will
			// take priority in the handler anyway.  If the read is contiguous then
			// this'll skip the seek delay.
			cdvd.Reading = 1;
		break;

		case N_CD_GET_TOC: // CdGetToc & cdvdman_call19
			//Param[0] is 0 for CdGetToc and any value for cdvdman_call19
			//the code below handles only CdGetToc!
			//if(cdvd.Param[0]==0x01)
			//{
			DevCon.WriteLn("CDGetToc Param[0]=%d, Param[1]=%d", cdvd.Param[0],cdvd.Param[1]);
			//}
			cdvdGetToc( iopPhysMem( HW_DMA3_MADR ) );
			cdvdSetIrq( (1<<Irq_CommandComplete) ); //| (1<<Irq_DataReady) );
			HW_DMA3_CHCR &= ~0x01000000;
			psxDmaInterrupt(3);
			break;

		case N_CD_READ_KEY: // CdReadKey
		{
			u8  arg0 = cdvd.Param[0];
			u16 arg1 = cdvd.Param[1] | (cdvd.Param[2]<<8);
			u32 arg2 = cdvd.Param[3] | (cdvd.Param[4]<<8) | (cdvd.Param[5]<<16) | (cdvd.Param[6]<<24);
			DevCon.WriteLn("cdvdReadKey(%d, %d, %d)", arg0, arg1, arg2);
			cdvdReadKey(arg0, arg1, arg2, cdvd.Key);
			cdvd.KeyXor = 0x00;
			cdvdSetIrq();
		}
		break;

		case N_CD_CHG_SPDL_CTRL: // CdChgSpdlCtrl
			Console.Warning("sceCdChgSpdlCtrl(%d)", cdvd.Param[0]);
			cdvdSetIrq();
		break;

		default:
			Console.Warning("NCMD Unknown %x", rt);
			cdvdSetIrq();
		break;
	}
	cdvd.ParamP = 0;
	cdvd.ParamC = 0;
}

static __forceinline void cdvdWrite05(u8 rt) { // NDATAIN
	CDVD_LOG("cdvdWrite05(NDataIn) %x", rt);

	if (cdvd.ParamP < 32) {
		cdvd.Param[cdvd.ParamP++] = rt;
		cdvd.ParamC++;
	}
}

static __forceinline void cdvdWrite06(u8 rt) { // HOWTO
	CDVD_LOG("cdvdWrite06(HowTo) %x", rt);
	cdvd.HowTo = rt;
}

static __forceinline void cdvdWrite07(u8 rt)		// BREAK
{
	CDVD_LOG("cdvdWrite07(Break) %x", rt);

	// If we're already in a Ready state or already Breaking, then do nothing:
	if ((cdvd.Ready != CDVD_NOTREADY) || (cdvd.Action == cdvdAction_Break)) return;

	DbgCon.WriteLn("*PCSX2*: CDVD BREAK %x", rt);

	// Aborts any one of several CD commands:
	// Pause, Seek, Read, Status, Standby, and Stop

	psxRegs.interrupt &= ~( (1<<IopEvt_Cdvd) | (1<<IopEvt_CdvdRead) );

	cdvd.Action = cdvdAction_Break;
	CDVD_INT( 64 );

	// Clear the cdvd status:
	cdvd.Readed = 0;
	cdvd.Reading = 0;
	cdvd.Status = CDVD_STATUS_STOP;
	//cdvd.nCommand = 0;
}

static __forceinline void cdvdWrite08(u8 rt) { // INTR_STAT
	CDVD_LOG("cdvdWrite08(IntrReason) = ACK(%x)", rt);
	cdvd.PwOff &= ~rt;
}

static __forceinline void cdvdWrite0A(u8 rt) { // STATUS
	CDVD_LOG("cdvdWrite0A(Status) %x", rt);
}

static __forceinline void cdvdWrite0F(u8 rt) { // TYPE
	CDVD_LOG("cdvdWrite0F(Type) %x", rt);
	DevCon.WriteLn("*PCSX2*: CDVD TYPE %x", rt);
}

static __forceinline void cdvdWrite14(u8 rt) { // PS1 MODE??
	u32 cycle = psxRegs.cycle;

	if (rt == 0xFE)
		Console.Warning("*PCSX2*: go PS1 mode DISC SPEED = FAST");
	else
		Console.Warning("*PCSX2*: go PS1 mode DISC SPEED = %dX", rt);

	psxReset();
	psxHu32(0x1f801450) = 0x8;
	psxHu32(0x1f801078) = 1;
	psxRegs.cycle = cycle;
}

static __forceinline void fail_pol_cal()
{
	Console.Error("[MG] ERROR - Make sure the file is already decrypted!!!");
	cdvd.Result[0] = 0x80;
}

static void cdvdWrite16(u8 rt)		 // SCOMMAND
{
//	cdvdTN	diskInfo;
//	cdvdTD	trackInfo;
//	int i, lbn, type, min, sec, frm, address;
	static bool oldTrayState = 0;
	int address;
	u8 tmp;

	CDVD_LOG("cdvdWrite16: SCMD %s (%x) (ParamP = %x)", sCmdName[rt], rt, cdvd.ParamP);

	cdvd.sCommand = rt;
	switch (rt) {
//		case 0x01: // GetDiscType - from cdvdman (0:1)
//			SetResultSize(1);
//			cdvd.Result[0] = 0;
//			break;

		case 0x02: // CdReadSubQ  (0:11)
			SetResultSize(11);
			cdvd.Result[0] = cdvdReadSubQ(cdvd.Sector, (cdvdSubQ*)&cdvd.Result[1]);
			break;

		case 0x03: // Mecacon-command
			switch (cdvd.Param[0])
			{
				case 0x00: // get mecha version (1:4)
					SetResultSize(4);
					cdvdGetMechaVer(&cdvd.Result[0]);
					break;

				case 0x44: // write console ID (9:1)
					SetResultSize(1);
					cdvd.Result[0] = cdvdWriteConsoleID(&cdvd.Param[1]);
					break;

				case 0x45: // read console ID (1:9)
					SetResultSize(9);
					cdvd.Result[0] = cdvdReadConsoleID(&cdvd.Result[1]);
					break;

				case 0xFD: // _sceCdReadRenewalDate (1:6) BCD
					SetResultSize(6);
					cdvd.Result[0] = 0;
					cdvd.Result[1] = 0x04;//year
					cdvd.Result[2] = 0x12;//month
					cdvd.Result[3] = 0x10;//day
					cdvd.Result[4] = 0x01;//hour
					cdvd.Result[5] = 0x30;//min
					break;

				default:
					SetResultSize(1);
					cdvd.Result[0] = 0x80;
					Console.WriteLn("*Unknown Mecacon Command param[0]=%02X", cdvd.Param[0]);
					break;
			}
			break;

		case 0x05: // CdTrayReqState  (0:1) - resets the tray open detection
			
			//Console.Warning("CdTrayReqState. trayState = %d oldTrayState = %d",trayState, oldTrayState);
			SetResultSize(1);
			if (trayState != oldTrayState)
				cdvd.Result[0] = 1;
			else
				cdvd.Result[0] = 0; // old behaviour was always this

			oldTrayState = trayState;
			break;

		case 0x06: // CdTrayCtrl  (1:1)
			SetResultSize(1);
			if(cdvd.Param[0] == 0)
				cdvd.Result[0] = cdvdCtrlTrayOpen();
			else
				cdvd.Result[0] = cdvdCtrlTrayClose();
			break;

		case 0x08: // CdReadRTC (0:8)
			SetResultSize(8);
			cdvd.Result[0] = 0;
			cdvd.Result[1] = itob(cdvd.RTC.second); //Seconds
			cdvd.Result[2] = itob(cdvd.RTC.minute); //Minutes
			cdvd.Result[3] = itob(cdvd.RTC.hour); //Hours
			cdvd.Result[4] = 0; //Nothing
			cdvd.Result[5] = itob(cdvd.RTC.day); //Day
			cdvd.Result[6] = itob(cdvd.RTC.month); //Month
			cdvd.Result[7] = itob(cdvd.RTC.year); //Year
			/*Console.WriteLn("RTC Read Sec %x Min %x Hr %x Day %x Month %x Year %x", cdvd.Result[1], cdvd.Result[2],
				cdvd.Result[3], cdvd.Result[5], cdvd.Result[6], cdvd.Result[7]);
			Console.WriteLn("RTC Read Real Sec %d Min %d Hr %d Day %d Month %d Year %d", cdvd.RTC.second, cdvd.RTC.minute,
				cdvd.RTC.hour, cdvd.RTC.day, cdvd.RTC.month, cdvd.RTC.year);*/
			break;

		case 0x09: // sceCdWriteRTC (7:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			cdvd.RTC.pad = 0;

			cdvd.RTC.second = btoi(cdvd.Param[cdvd.ParamP-7]);
			cdvd.RTC.minute = btoi(cdvd.Param[cdvd.ParamP-6]) % 60;
			cdvd.RTC.hour = btoi(cdvd.Param[cdvd.ParamP-5]) % 24;
			cdvd.RTC.day = btoi(cdvd.Param[cdvd.ParamP-3]);
			cdvd.RTC.month = btoi(cdvd.Param[cdvd.ParamP-2] & 0x7f);
			cdvd.RTC.year = btoi(cdvd.Param[cdvd.ParamP-1]);
			/*Console.WriteLn("RTC write incomming Sec %x Min %x Hr %x Day %x Month %x Year %x", cdvd.Param[cdvd.ParamP-7], cdvd.Param[cdvd.ParamP-6],
				cdvd.Param[cdvd.ParamP-5], cdvd.Param[cdvd.ParamP-3], cdvd.Param[cdvd.ParamP-2], cdvd.Param[cdvd.ParamP-1]);
			Console.WriteLn("RTC Write Sec %d Min %d Hr %d Day %d Month %d Year %d", cdvd.RTC.second, cdvd.RTC.minute,
				cdvd.RTC.hour, cdvd.RTC.day, cdvd.RTC.month, cdvd.RTC.year);*/
			//memcpy_fast((u8*)&cdvd.RTC, cdvd.Param, 7);
			break;

		case 0x0A: // sceCdReadNVM (2:3)
			address = (cdvd.Param[0]<<8) | cdvd.Param[1];

			if (address < 512)
			{
				SetResultSize(3);
				cdvd.Result[0] = cdvdReadNVM(&cdvd.Result[1], address*2, 2);
				// swap bytes around
				tmp = cdvd.Result[1];
				cdvd.Result[1] = cdvd.Result[2];
				cdvd.Result[2] = tmp;
			}
			else
			{
				SetResultSize(1);
				cdvd.Result[0] = 0xff;
			}
			break;

		case 0x0B: // sceCdWriteNVM (4:1)
			SetResultSize(1);
			address = (cdvd.Param[0]<<8) | cdvd.Param[1];

			if (address < 512)
			{
				// swap bytes around
				tmp = cdvd.Param[2];
				cdvd.Param[2] = cdvd.Param[3];
				cdvd.Param[3] = tmp;
				cdvd.Result[0] = cdvdWriteNVM(&cdvd.Param[2], address*2, 2);
			}
			else
			{
				cdvd.Result[0] = 0xff;
			}
			break;

//		case 0x0C: // sceCdSetHDMode (1:1)
//			break;


		case 0x0F: // sceCdPowerOff (0:1)- Call74 from Xcdvdman
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x12: // sceCdReadILinkId (0:9)
			SetResultSize(9);
			cdvd.Result[0] = cdvdReadILinkID(&cdvd.Result[1]);
			break;

		case 0x13: // sceCdWriteILinkID (8:1)
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteILinkID(&cdvd.Param[1]);
			break;

		case 0x14: // CdCtrlAudioDigitalOut (1:1)
			//parameter can be 2, 0, ...
			SetResultSize(1);
			cdvd.Result[0] = 0;		//8 is a flag; not used
			break;

		case 0x15: // sceCdForbidDVDP (0:1)
			//Console.WriteLn("sceCdForbidDVDP");
			SetResultSize(1);
			cdvd.Result[0] = 5;
			break;

		case 0x16: // AutoAdjustCtrl - from cdvdman (1:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x17: // CdReadModelNumber (1:9) - from xcdvdman
			SetResultSize(9);
			cdvd.Result[0] = cdvdReadModelNumber(&cdvd.Result[1], cdvd.Param[0]);
			break;

		case 0x18: // CdWriteModelNumber (9:1) - from xcdvdman
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteModelNumber(&cdvd.Param[1], cdvd.Param[0]);
			break;

//		case 0x19: // sceCdForbidRead (0:1) - from xcdvdman
//			break;

		case 0x1A: // sceCdBootCertify (4:1)//(4:16 in psx?)
			SetResultSize(1);//on input there are 4 bytes: 1;?10;J;C for 18000; 1;60;E;C for 39002 from ROMVER
			cdvd.Result[0] = 1;//i guess that means okay
			break;

		case 0x1B: // sceCdCancelPOffRdy (0:1) - Call73 from Xcdvdman (1:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x1C: // sceCdBlueLEDCtl (1:1) - Call72 from Xcdvdman
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

//		case 0x1D: // cdvdman_call116 (0:5) - In V10 Bios
//			break;

		case 0x1E: // sceRemote2Read (0:5) - // 00 14 AA BB CC -> remote key code
			SetResultSize(5);
			cdvd.Result[0] = 0x00;
			cdvd.Result[1] = 0x14;
			cdvd.Result[2] = 0x00;
			cdvd.Result[3] = 0x00;
			cdvd.Result[4] = 0x00;
			break;

//		case 0x1F: // sceRemote2_7 (2:1) - cdvdman_call117
//			break;

		case 0x20: // sceRemote2_6 (0:3)	// 00 01 00
			SetResultSize(3);
			cdvd.Result[0] = 0x00;
			cdvd.Result[1] = 0x01;
			cdvd.Result[2] = 0x00;
			break;

//		case 0x21: // sceCdWriteWakeUpTime (8:1)
//			break;

		case 0x22: // sceCdReadWakeUpTime (0:10)
			SetResultSize(10);
			cdvd.Result[0] = 0;
			cdvd.Result[1] = 0;
			cdvd.Result[2] = 0;
			cdvd.Result[3] = 0;
			cdvd.Result[4] = 0;
			cdvd.Result[5] = 0;
			cdvd.Result[6] = 0;
			cdvd.Result[7] = 0;
			cdvd.Result[8] = 0;
			cdvd.Result[9] = 0;
			break;

		case 0x24: // sceCdRCBypassCtrl (1:1) - In V10 Bios
			// FIXME: because PRId<0x23, the bit 0 of sio2 don't get updated 0xBF808284
			SetResultSize(1);
			cdvd.Result[0] = 0;
		break;

//		case 0x25: // cdvdman_call120 (1:1) - In V10 Bios
//			break;

//		case 0x26: // cdvdman_call128 (0,3) - In V10 Bios
//			break;

//		case 0x27: // cdvdman_call148 (0:13) - In V10 Bios
//			break;

//		case 0x28: // cdvdman_call150 (1:1) - In V10 Bios
//			break;

		case 0x29: //sceCdNoticeGameStart (1:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

//		case 0x2C: //sceCdXBSPowerCtl (2:2)
//			break;

//		case 0x2D: //sceCdXLEDCtl (2:2)
//			break;

//		case 0x2E: //sceCdBuzzerCtl (0:1)
//			break;

//		case 0x2F: //cdvdman_call167 (16:1)
//			break;

//		case 0x30: //cdvdman_call169 (1:9)
//			break;

		case 0x31: //sceCdSetMediumRemoval (1:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x32: //sceCdGetMediumRemoval (0:2)
			SetResultSize(2);
			cdvd.Result[0] = 0;
			//cdvd.Result[0] = 0; // fixme: I'm pretty sure that the same variable shouldn't be set twice here. Perhaps cdvd.Result[1]?
			break;

//		case 0x33: //sceCdXDVRPReset (1:1)
//			break;

		case 0x36: //cdvdman_call189 [__sceCdReadRegionParams - made up name] (0:15) i think it is 16, not 15
			SetResultSize(15);

			cdvdGetMechaVer(&cdvd.Result[1]);
			cdvd.Result[0] = cdvdReadRegionParams(&cdvd.Result[3]);//size==8
			Console.WriteLn("REGION PARAMS = %s %s", mg_zones[cdvd.Result[1]], &cdvd.Result[3]);
			cdvd.Result[1] = 1 << cdvd.Result[1];	//encryption zone; see offset 0x1C in encrypted headers
			//////////////////////////////////////////
			cdvd.Result[2] = 0;						//??
//			cdvd.Result[3] == ROMVER[4] == *0xBFC7FF04
//			cdvd.Result[4] == OSDVER[4] == CAP			Jjpn, Aeng, Eeng, Heng, Reng, Csch, Kkor?
//			cdvd.Result[5] == OSDVER[5] == small
//			cdvd.Result[6] == OSDVER[6] == small
//			cdvd.Result[7] == OSDVER[7] == small
//			cdvd.Result[8] == VERSTR[0x22] == *0xBFC7FF52
//			cdvd.Result[9] == DVDID						J U O E A R C M
//			cdvd.Result[10]== 0;					//??
			cdvd.Result[11] = 0;					//??
			cdvd.Result[12] = 0;					//??
			//////////////////////////////////////////
			cdvd.Result[13] = 0;					//0xFF - 77001
			cdvd.Result[14] = 0;					//??
			break;

		case 0x37: //called from EECONF [sceCdReadMAC - made up name] (0:9)
			SetResultSize(9);
			cdvd.Result[0] = cdvdReadMAC(&cdvd.Result[1]);
			break;

		case 0x38: //used to fix the MAC back after accidentally trashed it :D [sceCdWriteMAC - made up name] (8:1)
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteMAC(&cdvd.Param[0]);
			break;

		case 0x3E: //[__sceCdWriteRegionParams - made up name] (15:1) [Florin: hum, i was expecting 14:1]
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteRegionParams(&cdvd.Param[2]);
			break;

		case 0x40: // CdOpenConfig (3:1)
			SetResultSize(1);
			cdvd.CReadWrite = cdvd.Param[0];
			cdvd.COffset    = cdvd.Param[1];
			cdvd.CNumBlocks = cdvd.Param[2];
			cdvd.CBlockIndex= 0;
			cdvd.Result[0] = 0;
			break;

		case 0x41: // CdReadConfig (0:16)
			SetResultSize(16);
			cdvdReadConfig(&cdvd.Result[0]);
			break;

		case 0x42: // CdWriteConfig (16:1)
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteConfig(&cdvd.Param[0]);
			break;

		case 0x43: // CdCloseConfig (0:1)
			SetResultSize(1);
			cdvd.CReadWrite = 0;
			cdvd.COffset = 0;
			cdvd.CNumBlocks = 0;
			cdvd.CBlockIndex= 0;
			cdvd.Result[0] = 0;
			break;

		case 0x80: // secrman: __mechacon_auth_0x80
			SetResultSize(1);//in:1
			cdvd.mg_datatype = 0;//data
			cdvd.Result[0] = 0;
			break;

		case 0x81: // secrman: __mechacon_auth_0x81
			SetResultSize(1);//in:1
			cdvd.mg_datatype = 0;//data
			cdvd.Result[0] = 0;
			break;

		case 0x82: // secrman: __mechacon_auth_0x82
			SetResultSize(1);//in:16
			cdvd.Result[0] = 0;
			break;

		case 0x83: // secrman: __mechacon_auth_0x83
			SetResultSize(1);//in:8
			cdvd.Result[0] = 0;
			break;

		case 0x84: // secrman: __mechacon_auth_0x84
			SetResultSize(1+8+4);//in:0
			cdvd.Result[0] = 0;

			cdvd.Result[1] = 0x21;
			cdvd.Result[2] = 0xdc;
			cdvd.Result[3] = 0x31;
			cdvd.Result[4] = 0x96;
			cdvd.Result[5] = 0xce;
			cdvd.Result[6] = 0x72;
			cdvd.Result[7] = 0xe0;
			cdvd.Result[8] = 0xc8;

			cdvd.Result[9]  = 0x69;
			cdvd.Result[10] = 0xda;
			cdvd.Result[11] = 0x34;
			cdvd.Result[12] = 0x9b;
			break;

		case 0x85: // secrman: __mechacon_auth_0x85
			SetResultSize(1+4+8);//in:0
			cdvd.Result[0] = 0;

			cdvd.Result[1] = 0xeb;
			cdvd.Result[2] = 0x01;
			cdvd.Result[3] = 0xc7;
			cdvd.Result[4] = 0xa9;

			cdvd.Result[ 5] = 0x3f;
			cdvd.Result[ 6] = 0x9c;
			cdvd.Result[ 7] = 0x5b;
			cdvd.Result[ 8] = 0x19;
			cdvd.Result[ 9] = 0x31;
			cdvd.Result[10] = 0xa0;
			cdvd.Result[11] = 0xb3;
			cdvd.Result[12] = 0xa3;
			break;

		case 0x86: // secrman: __mechacon_auth_0x86
			SetResultSize(1);//in:16
			cdvd.Result[0] = 0;
			break;

		case 0x87: // secrman: __mechacon_auth_0x87
			SetResultSize(1);//in:8
			cdvd.Result[0] = 0;
			break;

		case 0x8D: // sceMgWriteData
			SetResultSize(1);//in:length<=16
			if (cdvd.mg_size + cdvd.ParamC > cdvd.mg_maxsize)
			{
				cdvd.Result[0] = 0x80;
			}
			else
			{
				memcpy_fast(cdvd.mg_buffer + cdvd.mg_size, cdvd.Param, cdvd.ParamC);
				cdvd.mg_size += cdvd.ParamC;
				cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			}
			break;

		case 0x8E: // sceMgReadData
			SetResultSize( std::min(16, cdvd.mg_size) );
			memcpy_fast(cdvd.Result, cdvd.mg_buffer, cdvd.ResultC);
			cdvd.mg_size -= cdvd.ResultC;
			memcpy_fast(cdvd.mg_buffer, cdvd.mg_buffer+cdvd.ResultC, cdvd.mg_size);
			break;

		case 0x88: // secrman: __mechacon_auth_0x88	//for now it is the same; so, fall;)
		case 0x8F: // secrman: __mechacon_auth_0x8F
			SetResultSize(1);//in:0
			if (cdvd.mg_datatype == 1) // header data
			{
				u64* psrc, *pdst;
				int bit_ofs, i;

				if ((cdvd.mg_maxsize != cdvd.mg_size)||(cdvd.mg_size < 0x20) || (cdvd.mg_size != *(u16*)&cdvd.mg_buffer[0x14]))
				{
					fail_pol_cal();
					break;
				}

				Console.Write("[MG] ELF_size=0x%X Hdr_size=0x%X unk=0x%X flags=0x%X count=%d zones=",
					*(u32*)&cdvd.mg_buffer[0x10], *(u16*)&cdvd.mg_buffer[0x14], *(u16*)&cdvd.mg_buffer[0x16],
					*(u16*)&cdvd.mg_buffer[0x18], *(u16*)&cdvd.mg_buffer[0x1A]);
				for (i=0; i<8; i++)
				{
					if (cdvd.mg_buffer[0x1C] & (1<<i)) Console.Write("%s ", mg_zones[i]);
				}
				Console.Newline();

				bit_ofs = mg_BIToffset(cdvd.mg_buffer);

				psrc = (u64*)&cdvd.mg_buffer[bit_ofs-0x20];

				pdst = (u64*)cdvd.mg_kbit;
				pdst[0] = psrc[0];
				pdst[1] = psrc[1];
				//memcpy(cdvd.mg_kbit, &cdvd.mg_buffer[bit_ofs-0x20], 0x10);

				pdst = (u64*)cdvd.mg_kcon;
				pdst[0] = psrc[2];
				pdst[1] = psrc[3];
				//memcpy(cdvd.mg_kcon, &cdvd.mg_buffer[bit_ofs-0x10], 0x10);

				if ((cdvd.mg_buffer[bit_ofs+5] || cdvd.mg_buffer[bit_ofs+6] || cdvd.mg_buffer[bit_ofs+7]) ||
				     (cdvd.mg_buffer[bit_ofs+4] * 16 + bit_ofs + 8 + 16 != *(u16*)&cdvd.mg_buffer[0x14]))
				{
					fail_pol_cal();
					break;
				}
			}
			cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			break;

		case 0x90: // sceMgWriteHeaderStart
			SetResultSize(1);//in:5
			cdvd.mg_size = 0;
			cdvd.mg_datatype = 1;//header data
			Console.WriteLn("[MG] hcode=%d cnum=%d a2=%d length=0x%X",
				cdvd.Param[0], cdvd.Param[3], cdvd.Param[4], cdvd.mg_maxsize = cdvd.Param[1] | (((int)cdvd.Param[2])<<8));

			cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			break;

		case 0x91: // sceMgReadBITLength
		{
			SetResultSize(3);//in:0
			int bit_ofs = mg_BIToffset(cdvd.mg_buffer);
			memcpy_fast(cdvd.mg_buffer, &cdvd.mg_buffer[bit_ofs], 8+16*cdvd.mg_buffer[bit_ofs+4]);

			cdvd.mg_maxsize = 0; // don't allow any write
			cdvd.mg_size = 8+16*cdvd.mg_buffer[4];//new offset, i just moved the data
			Console.WriteLn("[MG] BIT count=%d", cdvd.mg_buffer[4]);

			cdvd.Result[0] = (cdvd.mg_datatype == 1) ? 0 : 0x80; // 0 complete ; 1 busy ; 0x80 error
			cdvd.Result[1] = (cdvd.mg_size >> 0) & 0xFF;
			cdvd.Result[2] = (cdvd.mg_size >> 8) & 0xFF;
			break;
		}
		case 0x92: // sceMgWriteDatainLength
			SetResultSize(1);//in:2
			cdvd.mg_size = 0;
			cdvd.mg_datatype = 0;//data (encrypted)
			cdvd.mg_maxsize = cdvd.Param[0] | (((int)cdvd.Param[1])<<8);
			cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			break;

		case 0x93: // sceMgWriteDataoutLength
			SetResultSize(1);//in:2
			if (((cdvd.Param[0] | (((int)cdvd.Param[1])<<8)) == cdvd.mg_size) && (cdvd.mg_datatype == 0))
			{
				cdvd.mg_maxsize = 0; // don't allow any write
				cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			}
			else
			{
				cdvd.Result[0] = 0x80;
			}
			break;

		case 0x94: // sceMgReadKbit - read first half of BIT key
			SetResultSize(1+8);//in:0
			cdvd.Result[0] = 0;

			((int*)(cdvd.Result+1))[0] = ((int*)cdvd.mg_kbit)[0];
			((int*)(cdvd.Result+1))[1] = ((int*)cdvd.mg_kbit)[1];
			//memcpy(cdvd.Result+1, cdvd.mg_kbit, 8);
			break;

		case 0x95: // sceMgReadKbit2 - read second half of BIT key
			SetResultSize(1+8);//in:0
			cdvd.Result[0] = 0;
			((int*)(cdvd.Result+1))[0] = ((int*)(cdvd.mg_kbit+8))[0];
			((int*)(cdvd.Result+1))[1] = ((int*)(cdvd.mg_kbit+8))[1];
			//memcpy(cdvd.Result+1, cdvd.mg_kbit+8, 8);
			break;

		case 0x96: // sceMgReadKcon - read first half of content key
			SetResultSize(1+8);//in:0
			cdvd.Result[0] = 0;
			((int*)(cdvd.Result+1))[0] = ((int*)cdvd.mg_kcon)[0];
			((int*)(cdvd.Result+1))[1] = ((int*)cdvd.mg_kcon)[1];
			//memcpy(cdvd.Result+1, cdvd.mg_kcon, 8);
			break;

		case 0x97: // sceMgReadKcon2 - read second half of content key
			SetResultSize(1+8);//in:0
			cdvd.Result[0] = 0;
			((int*)(cdvd.Result+1))[0] = ((int*)(cdvd.mg_kcon+8))[0];
			((int*)(cdvd.Result+1))[1] = ((int*)(cdvd.mg_kcon+8))[1];
			//memcpy(cdvd.Result+1, cdvd.mg_kcon+8, 8);
			break;

		default:
			// fake a 'correct' command
			SetResultSize(1);		//in:0
			cdvd.Result[0] = 0;		// 0 complete ; 1 busy ; 0x80 error
			Console.WriteLn("SCMD Unknown %x", rt);
			break;
	} // end switch

	//Console.WriteLn("SCMD - 0x%x\n", rt);
	cdvd.ParamP = 0;
	cdvd.ParamC = 0;
}

static __forceinline void cdvdWrite17(u8 rt) { // SDATAIN
	CDVD_LOG("cdvdWrite17(SDataIn) %x", rt);

	if (cdvd.ParamP < 32) {
		cdvd.Param[cdvd.ParamP++] = rt;
		cdvd.ParamC++;
	}
}

static __forceinline void cdvdWrite18(u8 rt) { // SDATAOUT
	CDVD_LOG("cdvdWrite18(SDataOut) %x", rt);
	Console.WriteLn("*PCSX2* SDATAOUT");
}

static __forceinline void cdvdWrite3A(u8 rt) { // DEC-SET
	CDVD_LOG("cdvdWrite3A(DecSet) %x", rt);
	cdvd.decSet = rt;
	Console.WriteLn("DecSet Write: %02X", cdvd.decSet);
}

void cdvdWrite(u8 key, u8 rt)
{
	switch (key)
	{
		case 0x04: cdvdWrite04(rt); break;
		case 0x05: cdvdWrite05(rt); break;
		case 0x06: cdvdWrite06(rt); break;
		case 0x07: cdvdWrite07(rt); break;
		case 0x08: cdvdWrite08(rt); break;
		case 0x0A: cdvdWrite0A(rt); break;
		case 0x0F: cdvdWrite0F(rt); break;
		case 0x14: cdvdWrite14(rt); break;
		case 0x16: cdvdWrite16(rt); break;
		case 0x17: cdvdWrite17(rt); break;
		case 0x18: cdvdWrite18(rt); break;
		case 0x3A: cdvdWrite3A(rt); break;
		default:
			Console.Warning("IOP Unknown 8bit write to addr 0x1f4020%x = 0x%x", key, rt);
			break;
	}
}
