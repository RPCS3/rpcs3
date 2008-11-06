 /****************************************************************************\
 * CDVDbin PLUGIN for PS2 emus (PCSX2) based on 
 * CDiso PLUGIN for FPSE
 *
 *
 * AUTHOR: Xobro, mail: _xobro_@tin.it 
 *
 * WHAT'S NEW: It compiles and it seems to work! ;-)
 *
 * TO DO: cue/ccd reading; tracks stuff;
 *        subchannel reading
 *		  registry or ini save of iso/bin name
 *        detection of other types of CD/DVD
 *        proper audio support
 *
 * PORTED BY FLORIN
 *
\****************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <commdlg.h>

#include "CDVD.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////
// Internal variables & defines
/////////////////////////////////////////////////////////////////
#define OpenFile(a) CreateFile(a, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,\
						  NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)

static HANDLE hinstance = (HANDLE)-1;

#define RAW_SECTOR_SIZE     2352
#define DATA_SECTOR_SIZE	2048	// not used at the moment
#define RAWQ_SECTOR_SIZE	(RAW_SECTOR_SIZE+96)
//#define ?_SECTOR_SIZE	(RAW_SECTOR_SIZE+16)

//#define MSF2SECT(m,s,f)	(((m)*60+(s)-2)*75+(f)) 

#define	INT2BCD(a)	((a)/10*16+(a)%10)

static HANDLE hISOFile;

#define MAXFILENAMELENGHT 512
char ISOFileName[MAXFILENAMELENGHT] = {0}, format[MAXFILENAMELENGHT] = {0};

static int  sectorsize,	//0==2048; 1==2352; 2==2448
	offset, cdtype, cdtraystatus, debug=0, forcecdda=0;

static u8  readBuffer[RAWQ_SECTOR_SIZE], *pbuffer, *subqbuffer;
static u32 first, last, crt;
static __int64 unitsize;
static cdvdTN diskInfo={0, 0};
static cdvdTD tracks[100];

s32 msf_to_lba(u8 m, u8 s, u8 f) {
	u32 lsn;
	lsn = f;
	lsn+=(s - 2) * 75;
	lsn+= m * 75 * 60;
	return lsn;
}

void lba_to_msf(s32 lba, u8* m, u8* s, u8* f) {
	lba += 150;
	*m = lba / (60*75);
	*s = (lba / 75) % 60;
	*f = lba % 75;
}

#define btoi(b)		((b)/16*10 + (b)%16)		/* BCD to u_char */
#define itob(i)		((i)/10*16 + (i)%10)		/* u_char to BCD */

/////////////////////////////////////////////////////////////////
// generic plugin stuff
/////////////////////////////////////////////////////////////////
const unsigned char version  = PS2E_CDVD_VERSION;    // CDVD library v2
const unsigned char revision = VERSION;
const unsigned char build    = BUILD;   // increase that with each version

static char *libraryName      = "CDVDbin Driver";

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_CDVD;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return version<<16|revision<<8|build;
}

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "CDVDbin Msg", 0);
}

/////////////////////////////////////////////////////////////////
// CDVDinit: plugin init
/////////////////////////////////////////////////////////////////
s32 CALLBACK CDVDinit() {
	//some defaults
	hISOFile = NULL;
	cdtype=CDVD_TYPE_NODISC;	// no image loaded
	cdtraystatus=CDVD_TRAY_OPEN;	// no image loaded
	sectorsize=0;			// *bad* value
	offset=0;			// no offset in the file
					// till the beginning of the data
	return 0;
}

