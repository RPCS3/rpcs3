#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/FS/vfsStream.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDir.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/Static.h"
#include "Emu/SysCalls/ModuleManager.h"
#include "Emu/SysCalls/lv2/sys_prx.h"
#include "Emu/Cell/PPUInstrTable.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "ELF64.h"
#include "Ini.h"

using namespace PPU_instr;

namespace loader
{
	namespace handlers
	{
		handler::error_code elf64::init(vfsStream& stream)
		{
			m_ehdr = {};
			m_sprx_module_info = {};
			m_sprx_function_info = {};

			m_phdrs.clear();
			m_shdrs.clear();

			m_sprx_segments_info.clear();
			m_sprx_import_info.clear();
			m_sprx_export_info.clear();

			error_code res = handler::init(stream);

			if (res != ok)
			{
				return res;
			}

			m_stream->Read(&m_ehdr, sizeof(ehdr));

			if (!m_ehdr.check())
			{
				return bad_file;
			}

			if (m_ehdr.e_phnum && m_ehdr.e_phentsize != sizeof(phdr))
			{
				return broken_file;
			}

			if (m_ehdr.e_shnum && m_ehdr.e_shentsize != sizeof(shdr))
			{
				return broken_file;
			}

			if (m_ehdr.e_machine != MACHINE_PPC64 && m_ehdr.e_machine != MACHINE_SPU)
			{
				LOG_ERROR(LOADER, "Unknown elf64 machine type: 0x%x", m_ehdr.e_machine);
				return bad_version;
			}

			if (m_ehdr.e_phnum)
			{
				m_phdrs.resize(m_ehdr.e_phnum);
				m_stream->Seek(handler::get_stream_offset() + m_ehdr.e_phoff);
				if (m_stream->Read(m_phdrs.data(), m_ehdr.e_phnum * sizeof(phdr)) != m_ehdr.e_phnum * sizeof(phdr))
					return broken_file;
			}

			if (m_ehdr.e_shnum)
			{
				m_shdrs.resize(m_ehdr.e_shnum);
				m_stream->Seek(handler::get_stream_offset() + m_ehdr.e_shoff);
				if (m_stream->Read(m_shdrs.data(), m_ehdr.e_shnum * sizeof(shdr)) != m_ehdr.e_shnum * sizeof(shdr))
					return broken_file;
			}

			if (is_sprx())
			{
				m_stream->Seek(handler::get_stream_offset() + m_phdrs[0].p_paddr.addr());
				m_stream->Read(&m_sprx_module_info, sizeof(sprx_module_info));

				//m_stream->Seek(handler::get_stream_offset() + m_phdrs[1].p_vaddr.addr());
				//m_stream->Read(&m_sprx_function_info, sizeof(sprx_function_info));
			}

			return ok;
		}

