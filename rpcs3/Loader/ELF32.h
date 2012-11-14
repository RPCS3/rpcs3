#pragma once
#include "Loader.h"

struct Elf32_Ehdr
{
	u32 e_magic;
	u8  e_class;
	u8  e_data;
	u8  e_curver;
	u8  e_os_abi;
	u64 e_abi_ver;
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u16 e_entry;
	u32 e_phoff;
	u32 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;

	void Show()
	{
#ifdef LOADER_DEBUG
		ConLog.Write("Magic: %08x",								e_magic);
		ConLog.Write("Class: %s",								"ELF32");
		ConLog.Write("Data: %s",								Ehdr_DataToString(e_data));
		ConLog.Write("Current Version: %d",						e_curver);
		ConLog.Write("OS/ABI: %s",								Ehdr_OS_ABIToString(e_os_abi));
		ConLog.Write("ABI version: %lld",						e_abi_ver);
		ConLog.Write("Type: %s",								Ehdr_TypeToString(e_type));
		ConLog.Write("Machine: %s",								Ehdr_MachineToString(e_machine));
		ConLog.Write("Version: %d",								e_version);
		ConLog.Write("Entry point address: 0x%x",				e_entry);
		ConLog.Write("Program headers offset: 0x%08x",			e_phoff);
		ConLog.Write("Section headers offset: 0x%08x",			e_shoff);
		ConLog.Write("Flags: 0x%x",								e_flags);
		ConLog.Write("Size of this header: %d",					e_ehsize);
		ConLog.Write("Size of program headers: %d",				e_phentsize);
		ConLog.Write("Number of program headers: %d",			e_phnum);
		ConLog.Write("Size of section headers: %d",				e_shentsize);
		ConLog.Write("Number of section headers: %d",			e_shnum);
		ConLog.Write("Section header string table index: %d",	e_shstrndx);
#endif
	}

	void Load(wxFile& f)
	{
		e_magic		= Read32(f);
		e_class		= Read8(f);
		e_data		= Read8(f);
		e_curver	= Read8(f);
		e_os_abi	= Read8(f);
		e_abi_ver	= Read64(f);
		e_type		= Read16(f);
		e_machine	= Read16(f);
		e_version	= Read32(f);
		e_entry		= Read32(f);
		e_phoff		= Read32(f);
		e_shoff		= Read32(f);
		e_flags		= Read32(f);
		e_ehsize	= Read16(f);
		e_phentsize = Read16(f);
		e_phnum		= Read16(f);
		e_shentsize = Read16(f);
		e_shnum		= Read16(f);
		e_shstrndx  = Read16(f);
	}

	bool CheckMagic() const { return e_magic == 0x7F454C46; }
	u32 GetEntry() const { return e_entry; }
};

struct Elf32_Shdr
{
	u32 sh_name; 
	u32 sh_type;
	u32 sh_flags;
	u32 sh_addr;
	u32 sh_offset;
	u32 sh_size;
	u32 sh_link;
	u32 sh_info;
	u32 sh_addralign;
	u32 sh_entsize;

	void Load(wxFile& f)
	{
		sh_name			= Read32(f);
		sh_type			= Read32(f);
		sh_flags		= Read32(f);
		sh_addr			= Read32(f);
		sh_offset		= Read32(f);
		sh_size			= Read32(f);
		sh_link			= Read32(f);
		sh_info			= Read32(f);
		sh_addralign	= Read32(f);
		sh_entsize		= Read32(f);
	}

	void Show()
	{
#ifdef LOADER_DEBUG
		ConLog.Write("Name offset: %x",		sh_name);
		ConLog.Write("Type: %d",			sh_type);
		ConLog.Write("Addr: %x",			sh_addr);
		ConLog.Write("Offset: %x",			sh_offset);
		ConLog.Write("Size: %x",			sh_size);
		ConLog.Write("EntSize: %d",			sh_entsize);
		ConLog.Write("Flags: %x",			sh_flags);
		ConLog.Write("Link: %x",			sh_link);
		ConLog.Write("Info: %d",			sh_info);
		ConLog.Write("Address align: %x",	sh_addralign);
#endif
	}
};

struct Elf32_Phdr
{
	u32	p_type;
	u32	p_offset;
	u32	p_vaddr;
	u32	p_paddr;
	u32	p_filesz;
	u32	p_memsz;
	u32	p_flags;
	u32	p_align;

	void Load(wxFile& f)
	{
		p_type		= Read32(f);
		p_offset	= Read32(f);
		p_vaddr		= Read32(f);
		p_paddr		= Read32(f);
		p_filesz	= Read32(f);
		p_memsz		= Read32(f);
		p_flags		= Read32(f);
		p_align		= Read32(f);
	}

	void Show()
	{
#ifdef LOADER_DEBUG
		ConLog.Write("Type: %s",					Phdr_TypeToString(p_type));
		ConLog.Write("Offset: 0x%08x",				p_offset);
		ConLog.Write("Virtual address: 0x%08x",		p_vaddr);
		ConLog.Write("Physical address: 0x%08x",	p_paddr);
		ConLog.Write("File size: 0x%08x",			p_filesz);
		ConLog.Write("Memory size: 0x%08x",			p_memsz);
		ConLog.Write("Flags: %s",					Phdr_FlagsToString(p_flags));
		ConLog.Write("Align: 0x%x",					p_align);
#endif
	}
};

class ELF32Loader : public LoaderBase
{
	wxFile& elf32_f;

public:
	Elf32_Ehdr ehdr;
	wxArrayString shdr_name_arr;
	Array<Elf32_Shdr> shdr_arr;
	Array<Elf32_Phdr> phdr_arr;

	ELF32Loader(wxFile& f);
	ELF32Loader(const wxString& path);
	~ELF32Loader() {Close();}

	virtual bool LoadInfo();
	virtual bool LoadData();
	virtual bool Close();

private:
	bool LoadEhdrInfo();
	bool LoadPhdrInfo();
	bool LoadShdrInfo();
	bool LoadEhdrData();
	bool LoadPhdrData();
	bool LoadShdrData();
};