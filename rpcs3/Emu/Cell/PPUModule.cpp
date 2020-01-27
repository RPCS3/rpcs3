#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "Utilities/VirtualMemory.h"
#include "Utilities/bin_patch.h"
#include "Crypto/sha1.h"
#include "Crypto/unself.h"
#include "Loader/ELF.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPUAnalyser.h"

#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_overlay.h"

#include "Emu/Cell/Modules/StaticHLE.h"

#include <map>
#include <set>
#include <algorithm>



extern void ppu_initialize_syscalls();
extern std::string ppu_get_function_name(const std::string& module, u32 fnid);
extern std::string ppu_get_variable_name(const std::string& module, u32 vnid);
extern void ppu_register_range(u32 addr, u32 size);
extern void ppu_register_function_at(u32 addr, u32 size, ppu_function_t ptr);
extern void ppu_initialize(const ppu_module& info);
extern void ppu_initialize();

extern void sys_initialize_tls(ppu_thread&, u64, u32, u32, u32);

// HLE function name cache
std::vector<std::string> g_ppu_function_names;

template <>
void fmt_class_string<lib_loading_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](lib_loading_type value)
	{
		switch (value)
		{
		case lib_loading_type::manual: return "Manually load selected libraries";
		case lib_loading_type::hybrid: return "Load automatic and manual selection";
		case lib_loading_type::liblv2only: return "Load liblv2.sprx only";
		case lib_loading_type::liblv2both: return "Load liblv2.sprx and manual selection";
		case lib_loading_type::liblv2list: return "Load liblv2.sprx and strict selection";
		}

		return unknown;
	});
}

extern u32 ppu_generate_id(const char* name)
{
	// Symbol name suffix
	const auto suffix = "\x67\x59\x65\x99\x04\x25\x04\x90\x56\x64\x27\x49\x94\x89\x74\x1A";

	sha1_context ctx;
	u8 output[20];

	// Compute SHA-1 hash
	sha1_starts(&ctx);
	sha1_update(&ctx, reinterpret_cast<const u8*>(name), std::strlen(name));
	sha1_update(&ctx, reinterpret_cast<const u8*>(suffix), std::strlen(suffix));
	sha1_finish(&ctx, output);

	return reinterpret_cast<le_t<u32>&>(output[0]);
}

ppu_static_module::ppu_static_module(const char* name)
	: name(name)
{
	ppu_module_manager::register_module(this);
}

std::unordered_map<std::string, ppu_static_module*>& ppu_module_manager::access()
{
	static std::unordered_map<std::string, ppu_static_module*> map;

	return map;
}

void ppu_module_manager::register_module(ppu_static_module* module)
{
	access().emplace(module->name, module);
}

ppu_static_function& ppu_module_manager::access_static_function(const char* module, u32 fnid)
{
	auto& res = access().at(module)->functions[fnid];

	if (res.name)
	{
		fmt::throw_exception("PPU FNID duplication in module %s (%s, 0x%x)", module, res.name, fnid);
	}

	return res;
}

ppu_static_variable& ppu_module_manager::access_static_variable(const char* module, u32 vnid)
{
	auto& res = access().at(module)->variables[vnid];

	if (res.name)
	{
		fmt::throw_exception("PPU VNID duplication in module %s (%s, 0x%x)", module, res.name, vnid);
	}

	return res;
}

const ppu_static_module* ppu_module_manager::get_module(const std::string& name)
{
	const auto& map = access();
	const auto found = map.find(name);
	return found != map.end() ? found->second : nullptr;
}

// Global linkage information
struct ppu_linkage_info
{
	struct module
	{
		struct info
		{
			ppu_static_function* static_func = nullptr;
			ppu_static_variable* static_var = nullptr;
			u32 export_addr = 0;
			std::set<u32> imports;
			std::set<u32> frefss;
		};

		// FNID -> (export; [imports...])
		std::unordered_map<u32, info, value_hash<u32>> functions;
		std::unordered_map<u32, info, value_hash<u32>> variables;

		// Obsolete
		bool imported = false;
	};

	// Module map
	std::unordered_map<std::string, module> modules;
};

