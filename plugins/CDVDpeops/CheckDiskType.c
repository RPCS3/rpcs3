
#include <stdio.h>
#include "CDVDlib.h"
#include "CDVDiso.h"
#include "CDVDisodrv.h"

int CheckDiskType(int baseType){
	int		f;
	char	buffer[256];//if a file is longer...it should be shorter :D
	char	*pos;
	static struct TocEntry tocEntry;

	CDVDFS_init();

	// check if the file exists
	if (CDVD_findfile("SYSTEM.CNF;1", &tocEntry) != TRUE){
		if (CDVD_findfile("VIDEO_TS/VIDEO_TS.IFO;1", &tocEntry) != TRUE)
			if (CDVD_findfile("PSX.EXE;1", &tocEntry) != TRUE)
				return CDVD_TYPE_ILLEGAL;
			else
				return CDVD_TYPE_PSCD;
		else
			return CDVD_TYPE_DVDV;
	}

	f=CDVDFS_open("SYSTEM.CNF;1", 1);
	CDVDFS_read(f, buffer, 256);
	CDVDFS_close(f);

	buffer[tocEntry.fileSize]='\0';

	pos=strstr(buffer, "BOOT2");
	if (pos==NULL){
		pos=strstr(buffer, "BOOT");
		if (pos==NULL) {
			return CDVD_TYPE_ILLEGAL;
		}
		return CDVD_TYPE_PSCD;
	}
	return (baseType==CDVD_TYPE_DETCTCD)?CDVD_TYPE_PS2CD:CDVD_TYPE_PS2DVD;
}
