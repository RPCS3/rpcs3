#include "romdir.h"
#include "iopelf.h"

typedef struct {
	u32	st_name;
	u32	st_value;
	u32	st_size;
	u8	st_info;
	u8	st_other;
	u16	st_shndx;
} Elf32_Sym;

char *sections_names;
	
ELF_HEADER *elfHeader;	
ELF_PHR	*elfProgH;
ELF_SHR	*elfSectH;
u8 *elfdata;
int elfsize;
u32 elfbase;
static int debug=1;

#define _dprintf(fmt, args...) \
	if (debug > 0) __printf("iopelf: " fmt, ## args)

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
						_dprintf("loading program at %x, size=%x\n", elfProgH[ i ].p_paddr + elfbase, size);
						__memcpy((void*)(elfProgH[ i ].p_paddr + elfbase), 
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

void _relocateElfSection(int i) {
	ELF_REL *rel;
	int size = elfSectH[i].sh_size / 4;
	int r = 0;
	u32 *ptr, *tmp;
	ELF_SHR	*rsec = &elfSectH[elfSectH[i].sh_info];
	u8 *mem = (u8*)(elfSectH[i].sh_addr + elfbase);
	u32 imm;
	int j;

	ptr = (u32*)(elfdata+elfSectH[i].sh_offset);

//	_dprintf("relocating section %s\n", &sections_names[rsec->sh_name]);
//	__printf("sh_addr %x\n", elfSectH[i].sh_addr);

	while (size > 0) {
		rel = (ELF_REL*)&ptr[r];
//		__printf("rel size=%x: offset=%x, info=%x\n", size, rel->r_offset, rel->r_info);

		tmp = (u32*)&mem[rel->r_offset];
		switch ((u8)rel->r_info) {
			case 2: // R_MIPS_32
				*tmp+= elfbase; break;

			case 4: // R_MIPS_26
				*tmp = (*tmp & 0xfc000000) | 
					   (((*tmp & 0x03ffffff) + (elfbase >> 2)) & 0x03ffffff);
				break;

			case 5: // R_MIPS_HI16
				imm = (((*tmp & 0xffff) + (elfbase >> 16)) & 0xffff);
				for (j=(r+2)/2; j<elfSectH[i].sh_size / 4; j++) {
					if (j*2 == r) continue;
					if ((u8)((ELF_REL*)&ptr[j*2])->r_info == 6)
						break;
//					if ((rel->r_info >> 8) == (((ELF_REL*)&ptr[j*2])->r_info >> 8))
//						break;
				}

/*				if (j != elfSectH[i].sh_size / 4)*/ {
					u32 *p;

					rel = (ELF_REL*)&ptr[j*2];
//					__printf("HI16: found match: %x\n", rel->r_offset);
					p = (u32*)&mem[rel->r_offset];
//					__printf("%x + %x = %x\n", *p, elfbase, (*p & 0xffff) + (elfbase & 0xffff));
					if (((*p & 0xffff) + (elfbase & 0xffff)) & 0x8000) {
//						__printf("found\n");
						imm++;
					}
				}
				*tmp = (*tmp & 0xffff0000) | imm;
				break;

			case 6: // R_MIPS_LO16
				*tmp = (*tmp & 0xffff0000) | 
					   (((*tmp & 0xffff) + (elfbase & 0xffff)) & 0xffff);
				break;

			default:
				__printf("UNKNOWN R_MIPS REL!!\n");
				break;
		}

		size-= 2; r+= 2;
	}
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
	}


	// now that we have all the stuff loaded, relocate it
	for (i = 0 ; i < elfHeader->e_shnum ; i++) {
		if (elfSectH[i].sh_type == 0x09) { // relocations
			_relocateElfSection(i);
		}
	}

	return 0;
}

void* loadElfFile(ROMFILE_INFO* ri, u32 offset)
{
    imageInfo* ii;
    ELF_PHR* ph;
    ELF_IOPMOD* im;

	__printf("loadElfFile: base=%x, size=%x\n", ri->fileData, ri->entry->fileSize);
	elfdata = (u8*)(ri->fileData);
	elfsize = ri->entry->fileSize;
	elfbase = offset+0x30;

	loadHeaders();

    // fill the image info header
    ph= (ELF_PHR*)((char*)elfHeader +elfHeader->e_phoff);
    im= (ELF_IOPMOD*)((char*)elfHeader + ph[0].p_offset);
    ii = (imageInfo*)offset;

    if( *(u16*)(elfHeader->e_ident+4) != 0x101 )
        return NULL;
    if (elfHeader->e_machine != EM_MIPS)
        return NULL;
    if (elfHeader->e_phentsize != sizeof(ELF_PHR))
        return NULL;
    if (elfHeader->e_phnum != 2)
        return NULL;
    if (ph[0].p_type != PT_SCE_IOPMOD)
        return NULL;
    if (elfHeader->e_type != ET_SCE_IOPRELEXEC){
        if (elfHeader->e_type != elfHeader->e_phnum )//ET_EXEC)
            return NULL;
        //result->type=3;
    }
    //else result->type=4;

    ii->next	=0;
	ii->name	=NULL;
	ii->version	=0;
	ii->flags	=0;
	ii->modid	=0;
	if ((int)im->moduleinfo != -1) {
        moduleInfo* minfo = (moduleInfo*)(im->moduleinfo+ph[1].p_vaddr); // probably wrong
		ii->name	= minfo->name;
		ii->version	= minfo->version;
	}
    else {
        ii->name = NULL;
        ii->version = 0;
    }
	ii->entry = im->entry;
	ii->gp_value = im->gp_value;
	ii->p1_vaddr = ph[1].p_vaddr;
	ii->text_size = im->text_size;
	ii->data_size = im->data_size;
	ii->bss_size = im->bss_size;

	loadProgramHeaders();
	loadSectionHeaders();
	
	_dprintf("loadElfFile: e_entry=%x, hdr=%x\n", elfHeader->e_entry, elfHeader);
	return (void*)(elfbase+elfHeader->e_entry);
}

