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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "CDVD/IsoFSdrv.h"

using namespace std;

#ifdef _MSC_VER
#pragma warning(disable:4996) //ignore the stricmp deprecated warning
#endif

u32 ElfCRC;

struct ELF_HEADER {
	u8	e_ident[16];	//0x7f,"ELF"  (ELF file identifier)
	u16	e_type;		 //ELF type: 0=NONE, 1=REL, 2=EXEC, 3=SHARED, 4=CORE
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
};

struct ELF_PHR {
	u32 p_type;         //see notes1
	u32 p_offset;       //Offset from file start to program segment.
	u32 p_vaddr;        //Virtual address of the segment
	u32 p_paddr;        //Physical address of the segment
	u32 p_filesz;       //Number of bytes in the file image of the segment
	u32 p_memsz;        //Number of bytes in the memory image of the segment
	u32 p_flags;        //Flags for segment
	u32 p_align;        //Alignment. The address of 0x08 and 0x0C must fit this alignment. 0=no alignment
};

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

struct ELF_SHR {
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
};

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

struct Elf32_Sym {
	u32	st_name;
	u32	st_value;
	u32	st_size;
	u8	st_info;
	u8	st_other;
	u16	st_shndx;
};

#define ELF32_ST_TYPE(i) ((i)&0xf)

struct Elf32_Rel {
	u32	r_offset;
	u32	r_info;
};

//2002-09-19 (Florin)
char args[256]="ez.m2v";	//to be accessed by other files
uptr args_ptr;		//a big value; in fact, it is an address

static bool isEmpty(int addr)
{
	return ((PS2MEM_BASE[addr] == 0) || (PS2MEM_BASE[addr] == 32));
}

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

// The results of this function will normally be that it finds an arg at 13 chars, and another at 0.
// It'd probably be easier to 0 out all 256 chars, split args, copy all the arguments in, and note
// the locations of each split...  --arcum42
static uint parseCommandLine( const char *filename )
{
	if ( ( args_ptr != 0xFFFFFFFF ) && ( args_ptr > (4 + 4 + 256) ) )
	{
		const char *p;
		int argc, i, ret = 0;

		args_ptr -= 256;

		args[ 255 ] = 0;
		
		// Copy the parameters into the section of memory at args_ptr, 
		// then zero out anything past the end of args till 256 chars is reached.
		memcpy( &PS2MEM_BASE[ args_ptr ], args, 256 );
		memset( &PS2MEM_BASE[ args_ptr + strlen( args ) ], 0, 256 - strlen( args ) );
		
		// Set p to just the filename, no path.
#ifdef _WIN32
		p = strrchr( filename, '\\' );
#else	//linux
		p = strrchr( filename, '/' );
		if( p == NULL ) p = strchr(filename, '\\');
#endif
		if (p)
			p++;
		else
			p = filename;
		
		//DevCon::WriteLn("parseCommandLine: args = '%s'; p = '%s'", params args, p);
		
		args_ptr -= strlen( p ) + 1;
		
		//fill param 0; i.e. name of the program
		strcpy( (char*)&PS2MEM_BASE[ args_ptr ], p );
		
		// We only filled 256 chars above. Lets not loop for more then that.
		for ( i = /*strlen(p) + 1 + */256, argc = 0; i > 0; i-- )
		{
			// Decrease i until arg_ptr + i points at a spot that is not a space or 0 (or i is 0).
			while (i && isEmpty(args_ptr + i ))  { i--; }
			
			// If the last char is a space, set it to 0.
			if ( PS2MEM_BASE[ args_ptr + i + 1 ] == ' ') PS2MEM_BASE[ args_ptr + i + 1 ] = 0;
			
			// Decrease i until we run into another space or 0 (or i is 0). 
			// (in other words, so far, we went backwards by a word.)
			while (i && !isEmpty(args_ptr + i )) { i--; }
			
			// if the spot we are on is not a space or null (ie, i<=0, given the last while statement):
			if (!isEmpty(args_ptr + i ))
			{
				// Presumably increases the number of arguments, and lets the ps2 know about this argument.
				argc++;
				ret = args_ptr - 4 - 4 - argc * 4;
				
				//DevCon::WriteLn("parseCommandLine: i = %d", params i);
				if (ret < 0 ) return 0;
				((u32*)PS2MEM_BASE)[ args_ptr / 4 - argc ] = args_ptr + i;
				//DevCon::WriteLn("PS2MEM_BASE[%d / 4 - %d (%d)] = %d", params args_ptr, argc, (args_ptr / 4 - argc), (args_ptr + i));
			}
			else
			{
				// If we ran into a word.
				if (!isEmpty(args_ptr + i + 1))
				{
					// Presumably increases the number of arguments, and lets the ps2 know about this argument.
					argc++;
					ret = args_ptr - 4 - 4 - argc * 4;
					
					//DevCon::WriteLn("parseCommandLine: i = %d", params i);
					if (ret < 0 ) return 0;
					((u32*)PS2MEM_BASE)[ args_ptr / 4 - argc ] = args_ptr + i + 1;
					//DevCon::WriteLn("PS2MEM_BASE[%d / 4 - %d (%d)] = %d", params args_ptr, argc, (args_ptr / 4 - argc), (args_ptr + i));
				}
			}
		}
		
		// Pass the number of arguments, and if we have arguments.
		((u32*)PS2MEM_BASE)[ args_ptr /4 - argc - 1 ] = argc;		      //how many args
		((u32*)PS2MEM_BASE)[ args_ptr /4 - argc - 2 ] = ( argc > 0);	//have args?	//not used, cannot be filled at all
		//DevCon::WriteLn("parseCommandLine: argc = %d", params argc);

		return ret;
	}

	return 0;
}
//---------------

