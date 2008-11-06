/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	//2002-09-28 (Florin)
#include <sys/stat.h>

#include "Common.h"
#include "CDVDisodrv.h"

#ifdef _MSC_VER
#pragma warning(disable:4996) //ignore the stricmp deprecated warning
#endif

u32 ElfCRC;

typedef struct {
    u8	e_ident[16];    //0x7f,"ELF"  (ELF file identifier)
    u16	e_type;         //ELF type: 0=NONE, 1=REL, 2=EXEC, 3=SHARED, 4=CORE
	u16	e_machine;      //Processor: 8=MIPS R3000
	u32	e_version;      //Version: 1=current
	u32	e_entry;        //Entry point address
	u32	e_phoff;        //Start of program headers (offset from file start)
	u32	e_shoff;        //Start of section headers (offset from file start)
	u32	e_flags;        //Processor specific flags = 0x20924001 noreorder, mips
	u16	e_ehsize;       //ELF header size (0x34 = 52 bytes)
	u16	e_phentsize;    //Program headers entry size 
	u16	e_phnum;        //Number of program headers
	u16	e_shentsize;    //Section headers entry size
	u16	e_shnum;        //Number of section headers
	u16	e_shstrndx;     //Section header stringtable index	
} ELF_HEADER;

typedef struct {
	u32 p_type;         //see notes1
	u32 p_offset;       //Offset from file start to program segment.
	u32 p_vaddr;        //Virtual address of the segment
	u32 p_paddr;        //Physical address of the segment
	u32 p_filesz;       //Number of bytes in the file image of the segment
	u32 p_memsz;        //Number of bytes in the memory image of the segment
	u32 p_flags;        //Flags for segment
	u32 p_align;        //Alignment. The address of 0x08 and 0x0C must fit this alignment. 0=no alignment
} ELF_PHR;

/*
notes1
------
0=Inactive
1=Load the segment into memory, no. of bytes specified by 0x10 and 0x14
2=Dynamic linking
3=Interpreter. The array element must specify a path name
4=Note. The array element must specify the location and size of aux. info
5=reserved
6=The array element must specify location and size of the program header table.
*/

typedef struct {
	u32	sh_name;        //No. to the index of the Section header stringtable index
    u32	sh_type;        //See notes2
	u32	sh_flags;       //see notes3
	u32	sh_addr;        //Section start address
	u32	sh_offset;      //Offset from start of file to section
	u32	sh_size;        //Size of section
	u32	sh_link;        //Section header table index link
	u32	sh_info;        //Info
	u32	sh_addralign;   //Alignment. The adress of 0x0C must fit this alignment. 0=no alignment.
	u32	sh_entsize;     //Fixed size entries.
} ELF_SHR;
/*
notes 2
-------
Type:
0=Inactive
1=PROGBITS
2=SYMTAB symbol table
3=STRTAB string table
4=RELA relocation entries
5=HASH hash table
6=DYNAMIC dynamic linking information
7=NOTE
8=NOBITS 
9=REL relocation entries
10=SHLIB
0x70000000=LOPROC processor specifc
0x7fffffff=HIPROC
0x80000000=LOUSER lower bound
0xffffffff=HIUSER upper bound

notes 3
-------
Section Flags:  (1 bit, you may combine them like 3 = alloc & write permission)
1=Write section contains data the is be writeable during execution.
2=Alloc section occupies memory during execution
4=Exec section contains executable instructions
0xf0000000=Mask bits processor-specific
*/

typedef struct {
	u32	st_name;
	u32	st_value;
	u32	st_size;
	u8	st_info;
	u8	st_other;
	u16	st_shndx;
} Elf32_Sym;

#define ELF32_ST_TYPE(i) ((i)&0xf)

typedef struct {
	u32	r_offset;
	u32	r_info;
} Elf32_Rel;

//unfinished!!!!

char *sections_names;
	
ELF_HEADER *elfHeader;	
ELF_PHR	*elfProgH;
ELF_SHR	*elfSectH;
char *name;
u8 *elfdata;
u32 elfsize;

struct stat sbuf;
struct TocEntry toc;

