#include "stdafx.h"
#include "Utilities/Config.h"
#include "Utilities/AutoPause.h"
#include "Crypto/sha1.h"
#include "Loader/ELF.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUAnalyser.h"

#include "Emu/Cell/lv2/sys_prx.h"

#include <unordered_set>
#include <algorithm>
#include "yaml-cpp/yaml.h"

const ppu_decoder<ppu_itype> s_ppu_itype;
//const ppu_decoder<ppu_iname> s_ppu_iname;

LOG_CHANNEL(cellAdec);
LOG_CHANNEL(cellAtrac);
LOG_CHANNEL(cellAtracMulti);
LOG_CHANNEL(cellAudio);
LOG_CHANNEL(cellAvconfExt);
LOG_CHANNEL(cellBGDL);
LOG_CHANNEL(cellCamera);
LOG_CHANNEL(cellCelp8Enc);
LOG_CHANNEL(cellCelpEnc);
LOG_CHANNEL(cellDaisy);
LOG_CHANNEL(cellDmux);
LOG_CHANNEL(cellFiber);
LOG_CHANNEL(cellFont);
LOG_CHANNEL(cellFontFT);
LOG_CHANNEL(cellFs);
LOG_CHANNEL(cellGame);
LOG_CHANNEL(cellGameExec);
LOG_CHANNEL(cellGcmSys);
LOG_CHANNEL(cellGem);
LOG_CHANNEL(cellGifDec);
LOG_CHANNEL(cellHttp);
LOG_CHANNEL(cellHttpUtil);
LOG_CHANNEL(cellImeJp);
LOG_CHANNEL(cellJpgDec);
LOG_CHANNEL(cellJpgEnc);
LOG_CHANNEL(cellKey2char);
LOG_CHANNEL(cellL10n);
LOG_CHANNEL(cellMic);
LOG_CHANNEL(cellMusic);
LOG_CHANNEL(cellMusicDecode);
LOG_CHANNEL(cellMusicExport);
LOG_CHANNEL(cellNetCtl);
LOG_CHANNEL(cellOskDialog);
LOG_CHANNEL(cellOvis);
LOG_CHANNEL(cellPamf);
LOG_CHANNEL(cellPhotoDecode);
LOG_CHANNEL(cellPhotoExport);
LOG_CHANNEL(cellPhotoImportUtil);
LOG_CHANNEL(cellPngDec);
LOG_CHANNEL(cellPngEnc);
LOG_CHANNEL(cellPrint);
LOG_CHANNEL(cellRec);
LOG_CHANNEL(cellRemotePlay);
LOG_CHANNEL(cellResc);
LOG_CHANNEL(cellRtc);
LOG_CHANNEL(cellRudp);
LOG_CHANNEL(cellSail);
LOG_CHANNEL(cellSailRec);
LOG_CHANNEL(cellSaveData);
LOG_CHANNEL(cellScreenshot);
LOG_CHANNEL(cellSearch);
LOG_CHANNEL(cellSheap);
LOG_CHANNEL(cellSpudll);
LOG_CHANNEL(cellSpurs);
LOG_CHANNEL(cellSpursJq);
LOG_CHANNEL(cellSsl);
LOG_CHANNEL(cellSubdisplay);
LOG_CHANNEL(cellSync);
LOG_CHANNEL(cellSync2);
LOG_CHANNEL(cellSysconf);
LOG_CHANNEL(cellSysmodule);
LOG_CHANNEL(cellSysutil);
LOG_CHANNEL(cellSysutilAp);
LOG_CHANNEL(cellSysutilAvc);
LOG_CHANNEL(cellSysutilAvc2);
LOG_CHANNEL(cellSysutilMisc);
LOG_CHANNEL(cellUsbd);
LOG_CHANNEL(cellUsbPspcm);
LOG_CHANNEL(cellUserInfo);
LOG_CHANNEL(cellVdec);
LOG_CHANNEL(cellVideoExport);
LOG_CHANNEL(cellVideoUpload);
LOG_CHANNEL(cellVoice);
LOG_CHANNEL(cellVpost);
LOG_CHANNEL(libmixer);
LOG_CHANNEL(libsnd3);
LOG_CHANNEL(libsynth2);
LOG_CHANNEL(sceNp);
LOG_CHANNEL(sceNp2);
LOG_CHANNEL(sceNpClans);
LOG_CHANNEL(sceNpCommerce2);
LOG_CHANNEL(sceNpSns);
LOG_CHANNEL(sceNpTrophy);
LOG_CHANNEL(sceNpTus);
LOG_CHANNEL(sceNpUtil);
LOG_CHANNEL(sys_io);
LOG_CHANNEL(sys_libc);
LOG_CHANNEL(sys_lv2dbg);
LOG_CHANNEL(libnet);
LOG_CHANNEL(sysPrxForUser);

cfg::bool_entry g_cfg_hook_ppu_funcs(cfg::root.core, "Hook static functions");
cfg::bool_entry g_cfg_load_liblv2(cfg::root.core, "Load liblv2.sprx only");

cfg::set_entry g_cfg_load_libs(cfg::root.core, "Load libraries");

extern std::string ppu_get_function_name(const std::string& module, u32 fnid);
extern std::string ppu_get_variable_name(const std::string& module, u32 vnid);

extern void sys_initialize_tls(PPUThread&, u64, u32, u32, u32);

extern void ppu_initialize(const std::string& name, const std::vector<std::pair<u32, u32>>& set, u32 entry);

// Function lookup table. Not supposed to grow after emulation start.
std::vector<ppu_function_t> g_ppu_function_cache;

// Function name cache in format %s.%s (module name, function name)
std::vector<std::string> g_ppu_function_names;

// Function NID cache for autopause. Autopause tool should probably be rewritten.
std::vector<u32> g_ppu_fnid_cache;

extern std::string ppu_get_module_function_name(u32 index)
{
	if (index < g_ppu_function_names.size())
	{
		return g_ppu_function_names[index];
	}

	return fmt::format(".%u", index);
}

