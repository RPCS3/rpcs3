#include "CDVD.h"

#define TLogN(msg)           printf(msg);          sprintf(tlptr,msg); tlptr=tlptr+strlen(tlptr)
#define TLogN1(msg,p1)       printf(msg,p1);       sprintf(tlptr,msg,p1); tlptr=tlptr+strlen(tlptr)
#define TLogN3(msg,p1,p2,p3) printf(msg,p1,p2,p3); sprintf(tlptr,msg,p1,p2,p3); tlptr=tlptr+strlen(tlptr)
#define TLog(msg)           printf(" * CDVD Test: " msg);          sprintf(tlptr,msg); tlptr=tlptr+strlen(tlptr)
#define TLog1(msg,p1)       printf(" * CDVD Test: " msg,p1);       sprintf(tlptr,msg,p1); tlptr=tlptr+strlen(tlptr)
#define TLog3(msg,p1,p2,p3) printf(" * CDVD Test: " msg,p1,p2,p3); sprintf(tlptr,msg,p1,p2,p3); tlptr=tlptr+strlen(tlptr)
#define TReturn(k)   ret=k;break

s32 testReadSector(u32 snum, bool raw)
{
	if(CDVDreadTrack(snum,raw?CDVD_MODE_2352:CDVD_MODE_2048)<0)
	{
		printf(" * CDVD Test: Reading sector %d in %s mode...",snum,raw?"raw":"user");
		printf("failed.\n");
		return -1;
	}
	u8*bfr = CDVDgetBuffer();

	if(!bfr)
	{
		printf(" * CDVD Test: Reading sector %d in %s mode...",snum,raw?"raw":"user");
		printf("failed.\n");
		return -1;
	}

	//printf("ok.\n");

	return 0;
}