		handler::error_code elf64::load_sprx(sprx_info& info)
		{
			for (auto &phdr : m_phdrs)
			{
				switch ((u32)phdr.p_type)
				{
				case 0x1: //load
				{
					if (phdr.p_memsz)
					{
						sprx_segment_info segment;
						segment.size = phdr.p_memsz;
						segment.size_file = phdr.p_filesz;

						segment.begin.set(vm::alloc(segment.size, vm::main));

						if (!segment.begin)
						{
							LOG_ERROR(LOADER, "%s() sprx: AllocFixed(0x%llx, 0x%x) failed", __FUNCTION__, phdr.p_vaddr.addr(), (u32)phdr.p_memsz);

							return loading_error;
						}

						segment.initial_addr.set(phdr.p_vaddr.addr());
						LOG_ERROR(LOADER, "segment addr=0x%x, initial addr = 0x%x", segment.begin.addr(), segment.initial_addr.addr());

						if (phdr.p_filesz)
						{
							m_stream->Seek(handler::get_stream_offset() + phdr.p_offset);
							m_stream->Read(segment.begin.get_ptr(), phdr.p_filesz);
						}

						if (phdr.p_paddr)
						{
							sys_prx_module_info_t module_info;
							m_stream->Seek(handler::get_stream_offset() + phdr.p_paddr.addr());
							m_stream->Read(&module_info, sizeof(module_info));
							LOG_ERROR(LOADER, "%s (%x):", module_info.name, (u32)module_info.toc);
							info.name = std::string(module_info.name, 28);
							info.rtoc = module_info.toc;

							int import_count = (module_info.imports_end - module_info.imports_start) / sizeof(sys_prx_library_info_t);

							if (import_count)
							{
								LOG_ERROR(LOADER, "**** Lib '%s' has %d imports!", module_info.name, import_count);
							}

							sys_prx_library_info_t lib;
							for (u32 e = module_info.exports_start.addr();
								e < module_info.exports_end.addr();
								e += lib.size ? lib.size : sizeof(sys_prx_library_info_t))
							{
								m_stream->Seek(handler::get_stream_offset() + phdr.p_offset + e);
								m_stream->Read(&lib, sizeof(lib));

								std::string modulename;
								if (lib.name_addr)
								{
									char name[27];
									m_stream->Seek(handler::get_stream_offset() + phdr.p_offset + lib.name_addr);
									m_stream->Read(name, sizeof(name));
									modulename = std::string(name);
									LOG_ERROR(LOADER, "**** %s", name);
								}

								auto &module = info.modules[modulename];

								LOG_ERROR(LOADER, "**** 0x%x - 0x%x - 0x%x", (u32)lib.unk4, (u32)lib.unk5, (u32)lib.unk6);

								for (u16 i = 0, end = lib.num_func; i < end; ++i)
								{
									be_t<u32> fnid, fstub;
									m_stream->Seek(handler::get_stream_offset() + phdr.p_offset + lib.fnid_addr + i * sizeof(fnid));
									m_stream->Read(&fnid, sizeof(fnid));

									m_stream->Seek(handler::get_stream_offset() + phdr.p_offset + lib.fstub_addr + i * sizeof(fstub));
									m_stream->Read(&fstub, sizeof(fstub));

									module.exports[fnid] = fstub;

									//LOG_NOTICE(LOADER, "Exported function '%s' in '%s' module  (LLE)", SysCalls::GetHLEFuncName(fnid).c_str(), module_name.c_str());
									LOG_ERROR(LOADER, "**** %s: [%s] -> 0x%x", modulename.c_str(), SysCalls::GetHLEFuncName(fnid).c_str(), (u32)fstub);
								}
							}

							for (u32 i = module_info.imports_start;
								i < module_info.imports_end;
								i += lib.size ? lib.size : sizeof(sys_prx_library_info_t))
							{
								m_stream->Seek(handler::get_stream_offset() + phdr.p_offset + i);
								m_stream->Read(&lib, sizeof(lib));
							}
						}

						info.segments.push_back(segment);
					}

					break;
				}

				case 0x700000a4: //relocation
				{
					m_stream->Seek(handler::get_stream_offset() + phdr.p_offset);

					for (uint i = 0; i < phdr.p_filesz; i += sizeof(sys_prx_relocation_info_t))
					{
						sys_prx_relocation_info_t rel;
						m_stream->Read(&rel, sizeof(rel));

						u32 ADDR = info.segments[rel.index_addr].begin.addr() + rel.offset;

						switch ((u32)rel.type)
						{
						case 1:
							LOG_WARNING(LOADER, "**** RELOCATION(1): 0x%x <- 0x%x", ADDR, (u32)(info.segments[rel.index_value].begin.addr() + rel.ptr.addr()));
							*vm::ptr<u32>::make(ADDR) = info.segments[rel.index_value].begin.addr() + rel.ptr.addr();
							break;

						case 4:
							LOG_WARNING(LOADER, "**** RELOCATION(4): 0x%x <- 0x%x", ADDR, (u16)(rel.ptr.addr()));
							*vm::ptr<u16>::make(ADDR) = (u16)(u64)rel.ptr.addr();
							break;

						case 5:
							LOG_WARNING(LOADER, "**** RELOCATION(5): 0x%x <- 0x%x", ADDR, (u16)(info.segments[rel.index_value].begin.addr() >> 16));
							*vm::ptr<u16>::make(ADDR) = info.segments[rel.index_value].begin.addr() >> 16;
							break;

						case 6:
							LOG_ERROR(LOADER, "**** RELOCATION(6): 0x%x <- 0x%x", ADDR, (u16)(info.segments[1].begin.addr() >> 16));
							*vm::ptr<u16>::make(ADDR) = info.segments[1].begin.addr() >> 16;
							break;

						default:
							LOG_ERROR(LOADER, "unknown prx relocation type (0x%x)", (u32)rel.type);
							return bad_relocation_type;
						}
					}

					break;
				}
				}
			}

			for (auto &m : info.modules)
			{
				for (auto &e : m.second.exports)
				{
					u32 stub = e.second;

					for (auto &s : info.segments)
					{
						if (stub >= s.initial_addr.addr() && stub < s.initial_addr.addr() + s.size_file)
						{
							stub += s.begin.addr() - s.initial_addr.addr();
							break;
						}
					}

					e.second = stub;
				}
			}

			return ok;
		}

