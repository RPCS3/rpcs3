// 
// iopboot.c
// 
// this is the c code for the iopboot file in the ps2 rom.
// this file is located at 0xBFC4A000 in the ps2 bios rom0.
// modload.irx also from the ps2 bios executes this direct from the rom
// (no loading to ram first)
// 
// this is based on florin's disasm and converted to c code by xorloser and zerofrog
// 

#include <tamtypes.h>
#include <stdio.h>

#include "iopload.h"
#include "iopdebug.h"
#include "iopelf.h"
#include "romdir.h"
#include "kloadcore.h"

static void kstrcpy(char* dst, const char* src);
static int kstrlen(const char* src);

//BOOT_PARAMS boot_params;
//u32* next_free_address[0x40]; // up to 64 modules
    

// this is the start point of execution at 0xBFC4A000
// 
// it loads the IOPBTCONF module list from rom0 and compiles a
// list of modules and their addresses.
// 
// this list is then passed to loadcore as it is executed in order
// to then load the rest of the modules
// 
// args:	total size of IOP ram in MegaBytes
//			bootinfo flags
//			string containing the reboot image filepath
//			? doesnt seem to be used
void _start(int ramMBSize, int bootInfo, char* udnlString, int unk)
{
	ROMFS ri;
	void *(*sysmem_entry)(u32 iopmemsize);
	void (*loadcore_entry)(BOOT_PARAMS *init);
	int i;
    ROMDIR_INFO romdir_info;
    ROMFILE_INFO romfile_info;
	char conf_filename[10];
    int ram_byte_size, num_lines;
    u32 module_load_addr;
    u32** modules_ptr;
    char* file_data_ptr, *file_data_end;
    void* psysmemstart;
    BOOT_PARAMS* boot_params;

    if( ramMBSize <= 2 )
        ram_byte_size = 2;
    else
        ram_byte_size = ramMBSize;
	ram_byte_size <<= 20;
    
    // compile module list to send to loadcore
    boot_params = (BOOT_PARAMS*)0x30000; // random address, has to be clear before loadcore call
	boot_params->ramMBSize	= ramMBSize;
	boot_params->bootInfo		= bootInfo;
	boot_params->udnlString	= NULL;
	boot_params->moduleAddrs	= (u32**)((u32)boot_params + sizeof(BOOT_PARAMS)); // right after

    // if a undl string is specified, get a copy of it and store a pointer to it
	if(udnlString)
	{
		boot_params->udnlString = (char*)boot_params->moduleAddrs;
		kstrcpy(boot_params->udnlString, udnlString);
		boot_params->moduleAddrs = (u32**)((u32)boot_params->udnlString + ROUND_UP(kstrlen(udnlString) + 8, 4));
	}

    // find the romdir table in the rom
    if( searchRomDir((u32*)0xBFC00000, (u32*)0xBFC10000, &romdir_info) == NULL )
	{
        __printf("IOPBOOT: failed to find start of rom!\n");
		// error - cant find romdir!
		while(1) *(u8*)0x80000000 = 0;
	}

    // find the bootconf file in the romdir table
    kstrcpy(conf_filename, "IOPBTCONF");
	conf_filename[8] = '0' + bootInfo;
	if( !searchFileInRom(&romdir_info, conf_filename, &romfile_info) )
	{
		kstrcpy(conf_filename, "IOPBTCONF");
		if( !searchFileInRom(&romdir_info, conf_filename, &romfile_info) )
		{
            __printf("IOPBTCONF file not found!\n");
			// error - cant find conf file!
			while(1) *(u8*)0x80000000 = 1;
		}
	}

    // count the number of lines in conf file
    file_data_ptr = (char*)romfile_info.fileData;
    file_data_end = (char*)romfile_info.fileData + romfile_info.entry->fileSize;    
    {
        num_lines = 0;
        while( file_data_ptr < file_data_end ) {
            // loop until a "newline" charcter is found
            while(file_data_ptr < file_data_end) {
                if(*file_data_ptr++ < ' ')
                    break;
            }
            
            // loop until a "non-newline" charcter is found
            while(file_data_ptr < file_data_end) {
                if(*file_data_ptr++ >= ' ')
                    break;
            }
            
            num_lines++;
        }
        num_lines++;
    }

    // get the addresses of each module
    {
        module_load_addr = 0;
        boot_params->numConfLines = num_lines-1;
        modules_ptr = boot_params->moduleAddrs;
        char* file_data_ptr = (char*)romfile_info.fileData;
        while( file_data_ptr < file_data_end ) {
            if(*file_data_ptr == '@') {
                file_data_ptr++;
                module_load_addr = getHexNumber(&file_data_ptr);
            }
            else if(*file_data_ptr == '!') {
                if( file_data_ptr[1] == 'a' &&
                    file_data_ptr[2] == 'd' &&
                    file_data_ptr[3] == 'd' &&
                    file_data_ptr[4] == 'r' &&
                    file_data_ptr[5] == ' ' ) {
                    file_data_ptr += 6;
                    *modules_ptr++ = (u32*)(getHexNumber(&file_data_ptr) * 4 + 1);
                    *modules_ptr++ = 0;
                }
            }
            else if(*file_data_ptr != '#') {
                // 'file_data_ptr' should be pointing to a filename
                // this finds the address of that file in the rom
                ROMFILE_INFO module_fileinfo;
                char strmodule[16];
                for(i = 0; i < 16; ++i) {
                    if( file_data_ptr[i] < ' ' )
                        break;
                    strmodule[i] = file_data_ptr[i];
                }
                strmodule[i] = 0;
                
                if( searchFileInRom(&romdir_info, strmodule, &module_fileinfo) == NULL ) {
                    __printf("IOPBOOT: failed to find %s module\n", strmodule);
                    return;
                }

                //__printf("mod: %s:%x\n", strmodule, module_fileinfo.fileData);
                                
                *modules_ptr++ = (u32*)module_fileinfo.fileData;
                *modules_ptr = 0; // don't increment
            }
            
            // loop until a "newline" charcter is found
            while(file_data_ptr < file_data_end) {
                if(*file_data_ptr++ < ' ')
                    break;
            }
            
            // loop until a "non-newline" charcter is found
            while(file_data_ptr < file_data_end) {
                if(*file_data_ptr >= ' ')
                    break;
                file_data_ptr++;
            }
        }
    }

    if( searchFileInRom(&romdir_info, "IOPBOOT", &romfile_info) == NULL ) {
        __printf("loadElfFile: failed to find IOPBOOT module\n");
        return;
    }

    // load sysmem module to memory and execute it
    if( searchFileInRom(&romdir_info, "SYSMEM", &romfile_info) == NULL ) {
        __printf("loadElfFile: failed to find SYSMEM module\n");
        return;
    }
    sysmem_entry = (void *(*)(u32))loadElfFile(&romfile_info, module_load_addr);
    if( sysmem_entry == 0 )
        return;

    psysmemstart = sysmem_entry(ram_byte_size);
    //FlushIcache();
    if( psysmemstart == 0 ) {
        __printf("IOPBOOT: sysmem failed\n");
        return;
    }

    __printf("SYSMEM success, start addr: %x, alloc start: %x\n", module_load_addr, psysmemstart);
    
    if( searchFileInRom(&romdir_info, "LOADCORE", &romfile_info) == NULL ) {
        __printf("loadElfFile: failed to find SYSMEM module\n");
        return;
    }
    loadcore_entry = (void (*)())loadElfFile(&romfile_info, (u32)psysmemstart);
    if( loadcore_entry == 0 )
        return;
    
    boot_params->firstModuleAddr = (u32)module_load_addr + 0x30; // skip elf?
    if(0x1FC10000 < ram_byte_size) {
        boot_params->pos = 0x1FC00000;
		boot_params->size = 0x10100;
	}
	else {
        boot_params->pos = 0;
        boot_params->size = 0;
	}

	__printf("executing LOADCORE entry at %p\n", loadcore_entry);
	loadcore_entry(boot_params);

	__printf("iopboot error\n");

    // error - loadcore shouldnt ever return
	while(1) *(u8*)0x80000000 = 2;
}

void Kmemcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n) {
		*d++ = *s++; n--;
	}
}

static void kstrcpy(char* dst, const char* src)
{
    while(*src) *dst++ = *src++;
    *dst = 0;
}

static int kstrlen(const char* src)
{
    int len = 0;
    while(*src++) len++;
    return len;
}