//2002-09-19 (Florin)
char args[256]="ez.m2v";	//to be accessed by other files
unsigned int args_ptr;		//a big value; in fact, it is an address

//in a0 is passed the address of the command line args,
//i.e. a pointer to an area like this:
//+00 unknown/unused
//+04 argc; number of arguments
//+08 argv[0]; address of the first parameter - program name (char*) -
//+08 argv[1]; address of the second parameter (char*)                |
//+0C argv[2]; and so on											  |
//........															  |
//+08+4*argc the program name(first param)						   <--
//+08+4*argc+strlen(argv[0]+1) the rest of params; i.e. a copy of 'args'
//                                         see above 'char args[256];'
unsigned int parseCommandLine( char *filename )
{
	if ( ( args_ptr != 0xFFFFFFFF ) && ( args_ptr > 264 ) )
   {	// 4 + 4 + 256
		char * p;
		int  argc, 
           i;

		args_ptr -= 256;
      if ( args_ptr < 0 )
      {
         return 0;
      }
		args[ 255 ] = 0;
		memcpy( &PS2MEM_BASE[ args_ptr ], args, 256 );				//params 1, 2, etc copied
		memset( &PS2MEM_BASE[ args_ptr + strlen( args ) ], 0, 256 - strlen( args ) );
#ifdef _WIN32
		p = strrchr( filename, '\\' );
#else	//linux
		p = strrchr( filename, '/' );
        if( p == NULL )
            p = strchr(filename, '\\');
#endif
		if ( p )
      {
         p++;
      }
		else
      {
         p = filename;
      }
		args_ptr -= strlen( p ) + 1;
      if ( args_ptr < 0 )
      {
         return 0;
      }
		strcpy( (char*)&PS2MEM_BASE[ args_ptr ], p );						//fill param 0; i.e. name of the program

		for ( i = strlen( p ) + 1 + 256, argc = 0; i > 0; i-- )
      {
			while ( i && ( ( PS2MEM_BASE[ args_ptr + i ] == 0 ) || ( PS2MEM_BASE[ args_ptr + i ] == 32 ) ) )
         {
            i--;
         }
			if ( PS2MEM_BASE[ args_ptr + i + 1 ] == ' ' )
         {
            PS2MEM_BASE[ args_ptr + i + 1 ] = 0;
         }
			while ( i && ( PS2MEM_BASE[ args_ptr + i ] != 0 ) && ( PS2MEM_BASE[ args_ptr + i] != 32 ) )
         {
            i--;
         }
			if ( ( PS2MEM_BASE[ args_ptr + i ] != 0 ) && ( PS2MEM_BASE[ args_ptr + i ] != 32  ) )
         {	//i==0
				argc++;
				if ( args_ptr - 4 - 4 - argc * 4 < 0 )
            {
               return 0;
            }
				((u32*)PS2MEM_BASE)[ args_ptr / 4 - argc ] = args_ptr + i;
			}
         else
         {
				if ( ( PS2MEM_BASE[ args_ptr + i + 1 ] != 0 ) && ( PS2MEM_BASE[ args_ptr + i + 1 ] != 32 ) )
            {
					argc++;
					if ( args_ptr - 4 - 4 - argc * 4 < 0 )
               {
                  return 0;
               }
					((u32*)PS2MEM_BASE)[ args_ptr / 4 - argc ] = args_ptr + i + 1;
				}
         }
		}
		((u32*)PS2MEM_BASE)[ args_ptr /4 - argc - 1 ] = argc;		      //how many args
		((u32*)PS2MEM_BASE)[ args_ptr /4 - argc - 2 ] = ( argc > 0);	//have args?	//not used, cannot be filled at all

		return ( args_ptr - argc * 4 - 8 );
	}

	return 0;
}
//---------------

