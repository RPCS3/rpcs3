#include "stdafx.h"
#include "Utilities/Config.h"
#include "Loader/ELF.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "ARMv7Thread.h"
#include "ARMv7Opcodes.h"
#include "ARMv7Function.h"
#include "ARMv7Module.h"

LOG_CHANNEL(sceAppMgr);
LOG_CHANNEL(sceAppUtil);
LOG_CHANNEL(sceAudio);
LOG_CHANNEL(sceAudiodec);
LOG_CHANNEL(sceAudioenc);
LOG_CHANNEL(sceAudioIn);
LOG_CHANNEL(sceCamera);
LOG_CHANNEL(sceCodecEngine);
LOG_CHANNEL(sceCommonDialog);
LOG_CHANNEL(sceCtrl);
LOG_CHANNEL(sceDbg);
LOG_CHANNEL(sceDeci4p);
LOG_CHANNEL(sceDeflt);
LOG_CHANNEL(sceDisplay);
LOG_CHANNEL(sceFiber);
LOG_CHANNEL(sceFios);
LOG_CHANNEL(sceFpu);
LOG_CHANNEL(sceGxm);
LOG_CHANNEL(sceHttp);
LOG_CHANNEL(sceIme);
LOG_CHANNEL(sceJpeg);
LOG_CHANNEL(sceJpegEnc);
LOG_CHANNEL(sceLibc);
LOG_CHANNEL(sceLibKernel);
LOG_CHANNEL(sceLibm);
LOG_CHANNEL(sceLibstdcxx);
LOG_CHANNEL(sceLibXml);
LOG_CHANNEL(sceLiveArea);
LOG_CHANNEL(sceLocation);
LOG_CHANNEL(sceMd5);
LOG_CHANNEL(sceMotion);
LOG_CHANNEL(sceMt19937);
LOG_CHANNEL(sceNet);
LOG_CHANNEL(sceNetCtl);
LOG_CHANNEL(sceNgs);
LOG_CHANNEL(sceNpBasic);
LOG_CHANNEL(sceNpCommon);
LOG_CHANNEL(sceNpManager);
LOG_CHANNEL(sceNpMatching);
LOG_CHANNEL(sceNpScore);
LOG_CHANNEL(sceNpUtility);
LOG_CHANNEL(scePerf);
LOG_CHANNEL(scePgf);
LOG_CHANNEL(scePhotoExport);
LOG_CHANNEL(sceRazorCapture);
LOG_CHANNEL(sceRtc);
LOG_CHANNEL(sceSas);
LOG_CHANNEL(sceScreenShot);
LOG_CHANNEL(sceSfmt);
LOG_CHANNEL(sceSha);
LOG_CHANNEL(sceSqlite);
LOG_CHANNEL(sceSsl);
LOG_CHANNEL(sceSulpha);
LOG_CHANNEL(sceSysmodule);
LOG_CHANNEL(sceSystemGesture);
LOG_CHANNEL(sceTouch);
LOG_CHANNEL(sceUlt);
LOG_CHANNEL(sceVideodec);
LOG_CHANNEL(sceVoice);
LOG_CHANNEL(sceVoiceQoS);

extern void armv7_init_tls();

extern std::string arm_get_function_name(const std::string& module, u32 fnid);
extern std::string arm_get_variable_name(const std::string& module, u32 vnid);

// Function lookup table. Not supposed to grow after emulation start.
std::vector<arm_function_t> g_arm_function_cache;

std::vector<std::string> g_arm_function_names;

extern std::string arm_get_module_function_name(u32 index)
{
	if (index < g_arm_function_names.size())
	{
		return g_arm_function_names[index];
	}

	return fmt::format(".%u", index);
}

extern void arm_execute_function(ARMv7Thread& cpu, u32 index)
{
	if (index < g_arm_function_cache.size())
	{
		if (const auto func = g_arm_function_cache[index])
		{
			func(cpu);
			LOG_TRACE(ARMv7, "Function '%s' finished, r0=0x%x", arm_get_module_function_name(index), cpu.GPR[0]);
			return;
		}
	}

	throw fmt::exception("Function not registered (%u)" HERE, index);
}

arm_static_module::arm_static_module(const char* name)
	: name(name)
{
	arm_module_manager::register_module(this);
}

std::unordered_map<std::string, arm_static_module*>& arm_module_manager::access()
{
	static std::unordered_map<std::string, arm_static_module*> map;

	return map;
}

