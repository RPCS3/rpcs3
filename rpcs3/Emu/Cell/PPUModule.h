#pragma once

#include "PPUFunction.h"
#include "PPUCallback.h"
#include "ErrorCodes.h"
#include <typeinfo>
#include "Emu/Memory/vm_var.h"

// Helper function
constexpr const char* ppu_select_name(const char* name, u32 /*id*/)
{
	return name;
}

// Helper function
constexpr const char* ppu_select_name(const char* /*name*/, const char* orig_name)
{
	return orig_name;
}

// Generate FNID or VNID for given name
extern u32 ppu_generate_id(const char* name);

// Overload for REG_FNID, REG_VNID macro
constexpr u32 ppu_generate_id(u32 id)
{
	return id;
}

// Flags set with REG_FUNC
enum ppu_static_module_flags : u32
{
	MFF_FORCED_HLE = (1 << 0), // Always call HLE function
	MFF_PERFECT    = (1 << 1), // Indicates complete implementation and LLE interchangeability
	MFF_HIDDEN     = (1 << 2), // Invisible function for internal use (TODO)
};

// HLE function information
struct ppu_static_function
{
	const char* name;
	u32 index; // Index for ppu_function_manager
	u32 flags;
	const char* type;
	std::vector<const char*> args; // Arg names
	const u32* export_addr;

	ppu_static_function& flag(ppu_static_module_flags value)
	{
		flags |= value;
		return *this;
	}
};

// HLE variable information
struct ppu_static_variable
{
	const char* name;
	vm::gvar<char>* var; // Pointer to variable address storage
	void(*init)(); // Variable initialization function
	u32 size;
	u32 align;
	const char* type;
	u32 flags;
	u32 addr;
	const u32* export_addr;

	ppu_static_variable& flag(ppu_static_module_flags value)
	{
		flags |= value;
		return *this;
	}
};

// HLE module information
class ppu_static_module final
{
public:
	const std::string name;

	std::unordered_map<u32, ppu_static_function, value_hash<u32>> functions;
	std::unordered_map<u32, ppu_static_variable, value_hash<u32>> variables;

public:
	ppu_static_module(const char* name);

	ppu_static_module(const char* name, void(*init)())
		: ppu_static_module(name)
	{
		init();
	}

	ppu_static_module(const char* name, void(*init)(ppu_static_module* _this))
		: ppu_static_module(name)
	{
		init(this);
	}
};

class ppu_module_manager final
{
	friend class ppu_static_module;

	static std::unordered_map<std::string, ppu_static_module*>& access();

	static void register_module(ppu_static_module* module);

	static ppu_static_function& access_static_function(const char* module, u32 fnid);

	static ppu_static_variable& access_static_variable(const char* module, u32 vnid);

	// Global variable for each registered function
	template <auto* Func>
	struct registered
	{
		static ppu_static_function* info;
	};

public:
	static const ppu_static_module* get_module(const std::string& name);

	template <auto* Func>
	static auto& register_static_function(const char* module, const char* name, ppu_function_t func, u32 fnid)
	{
		auto& info = access_static_function(module, fnid);

		info.name  = name;
		info.index = ppu_function_manager::register_function<decltype(Func), Func>(func);
		info.flags = 0;
		info.type  = typeid(*Func).name();

		registered<Func>::info = &info;

		return info;
	}

	template <auto* Func>
	static auto& find_static_function()
	{
		return *registered<Func>::info;
	}

	template <auto* Var>
	static auto& register_static_variable(const char* module, const char* name, u32 vnid)
	{
		using gvar = std::decay_t<decltype(*Var)>;

		static_assert(std::is_same<u32, typename gvar::addr_type>::value, "Static variable registration: vm::gvar<T> expected");

		auto& info = access_static_variable(module, vnid);

		info.name  = name;
		info.var   = reinterpret_cast<vm::gvar<char>*>(Var);
		info.init  = [] {};
		info.size  = gvar::alloc_size;
		info.align = gvar::alloc_align;
		info.type  = typeid(*Var).name();
		info.flags = 0;
		info.addr  = 0;

		return info;
	}

	static const auto& get()
	{
		return access();
	}

