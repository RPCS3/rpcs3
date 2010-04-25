#include "CDVD.h"

toc_data cdtoc;

s32 cdvdParseTOC()
{
	memset(&cdtoc,0,sizeof(cdtoc));

	s32 len = src->GetSectorCount();

	tracks[0].length = len;
	tracks[0].start_lba=0;
	tracks[0].type=0;
	tracks[1].start_lba=0;

	if(len<=0)
	{
		curDiskType=CDVD_TYPE_NODISC;
		tracks[0].length=0;
		strack=1;
		etrack=0;
		return 0;
	}

    s32 lastaddr = src->GetLayerBreakAddress();


	if(lastaddr>=0)
	{
        if((lastaddr > 0)&&(tracks[0].length>lastaddr))
		{
			tracks[1].length=lastaddr+1;
			tracks[1].type=0;

			tracks[2].start_lba = tracks[1].length;
			tracks[2].length    = tracks[0].length-tracks[1].length;
			tracks[2].type=0;

			strack=1;
			etrack=2;
		}
        else
		{
			tracks[1].length=tracks[0].length;
			tracks[1].type=0;

			strack=1;
			etrack=1;
        }
	}
	else
	{
		u8 min, sec, frm;

		if(src->ReadTOC((char*)&cdtoc,sizeof(cdtoc))<0)
		{
			/*
			printf(" * CDVD: WARNING ReadTOC() failed, trying to use MCI instead...\n");
			delete src;
			int status = MCI_CDGetTOC(source_drive);

			src=new SOURCECLASS(csrc);

			if(status<0)*/
				return -1;

			//return 0;
		}

#define btoi(b) ((b>>4)*10+(b&0xF))

		int length = (cdtoc.Length[0]<<8) | cdtoc.Length[1];
		int descriptors = length/sizeof(cdtoc.Descriptors[0]);
		for(int i=0;i<descriptors;i++)
		{
			switch(cdtoc.Descriptors[i].Point)
			{
			case 0xa0:
				if(cdtoc.Descriptors[i].SessionNumber==cdtoc.FirstCompleteSession)
				{
					strack = cdtoc.Descriptors[i].Msf[0];
				}
				break;
			case 0xa1:
				if(cdtoc.Descriptors[i].SessionNumber==cdtoc.LastCompleteSession)
				{
					etrack = cdtoc.Descriptors[i].Msf[0];
				}
				break;
			case 0xa2: // session size
				if(cdtoc.Descriptors[i].SessionNumber==cdtoc.LastCompleteSession)
				{
					min=cdtoc.Descriptors[i].Msf[0];
					sec=cdtoc.Descriptors[i].Msf[1];
					frm=cdtoc.Descriptors[i].Msf[2];

					tracks[0].length = MSF_TO_LBA(min,sec,frm);
					tracks[0].type   = 0;
				}
				break;
			case 0xb0:
				break;
			case 0xb1:
				break;
			case 0xb2:
				break;
			case 0xc0:
				break;
			default:
				if((cdtoc.Descriptors[i].Point<100)&&(cdtoc.Descriptors[i].Point>0))
				{
					int tn=cdtoc.Descriptors[i].Point;

					min=cdtoc.Descriptors[i].Msf[0];
					sec=cdtoc.Descriptors[i].Msf[1];
					frm=cdtoc.Descriptors[i].Msf[2];

					tracks[tn].start_lba = MSF_TO_LBA(min,sec,frm);
					if(tn>1)
						tracks[tn-1].length = tracks[tn].start_lba - tracks[tn-1].start_lba;

					if((cdtoc.Descriptors[i].Control&4)==0)
					{
						tracks[tn].type = 1;
					}
					else if((cdtoc.Descriptors[i].Control&0xE)==4)
					{
						tracks[tn].type = CDVD_MODE1_TRACK;
					}
					else
					{
						tracks[tn].type = CDVD_MODE1_TRACK;
					}

					fprintf(stderr,"Track %d: %d mins %d secs %d frames\n",tn,min,sec,frm);
				}
				else if(cdtoc.Descriptors[i].Point>0)
				{
					printf("Found code 0x%02x\n",cdtoc.Descriptors[i].Point);
				}
				break;
			}
		}

		tracks[etrack].length = tracks[0].length - tracks[etrack].start_lba;
	}

	return 0;
}
