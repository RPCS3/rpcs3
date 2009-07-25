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
 
/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Modified by Florin for PCSX2 emu
 *  Fixed CdRead by linuzappz
 */
 
#include "PrecompiledHeader.h"

#include <ctype.h>

#include "IsoFStools.h"
#include "IsoFSdrv.h"
#include "CDVDaccess.h"

struct dir_toc_data
{
	u32 start_LBA;
	u32 num_sectors;
	u32 num_entries;
	u32 current_entry;
	u32 current_sector;
	u32 current_sector_offset;
	u32 inc_dirs;
	char extension_list[128+1];
};

//static u8 cdVolDescriptor[2048];
static dir_toc_data getDirTocData;
static cdVolDesc CDVolDesc;

static const int JolietMaxPath = 1024;

void _splitpath2(const char *constpath, char *dir, char *fname){
	// 255 char max path-length is an ISO9660 restriction
	// we must change this for Joliet or relaxed iso restriction support
	char pathcopy[JolietMaxPath+1];

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
//int tolower(int c);
int strcasecmp(const char *s1, const char *s2){
  while (*s1 != '\0' && tolower(*s1) == tolower(*s2))
    {
      s1++;
      s2++;
    }

  return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

// Copy a TOC Entry from the CD native format to our tidier format
void TocEntryCopy(TocEntry* tocEntry, const dirTocEntry* internalTocEntry){
	int i;
	int filenamelen;

	tocEntry->fileSize = internalTocEntry->fileSize;
	tocEntry->fileLBA = internalTocEntry->fileLBA;
	tocEntry->fileProperties = internalTocEntry->fileProperties;
	memcpy(tocEntry->date, internalTocEntry->dateStamp, 7); //TODO: Buffer read overrun, dateStamp is 6 bytes

	if (CDVolDesc.filesystemType == 2){
		// This is a Joliet Filesystem, so use Unicode to ISO string copy

		filenamelen = internalTocEntry->filenameLength/2;

//		if (!(tocEntry->fileProperties & 0x02)){
			// strip the ;1 from the filename
//			filenamelen -= 2;//(Florin) nah, do not strip ;1
//		}

		for (i=0; i < filenamelen; i++)
			tocEntry->filename[i] = internalTocEntry->filename[(i<<1)+1];

		tocEntry->filename[filenamelen] = 0;
	}
	else{
		filenamelen = internalTocEntry->filenameLength;

//		if (!(tocEntry->fileProperties & 0x02)){
			// strip the ;1 from the filename
//			filenamelen -= 2;//(Florin) nah, do not strip ;1
//		}

		// use normal string copy
		strncpy(tocEntry->filename,internalTocEntry->filename,128);
		tocEntry->filename[filenamelen] = 0;
	}
}

// Check if a TOC Entry matches our extension list
int TocEntryCompare(char* filename, char* extensions){
	static char ext_list[129];
	char* token, ext_point;

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

int IsoFS_readSectors(u32 lsn, u32 sectors, void *buf)
{
	for (u32 i=0; i<sectors; i++)
	{
		if (DoCDVDreadSector((u8*)((uptr)buf+2048*i), lsn+i, CDVD_MODE_2048) == -1) return 0;
	}
	return 1;
}

/**************************************************************
* The functions below are not exported for normal file-system *
* operations, but are used by the file-system operations, and *
* may also be exported  for use via RPC                       *
**************************************************************/

int IsoFS_getVolumeDescriptor(void)
{
	// Read until we find the last valid Volume Descriptor
	s32 volDescSector;
	cdVolDesc localVolDesc;

	DbgCon::WriteLn("IsoFS_GetVolumeDescriptor called");

	for (volDescSector = 16; volDescSector<20; volDescSector++)
	{
		IsoFS_readSectors(volDescSector,1,&localVolDesc);
//		CdSync(0x00);

		// If this is still a volume Descriptor
		if (strncmp((char*)localVolDesc.volID, "CD001", 5) == 0)
		{
			if ((localVolDesc.filesystemType == 1) ||
				(localVolDesc.filesystemType == 2))
			{
				memcpy_fast(&CDVolDesc, &localVolDesc, sizeof(cdVolDesc));
			}
		}
		else
			break;
	}

	if (CDVolDesc.filesystemType == 1)
		DbgCon::WriteLn( Color_Green, "CD FileSystem is ISO9660" );
	else if (CDVolDesc.filesystemType == 2)
		DbgCon::WriteLn( Color_Green, "CD FileSystem is Joliet");
	else 
		DbgCon::Notice("Could not detect CD FileSystem type");

	//	CdStop();

	return TRUE;
}

int IsoFS_findFile(const char* fname, TocEntry* tocEntry){
	char filename[g_MaxPath+1], pathname[JolietMaxPath+1], toc[2048];
	char* dirname;
	s32 found_dir, num_dir_sectors, current_sector, dir_lba;
	dirTocEntry* tocEntryPointer;
	TocEntry localTocEntry;	// used for internal checking only

	DbgCon::WriteLn("IsoFS_findfile(\"%s\") called", params fname);

	_splitpath2(fname, pathname, filename);

	// Find the TOC for a specific directory
	if (IsoFS_getVolumeDescriptor() != TRUE)
	{
		ISOFS_LOG("Could not get CD Volume Descriptor");
		return -1;
	}

	// Read the TOC of the root directory
	if (IsoFS_readSectors(CDVolDesc.rootToc.tocLBA,1,toc) != TRUE){
		ISOFS_LOG("Couldn't Read from CD !");
		return -1;
	}
	//CdSync(0x00);

	// point the tocEntryPointer at the first real toc entry
	tocEntryPointer = (dirTocEntry*)toc;

	num_dir_sectors = (tocEntryPointer->fileSize+2047) >> 11;	//round up fix
	current_sector = tocEntryPointer->fileLBA;

	tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
	tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);


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
					if (IsoFS_readSectors(current_sector,1,toc) != TRUE)
					{
						Console::Error("Couldn't Read from CD !");
						return -1;
					}
//					CdSync(0x00);

					tocEntryPointer = (dirTocEntry*)toc;
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
			tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
		}

		// If we havent found the directory name we wanted then fail
		if (found_dir != TRUE)
		{
			Console::Notice( "IsoFS_findfile: could not find dir" );
			return -1;
		}

		// Get next directory name
		dirname = strtok( NULL, "\\/" );

		// Read the TOC of the found subdirectory
		if (IsoFS_readSectors(localTocEntry.fileLBA,1,toc) != TRUE)
		{
			Console::Error("Couldn't Read from CD !");
			return -1;
		}
//		CdSync(0x00);

		num_dir_sectors = (localTocEntry.fileSize+2047) >> 11;	//round up fix
		current_sector = localTocEntry.fileLBA;

		// and point the tocEntryPointer at the first real toc entry
		tocEntryPointer = (dirTocEntry*)toc;
		tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
		tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
	}

	ISOFS_LOG("[IsoFStools] findfile: found dir, now looking for file");

	tocEntryPointer = (dirTocEntry*)toc;

	num_dir_sectors = (tocEntryPointer->fileSize+2047) >> 11;	//round up fix
	dir_lba = tocEntryPointer->fileLBA;

	tocEntryPointer = (dirTocEntry*)toc;
	tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
	tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);

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

				DbgCon::WriteLn("IsoFS_findfile: found file");

				return TRUE;
			}

			tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
		}

		num_dir_sectors--;

		if (num_dir_sectors > 0)
		{
			dir_lba++;

			if (IsoFS_readSectors(dir_lba,1,toc) != TRUE){
				Console::Error("Couldn't Read from CD !");
				return -1;
			}
//			CdSync(0x00);

			tocEntryPointer = (dirTocEntry*)toc;
		}
	}

	DbgCon::Notice("IsoFS_findfile: could not find file");

	return FALSE;
}

