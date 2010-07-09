/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "Common.h"

#include "GS.h"			// for sending game crc to mtgs
#include "Elfheader.h"

using namespace std;

u32 ElfCRC;
u32 ElfEntry;
wxString LastELF;

#if 0
// fixme: ELF command line option system.
// It parses a command line and pastes it into PS2 memory, and then points the a0 register at it.
// A user-written ELF can then load the data and respond accordingly.  Needs a rewrite using modern
// string parsing utils. :)

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

static uint parseCommandLine( const wxString& filename )
{
	if ( ( args_ptr != 0xFFFFFFFF ) && ( args_ptr > (4 + 4 + 256) ) )
	{
		const char *p;
		int argc, i, ret = 0;
		u32 args_end;

		args_ptr -= 256;

		args[ 255 ] = 0;

		// Copy the parameters into the section of memory at args_ptr,
		// then zero out anything past the end of args till 256 chars is reached.
		memcpy( &PS2MEM_BASE[ args_ptr ], args, 256 );
		memset( &PS2MEM_BASE[ args_ptr + strlen( args ) ], 0, 256 - strlen( args ) );
		args_end = args_ptr + strlen( args );

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


		args_ptr -= strlen( p ) + 1;

		//fill param 0; i.e. name of the program
		strcpy( (char*)&PS2MEM_BASE[ args_ptr ], p );

		// Start from the end of where we wrote to, not including all the zero'd out area.
		for ( i = args_end - args_ptr + 1, argc = 0; i > 0; i-- )
		{
			while (i && isEmpty(args_ptr + i ))  { i--; }

			// If the last char is a space, set it to 0.
			if ( PS2MEM_BASE[ args_ptr + i + 1 ] == ' ') PS2MEM_BASE[ args_ptr + i + 1 ] = 0;

			while (i && !isEmpty(args_ptr + i )) { i--; }

			// Now that we've gone back a word, increase the number of arguments,
			// and mark the location of the argument.
			if (!isEmpty(args_ptr + i )) // i <= 0
			{
				// If the spot we are on is not a space or null , use it.
				argc++;
				ret = args_ptr - 4 - 4 - argc * 4;

				if (ret < 0 ) return 0;
				((u32*)PS2MEM_BASE)[ args_ptr / 4 - argc ] = args_ptr + i;
			}
			else
			{
				if (!isEmpty(args_ptr + i + 1))
				{
					// Otherwise, use the next character .
					argc++;
					ret = args_ptr - 4 - 4 - argc * 4;

					if (ret < 0 ) return 0;
					((u32*)PS2MEM_BASE)[ args_ptr / 4 - argc ] = args_ptr + i + 1;
				}
			}
		}

		// Pass the number of arguments, and if we have arguments.
		((u32*)PS2MEM_BASE)[ args_ptr /4 - argc - 1 ] = argc;		      //how many args
		((u32*)PS2MEM_BASE)[ args_ptr /4 - argc - 2 ] = ( argc > 0);	//have args?	//not used, cannot be filled at all

		return ret;
	}

	return 0;
}
#endif
//---------------

// All of ElfObjects functions.
ElfObject::ElfObject(const wxString& srcfile, IsoFile isofile)
	: filename( srcfile )

	, data( wxULongLong(isofile.getLength()).GetLo(), L"ELF headers" )
	, header( *(ELF_HEADER*)data.GetPtr() )
	, proghead( NULL )
	, secthead( NULL )
{
	isCdvd = true;
	checkElfSize(data.GetSizeInBytes());
	readIso(isofile);
	initElfHeaders();
}

ElfObject::ElfObject( const wxString& srcfile, uint hdrsize )
	: filename( srcfile )
	, data( wxULongLong(hdrsize).GetLo(), L"ELF headers" )
	, header( *(ELF_HEADER*)data.GetPtr() )
	, proghead( NULL )
	, secthead( NULL )
{
	isCdvd = false;
	checkElfSize(data.GetSizeInBytes());
	readFile();
	initElfHeaders();
}

void ElfObject::initElfHeaders()
{
	Console.WriteLn( L"Initializing Elf: %d bytes", data.GetSizeInBytes());

	if ( header.e_phnum > 0 )
		proghead = (ELF_PHR*)&data[header.e_phoff];

	if ( header.e_shnum > 0 )
		secthead = (ELF_SHR*)&data[header.e_shoff];

	if ( ( header.e_shnum > 0 ) && ( header.e_shentsize != sizeof(ELF_SHR) ) )
		Console.Error( "(ELF) Size of section headers is not standard" );

	if ( ( header.e_phnum > 0 ) && ( header.e_phentsize != sizeof(ELF_PHR) ) )
		Console.Error( "(ELF) Size of program headers is not standard" );

	//getCRC();

	const char* elftype = NULL;
	switch( header.e_type )
	{
		default:
			ELF_LOG( "type:      unknown = %x", header.e_type );
			break;

		case 0x0: elftype = "no file type";	break;
		case 0x1: elftype = "relocatable";	break;
		case 0x2: elftype = "executable";	break;
	}

	if (elftype != NULL) ELF_LOG( "type:      %s", elftype );

	const char* machine = NULL;

	switch(header.e_machine)
	{
		case 1: machine = "AT&T WE 32100";	break;
		case 2: machine = "SPARC";			break;
		case 3: machine = "Intel 80386";	break;
		case 4: machine = "Motorola 68000";	break;
		case 5: machine = "Motorola 88000";	break;
		case 7: machine = "Intel 80860";	break;
		case 8: machine = "mips_rs3000";	break;

		default:
			ELF_LOG( "machine:  unknown = %x", header.e_machine );
			break;
	}

	if (machine != NULL) ELF_LOG( "machine:  %s", machine );

	ELF_LOG("version:   %d",header.e_version);
	ELF_LOG("entry:	 %08x",header.e_entry);
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

	//applyPatches();
}

