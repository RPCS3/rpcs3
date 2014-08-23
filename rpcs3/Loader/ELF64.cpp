#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/Static.h"
#include "Emu/Cell/PPUInstrTable.h"
#include "Emu/SysCalls/ModuleManager.h"
#include "ELF64.h"

using namespace PPU_instr;

void Elf64_Ehdr::Load(vfsStream& f)
{
	e_magic = Read32(f);
	e_class = Read8(f);
	e_data = Read8(f);
	e_curver = Read8(f);
	e_os_abi = Read8(f);
	e_abi_ver = Read64(f);
	e_type = Read16(f);
	e_machine = Read16(f);
	e_version = Read32(f);
	e_entry = Read64(f);
	e_phoff = Read64(f);
	e_shoff = Read64(f);
	e_flags = Read32(f);
	e_ehsize = Read16(f);
	e_phentsize = Read16(f);
	e_phnum = Read16(f);
	e_shentsize = Read16(f);
	e_shnum = Read16(f);
	e_shstrndx = Read16(f);
}

void Elf64_Ehdr::Show()
{
#ifdef LOADER_DEBUG
	LOG_NOTICE(LOADER, "Magic: %08x", e_magic);
	LOG_NOTICE(LOADER, "Class: %s", "ELF64");
	LOG_NOTICE(LOADER, "Data: %s", Ehdr_DataToString(e_data).c_str());
	LOG_NOTICE(LOADER, "Current Version: %d", e_curver);
	LOG_NOTICE(LOADER, "OS/ABI: %s", Ehdr_OS_ABIToString(e_os_abi).c_str());
	LOG_NOTICE(LOADER, "ABI version: %lld", e_abi_ver);
	LOG_NOTICE(LOADER, "Type: %s", Ehdr_TypeToString(e_type).c_str());
	LOG_NOTICE(LOADER, "Machine: %s", Ehdr_MachineToString(e_machine).c_str());
	LOG_NOTICE(LOADER, "Version: %d", e_version);
	LOG_NOTICE(LOADER, "Entry point address: 0x%08llx", e_entry);
	LOG_NOTICE(LOADER, "Program headers offset: 0x%08llx", e_phoff);
	LOG_NOTICE(LOADER, "Section headers offset: 0x%08llx", e_shoff);
	LOG_NOTICE(LOADER, "Flags: 0x%x", e_flags);
	LOG_NOTICE(LOADER, "Size of this header: %d", e_ehsize);
	LOG_NOTICE(LOADER, "Size of program headers: %d", e_phentsize);
	LOG_NOTICE(LOADER, "Number of program headers: %d", e_phnum);
	LOG_NOTICE(LOADER, "Size of section headers: %d", e_shentsize);
	LOG_NOTICE(LOADER, "Number of section headers: %d", e_shnum);
	LOG_NOTICE(LOADER, "Section header string table index: %d", e_shstrndx);
#endif
}

void Elf64_Shdr::Load(vfsStream& f)
{
	sh_name = Read32(f);
	sh_type = Read32(f);
	sh_flags = Read64(f);
	sh_addr = Read64(f);
	sh_offset = Read64(f);
	sh_size = Read64(f);
	sh_link = Read32(f);
	sh_info = Read32(f);
	sh_addralign = Read64(f);
	sh_entsize = Read64(f);
}

void Elf64_Shdr::Show()
{
#ifdef LOADER_DEBUG
	LOG_NOTICE(LOADER, "Name offset: %x", sh_name);
	LOG_NOTICE(LOADER, "Type: %d", sh_type);
	LOG_NOTICE(LOADER, "Addr: %llx", sh_addr);
	LOG_NOTICE(LOADER, "Offset: %llx", sh_offset);
	LOG_NOTICE(LOADER, "Size: %llx", sh_size);
	LOG_NOTICE(LOADER, "EntSize: %lld", sh_entsize);
	LOG_NOTICE(LOADER, "Flags: %llx", sh_flags);
	LOG_NOTICE(LOADER, "Link: %x", sh_link);
	LOG_NOTICE(LOADER, "Info: %x", sh_info);
	LOG_NOTICE(LOADER, "Address align: %llx", sh_addralign);
#endif
}

void Elf64_Phdr::Load(vfsStream& f)
{
	p_type = Read32(f);
	p_flags = Read32(f);
	p_offset = Read64(f);
	p_vaddr = Read64(f);
	p_paddr = Read64(f);
	p_filesz = Read64(f);
	p_memsz = Read64(f);
	p_align = Read64(f);
}