struct ElfObject
{
	string filename;
	SafeArray<u8> data;
	ELF_HEADER& header;
	ELF_PHR* proghead;
	ELF_SHR* secthead;

	// Destructor!
	// C++ does all the cleanup automagically for us.
	~ElfObject() { }

	ElfObject( const string& srcfile, uint hdrsize ) :
		filename( srcfile )
	,	data( hdrsize, "ELF headers" )
	,	header( *(ELF_HEADER*)data.GetPtr() )
	,	proghead( NULL )
	,	secthead( NULL )
	{
		readFile();

		if( header.e_phnum > 0 )
			proghead = (ELF_PHR*)&data[header.e_phoff];
			
		if( header.e_shnum > 0 )
			secthead = (ELF_SHR*)&data[header.e_shoff];

		if ( ( header.e_shnum > 0 ) && ( header.e_shentsize != sizeof(ELF_SHR) ) )
			Console::Error( "ElfLoader Warning > Size of section headers is not standard" );

		if ( ( header.e_phnum > 0 ) && ( header.e_phentsize != sizeof(ELF_PHR) ) )
			Console::Error( "ElfLoader Warning > Size of program headers is not standard" );

		ELF_LOG( "type:      " );
		switch( header.e_type ) 
		{
			default: 
				ELF_LOG( "unknown %x", header.e_type );
				break;

			case 0x0:
				ELF_LOG( "no file type" );
				break;

			case 0x1:
				ELF_LOG( "relocatable" );
				break;

			case 0x2: 
				ELF_LOG( "executable" );
				break;
		}  
		ELF_LOG( "\n" );
		ELF_LOG( "machine:   " );
		
		switch ( header.e_machine ) 
		{
			default:
				ELF_LOG( "unknown" );
				break;

			case 0x8:
				ELF_LOG( "mips_rs3000" );
				break;
		}
		
		ELF_LOG("\n");
		ELF_LOG("version:   %d",header.e_version);
		ELF_LOG("entry:     %08x",header.e_entry);
		ELF_LOG("flags:     %08x",header.e_flags);
		ELF_LOG("eh size:   %08x",header.e_ehsize);
		ELF_LOG("ph off:    %08x",header.e_phoff);
		ELF_LOG("ph entsiz: %08x",header.e_phentsize);
		ELF_LOG("ph num:    %08x",header.e_phnum);
		ELF_LOG("sh off:    %08x",header.e_shoff);
		ELF_LOG("sh entsiz: %08x",header.e_shentsize);
		ELF_LOG("sh num:    %08x",header.e_shnum);
		ELF_LOG("sh strndx: %08x",header.e_shstrndx);
		
		ELF_LOG("\n");
	}

	void readFile()
	{
		int rsize = 0;
		if ((strnicmp( filename.c_str(), "cdrom0:", strlen("cdromN:")) == 0) ||
			(strnicmp( filename.c_str(), "cdrom1:", strlen("cdromN:")) == 0))
		{
			int fi = IsoFS_open(filename.c_str() + strlen("cdromN:"), 1);//RDONLY
			
			if (fi < 0) throw Exception::FileNotFound( filename );
			
			IsoFS_lseek( fi, 0, SEEK_SET );
			rsize = IsoFS_read( fi, (char*)data.GetPtr(), data.GetSizeInBytes() );
			IsoFS_close( fi );
		}
		else
		{
			FILE *f;

			f = fopen( filename.c_str(), "rb" );
			if( f == NULL ) Exception::FileNotFound( filename );
			
			fseek( f, 0, SEEK_SET );
			rsize = fread( data.GetPtr(), 1, data.GetSizeInBytes(), f );
			fclose( f );
		}

		if( rsize < data.GetSizeInBytes() ) throw Exception::EndOfStream( filename );
	}