bool ElfObject::hasProgramHeaders() { return (proghead != NULL); }
bool ElfObject::hasSectionHeaders() { return (secthead != NULL); }
bool ElfObject::hasHeaders() { return (hasProgramHeaders() && hasSectionHeaders()); }

void ElfObject::readIso(IsoFile file)
{
	int rsize = file.read(data.GetPtr(), data.GetSizeInBytes());
	if (rsize < data.GetSizeInBytes()) throw Exception::EndOfStream(filename);
}

void ElfObject::readFile()
{
	int rsize = 0;
	FILE *f;

	f = fopen( filename.ToUTF8(), "rb" );
	if (f == NULL) Exception::FileNotFound( filename );

	fseek(f, 0, SEEK_SET);
	rsize = fread(data.GetPtr(), 1, data.GetSizeInBytes(), f);
	fclose( f );

	if (rsize < data.GetSizeInBytes()) throw Exception::EndOfStream(filename);
}

void ElfObject::checkElfSize(s64 elfsize)
{
	if (elfsize > 0xfffffff)
		throw Exception::BadStream(filename).SetBothMsgs(wxLt("Illegal ELF file size over 2GB!"));

	if (elfsize == -1)
		throw Exception::BadStream(filename).SetBothMsgs(wxLt("ELF file does not exist!"));

	if (elfsize == 0)
		throw Exception::BadStream(filename).SetBothMsgs(wxLt("Unexpected end of ELF file."));
}

u32 ElfObject::getCRC()
{
	u32 CRC = 0;

	const u32* srcdata = (u32*)data.GetPtr();
	for(u32 i=data.GetSizeInBytes()/4; i; --i, ++srcdata)
		CRC ^= *srcdata;

	return CRC;
}