void arm_module_manager::register_module(arm_static_module* module)
{
	access().emplace(module->name, module);
}

arm_static_function& arm_module_manager::access_static_function(const char* module, u32 fnid)
{
	return access().at(module)->functions[fnid];
}

arm_static_variable& arm_module_manager::access_static_variable(const char* module, u32 vnid)
{
	return access().at(module)->variables[vnid];
}

const arm_static_module* arm_module_manager::get_module(const std::string& name)
{
	const auto& map = access();
	const auto found = map.find(name);
	return found != map.end() ? found->second : nullptr;
}

static void arm_initialize_modules()
{
	const std::initializer_list<const arm_static_module*> registered
	{
		&arm_module_manager::SceAppMgr,
		&arm_module_manager::SceAppUtil,
		&arm_module_manager::SceAudio,
		&arm_module_manager::SceAudiodec,
		&arm_module_manager::SceAudioenc,
		&arm_module_manager::SceAudioIn,
		&arm_module_manager::SceCamera,
		&arm_module_manager::SceCodecEngine,
		&arm_module_manager::SceCommonDialog,
		&arm_module_manager::SceCpu,
		&arm_module_manager::SceCtrl,
		&arm_module_manager::SceDbg,
		&arm_module_manager::SceDebugLed,
		&arm_module_manager::SceDeci4p,
		&arm_module_manager::SceDeflt,
		&arm_module_manager::SceDipsw,
		&arm_module_manager::SceDisplay,
		&arm_module_manager::SceDisplayUser,
		&arm_module_manager::SceFiber,
		&arm_module_manager::SceFios,
		&arm_module_manager::SceFpu,
		&arm_module_manager::SceGxm,
		&arm_module_manager::SceHttp,
		&arm_module_manager::SceIme,
		&arm_module_manager::SceIofilemgr,
		&arm_module_manager::SceJpeg,
		&arm_module_manager::SceJpegEnc,
		&arm_module_manager::SceLibc,
		&arm_module_manager::SceLibKernel,
		&arm_module_manager::SceLibm,
		&arm_module_manager::SceLibstdcxx,
		&arm_module_manager::SceLibXml,
		&arm_module_manager::SceLiveArea,
		&arm_module_manager::SceLocation,
		&arm_module_manager::SceMd5,
		&arm_module_manager::SceModulemgr,
		&arm_module_manager::SceMotion,
		&arm_module_manager::SceMt19937,
		&arm_module_manager::SceNet,
		&arm_module_manager::SceNetCtl,
		&arm_module_manager::SceNgs,
		&arm_module_manager::SceNpBasic,
		&arm_module_manager::SceNpCommon,
		&arm_module_manager::SceNpManager,
		&arm_module_manager::SceNpMatching,
		&arm_module_manager::SceNpScore,
		&arm_module_manager::SceNpUtility,
		&arm_module_manager::ScePerf,
		&arm_module_manager::ScePgf,
		&arm_module_manager::ScePhotoExport,
		&arm_module_manager::SceProcessmgr,
		&arm_module_manager::SceRazorCapture,
		&arm_module_manager::SceRtc,
		&arm_module_manager::SceSas,
		&arm_module_manager::SceScreenShot,
		&arm_module_manager::SceSfmt,
		&arm_module_manager::SceSha,
		&arm_module_manager::SceSqlite,
		&arm_module_manager::SceSsl,
		&arm_module_manager::SceStdio,
		&arm_module_manager::SceSulpha,
		&arm_module_manager::SceSysmem,
		&arm_module_manager::SceSysmodule,
		&arm_module_manager::SceSystemGesture,
		&arm_module_manager::SceThreadmgr,
		&arm_module_manager::SceTouch,
		&arm_module_manager::SceUlt,
		&arm_module_manager::SceVideodec,
		&arm_module_manager::SceVoice,
		&arm_module_manager::SceVoiceQoS,
	};

	// Reinitialize function cache
	g_arm_function_cache = arm_function_manager::get();
	g_arm_function_names.clear();
	g_arm_function_names.resize(g_arm_function_cache.size());

	// "Use" all the modules for correct linkage
	for (auto& module : registered)
	{
		LOG_TRACE(LOADER, "Registered static module: %s", module->name);

		for (auto& function : module->functions)
		{
			LOG_TRACE(LOADER, "** 0x%08X: %s", function.first, function.second.name);
			g_arm_function_names.at(function.second.index) = fmt::format("%s.%s", module->name, function.second.name);
		}

		for (auto& variable : module->variables)
		{
			LOG_TRACE(LOADER, "** &0x%08X: %s (size=0x%x, align=0x%x)", variable.first, variable.second.name, variable.second.size, variable.second.align);
			variable.second.var->set(0);
		}
	}
}

