#pragma once
#include "Loader.h"

struct vfsStream;
class rFile;

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
	u32 e_entry;
	u32 e_phoff;
	u32 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;

	void Show();

	bool IsLittleEndian() const
	{
		return e_data == 1;
	}

	void Load(vfsStream& f);

	bool CheckMagic() const { return e_magic == 0x7F454C46; }
	u32 GetEntry() const { return e_entry; }
};

struct Elf32_Desc
{
	u32 revision;
	u32 ls_size;
	u32 stack_size;
	u32 flags;

	void Load(vfsStream& f);

	void LoadLE(vfsStream& f);
};

struct Elf32_Note
{
	u32 namesz;
	u32 descsz;
	u32 type;
	u8 name[8];
	union
	{
		Elf32_Desc desc;
		char desc_text[32];
	};

	void Load(vfsStream& f);

	void LoadLE(vfsStream& f);
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

	void Load(vfsStream& f);

	void LoadLE(vfsStream& f);

	void Show();
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

	void Load(vfsStream& f);

	void LoadLE(vfsStream& f);

	void Show();
};

class ELF32Loader : public LoaderBase
{
	vfsStream& elf32_f;

public:
	Elf32_Ehdr ehdr;
	std::vector<std::string> shdr_name_arr;
	std::vector<Elf32_Shdr> shdr_arr;
	std::vector<Elf32_Phdr> phdr_arr;

	ELF32Loader(vfsStream& f);
	~ELF32Loader() {Close();}

	virtual bool LoadInfo();
	virtual bool LoadData(u64 offset);
	virtual bool Close();

private:
	bool LoadEhdrInfo();
	bool LoadPhdrInfo();
	bool LoadShdrInfo();

	bool LoadEhdrData(u64 offset);
	bool LoadPhdrData(u64 offset);
	bool LoadShdrData(u64 offset);
};

void WriteEhdr(rFile& f, Elf32_Ehdr& ehdr);
void WritePhdr(rFile& f, Elf32_Phdr& phdr);
void WriteShdr(rFile& f, Elf32_Shdr& shdr);
