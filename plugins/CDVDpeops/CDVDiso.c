/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Modified by Florin for PCSX2 emu
 *  Fixed CdRead by linuzappz
 */

#include <string.h>
#ifdef __LINUX__
#define strnicmp strncasecmp
#endif

#include "CDVDiso.h"
#include "CDVDisodrv.h"

struct dir_toc_data{
	unsigned int start_LBA;
	unsigned int num_sectors;
	unsigned int num_entries;
	unsigned int current_entry;
	unsigned int current_sector;
	unsigned int current_sector_offset;
	unsigned int inc_dirs;
	unsigned char extension_list[128+1];
};

//static u8 cdVolDescriptor[2048];
static struct dir_toc_data getDirTocData;
static struct cdVolDesc CDVolDesc;

void _splitpath2(const char *constpath, char *dir, char *fname){
	// 255 char max path-length is an ISO9660 restriction
	// we must change this for Joliet or relaxed iso restriction support
	static char pathcopy[1024+1];

	char* slash;

	strncpy(pathcopy, constpath, 1024);

    slash = strrchr (pathcopy, '/');

	// if the path doesn't contain a '/' then look for a '\'
    if (!slash)
		slash = strrchr (pathcopy,  (int)'\\');

	// if a slash was found
	if (slash != NULL)
	{
		// null terminate the path
		slash[0] = 0;
		// and copy the path into 'dir'
		strncpy(dir, pathcopy, 1024);
		dir[255]=0;

		// copy the filename into 'fname'
		strncpy(fname, slash+1, 128);
		fname[128]=0;
	}
	else
	{
		dir[0] = 0;

		strncpy(fname, pathcopy, 128);
		fname[128]=0;
	}

}

