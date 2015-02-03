#include "stdafx.h"
#include "Ini.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/FS/vfsStream.h"
#include "Emu/Memory/Memory.h"
#include "ELF32.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"
#include "Emu/ARMv7/ARMv7Decoder.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/System.h"

extern void armv7_init_tls();

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
				struct psv_libc_param_t
				{
					u32 size; // 0x0000001c
					u32 unk1; // 0x00000000

					vm::psv::ptr<u32> sceLibcHeapSize;
					vm::psv::ptr<u32> sceLibcHeapSizeDefault;
					vm::psv::ptr<u32> sceLibcHeapExtendedAlloc;
					vm::psv::ptr<u32> sceLibcHeapDelayedAlloc;

					u32 unk2;
					u32 unk3;
					
					vm::psv::ptr<u32> __sce_libcmallocreplace;
					vm::psv::ptr<u32> __sce_libcnewreplace;
				};

				struct psv_process_param_t
				{
					u32 size; // 0x00000030
					u32 unk1; // 'PSP2'
					u32 unk2; // 0x00000005
					u32 unk3;

					vm::psv::ptr<const char> sceUserMainThreadName;
					vm::psv::ptr<s32>        sceUserMainThreadPriority;
					vm::psv::ptr<u32>        sceUserMainThreadStackSize;
					vm::psv::ptr<u32>        sceUserMainThreadAttribute;
					vm::psv::ptr<const char> sceProcessName;
					vm::psv::ptr<u32>        sce_process_preload_disabled;
					vm::psv::ptr<u32>        sceUserMainThreadCpuAffinityMask;

					vm::psv::ptr<psv_libc_param_t> __sce_libcparam;
				};

				initialize_psv_modules();

				auto armv7_thr_stop_data = vm::psv::ptr<u32>::make(Memory.PSV.RAM.AllocAlign(3 * 4));
				armv7_thr_stop_data[0] = 0xf870; // HACK instruction (Thumb)
				armv7_thr_stop_data[1] = 0x0001; // index 1
				Emu.SetCPUThreadExit(armv7_thr_stop_data.addr());

				u32 entry = 0; // actual entry point (ELFs entry point is ignored)
				u32 fnid_addr = 0;
				u32 code_start = 0;
				u32 code_end = 0;
				u32 vnid_addr = 0;
				std::unordered_map<u32, u32> vnid_list;

				auto proc_param = vm::psv::ptr<psv_process_param_t>::make(0);

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

					if (!strcmp(name.c_str(), ".text"))
					{
						LOG_NOTICE(LOADER, ".text analysis...");

						code_start = shdr.data_le.sh_addr;
						code_end = shdr.data_le.sh_size + code_start;
					}
					else if (!strcmp(name.c_str(), ".sceExport.rodata"))
					{
						LOG_NOTICE(LOADER, ".sceExport.rodata analysis...");

						auto enid = vm::psv::ptr<const u32>::make(shdr.data_le.sh_addr);
						auto edata = vm::psv::ptr<const u32>::make(enid.addr() + shdr.data_le.sh_size / 2);

						for (u32 j = 0; j < shdr.data_le.sh_size / 8; j++)
						{
							switch (const u32 nid = enid[j])
							{
							case 0x935cd196: // set entry point
							{
								entry = edata[j];
								break;
							}

							case 0x6c2224ba: // __sce_moduleinfo
							{
								// currently nothing, but it should theoretically be the root of analysis instead of section name comparison
								break;
							}

							case 0x70fba1e7: // __sce_process_param
							{
								proc_param.set(edata[j]);
								break;
							}

							default:
							{
								LOG_ERROR(LOADER, "Unknown export 0x%08x (addr=0x%08x)", nid, edata[j]);
							}
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
							const u32 nid = fnid[j];
							const u32 addr = fstub[j];

							if (auto func = get_psv_func_by_nid(nid))
							{
								if (func->module)
								{
									func->module->Notice("Imported function %s (nid=0x%08x, addr=0x%x)", func->name, nid, addr);
								}
								else
								{
									LOG_NOTICE(LOADER, "Imported function %s (nid=0x%08x, addr=0x%x)", func->name, nid, addr);
								}

								const u32 code = get_psv_func_index(func);
								vm::psv::write32(addr + 0, 0xe0700090 | (code & 0xfff0) << 4 | (code & 0xf)); // HACK instruction (ARM)
							}
							else
							{
								LOG_ERROR(LOADER, "Unknown function 0x%08x (addr=0x%x)", nid, addr);

								vm::psv::write32(addr + 0, 0xe0700090); // HACK instruction (ARM), unimplemented stub (code 0)
								vm::psv::write32(addr + 4, nid); // nid
							}

							code_end = std::min<u32>(addr, code_end);
						}
					}
					else if (!strcmp(name.c_str(), ".sceVNID.rodata"))
					{
						LOG_NOTICE(LOADER, ".sceVNID.rodata analysis...");

						vnid_addr = shdr.data_le.sh_addr;
					}
					else if (!strcmp(name.c_str(), ".sceVStub.rodata"))
					{
						LOG_NOTICE(LOADER, ".sceVStub.rodata analysis...");

						if (!vnid_addr)
						{
							if (shdr.data_le.sh_size)
							{
								LOG_ERROR(LOADER, ".sceVNID.rodata address not found, unable to process imports");
							}
							continue;
						}

						auto vnid = vm::psv::ptr<const u32>::make(vnid_addr);
						auto vstub = vm::psv::ptr<const u32>::make(shdr.data_le.sh_addr);

						for (u32 j = 0; j < shdr.data_le.sh_size / 4; j++)
						{
							const u32 nid = vnid[j];
							const u32 addr = vstub[j];

							LOG_ERROR(LOADER, "Unknown object 0x%08x (ref_addr=0x%x)", nid, addr);

							// TODO: find imported object (vtable, typeinfo or something), assign it to vnid_list[addr]
						}
					}
					else if (!strcmp(name.c_str(), ".tbss"))
					{
						LOG_NOTICE(LOADER, ".tbss analysis...");
						const u32 img_addr = shdr.data_le.sh_addr;                  // start address of TLS initialization image
						const u32 img_size = (&shdr)[1].data_le.sh_addr - img_addr; // calculate its size as the difference between sections
						const u32 tls_size = shdr.data_le.sh_size;                  // full size of TLS

						LOG_WARNING(LOADER, "TLS: img_addr=0x%08x, img_size=0x%x, tls_size=0x%x", img_addr, img_size, tls_size);
						Emu.SetTLSData(img_addr, img_size, tls_size);
					}
					else if (!strcmp(name.c_str(), ".sceRefs.rodata"))
					{
						LOG_NOTICE(LOADER, ".sceRefs.rodata analysis...");

						u32 data = 0;

						for (auto code = vm::psv::ptr<const u32>::make(shdr.data_le.sh_addr); code.addr() < shdr.data_le.sh_addr + shdr.data_le.sh_size; code++)
						{
							switch (*code)
							{
							case 0x000000ff: // save address for future use
							{
								data = *++code;
								break;
							}
							case 0x0000002f: // movw r*,# instruction is replaced
							{
								if (!data) // probably, imported object
								{
									auto found = vnid_list.find(code.addr());
									if (found != vnid_list.end())
									{
										data = found->second;
									}
								}

								if (!data)
								{
									LOG_ERROR(LOADER, ".sceRefs: movw writing failed (ref_addr=0x%x, addr=0x%x)", code, code[1]);
								}
								else //if (Ini.HLELogging.GetValue())
								{
									LOG_NOTICE(LOADER, ".sceRefs: movw written at 0x%x (ref_addr=0x%x, data=0x%x)", code[1], code, data);
								}

								const u32 addr = *++code;
								vm::psv::write16(addr + 0, vm::psv::read16(addr + 0) | (data & 0x800) >> 1 | (data & 0xf000) >> 12);
								vm::psv::write16(addr + 2, vm::psv::read16(addr + 2) | (data & 0x700) << 4 | (data & 0xff));
								break;
							}
							case 0x00000030: // movt r*,# instruction is replaced
							{
								if (!data)
								{
									LOG_ERROR(LOADER, ".sceRefs: movt writing failed (ref_addr=0x%x, addr=0x%x)", code, code[1]);
								}
								else //if (Ini.HLELogging.GetValue())
								{
									LOG_NOTICE(LOADER, ".sceRefs: movt written at 0x%x (ref_addr=0x%x, data=0x%x)", code[1], code, data);
								}

								const u32 addr = *++code;
								vm::psv::write16(addr + 0, vm::psv::read16(addr + 0) | (data & 0x8000000) >> 17 | (data & 0xf0000000) >> 28);
								vm::psv::write16(addr + 2, vm::psv::read16(addr + 2) | (data & 0x7000000) >> 12 | (data & 0xff0000) >> 16);
								break;
							}
							case 0x00000000:
							{
								data = 0;

								if (Ini.HLELogging.GetValue())
								{
									LOG_NOTICE(LOADER, ".sceRefs: zero code found");
								}
								break;
							}
							default:
							{
								LOG_ERROR(LOADER, "Unknown code in .sceRefs section (0x%08x)", *code);
							}
							}
						}
					}
				}

				LOG_NOTICE(LOADER, "__sce_process_param(addr=0x%x) analysis...", proc_param);

				if (proc_param->size != 0x30 || proc_param->unk1 != *(u32*)"PSP2" || proc_param->unk2 != 5)
				{
					LOG_ERROR(LOADER, "__sce_process_param: unexpected data found (size=0x%x, 0x%x, 0x%x, 0x%x)", proc_param->size, proc_param->unk1, proc_param->unk2, proc_param->unk3);
				}

				LOG_NOTICE(LOADER, "*** &sceUserMainThreadName            = 0x%x", proc_param->sceUserMainThreadName);
				LOG_NOTICE(LOADER, "*** &sceUserMainThreadPriority        = 0x%x", proc_param->sceUserMainThreadPriority);
				LOG_NOTICE(LOADER, "*** &sceUserMainThreadStackSize       = 0x%x", proc_param->sceUserMainThreadStackSize);
				LOG_NOTICE(LOADER, "*** &sceUserMainThreadAttribute       = 0x%x", proc_param->sceUserMainThreadAttribute);
				LOG_NOTICE(LOADER, "*** &sceProcessName                   = 0x%x", proc_param->sceProcessName);
				LOG_NOTICE(LOADER, "*** &sce_process_preload_disabled     = 0x%x", proc_param->sce_process_preload_disabled);
				LOG_NOTICE(LOADER, "*** &sceUserMainThreadCpuAffinityMask = 0x%x", proc_param->sceUserMainThreadCpuAffinityMask);

				auto libc_param = proc_param->__sce_libcparam;

				LOG_NOTICE(LOADER, "__sce_libcparam(addr=0x%x) analysis...", libc_param);

				if (libc_param->size != 0x1c || libc_param->unk1)
				{
					LOG_ERROR(LOADER, "__sce_libcparam: unexpected data found (size=0x%x, 0x%x, 0x%x)", libc_param->size, libc_param->unk1, libc_param->unk2);
				}

				LOG_NOTICE(LOADER, "*** &sceLibcHeapSize          = 0x%x", libc_param->sceLibcHeapSize);
				LOG_NOTICE(LOADER, "*** &sceLibcHeapSizeDefault   = 0x%x", libc_param->sceLibcHeapSizeDefault);
				LOG_NOTICE(LOADER, "*** &sceLibcHeapExtendedAlloc = 0x%x", libc_param->sceLibcHeapExtendedAlloc);
				LOG_NOTICE(LOADER, "*** &sceLibcHeapDelayedAlloc  = 0x%x", libc_param->sceLibcHeapDelayedAlloc);

				armv7_init_tls();
				armv7_decoder_initialize(code_start, code_end);

				const std::string& thread_name = proc_param->sceUserMainThreadName ? proc_param->sceUserMainThreadName.get_ptr() : "main_thread";
				const u32 stack_size = proc_param->sceUserMainThreadStackSize ? *proc_param->sceUserMainThreadStackSize : 0;
				const u32 priority = proc_param->sceUserMainThreadPriority ? *proc_param->sceUserMainThreadPriority : 0;

				armv7_thread(entry, thread_name, stack_size, priority).args({ Emu.GetPath(), "-emu" }).run();
				break;
			}
			case MACHINE_SPU: spu_thread(m_ehdr.is_le() ? m_ehdr.data_le.e_entry : m_ehdr.data_be.e_entry, "main_thread").args({ Emu.GetPath()/*, "-emu"*/ }).run(); break;
			}

			return ok;
		}

		handler::error_code elf32::load_data(u32 offset, bool skip_writeable)
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

						if (skip_writeable == true && (phdr.data_be.p_flags & 2/*PF_W*/) != 0)
						{
							continue;
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