int readFile( char *Exepath, char *ptr, u32 offset, int size ) {
	FILE *f;
	int fi;

	if ((strnicmp( Exepath, "cdrom0:", strlen("cdrom0:")) == 0) ||
		(strnicmp( Exepath, "cdrom1:", strlen("cdrom0:")) == 0)) {
		if ((u32)offset >= toc.fileSize) return -1;
		fi = CDVDFS_open(Exepath + strlen("cdromN:"), 1);//RDONLY
		if (fi < 0) return -1;
		CDVDFS_lseek(fi, offset, SEEK_SET);
		size = CDVDFS_read(fi, ptr, size);
		CDVDFS_close(fi );
	} else {
		f = fopen(Exepath, "rb");
		if (f == NULL) return -1;
		if ( offset >= (u64)sbuf.st_size) return -1;
		fseek(f, offset, SEEK_SET);
		size = fread(ptr, 1, size, f);
		fclose(f);
	}	

	return size;
}

int loadHeaders( char *Exepath ) {
	elfHeader = (ELF_HEADER*)elfdata;

	if ( ( elfHeader->e_shentsize != sizeof(ELF_SHR) ) && ( elfHeader->e_shnum > 0 ) ) {
		SysMessage( "size of section header not standard\n" );
	}
	
	if((elfHeader->e_shnum * elfHeader->e_shentsize) != 0) {
		elfSectH = (ELF_SHR *) malloc( elfHeader->e_shnum * elfHeader->e_shentsize );
	} else {
		elfSectH = NULL;
	}

	if ( ( elfHeader->e_phnum * elfHeader->e_phentsize ) != 0 ) {
		elfProgH = (ELF_PHR *) malloc( elfHeader->e_phnum * elfHeader->e_phentsize );
	} else {
		elfProgH = NULL;
	}

#ifdef ELF_LOG 
    ELF_LOG( "type:      " );
#endif
	switch( elfHeader->e_type ) 
   {
		default:
#ifdef ELF_LOG        
		   ELF_LOG( "unknown %x", elfHeader->e_type );
#endif
			break;

		case 0x0:
#ifdef ELF_LOG        
			ELF_LOG( "no file type" );
#endif
			break;

		case 0x1:
#ifdef ELF_LOG        
		   ELF_LOG( "relocatable" );
#endif
			break;

		case 0x2:
#ifdef ELF_LOG        
		   ELF_LOG( "executable" );
#endif
			break;
	}
#ifdef ELF_LOG        
	ELF_LOG( "\n" );
	ELF_LOG( "machine:   " );
#endif
	switch ( elfHeader->e_machine ) 
   {
		default:
#ifdef ELF_LOG        
			ELF_LOG( "unknown" );
#endif
			break;

		case 0x8:
#ifdef ELF_LOG        
			ELF_LOG( "mips_rs3000" );
#endif
			break;
	}
#ifdef ELF_LOG  
	ELF_LOG("\n");
	ELF_LOG("version:   %d\n",elfHeader->e_version);
	ELF_LOG("entry:     %08x\n",elfHeader->e_entry);
	ELF_LOG("flags:     %08x\n",elfHeader->e_flags);
	ELF_LOG("eh size:   %08x\n",elfHeader->e_ehsize);
	ELF_LOG("ph off:    %08x\n",elfHeader->e_phoff);
	ELF_LOG("ph entsiz: %08x\n",elfHeader->e_phentsize);
	ELF_LOG("ph num:    %08x\n",elfHeader->e_phnum);
	ELF_LOG("sh off:    %08x\n",elfHeader->e_shoff);
	ELF_LOG("sh entsiz: %08x\n",elfHeader->e_shentsize);
	ELF_LOG("sh num:    %08x\n",elfHeader->e_shnum);
	ELF_LOG("sh strndx: %08x\n",elfHeader->e_shstrndx);
	
	ELF_LOG("\n");
#endif 	

	return TRUE;
}