extern void ppu_execute_function(PPUThread& ppu, u32 index)
{
	if (index < g_ppu_function_cache.size())
	{
		// If autopause occures, check_status() will hold the thread until unpaused.
		if (debug::autopause::pause_function(g_ppu_fnid_cache[index]) && ppu.check_status()) throw cpu_state::ret;

		if (const auto func = g_ppu_function_cache[index])
		{
			func(ppu);
			LOG_TRACE(HLE, "'%s' finished, r3=0x%llx", ppu_get_module_function_name(index), ppu.GPR[3]);
			return;
		}
	}

	throw fmt::exception("Function not registered (index %u)" HERE, index);
}

extern ppu_function_t ppu_get_function(u32 index)
{
	if (index < g_ppu_function_cache.size())
	{
		return g_ppu_function_cache[index];
	}

	return nullptr;
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
	return access().at(module)->functions[fnid];
}

ppu_static_variable& ppu_module_manager::access_static_variable(const char* module, u32 vnid)
{
	return access().at(module)->variables[vnid];
}

const ppu_static_module* ppu_module_manager::get_module(const std::string& name)
{
	const auto& map = access();
	const auto found = map.find(name);
	return found != map.end() ? found->second : nullptr;
}

// Initialize static modules.
static void ppu_initialize_modules()
{
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
		&ppu_module_manager::cellDaisy,
		&ppu_module_manager::cellDmux,
		&ppu_module_manager::cellFiber,
		&ppu_module_manager::cellFont,
		&ppu_module_manager::cellFontFT,
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
		&ppu_module_manager::cellSubdisplay,
		&ppu_module_manager::cellSync,
		&ppu_module_manager::cellSync2,
		&ppu_module_manager::cellSysconf,
		&ppu_module_manager::cellSysmodule,
		&ppu_module_manager::cellSysutil,
		&ppu_module_manager::cellSysutilAp,
		&ppu_module_manager::cellSysutilAvc,
		&ppu_module_manager::cellSysutilAvc2,
		&ppu_module_manager::cellSysutilMisc,
		&ppu_module_manager::cellUsbd,
		&ppu_module_manager::cellUsbPspcm,
		&ppu_module_manager::cellUserInfo,
		&ppu_module_manager::cellVdec,
		&ppu_module_manager::cellVideoExport,
		&ppu_module_manager::cellVideoUpload,
		&ppu_module_manager::cellVoice,
		&ppu_module_manager::cellVpost,
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

	// Reinitialize function cache
	g_ppu_function_cache = ppu_function_manager::get();
	g_ppu_function_names.clear();
	g_ppu_function_names.resize(g_ppu_function_cache.size());
	g_ppu_fnid_cache.clear();
	g_ppu_fnid_cache.resize(g_ppu_function_cache.size());
	
	// "Use" all the modules for correct linkage
	for (auto& module : registered)
	{
		LOG_TRACE(LOADER, "Registered static module: %s", module->name);

		for (auto& function : module->functions)
		{
			LOG_TRACE(LOADER, "** 0x%08X: %s", function.first, function.second.name);
			g_ppu_function_names.at(function.second.index) = fmt::format("%s.%s", module->name, function.second.name);
			g_ppu_fnid_cache.at(function.second.index) = function.first;
		}

		for (auto& variable : module->variables)
		{
			LOG_TRACE(LOADER, "** &0x%08X: %s (size=0x%x, align=0x%x)", variable.first, variable.second.name, variable.second.size, variable.second.align);
			variable.second.var->set(0);
		}
	}
}

// Detect import stub at specified address and inject HACK instruction with index immediate.
static bool ppu_patch_import_stub(u32 addr, u32 index)
{
	const auto data = vm::cptr<u32>::make(addr);

	using namespace ppu_instructions;

	// Check various patterns:

	if (vm::check_addr(addr, 32) &&
		(data[0] & 0xffff0000) == LI(r12, 0) &&
		(data[1] & 0xffff0000) == ORIS(r12, r12, 0) &&
		(data[2] & 0xffff0000) == LWZ(r12, r12, 0) &&
		data[3] == STD(r2, r1, 0x28) &&
		data[4] == LWZ(r0, r12, 0) &&
		data[5] == LWZ(r2, r12, 4) &&
		data[6] == MTCTR(r0) &&
		data[7] == BCTR())
	{
		std::memset(vm::base(addr), 0, 32);
		vm::write32(addr + 0, STD(r2, r1, 0x28)); // Save RTOC
		vm::write32(addr + 4, HACK(index));
		vm::write32(addr + 8, BLR());
		return true;
	}

	if (vm::check_addr(addr, 12) &&
		(data[0] & 0xffff0000) == LI(r0, 0) &&
		(data[1] & 0xffff0000) == ORIS(r0, r0, 0) &&
		(data[2] & 0xfc000003) == B(0, 0, 0))
	{
		const auto sub = vm::cptr<u32>::make(addr + 8 + ((s32)data[2] << 6 >> 8 << 2));

		if (vm::check_addr(sub.addr(), 60) &&
			sub[0x0] == STDU(r1, r1, -0x80) &&
			sub[0x1] == STD(r2, r1, 0x70) &&
			sub[0x2] == MR(r2, r0) &&
			sub[0x3] == MFLR(r0) &&
			sub[0x4] == STD(r0, r1, 0x90) &&
			sub[0x5] == LWZ(r2, r2, 0) &&
			sub[0x6] == LWZ(r0, r2, 0) &&
			sub[0x7] == LWZ(r2, r2, 4) &&
			sub[0x8] == MTCTR(r0) &&
			sub[0x9] == BCTRL() &&
			sub[0xa] == LD(r2, r1, 0x70) &&
			sub[0xb] == ADDI(r1, r1, 0x80) &&
			sub[0xc] == LD(r0, r1, 0x10) &&
			sub[0xd] == MTLR(r0) &&
			sub[0xe] == BLR())
		{
			vm::write32(addr + 0, HACK(index));
			vm::write32(addr + 4, BLR());
			vm::write32(addr + 8, 0);
			return true;
		}
	}

	if (vm::check_addr(addr, 64) &&
		data[0x0] == MFLR(r0) &&
		data[0x1] == STD(r0, r1, 0x10) &&
		data[0x2] == STDU(r1, r1, -0x80) &&
		data[0x3] == STD(r2, r1, 0x70) &&
		(data[0x4] & 0xffff0000) == LI(r2, 0) &&
		(data[0x5] & 0xffff0000) == ORIS(r2, r2, 0) &&
		data[0x6] == LWZ(r2, r2, 0) &&
		data[0x7] == LWZ(r0, r2, 0) &&
		data[0x8] == LWZ(r2, r2, 4) &&
		data[0x9] == MTCTR(r0) &&
		data[0xa] == BCTRL() &&
		data[0xb] == LD(r2, r1, 0x70) &&
		data[0xc] == ADDI(r1, r1, 0x80) &&
		data[0xd] == LD(r0, r1, 0x10) &&
		data[0xe] == MTLR(r0) &&
		data[0xf] == BLR())
	{
		std::memset(vm::base(addr), 0, 64);
		vm::write32(addr + 0, HACK(index));
		vm::write32(addr + 4, BLR());
		return true;
	}

	if (vm::check_addr(addr, 64) &&
		data[0x0] == MFLR(r0) &&
		data[0x1] == STD(r0, r1, 0x10) &&
		data[0x2] == STDU(r1, r1, -0x80) &&
		data[0x3] == STD(r2, r1, 0x70) &&
		(data[0x4] & 0xffff0000) == LIS(r12, 0) &&
		(data[0x5] & 0xffff0000) == LWZ(r12, r12, 0) &&
		data[0x6] == LWZ(r0, r12, 0) &&
		data[0x7] == LWZ(r2, r12, 4) &&
		data[0x8] == MTCTR(r0) &&
		data[0x9] == BCTRL() &&
		data[0xa] == LD(r2, r1, 0x70) &&
		data[0xb] == ADDI(r1, r1, 0x80) &&
		data[0xc] == LD(r0, r1, 0x10) &&
		data[0xd] == MTLR(r0) &&
		data[0xe] == BLR())
	{
		std::memset(vm::base(addr), 0, 64);
		vm::write32(addr + 0, HACK(index));
		vm::write32(addr + 4, BLR());
		return true;
	}

	if (vm::check_addr(addr, 56) &&
		(data[0x0] & 0xffff0000) == LI(r12, 0) &&
		(data[0x1] & 0xffff0000) == ORIS(r12, r12, 0) &&
		(data[0x2] & 0xffff0000) == LWZ(r12, r12, 0) &&
		data[0x3] == STD(r2, r1, 0x28) &&
		data[0x4] == MFLR(r0) &&
		data[0x5] == STD(r0, r1, 0x20) &&
		data[0x6] == LWZ(r0, r12, 0) &&
		data[0x7] == LWZ(r2, r12, 4) &&
		data[0x8] == MTCTR(r0) &&
		data[0x9] == BCTRL() &&
		data[0xa] == LD(r0, r1, 0x20) &&
		data[0xb] == MTLR(r0) &&
		data[0xc] == LD(r2, r1, 0x28) &&
		data[0xd] == BLR())
	{
		std::memset(vm::base(addr), 0, 56);
		vm::write32(addr + 0, HACK(index));
		vm::write32(addr + 4, BLR());
		return true;
	}

	return false;
}

