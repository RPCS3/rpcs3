#include "romdir.h"
#include "eedebug.h"

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

char *sections_names;
	
ELF_HEADER *elfHeader;	
ELF_PHR	*elfProgH;
ELF_SHR	*elfSectH;
u8 *elfdata;
int elfsize;


static void __memcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n) {
		*d++ = *s++; n--;
	}
}


int loadHeaders() {
	elfHeader = (ELF_HEADER*)elfdata;

	if ((elfHeader->e_shentsize != sizeof(ELF_SHR)) && (elfHeader->e_shnum > 0)) {
		return -1;
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

	return 0;
}


int loadProgramHeaders() {
	int i;

	if (elfHeader->e_phnum == 0) {
		return 0;
	}

	if (elfHeader->e_phentsize != sizeof(ELF_PHR)) {
		return -1;
	}

	elfProgH = (ELF_PHR*)&elfdata[elfHeader->e_phoff];
		
	for ( i = 0 ; i < elfHeader->e_phnum ; i++ ) 
   {
#ifdef ELF_LOG  
      ELF_LOG( "Elf32 Program Header\n" );	
		ELF_LOG( "type:      " );
#endif
		switch ( elfProgH[ i ].p_type ) 
      {
			default:
#ifdef ELF_LOG  
	         ELF_LOG( "unknown %x", (int)elfProgH[ i ].p_type );
#endif
				break;

			case 0x1:
#ifdef ELF_LOG  
	         ELF_LOG("load");
#endif
/*				if ( elfHeader->e_shnum == 0 ) {*/
					if (elfProgH[ i ].p_offset < elfsize) {
						int size;

						if ((elfProgH[ i ].p_filesz + elfProgH[ i ].p_offset) > elfsize) {
							size = elfsize - elfProgH[ i ].p_offset;
						} else {
							size = elfProgH[ i ].p_filesz;
						}
//						__printf("loading program to %x\n", elfProgH[ i ].p_paddr + elfbase);
						__memcpy(elfProgH[ i ].p_paddr, 
								 &elfdata[elfProgH[ i ].p_offset],
								 size);
					}
#ifdef ELF_LOG  
					ELF_LOG("\t*LOADED*");
#endif
//				}
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

	return 0;
}

int loadSectionHeaders() {
	int i;
	int i_st = -1;
	int i_dt = -1;

	if (elfHeader->e_shnum == 0) {
		return -1;
	}

	elfSectH = (ELF_SHR*)&elfdata[elfHeader->e_shoff];
	
	if ( elfHeader->e_shstrndx < elfHeader->e_shnum ) {
		sections_names = (char *)&elfdata[elfSectH[ elfHeader->e_shstrndx ].sh_offset];
	}
		
	for ( i = 0 ; i < elfHeader->e_shnum ; i++ )
   {
#ifdef ELF_LOG  
      ELF_LOG( "Elf32 Section Header [%x] %s", i, &sections_names[ elfSectH[ i ].sh_name ] );
#endif
/*		if ( elfSectH[i].sh_flags & 0x2 ) {
			if (elfSectH[i].sh_offset < elfsize) {
				int size;

				if ((elfSectH[i].sh_size + elfSectH[i].sh_offset) > elfsize) {
					size = elfsize - elfSectH[i].sh_offset;
				} else {
					size = elfSectH[i].sh_size;
				}
				memcpy(&psM[ elfSectH[ i ].sh_addr &0x1ffffff ],
					   &elfdata[elfSectH[i].sh_offset],
					   size);
			}
#ifdef ELF_LOG  
         ELF_LOG( "\t*LOADED*" );
#endif
		}*/
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
	
		if (elfSectH[i].sh_type == 0x02) {
			i_st = i; i_dt = elfSectH[i].sh_link;
		}
/*
		if (elfSectH[i].sh_type == 0x01) {
			int size = elfSectH[i].sh_size / 4;
			u32 *ptr = (u32*)&psxM[(elfSectH[i].sh_addr + irx_addr) & 0x1fffff];

			while (size) {

				if (*ptr == 0x41e00000) { // import func
					int ret = iopSetImportFunc(ptr+1);
					size-= ret; ptr+= ret;
				}

				if (*ptr == 0x41c00000) { // export func
					int ret = iopSetExportFunc(ptr+1);
					size-= ret; ptr+= ret;
				}

				size--; ptr++;
			}
		}
*/
/*		if (!strcmp(".data", &sections_names[elfSectH[i].sh_name])) {
			// seems so..

			psxRegs.GPR.n.gp = 0x8000 + irx_addr + elfSectH[i].sh_addr;
		}*/
	}

	return 0;
}


u32 loadElfFile(char *filename, struct elfinfo *info) {
	struct rominfo ri;
	char str[256];
	char str2[256];
	int i;

	__printf("loadElfFile: %s\n", filename);

	if (romdirGetFile(filename, &ri) == NULL) {
		__printf("file %s not found!!\n", filename);
		return -1;
	}
	elfdata = (u8*)(0xbfc00000 + ri.fileOffset);
	elfsize = ri.fileSize;

	loadHeaders();
	loadProgramHeaders();
	loadSectionHeaders();

//	__printf("loadElfFile: e_entry=%x\n", elfHeader->e_entry);
	return elfHeader->e_entry;
}