HANDLE detectMultipleSplits(__int64 *filesize){
	__int64 size;
	HANDLE h;
	int len, filenamelen=strlen(ISOFileName);

	first=last=0;

	for (len=filenamelen; len && ISOFileName[len-1]>='0' && ISOFileName[len-1]<='9'; len--);
	ISOFileName[len]=0;
	len=filenamelen-len;
	if (len){
		sprintf(format, "%s%%0%dd", ISOFileName, len);

		for (first=0; first<2; first++){//check for first if it is 0 or 1
			sprintf(ISOFileName, format, first);
			if ((h = OpenFile(ISOFileName)) != INVALID_HANDLE_VALUE){
				for (last=first; h != INVALID_HANDLE_VALUE; h = OpenFile(ISOFileName)){
					(*(u32*)(&size))=GetFileSize(h, ((u32*)(&size))+1);
					*filesize+=size;
					CloseHandle(h);
					sprintf(ISOFileName, format, ++last);
				}
				last--;
				if (last>first && debug)
					SysMessage("Found multiple files (%d->%d) with format:\n\"%s\"\n"
							   "Total size=%I64d / Unit size=%I64d",
								first, last, format, *filesize, (*filesize-size)/(last-first));
				break;
			}
		}
		if (first == 2) return INVALID_HANDLE_VALUE;
	}else
		strcpy(format, ISOFileName);

	sprintf(ISOFileName, format, crt=first);
	return OpenFile(ISOFileName);
}

int detect(__int64 filesize){
	register struct cdVolDesc *volDesc;
	register u32 sectors;

	if (CDVDreadTrack(16, CDVD_MODE_2048) == -1) {
		SysMessage("file too small");
		return CDVD_TYPE_ILLEGAL;
	}
	volDesc = (struct cdVolDesc *)CDVDgetBuffer();
	if (memcmp(volDesc->volID, "CD001", 5))
		return CDVD_TYPE_DETCT;

	if (memcmp(volDesc->sysIdName, "PLAYSTATION", 11))
		SysMessage("Warrning: Not a Playstation disc?");

	sectors=(u32)(filesize / sectorsize);

	diskInfo.strack  =1;
	diskInfo.etrack  =1;
	memset(tracks, 0, sizeof(cdvdTD) * 100);
	tracks[0].lsn=sectors-offset;
	tracks[0].type  =(volDesc->rootToc.tocSize == DATA_SECTOR_SIZE) ?
			 CDVD_TYPE_PS2CD : CDVD_TYPE_PS2DVD;	//simple & lame detection:p

	tracks[1].lsn=0;
	tracks[1].type  =CDVD_MODE2_TRACK;

	if (debug || (filesize % sectorsize))
	SysMessage("%s (%d bytes/sector - offset=%d) %s %simage detected\n"
				"Datasize: %I64d\n"
				"Sectors: %d LSN:%d\n%s",
				offset ? "NRG" :
				sectorsize==DATA_SECTOR_SIZE ? "ISO" :
				sectorsize==RAW_SECTOR_SIZE ? "RAW" : "RAW+SUBQ",
				sectorsize, offset,
				volDesc->rootToc.tocSize == DATA_SECTOR_SIZE ? "CD" : "DVD",
				last>first ? "splitted " : "",
				filesize, sectors,
				tracks[0].lsn,
				filesize % sectorsize ? "Warrning: Truncated!" : "");

	return tracks[0].type;
}