// Used in findfile
int tolower(int c);
int strcasecmp(const char *s1, const char *s2){
  while (*s1 != '\0' && tolower(*s1) == tolower(*s2))
    {
      s1++;
      s2++;
    }

  return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

// Copy a TOC Entry from the CD native format to our tidier format
void TocEntryCopy(struct TocEntry* tocEntry, struct dirTocEntry* internalTocEntry){
	int i;
	int filenamelen;

	tocEntry->fileSize = internalTocEntry->fileSize;
	tocEntry->fileLBA = internalTocEntry->fileLBA;
	tocEntry->fileProperties = internalTocEntry->fileProperties;
	memcpy(tocEntry->date, internalTocEntry->dateStamp, 7);

	if (CDVolDesc.filesystemType == 2){
		// This is a Joliet Filesystem, so use Unicode to ISO string copy

		filenamelen = internalTocEntry->filenameLength/2;

		if (!(tocEntry->fileProperties & 0x02)){
			// strip the ;1 from the filename
//			filenamelen -= 2;//(Florin) nah, do not strip ;1
		}

		for (i=0; i < filenamelen; i++)
			tocEntry->filename[i] = internalTocEntry->filename[(i<<1)+1];

		tocEntry->filename[filenamelen] = 0;
	}
	else{
		filenamelen = internalTocEntry->filenameLength;

		if (!(tocEntry->fileProperties & 0x02)){
			// strip the ;1 from the filename
//			filenamelen -= 2;//(Florin) nah, do not strip ;1
		}

		// use normal string copy
		strncpy(tocEntry->filename,internalTocEntry->filename,128);
		tocEntry->filename[filenamelen] = 0;
	}
}

// Check if a TOC Entry matches our extension list
int TocEntryCompare(char* filename, char* extensions){
	static char ext_list[129];

	char* token;

	char* ext_point;

	strncpy(ext_list,extensions,128);
	ext_list[128]=0;

	token = strtok( ext_list, " ," );
	while( token != NULL )
	{
		// if 'token' matches extension of 'filename'
		// then return a match
		ext_point = strrchr(filename,'.');

		if (strnicmp(ext_point, token, strlen(token)) == 0)
			return (TRUE);

		/* Get next token: */
		token = strtok( NULL, " ," );
	}

	// If not match found then return FALSE
	return (FALSE);

}

#define CD_SECS              60 /* seconds per minute */
#define CD_FRAMES            75 /* frames per second */
#define CD_MSF_OFFSET       150 /* MSF numbering offset of first frame */

int CdRead(u32 lsn, u32 sectors, void *buf, CdRMode *mode){
	u32	i;
	u8*	buff;
	int rmode;

	switch (mode->datapattern) {
		case CdSecS2048:
			rmode = CDVD_MODE_2048; break;
		case CdSecS2328:
			rmode = CDVD_MODE_2328; break;
		case CdSecS2340:
			rmode = CDVD_MODE_2340; break;
		default:
			return 0;
	}
	
	for (i=0; i<sectors; i++){
		if (CDVDreadTrack(lsn+i, rmode)==-1)
			return 0;
		buff = CDVDgetBuffer();
		if (buff==NULL) return 0;

		switch (mode->datapattern){
			case CdSecS2048:
				memcpy((void*)((uptr)buf+2048*i), buff, 2048);break;//only data
			case CdSecS2328:
				memcpy((void*)((uptr)buf+2328*i), buff, 2328);break;//without sync & head & sub
			case CdSecS2340:
				memcpy((void*)((uptr)buf+2340*i), buff, 2340);break;//without sync
		}
	}
	return 1;
}

int DvdRead(u32 lsn, u32 sectors, void *buf, CdRMode *mode){
	u32	i;
	u8*	buff;

	for (i=lsn; i<(lsn+sectors); i++){
		if (CDVDreadTrack(i, CDVD_MODE_2048)==-1)
			return 0;
		buff = CDVDgetBuffer();
		if (buff==NULL) return 0;

//		switch (mode->datapattern){
//			case CdSecS2064:
				((u32*)buf)[0] = i + 0x30000;
				memcpy((u8*)buf+12, buff, 2048); 
				(u8*)buf+= 2064; break;
//			default:
//				return 0;
//		}
	}

	return 1;
}

/**************************************************************
* The functions below are not exported for normal file-system *
* operations, but are used by the file-system operations, and *
* may also be exported  for use via RPC                       *
**************************************************************/

int CDVD_GetVolumeDescriptor(void){
	// Read until we find the last valid Volume Descriptor
	int volDescSector;

	static struct cdVolDesc localVolDesc;

#ifdef DEBUG
	printf("CDVD_GetVolumeDescriptor called\n");
#endif

	for (volDescSector = 16; volDescSector<20; volDescSector++)
	{
		CdRead(volDescSector,1,&localVolDesc,&cdReadMode);
//		CdSync(0x00);

		// If this is still a volume Descriptor
		if (strncmp(localVolDesc.volID, "CD001", 5) == 0)
		{
			if ((localVolDesc.filesystemType == 1) ||
				(localVolDesc.filesystemType == 2))
			{
				memcpy(&CDVolDesc, &localVolDesc, sizeof(struct cdVolDesc));
			}
		}
		else
			break;
	}

#ifdef DEBUG
	if (CDVolDesc.filesystemType == 1)
		printf("CD FileSystem is ISO9660\n");
	else if (CDVolDesc.filesystemType == 2)
		printf("CD FileSystem is Joliet\n");
	else printf("Could not detect CD FileSystem type\n");
#endif
//	CdStop();

	return TRUE;
}

int CDVD_findfile(char* fname, struct TocEntry* tocEntry){
	static char filename[128+1];
	static char pathname[1024+1];
	static char toc[2048];
	char* dirname;


	static struct TocEntry localTocEntry;	// used for internal checking only

	int found_dir;

	int num_dir_sectors;
	int current_sector;

	int dir_lba;

	struct dirTocEntry* tocEntryPointer;

#ifdef DEBUG
	printf("CDVD_findfile called\n");
#endif

	//make sure we have good cdReadMode
	cdReadMode.trycount = 0;
	cdReadMode.spindlctrl = CdSpinStm;
	cdReadMode.datapattern = CdSecS2048;

	_splitpath2(fname, pathname, filename);

	// Find the TOC for a specific directory
	if (CDVD_GetVolumeDescriptor() != TRUE){
#ifdef RPC_LOG
		RPC_LOG("Could not get CD Volume Descriptor\n");
#endif
		return -1;
	}

	// Read the TOC of the root directory
	if (CdRead(CDVolDesc.rootToc.tocLBA,1,toc,&cdReadMode) != TRUE){
#ifdef RPC_LOG
		RPC_LOG("Couldn't Read from CD !\n");
#endif
		return -1;
	}
	//CdSync(0x00);

	// point the tocEntryPointer at the first real toc entry
	(char*)tocEntryPointer = toc;

	num_dir_sectors = (tocEntryPointer->fileSize+2047) >> 11;	//round up fix
	current_sector = tocEntryPointer->fileLBA;

	(char*)tocEntryPointer += tocEntryPointer->length;
	(char*)tocEntryPointer += tocEntryPointer->length;


	localTocEntry.fileLBA = CDVolDesc.rootToc.tocLBA;
	// while (there are more dir names in the path)
	dirname = strtok( pathname, "\\/" );

	while( dirname != NULL )
	{
		found_dir = FALSE;
/*
		while(tocEntryPointer->length > 0)
		{
			// while there are still more directory entries then search through
			// for the one we want

			if (tocEntryPointer->fileProperties & 0x02)
			{
				// Copy the CD format TOC Entry to our format
				TocEntryCopy(&localTocEntry, tocEntryPointer);

				// If this TOC Entry is a directory,
				// then see if it has the right name
				if (strcasecmp(dirname,localTocEntry.filename) == 0)
				{
					// if the name matches then we've found the directory
					found_dir = TRUE;
					break;
				}
			}

			// point to the next entry
			(char*)tocEntryPointer += tocEntryPointer->length;
		}
*/
		while(1)
		{
			if ((tocEntryPointer->length == 0) || (((char*)tocEntryPointer-toc)>=2048))
			{
				num_dir_sectors--;

				if (num_dir_sectors > 0)
				{
					// If we've run out of entries, but arent on the last sector
					// then load another sector

					current_sector++;
					if (CdRead(current_sector,1,toc,&cdReadMode) != TRUE)
					{
						return -1;
					}
//					CdSync(0x00);

					(char*)tocEntryPointer = toc;
				}
				else
				{
					// Couldnt find the directory, and got to end of directory
					return -1;
				}
			}


			if (tocEntryPointer->fileProperties & 0x02)
			{
				TocEntryCopy(&localTocEntry, tocEntryPointer);

				// If this TOC Entry is a directory,
				// then see if it has the right name
				if (strcmp(dirname,localTocEntry.filename) == 0)
				{
					// if the name matches then we've found the directory
					found_dir = TRUE;
					break;
				}
			}

			// point to the next entry
			(char*)tocEntryPointer += tocEntryPointer->length;
		}

		// If we havent found the directory name we wanted then fail
		if (found_dir != TRUE)
		{
			return -1;
		}

		// Get next directory name
		dirname = strtok( NULL, "\\/" );

		// Read the TOC of the found subdirectory
		if (CdRead(localTocEntry.fileLBA,1,toc,&cdReadMode) != TRUE)
		{
			return -1;
		}
//		CdSync(0x00);

		num_dir_sectors = (localTocEntry.fileSize+2047) >> 11;	//round up fix
		current_sector = localTocEntry.fileLBA;

		// and point the tocEntryPointer at the first real toc entry
		(char*)tocEntryPointer = toc;
		(char*)tocEntryPointer += tocEntryPointer->length;
		(char*)tocEntryPointer += tocEntryPointer->length;
	}

	(char*)tocEntryPointer = toc;

	num_dir_sectors = (tocEntryPointer->fileSize+2047) >> 11;	//round up fix
	dir_lba = tocEntryPointer->fileLBA;

	(char*)tocEntryPointer = toc;
	(char*)tocEntryPointer += tocEntryPointer->length;
	(char*)tocEntryPointer += tocEntryPointer->length;

	while (num_dir_sectors > 0)
	{
		while(tocEntryPointer->length != 0)
		{
			// Copy the CD format TOC Entry to our format
			TocEntryCopy(&localTocEntry, tocEntryPointer);

			if ((strnicmp(localTocEntry.filename, filename, strlen(filename)) == 0) ||
				((filename[strlen(filename)-2] == ';') &&
				 (localTocEntry.filename[strlen(localTocEntry.filename)-2] == ';') && 
				 (strnicmp(localTocEntry.filename, filename, strlen(filename)-2) == 0)))
			{
				// if the filename matches then copy the toc Entry
				tocEntry->fileLBA = localTocEntry.fileLBA;
				tocEntry->fileProperties = localTocEntry.fileProperties;
				tocEntry->fileSize = localTocEntry.fileSize;

				strcpy(tocEntry->filename, localTocEntry.filename);
				memcpy(tocEntry->date, localTocEntry.date, 7);

#ifdef DEBUG
				printf("CDVD_findfile: found file\n");
#endif

				return TRUE;
			}

			(char*)tocEntryPointer += tocEntryPointer->length;
		}

		num_dir_sectors--;

		if (num_dir_sectors > 0)
		{
			dir_lba++;

			if (CdRead(dir_lba,1,toc,&cdReadMode) != TRUE){
				return -1;
			}
//			CdSync(0x00);

			(char*)tocEntryPointer = toc;
		}
	}

	return FALSE;
}

// This is the RPC-ready function which takes the request to start the tocEntry retrieval
int CDVD_GetDir_RPC_request(char* pathname, char* extensions, unsigned int inc_dirs){
//	int dir_depth = 1;
	static char toc[2048];
	char* dirname;
	int found_dir;
	int num_dir_sectors;
	unsigned int toc_entry_num;
	struct dirTocEntry* tocEntryPointer;
	static struct TocEntry localTocEntry;
	int current_sector;

	// store the extension list statically for the retrieve function
	strncpy(getDirTocData.extension_list, extensions, 128);
	getDirTocData.extension_list[128]=0;

	getDirTocData.inc_dirs = inc_dirs;

	// Find the TOC for a specific directory
	if (CDVD_GetVolumeDescriptor() != TRUE){
#ifdef RPC_LOG
		RPC_LOG("[RPC:cdvd] Could not get CD Volume Descriptor\n");
#endif
		return -1;
	}

#ifdef RPC_LOG
	RPC_LOG("[RPC:cdvd] Getting Directory Listing for: \"%s\"\n", pathname);
#endif

	// Read the TOC of the root directory
	if (CdRead(CDVolDesc.rootToc.tocLBA,1,toc,&cdReadMode) != TRUE){
#ifdef RPC_LOG
	RPC_LOG("[RPC:    ] Couldn't Read from CD !\n");
#endif
		return -1;
	}
	//CdSync(0x00);

	// point the tocEntryPointer at the first real toc entry
	(char*)tocEntryPointer = toc;

	num_dir_sectors = (tocEntryPointer->fileSize+2047) >> 11;
	current_sector = tocEntryPointer->fileLBA;

	(char*)tocEntryPointer += tocEntryPointer->length;
	(char*)tocEntryPointer += tocEntryPointer->length;

	// use strtok to get the next dir name

	// if there isnt one, then assume we want the LBA
	// for the current one, and exit the while loop

	// if there is another dir name then increment dir_depth
	// and look through dir table entries until we find the right name
	// if we dont find the right name
	// before finding an entry at a higher level (lower num), then return nothing

	localTocEntry.fileLBA = CDVolDesc.rootToc.tocLBA;

	// while (there are more dir names in the path)
	dirname = strtok( pathname, "\\/" );
	while( dirname != NULL ){
		found_dir = FALSE;

		while(1){
			if ((tocEntryPointer->length == 0) || (((char*)tocEntryPointer-toc)>=2048))	{
				num_dir_sectors--;

				if (num_dir_sectors > 0){
					// If we've run out of entries, but arent on the last sector
					// then load another sector

					current_sector++;
					if (CdRead(current_sector,1,toc,&cdReadMode) != TRUE){
#ifdef RPC_LOG
						RPC_LOG("[RPC:    ] Couldn't Read from CD !\n");
#endif
						return -1;
					}
					//CdSync(0x00);

					(char*)tocEntryPointer = toc;
				}
				else{
					// Couldnt find the directory, and got to end of directory
					return -1;
				}
			}

			if (tocEntryPointer->fileProperties & 0x02){
				TocEntryCopy(&localTocEntry, tocEntryPointer);

				// If this TOC Entry is a directory,
				// then see if it has the right name
				if (strcmp(dirname,localTocEntry.filename) == 0){
					// if the name matches then we've found the directory
					found_dir = TRUE;
#ifdef RPC_LOG
					RPC_LOG("[RPC:    ] Found directory %s in subdir at sector %d\n",dirname,current_sector);
					RPC_LOG("[RPC:    ] LBA of found subdirectory = %d\n",localTocEntry.fileLBA);
#endif
					break;
				}
			}

			// point to the next entry
			(char*)tocEntryPointer += tocEntryPointer->length;
		}

		// If we havent found the directory name we wanted then fail
		if (found_dir != TRUE)
			return -1;

		// Get next directory name
		dirname = strtok( NULL, "\\/" );

		// Read the TOC of the found subdirectory
		if (CdRead(localTocEntry.fileLBA,1,toc,&cdReadMode) != TRUE){
#ifdef RPC_LOG
			RPC_LOG("[RPC:    ] Couldn't Read from CD !\n");
#endif
			return -1;
		}
		//CdSync(0x00);

		num_dir_sectors = (localTocEntry.fileSize+2047) >> 11;
		current_sector = localTocEntry.fileLBA;

		// and point the tocEntryPointer at the first real toc entry
		(char*)tocEntryPointer = toc;
		(char*)tocEntryPointer += tocEntryPointer->length;
		(char*)tocEntryPointer += tocEntryPointer->length;
	}

	// We know how much data we need to read in from the DirTocHeader
	// but we need to read in at least 1 sector before we can get this value

	// Now we need to COUNT the number of entries (dont do anything with info at this point)
	// so set the tocEntryPointer to point to the first actual file entry

	// This is a bit of a waste of reads since we're not actually copying the data out yet,
	// but we dont know how big this TOC might be, so we cant allocate a specific size

	(char*)tocEntryPointer = toc;

	// Need to STORE the start LBA and number of Sectors, for use by the retrieve func.
	getDirTocData.start_LBA = localTocEntry.fileLBA;
	getDirTocData.num_sectors = (tocEntryPointer->fileSize+2047) >> 11;
	getDirTocData.num_entries = 0;
	getDirTocData.current_entry = 0;
	getDirTocData.current_sector = getDirTocData.start_LBA;
	getDirTocData.current_sector_offset = 0;

	num_dir_sectors = getDirTocData.num_sectors;

	(char*)tocEntryPointer = toc;
	(char*)tocEntryPointer += tocEntryPointer->length;
	(char*)tocEntryPointer += tocEntryPointer->length;

	toc_entry_num=0;

	while(1){
		if ((tocEntryPointer->length == 0) || (((char*)tocEntryPointer-toc)>=2048)){
			// decrease the number of dirs remaining
			num_dir_sectors--;

			if (num_dir_sectors > 0){
				// If we've run out of entries, but arent on the last sector
				// then load another sector
				getDirTocData.current_sector++;

				if (CdRead(getDirTocData.current_sector,1,toc,&cdReadMode) != TRUE){
#ifdef RPC_LOG
			RPC_LOG("[RPC:    ] Couldn't Read from CD !\n");
#endif
					return -1;
				}
				//CdSync(0x00);

				(char*)tocEntryPointer = toc;

//				continue;
			}
			else{
				getDirTocData.num_entries = toc_entry_num;
				getDirTocData.current_sector = getDirTocData.start_LBA;
				return (toc_entry_num);
			}
		}

		// We've found a file/dir in this directory
		// now check if it matches our extension list (if there is one)
		TocEntryCopy(&localTocEntry, tocEntryPointer);

		if (localTocEntry.fileProperties & 0x02){
			// If this is a subdir, then check if we want to include subdirs
			if (getDirTocData.inc_dirs){
				toc_entry_num++;
			}
		}
		else{
			if (strlen(getDirTocData.extension_list) > 0){
				if (TocEntryCompare(localTocEntry.filename, getDirTocData.extension_list) == TRUE){
					// increment the number of matching entries counter
					toc_entry_num++;
				}
			}
			else{
				toc_entry_num++;
			}
		}

		(char*)tocEntryPointer += tocEntryPointer->length;

	}


	// THIS SHOULD BE UNREACHABLE - 
	// since we are trying to count ALL matching entries, rather than upto a limit


	// STORE total number of TOC entries
	getDirTocData.num_entries = toc_entry_num;
	getDirTocData.current_sector = getDirTocData.start_LBA;


	// we've reached the toc entry limit, so return how many we've done
	return (toc_entry_num);

}

// This function can be called repeatedly after CDVD_GetDir_RPC_request to get the actual entries
// buffer (tocEntry) must be 18KB in size, and this will be filled with a maximum of 128 entries in one go
int CDVD_GetDir_RPC_get_entries(struct TocEntry tocEntry[], int req_entries){
	static char toc[2048];
	int toc_entry_num;

	struct dirTocEntry* tocEntryPointer;

	if (CdRead(getDirTocData.current_sector,1,toc,&cdReadMode) != TRUE){
#ifdef RPC_LOG
		RPC_LOG("[RPC:cdvd]	Couldn't Read from CD !\n");
#endif
		return -1;
	}
	//CdSync(0x00);

	if (getDirTocData.current_entry == 0){
		// if this is the first read then make sure we point to the first real entry
		(char*)tocEntryPointer = toc;
		(char*)tocEntryPointer += tocEntryPointer->length;
		(char*)tocEntryPointer += tocEntryPointer->length;

		getDirTocData.current_sector_offset = (char*)tocEntryPointer - toc;
	}
	else{
		(char*)tocEntryPointer = toc + getDirTocData.current_sector_offset;
	}

	if (req_entries > 128)
		req_entries = 128;

	for (toc_entry_num=0; toc_entry_num < req_entries;){
		if ((tocEntryPointer->length == 0) || (getDirTocData.current_sector_offset >= 2048)){
			// decrease the number of dirs remaining
			getDirTocData.num_sectors--;

			if (getDirTocData.num_sectors > 0){
				// If we've run out of entries, but arent on the last sector
				// then load another sector
				getDirTocData.current_sector++;

				if (CdRead(getDirTocData.current_sector,1,toc,&cdReadMode) != TRUE){
#ifdef RPC_LOG
					RPC_LOG("[RPC:cdvd]	Couldn't Read from CD !\n");
#endif
					return -1;
				}
				//CdSync(0x00);

				getDirTocData.current_sector_offset = 0;
				(char*)tocEntryPointer = toc + getDirTocData.current_sector_offset;

//				continue;
			}
			else{
				return (toc_entry_num);
			}
		}

		// This must be incremented even if the filename doesnt match extension list
		getDirTocData.current_entry++;  

		// We've found a file in this directory
		// now check if it matches our extension list (if there is one)

		// Copy the entry regardless, as it makes the comparison easier
		// if it doesn't match then it will just be overwritten
		TocEntryCopy(&tocEntry[toc_entry_num], tocEntryPointer);

		if (tocEntry[toc_entry_num].fileProperties & 0x02){
			// If this is a subdir, then check if we want to include subdirs
			if (getDirTocData.inc_dirs)	{
				toc_entry_num++;
			}

			getDirTocData.current_sector_offset += tocEntryPointer->length;
			(char*)tocEntryPointer = toc + getDirTocData.current_sector_offset;
		}
		else{
			if (strlen(getDirTocData.extension_list) > 0){
				if (TocEntryCompare(tocEntry[toc_entry_num].filename, getDirTocData.extension_list) == TRUE){
					// increment the number of matching entries counter
					toc_entry_num++;
				}

				getDirTocData.current_sector_offset += tocEntryPointer->length;
				(char*)tocEntryPointer = toc + getDirTocData.current_sector_offset;

			}
			else{
				toc_entry_num++;
				getDirTocData.current_sector_offset += tocEntryPointer->length;
				(char*)tocEntryPointer = toc + getDirTocData.current_sector_offset;
			}
		}
/*
		if (strlen(getDirTocData.extension_list) > 0)
		{
			if (TocEntryCompare(tocEntry[toc_entry_num].filename, getDirTocData.extension_list) == TRUE)
			{

				// increment this here, rather than in the main for loop
				// since this should count the number of matching entries
				toc_entry_num++; 
			}

			getDirTocData.current_sector_offset += tocEntryPointer->length;
			(char*)tocEntryPointer = toc + getDirTocData.current_sector_offset;
		}
		else
		{
			toc_entry_num++; 
			getDirTocData.current_sector_offset += tocEntryPointer->length;
			(char*)tocEntryPointer = toc + getDirTocData.current_sector_offset;
		}
*/
	}
	return (toc_entry_num);
}