// This is the RPC-ready function which takes the request to start the tocEntry retrieval
int IsoFS_initDirectoryList(char* pathname, char* extensions, unsigned int inc_dirs){
	char toc[2048];
	char* dirname;
	s32 found_dir, num_dir_sectors, current_sector;
	u32 toc_entry_num;
	dirTocEntry* tocEntryPointer;
	TocEntry localTocEntry;

	// store the extension list statically for the retrieve function
	strncpy(getDirTocData.extension_list, extensions, 128);
	getDirTocData.extension_list[128]=0;

	getDirTocData.inc_dirs = inc_dirs;

	// Find the TOC for a specific directory
	if (IsoFS_getVolumeDescriptor() != TRUE){
		ISOFS_LOG("[IsoFStools] Could not get CD Volume Descriptor");
		return -1;
	}

	ISOFS_LOG("[IsoFStools] Getting Directory Listing for: \"%s\"", pathname);

	// Read the TOC of the root directory
	if (IsoFS_readSectors(CDVolDesc.rootToc.tocLBA,1,toc) != TRUE){
		ISOFS_LOG("[IsoFStools] Couldn't Read from CD !");
		return -1;
	}
	//CdSync(0x00);

	// point the tocEntryPointer at the first real toc entry
	tocEntryPointer = (dirTocEntry*)toc;

	num_dir_sectors = (tocEntryPointer->fileSize+2047) >> 11;
	current_sector = tocEntryPointer->fileLBA;

	tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
	tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);

	// use strtok to get the next dir name

	// if there isn't one, then assume we want the LBA
	// for the current one, and exit the while loop

	// if there is another dir name then increment dir_depth
	// and look through dir table entries until we find the right name
	// if we don't find the right name
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
					if (IsoFS_readSectors(current_sector,1,toc) != TRUE){
						ISOFS_LOG("[IsoFStools] Couldn't Read from CD !");
						
						return -1;
					}
					//CdSync(0x00);

					tocEntryPointer = (dirTocEntry*)toc;
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
					ISOFS_LOG("[IsoFStools] Found directory %s in subdir at sector %d",dirname,current_sector);
					ISOFS_LOG("[IsoFStools] LBA of found subdirectory = %d",localTocEntry.fileLBA);
					
					break;
				}
			}

			// point to the next entry
			tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
		}

		// If we havent found the directory name we wanted then fail
		if (found_dir != TRUE) return -1;

		// Get next directory name
		dirname = strtok( NULL, "\\/" );

		// Read the TOC of the found subdirectory
		if (IsoFS_readSectors(localTocEntry.fileLBA,1,toc) != TRUE){
			ISOFS_LOG("[IsoFStools] Couldn't Read from CD !");
			return -1;
		}
		//CdSync(0x00);

		num_dir_sectors = (localTocEntry.fileSize+2047) >> 11;
		current_sector = localTocEntry.fileLBA;

		// and point the tocEntryPointer at the first real toc entry
		tocEntryPointer = (dirTocEntry*)toc;
		tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
		tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
	}

	// We know how much data we need to read in from the DirTocHeader
	// but we need to read in at least 1 sector before we can get this value

	// Now we need to COUNT the number of entries (dont do anything with info at this point)
	// so set the tocEntryPointer to point to the first actual file entry

	// This is a bit of a waste of reads since we're not actually copying the data out yet,
	// but we dont know how big this TOC might be, so we cant allocate a specific size

	tocEntryPointer = (dirTocEntry*)toc;

	// Need to STORE the start LBA and number of Sectors, for use by the retrieve func.
	getDirTocData.start_LBA = localTocEntry.fileLBA;
	getDirTocData.num_sectors = (tocEntryPointer->fileSize+2047) >> 11;
	getDirTocData.num_entries = 0;
	getDirTocData.current_entry = 0;
	getDirTocData.current_sector = getDirTocData.start_LBA;
	getDirTocData.current_sector_offset = 0;

	num_dir_sectors = getDirTocData.num_sectors;

	tocEntryPointer = (dirTocEntry*)toc;
	tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
	tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);

	toc_entry_num=0;

	while(1){
		if ((tocEntryPointer->length == 0) || (((char*)tocEntryPointer-toc)>=2048)){
			// decrease the number of dirs remaining
			num_dir_sectors--;

			if (num_dir_sectors > 0){
				// If we've run out of entries, but arent on the last sector
				// then load another sector
				getDirTocData.current_sector++;

				if (IsoFS_readSectors(getDirTocData.current_sector,1,toc) != TRUE){
					ISOFS_LOG("[IsoFStools] Couldn't Read from CD !");
					return -1;
				}
				//CdSync(0x00);

				tocEntryPointer = (dirTocEntry*)toc;

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

		tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
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
int IsoFS_getDirectories(TocEntry tocEntry[], int req_entries){
	char toc[2048];
	s32 toc_entry_num;
	dirTocEntry* tocEntryPointer;

	if (IsoFS_readSectors(getDirTocData.current_sector,1,toc) != TRUE){
		ISOFS_LOG("[IsoFStools]	Couldn't Read from CD !");
		return -1;
	}
	//CdSync(0x00);

	if (getDirTocData.current_entry == 0){
		// if this is the first read then make sure we point to the first real entry
		tocEntryPointer = (dirTocEntry*)toc;
		tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);
		tocEntryPointer = (dirTocEntry*)((char*)tocEntryPointer + tocEntryPointer->length);

		getDirTocData.current_sector_offset = (char*)tocEntryPointer - toc;
	}
	else{
		tocEntryPointer = (dirTocEntry*)(toc + getDirTocData.current_sector_offset);
	}

	if (req_entries > 128) req_entries = 128;

	for (toc_entry_num=0; toc_entry_num < req_entries;)
	{
		if ((tocEntryPointer->length == 0) || (getDirTocData.current_sector_offset >= 2048))
		{
			// decrease the number of dirs remaining
			getDirTocData.num_sectors--;

			if (getDirTocData.num_sectors > 0)
			{
				// If we've run out of entries, but arent on the last sector
				// then load another sector
				getDirTocData.current_sector++;

				if (IsoFS_readSectors(getDirTocData.current_sector,1,toc) != TRUE){
					ISOFS_LOG("[IsoFStools]	Couldn't Read from CD !");
					return -1;
				}
				//CdSync(0x00);

				getDirTocData.current_sector_offset = 0;
				tocEntryPointer = (dirTocEntry*)(toc + getDirTocData.current_sector_offset);

//				continue;
			}
			else
			{
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
			tocEntryPointer = (dirTocEntry*)(toc + getDirTocData.current_sector_offset);
		}
		else{
			if (strlen(getDirTocData.extension_list) > 0){
				if (TocEntryCompare(tocEntry[toc_entry_num].filename, getDirTocData.extension_list) == TRUE){
					// increment the number of matching entries counter
					toc_entry_num++;
				}

				getDirTocData.current_sector_offset += tocEntryPointer->length;
				tocEntryPointer = (dirTocEntry*)(toc + getDirTocData.current_sector_offset);

			}
			else{
				toc_entry_num++;
				getDirTocData.current_sector_offset += tocEntryPointer->length;
				tocEntryPointer = (dirTocEntry*)(toc + getDirTocData.current_sector_offset);
			}
		}
	}
	return (toc_entry_num);
}