void Elf64_Phdr::Show()
{
#ifdef LOADER_DEBUG
	LOG_NOTICE(LOADER, "Type: %s", Phdr_TypeToString(p_type).c_str());
	LOG_NOTICE(LOADER, "Offset: 0x%08llx", p_offset);
	LOG_NOTICE(LOADER, "Virtual address: 0x%08llx", p_vaddr);
	LOG_NOTICE(LOADER, "Physical address: 0x%08llx", p_paddr);
	LOG_NOTICE(LOADER, "File size: 0x%08llx", p_filesz);
	LOG_NOTICE(LOADER, "Memory size: 0x%08llx", p_memsz);
	LOG_NOTICE(LOADER, "Flags: %s", Phdr_FlagsToString(p_flags).c_str());
	LOG_NOTICE(LOADER, "Align: 0x%llx", p_align);
#endif
}

void WriteEhdr(rFile& f, Elf64_Ehdr& ehdr)
{
	Write32(f, ehdr.e_magic);
	Write8(f, ehdr.e_class);
	Write8(f, ehdr.e_data);
	Write8(f, ehdr.e_curver);
	Write8(f, ehdr.e_os_abi);
	Write64(f, ehdr.e_abi_ver);
	Write16(f, ehdr.e_type);
	Write16(f, ehdr.e_machine);
	Write32(f, ehdr.e_version);
	Write64(f, ehdr.e_entry);
	Write64(f, ehdr.e_phoff);
	Write64(f, ehdr.e_shoff);
	Write32(f, ehdr.e_flags);
	Write16(f, ehdr.e_ehsize);
	Write16(f, ehdr.e_phentsize);
	Write16(f, ehdr.e_phnum);
	Write16(f, ehdr.e_shentsize);
	Write16(f, ehdr.e_shnum);
	Write16(f, ehdr.e_shstrndx);
}

void WritePhdr(rFile& f, Elf64_Phdr& phdr)
{
	Write32(f, phdr.p_type);
	Write32(f, phdr.p_flags);
	Write64(f, phdr.p_offset);
	Write64(f, phdr.p_vaddr);
	Write64(f, phdr.p_paddr);
	Write64(f, phdr.p_filesz);
	Write64(f, phdr.p_memsz);
	Write64(f, phdr.p_align);
}

void WriteShdr(rFile& f, Elf64_Shdr& shdr)
{
	Write32(f, shdr.sh_name);
	Write32(f, shdr.sh_type);
	Write64(f, shdr.sh_flags);
	Write64(f, shdr.sh_addr);
	Write64(f, shdr.sh_offset);
	Write64(f, shdr.sh_size);
	Write32(f, shdr.sh_link);
	Write32(f, shdr.sh_info);
	Write64(f, shdr.sh_addralign);
	Write64(f, shdr.sh_entsize);
}

ELF64Loader::ELF64Loader(vfsStream& f)
	: elf64_f(f)
	, LoaderBase()
{
	int a = 0;
}

bool ELF64Loader::LoadInfo()
{
	if(!elf64_f.IsOpened()) return false;

	if(!LoadEhdrInfo()) return false;
	if(!LoadPhdrInfo()) return false;
	if(!LoadShdrInfo()) return false;

	return true;
}

bool ELF64Loader::LoadData(u64 offset)
{
	if(!elf64_f.IsOpened()) return false;

	if(!LoadEhdrData(offset)) return false;
	if(!LoadPhdrData(offset)) return false;
	if(!LoadShdrData(offset)) return false;

	return true;
}

bool ELF64Loader::Close()
{
	return elf64_f.Close();
}

