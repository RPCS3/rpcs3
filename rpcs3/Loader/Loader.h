#pragma once

#ifdef _DEBUG	
	#define LOADER_DEBUG
#endif

enum Elf_Machine
{
	MACHINE_Unknown,
	MACHINE_PPC64 = 0x15,
	MACHINE_SPU = 0x17,
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
	SHF_WRITE		= 0x1,
	SHF_ALLOC		= 0x2,
	SHF_EXECINSTR	= 0x4,
	SHF_MASKPROC	= 0xf0000000,
};

u8  Read8 (wxFile& f);
u16 Read16(wxFile& f);
u32 Read32(wxFile& f);
u64 Read64(wxFile& f);

const wxString Ehdr_DataToString(const u8 data);
const wxString Ehdr_TypeToString(const u16 type);
const wxString Ehdr_OS_ABIToString(const u8 os_abi);
const wxString Ehdr_MachineToString(const u16 machine);
const wxString Phdr_FlagsToString(u32 flags);
const wxString Phdr_TypeToString(const u32 type);

struct sys_process_param_info
{
	u32 sdk_version;
	s32 primary_prio;
	u32 primary_stacksize;
	u32 malloc_pagesize;
	u32 ppc_seg;
	//u32 crash_dump_param_addr;
};

struct sys_process_param
{
	u32 size;
	u32 magic;
	u32 version;
	sys_process_param_info info;
};

struct sys_proc_prx_param
{
	u32 size;
	u32 magic;
	u32 version;
	u32 pad0;
	u32 libentstart;
	u32 libentend;
	u32 libstubstart;
	u32 libstubend;
	u16 ver;
	u16 pad1;
	u32 pad2;
};

struct Elf64_StubHeader
{
	u8 s_size; // = 0x2c
	u8 s_unk0;
	u16 s_version; // = 0x1
	u16 s_unk1; // = 0x9 // flags?
	u16 s_imports;
	u32 s_unk2; // = 0x0
	u32 s_unk3; // = 0x0
	u32 s_modulename;
	u32 s_nid;
	u32 s_text;
	u32 s_unk4; // = 0x0
	u32 s_unk5; // = 0x0
	u32 s_unk6; // = 0x0
	u32 s_unk7; // = 0x0
};

class LoaderBase
{
protected:
	u32 entry;
	Elf_Machine machine;

	LoaderBase()
		: machine(MACHINE_Unknown)
		, entry(0)
	{
	}

public:
	virtual bool LoadInfo(){return false;};
	virtual bool LoadData(){return false;};
	virtual bool Close(){return false;};
	Elf_Machine GetMachine() { return machine; }
	u32 GetEntry() { return entry; }
};

class Loader : public LoaderBase
{
	wxFile* f;
	wxString m_path;

public:
	Loader();
	Loader(const wxString& path);
	void Open(const wxString& path);
	void Open(wxFile& f, const wxString& path);
	~Loader();

	bool Load();

private:
	LoaderBase* SearchLoader();
};