void ElfObject::loadProgramHeaders()
{
	if (proghead == NULL) return;

	for( int i = 0 ; i < header.e_phnum ; i++ )
	{
		ELF_LOG( "Elf32 Program Header" );
		ELF_LOG( "type:      " );

		switch(proghead[ i ].p_type)
		{
			default:
				ELF_LOG( "unknown %x", (int)proghead[ i ].p_type );
				break;

			case 0x1:
			{
				ELF_LOG("load");
				const uint elfsize = data.GetLength();

				if (proghead[ i ].p_offset < elfsize)
				{
					int size;

					if ((proghead[ i ].p_filesz + proghead[ i ].p_offset) > elfsize)
						size = elfsize - proghead[ i ].p_offset;
					else
						size = proghead[ i ].p_filesz;

					if (proghead[ i ].p_vaddr != proghead[ i ].p_paddr)
						Console.Warning( "ElfProgram different load addrs: paddr=0x%8.8x, vaddr=0x%8.8x",
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
		ELF_LOG("offset:    %08x",proghead[i].p_offset);
		ELF_LOG("vaddr:     %08x",proghead[i].p_vaddr);
		ELF_LOG("paddr:     %08x",proghead[i].p_paddr);
		ELF_LOG("file size: %08x",proghead[i].p_filesz);
		ELF_LOG("mem size:  %08x",proghead[i].p_memsz);
		ELF_LOG("flags:     %08x",proghead[i].p_flags);
		ELF_LOG("palign:    %08x",proghead[i].p_align);
		ELF_LOG("\n");
	}
}

void ElfObject::loadSectionHeaders()
{
	if (secthead == NULL || header.e_shoff > (u32)data.GetLength()) return;

	const u8* sections_names = data.GetPtr( secthead[ (header.e_shstrndx == 0xffff ? 0 : header.e_shstrndx) ].sh_offset );

	int i_st = -1, i_dt = -1;

	for( int i = 0 ; i < header.e_shnum ; i++ )
	{
		ELF_LOG( "ELF32 Section Header [%x] %s", i, &sections_names[ secthead[ i ].sh_name ] );

		// used by parseCommandLine
		//if ( secthead[i].sh_flags & 0x2 )
		//	args_ptr = min( args_ptr, secthead[ i ].sh_addr & 0x1ffffff );

		ELF_LOG("\n");

		const char* sectype = NULL;
		switch(secthead[ i ].sh_type)
		{
			case 0x0: sectype = "null";		break;
			case 0x1: sectype = "progbits";	break;
			case 0x2: sectype = "symtab";	break;
			case 0x3: sectype = "strtab";	break;
			case 0x4: sectype = "rela";		break;
			case 0x8: sectype = "no bits";	break;
			case 0x9: sectype = "rel";		break;

			default:
				ELF_LOG("type:      unknown %08x",secthead[i].sh_type);
			break;
		}

		ELF_LOG("type:      %s", sectype);
		ELF_LOG("flags:     %08x", secthead[i].sh_flags);
		ELF_LOG("addr:      %08x", secthead[i].sh_addr);
		ELF_LOG("offset:    %08x", secthead[i].sh_offset);
		ELF_LOG("size:      %08x", secthead[i].sh_size);
		ELF_LOG("link:      %08x", secthead[i].sh_link);
		ELF_LOG("info:      %08x", secthead[i].sh_info);
		ELF_LOG("addralign: %08x", secthead[i].sh_addralign);
		ELF_LOG("entsize:   %08x", secthead[i].sh_entsize);
		// dump symbol table

		if (secthead[ i ].sh_type == 0x02)
		{
			i_st = i;
			i_dt = secthead[i].sh_link;
		}
	}

	if ((i_st >= 0) && (i_dt >= 0))
	{
		const char * SymNames;
		Elf32_Sym * eS;

		SymNames = (char*)data.GetPtr(secthead[i_dt].sh_offset);
		eS = (Elf32_Sym*)data.GetPtr(secthead[i_st].sh_offset);
		Console.WriteLn("found %d symbols", secthead[i_st].sh_size / sizeof(Elf32_Sym));

		for(uint i = 1; i < (secthead[i_st].sh_size / sizeof(Elf32_Sym)); i++) {
			if ((eS[i].st_value != 0) && (ELF32_ST_TYPE(eS[i].st_info) == 2))
			{
				R5900::disR5900AddSym(eS[i].st_value, &SymNames[eS[i].st_name]);
			}
		}
	}
}

void ElfObject::loadHeaders()
{
	loadProgramHeaders();
	loadSectionHeaders();
}

// return value:
//   0 - Invalid or unknown disc.
//   1 - PS1 CD
//   2 - PS2 CD
int GetPS2ElfName( wxString& name )
{
	int retype = 0;

	try {
		IsoFSCDVD isofs;
		IsoFile file( isofs, L"SYSTEM.CNF;1");

		int size = file.getLength();
		if( size == 0 ) return 0;


		while( !file.eof() )
		{
			const wxString original( fromUTF8(file.readLine().c_str()) );
			const ParsedAssignmentString parts( original );

			if( parts.lvalue.IsEmpty() && parts.rvalue.IsEmpty() ) continue;
			if( parts.rvalue.IsEmpty() )
			{
				Console.Warning( "(SYSTEM.CNF) Unusual or malformed entry in SYSTEM.CNF ignored:" );
				Console.Indent().WriteLn( original );
				continue;
			}

			if( parts.lvalue == L"BOOT2" )
			{
				name = parts.rvalue;
				Console.WriteLn( Color_StrongBlue, L"(SYSTEM.CNF) Detected PS2 Disc = " + name );
				retype = 2;
			}
			else if( parts.lvalue == L"BOOT" )
			{
				name = parts.rvalue;
				Console.WriteLn( Color_StrongBlue, L"(SYSTEM.CNF) Detected PSX/PSone Disc = " + name );
				retype = 1;
			}
			else if( parts.lvalue == L"VMODE" )
			{
				Console.WriteLn( Color_Blue, L"(SYSTEM.CNF) Disc region type = " + parts.rvalue );
			}
			else if( parts.lvalue == L"VER" )
			{
				Console.WriteLn( Color_Blue, L"(SYSTEM.CNF) Software version = " + parts.rvalue );
			}
		}

		if( retype == 0 )
		{
			Console.Error("(GetElfName) Disc image is *not* a Playstation or PS2 game!");
			return 0;
		}
	}
	catch (Exception::BadStream& ex)
	{
		Console.Error(ex.FormatDiagnosticMessage());
		return 0;		// ISO error
	}
	catch( Exception::FileNotFound& )
	{
		//Console.Warning(ex.FormatDiagnosticMessage());
		return 0;		// no SYSTEM.CNF, not a PS1/PS2 disc.
	}

#ifdef PCSX2_DEVBUILD
	FILE *fp;
	int i;
	char buffer[512];

	fp = fopen("System.map", "r");
	if( fp == NULL ) return 2;

	u32 addr;

	Console.WriteLn("Loading System.map...");
	while (!feof(fp)) {
		fseek(fp, 8, SEEK_CUR);
		buffer[0] = '0'; buffer[1] = 'x';
		for (i=2; i<10; i++) buffer[i] = fgetc(fp); buffer[i] = 0;
		addr = strtoul(buffer, (char**)NULL, 0);
		fseek(fp, 3, SEEK_CUR);
		for (i=0; i<512; i++) {
			buffer[i] = fgetc(fp);
			if (buffer[i] == '\n' || buffer[i] == 0) break;
		}
		if (buffer[i] == 0) break;
		buffer[i] = 0;

		R5900::disR5900AddSym(addr, buffer);
	}
	fclose(fp);
#endif

	return retype;
}