BOOL loadProgramHeaders( char *Exepath ) 
{
	int i;

	if ( elfHeader->e_phnum == 0 )
   {
      return TRUE;
   }

	// is this critical, or warning?
	if ( elfHeader->e_phentsize != sizeof( ELF_PHR ) )
   {
		SysMessage( "size of program header not standard\n" );
   }

	elfProgH = (ELF_PHR*)&elfdata[elfHeader->e_phoff];
		
	for ( i = 0 ; i < elfHeader->e_phnum ; i++ ) {
#ifdef ELF_LOG  
      ELF_LOG( "Elf32 Program Header\n" );	
		ELF_LOG( "type:      " );
#endif
		switch ( elfProgH[ i ].p_type ) {
			default:
#ifdef ELF_LOG  
				ELF_LOG( "unknown %x", (int)elfProgH[ i ].p_type );
#endif
				break;

			case 0x1:
#ifdef ELF_LOG  
				ELF_LOG("load");
#endif
				if (elfProgH[ i ].p_offset < elfsize) {
					int size;

					if ((elfProgH[ i ].p_filesz + elfProgH[ i ].p_offset) > elfsize) {
						size = elfsize - elfProgH[ i ].p_offset;
					} else {
						size = elfProgH[ i ].p_filesz;
					}

                    if( elfProgH[ i ].p_vaddr != elfProgH[ i ].p_paddr )
                        SysPrintf("ElfProgram different load addrs: paddr=0x%8.8x, vaddr=0x%8.8x\n", elfProgH[ i ].p_paddr, elfProgH[ i ].p_vaddr);
                    // used to be paddr
					memcpy(&PS2MEM_BASE[elfProgH[ i ].p_vaddr & 0x1ffffff], 
						   &elfdata[elfProgH[ i ].p_offset],
						   size);
#ifdef ELF_LOG  
					ELF_LOG("\t*LOADED*");
#endif
				}
				break;
		}
#ifdef ELF_LOG  
      ELF_LOG("\n");
		ELF_LOG("offset:    %08x\n",(int)elfProgH[i].p_offset);
		ELF_LOG("vaddr:     %08x\n",(int)elfProgH[i].p_vaddr);
		ELF_LOG("paddr:     %08x\n",elfProgH[i].p_paddr);
		ELF_LOG("file size: %08x\n",elfProgH[i].p_filesz);
		ELF_LOG("mem size:  %08x\n",elfProgH[i].p_memsz);
		ELF_LOG("flags:     %08x\n",elfProgH[i].p_flags);
		ELF_LOG("palign:    %08x\n",elfProgH[i].p_align);
		ELF_LOG("\n");
#endif
	}

	return TRUE;
}


