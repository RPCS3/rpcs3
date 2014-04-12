#pragma once
#include "Loader.h"

struct Elf64_Ehdr
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
	u64 e_entry;
	u64 e_phoff;
	u64 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;

	void Load(vfsStream& f)
	{
		e_magic     = Read32(f);
		e_class     = Read8(f);
		e_data      = Read8(f);
		e_curver    = Read8(f);
		e_os_abi    = Read8(f);
		e_abi_ver   = Read64(f);
		e_type      = Read16(f);
		e_machine   = Read16(f);
		e_version   = Read32(f);
		e_entry     = Read64(f);
		e_phoff     = Read64(f);
		e_shoff     = Read64(f);
		e_flags     = Read32(f);
		e_ehsize    = Read16(f);
		e_phentsize = Read16(f);
		e_phnum     = Read16(f);
		e_shentsize = Read16(f);
		e_shnum     = Read16(f);
		e_shstrndx  = Read16(f);
	}

	void Show()
	{
#ifdef LOADER_DEBUG
		ConLog.Write("Magic: %08x",                           e_magic);
		ConLog.Write("Class: %s",                             "ELF64");
		ConLog.Write("Data: %s",                              Ehdr_DataToString(e_data).c_str());
		ConLog.Write("Current Version: %d",                   e_curver);
		ConLog.Write("OS/ABI: %s",                            Ehdr_OS_ABIToString(e_os_abi).c_str());
		ConLog.Write("ABI version: %lld",                     e_abi_ver);
		ConLog.Write("Type: %s",                              Ehdr_TypeToString(e_type).c_str());
		ConLog.Write("Machine: %s",                           Ehdr_MachineToString(e_machine).c_str());
		ConLog.Write("Version: %d",                           e_version);
		ConLog.Write("Entry point address: 0x%08llx",         e_entry);
		ConLog.Write("Program headers offset: 0x%08llx",      e_phoff);
		ConLog.Write("Section headers offset: 0x%08llx",      e_shoff);
		ConLog.Write("Flags: 0x%x",                           e_flags);
		ConLog.Write("Size of this header: %d",               e_ehsize);
		ConLog.Write("Size of program headers: %d",           e_phentsize);
		ConLog.Write("Number of program headers: %d",         e_phnum);
		ConLog.Write("Size of section headers: %d",           e_shentsize);
		ConLog.Write("Number of section headers: %d",         e_shnum);
		ConLog.Write("Section header string table index: %d", e_shstrndx);
#endif
	}

	bool CheckMagic() const { return e_magic == 0x7F454C46; }
	u32 GetEntry() const { return e_entry; }
};

struct Elf64_Shdr
{
	u32 sh_name; 
	u32 sh_type;
	u64 sh_flags;
	u64 sh_addr;
	u64 sh_offset;
	u64 sh_size;
	u32 sh_link;
	u32 sh_info;
	u64 sh_addralign;
	u64 sh_entsize;

	void Load(vfsStream& f)
	{
		sh_name      = Read32(f);
		sh_type      = Read32(f);
		sh_flags     = Read64(f);
		sh_addr      = Read64(f);
		sh_offset    = Read64(f);
		sh_size      = Read64(f);
		sh_link      = Read32(f);
		sh_info      = Read32(f);
		sh_addralign = Read64(f);
		sh_entsize   = Read64(f);
	}

	void Show()
	{
#ifdef LOADER_DEBUG
		ConLog.Write("Name offset: %x",     sh_name);
		ConLog.Write("Type: %d",            sh_type);
		ConLog.Write("Addr: %llx",          sh_addr);
		ConLog.Write("Offset: %llx",        sh_offset);
		ConLog.Write("Size: %llx",          sh_size);
		ConLog.Write("EntSize: %lld",       sh_entsize);
		ConLog.Write("Flags: %llx",         sh_flags);
		ConLog.Write("Link: %x",            sh_link);
		ConLog.Write("Info: %x",            sh_info);
		ConLog.Write("Address align: %llx", sh_addralign);
#endif
	}
};

struct Elf64_Phdr
{
	u32	p_type;
	u32	p_flags;
	u64	p_offset;
	u64	p_vaddr;
	u64	p_paddr;
	u64	p_filesz;
	u64	p_memsz;
	u64	p_align;

	void Load(vfsStream& f)
	{
		p_type   = Read32(f);
		p_flags  = Read32(f);
		p_offset = Read64(f);
		p_vaddr  = Read64(f);
		p_paddr  = Read64(f);
		p_filesz = Read64(f);
		p_memsz  = Read64(f);
		p_align  = Read64(f);
	}

	void Show()
	{
#ifdef LOADER_DEBUG
		ConLog.Write("Type: %s",                   Phdr_TypeToString(p_type).c_str());
		ConLog.Write("Offset: 0x%08llx",           p_offset);
		ConLog.Write("Virtual address: 0x%08llx",  p_vaddr);
		ConLog.Write("Physical address: 0x%08llx", p_paddr);
		ConLog.Write("File size: 0x%08llx",        p_filesz);
		ConLog.Write("Memory size: 0x%08llx",      p_memsz);
		ConLog.Write("Flags: %s",                  Phdr_FlagsToString(p_flags).c_str());
		ConLog.Write("Align: 0x%llx",              p_align);
#endif
	}
};

class ELF64Loader : public LoaderBase
{
	vfsStream& elf64_f;

public:
	Elf64_Ehdr ehdr;
	std::vector<std::string> shdr_name_arr;
	std::vector<Elf64_Shdr> shdr_arr;
	std::vector<Elf64_Phdr> phdr_arr;

	ELF64Loader(vfsStream& f);
	~ELF64Loader() {Close();}

	virtual bool LoadInfo();
	virtual bool LoadData(u64 offset = 0);
	virtual bool Close();

	bool LoadEhdrInfo(s64 offset=-1);
	bool LoadPhdrInfo(s64 offset=-1);
	bool LoadShdrInfo(s64 offset=-1);

private:
	bool LoadEhdrData(u64 offset);
	bool LoadPhdrData(u64 offset);
	bool LoadShdrData(u64 offset);

	//bool LoadImports();
};

void WriteEhdr(wxFile& f, Elf64_Ehdr& ehdr);
void WritePhdr(wxFile& f, Elf64_Phdr& phdr);
void WriteShdr(wxFile& f, Elf64_Shdr& shdr);