	static const ppu_static_module cellAdec;
	static const ppu_static_module cellAtrac;
	static const ppu_static_module cellAtracMulti;
	static const ppu_static_module cellAudio;
	static const ppu_static_module cellAvconfExt;
	static const ppu_static_module cellAuthDialogUtility;
	static const ppu_static_module cellBGDL;
	static const ppu_static_module cellCamera;
	static const ppu_static_module cellCelp8Enc;
	static const ppu_static_module cellCelpEnc;
	static const ppu_static_module cellCrossController;
	static const ppu_static_module cellDaisy;
	static const ppu_static_module cellDmux;
	static const ppu_static_module cellDtcpIpUtility;
	static const ppu_static_module cellFiber;
	static const ppu_static_module cellFont;
	static const ppu_static_module cellFontFT;
	static const ppu_static_module cell_FreeType2;
	static const ppu_static_module cellFs;
	static const ppu_static_module cellGame;
	static const ppu_static_module cellGameExec;
	static const ppu_static_module cellGcmSys;
	static const ppu_static_module cellGem;
	static const ppu_static_module cellGifDec;
	static const ppu_static_module cellHttp;
	static const ppu_static_module cellHttps;
	static const ppu_static_module cellHttpUtil;
	static const ppu_static_module cellImeJp;
	static const ppu_static_module cellJpgDec;
	static const ppu_static_module cellJpgEnc;
	static const ppu_static_module cellKey2char;
	static const ppu_static_module cellL10n;
	static const ppu_static_module cellLibprof;
	static const ppu_static_module cellMic;
	static const ppu_static_module cellMusic;
	static const ppu_static_module cellMusicDecode;
	static const ppu_static_module cellMusicExport;
	static const ppu_static_module cellNetAoi;
	static const ppu_static_module cellNetCtl;
	static const ppu_static_module cellOskDialog;
	static const ppu_static_module cellOvis;
	static const ppu_static_module cellPamf;
	static const ppu_static_module cellPesmUtility;
	static const ppu_static_module cellPhotoDecode;
	static const ppu_static_module cellPhotoExport;
	static const ppu_static_module cellPhotoImportUtil;
	static const ppu_static_module cellPngDec;
	static const ppu_static_module cellPngEnc;
	static const ppu_static_module cellPrint;
	static const ppu_static_module cellRec;
	static const ppu_static_module cellRemotePlay;
	static const ppu_static_module cellResc;
	static const ppu_static_module cellRtc;
	static const ppu_static_module cellRtcAlarm;
	static const ppu_static_module cellRudp;
	static const ppu_static_module cellSail;
	static const ppu_static_module cellSailRec;
	static const ppu_static_module cellSaveData;
	static const ppu_static_module cellMinisSaveData;
	static const ppu_static_module cellScreenShot;
	static const ppu_static_module cellSearch;
	static const ppu_static_module cellSheap;
	static const ppu_static_module cellSpudll;
	static const ppu_static_module cellSpurs;
	static const ppu_static_module cellSpursJq;
	static const ppu_static_module cellSsl;
	static const ppu_static_module cellSubDisplay;
	static const ppu_static_module cellSync;
	static const ppu_static_module cellSync2;
	static const ppu_static_module cellSysconf;
	static const ppu_static_module cellSysmodule;
	static const ppu_static_module cellSysutil;
	static const ppu_static_module cellSysutilAp;
	static const ppu_static_module cellSysutilAvc2;
	static const ppu_static_module cellSysutilAvcExt;
	static const ppu_static_module cellSysutilNpEula;
	static const ppu_static_module cellSysutilMisc;
	static const ppu_static_module cellUsbd;
	static const ppu_static_module cellUsbPspcm;
	static const ppu_static_module cellUserInfo;
	static const ppu_static_module cellVdec;
	static const ppu_static_module cellVideoExport;
	static const ppu_static_module cellVideoPlayerUtility;
	static const ppu_static_module cellVideoUpload;
	static const ppu_static_module cellVoice;
	static const ppu_static_module cellVpost;
	static const ppu_static_module libad_async;
	static const ppu_static_module libad_core;
	static const ppu_static_module libmedi;
	static const ppu_static_module libmixer;
	static const ppu_static_module libsnd3;
	static const ppu_static_module libsynth2;
	static const ppu_static_module sceNp;
	static const ppu_static_module sceNp2;
	static const ppu_static_module sceNpClans;
	static const ppu_static_module sceNpCommerce2;
	static const ppu_static_module sceNpMatchingInt;
	static const ppu_static_module sceNpSns;
	static const ppu_static_module sceNpTrophy;
	static const ppu_static_module sceNpTus;
	static const ppu_static_module sceNpUtil;
	static const ppu_static_module sys_io;
	static const ppu_static_module sys_net;
	static const ppu_static_module sysPrxForUser;
	static const ppu_static_module sys_libc;
	static const ppu_static_module sys_lv2dbg;
	static const ppu_static_module static_hle;
};

template <auto* Func>
ppu_static_function* ppu_module_manager::registered<Func>::info = nullptr;

// Call specified function directly if LLE is not available, call LLE equivalent in callback style otherwise
template <auto* Func, typename... Args, typename RT = std::invoke_result_t<decltype(Func), ppu_thread&, Args...>>
inline RT ppu_execute(ppu_thread& ppu, Args... args)
{
	vm::ptr<RT(Args...)> func = vm::cast(*ppu_module_manager::find_static_function<Func>().export_addr);
	return func(ppu, args...);
}

#define REG_FNID(module, nid, func) ppu_module_manager::register_static_function<&func>(#module, ppu_select_name(#func, nid), BIND_FUNC(func, ppu.cia = static_cast<u32>(ppu.lr) & ~3), ppu_generate_id(nid))

#define REG_FUNC(module, func) REG_FNID(module, #func, func)

#define REG_VNID(module, nid, var) ppu_module_manager::register_static_variable<&var>(#module, ppu_select_name(#var, nid), ppu_generate_id(nid))

#define REG_VAR(module, var) REG_VNID(module, #var, var)

#define UNIMPLEMENTED_FUNC(module) module.todo("%s()", __func__)