BOOL loadSectionHeaders( char * Exepath ) 
{
	int i;
	int i_st = -1;
	int i_dt = -1;

	if (elfHeader->e_shnum == 0 ||
		elfHeader->e_shoff > elfsize) {
		return TRUE;
	}

	elfSectH = (ELF_SHR*)&elfdata[elfHeader->e_shoff];
	
	if ( elfHeader->e_shstrndx < elfHeader->e_shnum ) {
		sections_names = (char *)&elfdata[elfSectH[ elfHeader->e_shstrndx ].sh_offset];
	}
		
	for ( i = 0 ; i < elfHeader->e_shnum ; i++ ) {
#ifdef ELF_LOG  
		ELF_LOG( "Elf32 Section Header [%x] %s", i, &sections_names[ elfSectH[ i ].sh_name ] );
#endif
		if ( elfSectH[i].sh_flags & 0x2 ) {  
			//2002-09-19 (Florin)
			args_ptr = min( args_ptr, elfSectH[ i ].sh_addr & 0x1ffffff );
			//---------------
/*			if (elfSectH[i].sh_offset < elfsize) {
				int size;

				if ((elfSectH[i].sh_size + elfSectH[i].sh_offset) > elfsize) {
					size = elfsize - elfSectH[i].sh_offset;
				} else {
					size = elfSectH[i].sh_size;
				}
				memcpy(&PS2MEM_BASE[ elfSectH[ i ].sh_addr &0x1ffffff ],
					   &elfdata[elfSectH[i].sh_offset],
					   size);
			}
#ifdef ELF_LOG  
			ELF_LOG( "\t*LOADED*" );
#endif*/
		}
#ifdef ELF_LOG  
      ELF_LOG("\n");
		ELF_LOG("type:      ");
#endif
		switch ( elfSectH[ i ].sh_type )
      {
			default:
#ifdef ELF_LOG  
	         ELF_LOG("unknown %08x",elfSectH[i].sh_type);
#endif	          
				break;

			case 0x0:
#ifdef ELF_LOG  
	         ELF_LOG("null");
#endif
				break;

			case 0x1:
#ifdef ELF_LOG  
	         ELF_LOG("progbits");
#endif
				break;

			case 0x2:
#ifdef ELF_LOG  
	         ELF_LOG("symtab");
#endif
				break;

			case 0x3:
#ifdef ELF_LOG  
	         ELF_LOG("strtab");
#endif
				break;

         case 0x4:
#ifdef ELF_LOG  
	         ELF_LOG("rela");
#endif
				break;

			case 0x8:
#ifdef ELF_LOG  
	         ELF_LOG("no bits");
#endif
				break;

         case 0x9:
#ifdef ELF_LOG  
	         ELF_LOG("rel");
#endif
				break;
		}
#ifdef ELF_LOG  
      ELF_LOG("\n");
		ELF_LOG("flags:     %08x\n", elfSectH[i].sh_flags);
		ELF_LOG("addr:      %08x\n", elfSectH[i].sh_addr);
		ELF_LOG("offset:    %08x\n", elfSectH[i].sh_offset);
		ELF_LOG("size:      %08x\n", elfSectH[i].sh_size);
		ELF_LOG("link:      %08x\n", elfSectH[i].sh_link);
		ELF_LOG("info:      %08x\n", elfSectH[i].sh_info);
		ELF_LOG("addralign: %08x\n", elfSectH[i].sh_addralign);
		ELF_LOG("entsize:   %08x\n", elfSectH[i].sh_entsize);
#endif			
		// dump symbol table
	
		if ( elfSectH[ i ].sh_type == 0x02 ) 
      {
			i_st = i; 
         i_dt = elfSectH[i].sh_link; 
      }
   }

	if ( ( i_st >= 0 ) && ( i_dt >= 0 ) )
   {
		char * SymNames;
		Elf32_Sym * eS;

		SymNames = (char*)&elfdata[elfSectH[ i_dt ].sh_offset];
		eS = (Elf32_Sym*)&elfdata[elfSectH[ i_st ].sh_offset];
		SysPrintf("found %d symbols\n", elfSectH[ i_st ].sh_size / sizeof( Elf32_Sym ));

		for ( i = 1; i < (int)( elfSectH[ i_st ].sh_size / sizeof( Elf32_Sym ) ); i++ ) {
			if ( ( eS[ i ].st_value != 0 ) && ( ELF32_ST_TYPE( eS[ i ].st_info ) == 2 ) ) {
//				SysPrintf("%x:%s\n", eS[i].st_value, &SymNames[eS[i].st_name]);
				disR5900AddSym( eS[i].st_value, &SymNames[ eS[ i ].st_name ] );
/*				if (!strcmp(&SymNames[eS[i].st_name], "sceSifCheckStatRpc")) {
					psMu32(eS[i].st_value & 0x1ffffff) = (0x3b << 26) | 1;
					SysPrintf("found sceSifCheckStatRpc!!\n");
				}*/
			}
		}
	}

	return TRUE;
}

extern int LoadPatch(char *patchfile);
extern void LoadGameSpecificSettings();