struct psv_moduleinfo_t
{
	le_t<u16> attr; // ???
	u8 major; // ???
	u8 minor; // ???
	char name[24]; // ???
	le_t<u32> unk0;
	le_t<u32> unk1;
	le_t<u32> libent_top;
	le_t<u32> libent_end;
	le_t<u32> libstub_top;
	le_t<u32> libstub_end;
	le_t<u32> data[1]; // ...
};

struct psv_libent_t
{
	le_t<u16> size; // ???
	le_t<u16> unk0;
	le_t<u16> unk1;
	le_t<u16> fcount;
	le_t<u16> vcount;
	le_t<u16> unk2;
	le_t<u32> unk3;
	le_t<u32> data[1]; // ...
};

struct psv_libstub_t
{
	le_t<u16> size; // 0x2C, 0x34
	le_t<u16> unk0; // (usually 1, 5 for sceLibKernel)
	le_t<u16> unk1; // (usually 0)
	le_t<u16> fcount;
	le_t<u16> vcount;
	le_t<u16> unk2;
	le_t<u32> unk3;
	le_t<u32> data[1]; // ...
};

struct psv_libcparam_t
{
	le_t<u32> size;
	le_t<u32> unk0;

	vm::lcptr<u32> sceLibcHeapSize;
	vm::lcptr<u32> sceLibcHeapSizeDefault;
	vm::lcptr<u32> sceLibcHeapExtendedAlloc;
	vm::lcptr<u32> sceLibcHeapDelayedAlloc;

	le_t<u32> unk1;
	le_t<u32> unk2;

	vm::lptr<u32> __sce_libcmallocreplace;
	vm::lptr<u32> __sce_libcnewreplace;
};

struct psv_process_param_t
{
	le_t<u32> size; // 0x00000030
	nse_t<u32> ver; // 'PSP2'
	le_t<u32> unk0; // 0x00000005
	le_t<u32> unk1;

	vm::lcptr<char> sceUserMainThreadName;
	vm::lcptr<s32>  sceUserMainThreadPriority;
	vm::lcptr<u32>  sceUserMainThreadStackSize;
	vm::lcptr<u32>  sceUserMainThreadAttribute;
	vm::lcptr<char> sceProcessName;
	vm::lcptr<u32>  sce_process_preload_disabled;
	vm::lcptr<u32>  sceUserMainThreadCpuAffinityMask;

	vm::lcptr<psv_libcparam_t> sce_libcparam;
};

static void arm_patch_refs(u32 refs, u32 addr)
{
	auto ptr = vm::cptr<u32>::make(refs);
	LOG_NOTICE(LOADER, "**** Processing refs at 0x%x:", ptr);

	if (ptr[0] != 0xff || ptr[1] != addr)
	{
		LOG_ERROR(LOADER, "**** Unexpected ref format ([0]=0x%x, [1]=0x%x)", ptr[0], ptr[1]);
	}
	else for (ptr += 2; *ptr; ptr++)
	{
		switch (u32 code = *ptr)
		{
		case 0x0000002f: // movw r*,# instruction
		{
			const u32 raddr = *++ptr;
			vm::write16(raddr + 0, vm::read16(raddr + 0) | (addr & 0x800) >> 1 | (addr & 0xf000) >> 12);
			vm::write16(raddr + 2, vm::read16(raddr + 2) | (addr & 0x700) << 4 | (addr & 0xff));

			LOG_NOTICE(LOADER, "**** MOVW written at *0x%x", raddr);
			break;
		}
		case 0x00000030: // movt r*,# instruction
		{
			const u32 raddr = *++ptr;
			vm::write16(raddr + 0, vm::read16(raddr + 0) | (addr & 0x8000000) >> 17 | (addr & 0xf0000000) >> 28);
			vm::write16(raddr + 2, vm::read16(raddr + 2) | (addr & 0x7000000) >> 12 | (addr & 0xff0000) >> 16);

			LOG_NOTICE(LOADER, "**** MOVT written at *0x%x", raddr);
			break;
		}
		default:
		{
			LOG_ERROR(LOADER, "**** Unknown ref code found (0x%08x)", code);
		}
		}
	}
	
}

