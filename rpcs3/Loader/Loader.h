#pragma once

struct vfsFileBase;
class rFile;

#ifdef _DEBUG	
	//#define LOADER_DEBUG
#endif

enum Elf_Machine
{
	MACHINE_Unknown,
	MACHINE_MIPS = 0x08,
	MACHINE_PPC64 = 0x15,
	MACHINE_SPU = 0x17,
	MACHINE_ARM = 0x28,
};

enum ShdrType
{
	SHT_NULL,
	SHT_PROGBITS,
	SHT_SYMTAB,
	SHT_STRTAB,
	SHT_RELA,
	SHT_HASH,
	SHT_DYNAMIC,
	SHT_NOTE,
	SHT_NOBITS,
	SHT_REL,
	SHT_SHLIB,
	SHT_DYNSYM,
};

enum ShdrFlag
{
	SHF_WRITE     = 0x1,
	SHF_ALLOC     = 0x2,
	SHF_EXECINSTR = 0x4,
	SHF_MASKPROC  = 0xf0000000,
};

const std::string Ehdr_DataToString(const u8 data);
const std::string Ehdr_TypeToString(const u16 type);
const std::string Ehdr_OS_ABIToString(const u8 os_abi);
const std::string Ehdr_MachineToString(const u16 machine);
const std::string Phdr_FlagsToString(u32 flags);
const std::string Phdr_TypeToString(const u32 type);

struct sys_process_param_info
{
	be_t<u32> sdk_version;
	be_t<s32> primary_prio;
	be_t<u32> primary_stacksize;
	be_t<u32> malloc_pagesize;
	be_t<u32> ppc_seg;
	//be_t<u32> crash_dump_param_addr;
};

struct sys_process_param
{
	be_t<u32> size;
	be_t<u32> magic;
	be_t<u32> version;
	sys_process_param_info info;
};

struct sys_proc_prx_param
{
	be_t<u32> size;
	be_t<u32> magic;
	be_t<u32> version;
	be_t<u32> pad0;
	be_t<u32> libentstart;
	be_t<u32> libentend;
	be_t<u32> libstubstart;
	be_t<u32> libstubend;
	be_t<u16> ver;
	be_t<u16> pad1;
	be_t<u32> pad2;
};

struct Elf64_StubHeader
{
	u8 s_size; // = 0x2c
	u8 s_unk0;
	be_t<u16> s_version; // = 0x1
	be_t<u16> s_unk1; // = 0x9 // flags?
	be_t<u16> s_imports;
	be_t<u32> s_unk2; // = 0x0
	be_t<u32> s_unk3; // = 0x0
	be_t<u32> s_modulename;
	be_t<u32> s_nid;
	be_t<u32> s_text;
	be_t<u32> s_unk4; // = 0x0
	be_t<u32> s_unk5; // = 0x0
	be_t<u32> s_unk6; // = 0x0
	be_t<u32> s_unk7; // = 0x0
};

class LoaderBase
{
protected:
	u32 entry;
	u32 min_addr;
	u32 max_addr;
	Elf_Machine machine;

	LoaderBase()
		: machine(MACHINE_Unknown)
		, entry(0)
		, min_addr(0)
		, max_addr(0)
	{
	}

public:
	virtual ~LoaderBase() = default;

	virtual bool LoadInfo() { return false; }
	virtual bool LoadData(u64 offset = 0) { return false; }
	Elf_Machine GetMachine() const { return machine; }

	u32 GetEntry()   const { return entry; }
	u32 GetMinAddr() const { return min_addr; }
	u32 GetMaxAddr() const { return max_addr; }
};

class Loader : public LoaderBase
{
	vfsFileBase* m_stream;
	LoaderBase* m_loader;

public:
	Loader();
	Loader(vfsFileBase& stream);
	virtual ~Loader();

	void Open(const std::string& path);
	void Open(vfsFileBase& stream);
	bool Analyze();

	bool Load();

private:
	LoaderBase* SearchLoader();
};