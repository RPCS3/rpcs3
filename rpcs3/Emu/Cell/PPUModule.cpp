#include "stdafx.h"
#include "Utilities/VirtualMemory.h"
#include "Utilities/bin_patch.h"
#include "Crypto/sha1.h"
#include "Crypto/unself.h"
#include "Loader/ELF.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUAnalyser.h"

#include "Emu/Cell/lv2/sys_prx.h"

#include <map>
#include <set>
#include <algorithm>

namespace vm { using namespace ps3; }

extern void ppu_initialize_syscalls();
extern std::string ppu_get_function_name(const std::string& module, u32 fnid);
extern std::string ppu_get_variable_name(const std::string& module, u32 vnid);
extern void ppu_register_range(u32 addr, u32 size);
extern void ppu_register_function_at(u32 addr, u32 size, ppu_function_t ptr);
extern void ppu_initialize(const ppu_module& info);
extern void ppu_initialize();

extern void sys_initialize_tls(ppu_thread&, u64, u32, u32, u32);

extern u32 g_ps3_sdk_version;

// HLE function name cache
std::vector<std::string> g_ppu_function_names;

template <>
void fmt_class_string<lib_loading_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](lib_loading_type value)
	{
		switch (value)
		{
		case lib_loading_type::automatic: return "Automatically load required libraries";
		case lib_loading_type::manual: return "Manually load selected libraries";
		case lib_loading_type::both: return "Load automatic and manual selection";
		case lib_loading_type::liblv2only: return "Load liblv2.sprx only";
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
			bool hle = false;
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
static void ppu_initialize_modules(const std::shared_ptr<ppu_linkage_info>& link)
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
		&ppu_module_manager::cellNetCtl,
		&ppu_module_manager::cellOskDialog,
		&ppu_module_manager::cellOvis,
		&ppu_module_manager::cellPamf,
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
		&ppu_module_manager::cellSysutilAvc,
		&ppu_module_manager::cellSysutilAvc2,
		&ppu_module_manager::cellSysutilNpEula,
		&ppu_module_manager::cellSysutilMisc,
		&ppu_module_manager::cellUsbd,
		&ppu_module_manager::cellUsbPspcm,
		&ppu_module_manager::cellUserInfo,
		&ppu_module_manager::cellVdec,
		&ppu_module_manager::cellVideoExport,
		&ppu_module_manager::cellVideoUpload,
		&ppu_module_manager::cellVoice,
		&ppu_module_manager::cellVpost,
		&ppu_module_manager::libmedi,
		&ppu_module_manager::libmixer,
		&ppu_module_manager::libsnd3,
		&ppu_module_manager::libsynth2,
		&ppu_module_manager::sceNp,
		&ppu_module_manager::sceNp2,
		&ppu_module_manager::sceNpClans,
		&ppu_module_manager::sceNpCommerce2,
		&ppu_module_manager::sceNpSns,
		&ppu_module_manager::sceNpTrophy,
		&ppu_module_manager::sceNpTus,
		&ppu_module_manager::sceNpUtil,
		&ppu_module_manager::sys_io,
		&ppu_module_manager::libnet,
		&ppu_module_manager::sysPrxForUser,
		&ppu_module_manager::sys_libc,
		&ppu_module_manager::sys_lv2dbg,
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
		vm::ps3::write32(addr + 0, addr);
		vm::ps3::write32(addr + 4, ppu_instructions::BLR());

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

				flink.hle = true;
				flink.export_addr = ppu_function_manager::addr + 8 * function.second.index;
			}
		}

		for (auto& variable : module->variables)
		{
			LOG_TRACE(LOADER, "** &0x%08X: %s (size=0x%x, align=0x%x)", variable.first, variable.second.name, variable.second.size, variable.second.align);

			// Allocate HLE variable
			if (variable.second.size >= 4096 || variable.second.align >= 4096)
			{
				variable.second.var->set(vm::alloc(variable.second.size, vm::main, std::max<u32>(variable.second.align, 4096)));
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

				variable.second.var->set(alloc_addr);
				alloc_addr += variable.second.size;
			}

			LOG_TRACE(LOADER, "Allocated HLE variable %s.%s at 0x%x", module->name, variable.second.name, variable.second.var->addr());

			// Initialize HLE variable
			if (variable.second.init)
			{
				variable.second.init();
			}

			auto& vlink = linkage.variables[variable.first];
			
			vlink.hle = true;
			vlink.export_addr = variable.second.var->addr();
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
static auto ppu_load_exports(const std::shared_ptr<ppu_linkage_info>& link, u32 exports_start, u32 exports_end)
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

			if (flink.export_addr && !flink.hle)
			{
				LOG_ERROR(LOADER, "Already linked function '%s' in module '%s'", ppu_get_function_name(module_name, fnid), module_name);
			}
			//else
			{
				// Static function
				const auto _sf = _sm && _sm->functions.count(fnid) ? &_sm->functions.at(fnid) : nullptr;

				if (_sf && (_sf->flags & MFF_FORCED_HLE) && g_cfg.core.hook_functions)
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
					flink.hle = false;

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

			if (vlink.export_addr && !vlink.hle)
			{
				LOG_ERROR(LOADER, "Already linked variable '%s' in module '%s'", ppu_get_variable_name(module_name, vnid), module_name);
			}
			//else
			{
				// Set exported variable
				vlink.export_addr = vaddr;
				vlink.hle = false;

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

static auto ppu_load_imports(std::vector<ppu_reloc>& relocs, const std::shared_ptr<ppu_linkage_info>& link, u32 imports_start, u32 imports_end)
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
	const auto link = fxm::get_always<ppu_linkage_info>();

	// Initialize HLE modules
	ppu_initialize_modules(link);

	for (const auto& prog : elf.progs)
	{
		LOG_NOTICE(LOADER, "** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

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

				// Initialize executable code if necessary
				if (prog.p_flags & 0x1)
				{
					ppu_register_range(addr, mem_size);
				}

				ppu_segment _seg;
				_seg.addr = addr;
				_seg.size = mem_size;
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
				const u64 rdata = _rel.data = prx->segs.at(rel.index_value).addr + rel.ptr.addr();
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
			be_t<u16> version;
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

		LOG_WARNING(LOADER, "Library %s (rtoc=0x%x):", lib_name, lib_info->toc);

		prx->specials = ppu_load_exports(link, lib_info->exports_start, lib_info->exports_end);
		prx->imports = ppu_load_imports(prx->relocs, link, lib_info->imports_start, lib_info->imports_end);

		prx->analyse(lib_info->toc, 0);
	}
	else
	{
		LOG_FATAL(LOADER, "Library %s: PRX library info not found");
	}

	prx->start.set(prx->specials[0xbc9a0086]);
	prx->stop.set(prx->specials[0xab779874]);
	prx->exit.set(prx->specials[0x3ab9a95e]);
	prx->prologue.set(prx->specials[0x0D10FD3F]);
	prx->epilogue.set(prx->specials[0x330F7005]);
	prx->name = path.substr(path.find_last_of('/') + 1);
	prx->path = path;

	if (Emu.IsReady() && fxm::import<ppu_module>([&] { return prx; }))
	{
		// Special loading mode
		auto ppu = idm::make_ptr<ppu_thread>("test_thread", 0, 0x100000);

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

	for (auto& seg : prx.segs)
	{
		vm::dealloc(seg.addr, vm::main);
	}
}

void ppu_load_exec(const ppu_exec_object& elf)
{
	// Set for delayed initialization in ppu_initialize()
	const auto _main = fxm::make<ppu_module>();

	// Access linkage information object
	const auto link = fxm::get_always<ppu_linkage_info>();

	// TLS information
	u32 tls_vaddr = 0;
	u32 tls_fsize = 0;
	u32 tls_vsize = 0;

	// Process information
	u32 sdk_version = 0x360001;
	s32 primary_prio = 0x50;
	u32 primary_stacksize = 0x100000;
	u32 malloc_pagesize = 0x100000;

	// Executable hash
	sha1_context sha;
	sha1_starts(&sha);
	u8 sha1_hash[20];

	// Allocate memory at fixed positions
	for (const auto& prog : elf.progs)
	{
		LOG_NOTICE(LOADER, "** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

		ppu_segment _seg;
		const u32 addr = _seg.addr = vm::cast(prog.p_vaddr, HERE);
		const u32 size = _seg.size = ::narrow<u32>(prog.p_memsz, "p_memsz" HERE);
		const u32 type = _seg.type = prog.p_type;
		const u32 flag = _seg.flags = prog.p_flags;

		// Hash big-endian values
		sha1_update(&sha, (uchar*)&prog.p_type, sizeof(prog.p_type));
		sha1_update(&sha, (uchar*)&prog.p_flags, sizeof(prog.p_flags));

		if (type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			if (prog.bin.size() > size || prog.bin.size() != prog.p_filesz)
				fmt::throw_exception("Invalid binary size (0x%llx, memsz=0x%x)", prog.bin.size(), size);

			if (!vm::falloc(addr, size))
				fmt::throw_exception("vm::falloc() failed (addr=0x%x, memsz=0x%x)", addr, size);

			// Copy segment data, hash it
			std::memcpy(vm::base(addr), prog.bin.data(), prog.bin.size());
			sha1_update(&sha, (uchar*)&prog.p_vaddr, sizeof(prog.p_vaddr));
			sha1_update(&sha, (uchar*)&prog.p_memsz, sizeof(prog.p_memsz));
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

		if (s.sh_type == 1 && addr && size)
		{
			_main->secs.emplace_back(_sec);
		}
	}

	sha1_finish(&sha, sha1_hash);

	// Format patch name
	std::string hash("PPU-0000000000000000000000000000000000000000");
	for (u32 i = 0; i < sizeof(sha1_hash); i++)
	{
		constexpr auto pal = "0123456789abcdef";
		hash[4 + i * 2] = pal[sha1_hash[i] >> 4];
		hash[5 + i * 2] = pal[sha1_hash[i] & 15];
	}

	// Apply the patch
	auto applied = fxm::check_unlocked<patch_engine>()->apply(hash, vm::g_base_addr);

	if (!Emu.GetTitleID().empty())
	{
		// Alternative patch
		applied += fxm::check_unlocked<patch_engine>()->apply(Emu.GetTitleID() + '-' + hash, vm::g_base_addr);
	}

	LOG_NOTICE(LOADER, "PPU executable hash: %s (<- %u)", hash, applied);

	// Initialize HLE modules
	ppu_initialize_modules(link);

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

				const auto& info = vm::ps3::_ref<process_param_t>(vm::cast(prog.p_vaddr, HERE));

				if (info.size < sizeof(process_param_t))
				{
					LOG_WARNING(LOADER, "Bad process_param size! [0x%x : 0x%x]", info.size, SIZE_32(process_param_t));
				}

				if (info.magic != 0x13bcc5f6)
				{
					LOG_ERROR(LOADER, "Bad process_param magic! [0x%x]", info.magic);
				}
				else
				{
					sdk_version = info.sdk_version;
					primary_prio = info.primary_prio;
					primary_stacksize = info.primary_stacksize;
					malloc_pagesize = info.malloc_pagesize;

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

	if (g_cfg.core.lib_loading == lib_loading_type::manual || g_cfg.core.lib_loading == lib_loading_type::both)
	{
		// Load required set of modules
		load_libs = g_cfg.core.load_libraries.get_set();
	}
	else if (g_cfg.core.lib_loading == lib_loading_type::liblv2only)
	{
		// Load only liblv2.sprx
		load_libs.emplace("liblv2.sprx");
	}

	if (g_cfg.core.lib_loading == lib_loading_type::automatic || g_cfg.core.lib_loading == lib_loading_type::both)
	{
		// Load only libsysmodule.sprx
		load_libs.emplace("libsysmodule.sprx");
	}
	else if (0)
	{
		// Load recommended set of modules: Module name -> SPRX
		std::unordered_multimap<std::string, std::string> sprx_map
		{
			{ "cellAdec", "libadec.sprx" }, // cellSpurs|cell_libac3dec|cellAtrac3dec|cellAtracXdec|cellCelpDec|cellDTSdec|cellM2AACdec|cellM2BCdec|cellM4AacDec|cellMP3dec|cellTRHDdec|cellWMAdec|cellDTSLBRdec|cellDDPdec|cellM4AacDec2ch|cellDTSHDdec|cellMPL1dec|cellMP3Sdec|cellM4AacDec2chmod|cellCelp8Dec|cellWMAPROdec|cellWMALSLdec|cellDTSHDCOREdec|cellAtrac3multidec
			{ "cellAdec", "libsre.sprx" },
			{ "cellAdec", "libac3dec.sprx" },
			{ "cellAdec", "libat3dec.sprx" },
			{ "cellAdec", "libat3multidec.sprx" },
			{ "cellAdec", "libatxdec.sprx" },
			{ "cellAdec", "libcelp8dec.sprx" },
			{ "cellAdec", "libcelpdec.sprx" },
			{ "cellAdec", "libddpdec.sprx" },
			{ "cellAdec", "libm2bcdec.sprx" },
			{ "cellAdec", "libm4aacdec.sprx" },
			{ "cellAdec", "libm4aacdec2ch.sprx" },
			{ "cellAdec", "libmp3dec.sprx" },
			{ "cellAdec", "libmpl1dec.sprx" },
			{ "cellAdec", "libwmadec.sprx" },
			{ "cellAtrac", "libatrac3plus.sprx" },
			{ "cellAtrac", "cellAdec" },
			{ "cellAtracMulti", "libatrac3multi.sprx" },
			{ "cellAtracMulti", "cellAdec" },
			{ "cellCelp8Enc", "libcelp8enc.sprx" },
			{ "cellCelp8Enc", "libsre.sprx" },
			{ "cellCelpEnc", "libcelpenc.sprx" },
			{ "cellCelpEnc", "libsre.sprx" },
			{ "cellDmux", "libdmux.sprx" },
			{ "cellDmux", "libdmuxpamf.sprx" },
			{ "cellDmux", "libsre.sprx" },
			{ "cellFiber", "libfiber.sprx" },
			{ "cellFont", "libfont.sprx" },
			{ "cellFontFT", "libfontFT.sprx" },
			{ "cellFontFT", "libfreetype.sprx" },
			{ "cellGcmSys", "libgcm_sys.sprx" },
			{ "cellGifDec", "libgifdec.sprx" },
			{ "cellGifDec", "libsre.sprx" },
			{ "cellJpgDec", "libjpgdec.sprx" },
			{ "cellJpgDec", "libsre.sprx" },
			{ "cellJpgEnc", "libjpgenc.sprx" },
			{ "cellJpgEnc", "libsre.sprx" },
			{ "cellKey2char", "libkey2char.sprx" },
			{ "cellL10n", "libl10n.sprx" },
			{ "cellM4hdEnc", "libm4hdenc.sprx" },
			{ "cellM4hdEnc", "libsre.sprx" },
			{ "cellPamf", "libpamf.sprx" },
			{ "cellPngDec", "libpngdec.sprx" },
			{ "cellPngDec", "libsre.sprx" },
			{ "cellPngEnc", "libpngenc.sprx" },
			{ "cellPngEnc", "libsre.sprx" },
			{ "cellResc", "libresc.sprx" },
			{ "cellRtc", "librtc.sprx" },
			{ "cellSsl", "libssl.sprx" },
			{ "cellSsl", "librtc.sprx" },
			{ "cellHttp", "libhttp.sprx" },
			{ "cellHttp", "cellSsl" },
			{ "cellHttpUtil", "libhttp.sprx" },
			{ "cellHttpUtil", "cellSsl" },
			{ "cellSail", "libsail.sprx" },
			{ "cellSail", "libsre.sprx" },
			{ "cellSail", "libmp4.sprx" },
			{ "cellSail", "libpamf.sprx" },
			{ "cellSail", "libdmux.sprx" },
			{ "cellSail", "libdmuxpamf.sprx" },
			{ "cellSail", "libapostsrc_mini.sprx" },
			{ "cellSail", "libsail_avi.sprx" },
			{ "cellSail", "libvpost.sprx" },
			{ "cellSail", "cellAdec" },
			{ "cellSpursJq", "libspurs_jq.sprx" },
			{ "cellSpursJq", "libsre.sprx" },
			{ "cellSync", "libsre.sprx" },
			{ "cellSheap", "libsre.sprx" },
			{ "cellOvis", "libsre.sprx" },
			{ "cellSpurs", "libsre.sprx" },
			{ "cellDaisy", "libsre.sprx" },
			{ "cellSpudll", "libsre.sprx" },
			{ "cellSync2", "libsync2.sprx" },
			{ "cellSync2", "libsre.sprx" },
			{ "cellVpost", "libvpost.sprx" },
			{ "cellVpost", "libsre.sprx" },
			{ "sys_fs", "libfs.sprx" },
		};

		// Expand dependencies
		for (bool repeat = true; repeat;)
		{
			repeat = false;

			for (auto it = sprx_map.begin(), end = sprx_map.end(); it != end; ++it)
			{
				auto range = sprx_map.equal_range(it->second);

				if (range.first != range.second)
				{
					decltype(sprx_map) add;

					for (; range.first != range.second; ++range.first)
					{
						add.emplace(it->first, range.first->second);
					}

					sprx_map.erase(it);
					sprx_map.insert(add.begin(), add.end());
					repeat = true;
					break;
				}
			}
		}

		for (const auto& pair : link->modules)
		{
			if (!pair.second.imported)
			{
				continue;
			}

			for (auto range = sprx_map.equal_range(pair.first); range.first != range.second; ++range.first)
			{
				load_libs.emplace(range.first->second);
			}
		}
	}
	
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

				loaded_modules.emplace_back(std::move(prx));
			}
			else
			{
				fmt::throw_exception("Failed to load /dev_flash/sys/external/%s: %s", name, obj.get_error());
			}
		}
	}

	// Set path (TODO)
	_main->name = "";
	_main->path = vfs::get(Emu.GetPath());

	// Analyse executable (TODO)
	_main->analyse(0, static_cast<u32>(elf.header.e_entry));

	// Validate analyser results (not required)
	_main->validate(0);

	// Set SDK version
	g_ps3_sdk_version = sdk_version;

	// Initialize process arguments
	std::initializer_list<std::string> args = { Emu.GetPath()/*, "-emu"s*/ };

	auto argv = vm::ptr<u64>::make(vm::alloc(SIZE_32(u64) * ::size32(args), vm::main));
	auto envp = vm::ptr<u64>::make(vm::alloc(::align(SIZE_32(u64), 0x10), vm::main));
	*envp = 0;

	for (const auto& arg : args)
	{
		const u32 arg_size = ::align(::size32(arg) + 1, 0x10);
		const u32 arg_addr = vm::alloc(arg_size, vm::main);

		std::memcpy(vm::base(arg_addr), arg.data(), arg_size);

		*argv++ = arg_addr;
	}

	argv -= ::size32(args);

	// Initialize main thread
	auto ppu = idm::make_ptr<ppu_thread>("main_thread", primary_prio, primary_stacksize);

	ppu->cmd_push({ppu_cmd::initialize, 0});

	// TODO: adjust for liblv2 loading option
	u32 entry = static_cast<u32>(elf.header.e_entry);

	if (g_cfg.core.lib_loading != lib_loading_type::liblv2only)
	{
		// Set TLS args, call sys_initialize_tls
		ppu->cmd_list
		({
			{ ppu_cmd::set_args, 4 }, u64{ppu->id}, u64{tls_vaddr}, u64{tls_fsize}, u64{tls_vsize},
			{ ppu_cmd::hle_call, FIND_FUNC(sys_initialize_tls) },
		});
	}
	else
	{
		// Run liblv2.sprx entry point (TODO)
		entry = loaded_modules[0]->start.addr();

		loaded_modules.clear();
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
		{ ppu_cmd::set_args, 8 }, u64{args.size()}, u64{argv.addr()}, u64{envp.addr()}, u64{0}, u64{ppu->id}, u64{tls_vaddr}, u64{tls_fsize}, u64{tls_vsize},
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