/////////////////////////////////////////////////////////////////
// CDVDopen: CD hw init
/////////////////////////////////////////////////////////////////
s32 CALLBACK CDVDopen() {
	__int64 filesize=0;

	OPENFILENAME ofn = {sizeof(OPENFILENAME),			// size
						NULL,							// owner
						NULL,							// hInstance
						"CD/DVD images (*.bin;*.iso;*.img;*.bwi;*.mdf;*.nrg)\0*.bin;*.iso;*.img;*.bwi;*.mdf;*.nrg\0"
						"RAW(2352) CD/DVD images (*.bin;*.img;*.bwi;*.mdf)\0*.bin;*.img;*.bwi;*.mdf\0"
						"ISO(2048) CD/DVD images (*.iso)\0*.iso\0"
						"NERO CD/DVD images (*.nrg)\0*.nrg\0"
						"All Files (*.*)\0*.*\0"
														,// filter
						0,								// custom filter
						0,								// max custom filter
						0,								// filter index
						(char *)&ISOFileName,			// file name
						MAXFILENAMELENGHT,				// max filename lenght
						0,								// file title
						0,								// max file title
						NULL,							// initial dir 
						"Select a CD/DVD image...",		// title
						OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,// flags
						0,								// file offset
						0,								// file extension
						0,								// def ext
						0,								// cust data
						0,								// hook
						0};								// template

	//clean any old stuff
	if (hISOFile){
		CloseHandle(hISOFile);
		hISOFile = NULL;
	}

	cdtype=CDVD_TYPE_NODISC;	// no image loaded
	cdtraystatus=CDVD_TRAY_OPEN;	// no image loaded

	if (!ISOFileName[0])
		if(!GetOpenFileName(&ofn))	//Common open file dialog
			return 0;	
			//no image/dusc loaded => null plugin

	//detect & open the selected file (or first if more than 1)
	hISOFile = detectMultipleSplits(&filesize);

	cdtraystatus=CDVD_TRAY_CLOSE;
	cdtype=CDVD_TYPE_UNKNOWN;

	if (hISOFile == INVALID_HANDLE_VALUE){
		SysMessage("error opening the file\n");
		cdtype=CDVD_TYPE_ILLEGAL;
		return 0;
	}

	(*(u32*)(&unitsize))=GetFileSize(hISOFile, ((u32*)(&unitsize))+1);
	if (!filesize)	filesize=unitsize;//in case of non-splitted images

	sectorsize=DATA_SECTOR_SIZE;	offset=0;	//ISO 2048 detection
	cdtype=detect(filesize);
	if (cdtype!=CDVD_TYPE_DETCT)	return 0;

	sectorsize=RAW_SECTOR_SIZE;	offset=0;	//RAW 2352 detection
	cdtype=detect(filesize);
	if (cdtype!=CDVD_TYPE_DETCT)	return 0;

	sectorsize=RAWQ_SECTOR_SIZE;	offset=0;	//RAWQ 2448 detection
	cdtype=detect(filesize);
	if (cdtype!=CDVD_TYPE_DETCT)	return 0;

	filesize-=156;//ending junk

	sectorsize=DATA_SECTOR_SIZE;	offset=150;	//NERO ISO 2048 detection
	cdtype=detect(filesize);
	if (cdtype!=CDVD_TYPE_DETCT)	return 0;

	sectorsize=RAW_SECTOR_SIZE;	offset=150;	//NERO RAW 2352 detection
	cdtype=detect(filesize);
	if (cdtype!=CDVD_TYPE_DETCT)	return 0;

	sectorsize=RAWQ_SECTOR_SIZE;	offset=150;	//NERO RAWQ 2448 detection
	cdtype=detect(filesize);
	if (cdtype!=CDVD_TYPE_DETCT)	return 0;

	if (forcecdda && (filesize>RAW_SECTOR_SIZE)){
		u32 sectors=(u32)(filesize/(sectorsize=RAW_SECTOR_SIZE));
		offset=0;	//FIXME: offset for nero images

		memset(tracks, 0, sizeof(cdvdTD) * 100);
		diskInfo.strack  =1;
		diskInfo.etrack  =1;
		tracks[0].lsn=sectors-offset;
		tracks[0].type  =cdtype=CDVD_TYPE_CDDA;//DVDV;

		tracks[1].lsn=0;
		tracks[1].type  =CDVD_AUDIO_TRACK;

		if (debug)
		SysMessage("CDDA (%d bytes/sector - offset=%d) image\n"
				"Datasize: %I64d\n"
				"Sectors: %d LSN:%d\n",
				sectorsize, offset,
				filesize, sectors,
				tracks[0].lsn);
		return 0;
	}

	//nothing detected
	SysMessage("The file you picked is not a usable cd/dvd image\n");
	CloseHandle(hISOFile);
	hISOFile=NULL;
	cdtraystatus=CDVD_TRAY_OPEN;
	cdtype=CDVD_TYPE_NODISC;
	return 0;
}

/////////////////////////////////////////////////////////////////
// CDVDclose: CD shutdown
/////////////////////////////////////////////////////////////////
void CALLBACK CDVDclose() {
	CloseHandle(hISOFile);
	CDVDinit();
}

