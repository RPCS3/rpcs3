#ifndef __ROMDIR_H__
#define __ROMDIR_H__

#include <tamtypes.h>

// an entry in a romdir table. a romdir table is an array of these.
// gets filled in by searchRomDir()
typedef struct romdir_entry {
	char	name[10];		//+00
	short	extSize;		//+0A
	int		fileSize;		//+0C
} __attribute__ ((packed)) ROMDIR_ENTRY;


// info about a file in a romdir
// gets filled in by searchFileInRom()
typedef struct {
	ROMDIR_ENTRY*	entry;	// pointer to the file's ROMDIR_ENTRY
	u32		fileData;		// address of file contents
	u32		extData;		// address of file's ext info
} ROMFILE_INFO;

// info about the location of a romdir table
typedef struct {
	u32		romPtr;			//+00
	ROMDIR_ENTRY* romdirPtr;//+04
	u32		extinfoPtr;		//+08
} ROMDIR_INFO;

typedef struct romfs {
	char*	filename;		//+00
	int		fd;				//+04
	int		size;			//+08
	ROMDIR_INFO romdirInfo;	//+0C
} ROMFS;

// rounds off a value to the next multiple
#define ROUND_UP(num, multiple)	(((num)+multiple-1) & (~(multiple-1)))

// searches between beginning and end addresses for a romdir structure.
// if found it returns info about it in romDirInfo.
// 
// args:	address to start searching from
//			address to stop searching at
//			gets filled in with info if found
// returns:	a pointer to the filled in romDirInfo if successful
//			NULL if error
extern ROMDIR_INFO* searchRomDir(const u32* searchStartAddr, const u32* searchEndAddr, ROMDIR_INFO* romDirInfo);

// find a file in the romdir table and return info about it
// 
// args:	info about romdir to search through
//			filename to search for
//			structure to get info about file into
// returns: a pointer to fileinfo if successful
//			NULL otherwise
extern ROMFILE_INFO* searchFileInRom(const ROMDIR_INFO* romdirInfo, const char* filename, ROMFILE_INFO* fileinfo);

// gets a hex number from *addr and updates the pointer
// 
// args:	pointer to string buffer containing a hex number
// returns:	the value of the hex number
extern u32 getHexNumber(char** addr);

/*
IOPBTCONF file format
=====================
each line is one of this:

1. loading address of IOP kernel
Format:  @<HEXNUMBER>
Example: @800
Default: 0
Description: starting with <HEXNUMBER> (or 0 if not specified) the loader will
 place modules in memory on 256 boundaries at +0x30 (the first 48 bytes are 
 reserved for info about the following module)

2. address of function to be called while loading of IOP kernel
Format: !addr <HEXNUMBER>
Description: the code indicated by <HEXNUMBER> will be run on kernel loading

3. name of (parent) included IOPBTCONF
Format: !include <NAME>
Description: allow to have another file with same format to be loaded
 recursively; support for this is limited because of the BUGGY parsing;
Note: you can have such option only at the begining of IOPBTCONF and the
 inclusion can be made without getting stucked in recursion like this:

ioprp1.img contains IOPBTCONF1 (!include IOPBTCON2) and IOPBTCONF11
ioprp2.img contains IOPBTCONF2 (!include IOPBTCONF1)
ioprp.img  contains IOPBTCONF  (!include IOPBTCONF2<EOL>!include IOPBTCONF11)
rom0       contains IOPBTCONF and IOPBTCON2

udnl cdrom0:ioprp1.img cdrom0:ioprp2.img cdrom0:ioprp.img

the starting of the chain can be named IOPBTCONF only
also you can include only from the previous romz

4. comment
Format: #<TEXT>
Example: #APPLOAD
Description: you can have comments on a line that starts with #

5. modules that have to be loaded after a reset
Format: <MODULENAME>
Example: SYSMEM
Description: each line of IOPBTCONF usualy have a module name; the order the
 modules appear in the list is the order they are loaded as IOP kernel
--------------------------------
Notes:
 - each line ends with <EOL>, that is 0x0A
 - in the final loading list the first 2 positions must be SYSMEM and LOADCORE
*/

#endif /* __ROMDIR_H__ */