bool ELF64Loader::LoadEhdrInfo(s64 offset)
{
	elf64_f.Seek(offset < 0 ? 0 : offset);
	ehdr.Load(elf64_f);

	if(!ehdr.CheckMagic()) return false;

	if(ehdr.e_phentsize != sizeof(Elf64_Phdr))
	{
		LOG_ERROR(LOADER, "elf64 error:  e_phentsize[0x%x] != sizeof(Elf64_Phdr)[0x%x]", ehdr.e_phentsize, sizeof(Elf64_Phdr));
		return false;
	}

	if(ehdr.e_shentsize != sizeof(Elf64_Shdr))
	{
		LOG_ERROR(LOADER, "elf64 error: e_shentsize[0x%x] != sizeof(Elf64_Shdr)[0x%x]", ehdr.e_shentsize, sizeof(Elf64_Shdr));
		return false;
	}

	switch(ehdr.e_machine)
	{
	case MACHINE_PPC64:
	case MACHINE_SPU:
		machine = (Elf_Machine)ehdr.e_machine;
	break;

	default:
		machine = MACHINE_Unknown;
		LOG_ERROR(LOADER, "Unknown elf64 type: 0x%x", ehdr.e_machine);
		return false;
	}

	entry = ehdr.GetEntry();
	if(entry == 0)
	{
		LOG_ERROR(LOADER, "elf64 error: entry is null!");
		return false;
	}

	return true;
}

bool ELF64Loader::LoadPhdrInfo(s64 offset)
{
	phdr_arr.clear();

	if(ehdr.e_phoff == 0 && ehdr.e_phnum)
	{
		LOG_ERROR(LOADER, "LoadPhdr64 error: Program header offset is null!");
		return false;
	}

	elf64_f.Seek(offset < 0 ? ehdr.e_phoff : offset);

	for(u32 i=0; i<ehdr.e_phnum; ++i)
	{
		phdr_arr.emplace_back();
		phdr_arr.back().Load(elf64_f);
	}

	return true;
}

bool ELF64Loader::LoadShdrInfo(s64 offset)
{
	shdr_arr.clear();
	shdr_name_arr.clear();
	if(ehdr.e_shoff == 0 && ehdr.e_shnum)
	{
		LOG_NOTICE(LOADER, "LoadShdr64 error: Section header offset is null!");
		return true;
	}

	elf64_f.Seek(offset < 0 ? ehdr.e_shoff : offset);
	for(u32 i=0; i<ehdr.e_shnum; ++i)
	{
		shdr_arr.emplace_back();
		shdr_arr.back().Load(elf64_f);
	}

	if(ehdr.e_shstrndx >= shdr_arr.size())
	{
		LOG_WARNING(LOADER, "LoadShdr64 error: shstrndx too big!");
		return true;
	}

	for(u32 i=0; i<shdr_arr.size(); ++i)
	{
		elf64_f.Seek((offset < 0 ? shdr_arr[ehdr.e_shstrndx].sh_offset : shdr_arr[ehdr.e_shstrndx].sh_offset - ehdr.e_shoff + offset) + shdr_arr[i].sh_name);
		std::string name;
		while(!elf64_f.Eof())
		{
			char c;
			elf64_f.Read(&c, 1);
			if(c == 0) break;
			name.push_back(c);
		}
		shdr_name_arr.push_back(name);
	}

	return true;
}

bool ELF64Loader::LoadEhdrData(u64 offset)
{
#ifdef LOADER_DEBUG
	LOG_NOTICE(LOADER, "");
	ehdr.Show();
	LOG_NOTICE(LOADER, "");
#endif
	return true;
}

