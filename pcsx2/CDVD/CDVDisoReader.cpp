#include "PrecompiledHeader.h"

#include "CDVDisoReader.h"

#ifndef MAX_PATH
#define MAX_PATH 255
#endif

char IsoFile[256];

u8 *pbuffer;

int BlockDump;
FILE* fdump;
FILE* isoFile;

FILE *cdvdLog = NULL;

int isoType;
int isoNumAudioTracks = 0;
int isoNumSectors = 0;
int isoSectorSize = 0;
int isoSectorOffset = 0;

#ifndef _WIN32
// if this doesn't work in linux, sorry
#define _ftelli64 ftell64
#define _fseeki64 fseek64
#endif

// This var is used to detect resume-style behavior of the Pcsx2 emulator,
// and skip prompting the user for a new CD when it's likely they want to run the existing one.
static char cdvdCurrentIso[MAX_PATH];

u8 cdbuffer[CD_FRAMESIZE_RAW * 10] = {0};

s32 msf_to_lba(u8 m, u8 s, u8 f)
{
	u32 lsn;
	lsn = f;
	lsn += (s - 2) * 75;
	lsn += m * 75 * 60;
	return lsn;
}

void lba_to_msf(s32 lba, u8* m, u8* s, u8* f)
{
	lba += 150;
	*m = lba / (60 * 75);
	*s = (lba / 75) % 60;
	*f = lba % 75;
}

#define btoi(b)	((b)/16*10 + (b)%16)	/* BCD to u_char */
#define itob(i)		((i)/10*16 + (i)%10)		/* u_char to BCD */

#ifdef PCSX2_DEBUG
void __Log(char *fmt, ...)
{
	va_list list;

	if (cdvdLog == NULL) return;

	va_start(list, fmt);
	vfprintf(cdvdLog, fmt, list);
	va_end(list);
}
#define CDVD_LOG __Log
#else
#define __Log 0&&
#endif


s32 ISOinit()
{
#ifdef PCSX2_DEBUG
	cdvdLog = fopen("logs/cdvdLog.txt", "w");
	if (cdvdLog == NULL)
	{
		cdvdLog = fopen("cdvdLog.txt", "w");
		if (cdvdLog == NULL)
		{
			Console::Error("Can't create cdvdLog.txt");
			return -1;
		}
	}
	setvbuf(cdvdLog, NULL,  _IONBF, 0);
	CDVD_LOG("ISOinit\n");
#endif

	cdvdCurrentIso[0] = 0;
	return 0;
}

void ISOshutdown()
{
	cdvdCurrentIso[0] = 0;
#ifdef CDVD_LOG
	if (cdvdLog != NULL) fclose(cdvdLog);
#endif
}

s32 ISOopen(const char* pTitle)
{
	if (pTitle != NULL) strcpy(IsoFile, pTitle);

	if (*IsoFile == 0) strcpy(IsoFile, cdvdCurrentIso);

	if (*IsoFile == 0)
	{
		char temp[256];

		//CfgOpenFile();
		
		strcpy(temp, IsoFile);
		*IsoFile = 0;
		strcpy(IsoFile, temp);
	}

	isoFile = fopen(IsoFile,"rb");
	if (isoFile == NULL)
	{
		Console::Error("Error loading %s\n", params IsoFile);
		return -1;
	}

	// pretend it's a ps2 dvd... for now
	isoType=CDVD_TYPE_PS2DVD;

	isoSectorSize = 2048;
	isoSectorOffset = 0;

	// probably vc++ only, CBA to find the unix equivalent
	isoNumSectors = (int)(_ftelli64(isoFile)/isoSectorSize);

	if (BlockDump)
	{
		char fname_only[MAX_PATH];

#ifdef _WIN32
		char fname[MAX_PATH], ext[MAX_PATH];
		_splitpath(IsoFile, NULL, NULL, fname, ext);
		_makepath(fname_only, NULL, NULL, fname, NULL);
#else
		char* p, *plast;
		
		plast = p = strchr(IsoFile, '/');
		while (p != NULL)
		{
			plast = p;
			p = strchr(p + 1, '/');
		}

		// Lets not create dumps in the plugin directory.
		strcpy(fname_only, "../");
		if (plast != NULL) 
			strcat(fname_only, plast + 1);
		else 
			strcat(fname_only, IsoFile);
	
		plast = p = strchr(fname_only, '.');
		
		while (p != NULL)
		{
			plast = p;
			p = strchr(p + 1, '.');
		}

		if (plast != NULL) *plast = 0;

#endif
		strcat(fname_only, ".dump");
		fdump = fopen(fname_only, "wb");
		if(fdump)
		{
			if(isoNumAudioTracks)
			{
				int k;
				fwrite("BDV2",4,1,fdump);
				k=2352;
				fwrite(&k,4,1,fdump);
				k=isoNumSectors;
				fwrite(&k,4,1,fdump);
				k=0;
				fwrite(&k,4,1,fdump);
			}
			else
			{
				int k;
				fwrite("BDV2",4,1,fdump);
				k=2048;
				fwrite(&k,4,1,fdump);
				k=isoNumSectors;
				fwrite(&k,4,1,fdump);
				k=0x18;
				fwrite(&k,4,1,fdump);
			}
		}
	}
	else
	{
		fdump = NULL;
	}

	return 0;
}

void ISOclose()
{
	strcpy(cdvdCurrentIso, IsoFile);

	fclose(isoFile);
	if (fdump != NULL) fclose(fdump);
}

