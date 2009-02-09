#ifndef __IOPELF_H__
#define __IOPELF_H__

#include "romdir.h"

#define MOD_TYPE_REL	1
#define MOD_TYPE_2		2
#define MOD_TYPE_DYN	3
#define MOD_TYPE_CORE	4

typedef struct {
	char	*name;
	short	version;
} moduleInfo;

// info about a module file
typedef struct module_info {
	int		type;			// module type (MOD_TYPE_*)
	int		(*entry)(void*);// module entry point address
	int		gpValue;		// module gp value
	int		progVAddr;		// programs virtual address
	int		textSize;		// size of text section
	int		dataSize;		// size of data section
	int		bssSize;		// size of bss section
	int		progMemSize;	// size of program memory
	int		moduleInfo;		// info about module ?
} MODULE_INFO;

typedef struct {
	u32		ei_magic;
	u8		ei_class;
	u8		ei_data;
	u8		ei_version;
	u8		ei_pad[9];
} E_IDENT;

typedef struct {
	u8	e_ident[16];   	//+00 0x7f,"ELF"  (ELF file identifier), E_IDENT*
	u16	e_type;        	//+10 ELF type: 0=NONE, 1=REL, 2=EXEC, 3=SHARED, 4=CORE
	u16	e_machine;      //+12 Processor: 8=MIPS R3000
	u32	e_version;      //+14 Version: 1=current
	u32	e_entry;        //+18 Entry point address
	u32	e_phoff;        //+1C Start of program headers (offset from file start)
	u32	e_shoff;        //+20 Start of section headers (offset from file start)
	u32	e_flags;        //+24 Processor specific flags = 0x20924001 noreorder, mips
	u16	e_ehsize;       //+28 ELF header size (0x34 = 52 bytes)
	u16	e_phentsize;    //+2A Program headers entry size 
	u16	e_phnum;        //+2C Number of program headers
	u16	e_shentsize;    //+2E Section headers entry size
	u16	e_shnum;        //+30 Number of section headers
	u16	e_shstrndx;     //+32 Section header stringtable index	
} ELF_HEADER;			//+34=sizeof
#define ET_SCE_IOPRELEXEC	0xFF80
#define PT_SCE_IOPMOD		0x70000080
#define SHT_REL			9
#define EM_MIPS 8

typedef struct tag_COFF_AOUTHDR {
	short	magic;		//+00
	short	vstamp;		//+02
	int	tsize;		//+04
	int	dsize;		//+08
	int	bsize;		//+0C
	int	entry;		//+10
	int	text_start;	//+14
	int	data_start;	//+18
	int	bss_start;	//+1C
	int	field_20;	//+20
	int	field_24;	//+24
	int	field_28;	//+28
	int	field_2C;	//+2C
	int	moduleinfo;	//+30
	int	gp_value;	//+34
} COFF_AOUTHDR, COHDR;		//=38

typedef struct tag_CHDR{
	short	f_magic;	//+00
	short	f_nscns;	//+02
	int	f_timdat;	//+04
	int	f_symptr;	//+08
	int	f_nsyms;	//+0C
	short	f_opthdr;	//+10
	short	f_flags;	//+12
} CHDR;				//=14

typedef struct tag_COFF_HEADER{		//same header as above
	short	f_magic;	//+00
	short	f_nscns;	//+02
	int	f_timdat;	//+04
	int	f_symptr;	//+08
	int	f_nsyms;	//+0C
	short	f_opthdr;	//+10
	short	f_flags;	//+12
	COFF_AOUTHDR	opthdr;	//+14
} COFF_HEADER;			//=4C

typedef struct tag_COFF_scnhdr
{
	char	s_name[8];	//+00
	int	s_paddr;	//+08
	int	s_vaddr;	//+0C
	int	s_size;		//+10
	int	s_scnptr;	//+14
	int	s_relptr;	//+18
	int	s_lnnoptr;	//+1C
	short	s_nreloc;	//+20
	short	s_nlnno;	//+22
	int	s_flags;	//+24
} COFF_scnhdr, SHDR;		//=28