template<>
void arm_exec_loader::load() const
{
	arm_initialize_modules();

	vm::cptr<psv_moduleinfo_t> module_info{};
	vm::cptr<psv_libent_t> libent{};
	vm::cptr<psv_libstub_t> libstub{};
	vm::cptr<psv_process_param_t> proc_param{};

	u32 entry_point{};
	u32 start_addr{};
	u32 arm_exidx{};
	u32 arm_extab{};
	u32 tls_faddr{};
	u32 tls_fsize{};
	u32 tls_vsize{};

	for (const auto& prog : progs)
	{
		if (prog.p_type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			if (!vm::falloc(prog.p_vaddr, prog.p_memsz, vm::main))
			{
				throw fmt::exception("vm::falloc() failed (addr=0x%x, size=0x%x)", prog.p_vaddr, prog.p_memsz);
			}

			if (prog.p_paddr)
			{
				module_info.set(prog.p_vaddr + (prog.p_paddr - prog.p_offset));
				LOG_NOTICE(LOADER, "Found program with p_paddr=0x%x", prog.p_paddr);
			}

			if (!start_addr)
			{
				start_addr = prog.p_vaddr;
			}

			std::memcpy(vm::base(prog.p_vaddr), prog.bin.data(), prog.p_filesz);
		}
	}

	if (!module_info) module_info.set(start_addr + header.e_entry);
	if (!libent) libent.set(start_addr + module_info->libent_top);
	if (!libstub) libstub.set(start_addr + module_info->libstub_top);

	LOG_NOTICE(LOADER, "__sce_moduleinfo(*0x%x) analysis...", module_info);

	if (module_info->data[2] == 0xffffffff)
	{
		arm_exidx = module_info->data[3];
		arm_extab = module_info->data[4];
		tls_faddr = module_info->data[5];
		tls_fsize = module_info->data[6];
		tls_vsize = module_info->data[7];
	}
	else if (module_info->data[5] == 0xffffffff)
	{
		tls_faddr = module_info->data[1]; // Guess
		tls_fsize = module_info->data[2];
		tls_vsize = module_info->data[3];
		arm_exidx = module_info->data[6];
		arm_extab = module_info->data[8];
	}
	else
	{
		LOG_ERROR(LOADER, "Failed to recognize __sce_moduleinfo format");
	}

	LOG_NOTICE(LOADER, "** arm_exidx=0x%x", arm_exidx);
	LOG_NOTICE(LOADER, "** arm_extab=0x%x", arm_extab);
	LOG_NOTICE(LOADER, "** tls_faddr=0x%x", tls_faddr);
	LOG_NOTICE(LOADER, "** tls_fsize=0x%x", tls_fsize);
	LOG_NOTICE(LOADER, "** tls_vsize=0x%x", tls_vsize);

	Emu.SetTLSData(tls_faddr + start_addr, tls_fsize, tls_vsize);

	// Process exports
	while (libent.addr() < start_addr + module_info->libent_end)
	{
		const u32 size = libent->size;

		// TODO: check addresses
		if (size != 0x1c && size != 0x20)
		{
			LOG_ERROR(LOADER, "Unknown libent size (0x%x) at *0x%x", libent->size, libent);
		}
		else
		{
			LOG_NOTICE(LOADER, "Loading libent at *0x%x", libent);
			LOG_NOTICE(LOADER, "** 0x%x, 0x%x", libent->unk0, libent->unk1);
			LOG_NOTICE(LOADER, "** Functions: %u", libent->fcount);
			LOG_NOTICE(LOADER, "** Variables: %u", libent->vcount);
			LOG_NOTICE(LOADER, "** 0x%x, 0x%08x", libent->unk2, libent->unk3);

			const auto export_nids = vm::cptr<u32>::make(libent->data[size == 0x20 ? 2 : 1]);
			const auto export_data = vm::cptr<u32>::make(libent->data[size == 0x20 ? 3 : 2]);

			for (u32 i = 0, count = export_data - export_nids; i < count; i++)
			{
				const u32 nid = export_nids[i];
				const u32 addr = export_data[i];

				// Known exports
				switch (nid)
				{
				case 0x935cd196: // set entry point
				{
					entry_point = addr;
					break;
				}

				case 0x6c2224ba: // __sce_moduleinfo
				{
					VERIFY(addr == module_info.addr());
					break;
				}

				case 0x70fba1e7: // __sce_process_param
				{
					proc_param.set(addr);
					break;
				}

				default:
				{
					LOG_ERROR(LOADER, "** Unknown export '0x%08X' (*0x%x)", nid, addr);
				}
				}
			}
		}

		// Next entry
		libent.set(libent.addr() + libent->size);
	}

	// Process imports
	while (libstub.addr() < start_addr + module_info->libstub_end)
	{
		const u32 size = libstub->size;

		// TODO: check addresses
		if (size != 0x2c && size != 0x34)
		{
			LOG_ERROR(LOADER, "Unknown libstub size (0x%x) at *0x%x)", libstub->size, libstub);
		}
		else
		{
			const std::string module_name(vm::_ptr<char>(libstub->data[size == 0x34 ? 1 : 0]));

			LOG_NOTICE(LOADER, "Loading libstub at 0x%x: %s", libstub, module_name);

			const auto _sm = arm_module_manager::get_module(module_name);

			if (!_sm)
			{
				LOG_ERROR(LOADER, "** Unknown module '%s'", module_name);
			}
			else
			{
				// Allocate HLE variables (TODO)
				for (auto& var : _sm->variables)
				{
					var.second.var->set(vm::alloc(var.second.size, vm::main, std::max<u32>(var.second.align, 4096)));
					LOG_WARNING(LOADER, "** Allocated variable '%s' in module '%s' at *0x%x", var.second.name, module_name, var.second.var->addr());
				}

				// Initialize HLE variables (TODO)
				for (auto& var : _sm->variables)
				{
					var.second.init();
				}
			}

			LOG_NOTICE(LOADER, "** 0x%x, 0x%x", libstub->unk0, libstub->unk1);
			LOG_NOTICE(LOADER, "** Functions: %u", libstub->fcount);
			LOG_NOTICE(LOADER, "** Variables: %u", libstub->vcount);
			LOG_NOTICE(LOADER, "** 0x%x, 0x%08x", libstub->unk2, libstub->unk3);

			const auto fnids = vm::cptr<u32>::make(libstub->data[size == 0x34 ? 3 : 1]);
			const auto fstubs = vm::cptr<u32>::make(libstub->data[size == 0x34 ? 4 : 2]);

			for (u32 j = 0; j < libstub->fcount; j++)
			{
				const u32 fnid = fnids[j];
				const u32 faddr = fstubs[j];

				u32 index = 0;

				const auto fstub = vm::ptr<u32>::make(faddr);
				const auto fname = arm_get_function_name(module_name, fnid);

				if (_sm && _sm->functions.count(fnid))
				{
					index = _sm->functions.at(fnid).index;
					LOG_NOTICE(LOADER, "** Imported function '%s' in module '%s' (*0x%x)", fname, module_name, faddr);
				}
				else
				{
					// TODO
					index = ::size32(g_arm_function_cache);
					g_arm_function_cache.emplace_back();
					g_arm_function_names.emplace_back(fmt::format("%s.%s", module_name, fname));

					LOG_ERROR(LOADER, "** Unknown function '%s' in module '%s' (*0x%x) -> index %u", fname, module_name, faddr, index);
				}

				// Check import stub
				if (fstub[2] != 0xE1A00000)
				{
					LOG_ERROR(LOADER, "**** Unexpected import function stub (*0x%x, [2]=0x%08x)", faddr, fstub[2]);
				}

				// Process refs
				if (const u32 refs = fstub[3])
				{
					arm_patch_refs(refs, faddr);
				}

				// Install HACK instruction (ARM)
				fstub[0] = 0xe0700090 | arm_code::hack<arm_encoding::A1>::index::insert(index);
			}

			const auto vnids = vm::cptr<u32>::make(libstub->data[size == 0x34 ? 5 : 3]);
			const auto vstub = vm::cptr<u32>::make(libstub->data[size == 0x34 ? 6 : 4]);

			for (u32 j = 0; j < libstub->vcount; j++)
			{
				const u32 vnid = vnids[j];
				const u32 refs = vstub[j];

				// Static variable
				if (const auto _sv = _sm && _sm->variables.count(vnid) ? &_sm->variables.at(vnid) : nullptr)
				{
					LOG_NOTICE(LOADER, "** Imported variable '%s' in module '%s' (refs=*0x%x)", arm_get_variable_name(module_name, vnid), module_name, refs);
					arm_patch_refs(refs, _sv->var->addr());
				}
				else
				{
					LOG_FATAL(LOADER, "** Unknown variable '%s' in module '%s' (refs=*0x%x)", arm_get_variable_name(module_name, vnid), module_name, refs);
				}
			}
		}

		// Next lib
		libstub.set(libstub.addr() + size);
	}

	LOG_NOTICE(LOADER, "__sce_process_param(*0x%x) analysis...", proc_param);

	VERIFY(proc_param->size >= sizeof(psv_process_param_t));
	VERIFY(proc_param->ver == "PSP2"_u32);

	LOG_NOTICE(LOADER, "*** size=0x%x; 0x%x, 0x%x, 0x%x", proc_param->size, proc_param->ver, proc_param->unk0, proc_param->unk1);

	LOG_NOTICE(LOADER, "*** &sceUserMainThreadName            = 0x%x", proc_param->sceUserMainThreadName);
	LOG_NOTICE(LOADER, "*** &sceUserMainThreadPriority        = 0x%x", proc_param->sceUserMainThreadPriority);
	LOG_NOTICE(LOADER, "*** &sceUserMainThreadStackSize       = 0x%x", proc_param->sceUserMainThreadStackSize);
	LOG_NOTICE(LOADER, "*** &sceUserMainThreadAttribute       = 0x%x", proc_param->sceUserMainThreadAttribute);
	LOG_NOTICE(LOADER, "*** &sceProcessName                   = 0x%x", proc_param->sceProcessName);
	LOG_NOTICE(LOADER, "*** &sce_process_preload_disabled     = 0x%x", proc_param->sce_process_preload_disabled);
	LOG_NOTICE(LOADER, "*** &sceUserMainThreadCpuAffinityMask = 0x%x", proc_param->sceUserMainThreadCpuAffinityMask);

	const auto libc_param = proc_param->sce_libcparam;

	LOG_NOTICE(LOADER, "__sce_libcparam(*0x%x) analysis...", libc_param);

	VERIFY(libc_param->size >= 0x1c);

	LOG_NOTICE(LOADER, "*** size=0x%x; 0x%x, 0x%x, 0x%x", libc_param->size, libc_param->unk0, libc_param->unk1, libc_param->unk2);

	LOG_NOTICE(LOADER, "*** &sceLibcHeapSize                  = 0x%x", libc_param->sceLibcHeapSize);
	LOG_NOTICE(LOADER, "*** &sceLibcHeapSizeDefault           = 0x%x", libc_param->sceLibcHeapSizeDefault);
	LOG_NOTICE(LOADER, "*** &sceLibcHeapExtendedAlloc         = 0x%x", libc_param->sceLibcHeapExtendedAlloc);
	LOG_NOTICE(LOADER, "*** &sceLibcHeapDelayedAlloc          = 0x%x", libc_param->sceLibcHeapDelayedAlloc);

	const auto stop_code = vm::ptr<u32>::make(vm::alloc(3 * 4, vm::main));
	stop_code[0] = 0xf870; // HACK instruction (Thumb)
	stop_code[1] = 1; // Predefined function index (HLE return)
	Emu.SetCPUThreadStop(stop_code.addr());

	armv7_init_tls();

	const std::string& thread_name = proc_param->sceUserMainThreadName ? proc_param->sceUserMainThreadName.get_ptr() : "main_thread";
	const u32 stack_size = proc_param->sceUserMainThreadStackSize ? proc_param->sceUserMainThreadStackSize->value() : 256 * 1024;
	const u32 priority = proc_param->sceUserMainThreadPriority ? proc_param->sceUserMainThreadPriority->value() : 160;

	auto thread = idm::make_ptr<ARMv7Thread>(thread_name);

	thread->PC = entry_point;
	thread->stack_size = stack_size;
	thread->prio = priority;
	thread->cpu_init();

	// Initialize args
	std::vector<char> argv_data;

	for (const auto& arg : { Emu.GetPath(), "-emu"s })
	{
		argv_data.insert(argv_data.end(), arg.begin(), arg.end());
		argv_data.insert(argv_data.end(), '\0');

		thread->GPR[0]++; // argc
	}

	const u32 argv = vm::alloc(::size32(argv_data), vm::main);
	std::memcpy(vm::base(argv), argv_data.data(), argv_data.size()); // copy arg list
	thread->GPR[1] = argv;
}