s32 CALLBACK CDVDtest() 
{
	char *buffer = (char*)malloc(1024*1024);
	char *tlptr  = buffer;

	int ret;

	for(;;)
	{
		ReadSettings();

		if(source_drive=='@')
		{
			TLog("WARNING: Plugin configured to use no disc. Nothing to test.");
			TReturn(0);
		}

		sprintf(csrc,"\\\\.\\%c:",source_drive);

		TLog1("1. Opening drive '%s'... ",csrc);

		//open device file
		src=new IOCtlSrc(csrc);
		if(!src)
		{
			TLogN(" failed!\n");
			TReturn(-1);
		}

		if(!src->IsOK())
		{
			TLogN(" failed!\n");
			TReturn(-1);
		}
		TLogN("ok.\n");

		TLog("2. Starting read thread... ");
		//setup threading manager
		if(cdvdStartThread()<0)
		{
			TLogN(" failed!\n");
			TReturn(-1);
		}
		TLogN("ok.\n");

		TLog("3. Checking Sizes...\n");
		TLog(" * Checking Sector Count... ");
		s32 sectorcount = src->GetSectorCount();

		if(sectorcount<0)
		{
			TLogN("error. Is there a disc in the drive?\n");
			TReturn(-1);
		}
		else if(sectorcount==0)
		{
			TLogN("0 sectors. Bad disc?\n");
			TReturn(-1);
		}
		else
		{
			TLogN1("%d sectors.\n",sectorcount);
		}

		TLog(" * Checking Layer Break Address... ");
	    s32 lastaddr = src->GetLayerBreakAddress();

		if(lastaddr<0)
		{
			TLogN("error. Probably not a DVD. Assuming CDROM....\n");
		}
		else if(lastaddr==0)
		{
			TLogN("0. Single layer.\n");
		}
		else
		{
			TLogN1("%d. Double layer disc.\n",lastaddr);
		}

		TLog("4. Attempting to read disc TOC... ");
		if(lastaddr<0)
		{
			if(src->ReadTOC((char*)&cdtoc,sizeof(cdtoc))<0)
			{
				TLogN("failed!\n");
				TReturn(-1);
			}
			TLogN("ok.\n");

			TLog(" * Reading TOC descriptors...\n");
			for(int i=0;i<101;i++)
			{
				if(cdtoc.Descriptors[i].Point!=0)
				{
					TLog1(" * Descriptor %d: ",i);
					TLogN1("Session %d, ",cdtoc.Descriptors[i].SessionNumber);
					switch(cdtoc.Descriptors[i].Point)
					{
					case 0xa0:
						TLogN1("0xA0: First track = %d.\n",cdtoc.Descriptors[i].Msf[0]);
						break;
					case 0xa1:
						TLogN1("0xA1: Last track = %d.\n",cdtoc.Descriptors[i].Msf[0]);
						if(cdtoc.Descriptors[i].SessionNumber==cdtoc.LastCompleteSession)
						{
							etrack = cdtoc.Descriptors[i].Msf[0];
						}
						break;
					case 0xa2: // session size
						{
							int min,sec,frm,lba;

							min=cdtoc.Descriptors[i].Msf[0];
							sec=cdtoc.Descriptors[i].Msf[1];
							frm=cdtoc.Descriptors[i].Msf[2];
							lba=MSF_TO_LBA(min,sec,frm);

							TLogN3("0xA2: Start of lead-out track: msf(%d,%d,%d)",min,sec,frm);
							TLogN1("=lba(%d).\n",lba);
						}
						break;
					case 0xb0:
						{
							int min,sec,frm,lba;

							min=cdtoc.Descriptors[i].Msf[0];
							sec=cdtoc.Descriptors[i].Msf[1];
							frm=cdtoc.Descriptors[i].Msf[2];
							lba=MSF_TO_LBA(min,sec,frm);

							TLogN3("0xB0: Leadout start? msf(%d,%d,%d)",min,sec,frm);
							TLogN1("=lba(%d).\n",lba);
						}
						break;
					case 0xc0:
						{
							int min,sec,frm,lba;

							min=cdtoc.Descriptors[i].Msf[0];
							sec=cdtoc.Descriptors[i].Msf[1];
							frm=cdtoc.Descriptors[i].Msf[2];
							lba=MSF_TO_LBA(min,sec,frm);

							TLogN3("0xC0: Leadout end? msf(%d,%d,%d)",min,sec,frm);
							TLogN1("=lba(%d).\n",lba);
						}
						break;
					default:
						if((cdtoc.Descriptors[i].Point<101)&&(cdtoc.Descriptors[i].Point>0))
						{
							int min,sec,frm,lba;
							int tn=cdtoc.Descriptors[i].Point;

							min=cdtoc.Descriptors[i].Msf[0];
							sec=cdtoc.Descriptors[i].Msf[1];
							frm=cdtoc.Descriptors[i].Msf[2];
							lba = MSF_TO_LBA(min,sec,frm);

							TLogN1("%d: Track start:",tn);
							TLogN3(" msf(%d,%d,%d)",min,sec,frm);
							TLogN3("=lba(%d), adr=%x control=%x.\n",lba,cdtoc.Descriptors[i].Adr,cdtoc.Descriptors[i].Control);


						}
						else if(cdtoc.Descriptors[i].Point>0)
						{
							int min,sec,frm,lba;
							int tn=cdtoc.Descriptors[i].Point;

							min=cdtoc.Descriptors[i].Msf[0];
							sec=cdtoc.Descriptors[i].Msf[1];
							frm=cdtoc.Descriptors[i].Msf[2];
							lba = MSF_TO_LBA(min,sec,frm);

							TLogN1("0x%02x: Unknon descriptor point code:",tn);
							TLogN3(" msf(%d,%d,%d)",min,sec,frm);
							TLogN1("=lba(%d).\n",lba);


						}
						break;
					}
				}
			}
		}
		else
		{
			TLogN("skipped (not a cdrom).\n");
		}

		TLog("6. Attempting to detect disc type... ");

		cdvdParseTOC();
		curDiskType = FindDiskType();

		if(curDiskType<0)
		{
			TLogN("failed!\n");
			TReturn(-1);
		}

		char *diskTypeName="Unknown";

		switch(curDiskType)
		{
			case CDVD_TYPE_ILLEGAL:   diskTypeName="Illegal Disc"; break;
			case CDVD_TYPE_DVDV:      diskTypeName="DVD-Video"; break;
			case CDVD_TYPE_CDDA:      diskTypeName="CD-Audio"; break;
			case CDVD_TYPE_PS2DVD:    diskTypeName="PS2 DVD"; break;
			case CDVD_TYPE_PS2CDDA:   diskTypeName="PS2 CD+Audio"; break;
			case CDVD_TYPE_PS2CD:     diskTypeName="PS2 CD"; break;
			case CDVD_TYPE_PSCDDA:    diskTypeName="PS1 CD+Audio"; break;
			case CDVD_TYPE_PSCD:      diskTypeName="PS1 CD"; break;
			case CDVD_TYPE_UNKNOWN:   diskTypeName="Unknown"; break;
			case CDVD_TYPE_DETCTDVDD: diskTypeName="Detecting (Single-Layer DVD)"; break;
			case CDVD_TYPE_DETCTDVDS: diskTypeName="Detecting (Double-Layer DVD)"; break;
			case CDVD_TYPE_DETCTCD:   diskTypeName="Detecting (CD)"; break;
			case CDVD_TYPE_DETCT:     diskTypeName="Detecting"; break;
			case CDVD_TYPE_NODISC:    diskTypeName="No Disc"; break;
		}

		TLogN1(" %s.\n",diskTypeName);

		TLog("7. Testing USER sector reading... ");

		//force a spinup
		testReadSector(16,true);

		DWORD ticksStart = GetTickCount();

		for(int i=16;i<2016;i++)
		{
			if(testReadSector(i,false)<0)
			{
				TLogN1("%d failed!\n",i);
				TReturn(-1);
			}
		}
		DWORD ticksEnd = GetTickCount();
		float speed =(2000.0f*2048.0f*1000.0f)/(ticksEnd-ticksStart);

		TLogN1("OK: %f Kbytes/s.\n",speed/1024);

		if(src->GetMediaType()<0)
		{
		TLog("8. Testing RAW sector reading... ");

		//force a spinup
		testReadSector(16,true);

		ticksStart = GetTickCount();
		for(int i=16;i<2016;i++)
		{
			if(testReadSector(i,true)<0)
			{
				TLogN1("%d failed!\n",i);
				TReturn(-1);
			}
		}
		ticksEnd = GetTickCount();
		speed =(2000.0f*2352.0f*1000.0f)/(ticksEnd-ticksStart);

		TLogN1("OK: %f Kbytes/s.\n",speed/1024);
		}

		TReturn(0);
	}

	MessageBoxEx(0,buffer,"Test Results",MB_OK,0);

	if(src!=NULL)
	{
		cdvdStopThread();
		delete src;
		src=NULL;
	}

	free(buffer);
	return ret;
}
