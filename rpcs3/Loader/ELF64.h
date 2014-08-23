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

	void Load(vfsStream& f);

	void Show();

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

	void Load(vfsStream& f);

	void Show();
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

	void Load(vfsStream& f);

	void Show();
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

void WriteEhdr(rFile& f, Elf64_Ehdr& ehdr);
void WritePhdr(rFile& f, Elf64_Phdr& phdr);
void WriteShdr(rFile& f, Elf64_Shdr& shdr);