// Initialize static modules.
static void ppu_initialize_modules(ppu_linkage_info* link)
{
	if (!link->modules.empty())
	{
		return;
	}

	ppu_initialize_syscalls();

	const std::initializer_list<const ppu_static_module*> registered
	{
		&ppu_module_manager::cellAdec,
		&ppu_module_manager::cellAtrac,
		&ppu_module_manager::cellAtracMulti,
		&ppu_module_manager::cellAudio,
		&ppu_module_manager::cellAvconfExt,
		&ppu_module_manager::cellBGDL,
		&ppu_module_manager::cellCamera,
		&ppu_module_manager::cellCelp8Enc,
		&ppu_module_manager::cellCelpEnc,
		&ppu_module_manager::cellCrossController,
		&ppu_module_manager::cellDaisy,
		&ppu_module_manager::cellDmux,
		&ppu_module_manager::cellDtcpIpUtility,
		&ppu_module_manager::cellFiber,
		&ppu_module_manager::cellFont,
		&ppu_module_manager::cellFontFT,
		&ppu_module_manager::cell_FreeType2,
		&ppu_module_manager::cellFs,
		&ppu_module_manager::cellGame,
		&ppu_module_manager::cellGameExec,
		&ppu_module_manager::cellGcmSys,
		&ppu_module_manager::cellGem,
		&ppu_module_manager::cellGifDec,
		&ppu_module_manager::cellHttp,
		&ppu_module_manager::cellHttps,
		&ppu_module_manager::cellHttpUtil,
		&ppu_module_manager::cellImeJp,
		&ppu_module_manager::cellJpgDec,
		&ppu_module_manager::cellJpgEnc,
		&ppu_module_manager::cellKey2char,
		&ppu_module_manager::cellL10n,
		&ppu_module_manager::cellLibprof,
		&ppu_module_manager::cellMic,
		&ppu_module_manager::cellMusic,
		&ppu_module_manager::cellMusicDecode,
		&ppu_module_manager::cellMusicExport,
		&ppu_module_manager::cellNetAoi,
		&ppu_module_manager::cellNetCtl,
		&ppu_module_manager::cellOskDialog,
		&ppu_module_manager::cellOvis,
		&ppu_module_manager::cellPamf,
		&ppu_module_manager::cellPesmUtility,
		&ppu_module_manager::cellPhotoDecode,
		&ppu_module_manager::cellPhotoExport,
		&ppu_module_manager::cellPhotoImportUtil,
		&ppu_module_manager::cellPngDec,
		&ppu_module_manager::cellPngEnc,
		&ppu_module_manager::cellPrint,
		&ppu_module_manager::cellRec,
		&ppu_module_manager::cellRemotePlay,
		&ppu_module_manager::cellResc,
		&ppu_module_manager::cellRtc,
		&ppu_module_manager::cellRtcAlarm,
		&ppu_module_manager::cellRudp,
		&ppu_module_manager::cellSail,
		&ppu_module_manager::cellSailRec,
		&ppu_module_manager::cellSaveData,
		&ppu_module_manager::cellMinisSaveData,
		&ppu_module_manager::cellScreenShot,
		&ppu_module_manager::cellSearch,
		&ppu_module_manager::cellSheap,
		&ppu_module_manager::cellSpudll,
		&ppu_module_manager::cellSpurs,
		&ppu_module_manager::cellSpursJq,
		&ppu_module_manager::cellSsl,
		&ppu_module_manager::cellSubDisplay,
		&ppu_module_manager::cellSync,
		&ppu_module_manager::cellSync2,
		&ppu_module_manager::cellSysconf,
		&ppu_module_manager::cellSysmodule,
		&ppu_module_manager::cellSysutil,
		&ppu_module_manager::cellSysutilAp,
		&ppu_module_manager::cellSysutilAvc2,
		&ppu_module_manager::cellSysutilAvcExt,
		&ppu_module_manager::cellSysutilNpEula,
		&ppu_module_manager::cellSysutilMisc,
		&ppu_module_manager::cellUsbd,
		&ppu_module_manager::cellUsbPspcm,
		&ppu_module_manager::cellUserInfo,
		&ppu_module_manager::cellVdec,
		&ppu_module_manager::cellVideoExport,
		&ppu_module_manager::cellVideoPlayerUtility,
		&ppu_module_manager::cellVideoUpload,
		&ppu_module_manager::cellVoice,
		&ppu_module_manager::cellVpost,
		&ppu_module_manager::libad_async,
		&ppu_module_manager::libad_core,
		&ppu_module_manager::libmedi,
		&ppu_module_manager::libmixer,
		&ppu_module_manager::libsnd3,
		&ppu_module_manager::libsynth2,
		&ppu_module_manager::sceNp,
		&ppu_module_manager::sceNp2,
		&ppu_module_manager::sceNpClans,
		&ppu_module_manager::sceNpCommerce2,
		&ppu_module_manager::sceNpMatchingInt,
		&ppu_module_manager::sceNpSns,
		&ppu_module_manager::sceNpTrophy,
		&ppu_module_manager::sceNpTus,
		&ppu_module_manager::sceNpUtil,
		&ppu_module_manager::sys_io,
		&ppu_module_manager::sys_net,
		&ppu_module_manager::sysPrxForUser,
		&ppu_module_manager::sys_libc,
		&ppu_module_manager::sys_lv2dbg,
		&ppu_module_manager::static_hle,
	};

	// Initialize double-purpose fake OPD array for HLE functions
	const auto& hle_funcs = ppu_function_manager::get();

	// Allocate memory for the array (must be called after fixed allocations)
	ppu_function_manager::addr = vm::alloc(::size32(hle_funcs) * 8, vm::main);

	// Initialize as PPU executable code
	ppu_register_range(ppu_function_manager::addr, ::size32(hle_funcs) * 8);

	// Fill the array (visible data: self address and function index)
	for (u32 addr = ppu_function_manager::addr, index = 0; index < hle_funcs.size(); addr += 8, index++)
	{
		// Function address = current address, RTOC = BLR instruction for the interpreter
		vm::write32(addr + 0, addr);
		vm::write32(addr + 4, ppu_instructions::BLR());

		// Register the HLE function directly
		ppu_register_function_at(addr + 0, 4, hle_funcs[index]);
		ppu_register_function_at(addr + 4, 4, nullptr);
	}

	// Set memory protection to read-only
	vm::page_protect(ppu_function_manager::addr, ::align(::size32(hle_funcs) * 8, 0x1000), 0, 0, vm::page_writable);

	// Initialize function names
	const bool is_first = g_ppu_function_names.empty();

	if (is_first)
	{
		g_ppu_function_names.resize(hle_funcs.size());
		g_ppu_function_names[0] = "INVALID";
		g_ppu_function_names[1] = "HLE RETURN";
	}

	// For HLE variable allocation
	u32 alloc_addr = 0;

	// "Use" all the modules for correct linkage
	for (auto& module : registered)
	{
		LOG_TRACE(LOADER, "Registered static module: %s", module->name);
	}

	for (auto& pair : ppu_module_manager::get())
	{
		const auto module = pair.second;
		auto& linkage = link->modules[module->name];

		for (auto& function : module->functions)
		{
			LOG_TRACE(LOADER, "** 0x%08X: %s", function.first, function.second.name);

			if (is_first)
			{
				g_ppu_function_names[function.second.index] = fmt::format("%s.%s", module->name, function.second.name);
			}

			if ((function.second.flags & MFF_HIDDEN) == 0)
			{
				auto& flink = linkage.functions[function.first];

				flink.static_func = &function.second;
				flink.export_addr = ppu_function_manager::addr + 8 * function.second.index;
				function.second.export_addr = &flink.export_addr;
			}
		}

		for (auto& variable : module->variables)
		{
			LOG_TRACE(LOADER, "** &0x%08X: %s (size=0x%x, align=0x%x)", variable.first, variable.second.name, variable.second.size, variable.second.align);

			// Allocate HLE variable
			if (variable.second.size >= 4096 || variable.second.align >= 4096)
			{
				variable.second.addr = vm::alloc(variable.second.size, vm::main, std::max<u32>(variable.second.align, 0x10000));
			}
			else
			{
				const u32 next = ::align(alloc_addr, variable.second.align);
				const u32 end = next + variable.second.size;

				if (!next || (end >> 12 != alloc_addr >> 12))
				{
					alloc_addr = vm::alloc(4096, vm::main);
				}
				else
				{
					alloc_addr = next;
				}

				variable.second.addr = alloc_addr;
				alloc_addr += variable.second.size;
			}

			if (variable.second.var)
			{
				variable.second.var->set(variable.second.addr);
			}

			LOG_TRACE(LOADER, "Allocated HLE variable %s.%s at 0x%x", module->name, variable.second.name, variable.second.var->addr());

			// Initialize HLE variable
			if (variable.second.init)
			{
				variable.second.init();
			}

			if ((variable.second.flags & MFF_HIDDEN) == 0)
			{
				auto& vlink = linkage.variables[variable.first];

				vlink.static_var = &variable.second;
				vlink.export_addr = variable.second.addr;
				variable.second.export_addr = &vlink.export_addr;
			}
		}
	}
}

// Resolve relocations for variable/function linkage.
static void ppu_patch_refs(std::vector<ppu_reloc>* out_relocs, u32 fref, u32 faddr)
{
	struct ref_t
	{
		be_t<u32> type;
		be_t<u32> addr;
		be_t<u32> addend; // Note: Treating it as addend seems to be correct for now, but still unknown if theres more in this variable
	};

	for (auto ref = vm::ptr<ref_t>::make(fref); ref->type; ref++)
	{
		if (ref->addend) LOG_WARNING(LOADER, "**** REF(%u): Addend value(0x%x, 0x%x)", ref->type, ref->addr, ref->addend);

		const u32 raddr = ref->addr;
		const u32 rtype = ref->type;
		const u32 rdata = faddr + ref->addend;

		if (out_relocs)
		{
			// Register relocation with unpredictable target (data=0)
			ppu_reloc _rel;
			_rel.addr = raddr;
			_rel.type = rtype;
			_rel.data = 0;
			out_relocs->emplace_back(_rel);
		}

		// OPs must be similar to relocations
		switch (rtype)
		{
		case 1:
		{
			const u32 value = vm::_ref<u32>(ref->addr) = rdata;
			LOG_TRACE(LOADER, "**** REF(1): 0x%x <- 0x%x", ref->addr, value);
			break;
		}

		case 4:
		{
			const u16 value = vm::_ref<u16>(ref->addr) = static_cast<u16>(rdata);
			LOG_TRACE(LOADER, "**** REF(4): 0x%x <- 0x%04x (0x%llx)", ref->addr, value, faddr);
			break;
		}

		case 6:
		{
			const u16 value = vm::_ref<u16>(ref->addr) = static_cast<u16>(rdata >> 16) + (rdata & 0x8000 ? 1 : 0);
			LOG_TRACE(LOADER, "**** REF(6): 0x%x <- 0x%04x (0x%llx)", ref->addr, value, faddr);
			break;
		}

		case 57:
		{
			const u16 value = vm::_ref<ppu_bf_t<be_t<u16>, 0, 14>>(ref->addr) = static_cast<u16>(rdata) >> 2;
			LOG_TRACE(LOADER, "**** REF(57): 0x%x <- 0x%04x (0x%llx)", ref->addr, value, faddr);
			break;
		}

		default: LOG_ERROR(LOADER, "**** REF(%u): Unknown/Illegal type (0x%x, 0x%x)", rtype, raddr, ref->addend);
		}
	}
}