/////////////////////////////////////////////////////////////////
// CDVDshutdown: plugin shutdown
/////////////////////////////////////////////////////////////////
void CALLBACK CDVDshutdown() {
}

/////////////////////////////////////////////////////////////////
// CDVDreadTrack: read 1(one) sector/frame (2352 bytes)
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDreadTrack(u32 lsn, int mode) {
	u32 nbytesRead, chunk;
	s32 Ret;
	__int64 filepos;

	pbuffer = readBuffer;
	subqbuffer = readBuffer+RAW_SECTOR_SIZE;

	memset(readBuffer, 0, RAWQ_SECTOR_SIZE);
	
	//if no iso selected act as a null CD plugin
	if (hISOFile == NULL)
		return 0;

	// Set the file pointer at the right sector
	filepos=(__int64)(lsn+offset)*sectorsize;
	chunk=(u32)(filepos / unitsize);
	filepos-=chunk * unitsize;
	chunk+=first;
	if (chunk!=crt){
		crt=chunk;
		CloseHandle(hISOFile);
		sprintf(ISOFileName, format, crt);
		hISOFile=OpenFile(ISOFileName);
	}
	SetFilePointer (hISOFile,
                    *(u32*)(&filepos),
                    ((u32*)(&filepos))+1,
                    FILE_BEGIN);
	
	Ret = ReadFile(hISOFile, 
		readBuffer+(sectorsize==DATA_SECTOR_SIZE?12+12:0),
		sectorsize, &nbytesRead, NULL);

	if (Ret && nbytesRead < (u32)sectorsize && crt<last){
		crt++;
		CloseHandle(hISOFile);
		sprintf(ISOFileName, format, crt);
		hISOFile=OpenFile(ISOFileName);

		Ret = ReadFile(hISOFile, 
			readBuffer+(sectorsize==DATA_SECTOR_SIZE?12+12:0)+nbytesRead,
			sectorsize-nbytesRead, &nbytesRead, NULL);
			//subchannel buffer is at readBuffer+2352 ;)
	}

	if (!Ret) SysMessage("Error = %08x\nbytesRead=%d",GetLastError(), nbytesRead);

	// put buffer pointer at the correct data start point
	switch (mode) {
		case CDVD_MODE_2368://not realy supported atm
		case CDVD_MODE_2352: break;
		case CDVD_MODE_2340: pbuffer+= 12; break;
		case CDVD_MODE_2328: pbuffer+= 24; break;
		case CDVD_MODE_2048: pbuffer+= 24; break;
	}
	
	return 0;
}

/////////////////////////////////////////////////////////////////
// CDVDgetBuffer: address of sector/frame buffer
/////////////////////////////////////////////////////////////////
// return can be NULL (for async modes); no async mode for now...
u8*  CALLBACK CDVDgetBuffer() {
	return pbuffer;
}

/////////////////////////////////////////////////////////////////
// CDVDreadSubQ: read subq data
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ* subq) {
	// fake it
	u8 min, sec, frm;
	subq->ctrl		= 4;
	subq->mode		= 1;
	subq->trackNum	= itob(1);
	subq->trackIndex= itob(1);
	
	lba_to_msf(lsn, &min, &sec, &frm);
	subq->trackM	= itob(min);
	subq->trackS	= itob(sec);
	subq->trackF	= itob(frm);
	
	subq->pad		= 0;
	
	lba_to_msf(lsn + (2*75), &min, &sec, &frm);
	subq->discM		= itob(min);
	subq->discS		= itob(sec);
	subq->discF		= itob(frm);
	return 0;
}

/////////////////////////////////////////////////////////////////
// CDVDgetTN: tracks number
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDgetTN(cdvdTN *Buffer) {
	//if no iso selected act as a null CD plugin
	if (hISOFile == NULL){
		Buffer->strack = 1;
		Buffer->etrack = 1;
		return 0;
	}

	//should be read from .cue
	memcpy(Buffer, &diskInfo, sizeof(cdvdTN));
	return 0;
}