int loadElfFile(char *filename) {
	char str[256];
	char str2[256];
	u32 crc;
	u32 i;

	SysPrintf("loadElfFile: %s\n", filename);
	if (strnicmp( filename, "cdrom0:", strlen( "cdrom0:" ) ) &&
		strnicmp( filename, "cdrom1:", strlen( "cdrom1:" ) ) ) {
		if ( stat( filename, &sbuf ) != 0 )
			return -1;
		elfsize = sbuf.st_size;
	} else {
		CDVDFS_init( );
		if ( CDVD_findfile( filename + strlen( "cdromN:" ), &toc ) == -1 )
			return -1;
		elfsize = toc.fileSize;
	}

	SysPrintf("loadElfFile: %d\n", elfsize);
	elfdata = (u8*)malloc(elfsize);
	if (elfdata == NULL) return -1;
	readFile(filename, (char*)elfdata, 0, elfsize);

/*	{
		FILE *f = fopen("game.elf", "wb");
		fwrite(elfdata, 1, elfsize, f);
		fclose(f);
	}*/

	//2002-09-19 (Florin)
	args_ptr = 0xFFFFFFFF;	//big value, searching for minimum
	//-------------------
	loadHeaders( filename );
	cpuRegs.pc = elfHeader->e_entry; //set pc to proper place 
	loadProgramHeaders( filename );
	loadSectionHeaders( filename );
	
#ifdef ELF_LOG
	ELF_LOG( "PC set to: %8.8lx\n", cpuRegs.pc );
#endif
	cpuRegs.GPR.n.sp.UL[0] = 0x81f00000;
	cpuRegs.GPR.n.gp.UL[0] = 0x81f80000; // might not be 100% ok

	//2002-09-19 (Florin)
	cpuRegs.GPR.n.a0.UL[0] = parseCommandLine( filename );
	//---------------

	for ( i = 0; i < 0x100000; i++ ) {
		if ( strcmp( "rom0:OSDSYS", (char*)PSM( i ) ) == 0 ) {
			strcpy( (char*)PSM( i ), filename );
			SysPrintf( "addr %x \"%s\" -> \"%s\"\n", i, "rom0:OSDSYS", filename );
		}
	}

	//CRC
	for (i=0, crc=0; i<elfsize/4; i++) {
		crc^= ((u32*)elfdata)[i];
	}
	ElfCRC = crc;

	SysPrintf("loadElfFile: %s; CRC = %8.8X\n", filename, crc);

   // Applying patches
    if (Config.Patch) {
		sprintf(str, "%8.8x", crc);
#ifdef _WIN32
		sprintf(str2,"No patch found.Game will run normally. [CRC=%8.8x]",crc);//if patches found it will overwritten :p
		if (gApp.hConsole) SetConsoleTitle(str2);
#endif
		if(LoadPatch(str)!=0)
		{
			SysPrintf("XML Loader returned an error. Trying to load a pnach...\n");
			inifile_read(str);
		}
		else SysPrintf("XML Loading success. Will not load from pnach...\n");
		applypatch( 0 );
	}

	free(elfdata);

	LoadGameSpecificSettings();
	return 0;
}

#include "VU.h"
extern int g_FFXHack;
extern int path3hack;
int g_VUGameFixes = 0;
void LoadGameSpecificSettings()
{
    // default
    g_VUGameFixes = 0;
    g_FFXHack = 0;

    switch(ElfCRC) {
        case 0x0c414549: // spacefisherman, missing gfx
		    g_VUGameFixes |= VUFIX_SIGNEDZERO;
            break;
        case 0x4C9EE7DF: // crazy taxi (u)
        case 0xC9C145BF: // crazy taxi, missing gfx
            g_VUGameFixes |= VUFIX_EXTRAFLAGS;
            break;

        case 0xb99379b7: // erementar gerad (discolored chars)
            g_VUGameFixes |= VUFIX_XGKICKDELAY2;
            break;
		case 0xa08c4057:  //Sprint Cars (SLUS)
		case 0x8b0725d5:  //Flinstones Bedrock Racing (SLES)
			path3hack = 1;
			break;

        case 0x6a4efe60: // ffx(j)
		case 0xA39517AB: // ffx(e)
		case 0xBB3D833A: // ffx(u)
		case 0x941bb7d9: // ffx(g)
		case 0xD9FC6310: // ffx int(j)
        case 0xa39517ae: // ffx(f)
        case 0xa39517a9: // ffx(i)
        case 0x658597e2: // ffx int
        case 0x941BB7DE: // ffx(s)
        case 0x3866CA7E: // ffx(asia)
        case 0x48FE0C71: // ffx2 (u)
		case 0x9aac530d: // ffx2 (g)
		case 0x9AAC5309: // ffx2 (e)
		case 0x8A6D7F14: // ffx2 (j)
        case 0x9AAC530B: // ffx2 (i)
        case 0x9AAC530A: // ffx2 (f)
        case 0xe1fd9a2d: // ffx2 last mission (?)
        case 0x93f9b89a: // ffx2 demo (g)
        case 0x304C115C: // harvest moon - awl
		case 0xF0A6D880: // harvest moon - sth
            g_FFXHack = 1;
            break;		
	}
}