// Global linkage information
struct ppu_linkage_info
{
	struct module
	{
		using info_t = std::unordered_map<u32, std::pair<u32, std::unordered_set<u32>>>;

		info_t functions;
		info_t variables;
	};
	
	// Module -> (NID -> (export; [imports...]))
	std::unordered_map<std::string, module> modules;
};

// Link variable
static void ppu_patch_variable_stub(u32 vref, u32 vaddr)
{
	struct vref_t
	{
		be_t<u32> type;
		be_t<u32> addr;
		be_t<u32> unk0;
	};

	for (auto ref = vm::ptr<vref_t>::make(vref); ref->type; ref++)
	{
		if (ref->unk0) LOG_ERROR(LOADER, "**** VREF(%u): Unknown values (0x%x, 0x%x)", ref->type, ref->addr, ref->unk0);

		// OPs are probably similar to relocations
		switch (u32 type = ref->type)
		{
		case 0x1:
		{
			const u32 value = vm::_ref<u32>(ref->addr) = vaddr;
			LOG_WARNING(LOADER, "**** VREF(1): 0x%x <- 0x%x", ref->addr, value);
			break;
		}

		case 0x4:
		case 0x6:
		default: LOG_ERROR(LOADER, "**** VREF(%u): Unknown/Illegal type (0x%x, 0x%x)", ref->type, ref->addr, ref->unk0);
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

		const auto fnids = +lib.nids;
		const auto faddrs = +lib.addrs;

		// Get functions
		for (u32 i = 0, end = lib.num_func; i < end; i++)
		{
			const u32 fnid = fnids[i];
			const u32 faddr = faddrs[i];
			LOG_NOTICE(LOADER, "**** %s export: [%s] at 0x%x", module_name, ppu_get_function_name(module_name, fnid), faddr);

			// Function linkage info
			auto& flink = link->modules[module_name].functions[fnid];

			if (flink.first)
			{
				LOG_FATAL(LOADER, "Already linked function '%s' in module '%s'", ppu_get_function_name(module_name, fnid), module_name);
			}
			else
			{
				// Static function
				const auto _sf = _sm && _sm->functions.count(fnid) ? &_sm->functions.at(fnid) : nullptr;

				if (_sf && (_sf->flags & MFF_FORCED_HLE))
				{
					// Inject HACK instruction (TODO: guess function size and analyse B instruction, or reimplement BLR flag for HACK instruction)
					const auto code = vm::ptr<u32>::make(vm::read32(faddr));
					code[0] = ppu_instructions::HACK(_sf->index);
					code[1] = ppu_instructions::BLR();
				}
				else
				{
					// Set exported function
					flink.first = faddr;

					// Fix imports
					for (const auto addr : flink.second)
					{
						vm::write32(addr, faddr);
						//LOG_WARNING(LOADER, "Exported function '%s' in module '%s'", ppu_get_function_name(module_name, fnid), module_name);
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
			auto& vlink = link->modules[module_name].variables[vnid];

			if (vlink.first)
			{
				LOG_FATAL(LOADER, "Already linked variable '%s' in module '%s'", ppu_get_variable_name(module_name, vnid), module_name);
			}
			else
			{
				// Set exported variable
				vlink.first = vaddr;

				// Fix imports
				for (const auto vref : vlink.second)
				{
					ppu_patch_variable_stub(vref, vaddr);
					//LOG_WARNING(LOADER, "Exported variable '%s' in module '%s'", ppu_get_variable_name(module_name, vnid), module_name);
				}
			}
		}

		addr += lib.size ? lib.size : sizeof(ppu_prx_module_info);
	}

	return result;
}

static u32 ppu_load_imports(const std::shared_ptr<ppu_linkage_info>& link, u32 imports_start, u32 imports_end)
{
	u32 result = imports_start;

	for (u32 addr = imports_start; addr < imports_end;)
	{
		const auto& lib = vm::_ref<const ppu_prx_module_info>(addr);

		result = std::min<u32>(result, lib.name.addr());

		const std::string module_name(lib.name.get_ptr());

		LOG_NOTICE(LOADER, "** Imported module '%s' (0x%x, 0x%x)", module_name, lib.unk4, lib.unk5);

		if (lib.num_tlsvar)
		{
			LOG_FATAL(LOADER, "Unexpected num_tlsvar (%u)!", lib.num_tlsvar);
		}

		// Static module
		const auto _sm = ppu_module_manager::get_module(module_name);

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
			flink.second.emplace(faddr);

			// Link if available
			if (flink.first) vm::write32(faddr, flink.first);

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
			vlink.second.emplace(vref);

			// Link if available
			if (vlink.first) ppu_patch_variable_stub(vref, vlink.first);

			//LOG_WARNING(LOADER, "Imported variable '%s' in module '%s' (0x%x)", ppu_get_variable_name(module_name, vnid), module_name, vlink.first);
		}

		addr += lib.size ? lib.size : sizeof(ppu_prx_module_info);
	}

	return result;
}

// Returns max branch address of jumptable
never_inline static u32 ppu_is_jumptable(vm::ptr<u32>& start_ptr, u32 start, u32 end)
{
	u32 max_addr = 0;

	if (end - start_ptr.addr() < 8)
	{
		return 0;
	}

	for (vm::ptr<u32> ptr = start_ptr; ptr.addr() < end; ptr++)
	{
		const u32 addr = start_ptr.addr() + *ptr;

		if (addr % 4 || addr < start || addr >= end)
		{
			if (ptr - start_ptr < 2)
			{
				return 0;
			}

			start_ptr = ptr;
			return max_addr;
		}

		max_addr = std::max<u32>(max_addr, addr);
	}

	start_ptr = vm::cast(end);
	return max_addr;
}

// Guess whether the function cannot be divided at specific position `split`
static bool ppu_is_coherent(u32 start, u32 end, u32 split)
{
	// Check if the block before `split` is directly connected (can fall through)
	for (vm::ptr<u32> rptr = vm::cast(split - 4);; rptr--)
	{
		const u32 _last = *rptr;

		// Skip NOPs
		if (_last == ppu_instructions::NOP())
		{
			if (rptr.addr() == start) return true;
			continue;
		}

		switch (const auto type = s_ppu_itype.decode(_last))
		{
		case ppu_itype::UNK:
		case ppu_itype::TD:
		case ppu_itype::TDI:
		case ppu_itype::TW:
		case ppu_itype::TWI:
		{
			break;
		}

		case ppu_itype::B:
		{
			if (ppu_opcode_t{_last}.lk) return true;
			break;
		}

		case ppu_itype::BC:
		case ppu_itype::BCLR:
		{
			if (ppu_opcode_t{_last}.lk || (ppu_opcode_t{_last}.bo & 0x14) != 0x14) return true;
			break;
		}

		case ppu_itype::BCCTR:
		{
			if (ppu_opcode_t{_last}.lk || (ppu_opcode_t{_last}.bo & 0x10) == 0) return true;
			break;
		}

		default:
		{
			return true;
		}
		}

		break;
	}

	// Find branches from one part to another
	for (vm::ptr<u32> ptr = vm::cast(start); ptr.addr() < split; ptr++)
	{
		const u32 value = *ptr;
		const auto type = s_ppu_itype.decode(value);

		const ppu_opcode_t op{value};

		if (type == ppu_itype::B || type == ppu_itype::BC)
		{
			const u32 target = ppu_branch_target(op.aa ? 0 : ptr.addr(), type == ppu_itype::B ? +op.ll : +op.simm16);

			if (target % 4 == 0 && target >= split && target < end)
			{
				return !op.lk;
			}
		}

		if (type == ppu_itype::BCCTR && !op.lk)
		{
			const u32 max = ppu_is_jumptable(++ptr, start, end);

			if (max && max >= split)
			{
				return true;
			}

			ptr--;
		}
	}

	// TODO: ???
	return false;
}

static std::vector<std::pair<u32, u32>> ppu_analyse(u32 start, u32 end, const std::vector<std::pair<u32, u32>>& segs, u32 rtoc)
{
	// Function entries (except the last one)
	std::set<u32> result
	{
		end,
	};

	// Instruction usage stats
	//std::unordered_map<const char*, u64> stats;

	// Jumptable entries (addr->size)
	std::unordered_map<u32, u32> jts;

	// Block entries
	std::set<u32> blocks;

	// First pass; Detect branch + link instructions
	for (vm::ptr<u32> ptr = vm::cast(start); ptr.addr() < end; ptr++)
	{
		const u32 value = *ptr;
		const auto type = s_ppu_itype.decode(value);
		//const auto name = s_ppu_iname.decode(value);

		const ppu_opcode_t op{value};

		if (type == ppu_itype::B || type == ppu_itype::BC)
		{
			const u32 target = ppu_branch_target(op.aa ? 0 : ptr.addr(), type == ppu_itype::B ? +op.ll : +op.simm16);

			if (op.lk && target % 4 == 0 && target >= start && target < end && target != ptr.addr())
			{
				LOG_NOTICE(PPU, "BCall: 0x%x -> 0x%x", ptr, target);
				result.emplace(target);
			}

			if (!op.lk && target % 4 == 0 && target >= start && target < end)
			{
				blocks.emplace(target);
			}
		}
		
		if (type == ppu_itype::BCCTR && !op.lk)
		{
			const auto jt = ++ptr;

			if (ppu_is_jumptable(ptr, start, end))
			{
				LOG_NOTICE(PPU, "JTable: 0x%x .. 0x%x", jt, ptr);
				jts.emplace(jt.addr(), ptr.addr() - jt.addr());

				for (auto _ptr = jt; _ptr != ptr; _ptr++)
				{
					blocks.emplace(jt.addr() + *_ptr);
				}
			}
			else
			{
				LOG_NOTICE(PPU, "BCCTR: 0x%x", ptr - 1);
			}

			ptr--;
		}

		//stats[name]++;
	}

	// Find OPD table
	for (const auto& seg : segs)
	{
		for (vm::ptr<u32> ptr = vm::cast(seg.first); ptr.addr() < seg.first + seg.second; ptr++)
		{
			if (ptr[0] >= start && ptr[0] < end && ptr[0] % 4 == 0 && ptr[1] == rtoc)
			{
				while (ptr[0] >= start && ptr[0] < end && ptr[0] % 4 == 0 && !jts.count(ptr[0]) /*&& ptr[1] == rtoc*/)
				{
					LOG_NOTICE(PPU, "OPD: 0x%x -> 0x%x (rtoc=0x%x)", ptr, ptr[0], ptr[1]);
					result.emplace(ptr[0]);
					ptr += 2;
				}

				break;
			}
		}
	}

	// Find more block entries
	for (const auto& seg : segs)
	{
		for (vm::ptr<u32> ptr = vm::cast(seg.first); ptr.addr() < seg.first + seg.second; ptr++)
		{
			const u32 value = *ptr;

			if (value % 4 == 0 && value >= start && value < end)
			{
				blocks.emplace(value);
			}
		}
	}

	// Detect tail calls
	std::deque<u32> task{result.begin(), result.end()};

	while (!task.empty())
	{
		const u32 f_start = task.front();
		const auto f_up = result.upper_bound(f_start);

		if (f_up != result.end()) for (vm::ptr<u32> ptr = vm::cast(f_start); ptr.addr() < *f_up; ptr++)
		{
			const u32 value = *ptr;
			const auto type = s_ppu_itype.decode(value);

			const ppu_opcode_t op{value};

			if (type == ppu_itype::B || type == ppu_itype::BC)
			{
				const u32 target = ppu_branch_target(op.aa ? 0 : ptr.addr(), type == ppu_itype::B ? +op.ll : +op.simm16);

				if (!op.lk && target % 4 == 0 && target >= start && target < end && (target < f_start || target >= *f_up))
				{
					auto _lower = result.lower_bound(target);

					if (*_lower == target || _lower == result.begin())
					{
						continue;
					}

					const u32 f2_end = *_lower;
					const u32 f2_start = *--_lower;

					if (ppu_is_coherent(f2_start, f2_end, target))
					{
						continue;
					}

					LOG_NOTICE(LOADER, "Tail call: 0x%x -> 0x%x", ptr, target);
					result.emplace(target);

					// Rescan two new functions if the insertion took place
					task.push_back(target);
					task.push_back(f2_start);
				}
			}
		}

		task.pop_front();
	}

	// Fill (addr, size) vector
	std::vector<std::pair<u32, u32>> vr;

	for (auto it = result.begin(), end = --result.end(); it != end; it++)
	{
		const u32 addr = *it;
		const auto _up = result.upper_bound(addr);

		// Set initial (addr, size)
		vr.emplace_back(std::make_pair(addr, *_up - addr));

		// Analyse function against its end
		for (u32& size = vr.back().second; size;)
		{
			const auto next = result.upper_bound(addr + size);

			if (next != result.end() && ppu_is_coherent(addr, *next, addr + size))
			{
				// Extend and check again
				const u32 new_size = *next - addr;

				LOG_NOTICE(LOADER, "Extended: 0x%x (0x%x --> 0x%x)", addr, size, new_size);
				size = new_size;
				continue;
			}

			break;
		}
	}

	// Add blocks as (addr, 0) // TODO
	for (auto value : blocks)
	{
		vr.emplace_back(std::make_pair(value, 0));
	}

	// Print some stats
	//{
	//	std::multimap<u64, const char*, std::greater<u64>> sorted;

	//	for (const auto& pair : stats)
	//		sorted.emplace(pair.second, pair.first);

	//	for (const auto& stat : sorted)
	//		LOG_NOTICE(PPU, "Stats: (%llu) %s", stat.first, stat.second);
	//}

	return vr;
}

static void ppu_validate(const std::string& fname, const std::vector<std::pair<u32, u32>>& funcs, u32 reloc)
{
	// Load custom PRX configuration if available
	if (fs::file yml{fname + ".yml"})
	{
		const auto cfg = YAML::Load(yml.to_string());

		u32 index = 0;

		// Validate detected functions using information provided
		for (const auto func : cfg["functions"])
		{
			const u32 addr = func["addr"].as<u32>(-1);
			const u32 size = func["size"].as<u32>(0);

			if (addr != -1 && index < funcs.size())
			{
				u32 found = funcs[index].first - reloc;

				while (addr > found && index + 1 < funcs.size())
				{
					LOG_ERROR(LOADER, "%s.yml : validation failed at 0x%x (0x%x, 0x%x)", fname, found, addr, size);
					index++;
					found = funcs[index].first - reloc;
				}

				if (addr < found)
				{
					LOG_ERROR(LOADER, "%s.yml : function not found (0x%x, 0x%x)", fname, addr, size);
					continue;
				}

				if (size && size < funcs[index].second)
				{
					LOG_WARNING(LOADER, "%s.yml : function size mismatch at 0x%x(size=0x%x) (0x%x, 0x%x)", fname, found, funcs[index].second, addr, size);
				}

				if (size > funcs[index].second)
				{
					LOG_ERROR(LOADER, "%s.yml : function size mismatch at 0x%x(size=0x%x) (0x%x, 0x%x)", fname, found, funcs[index].second, addr, size);
				}

				index++;
			}
			else
			{
				LOG_ERROR(LOADER, "%s.yml : function not found at the end (0x%x, 0x%x)", fname, addr, size);
				break;
			}
		}

		if (!index)
		{
			return; // ???
		}

		while (index < funcs.size())
		{
			if (funcs[index].second)
			{
				LOG_ERROR(LOADER, "%s.yml : function not covered at 0x%x (size=0x%x)", fname, funcs[index].first, funcs[index].second);
			}

			index++;
		}

		LOG_SUCCESS(LOADER, "%s.yml : validation completed", fname);
	}
}

template<>
std::shared_ptr<lv2_prx_t> ppu_prx_loader::load() const
{
	std::vector<std::pair<u32, u32>> segments;

	for (const auto& prog : progs)
	{
		LOG_NOTICE(LOADER, "** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

		switch (const u32 p_type = prog.p_type)
		{
		case 0x1: // LOAD
		{
			if (prog.p_memsz)
			{
				const u32 mem_size = fmt::narrow<u32>("Invalid p_memsz (0x%llx)" HERE, prog.p_memsz);
				const u32 file_size = fmt::narrow<u32>("Invalid p_filesz (0x%llx)" HERE, prog.p_filesz);
				const u32 init_addr = fmt::narrow<u32>("Invalid p_vaddr (0x%llx)" HERE, prog.p_vaddr);

				// Alloc segment memory
				const u32 addr = vm::alloc(mem_size, vm::main);

				if (!addr)
				{
					throw fmt::exception("vm::alloc() failed (size=0x%x)", mem_size);
				}

				// Copy data
				std::memcpy(vm::base(addr), prog.bin.data(), file_size);
				LOG_WARNING(LOADER, "**** Loaded to 0x%x (size=0x%x)", addr, mem_size);

				segments.emplace_back(std::make_pair(addr, mem_size));
			}

			break;
		}

		case 0x700000a4: break; // Relocations

		default: LOG_ERROR(LOADER, "Unknown segment type! 0x%08x", p_type);
		}
	}

	// Do relocations
	for (auto& prog : progs)
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

				const u32 raddr = vm::cast(segments.at(rel.index_addr).first + rel.offset, HERE);
				const u64 rdata = segments.at(rel.index_value).first + rel.ptr.addr();

				switch (const u32 type = rel.type)
				{
				case 1:
				{
					const u32 value = vm::_ref<u32>(raddr) = static_cast<u32>(rdata);
					LOG_TRACE(LOADER, "**** RELOCATION(1): 0x%x <- 0x%08x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 4:
				{
					const u16 value = vm::_ref<u16>(raddr) = static_cast<u16>(rdata);
					LOG_TRACE(LOADER, "**** RELOCATION(4): 0x%x <- 0x%04x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 5:
				{
					const u16 value = vm::_ref<u16>(raddr) = static_cast<u16>(rdata >> 16);
					LOG_TRACE(LOADER, "**** RELOCATION(5): 0x%x <- 0x%04x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 6:
				{
					const u16 value = vm::_ref<u16>(raddr) = static_cast<u16>(rdata >> 16) + (rdata & 0x8000 ? 1 : 0);
					LOG_TRACE(LOADER, "**** RELOCATION(6): 0x%x <- 0x%04x (0x%llx)", raddr, value, rdata);
					break;
				}

				case 10:
				case 44:
				case 57:
				default: LOG_ERROR(LOADER, "**** RELOCATION(%u): Illegal/Unknown type! (addr=0x%x)", type, raddr);
				}
			}

			break;
		}
		}
	}

	// Access linkage information object
	const auto link = fxm::get_always<ppu_linkage_info>();

	// Create new PRX object
	auto prx = idm::make_ptr<lv2_prx_t>();

	if (!progs.empty() && progs[0].p_paddr)
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
		const auto& lib_info = vm::cptr<ppu_prx_library_info>(vm::cast(segments[0].first + progs[0].p_paddr - progs[0].p_offset, HERE));
		const auto& lib_name = std::string(lib_info->name);

		LOG_WARNING(LOADER, "Library %s (rtoc=0x%x):", lib_name, lib_info->toc);

		prx->specials = ppu_load_exports(link, lib_info->exports_start, lib_info->exports_end);

		const std::initializer_list<u32> addr_list
		{
			ppu_load_imports(link, lib_info->imports_start, lib_info->imports_end),

			lib_info.addr(),
			lib_info->imports_start,
			lib_info->exports_start,
		};

		// Get functions
		prx->func = ppu_analyse(segments[0].first, std::min(addr_list), segments, lib_info->toc);
	}
	else
	{
		LOG_FATAL(LOADER, "Library %s: PRX library info not found");
	}

	prx->start.set(prx->specials[0xbc9a0086]);
	prx->stop.set(prx->specials[0xab779874]);
	prx->exit.set(prx->specials[0x3ab9a95e]);

	return prx;
}

template<>
void ppu_exec_loader::load() const
{
	ppu_initialize_modules();

	if (g_cfg_hook_ppu_funcs)
	{
		LOG_TODO(LOADER, "'Hook static functions' option deactivated");
	}

	// Access linkage information object
	const auto link = fxm::get_always<ppu_linkage_info>();

	// Segment info
	std::vector<std::pair<u32, u32>> segments;

	// Functions
	std::vector<std::pair<u32, u32>> exec_set;
	u32 exec_end{};

	// Allocate memory at fixed positions
	for (const auto& prog : progs)
	{
		LOG_NOTICE(LOADER, "** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

		const u32 addr = vm::cast(prog.p_vaddr, HERE);
		const u32 size = fmt::narrow<u32>("Invalid p_memsz: 0x%llx" HERE, prog.p_memsz);

		if (prog.p_type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			if (prog.bin.size() > size || prog.bin.size() != prog.p_filesz)
				throw fmt::exception("Invalid binary size (0x%llx, memsz=0x%x)", prog.bin.size(), size);

			if (!vm::falloc(addr, size, vm::main))
				throw fmt::exception("vm::falloc() failed (addr=0x%x, memsz=0x%x)", addr, size);

			std::memcpy(vm::base(addr), prog.bin.data(), prog.bin.size());

			segments.emplace_back(std::make_pair(addr, size));

			if (prog.p_flags & 1) // Test EXEC flag
				exec_end = addr + size;
		}
	}

	// Load other programs
	for (auto& prog : progs)
	{
		switch (const u32 p_type = prog.p_type)
		{
		case 0x00000001: break; //LOAD

		case 0x00000007: //TLS
		{
			const u32 addr = vm::cast(prog.p_vaddr, HERE);
			const u32 filesz = fmt::narrow<u32>("Invalid p_filesz (0x%llx)" HERE, prog.p_filesz);
			const u32 memsz = fmt::narrow<u32>("Invalid p_memsz (0x%llx)" HERE, prog.p_memsz);
			Emu.SetTLSData(addr, filesz, memsz);
			LOG_NOTICE(LOADER, "*** TLS segment addr: 0x%08x", Emu.GetTLSAddr());
			LOG_NOTICE(LOADER, "*** TLS segment size: 0x%08x", Emu.GetTLSFilesz());
			LOG_NOTICE(LOADER, "*** TLS memory size: 0x%08x", Emu.GetTLSMemsz());
			break;
		}

		case 0x60000001: //LOOS+1
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
					LOG_NOTICE(LOADER, "*** sdk version: 0x%x", info.sdk_version);
					LOG_NOTICE(LOADER, "*** primary prio: %d", info.primary_prio);
					LOG_NOTICE(LOADER, "*** primary stacksize: 0x%x", info.primary_stacksize);
					LOG_NOTICE(LOADER, "*** malloc pagesize: 0x%x", info.malloc_pagesize);
					LOG_NOTICE(LOADER, "*** ppc seg: 0x%x", info.ppc_seg);
					//LOG_NOTICE(LOADER, "*** crash dump param addr: 0x%x", info.crash_dump_param_addr);

					Emu.SetParams(info.sdk_version, info.malloc_pagesize, std::max<u32>(info.primary_stacksize, 0x4000), info.primary_prio);
				}
			}
			break;
		}

		case 0x60000002: //LOOS+2
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

				if (proc_prx_param.magic != 0x1b434cec)
				{
					throw fmt::exception("Bad magic! (0x%x)", proc_prx_param.magic);
				}

				ppu_load_exports(link, proc_prx_param.libent_start, proc_prx_param.libent_end);

				const u32 min_addr = ppu_load_imports(link, proc_prx_param.libstub_start, proc_prx_param.libstub_end);

				exec_end = std::min<u32>(min_addr, exec_end);
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
	std::vector<u32> start_funcs;

	// Load modules
	const std::string& lle_dir = vfs::get("/dev_flash/sys/external");

	if (g_cfg_load_liblv2)
	{
		const ppu_prx_loader loader = fs::file(lle_dir + "/liblv2.sprx");

		if (loader == elf_error::ok)
		{
			start_funcs.push_back(loader.load()->start.addr());
		}
		else
		{
			throw fmt::exception("Failed to load liblv2.sprx: %s", loader.get_error());
		}
	}
	else
	{
		for (const auto& name : g_cfg_load_libs.get_set())
		{
			const ppu_prx_loader loader = fs::file(lle_dir + '/' + name);

			if (loader == elf_error::ok)
			{
				LOG_WARNING(LOADER, "Loading library: %s", name);

				const auto prx = loader.load();

				// Register start function
				if (prx->start)
				{
					start_funcs.push_back(prx->start.addr());
				}

				// Add functions
				exec_set.insert(exec_set.end(), prx->func.begin(), prx->func.end());

				ppu_validate(lle_dir + '/' + name, prx->func, prx->func[0].first);
			}
			else
			{
				LOG_FATAL(LOADER, "Failed to load %s: %s", name, loader.get_error());
			}
		}
	}

	// Check unlinked functions and variables
	for (auto& module : link->modules)
	{
		const auto _sm = ppu_module_manager::get_module(module.first);

		if (!_sm)
		{
			LOG_ERROR(LOADER, "Unknown module '%s'", module.first);
		}
		else
		{
			// Allocate HLE variables (TODO)
			for (auto& var : _sm->variables)
			{
				var.second.var->set(vm::alloc(var.second.size, vm::main, std::max<u32>(var.second.align, 4096)));
				LOG_WARNING(LOADER, "Allocated variable '%s' in module '%s' at *0x%x", var.second.name, module.first, var.second.var->addr());
			}

			// Initialize HLE variables (TODO)
			for (auto& var : _sm->variables)
			{
				var.second.init();
			}
		}

		for (auto& entry : module.second.functions)
		{
			const u32 fnid = entry.first;
			const u32 faddr = entry.second.first;

			if (faddr == 0)
			{
				if (const auto _sf = _sm && _sm->functions.count(fnid) ? &_sm->functions.at(fnid) : nullptr)
				{
					// Static function
					for (auto& import : entry.second.second)
					{
						const u32 stub = vm::read32(import);

						if (!ppu_patch_import_stub(stub, _sf->index))
						{
							LOG_ERROR(LOADER, "Failed to inject code for function '%s' in module '%s' (0x%x)", _sf->name, module.first, stub);
						}
						else
						{
							LOG_NOTICE(LOADER, "Injected hack for function '%s' in module '%s' (*0x%x)", _sf->name, module.first, stub);
						}
					}
				}
				else
				{
					// TODO
					const u32 index = ::size32(g_ppu_function_cache);
					const std::string& fname = ppu_get_function_name(module.first, fnid);
					g_ppu_function_cache.emplace_back();
					g_ppu_function_names.emplace_back(fmt::format("%s.%s", module.first, fname));
					g_ppu_fnid_cache.emplace_back(fnid);

					LOG_ERROR(LOADER, "Unknown function '%s' in module '%s' (index %u)", fname, module.first, index);

					for (auto& import : entry.second.second)
					{
						if (_sm)
						{
							const u32 stub = vm::read32(import);

							if (!ppu_patch_import_stub(stub, index))
							{
								LOG_ERROR(LOADER, "Failed to inject code for function '%s' in module '%s' (0x%x)", fname, module.first, stub);
							}
							else
							{
								LOG_NOTICE(LOADER, "Injected hack for function '%s' in module '%s' (*0x%x)", fname, module.first, stub);
							}
						}

						LOG_WARNING(LOADER, "** Not linked at *0x%x", import);
					}
				}
			}
		}

		for (auto& entry : module.second.variables)
		{
			const u32 vnid = entry.first;
			const u32 vaddr = entry.second.first;

			if (vaddr == 0)
			{
				// Static variable
				if (const auto _sv = _sm && _sm->variables.count(vnid) ? &_sm->variables.at(vnid) : nullptr)
				{
					LOG_NOTICE(LOADER, "Linking HLE variable '%s' in module '%s' (*0x%x):", ppu_get_variable_name(module.first, vnid), module.first, _sv->var->addr());

					for (auto& ref : entry.second.second)
					{
						ppu_patch_variable_stub(ref, _sv->var->addr());
						LOG_NOTICE(LOADER, "** Linked at ref=*0x%x", ref);
					}
				}
				else
				{
					LOG_ERROR(LOADER, "Unknown variable '%s' in module '%s'", ppu_get_variable_name(module.first, vnid), module.first);

					for (auto& ref : entry.second.second)
					{
						LOG_WARNING(LOADER, "** Not linked at ref=*0x%x", ref);
					}
				}
			}
			else
			{
				// Retro-link LLE variable (TODO: HLE must not be allocated/initialized in this case)
				if (const auto _sv = _sm && _sm->variables.count(vnid) ? &_sm->variables.at(vnid) : nullptr)
				{
					_sv->var->set(vaddr);
					LOG_NOTICE(LOADER, "Linked LLE variable '%s' in module '%s' -> 0x%x", ppu_get_variable_name(module.first, vnid), module.first, vaddr);
				}
			}
		}
	}

	// Analyse executable
	const u32 entry_rtoc = vm::read32(vm::cast(header.e_entry, HERE) + 4);
	const auto funcs = ppu_analyse(segments[0].first, exec_end, segments, entry_rtoc);

	ppu_validate(vfs::get(Emu.GetPath()), funcs, 0);

	for (const auto& pair : funcs)
	{
		exec_set.emplace_back(pair);
	}

	// TODO: adjust for liblv2 loading option
	using namespace ppu_instructions;

	auto ppu_thr_stop_data = vm::ptr<u32>::make(vm::alloc(2 * 4, vm::main));
	Emu.SetCPUThreadStop(ppu_thr_stop_data.addr());
	ppu_thr_stop_data[0] = HACK(1);
	ppu_thr_stop_data[1] = BLR();

	static const int branch_size = 10 * 4;

	auto make_branch = [](vm::ptr<u32>& ptr, u32 addr, bool last)
	{
		const u32 stub = vm::read32(addr);
		const u32 rtoc = vm::read32(addr + 4);

		*ptr++ = LI(r0, 0);
		*ptr++ = ORI(r0, r0, stub & 0xffff);
		*ptr++ = ORIS(r0, r0, stub >> 16);
		*ptr++ = LI(r2, 0);
		*ptr++ = ORI(r2, r2, rtoc & 0xffff);
		*ptr++ = ORIS(r2, r2, rtoc >> 16);
		*ptr++ = MTCTR(r0);
		*ptr++ = last ? BCTR() : BCTRL();
	};

	auto entry = vm::ptr<u32>::make(vm::alloc(48 + branch_size * (::size32(start_funcs) + 1), vm::main));

	// Save initialization args
	*entry++ = MR(r14, r3);
	*entry++ = MR(r15, r4);
	*entry++ = MR(r16, r5);
	*entry++ = MR(r17, r6);
	*entry++ = MR(r18, r11);
	*entry++ = MR(r19, r12);

	if (!g_cfg_load_liblv2)
	{
		// Call sys_initialize_tls explicitly
		*entry++ = MR(r3, r7);
		*entry++ = MR(r4, r8);
		*entry++ = MR(r5, r9);
		*entry++ = MR(r6, r10);
		*entry++ = HACK(FIND_FUNC(sys_initialize_tls));
	}

	for (auto& f : start_funcs)
	{
		// Reset arguments (TODO)
		*entry++ = LI(r3, 0);
		*entry++ = LI(r4, 0);
		make_branch(entry, f, false);
	}

	// Restore initialization args
	*entry++ = MR(r3, r14);
	*entry++ = MR(r4, r15);
	*entry++ = MR(r5, r16);
	*entry++ = MR(r6, r17);
	*entry++ = MR(r11, r18);
	*entry++ = MR(r12, r19);

	// Branch to initialization
	make_branch(entry, static_cast<u32>(header.e_entry), true);

	// Register entry function (addr, size)
	exec_set.emplace_back(std::make_pair(entry.addr() & -0x1000, entry.addr() & 0xfff));

	// Initialize recompiler
	ppu_initialize("", exec_set, static_cast<u32>(header.e_entry));

	auto ppu = idm::make_ptr<PPUThread>("main_thread");

	ppu->pc = entry.addr() & -0x1000;
	ppu->stack_size = Emu.GetPrimaryStackSize();
	ppu->prio = Emu.GetPrimaryPrio();
	ppu->cpu_init();

	ppu->GPR[2] = 0xdeadbeef; // rtoc
	ppu->GPR[11] = 0xabadcafe; // OPD ???
	ppu->GPR[12] = Emu.GetMallocPageSize();

	std::initializer_list<std::string> args = { Emu.GetPath()/*, "-emu"s*/ };

	auto argv = vm::ptr<u64>::make(vm::alloc(SIZE_32(u64) * ::size32(args), vm::main));
	auto envp = vm::ptr<u64>::make(vm::alloc(::align(SIZE_32(u64), 0x10), vm::main));
	*envp = 0;

	ppu->GPR[3] = args.size(); // argc
	ppu->GPR[4] = argv.addr();
	ppu->GPR[5] = envp.addr();
	ppu->GPR[6] = 0; // ???

	for (const auto& arg : args)
	{
		const u32 arg_size = ::align(::size32(arg) + 1, 0x10);
		const u32 arg_addr = vm::alloc(arg_size, vm::main);

		std::memcpy(vm::base(arg_addr), arg.data(), arg_size);

		*argv++ = arg_addr;
	}

	// Arguments for sys_initialize_tls()
	ppu->GPR[7] = ppu->id;
	ppu->GPR[8] = Emu.GetTLSAddr();
	ppu->GPR[9] = Emu.GetTLSFilesz();
	ppu->GPR[10] = Emu.GetTLSMemsz();

	//ppu->state += cpu_state::interrupt;

	// Set memory protections
	//for (const auto& prog : progs)
	//{
	//	const u32 addr = static_cast<u32>(prog.p_vaddr);
	//	const u32 size = static_cast<u32>(prog.p_memsz);

	//	if (prog.p_type == 0x1 /* LOAD */ && prog.p_memsz && (prog.p_flags & 0x2) == 0 /* W */)
	//	{
	//		// Set memory protection to read-only where necessary
	//		VERIFY(vm::page_protect(addr, ::align(size, 0x1000), 0, 0, vm::page_writable));
	//	}
	//}
}