	u32 GetCRC() const
	{
		u32 CRC = 0;

		const  u32* srcdata = (u32*)data.GetPtr();
		for(u32 i=data.GetSizeInBytes()/4; i; --i, ++srcdata)
			CRC ^= *srcdata;

		return CRC;
	}


	void loadProgramHeaders()
	{
		if ( proghead == NULL )
			return;

		for( int i = 0 ; i < header.e_phnum ; i++ )
		{
			ELF_LOG( "Elf32 Program Header" );	
			ELF_LOG( "type:      " );
			
			switch ( proghead[ i ].p_type ) {
				default:
					ELF_LOG( "unknown %x", (int)proghead[ i ].p_type );
				break;

				case 0x1:
				{
					ELF_LOG("load");
					const uint elfsize = data.GetLength();

					if (proghead[ i ].p_offset < elfsize) {
						int size;

						if ((proghead[ i ].p_filesz + proghead[ i ].p_offset) > elfsize)
							size = elfsize - proghead[ i ].p_offset;
						else
							size = proghead[ i ].p_filesz;

						if( proghead[ i ].p_vaddr != proghead[ i ].p_paddr )
							Console::Notice( "ElfProgram different load addrs: paddr=0x%8.8x, vaddr=0x%8.8x", params
								proghead[ i ].p_paddr, proghead[ i ].p_vaddr);

						// used to be paddr
						memcpy_fast(
							&PS2MEM_BASE[proghead[ i ].p_vaddr & 0x1ffffff],
							data.GetPtr(proghead[ i ].p_offset), size
						);

						ELF_LOG("\t*LOADED*");
					}
				}
				break;
			}
			
			ELF_LOG("\n");
			ELF_LOG("offset:    %08x",(int)proghead[i].p_offset);
			ELF_LOG("vaddr:     %08x",(int)proghead[i].p_vaddr);
			ELF_LOG("paddr:     %08x",proghead[i].p_paddr);
			ELF_LOG("file size: %08x",proghead[i].p_filesz);
			ELF_LOG("mem size:  %08x",proghead[i].p_memsz);
			ELF_LOG("flags:     %08x",proghead[i].p_flags);
			ELF_LOG("palign:    %08x",proghead[i].p_align);
			ELF_LOG("\n");
		}
	}

	void loadSectionHeaders() 
	{
		if( secthead == NULL || header.e_shoff > (u32)data.GetLength() )
			return;

		const u8* sections_names = data.GetPtr( secthead[ (header.e_shstrndx == 0xffff ? 0 : header.e_shstrndx) ].sh_offset );
			
		int i_st = -1;
		int i_dt = -1;

		for( int i = 0 ; i < header.e_shnum ; i++ )
		{
			ELF_LOG( "Elf32 Section Header [%x] %s", i, &sections_names[ secthead[ i ].sh_name ] );

			if ( secthead[i].sh_flags & 0x2 )
				args_ptr = min( args_ptr, secthead[ i ].sh_addr & 0x1ffffff );
			
#ifdef PCSX2_DEVBULD
			ELF_LOG("\n");
			ELF_LOG("type:      ");
			
			switch ( secthead[ i ].sh_type )
			{
				case 0x0: ELF_LOG("null"); break;
				case 0x1: ELF_LOG("progbits"); break;
				case 0x2: ELF_LOG("symtab"); break;
				case 0x3: ELF_LOG("strtab"); break;
				case 0x4: ELF_LOG("rela"); break;
				case 0x8: ELF_LOG("no bits"); break;
				case 0x9: ELF_LOG("rel"); break;
				default: ELF_LOG("unknown %08x",secthead[i].sh_type); break;
			}
			
			ELF_LOG("\n");
			ELF_LOG("flags:     %08x", secthead[i].sh_flags);
			ELF_LOG("addr:      %08x", secthead[i].sh_addr);
			ELF_LOG("offset:    %08x", secthead[i].sh_offset);
			ELF_LOG("size:      %08x", secthead[i].sh_size);
			ELF_LOG("link:      %08x", secthead[i].sh_link);
			ELF_LOG("info:      %08x", secthead[i].sh_info);
			ELF_LOG("addralign: %08x", secthead[i].sh_addralign);
			ELF_LOG("entsize:   %08x", secthead[i].sh_entsize);
			// dump symbol table
		
			if( secthead[ i ].sh_type == 0x02 ) 
			{
				i_st = i; 
				i_dt = secthead[i].sh_link; 
			}
#endif
		}

		if( ( i_st >= 0 ) && ( i_dt >= 0 ) )
		{
			const char * SymNames;
			Elf32_Sym * eS;

			SymNames = (char*)data.GetPtr( secthead[ i_dt ].sh_offset );
			eS = (Elf32_Sym*)data.GetPtr( secthead[ i_st ].sh_offset );
			Console::WriteLn("found %d symbols", params secthead[ i_st ].sh_size / sizeof( Elf32_Sym ));

			for( uint i = 1; i < ( secthead[ i_st ].sh_size / sizeof( Elf32_Sym ) ); i++ ) {
				if ( ( eS[ i ].st_value != 0 ) && ( ELF32_ST_TYPE( eS[ i ].st_info ) == 2 ) ) {
					R5900::disR5900AddSym( eS[i].st_value, &SymNames[ eS[ i ].st_name ] );
				}
			}
		}
	}
};