typedef struct _fileInfo
{
	u32			type,		//+00
				entry,		//+04
				gp_value,	//+08
				p1_vaddr,	//+0C
				text_size,	//+10
				data_size,	//+14
				bss_size,	//+18
				p1_memsz;	//+1C
	moduleInfo	*moduleinfo;	//+20
} fileInfo;

typedef struct {
	u32 p_type;         //+00 see notes1
	u32 p_offset;       //+04 Offset from file start to program segment.
	u32 p_vaddr;        //+08 Virtual address of the segment
	u32 p_paddr;        //+0C Physical address of the segment
	u32 p_filesz;       //+10 Number of bytes in the file image of the segment
	u32 p_memsz;        //+14 Number of bytes in the memory image of the segment
	u32 p_flags;        //+18 Flags for segment
	u32 p_align;        //+1C Alignment. The address of 0x08 and 0x0C must fit this alignment. 0=no alignment
} ELF_PHR;

//notes1
//------
//0=Inactive
//1=Load the segment into memory, no. of bytes specified by 0x10 and 0x14
//2=Dynamic linking
//3=Interpreter. The array element must specify a path name
//4=Note. The array element must specify the location and size of aux. info
//5=reserved
//6=The array element must specify location and size of the program header table.

typedef struct {
    u32 r_offset;
    u32 r_info;
} ELF_REL;

typedef struct {
    char* moduleinfo;
    u32 entry; //+04
    u32 gp_value; //+08
    u32 text_size; //+0C
    u32 data_size; //+10
    u32 bss_size;//+14
    short moduleversion;//+18
    char* modulename;   //+1A
} ELF_IOPMOD;

typedef struct {
	u32	sh_name;        //+00 No. to the index of the Section header stringtable index
    u32	sh_type;        //+04 See notes2
	u32	sh_flags;       //+08 see notes3
	u32	sh_addr;        //+0C Section start address
	u32	sh_offset;      //+10 Offset from start of file to section
	u32	sh_size;        //+14 Size of section
	u32	sh_link;        //+18 Section header table index link
	u32	sh_info;        //+1C Info
	u32	sh_addralign;   //+20 Alignment. The adress of 0x0C must fit this alignment. 0=no alignment.
	u32	sh_entsize;     //+24 Fixed size entries.
} ELF_SHR;

//notes 2
//-------
//Type:
//0=Inactive
//1=PROGBITS
//2=SYMTAB symbol table
//3=STRTAB string table
//4=RELA relocation entries
//5=HASH hash table
//6=DYNAMIC dynamic linking information
//7=NOTE
//8=NOBITS 
//9=REL relocation entries
//10=SHLIB
//0x70000000=LOPROC processor specifc
//0x7fffffff=HIPROC
//0x80000000=LOUSER lower bound
//0xffffffff=HIUSER upper bound
//
//notes 3
//-------
//Section Flags:  (1 bit, you may combine them like 3 = alloc & write permission)
//1=Write section contains data the is be writeable during execution.
//2=Alloc section occupies memory during execution
//4=Exec section contains executable instructions
//0xf0000000=Mask bits processor-specific


#define ELF32_ST_TYPE(i) ((i)&0xf)

// special header for every iop module?
typedef struct {
	u32 next;		//+00
	char	*name;		//+04
	short	version,	//+08
		flags,		//+0A
		modid,		//+0C
		HE;		//+0E
	u32	entry,		//+10
		gp_value,	//+14
		p1_vaddr,	//+18
		text_size,	//+1C
		data_size,	//+20
		bss_size,	//+24
		H28,		//+28
		H2C;		//+2C
} imageInfo;

// pass in the memory to load the elf file from
// elfoffset - offset to load elf file to
void* loadElfFile(ROMFILE_INFO* ri, u32 elfoffset);

#endif