bool ELF64Loader::LoadPhdrData(u64 offset)
{
	for(auto& phdr: phdr_arr)
	{
		phdr.Show();

		if (phdr.p_vaddr < min_addr)
		{
			min_addr = phdr.p_vaddr;
		}

		if (phdr.p_vaddr + phdr.p_memsz > max_addr)
		{
			max_addr = phdr.p_vaddr + phdr.p_memsz;
		}

		if (phdr.p_vaddr != phdr.p_paddr)
		{
			LOG_WARNING(LOADER,	"ElfProgram different load addrs: paddr=0x%8.8x, vaddr=0x%8.8x", 
				phdr.p_paddr, phdr.p_vaddr);
		}

		if(!Memory.MainMem.IsInMyRange(offset + phdr.p_vaddr, phdr.p_memsz))
		{
#ifdef LOADER_DEBUG
			LOG_WARNING(LOADER, "Skipping...");
			LOG_WARNING(LOADER, "");
#endif
			continue;
		}

		switch(phdr.p_type)
		{
			case 0x00000001: //LOAD
				if(phdr.p_memsz)
				{
					Memory.MainMem.AllocFixed(offset + phdr.p_vaddr, phdr.p_memsz);

					if(phdr.p_filesz)
					{
						elf64_f.Seek(phdr.p_offset);
						elf64_f.Read(&Memory[offset + phdr.p_vaddr], phdr.p_filesz);
						Emu.GetSFuncManager().StaticAnalyse(&Memory[offset + phdr.p_vaddr], phdr.p_filesz, phdr.p_vaddr);
					}
				}
			break;

			case 0x00000007: //TLS
				Emu.SetTLSData(offset + phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz);
			break;

			case 0x60000001: //LOOS+1
			{
				if(!phdr.p_filesz)
					break;

				const sys_process_param& proc_param = *(sys_process_param*)&Memory[offset + phdr.p_vaddr];

				if (proc_param.size < sizeof(sys_process_param)) {
					LOG_WARNING(LOADER, "Bad proc param size! [0x%x : 0x%x]", proc_param.size, sizeof(sys_process_param));
				}
				if (proc_param.magic != 0x13bcc5f6) {
					LOG_ERROR(LOADER, "Bad magic! [0x%x]", proc_param.magic);
				}
				else {				
#ifdef LOADER_DEBUG
					sys_process_param_info& info = Emu.GetInfo().GetProcParam();
					LOG_NOTICE(LOADER, "*** sdk version: 0x%x", info.sdk_version.ToLE());
					LOG_NOTICE(LOADER, "*** primary prio: %d", info.primary_prio.ToLE());
					LOG_NOTICE(LOADER, "*** primary stacksize: 0x%x", info.primary_stacksize.ToLE());
					LOG_NOTICE(LOADER, "*** malloc pagesize: 0x%x", info.malloc_pagesize.ToLE());
					LOG_NOTICE(LOADER, "*** ppc seg: 0x%x", info.ppc_seg.ToLE());
					//LOG_NOTICE(LOADER, "*** crash dump param addr: 0x%x", info.crash_dump_param_addr.ToLE());
#endif
				}
			}
			break;

			case 0x60000002: //LOOS+2
			{
				if(!phdr.p_filesz)
					break;

				sys_proc_prx_param proc_prx_param = *(sys_proc_prx_param*)&Memory[offset + phdr.p_vaddr];


#ifdef LOADER_DEBUG
				LOG_NOTICE(LOADER, "*** size: 0x%x", proc_prx_param.size.ToLE());
				LOG_NOTICE(LOADER, "*** magic: 0x%x", proc_prx_param.magic.ToLE());
				LOG_NOTICE(LOADER, "*** version: 0x%x", proc_prx_param.version.ToLE());
				LOG_NOTICE(LOADER, "*** libentstart: 0x%x", proc_prx_param.libentstart.ToLE());
				LOG_NOTICE(LOADER, "*** libentend: 0x%x", proc_prx_param.libentend.ToLE());
				LOG_NOTICE(LOADER, "*** libstubstart: 0x%x", proc_prx_param.libstubstart.ToLE());
				LOG_NOTICE(LOADER, "*** libstubend: 0x%x", proc_prx_param.libstubend.ToLE());
				LOG_NOTICE(LOADER, "*** ver: 0x%x", proc_prx_param.ver.ToLE());
#endif

				if (proc_prx_param.magic != 0x1b434cec) {
					LOG_ERROR(LOADER, "Bad magic! (0x%x)", proc_prx_param.magic.ToLE());
					break;
				}

				for(u32 s=proc_prx_param.libstubstart; s<proc_prx_param.libstubend; s+=sizeof(Elf64_StubHeader))
				{
					Elf64_StubHeader stub = *(Elf64_StubHeader*)Memory.GetMemFromAddr(offset + s);

					const std::string& module_name = Memory.ReadString(stub.s_modulename);
					Module* module = Emu.GetModuleManager().GetModuleByName(module_name);
					if (module) {
						//module->SetLoaded();
					}
					else {
						LOG_WARNING(LOADER, "Unknown module '%s'", module_name.c_str());
					}

#ifdef LOADER_DEBUG
					LOG_NOTICE(LOADER, "");
					LOG_NOTICE(LOADER, "*** size: 0x%x", stub.s_size);
					LOG_NOTICE(LOADER, "*** version: 0x%x", stub.s_version.ToLE());
					LOG_NOTICE(LOADER, "*** unk0: 0x%x", stub.s_unk0);
					LOG_NOTICE(LOADER, "*** unk1: 0x%x", stub.s_unk1.ToLE());
					LOG_NOTICE(LOADER, "*** imports: %d", stub.s_imports.ToLE());
					LOG_NOTICE(LOADER, "*** module name: %s [0x%x]", module_name.c_str(), stub.s_modulename.ToLE());
					LOG_NOTICE(LOADER, "*** nid: 0x%016llx [0x%x]", Memory.Read64(stub.s_nid), stub.s_nid.ToLE());
					LOG_NOTICE(LOADER, "*** text: 0x%x", stub.s_text.ToLE());
#endif
					static const u32 section = 4 * 3;
					u64 tbl = Memory.MainMem.AllocAlign(stub.s_imports * 4 * 2);
					u64 dst = Memory.MainMem.AllocAlign(stub.s_imports * section);

					for(u32 i=0; i<stub.s_imports; ++i)
					{
						const u32 nid = Memory.Read32(stub.s_nid + i*4);
						const u32 text = Memory.Read32(stub.s_text + i*4);

						if (module && !module->Load(nid))
						{
							LOG_WARNING(LOADER, "Unimplemented function '%s' in '%s' module", SysCalls::GetHLEFuncName(nid).c_str(), module_name.c_str());
						}
						else //if (Ini.HLELogging.GetValue())
						{
							LOG_NOTICE(LOADER, "Imported function '%s' in '%s' module", SysCalls::GetHLEFuncName(nid).c_str(), module_name.c_str());
						}
#ifdef LOADER_DEBUG
						LOG_NOTICE(LOADER, "import %d:", i+1);
						LOG_NOTICE(LOADER, "*** nid: 0x%x (0x%x)", nid, stub.s_nid + i*4);
						LOG_NOTICE(LOADER, "*** text: 0x%x (0x%x)", text, stub.s_text + i*4);
#endif
						Memory.Write32(stub.s_text + i*4, tbl + i*8);

						mem32_ptr_t out_tbl(tbl + i*8);
						out_tbl += dst + i*section;
						out_tbl += Emu.GetModuleManager().GetFuncNumById(nid);

						mem32_ptr_t out_dst(dst + i*section);
						out_dst += OR(11, 2, 2, 0);
						out_dst += SC(2);
						out_dst += BCLR(0x10 | 0x04, 0, 0, 0);
					}
				}
#ifdef LOADER_DEBUG
				LOG_NOTICE(LOADER, "");
#endif
			}
			break;
		}
#ifdef LOADER_DEBUG
		LOG_NOTICE(LOADER, "");
#endif
	}

	return true;
}