/////////////////////////////////////////////////////////////////
// CDVDgetTD: track start end addresses
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDgetTD(u8 Track, cdvdTD *Buffer) {
	//if no iso selected act as a null CD plugin
	if (hISOFile == NULL)
		return -1;

	if (Track>diskInfo.etrack)
		return -1;

	memcpy(Buffer, &tracks[Track], sizeof(cdvdTD));
	return 0;
}

/////////////////////////////////////////////////////////////////
// CDVDgetTOC: get comlpete toc
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDgetTOC(void* toc) {
	u8 type = CDVDgetDiskType();
	u8* tocBuff = (u8*)toc;
	
	if(	type == CDVD_TYPE_DVDV ||
		type == CDVD_TYPE_PS2DVD)
	{
		// get dvd structure format
		// scsi command 0x43
		memset(tocBuff, 0, 2048);
		// fake it
		tocBuff[ 0] = 0x04;
		tocBuff[ 1] = 0x02;
		tocBuff[ 2] = 0xF2;
		tocBuff[ 3] = 0x00;
		tocBuff[ 4] = 0x86;
		tocBuff[ 5] = 0x72;

		tocBuff[16] = 0x00;
		tocBuff[17] = 0x03;
		tocBuff[18] = 0x00;
		tocBuff[19] = 0x00;
	}
	else if(type == CDVD_TYPE_CDDA ||
			type == CDVD_TYPE_PS2CDDA ||
			type == CDVD_TYPE_PS2CD ||
			type == CDVD_TYPE_PSCDDA ||
			type == CDVD_TYPE_PSCD)
	{
		// cd toc
		// (could be replaced by 1 command that reads the full toc)
		u8 min, sec, frm;
		s32 i, err;
		cdvdTN diskInfo;
		cdvdTD trackInfo;
		memset(tocBuff, 0, 1024);
		if (CDVDgetTN(&diskInfo) == -1)	{ diskInfo.etrack = 0;diskInfo.strack = 1; }
		if (CDVDgetTD(0, &trackInfo) == -1) trackInfo.lsn = 0;
		
		tocBuff[0] = 0x41;
		tocBuff[1] = 0x00;
		
		//Number of FirstTrack
		tocBuff[2] = 0xA0;
		tocBuff[7] = itob(diskInfo.strack);
		
		//Number of LastTrack
		tocBuff[12] = 0xA1;
		tocBuff[17] = itob(diskInfo.etrack);

		//DiskLength
		lba_to_msf(trackInfo.lsn, &min, &sec, &frm);
		tocBuff[22] = 0xA2;
		tocBuff[27] = itob(min);
		tocBuff[28] = itob(sec);
		
		for (i=diskInfo.strack; i<=diskInfo.etrack; i++)
		{
			err = CDVDgetTD(i, &trackInfo);
			lba_to_msf(trackInfo.lsn, &min, &sec, &frm);
			tocBuff[i*10+30] = trackInfo.type;
			tocBuff[i*10+32] = err == -1 ? 0 : itob(i);	  //number
			tocBuff[i*10+37] = itob(min);
			tocBuff[i*10+38] = itob(sec);
			tocBuff[i*10+39] = itob(frm);
		}
	}
	else
		return -1;
	
	return 0;
}

/////////////////////////////////////////////////////////////////
// CDVDgetDiskType: type of the disc: CD/DVD
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDgetDiskType() {
	if (debug)	SysMessage("CDVDgetType() == %02X", cdtype);
	return cdtype;
}

/////////////////////////////////////////////////////////////////
// CDVDgetTrayStatus: the status of the tray
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDgetTrayStatus() {
	if (debug)	SysMessage("CDVDgetTrayStatus() == %02X", cdtraystatus);
	return cdtraystatus;
}

/////////////////////////////////////////////////////////////////
// CDVDctrlTrayOpen: open the disc tray
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDctrlTrayOpen() {
	return 0;
}