// Export or import module struct
struct ppu_prx_module_info
{
	u8 size;
	u8 unk0;
	be_t<u16> version;
	be_t<u16> attributes;
	be_t<u16> num_func;
	be_t<u16> num_var;
	be_t<u16> num_tlsvar;
	u8 info_hash;
	u8 info_tlshash;
	u8 unk1[2];
	vm::bcptr<char> name;
	vm::bcptr<u32> nids; // Imported FNIDs, Exported NIDs
	vm::bptr<u32> addrs;
	vm::bcptr<u32> vnids; // Imported VNIDs
	vm::bcptr<u32> vstubs;
	be_t<u32> unk4;
	be_t<u32> unk5;
};

// Load and register exports; return special exports found (nameless module)
static auto ppu_load_exports(ppu_linkage_info* link, u32 exports_start, u32 exports_end)
{
	std::unordered_map<u32, u32> result;

	for (u32 addr = exports_start; addr < exports_end;)
	{
		const auto& lib = vm::_ref<const ppu_prx_module_info>(addr);

		if (!lib.name)
		{
			// Set special exports
			for (u32 i = 0, end = lib.num_func + lib.num_var; i < end; i++)
			{
				const u32 nid = lib.nids[i];
				const u32 addr = lib.addrs[i];

				if (i < lib.num_func)
				{
					LOG_NOTICE(LOADER, "** Special: [%s] at 0x%x", ppu_get_function_name({}, nid), addr);
				}
				else
				{
					LOG_NOTICE(LOADER, "** Special: &[%s] at 0x%x", ppu_get_variable_name({}, nid), addr);
				}

				result.emplace(nid, addr);
			}

			addr += lib.size ? lib.size : sizeof(ppu_prx_module_info);
			continue;
		}

		const std::string module_name(lib.name.get_ptr());

		LOG_NOTICE(LOADER, "** Exported module '%s' (0x%x, 0x%x, 0x%x, 0x%x)", module_name, lib.vnids, lib.vstubs, lib.unk4, lib.unk5);

		if (lib.num_tlsvar)
		{
			LOG_FATAL(LOADER, "Unexpected num_tlsvar (%u)!", lib.num_tlsvar);
		}

		// Static module
		const auto _sm = ppu_module_manager::get_module(module_name);

		// Module linkage
		auto& mlink = link->modules[module_name];

		const auto fnids = +lib.nids;
		const auto faddrs = +lib.addrs;

		// Get functions
		for (u32 i = 0, end = lib.num_func; i < end; i++)
		{
			const u32 fnid = fnids[i];
			const u32 faddr = faddrs[i];
			LOG_NOTICE(LOADER, "**** %s export: [%s] at 0x%x", module_name, ppu_get_function_name(module_name, fnid), faddr);

			// Function linkage info
			auto& flink = mlink.functions[fnid];

			if (flink.static_func && flink.export_addr == ppu_function_manager::addr + 8 * flink.static_func->index)
			{
				flink.export_addr = 0;
			}

			if (flink.export_addr)
			{
				LOG_ERROR(LOADER, "Already linked function '%s' in module '%s'", ppu_get_function_name(module_name, fnid), module_name);
			}
			//else
			{
				// Static function
				const auto _sf = _sm && _sm->functions.count(fnid) ? &_sm->functions.at(fnid) : nullptr;

				if (_sf && (_sf->flags & MFF_FORCED_HLE))
				{
					// Inject a branch to the HLE implementation
					const u32 _entry = vm::read32(faddr);
					const u32 target = ppu_function_manager::addr + 8 * _sf->index;

					if ((target <= _entry && _entry - target <= 0x2000000) || (target > _entry && target - _entry < 0x2000000))
					{
						// Use relative branch
						vm::write32(_entry, ppu_instructions::B(target - _entry));
					}
					else if (target < 0x2000000)
					{
						// Use absolute branch if possible
						vm::write32(_entry, ppu_instructions::B(target, true));
					}
					else
					{
						LOG_FATAL(LOADER, "Failed to patch function at 0x%x (0x%x)", _entry, target);
					}
				}
				else
				{
					// Set exported function
					flink.export_addr = faddr;

					// Fix imports
					for (const u32 addr : flink.imports)
					{
						vm::write32(addr, faddr);
						//LOG_WARNING(LOADER, "Exported function '%s' in module '%s'", ppu_get_function_name(module_name, fnid), module_name);
					}

					for (const u32 fref : flink.frefss)
					{
						ppu_patch_refs(nullptr, fref, faddr);
					}
				}
			}
		}

		const auto vnids = lib.nids + lib.num_func;
		const auto vaddrs = lib.addrs + lib.num_func;

		// Get variables
		for (u32 i = 0, end = lib.num_var; i < end; i++)
		{
			const u32 vnid = vnids[i];
			const u32 vaddr = vaddrs[i];
			LOG_NOTICE(LOADER, "**** %s export: &[%s] at 0x%x", module_name, ppu_get_variable_name(module_name, vnid), vaddr);

			// Variable linkage info
			auto& vlink = mlink.variables[vnid];

			if (vlink.static_var && vlink.export_addr == vlink.static_var->addr)
			{
				vlink.export_addr = 0;
			}

			if (vlink.export_addr)
			{
				LOG_ERROR(LOADER, "Already linked variable '%s' in module '%s'", ppu_get_variable_name(module_name, vnid), module_name);
			}
			//else
			{
				// Set exported variable
				vlink.export_addr = vaddr;

				// Fix imports
				for (const auto vref : vlink.imports)
				{
					ppu_patch_refs(nullptr, vref, vaddr);
					//LOG_WARNING(LOADER, "Exported variable '%s' in module '%s'", ppu_get_variable_name(module_name, vnid), module_name);
				}
			}
		}

		addr += lib.size ? lib.size : sizeof(ppu_prx_module_info);
	}

	return result;
}