bool ELF64Loader::LoadShdrData(u64 offset)
{
	u64 max_addr = 0;

	for(uint i=0; i<shdr_arr.size(); ++i)
	{
		Elf64_Shdr& shdr = shdr_arr[i];

		if(i < shdr_name_arr.size())
		{
#ifdef LOADER_DEBUG
			LOG_NOTICE(LOADER, "Name: %s", shdr_name_arr[i].c_str());
#endif
		}

#ifdef LOADER_DEBUG
		shdr.Show();
		LOG_NOTICE(LOADER, " ");
#endif
		if(shdr.sh_addr + shdr.sh_size > max_addr) max_addr = shdr.sh_addr + shdr.sh_size;

		if((shdr.sh_flags & SHF_ALLOC) != SHF_ALLOC) continue;

		const u64 addr = shdr.sh_addr;
		const u64 size = shdr.sh_size;

		if(size == 0 || !Memory.IsGoodAddr(offset + addr, size)) continue;

		if(shdr.sh_addr && shdr.sh_addr < min_addr)
		{
			min_addr = shdr.sh_addr;
		}

		if(shdr.sh_addr + shdr.sh_size > max_addr)
		{
			max_addr = shdr.sh_addr + shdr.sh_size;
		}

		if((shdr.sh_type == SHT_RELA) || (shdr.sh_type == SHT_REL))
		{
			LOG_ERROR(LOADER, "ELF64 ERROR: Relocation");
			continue;
		}

		switch(shdr.sh_type)
		{
		case SHT_NOBITS:
			//LOG_WARNING(LOADER, "SHT_NOBITS: addr=0x%llx, size=0x%llx", offset + addr, size);
			//memset(&Memory[offset + addr], 0, size);
		break;

		case SHT_PROGBITS:
			//elf64_f.Seek(shdr.sh_offset);
			//elf64_f.Read(&Memory[offset + addr], shdr.sh_size);
		break;
		}
	}

	return true;
}