		handler::error_code elf64::load()
		{
			if (is_sprx())
			{
				sprx_info info;
				return load_sprx(info);
			}

			Emu.m_sdk_version = -1;

			//store elf to memory
			vm::ps3::init();

			error_code res = alloc_memory(0);
			if (res != ok)
			{
				return res;
			}

			std::vector<u32> start_funcs;
			std::vector<u32> stop_funcs;

			//load modules
			vfsDir lle_dir("/dev_flash/sys/external");
			
			for (const auto module : lle_dir)
			{
				elf64 sprx_handler;

				vfsFile fsprx(lle_dir.GetPath() + "/" + module->name);

				if (fsprx.IsOpened())
				{
					sprx_handler.init(fsprx);

					if (sprx_handler.is_sprx())
					{
						IniEntry<bool> load_lib;
						load_lib.Init(sprx_handler.sprx_get_module_name(), "LLE");

						if (!load_lib.LoadValue(false))
						{
							LOG_WARNING(LOADER, "Skipped LLE library '%s'", sprx_handler.sprx_get_module_name().c_str());
							continue;
						}
						else
						{
							LOG_WARNING(LOADER, "Loading LLE library '%s'", sprx_handler.sprx_get_module_name().c_str());
						}

						sprx_info info;
						sprx_handler.load_sprx(info);

						for (auto &m : info.modules)
						{
							if (m.first == "")
							{
								for (auto &e : m.second.exports)
								{
									switch (e.first)
									{
									case 0xbc9a0086: start_funcs.push_back(e.second); break;
									case 0xab779874: stop_funcs.push_back(e.second); break;

									default: LOG_ERROR(LOADER, "unknown special func 0x%08x in '%s' library", e.first, info.name.c_str()); break;
									}
								}

								continue;
							}

							Module* module = Emu.GetModuleManager().GetModuleByName(m.first);

							if (!module)
							{
								LOG_ERROR(LOADER, "unknown module '%s' in '%s' library", m.first.c_str(), info.name.c_str());
								module = new Module(-1, m.first.c_str());
							}

							for (auto &e : m.second.exports)
							{
								module->RegisterLLEFunc(e.first, vm::ptr<void()>::make(e.second));
							}
						}
					}
				}
			}

			res = load_data(0);
			if (res != ok)
				return res;

			//initialize process
			auto rsx_callback_data = vm::ptr<u32>::make(Memory.MainMem.AllocAlign(4 * 4));
			*rsx_callback_data++ = (rsx_callback_data + 1).addr();
			Emu.SetRSXCallback(rsx_callback_data.addr());

			rsx_callback_data[0] = ADDI(r11, 0, 0x3ff);
			rsx_callback_data[1] = SC(0);
			rsx_callback_data[2] = BLR();

			auto ppu_thr_stop_data = vm::ptr<u32>::make(Memory.MainMem.AllocAlign(2 * 4));
			ppu_thr_stop_data[0] = SC(3);
			ppu_thr_stop_data[1] = BLR();
			Emu.SetCPUThreadStop(ppu_thr_stop_data.addr());

			/*
			//TODO
			static const int branch_size = 6 * 4;
			auto make_branch = [](vm::ptr<u32>& ptr, u32 addr)
			{
				u32 stub = vm::read32(addr);
				u32 rtoc = vm::read32(addr + 4);

				*ptr++ = implicts::LI(r0, stub >> 16);
				*ptr++ = ORIS(r0, r0, stub & 0xffff);
				*ptr++ = implicts::LI(r2, rtoc >> 16);
				*ptr++ = ORIS(r2, r2, rtoc & 0xffff);
				*ptr++ = MTCTR(r0);
				*ptr++ = BCTRL();
			};

			auto entry = vm::ptr<u32>::make(vm::alloc(branch_size * (start_funcs.size() + 1), vm::main));

			auto OPD =  vm::ptr<u32>::make(vm::alloc(2 * 4));
			OPD[0] = entry.addr();
			OPD[1] = 0;

			for (auto &f : start_funcs)
			{
				make_branch(entry, f);
			}

			make_branch(entry, m_ehdr.e_entry);
			*/

			ppu_thread main_thread(m_ehdr.e_entry, "main_thread");

			main_thread.args({ Emu.GetPath()/*, "-emu"*/ }).run();
			main_thread.gpr(11, m_ehdr.e_entry).gpr(12, Emu.GetMallocPageSize());

			return ok;
		}

