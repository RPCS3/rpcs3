/***************************************************************
* romdir.c, based over Alex Lau (http://alexlau.8k.com) RomDir *
****************************************************************/
#include "romdir.h"

// searches between beginning and end addresses for a romdir structure.
// if found it returns info about it in romDirInfo.
//
// args:	address to start searching from
//			address to stop searching at
//			gets filled in with info if found
// returns:	a pointer to the filled in romDirInfo if successful
//			NULL if error
ROMDIR_INFO* searchRomDir(const u32* searchStartAddr, const u32* searchEndAddr, ROMDIR_INFO* romDirInfo)
{
	int offset = 0;
	ROMDIR_ENTRY* dir_entry = (ROMDIR_ENTRY*)searchStartAddr;
	while (dir_entry < (ROMDIR_ENTRY*)searchEndAddr)
	{
		if(dir_entry->name[0] == 'R' &&
           dir_entry->name[1] == 'E' &&
           dir_entry->name[2] == 'S' &&
           dir_entry->name[3] == 'E' &&
           dir_entry->name[4] == 'T' &&
           dir_entry->name[5] == 0 &&
           (ROUND_UP(dir_entry->fileSize,16) == offset) )
		{
			romDirInfo->romPtr		= (u32)searchStartAddr;						// start of rom
			romDirInfo->romdirPtr	= dir_entry;							// start of romdir structure
			romDirInfo->extinfoPtr	= (u32)dir_entry + dir_entry[1].fileSize;	// start of extinfo
			return romDirInfo;
		}

		dir_entry++;
		offset += sizeof(ROMDIR_ENTRY);
	}

	// not found
	romDirInfo->romdirPtr = NULL;
	return NULL;
}

// find a file in the romdir table and return info about it
//
// args:	info about romdir to search through
//			filename to search for
//			structure to get info about file into
// returns: a pointer to fileinfo if successful
//			NULL otherwise
ROMFILE_INFO* searchFileInRom(const ROMDIR_INFO* romdirInfo, const char* filename, ROMFILE_INFO* fileinfo)
{
	register ROMDIR_ENTRY* dir_entry;
	register ext_offset=0, file_offset=0;
	int i;

	for (dir_entry = romdirInfo->romdirPtr; dir_entry->name[0]; dir_entry++) {

        for(i = 0; i < 10; ++i) {
            if( filename[i] == 0 )
                break;
            if( dir_entry->name[i] != filename[i] ) {
                i = -1;
                break;
            }
        }

		if (i > 0 ) {
			fileinfo->entry		= dir_entry;
			fileinfo->fileData	= file_offset + romdirInfo->romPtr;	// address of file in rom
			fileinfo->extData	= (u32)NULL;								// address of extinfo in rom

			if (dir_entry->extSize)
				fileinfo->extData = ext_offset + romdirInfo->extinfoPtr;	// address of extinfo in rom
			return fileinfo;
		}

        file_offset += ROUND_UP(dir_entry->fileSize,16);
        ext_offset += dir_entry->extSize;
	}

	// error - file not found
	return NULL;
}

// gets a hex number from *addr and updates the pointer
//
// args:	pointer to string buffer containing a hex number
// returns:	the value of the hex number
u32 getHexNumber(char** addr)
{
	register char *p;		//a1
	register u32   h = 0;	//a2;

	for (p=*addr; *p >= '0'; p++)
	{
		int num;
		if(*p <= '9')		num = *p - '0';
		else if(*p >= 'a')	num = *p - 'a' + 10;
		else				num = *p - 'A' + 10;

		h = h*16 + num;
	}

	*addr = p;
	return h;
}