/////////////////////////////////////////////////////////////////
// CDVDctrlTrayClose: close the disc tray
/////////////////////////////////////////////////////////////////
s32  CALLBACK CDVDctrlTrayClose() {
	return 0;
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	OPENFILENAME ofn = {sizeof(OPENFILENAME),			// size
						NULL,							// owner
						NULL,							// hInstance
						"CD/DVD images (*.bin;*.iso;*.img;*.bwi;*.mdf;*.nrg)\0*.bin;*.iso;*.img;*.bwi;*.mdf;*.nrg\0"
						"RAW(2352) CD/DVD images (*.bin;*.img;*.bwi;*.mdf)\0*.bin;*.img;*.bwi;*.mdf\0"
						"ISO(2048) CD/DVD images (*.iso)\0*.iso\0"
						"NERO CD/DVD images (*.nrg)\0*.nrg\0"
						"All Files (*.*)\0*.*\0"
														,// filter
						0,								// custom filter
						0,								// max custom filter
						0,								// filter index
						(char *)&ISOFileName,			// file name
						MAXFILENAMELENGHT,				// max filename lenght
						0,								// file title
						0,								// max file title
						NULL,							// initial dir 
						"Select a CD/DVD image...",// title
						OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,// flags
						0,								// file offset
						0,								// file extension
						0,								// def ext
						0,								// cust data
						0,								// hook
						0};								// template

	switch(uMsg) {
		case WM_INITDIALOG:
			CheckDlgButton(hW, IDC_DEBUG, debug ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hW, IDC_CDDA, forcecdda ? BST_CHECKED : BST_UNCHECKED);
			if (ISOFileName && *ISOFileName){
				SetDlgItemText(hW, IDC_FILE, ISOFileName);
				CheckDlgButton(hW, IDC_FILE, BST_CHECKED);
			}else
				CheckDlgButton(hW, IDC_FILE, BST_UNCHECKED);
			return TRUE;

		case WM_COMMAND:
			switch(HIWORD(wParam)){
			case BN_CLICKED:
				switch(LOWORD(wParam)) {
				case IDC_FILE:
					if (!IsDlgButtonChecked(hW, IDC_FILE)){
						CheckDlgButton(hW, IDC_FILE, BST_UNCHECKED);
						ISOFileName[0]='\0';
						SetDlgItemText(hW, IDC_FILE, "Choose a CD/DVD bin or iso image...");
					}else{
						if (GetOpenFileName(&ofn)){
							CheckDlgButton(hW, IDC_FILE, BST_CHECKED);
							SetDlgItemText(hW, IDC_FILE, ISOFileName);
						}else
							CheckDlgButton(hW, IDC_FILE, BST_UNCHECKED);
					}
					return TRUE;

				case IDOK:
					debug=IsDlgButtonChecked(hW, IDC_DEBUG);
					forcecdda=IsDlgButtonChecked(hW, IDC_CDDA);
					EndDialog(hW, FALSE);
					return TRUE;
				}
			}
			
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////
// CDVDconfigure: config dialog, if any...
/////////////////////////////////////////////////////////////////
void CALLBACK CDVDconfigure(){
	DialogBox(hinstance,
		MAKEINTRESOURCE(IDD_CONFIG),
		GetActiveWindow(),
		(DLGPROC)ConfigureDlgProc);
}

/////////////////////////////////////////////////////////////////
// CDVDabout: about dialog, if any...
/////////////////////////////////////////////////////////////////
void CALLBACK CDVDabout() {
	SysMessage("%s %d.%d\n"
		"PS1 version by: Xobro,  mail: _xobro_@tin.it\n"
		"PS2 version by: Florin, mail: florin@ngemu.com", libraryName, revision, build);
}

/////////////////////////////////////////////////////////////////
// CDVDtest: test hw and report if it works
/////////////////////////////////////////////////////////////////
s32 CALLBACK CDVDtest() {
	FILE *f;

	if (cdtype==CDVD_TYPE_ILLEGAL)
		return -1;

	if (*ISOFileName == 0)
		return 0;

	f = fopen(ISOFileName, "rb");
	if (f == NULL)
		return -1;
	fclose(f);

	return 0;	//if (sectorsize==2048) return 40;//PSE_CDR_WARN_LAMECD;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved){
        hinstance = (HINSTANCE)hModule;
        return TRUE;
}