		handler::error_code elf64::alloc_memory(u64 offset)
		{
			for (auto &phdr : m_phdrs)
			{
				switch (phdr.p_type.value())
				{
				case 0x00000001: //LOAD
				{
					if (phdr.p_memsz)
					{
						if (!vm::alloc(vm::cast(phdr.p_vaddr.addr()), vm::cast(phdr.p_memsz, "phdr.p_memsz"), vm::main))
						{
							LOG_ERROR(LOADER, "%s(): AllocFixed(0x%llx, 0x%llx) failed", __FUNCTION__, phdr.p_vaddr.addr(), phdr.p_memsz);

							return loading_error;
						}
					}
					break;
				}
				}
			}

			return ok;
		}

		handler::error_code elf64::load_data(u64 offset)
		{
			for (auto &phdr : m_phdrs)
			{
				switch (phdr.p_type.value())
				{
				case 0x00000001: //LOAD
				{
					if (phdr.p_memsz)
					{
						if (phdr.p_filesz)
						{
							m_stream->Seek(handler::get_stream_offset() + phdr.p_offset);
							m_stream->Read(phdr.p_vaddr.get_ptr(), phdr.p_filesz);
							Emu.GetSFuncManager().StaticAnalyse(phdr.p_vaddr.get_ptr(), (u32)phdr.p_filesz, phdr.p_vaddr.addr());
						}
					}
					break;
				}

				case 0x00000007: //TLS
				{
					Emu.SetTLSData(
						vm::cast(phdr.p_vaddr.addr(), "TLS: phdr.p_vaddr"),
						vm::cast(phdr.p_filesz.value(), "TLS: phdr.p_filesz"),
						vm::cast(phdr.p_memsz.value(), "TLS: phdr.p_memsz"));
					break;
				}

				case 0x60000001: //LOOS+1
				{
					if (phdr.p_filesz)
					{
						const sys_process_param& proc_param = *(sys_process_param*)phdr.p_vaddr.get_ptr();

						if (proc_param.size < sizeof(sys_process_param))
						{
							LOG_WARNING(LOADER, "Bad process_param size! [0x%x : 0x%x]", proc_param.size, sizeof(sys_process_param));
						}
						if (proc_param.magic != 0x13bcc5f6)
						{
							LOG_ERROR(LOADER, "Bad process_param magic! [0x%x]", proc_param.magic);
						}
						else
						{
							sys_process_param_info& info = Emu.GetInfo().GetProcParam();
							/*
							LOG_NOTICE(LOADER, "*** sdk version: 0x%x", info.sdk_version);
							LOG_NOTICE(LOADER, "*** primary prio: %d", info.primary_prio);
							LOG_NOTICE(LOADER, "*** primary stacksize: 0x%x", info.primary_stacksize);
							LOG_NOTICE(LOADER, "*** malloc pagesize: 0x%x", info.malloc_pagesize);
							LOG_NOTICE(LOADER, "*** ppc seg: 0x%x", info.ppc_seg);
							//LOG_NOTICE(LOADER, "*** crash dump param addr: 0x%x", info.crash_dump_param_addr);
							*/

							info = proc_param.info;
							Emu.m_sdk_version = info.sdk_version;
						}
					}
					break;
				}

				case 0x60000002: //LOOS+2
				{
					if (phdr.p_filesz)
					{
						const sys_proc_prx_param& proc_prx_param = *(sys_proc_prx_param*)phdr.p_vaddr.get_ptr();

						if (proc_prx_param.magic != 0x1b434cec)
						{
							LOG_ERROR(LOADER, "Bad magic! (0x%x)", proc_prx_param.magic);
							break;
						}

						for (auto stub = proc_prx_param.libstubstart; stub < proc_prx_param.libstubend; ++stub)
						{
							const std::string module_name = stub->s_modulename.get_ptr();
							Module* module = Emu.GetModuleManager().GetModuleByName(module_name);
							if (module)
							{
								//module->SetLoaded();
							}
							else
							{
								LOG_WARNING(LOADER, "Unknown module '%s'", module_name.c_str());
							}

							struct tbl_item
							{
								be_t<u32> stub;
								be_t<u32> rtoc;
							};

							struct stub_data_t
							{
								be_t<u32> data[3];
							}
							static const stub_data =
							{
								be_t<u32>::make(MR(11, 2)),
								be_t<u32>::make(SC(0)),
								be_t<u32>::make(BLR())
							};

							const auto& tbl = vm::get().alloc<tbl_item>(stub->s_imports);
							const auto& dst = vm::get().alloc<stub_data_t>(stub->s_imports);

							for (u32 i = 0; i < stub->s_imports; ++i)
							{
								const u32 nid = stub->s_nid[i];
								auto func = module ? module->GetFunc(nid) : nullptr;

								if (!func || !func->lle_func)
								{
									dst[i] = stub_data;

									tbl[i].stub = (dst + i).addr();
									tbl[i].rtoc = stub->s_nid[i];

									stub->s_text[i] = (tbl + i).addr();

									if (module && !module->Load(nid))
									{
										LOG_ERROR(LOADER, "Unimplemented function '%s' in '%s' module (HLE)", SysCalls::GetHLEFuncName(nid).c_str(), module_name.c_str());
									}
									else //if (Ini.HLELogging.GetValue())
									{
										LOG_NOTICE(LOADER, "Imported function '%s' in '%s' module  (HLE)", SysCalls::GetHLEFuncName(nid).c_str(), module_name.c_str());
									}
								}
								else
								{
									stub->s_text[i] = func->lle_func.addr();
									//Is function auto exported, than we can use it
									LOG_NOTICE(LOADER, "Imported function '%s' in '%s' module  (LLE: 0x%x)", SysCalls::GetHLEFuncName(nid).c_str(), module_name.c_str(), (u32)stub->s_text[i]);
								}
							}
						}
					}
					break;
				}
				}
			}

			return ok;
		}
	}
}