s32 ISOreadSubQ(u32 lsn, cdvdSubQ* subq)
{
	// fake it, until some kind of support for clonecd .sub files is implemented
	u8 min, sec, frm;
	subq->ctrl		= 4;
	subq->mode		= 1;
	subq->trackNum	= itob(1);
	subq->trackIndex = itob(1);

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

s32 ISOgetTN(cdvdTN *Buffer)
{
	Buffer->strack = 1;
	Buffer->etrack = 1;

	return 0;
}

s32 ISOgetTD(int tn, cdvdTD *Buffer)
{
	if(tn==1)
	{
		Buffer->lsn = 0;
		Buffer->type = CDVD_MODE1_TRACK;
	}
	else
	{
		Buffer->lsn = isoNumSectors;
		Buffer->type = 0;
	}
	return 0;
}

s32 ISOgetDiskType()
{
	return isoType;
}

s32 ISOgetTrayStatus()
{
	return CDVD_TRAY_CLOSE;
}

s32 ISOctrlTrayOpen()
{
	return 0;
}
s32 ISOctrlTrayClose()
{
	return 0;
}

s32 ISOreadSector(u8* tempbuffer, u32 lsn)
{
	// dummy function, doesn't create valid info for the data surrounding the userdata bytes!

	// probably vc++ only, CBA to figure out the unix equivalent of fseek with support for >2gb seeking
	_fseeki64(isoFile, lsn * (s64)isoSectorSize, SEEK_SET);
	return fread(tempbuffer+24, isoSectorSize, 1, isoFile)-1;
}

static s32 layer1start = -1;
s32 ISOgetTOC(void* toc)
{
	u8 type = ISOgetDiskType();
	u8* tocBuff = (u8*)toc;

	//__Log("ISOgetTOC\n");

	if (type == CDVD_TYPE_DVDV || type == CDVD_TYPE_PS2DVD)
	{
		// get dvd structure format
		// scsi command 0x43
		memset(tocBuff, 0, 2048);

		if (layer1start != -2 && isoNumSectors >= 0x300000)
		{
			int off = isoSectorOffset;
			u8* tempbuffer;

			// dual sided
			tocBuff[ 0] = 0x24;
			tocBuff[ 1] = 0x02;
			tocBuff[ 2] = 0xF2;
			tocBuff[ 3] = 0x00;
			tocBuff[ 4] = 0x41;
			tocBuff[ 5] = 0x95;

			tocBuff[14] = 0x60; // dual sided, ptp

			tocBuff[16] = 0x00;
			tocBuff[17] = 0x03;
			tocBuff[18] = 0x00;
			tocBuff[19] = 0x00;

			// search for it
			if (layer1start == -1)
			{
				printf("CDVD: searching for layer1...");
				tempbuffer = (u8*)malloc(CD_FRAMESIZE_RAW * 10);
				for (layer1start = (isoNumSectors / 2 - 0x10) & ~0xf; layer1start < 0x200010; layer1start += 16)
				{
					ISOreadSector(tempbuffer, layer1start);
					// CD001
					if (tempbuffer[off+1] == 0x43 && tempbuffer[off+2] == 0x44 && tempbuffer[off+3] == 0x30 && tempbuffer[off+4] == 0x30 && tempbuffer[off+5] == 0x31)
						break;
				}
				free(tempbuffer);

				if (layer1start == 0x200010)
				{
					printf("Couldn't find second layer on dual layer... ignoring\n");
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
					layer1start = -2;
					return 0;
				}

				printf("found at 0x%8.8x\n", layer1start);
				layer1start = layer1start + 0x30000 - 1;
			}

			tocBuff[20] = layer1start >> 24;
			tocBuff[21] = (layer1start >> 16) & 0xff;
			tocBuff[22] = (layer1start >> 8) & 0xff;
			tocBuff[23] = (layer1start >> 0) & 0xff;
		}
		else
		{
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
	}
	else if ((type == CDVD_TYPE_CDDA) || (type == CDVD_TYPE_PS2CDDA) ||
	             (type == CDVD_TYPE_PS2CD) || (type == CDVD_TYPE_PSCDDA) || (type == CDVD_TYPE_PSCD))
	{
		// cd toc
		// (could be replaced by 1 command that reads the full toc)
		u8 min, sec, frm;
		s32 i, err;
		cdvdTN diskInfo;
		cdvdTD trackInfo;
		memset(tocBuff, 0, 1024);
		if (ISOgetTN(&diskInfo) == -1)
		{
			diskInfo.etrack = 0;
			diskInfo.strack = 1;
		}
		if (ISOgetTD(0, &trackInfo) == -1) trackInfo.lsn = 0;

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

		for (i = diskInfo.strack; i <= diskInfo.etrack; i++)
		{
			err = ISOgetTD(i, &trackInfo);
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

s32 ISOreadTrack(u32 lsn, int mode)
{
	int _lsn = lsn;

	//__Log("ISOreadTrack: %x %x\n", lsn, mode);
	if (_lsn < 0)
	{
//		lsn = 2097152 + (-_lsn);
		lsn = isoNumSectors - (-_lsn);
	}
//	printf ("CDRreadTrack %d\n", lsn);

	ISOreadSector(cdbuffer, lsn);
	if (fdump != NULL)
	{
		fwrite(&lsn,4,1,fdump);
		fwrite(cdbuffer,isoSectorSize,1,fdump);
	}

	pbuffer = cdbuffer;
	switch (mode)
	{
		case CDVD_MODE_2352:
			break;
		case CDVD_MODE_2340:
			pbuffer += 12;
			break;
		case CDVD_MODE_2328:
			pbuffer += 24;
			break;
		case CDVD_MODE_2048:
			pbuffer += 24;
			break;
	}

	return 0;
}

u8* ISOgetBuffer()
{
	return pbuffer;
}