static auto ppu_load_imports(std::vector<ppu_reloc>& relocs, ppu_linkage_info* link, u32 imports_start, u32 imports_end)
{
	std::unordered_map<u32, void*> result;

	for (u32 addr = imports_start; addr < imports_end;)
	{
		const auto& lib = vm::_ref<const ppu_prx_module_info>(addr);

		const std::string module_name(lib.name.get_ptr());

		LOG_NOTICE(LOADER, "** Imported module '%s' (ver=0x%x, attr=0x%x, 0x%x, 0x%x) [0x%x]", module_name, lib.version, lib.attributes, lib.unk4, lib.unk5, addr);

		if (lib.num_tlsvar)
		{
			LOG_FATAL(LOADER, "Unexpected num_tlsvar (%u)!", lib.num_tlsvar);
		}

		// Static module
		const auto _sm = ppu_module_manager::get_module(module_name);

		// Module linkage
		auto& mlink = link->modules[module_name];

		const auto fnids = +lib.nids;
		const auto faddrs = +lib.addrs;

		for (u32 i = 0, end = lib.num_func; i < end; i++)
		{
			const u32 fnid = fnids[i];
			const u32 fstub = faddrs[i];
			const u32 faddr = (faddrs + i).addr();
			LOG_NOTICE(LOADER, "**** %s import: [%s] -> 0x%x", module_name, ppu_get_function_name(module_name, fnid), fstub);

			// Function linkage info
			auto& flink = link->modules[module_name].functions[fnid];

			// Add new import
			result.emplace(faddr, &flink);
			flink.imports.emplace(faddr);
			mlink.imported = true;

			// Link address (special HLE function by default)
			const u32 link_addr = flink.export_addr ? flink.export_addr : ppu_function_manager::addr;

			// Write import table
			vm::write32(faddr, link_addr);

			// Patch refs if necessary (0x2000 seems to be correct flag indicating the presence of additional info)
			if (const u32 frefs = (lib.attributes & 0x2000) ? +fnids[i + lib.num_func] : 0)
			{
				result.emplace(frefs, &flink);
				flink.frefss.emplace(frefs);
				ppu_patch_refs(&relocs, frefs, link_addr);
			}

			//LOG_WARNING(LOADER, "Imported function '%s' in module '%s' (0x%x)", ppu_get_function_name(module_name, fnid), module_name, faddr);
		}

		const auto vnids = +lib.vnids;
		const auto vstubs = +lib.vstubs;

		for (u32 i = 0, end = lib.num_var; i < end; i++)
		{
			const u32 vnid = vnids[i];
			const u32 vref = vstubs[i];
			LOG_NOTICE(LOADER, "**** %s import: &[%s] (ref=*0x%x)", module_name, ppu_get_variable_name(module_name, vnid), vref);

			// Variable linkage info
			auto& vlink = link->modules[module_name].variables[vnid];

			// Add new import
			result.emplace(vref, &vlink);
			vlink.imports.emplace(vref);
			mlink.imported = true;

			// Link if available
			ppu_patch_refs(&relocs, vref, vlink.export_addr);

			//LOG_WARNING(LOADER, "Imported variable '%s' in module '%s' (0x%x)", ppu_get_variable_name(module_name, vnid), module_name, vlink.first);
		}

		addr += lib.size ? lib.size : sizeof(ppu_prx_module_info);
	}

	return result;
}

