#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/FS/vfsStream.h"
#include "Emu/Memory/Memory.h"
#include "ELF32.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/System.h"

namespace loader
{
	namespace handlers
	{
		handler::error_code elf32::init(vfsStream& stream)
		{
			error_code res = handler::init(stream);

			if (res != ok)
				return res;

			m_stream->Read(&m_ehdr, sizeof(ehdr));

			if (!m_ehdr.check())
			{
				return bad_file;
			}

			if (m_ehdr.data_le.e_phnum && (m_ehdr.is_le() ? m_ehdr.data_le.e_phentsize : m_ehdr.data_be.e_phentsize) != sizeof(phdr))
			{
				return broken_file;
			}

			if (m_ehdr.data_le.e_shnum && (m_ehdr.is_le() ? m_ehdr.data_le.e_shentsize : m_ehdr.data_be.e_shentsize) != sizeof(shdr))
			{
				return broken_file;
			}

			LOG_WARNING(LOADER, "m_ehdr.e_type = 0x%x", (u16)(m_ehdr.is_le() ? m_ehdr.data_le.e_type : m_ehdr.data_be.e_type));

			if (m_ehdr.data_le.e_phnum)
			{
				m_phdrs.resize(m_ehdr.is_le() ? m_ehdr.data_le.e_phnum : m_ehdr.data_be.e_phnum);
				m_stream->Seek(handler::get_stream_offset() + (m_ehdr.is_le() ? m_ehdr.data_le.e_phoff : m_ehdr.data_be.e_phoff));
				size_t size = (m_ehdr.is_le() ? m_ehdr.data_le.e_phnum : m_ehdr.data_be.e_phnum) * sizeof(phdr);

				if (m_stream->Read(m_phdrs.data(), size) != size)
					return broken_file;
			}
			else
				m_phdrs.clear();

			if (m_ehdr.data_le.e_shnum)
			{
				m_shdrs.resize(m_ehdr.is_le() ? m_ehdr.data_le.e_shnum : m_ehdr.data_be.e_shnum);
				m_stream->Seek(handler::get_stream_offset() + (m_ehdr.is_le() ? m_ehdr.data_le.e_shoff : m_ehdr.data_be.e_shoff));
				size_t size = (m_ehdr.is_le() ? m_ehdr.data_le.e_shnum : m_ehdr.data_be.e_shnum) * sizeof(shdr);

				if (m_stream->Read(m_shdrs.data(), size) != size)
					return broken_file;
			}
			else
				m_shdrs.clear();

			return ok;
		}