void ElfApplyPatches()
{
	string filename;
	ssprintf( filename, "%8.8x", ElfCRC );

	// if patches found the following status msg will be overwritten
	Console::SetTitle( fmt_string( "Game running [CRC=%hs]", &filename ) );

	if( !Config.Patch ) return;

	if(LoadPatch( filename ) != 0)
	{
		Console::WriteLn("XML Loader returned an error. Trying to load a pnach...");
		inifile_read( filename.c_str() );
	}
	else 
		Console::WriteLn("XML Loading success. Will not load from pnach...");

	applypatch( 0 );
}

// Fetches the CRC of the game bound to the CDVD plugin.
u32 loadElfCRC( const char* filename )
{
	TocEntry toc;

	IsoFS_init( );
	
	Console::Status("loadElfCRC: %s", params filename);
	
	int mylen = strlen( "cdromN:" );
	if ( IsoFS_findFile( filename + mylen, &toc ) == -1 ) return 0;

	DevCon::Status( "loadElfFile: %d bytes", params toc.fileSize );
	u32 crcval = ElfObject( filename, toc.fileSize ).GetCRC();
	Console::Status( "loadElfFile: %s; CRC = %8.8X", params filename, crcval );

	return crcval;
}

int loadElfFile(const char *filename)
{
	// Reset all recompilers prior to initiating a BIOS or new ELF.  The cleaner the
	// slate, the happier the recompiler!

	SysClearExecutionCache();

	if( filename == NULL || filename[0] == 0 )
	{
		Console::Notice( "Running the PS2 BIOS..." );
		return -1;
	}

	// We still need to run the BIOS stub, so that all the EE hardware gets initialized correctly.
	cpuExecuteBios();

	int elfsize;

	Console::Status("loadElfFile: %s", params filename);
	if (strnicmp( filename, "cdrom0:", strlen( "cdromN:" ) ) &&
		strnicmp( filename, "cdrom1:", strlen( "cdromN:" ) ) )
	{
		DevCon::WriteLn("Loading from a file (or non-cd image)");
		struct stat sbuf;
		if ( stat( filename, &sbuf ) != 0 )
			return -1;
		elfsize = sbuf.st_size;
	}
	else
	{
		DevCon::WriteLn("Loading from a CD rom or CD image");
		TocEntry toc;
		IsoFS_init( );
		if ( IsoFS_findFile( filename + strlen( "cdromN:" ), &toc ) == -1 )
			return -1;
		elfsize = toc.fileSize;
	}

	Console::Status( "loadElfFile: %d", params elfsize);
	ElfObject elfobj( filename, elfsize );

	if( elfobj.proghead == NULL )
		throw Exception::CpuStateShutdown( fmt_string( "%s > This ELF has no program headers; Pcsx2 can't run what doesn't exist...", filename ) );

	//2002-09-19 (Florin)
	args_ptr = 0xFFFFFFFF;	//big value, searching for minimum

	elfobj.loadProgramHeaders();
	elfobj.loadSectionHeaders();
	
	cpuRegs.pc = elfobj.header.e_entry; //set pc to proper place 
	ELF_LOG( "PC set to: %8.8lx", cpuRegs.pc );
	
	cpuRegs.GPR.n.sp.UL[0] = 0x81f00000;
	cpuRegs.GPR.n.gp.UL[0] = 0x81f80000; // might not be 100% ok
	cpuRegs.GPR.n.a0.UL[0] = parseCommandLine( filename );

	for ( uint i = 0; i < 0x100000; i++ ) {
		if ( strcmp( "rom0:OSDSYS", (char*)PSM( i ) ) == 0 ) {
			strcpy( (char*)PSM( i ), filename );
			DevCon::Status( "addr %x \"%s\" -> \"%s\"", params i, "rom0:OSDSYS", filename );
		}
	}

	ElfCRC = elfobj.GetCRC();
	Console::Status( "loadElfFile: %s; CRC = %8.8X", params filename, ElfCRC);

	ElfApplyPatches();
	
	return 0;
}

