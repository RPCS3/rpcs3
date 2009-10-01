/* General ioctl() CD-ROM command function */
#include "CDVD.h"

#define MSF_TO_LBA(m,s,f) ((m*60+s)*75+f-150)

int MCI_CDioctl(int id, UINT msg, DWORD flags, void *arg)
{
     MCIERROR mci_error;

     mci_error = mciSendCommand(id, msg, flags, (DWORD)arg);
     if ( mci_error ) {
             char error[256];

             mciGetErrorString(mci_error, error, 256);
			 printf(" * CDVD mciTOC: mciSendCommand() error: %s", error);
     }
     return(!mci_error ? 0 : -1);
}

int MCI_CDGetTOC(char driveLetter)
{
	MCI_OPEN_PARMS mci_open;
	MCI_SET_PARMS mci_set;
    MCI_STATUS_PARMS mci_status;

	char device[3];
    int i, okay;
    DWORD flags;

    okay = 0;

	int cdromId;

	device[0] = driveLetter;
	device[1] = ':';
	device[2] = '\0';

	/* Open the requested device */
	mci_open.lpstrDeviceType = (LPCSTR) MCI_DEVTYPE_CD_AUDIO;
	mci_open.lpstrElementName = device;
	flags = (MCI_OPEN_TYPE|MCI_OPEN_SHAREABLE|MCI_OPEN_TYPE_ID|MCI_OPEN_ELEMENT);
	if ( MCI_CDioctl(0, MCI_OPEN, flags, &mci_open) < 0 ) 
	{
		flags &= ~MCI_OPEN_SHAREABLE;
		if ( MCI_CDioctl(0, MCI_OPEN, flags, &mci_open) < 0 ) 
		{
			return(-1);
		}
	}
	cdromId = mci_open.wDeviceID;

	/* Set the minute-second-frame time format */
	mci_set.dwTimeFormat = MCI_FORMAT_MSF;
	MCI_CDioctl(cdromId, MCI_SET, MCI_SET_TIME_FORMAT, &mci_set);

	/* Request number of tracks */
    mci_status.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    flags = MCI_STATUS_ITEM | MCI_WAIT;
    if ( MCI_CDioctl(cdromId, MCI_STATUS, flags, &mci_status) == 0 ) 
	{
		strack=1;
		etrack=mci_status.dwReturn;

        /* Read all the track TOC entries */
        flags = MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT;

        for ( i=strack; i<etrack; ++i ) 
		{
            mci_status.dwTrack = i+1;

			mci_status.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
            if ( MCI_CDioctl(cdromId, MCI_STATUS, flags, &mci_status) < 0 ) 
			{
				break;
            }
            if ( mci_status.dwReturn == MCI_CDA_TRACK_AUDIO ) 
			{
				tracks[i+1].type = CDVD_AUDIO_TRACK;
			} else {
				tracks[i+1].type = CDVD_MODE1_TRACK; //no way to detect between mode1 and mode2 yet
            }

			mci_status.dwItem = MCI_STATUS_POSITION;
			if ( MCI_CDioctl(cdromId, MCI_STATUS, flags, &mci_status) < 0 ) 
			{
				break;
            }
			tracks[i+1].start_lba = MSF_TO_LBA(
										MCI_MSF_MINUTE(mci_status.dwReturn),
										MCI_MSF_SECOND(mci_status.dwReturn),
										MCI_MSF_FRAME(mci_status.dwReturn));
			tracks[i+1].length = 0;
            if ( i > 0 ) 
			{
                tracks[i].length = tracks[i+1].start_lba - tracks[i].start_lba;
			}
		}
        if ( i == etrack ) 
		{
            mci_status.dwTrack = i;
            mci_status.dwItem = MCI_STATUS_LENGTH;
            if ( MCI_CDioctl(cdromId, MCI_STATUS, flags, &mci_status) == 0 ) 
			{
				tracks[i].length = MSF_TO_LBA(
                                     MCI_MSF_MINUTE(mci_status.dwReturn),
                                     MCI_MSF_SECOND(mci_status.dwReturn),
                                     MCI_MSF_FRAME(mci_status.dwReturn)) + 1; /* +1 to fix MCI last track length bug */
				/* compute lead-out offset */
				tracks[i+1].start_lba = tracks[i].start_lba + tracks[i].length;
				tracks[i+1].length = 0;
				okay = 1;
            }
        }
    }

    MCI_CDioctl(cdromId, MCI_CLOSE, MCI_WAIT, NULL);

    return(okay ? 0 : -1);
}