std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object& elf, const std::string& path)
{
	// Create new PRX object
	const auto prx = idm::make_ptr<lv2_obj, lv2_prx>();

	// Access linkage information object
	const auto link = g_fxo->get<ppu_linkage_info>();

	// Initialize HLE modules
	ppu_initialize_modules(link);

	// Library hash
	sha1_context sha;
	sha1_starts(&sha);

	for (const auto& prog : elf.progs)
	{
		LOG_NOTICE(LOADER, "** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

		// Hash big-endian values
		sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_type), sizeof(prog.p_type));
		sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_flags), sizeof(prog.p_flags));

		switch (const u32 p_type = prog.p_type)
		{
		case 0x1: // LOAD
		{
			if (prog.p_memsz)
			{
				const u32 mem_size = ::narrow<u32>(prog.p_memsz, "p_memsz" HERE);
				const u32 file_size = ::narrow<u32>(prog.p_filesz, "p_filesz" HERE);
				const u32 init_addr = ::narrow<u32>(prog.p_vaddr, "p_vaddr" HERE);

				// Alloc segment memory
				const u32 addr = vm::alloc(mem_size, vm::main);

				if (!addr)
				{
					fmt::throw_exception("vm::alloc() failed (size=0x%x)", mem_size);
				}

				// Copy segment data
				std::memcpy(vm::base(addr), prog.bin.data(), file_size);
				LOG_WARNING(LOADER, "**** Loaded to 0x%x (size=0x%x)", addr, mem_size);

				// Hash segment
				sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_vaddr), sizeof(prog.p_vaddr));
				sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_memsz), sizeof(prog.p_memsz));
				sha1_update(&sha, prog.bin.data(), prog.bin.size());

				// Initialize executable code if necessary
				if (prog.p_flags & 0x1)
				{
					ppu_register_range(addr, mem_size);
				}

				ppu_segment _seg;
				_seg.addr = addr;
				_seg.size = mem_size;
				_seg.filesz = file_size;
				_seg.type = p_type;
				_seg.flags = prog.p_flags;
				prx->segs.emplace_back(_seg);
			}

			break;
		}

		case 0x700000a4: break; // Relocations

		default: LOG_ERROR(LOADER, "Unknown segment type! 0x%08x", p_type);
		}
	}

	for (const auto& s : elf.shdrs)
	{
		LOG_NOTICE(LOADER, "** Section: sh_type=0x%x, addr=0x%llx, size=0x%llx, flags=0x%x", s.sh_type, s.sh_addr, s.sh_size, s.sh_flags);

		const u32 addr = vm::cast(s.sh_addr);
		const u32 size = vm::cast(s.sh_size);

		if (s.sh_type == 1 && addr && size) // TODO: some sections with addr=0 are valid
		{
			for (auto i = 0; i < prx->segs.size(); i++)
			{
				const u32 saddr = static_cast<u32>(elf.progs[i].p_vaddr);
				if (addr >= saddr && addr < saddr + elf.progs[i].p_memsz)
				{
					// "Relocate" section
					ppu_segment _sec;
					_sec.addr = addr - saddr + prx->segs[i].addr;
					_sec.size = size;
					_sec.type = s.sh_type;
					_sec.flags = s.sh_flags & 7;
					_sec.filesz = 0;
					prx->secs.emplace_back(_sec);
					break;
				}
			}
		}
	}

	// Do relocations
	for (auto& prog : elf.progs)
	{
		switch (const u32 p_type = prog.p_type)
		{
		case 0x700000a4:
		{
			// Relocation information of the SCE_PPURELA segment
			struct ppu_prx_relocation_info
			{
				be_t<u64> offset;
				be_t<u16> unk0;
				u8 index_value;
				u8 index_addr;
				be_t<u32> type;
				vm::bptr<void, u64> ptr;
			};

			for (uint i = 0; i < prog.p_filesz; i += sizeof(ppu_prx_relocation_info))
			{
				const auto& rel = reinterpret_cast<const ppu_prx_relocation_info&>(prog.bin[i]);

				ppu_reloc _rel;
				const u32 raddr = _rel.addr = vm::cast(prx->segs.at(rel.index_addr).addr + rel.offset, HERE);
				const u32 rtype = _rel.type = rel.type;
				const u64 rdata = _rel.data = rel.index_value == 0xFF ? rel.ptr.addr().value() : prx->segs.at(rel.index_value).addr + rel.ptr.addr();
				prx->relocs.emplace_back(_rel);

				switch (rtype)
				{
				case 1: // R_PPC64_ADDR32
				{
					const u32 value = vm::_ref<u32>(raddr) = static_cast<u32>(rdata);
					LOG_TRACE(LOADER, "**** RELOCATION(1): 0x%x <- 0x%08x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 4: //R_PPC64_ADDR16_LO
				{
					const u16 value = vm::_ref<u16>(raddr) = static_cast<u16>(rdata);
					LOG_TRACE(LOADER, "**** RELOCATION(4): 0x%x <- 0x%04x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 5: //R_PPC64_ADDR16_HI
				{
					const u16 value = vm::_ref<u16>(raddr) = static_cast<u16>(rdata >> 16);
					LOG_TRACE(LOADER, "**** RELOCATION(5): 0x%x <- 0x%04x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 6: //R_PPC64_ADDR16_HA
				{
					const u16 value = vm::_ref<u16>(raddr) = static_cast<u16>(rdata >> 16) + (rdata & 0x8000 ? 1 : 0);
					LOG_TRACE(LOADER, "**** RELOCATION(6): 0x%x <- 0x%04x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 10: //R_PPC64_REL24
				{
					const u32 value = vm::_ref<ppu_bf_t<be_t<u32>, 6, 24>>(raddr) = static_cast<u32>(rdata - raddr) >> 2;
					LOG_WARNING(LOADER, "**** RELOCATION(10): 0x%x <- 0x%06x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 11: //R_PPC64_REL14
				{
					const u32 value = vm::_ref<ppu_bf_t<be_t<u32>, 16, 14>>(raddr) = static_cast<u32>(rdata - raddr) >> 2;
					LOG_WARNING(LOADER, "**** RELOCATION(11): 0x%x <- 0x%06x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 38: //R_PPC64_ADDR64
				{
					const u64 value = vm::_ref<u64>(raddr) = rdata;
					LOG_TRACE(LOADER, "**** RELOCATION(38): 0x%x <- 0x%016llx (0x%llx)", raddr, value, rdata);
					break;
				}

				case 44: //R_PPC64_REL64
				{
					const u64 value = vm::_ref<u64>(raddr) = rdata - raddr;
					LOG_TRACE(LOADER, "**** RELOCATION(44): 0x%x <- 0x%016llx (0x%llx)", raddr, value, rdata);
					break;
				}

				case 57: //R_PPC64_ADDR16_LO_DS
				{
					const u16 value = vm::_ref<ppu_bf_t<be_t<u16>, 0, 14>>(raddr) = static_cast<u16>(rdata) >> 2;
					LOG_TRACE(LOADER, "**** RELOCATION(57): 0x%x <- 0x%04x (0x%llx)", raddr, value, rdata);
					break;
				}

				default: LOG_ERROR(LOADER, "**** RELOCATION(%u): Illegal/Unknown type! (addr=0x%x; 0x%llx)", rtype, raddr, rdata);
				}

				if (rdata == 0)
				{
					LOG_TODO(LOADER, "**** RELOCATION(%u): 0x%x <- (zero-based value)", rtype, raddr);
				}
			}

			break;
		}
		}
	}

	if (!elf.progs.empty() && elf.progs[0].p_paddr)
	{
		struct ppu_prx_library_info
		{
			be_t<u16> attributes;
			u8 version[2];
			char name[28];
			be_t<u32> toc;
			be_t<u32> exports_start;
			be_t<u32> exports_end;
			be_t<u32> imports_start;
			be_t<u32> imports_end;
		};

		// Access library information (TODO)
		const auto& lib_info = vm::cptr<ppu_prx_library_info>(vm::cast(prx->segs[0].addr + elf.progs[0].p_paddr - elf.progs[0].p_offset, HERE));
		const auto& lib_name = std::string(lib_info->name);

		std::memcpy(prx->module_info_name, lib_info->name, sizeof(prx->module_info_name));
		prx->module_info_version[0] = lib_info->version[0];
		prx->module_info_version[1] = lib_info->version[1];
		prx->module_info_attributes = lib_info->attributes;

		LOG_WARNING(LOADER, "Library %s (rtoc=0x%x):", lib_name, lib_info->toc);

		prx->specials = ppu_load_exports(link, lib_info->exports_start, lib_info->exports_end);
		prx->imports = ppu_load_imports(prx->relocs, link, lib_info->imports_start, lib_info->imports_end);
		std::stable_sort(prx->relocs.begin(), prx->relocs.end());
		prx->analyse(lib_info->toc, 0);
	}
	else
	{
		LOG_FATAL(LOADER, "Library %s: PRX library info not found");
	}

	prx->start.set(prx->specials[0xbc9a0086]);
	prx->stop.set(prx->specials[0xab779874]);
	prx->exit.set(prx->specials[0x3ab9a95e]);
	prx->prologue.set(prx->specials[0x0d10fd3f]);
	prx->epilogue.set(prx->specials[0x330f7005]);
	prx->name = path.substr(path.find_last_of('/') + 1);
	prx->path = path;

	sha1_finish(&sha, prx->sha1);

	// Format patch name
	std::string hash("PRX-0000000000000000000000000000000000000000");
	for (u32 i = 0; i < 20; i++)
	{
		constexpr auto pal = "0123456789abcdef";
		hash[4 + i * 2] = pal[prx->sha1[i] >> 4];
		hash[5 + i * 2] = pal[prx->sha1[i] & 15];
	}

	// Apply the patch
	auto applied = g_fxo->get<patch_engine>()->apply(hash, vm::g_base_addr);

	if (!Emu.GetTitleID().empty())
	{
		// Alternative patch
		applied += g_fxo->get<patch_engine>()->apply(Emu.GetTitleID() + '-' + hash, vm::g_base_addr);
	}

	LOG_NOTICE(LOADER, "PRX library hash: %s (<- %u)", hash, applied);

	if (Emu.IsReady() && g_fxo->get<ppu_module>()->segs.empty())
	{
		// Special loading mode
		ppu_thread_params p{};
		p.stack_addr = vm::cast(vm::alloc(0x100000, vm::stack, 4096));
		p.stack_size = 0x100000;

		auto ppu = idm::make_ptr<named_thread<ppu_thread>>("PPU[0x1000000] Thread (test_thread)", p, "test_thread", 0);

		ppu->cmd_push({ppu_cmd::initialize, 0});
	}

	return prx;
}

void ppu_unload_prx(const lv2_prx& prx)
{
	// Clean linkage info
	for (auto& imp : prx.imports)
	{
		auto pinfo = static_cast<ppu_linkage_info::module::info*>(imp.second);
		pinfo->frefss.erase(imp.first);
		pinfo->imports.erase(imp.first);
	}

	//for (auto& exp : prx.exports)
	//{
	//	auto pinfo = static_cast<ppu_linkage_info::module::info*>(exp.second);
	//	if (pinfo->static_func)
	//	{
	//		pinfo->export_addr = ppu_function_manager::addr + 8 * pinfo->static_func->index;
	//	}
	//	else if (pinfo->static_var)
	//	{
	//		pinfo->export_addr = pinfo->static_var->addr;
	//	}
	//	else
	//	{
	//		pinfo->export_addr = 0;
	//	}
	//}

	for (auto& seg : prx.segs)
	{
		vm::dealloc(seg.addr, vm::main);
	}
}

void ppu_load_exec(const ppu_exec_object& elf)
{
	// Set for delayed initialization in ppu_initialize()
	const auto _main = g_fxo->get<ppu_module>();

	// Access linkage information object
	const auto link = g_fxo->init<ppu_linkage_info>();

	// TLS information
	u32 tls_vaddr = 0;
	u32 tls_fsize = 0;
	u32 tls_vsize = 0;

	// Process information
	u32 sdk_version = 0xffffffff;
	s32 primary_prio = 1001;
	u32 primary_stacksize = 0x100000;
	u32 malloc_pagesize = 0x100000;
	u32 ppc_seg = 0;

	// Executable hash
	sha1_context sha;
	sha1_starts(&sha);

	// Allocate memory at fixed positions
	for (const auto& prog : elf.progs)
	{
		LOG_NOTICE(LOADER, "** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

		ppu_segment _seg;
		const u32 addr = _seg.addr = vm::cast(prog.p_vaddr, HERE);
		const u32 size = _seg.size = ::narrow<u32>(prog.p_memsz, "p_memsz" HERE);
		const u32 type = _seg.type = prog.p_type;
		const u32 flag = _seg.flags = prog.p_flags;
		_seg.filesz = ::narrow<u32>(prog.p_filesz, "p_filesz" HERE);

		// Hash big-endian values
		sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_type), sizeof(prog.p_type));
		sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_flags), sizeof(prog.p_flags));

		if (type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			if (prog.bin.size() > size || prog.bin.size() != prog.p_filesz)
				fmt::throw_exception("Invalid binary size (0x%llx, memsz=0x%x)", prog.bin.size(), size);

			if (!vm::falloc(addr, size))
				fmt::throw_exception("vm::falloc() failed (addr=0x%x, memsz=0x%x)", addr, size);

			// Copy segment data, hash it
			std::memcpy(vm::base(addr), prog.bin.data(), prog.bin.size());
			sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_vaddr), sizeof(prog.p_vaddr));
			sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_memsz), sizeof(prog.p_memsz));
			sha1_update(&sha, prog.bin.data(), prog.bin.size());

			// Initialize executable code if necessary
			if (prog.p_flags & 0x1)
			{
				ppu_register_range(addr, size);
			}

			// Store only LOAD segments (TODO)
			_main->segs.emplace_back(_seg);
		}
	}

	// Load section list, used by the analyser
	for (const auto& s : elf.shdrs)
	{
		LOG_NOTICE(LOADER, "** Section: sh_type=0x%x, addr=0x%llx, size=0x%llx, flags=0x%x", s.sh_type, s.sh_addr, s.sh_size, s.sh_flags);

		ppu_segment _sec;
		const u32 addr = _sec.addr = vm::cast(s.sh_addr);
		const u32 size = _sec.size = vm::cast(s.sh_size);
		const u32 type = _sec.type = s.sh_type;
		const u32 flag = _sec.flags = s.sh_flags & 7;
		_sec.filesz = 0;

		if (s.sh_type == 1 && addr && size)
		{
			_main->secs.emplace_back(_sec);
		}
	}

	sha1_finish(&sha, _main->sha1);

	// Format patch name
	std::string hash("PPU-0000000000000000000000000000000000000000");
	for (u32 i = 0; i < 20; i++)
	{
		constexpr auto pal = "0123456789abcdef";
		hash[4 + i * 2] = pal[_main->sha1[i] >> 4];
		hash[5 + i * 2] = pal[_main->sha1[i] & 15];
	}

	// Apply the patch
	auto applied = g_fxo->get<patch_engine>()->apply(hash, vm::g_base_addr);

	if (!Emu.GetTitleID().empty())
	{
		// Alternative patch
		applied += g_fxo->get<patch_engine>()->apply(Emu.GetTitleID() + '-' + hash, vm::g_base_addr);
	}

	LOG_NOTICE(LOADER, "PPU executable hash: %s (<- %u)", hash, applied);

	// Initialize HLE modules
	ppu_initialize_modules(link);

	// Static HLE patching
	if (g_cfg.core.hook_functions)
	{
		auto shle = g_fxo->init<statichle_handler>(0);

		for (u32 i = _main->segs[0].addr; i < (_main->segs[0].addr + _main->segs[0].size); i += 4)
		{
			vm::cptr<u8> _ptr = vm::cast(i);
			shle->check_against_patterns(_ptr, (_main->segs[0].addr + _main->segs[0].size) - i, i);
		}
	}

	// Read control flags (0 if doesn't exist)
	g_ps3_process_info.ctrl_flags1 = 0;

	if (bool not_found = g_ps3_process_info.self_info.valid)
	{
		for (const auto& ctrl : g_ps3_process_info.self_info.ctrl_info)
		{
			if (ctrl.type == 1)
			{
				if (!std::exchange(not_found, false))
				{
					LOG_ERROR(LOADER, "More than one control flags header found! (flags1=0x%x)",
						ctrl.control_flags.ctrl_flag1);
					break;
				}

				g_ps3_process_info.ctrl_flags1 |= ctrl.control_flags.ctrl_flag1;
			}
		}

		LOG_NOTICE(LOADER, "SELF header information found: ctrl_flags1=0x%x, authid=0x%llx", 
			g_ps3_process_info.ctrl_flags1, g_ps3_process_info.self_info.app_info.authid);
	}

	// Load other programs
	for (auto& prog : elf.progs)
	{
		switch (const u32 p_type = prog.p_type)
		{
		case 0x00000001: break; // LOAD (already loaded)

		case 0x00000007: // TLS
		{
			tls_vaddr = vm::cast(prog.p_vaddr, HERE);
			tls_fsize = ::narrow<u32>(prog.p_filesz, "p_filesz" HERE);
			tls_vsize = ::narrow<u32>(prog.p_memsz, "p_memsz" HERE);
			break;
		}

		case 0x60000001: // LOOS+1
		{
			if (prog.p_filesz)
			{
				struct process_param_t
				{
					be_t<u32> size;
					be_t<u32> magic;
					be_t<u32> version;
					be_t<u32> sdk_version;
					be_t<s32> primary_prio;
					be_t<u32> primary_stacksize;
					be_t<u32> malloc_pagesize;
					be_t<u32> ppc_seg;
					//be_t<u32> crash_dump_param_addr;
				};

				const auto& info = vm::_ref<process_param_t>(vm::cast(prog.p_vaddr, HERE));

				if (info.size < sizeof(process_param_t))
				{
					LOG_WARNING(LOADER, "Bad process_param size! [0x%x : 0x%x]", info.size, sizeof(process_param_t));
				}

				if (info.magic != 0x13bcc5f6)
				{
					LOG_ERROR(LOADER, "Bad process_param magic! [0x%x]", info.magic);
				}
				else
				{
					sdk_version = info.sdk_version;

					if (s32 prio = info.primary_prio; prio < 3072 
						&& (prio >= (g_ps3_process_info.debug_or_root() ? 0 : -512)))
					{
						primary_prio = prio;
					}

					primary_stacksize = info.primary_stacksize;
					malloc_pagesize = info.malloc_pagesize;
					ppc_seg = info.ppc_seg;

					LOG_NOTICE(LOADER, "*** sdk version: 0x%x", info.sdk_version);
					LOG_NOTICE(LOADER, "*** primary prio: %d", info.primary_prio);
					LOG_NOTICE(LOADER, "*** primary stacksize: 0x%x", info.primary_stacksize);
					LOG_NOTICE(LOADER, "*** malloc pagesize: 0x%x", info.malloc_pagesize);
					LOG_NOTICE(LOADER, "*** ppc seg: 0x%x", info.ppc_seg);
					//LOG_NOTICE(LOADER, "*** crash dump param addr: 0x%x", info.crash_dump_param_addr);
				}
			}
			break;
		}

		case 0x60000002: // LOOS+2
		{
			if (prog.p_filesz)
			{
				struct ppu_proc_prx_param_t
				{
					be_t<u32> size;
					be_t<u32> magic;
					be_t<u32> version;
					be_t<u32> unk0;
					be_t<u32> libent_start;
					be_t<u32> libent_end;
					be_t<u32> libstub_start;
					be_t<u32> libstub_end;
					be_t<u16> ver;
					be_t<u16> unk1;
					be_t<u32> unk2;
				};

				const auto& proc_prx_param = vm::_ref<const ppu_proc_prx_param_t>(vm::cast(prog.p_vaddr, HERE));

				LOG_NOTICE(LOADER, "* libent_start = *0x%x", proc_prx_param.libent_start);
				LOG_NOTICE(LOADER, "* libstub_start = *0x%x", proc_prx_param.libstub_start);
				LOG_NOTICE(LOADER, "* unk0 = 0x%x", proc_prx_param.unk0);
				LOG_NOTICE(LOADER, "* unk2 = 0x%x", proc_prx_param.unk2);

				if (proc_prx_param.magic != 0x1b434cec)
				{
					fmt::throw_exception("Bad magic! (0x%x)", proc_prx_param.magic);
				}

				ppu_load_exports(link, proc_prx_param.libent_start, proc_prx_param.libent_end);
				ppu_load_imports(_main->relocs, link, proc_prx_param.libstub_start, proc_prx_param.libstub_end);
				std::stable_sort(_main->relocs.begin(), _main->relocs.end());
			}
			break;
		}
		default:
		{
			LOG_ERROR(LOADER, "Unknown phdr type (0x%08x)", p_type);
		}
		}
	}

	// Initialize process
	std::vector<std::shared_ptr<lv2_prx>> loaded_modules;

	// Get LLE module list
	std::set<std::string> load_libs;

	if (g_cfg.core.lib_loading == lib_loading_type::manual)
	{
		// Load required set of modules (lib_loading_type::both processed in sys_prx.cpp)
		load_libs = g_cfg.core.load_libraries.get_set();
	}
	else
	{
		if (g_cfg.core.lib_loading != lib_loading_type::hybrid || g_cfg.core.load_libraries.get_set().count("liblv2.sprx"))
		{
			// Will load libsysmodule.sprx internally
			load_libs.emplace("liblv2.sprx");
		}
		else
		{
			// Load only libsysmodule.sprx
			load_libs.emplace("libsysmodule.sprx");
		}
	}

	// Program entry
	u32 entry = 0;

	if (!load_libs.empty())
	{
		const std::string lle_dir = vfs::get("/dev_flash/sys/external/");

		if (!fs::is_dir(lle_dir) || !fs::is_file(lle_dir + "libsysmodule.sprx"))
		{
			LOG_ERROR(GENERAL, "PS3 firmware is not installed or the installed firmware is invalid."
				"\nYou should install the PS3 Firmware (Menu: File -> Install Firmware)."
				"\nVisit https://rpcs3.net/ for Quickstart Guide and more information.");
		}

		for (const auto& name : load_libs)
		{
			const ppu_prx_object obj = decrypt_self(fs::file(lle_dir + name));

			if (obj == elf_error::ok)
			{
				LOG_WARNING(LOADER, "Loading library: %s", name);

				auto prx = ppu_load_prx(obj, lle_dir + name);

				if (prx->funcs.empty())
				{
					LOG_FATAL(LOADER, "Module %s has no functions!", name);
				}
				else
				{
					// TODO: fix arguments
					prx->validate(prx->funcs[0].addr);
				}

				if (name == "liblv2.sprx")
				{
 					// Run liblv2.sprx entry point (TODO)
					entry = prx->start.addr();
				}
				else
				{
					loaded_modules.emplace_back(std::move(prx));
				}
			}
			else
			{
				fmt::throw_exception("Failed to load /dev_flash/sys/external/%s: %s", name, obj.get_error());
			}
		}
	}

	// Set path (TODO)
	_main->name.clear();
	_main->path = vfs::get(Emu.argv[0]);

	// Analyse executable (TODO)
	_main->analyse(0, static_cast<u32>(elf.header.e_entry));

	// Validate analyser results (not required)
	_main->validate(0);

	// Set SDK version
	g_ps3_process_info.sdk_ver = sdk_version;

	// Set ppc fixed allocations segment permission
	g_ps3_process_info.ppc_seg = ppc_seg;

	if (ppc_seg != 0x0)
	{
		if (ppc_seg != 0x1)
		{
			LOG_TODO(LOADER, "Unknown ppc_seg flag value = 0x%x", ppc_seg);
		}

		// Additional segment for fixed allocations
		if (!vm::map(0x30000000, 0x10000000, 0x200))
		{
			fmt::throw_exception("Failed to map ppc_seg's segment!" HERE);
		}
	}

	// Initialize process arguments
	auto args = vm::ptr<u64>::make(vm::alloc(u32{sizeof(u64)} * (::size32(Emu.argv) + ::size32(Emu.envp) + 2), vm::main));
	auto argv = args;

	for (const auto& arg : Emu.argv)
	{
		const u32 arg_size = ::align(::size32(arg) + 1, 0x10);
		const u32 arg_addr = vm::alloc(arg_size, vm::main);

		std::memcpy(vm::base(arg_addr), arg.data(), arg_size);

		*args++ = arg_addr;
	}

	*args++ = 0;
	auto envp = args;

	for (const auto& arg : Emu.envp)
	{
		const u32 arg_size = ::align(::size32(arg) + 1, 0x10);
		const u32 arg_addr = vm::alloc(arg_size, vm::main);

		std::memcpy(vm::base(arg_addr), arg.data(), arg_size);

		*args++ = arg_addr;
	}

	// Fix primary stack size
	switch (u32 sz = primary_stacksize)
	{
	case 0x10: primary_stacksize = 32 * 1024; break; // SYS_PROCESS_PRIMARY_STACK_SIZE_32K
	case 0x20: primary_stacksize = 64 * 1024; break; // SYS_PROCESS_PRIMARY_STACK_SIZE_64K
	case 0x30: primary_stacksize = 96 * 1024; break; // SYS_PROCESS_PRIMARY_STACK_SIZE_96K
	case 0x40: primary_stacksize = 128 * 1024; break; // SYS_PROCESS_PRIMARY_STACK_SIZE_128K
	case 0x50: primary_stacksize = 256 * 1024; break; // SYS_PROCESS_PRIMARY_STACK_SIZE_256K
	case 0x60: primary_stacksize = 512 * 1024; break; // SYS_PROCESS_PRIMARY_STACK_SIZE_512K
	case 0x70: primary_stacksize = 1024 * 1024; break; // SYS_PROCESS_PRIMARY_STACK_SIZE_1M
	default:
	{
		primary_stacksize = ::align<u32>(std::clamp<u32>(sz, 0x10000, 0x100000), 4096);
		break;
	}
	}

	// Initialize main thread
	ppu_thread_params p{};
	p.stack_addr = vm::cast(vm::alloc(primary_stacksize, vm::stack, 4096));
	p.stack_size = primary_stacksize;

	auto ppu = idm::make_ptr<named_thread<ppu_thread>>("PPU[0x1000000] Thread (main_thread)", p, "main_thread", primary_prio, 1);

	// Write initial data (exitspawn)
	if (Emu.data.size())
	{
		std::memcpy(vm::base(ppu->stack_addr + ppu->stack_size - ::size32(Emu.data)), Emu.data.data(), Emu.data.size());
		ppu->gpr[1] -= Emu.data.size();
	}

	// Initialize memory stats (according to sdk version)
	// TODO: This is probably wrong with vsh.self
	u32 mem_size;
	if (sdk_version > 0x0021FFFF)
	{
		mem_size = 0xD500000;
	}
	else if (sdk_version > 0x00192FFF)
	{
		mem_size = 0xD300000;
	}
	else if (sdk_version > 0x0018FFFF)
	{
		mem_size = 0xD100000;
	}
	else if (sdk_version > 0x0017FFFF)
	{
		mem_size = 0xD000000;
	}
	else if (sdk_version > 0x00154FFF)
	{
		mem_size = 0xCC00000;
	}
	else
	{
		mem_size = 0xC800000;
	}

	if (g_cfg.core.debug_console_mode)
	{
		// TODO: Check for all sdk versions
		mem_size += 0xC000000;
	}

	g_fxo->init<lv2_memory_container>(mem_size);

	ppu->cmd_push({ppu_cmd::initialize, 0});

	if (!entry)
	{
		// Set TLS args, call sys_initialize_tls
		ppu->cmd_list
		({
			{ ppu_cmd::set_args, 4 }, u64{ppu->id}, u64{tls_vaddr}, u64{tls_fsize}, u64{tls_vsize},
			{ ppu_cmd::hle_call, FIND_FUNC(sys_initialize_tls) },
		});

		entry = static_cast<u32>(elf.header.e_entry); // Run entry from elf
	}

	// Run start functions
	for (const auto& prx : loaded_modules)
	{
		if (!prx->start)
		{
			continue;
		}

		// Reset arguments, run module entry point function
		ppu->cmd_list
		({
			{ ppu_cmd::set_args, 2 }, u64{0}, u64{0},
			{ ppu_cmd::lle_call, prx->start.addr() },
		});
	}

	// Set command line arguments, run entry function
	ppu->cmd_list
	({
		{ ppu_cmd::set_args, 8 }, u64{Emu.argv.size()}, u64{argv.addr()}, u64{envp.addr()}, u64{0}, u64{ppu->id}, u64{tls_vaddr}, u64{tls_fsize}, u64{tls_vsize},
		{ ppu_cmd::set_gpr, 11 }, u64{elf.header.e_entry},
		{ ppu_cmd::set_gpr, 12 }, u64{malloc_pagesize},
		{ ppu_cmd::lle_call, entry },
	});

	// Set actual memory protection (experimental)
	for (const auto& prog : elf.progs)
	{
		const u32 addr = static_cast<u32>(prog.p_vaddr);
		const u32 size = static_cast<u32>(prog.p_memsz);

		if (prog.p_type == 0x1 /* LOAD */ && prog.p_memsz && (prog.p_flags & 0x2) == 0 /* W */)
		{
			// Set memory protection to read-only when necessary
			verify(HERE), vm::page_protect(addr, ::align(size, 0x1000), 0, 0, vm::page_writable);
		}
	}
}

std::shared_ptr<lv2_overlay> ppu_load_overlay(const ppu_exec_object& elf, const std::string& path)
{
	const auto ovlm = idm::make_ptr<lv2_obj, lv2_overlay>();

	// Access linkage information object
	const auto link = g_fxo->get<ppu_linkage_info>();

	// Executable hash
	sha1_context sha;
	sha1_starts(&sha);

	// Allocate memory at fixed positions
	for (const auto& prog : elf.progs)
	{
		LOG_NOTICE(LOADER, "** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

		ppu_segment _seg;
		const u32 addr = _seg.addr = vm::cast(prog.p_vaddr, HERE);
		const u32 size = _seg.size = ::narrow<u32>(prog.p_memsz, "p_memsz" HERE);
		const u32 type = _seg.type = prog.p_type;
		const u32 flag = _seg.flags = prog.p_flags;
		_seg.filesz = ::narrow<u32>(prog.p_filesz, "p_filesz" HERE);

		// Hash big-endian values
		sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_type), sizeof(prog.p_type));
		sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_flags), sizeof(prog.p_flags));

		if (type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			if (prog.bin.size() > size || prog.bin.size() != prog.p_filesz)
				fmt::throw_exception("Invalid binary size (0x%llx, memsz=0x%x)", prog.bin.size(), size);

			if (!vm::falloc(addr, size))
				fmt::throw_exception("vm::falloc() failed (addr=0x%x, memsz=0x%x)", addr, size);

			// Copy segment data, hash it
			std::memcpy(vm::base(addr), prog.bin.data(), prog.bin.size());
			sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_vaddr), sizeof(prog.p_vaddr));
			sha1_update(&sha, reinterpret_cast<const uchar*>(&prog.p_memsz), sizeof(prog.p_memsz));
			sha1_update(&sha, prog.bin.data(), prog.bin.size());

			// Initialize executable code if necessary
			if (prog.p_flags & 0x1)
			{
				ppu_register_range(addr, size);
			}

			// Store only LOAD segments (TODO)
			ovlm->segs.emplace_back(_seg);
		}
	}

	// Load section list, used by the analyser
	for (const auto& s : elf.shdrs)
	{
		LOG_NOTICE(LOADER, "** Section: sh_type=0x%x, addr=0x%llx, size=0x%llx, flags=0x%x", s.sh_type, s.sh_addr, s.sh_size, s.sh_flags);

		ppu_segment _sec;
		const u32 addr = _sec.addr = vm::cast(s.sh_addr);
		const u32 size = _sec.size = vm::cast(s.sh_size);
		const u32 type = _sec.type = s.sh_type;
		const u32 flag = _sec.flags = s.sh_flags & 7;
		_sec.filesz = 0;

		if (s.sh_type == 1 && addr && size)
		{
			ovlm->secs.emplace_back(_sec);
		}
	}

	sha1_finish(&sha, ovlm->sha1);

	// Format patch name
	std::string hash("OVL-0000000000000000000000000000000000000000");
	for (u32 i = 0; i < 20; i++)
	{
		constexpr auto pal = "0123456789abcdef";
		hash[4 + i * 2] = pal[ovlm->sha1[i] >> 4];
		hash[5 + i * 2] = pal[ovlm->sha1[i] & 15];
	}

	// Apply the patch
	auto applied = g_fxo->get<patch_engine>()->apply(hash, vm::g_base_addr);

	if (!Emu.GetTitleID().empty())
	{
		// Alternative patch
		applied += g_fxo->get<patch_engine>()->apply(Emu.GetTitleID() + '-' + hash, vm::g_base_addr);
	}

	LOG_NOTICE(LOADER, "OVL executable hash: %s (<- %u)", hash, applied);

	// Load other programs
	for (auto& prog : elf.progs)
	{
		switch (const u32 p_type = prog.p_type)
		{
		case 0x00000001: break; // LOAD (already loaded)

		case 0x60000001: // LOOS+1
		{
			if (prog.p_filesz)
			{
				struct process_param_t
				{
					be_t<u32> size;		//0x60
					be_t<u32> magic;	//string OVLM
					be_t<u32> version;	//0x17000
					be_t<u32> sdk_version;	//seems to be correct
											//string "stage_ovlm"
											//and a lot of zeros.
				};

				const auto& info = vm::_ref<process_param_t>(vm::cast(prog.p_vaddr, HERE));

				if (info.size < sizeof(process_param_t))
				{
					LOG_WARNING(LOADER, "Bad process_param size! [0x%x : 0x%x]", info.size, u32{sizeof(process_param_t)});
				}

				if (info.magic != 0x4f564c4d)	//string "OVLM"
				{
					LOG_ERROR(LOADER, "Bad process_param magic! [0x%x]", info.magic);
				}
				else
				{
					LOG_NOTICE(LOADER, "*** sdk version: 0x%x", info.sdk_version);
				}
			}
			break;
		}

		case 0x60000002: // LOOS+2 seems to be 0x0 in size for overlay elfs, at least in known cases
		{
			if (prog.p_filesz)
			{
				struct ppu_proc_prx_param_t
				{
					be_t<u32> size;
					be_t<u32> magic;
					be_t<u32> version;
					be_t<u32> unk0;
					be_t<u32> libent_start;
					be_t<u32> libent_end;
					be_t<u32> libstub_start;
					be_t<u32> libstub_end;
					be_t<u16> ver;
					be_t<u16> unk1;
					be_t<u32> unk2;
				};

				const auto& proc_prx_param = vm::_ref<const ppu_proc_prx_param_t>(vm::cast(prog.p_vaddr, HERE));

				LOG_NOTICE(LOADER, "* libent_start = *0x%x", proc_prx_param.libent_start);
				LOG_NOTICE(LOADER, "* libstub_start = *0x%x", proc_prx_param.libstub_start);
				LOG_NOTICE(LOADER, "* unk0 = 0x%x", proc_prx_param.unk0);
				LOG_NOTICE(LOADER, "* unk2 = 0x%x", proc_prx_param.unk2);

				if (proc_prx_param.magic != 0x1b434cec)
				{
					fmt::throw_exception("Bad magic! (0x%x)", proc_prx_param.magic);
				}

				ppu_load_exports(link, proc_prx_param.libent_start, proc_prx_param.libent_end);
				ppu_load_imports(ovlm->relocs, link, proc_prx_param.libstub_start, proc_prx_param.libstub_end);
			}
			break;
		}
		default:
		{
			LOG_ERROR(LOADER, "Unknown phdr type (0x%08x)", p_type);
		}
		}
	}

	ovlm->entry = static_cast<u32>(elf.header.e_entry);

	// Analyse executable (TODO)
	ovlm->analyse(0, ovlm->entry);

	// Validate analyser results (not required)
	ovlm->validate(0);

	// Set path (TODO)
	ovlm->name = path.substr(path.find_last_of('/') + 1);
	ovlm->path = path;

	return ovlm;
}