		handler::error_code elf32::load()
		{
			Elf_Machine machine;
			switch (machine = (Elf_Machine)(u16)(m_ehdr.is_le() ? m_ehdr.data_le.e_machine : m_ehdr.data_be.e_machine))
			{
			case MACHINE_MIPS: vm::psp::init(); break;
			case MACHINE_ARM: vm::psv::init(); break;
			case MACHINE_SPU: vm::ps3::init(); break;

			default:
				return bad_version;
			}

			error_code res = load_data(0);

			if (res != ok)
				return res;

			switch (machine)
			{
			case MACHINE_MIPS: break;
			case MACHINE_ARM:
			{
				initialize_psv_modules();

				auto armv7_thr_stop_data = vm::psv::ptr<u32>::make(Memory.PSV.RAM.AllocAlign(3 * 4));
				armv7_thr_stop_data[0] = 0xf870; // HACK instruction (Thumb)
				armv7_thr_stop_data[1] = 0x0001; // index 1
				Emu.SetCPUThreadExit(armv7_thr_stop_data.addr());

				u32 entry = 0; // actual entry point (ELFs entry point is ignored)
				u32 fnid_addr = 0;

				// load section names
				//assert(m_ehdr.data_le.e_shstrndx < m_shdrs.size());
				//const u32 sname_off = m_shdrs[m_ehdr.data_le.e_shstrndx].data_le.sh_offset;
				//const u32 sname_size = m_shdrs[m_ehdr.data_le.e_shstrndx].data_le.sh_size;
				//const u32 sname_base = sname_size ? Memory.PSV.RAM.AllocAlign(sname_size) : 0;
				//if (sname_base)
				//{
				//	m_stream->Seek(handler::get_stream_offset() + sname_off);
				//	m_stream->Read(vm::get_ptr<void>(sname_base), sname_size);
				//}

				for (auto& shdr : m_shdrs)
				{
					// get secton name
					//auto name = vm::psv::ptr<const char>::make(sname_base + shdr.data_le.sh_name);

					m_stream->Seek(handler::get_stream_offset() + m_shdrs[m_ehdr.data_le.e_shstrndx].data_le.sh_offset + shdr.data_le.sh_name);
					std::string name;
					while (!m_stream->Eof())
					{
						char c;
						m_stream->Read(&c, 1);
						if (c == 0) break;
						name.push_back(c);
					}

					if (!strcmp(name.c_str(), ".sceModuleInfo.rodata"))
					{
						LOG_NOTICE(LOADER, ".sceModuleInfo.rodata analysis...");

						auto code = vm::psv::ptr<const u32>::make(shdr.data_le.sh_addr);

						// very rough way to find the entry point
						while (code[0] != 0xffffffffu)
						{
							entry = code[0] + 0x81000000;
							code++;

							if (code.addr() >= shdr.data_le.sh_addr + shdr.data_le.sh_size)
							{
								LOG_ERROR(LOADER, "Unable to find entry point in .sceModuleInfo.rodata");
								entry = 0;
								break;
							}
						}
					}
					else if (!strcmp(name.c_str(), ".sceFNID.rodata"))
					{
						LOG_NOTICE(LOADER, ".sceFNID.rodata analysis...");

						fnid_addr = shdr.data_le.sh_addr;
					}
					else if (!strcmp(name.c_str(), ".sceFStub.rodata"))
					{
						LOG_NOTICE(LOADER, ".sceFStub.rodata analysis...");

						if (!fnid_addr)
						{
							LOG_ERROR(LOADER, ".sceFNID.rodata address not found, unable to process imports");
							continue;
						}

						auto fnid = vm::psv::ptr<const u32>::make(fnid_addr);
						auto fstub = vm::psv::ptr<const u32>::make(shdr.data_le.sh_addr);

						for (u32 j = 0; j < shdr.data_le.sh_size / 4; j++)
						{
							u32 nid = fnid[j];
							u32 addr = fstub[j];

							if (auto func = get_psv_func_by_nid(nid))
							{
								if (func->module)
									func->module->Notice("Imported function %s (nid=0x%08x, addr=0x%x)", func->name, nid, addr);
								else
									LOG_NOTICE(LOADER, "Imported function %s (nid=0x%08x, addr=0x%x)", func->name, nid, addr);

								// writing Thumb code (temporarily, because it should be ARM)
								vm::psv::write16(addr + 0, 0xf870); // HACK instruction (Thumb)
								vm::psv::write16(addr + 2, (u16)get_psv_func_index(func)); // function index
								vm::psv::write16(addr + 4, 0x4770); // BX LR
								vm::psv::write16(addr + 6, 0); // null
							}
							else
							{
								LOG_ERROR(LOADER, "Unimplemented function 0x%08x (addr=0x%x)", nid, addr);

								vm::psv::write16(addr + 0, 0xf870); // HACK instruction (Thumb)
								vm::psv::write16(addr + 2, 0); // index 0 (unimplemented stub)
								vm::psv::write32(addr + 4, nid); // nid
							}
						}
					}
					else if (!strcmp(name.c_str(), ".sceRefs.rodata"))
					{
						LOG_NOTICE(LOADER, ".sceRefs.rodata analysis...");

						u32 data = 0;

						for (auto code = vm::psv::ptr<const u32>::make(shdr.data_le.sh_addr); code.addr() < shdr.data_le.sh_addr + shdr.data_le.sh_size; code++)
						{
							switch (*code)
							{
							case 0x000000ff:
							{
								// save address for future use
								data = *++code;
								break;
							}
							case 0x0000002f:
							{
								// movw r12,# instruction will be replaced
								const u32 addr = *++code;
								vm::psv::write16(addr + 0, 0xf240 | (data & 0x800) >> 1 | (data & 0xf000) >> 12); // MOVW
								vm::psv::write16(addr + 2, 0x0c00 | (data & 0x700) << 4 | (data & 0xff));
								break;
							}
							case 0x00000030:
							{
								// movt r12,# instruction will be replaced
								const u32 addr = *++code;
								vm::psv::write16(addr + 0, 0xf2c0 | (data & 0x8000000) >> 17 | (data & 0xf0000000) >> 28); // MOVT
								vm::psv::write16(addr + 2, 0x0c00 | (data & 0x7000000) >> 12 | (data & 0xff0000) >> 16);
								break;
							}
							case 0x00000000:
							{
								// probably, no operation
								break;
							}
							default:
							{
								LOG_NOTICE(LOADER, "sceRefs: unknown code found (0x%08x)", *code);
							}
							}
						}
					}
				}

				arm7_thread(entry & ~1 /* TODO: Thumb/ARM encoding selection */, "main_thread").args({ Emu.GetPath()/*, "-emu"*/ }).run();
				break;
			}
			case MACHINE_SPU: spu_thread(m_ehdr.is_le() ? m_ehdr.data_le.e_entry : m_ehdr.data_be.e_entry, "main_thread").args({ Emu.GetPath()/*, "-emu"*/ }).run(); break;
			}

			return ok;
		}

		handler::error_code elf32::load_data(u32 offset)
		{
			Elf_Machine machine = (Elf_Machine)(u16)(m_ehdr.is_le() ? m_ehdr.data_le.e_machine : m_ehdr.data_be.e_machine);

			for (auto &phdr : m_phdrs)
			{
				u32 memsz = m_ehdr.is_le() ? phdr.data_le.p_memsz : phdr.data_be.p_memsz;
				u32 filesz = m_ehdr.is_le() ? phdr.data_le.p_filesz : phdr.data_be.p_filesz;
				u32 vaddr = offset + (m_ehdr.is_le() ? phdr.data_le.p_vaddr : phdr.data_be.p_vaddr);
				u32 offset = m_ehdr.is_le() ? phdr.data_le.p_offset : phdr.data_be.p_offset;

				switch (m_ehdr.is_le() ? phdr.data_le.p_type : phdr.data_be.p_type)
				{
				case 0x00000001: //LOAD
					if (phdr.data_le.p_memsz)
					{
						if (machine == MACHINE_ARM && !Memory.PSV.RAM.AllocFixed(vaddr, memsz))
						{
							LOG_ERROR(LOADER, "%s(): AllocFixed(0x%llx, 0x%x) failed", __FUNCTION__, vaddr, memsz);

							return loading_error;
						}

						if (filesz)
						{
							m_stream->Seek(handler::get_stream_offset() + offset);
							m_stream->Read(vm::get_ptr(vaddr), filesz);
						}
					}
					break;
				}
			}

			return ok;
		}
	}
}
