#pragma once

#include "Utilities/Config.h"
#include "PPUFunction.h"
#include "PPUCallback.h"
#include "ErrorCodes.h"

namespace vm { using namespace ps3; }

// Generate FNID or VNID for given name
extern u32 ppu_generate_id(const char* name);

// Flags set with REG_FUNC
enum ppu_static_function_flags : u32
{
	MFF_FORCED_HLE = (1 << 0), // Always call HLE function (TODO: deactivated)

	MFF_PERFECT = MFF_FORCED_HLE, // Indicates that function is completely implemented and can replace LLE implementation
};

// HLE function information
struct ppu_static_function
{
	const char* name;
	u32 index; // Index for ppu_function_manager
	u32 flags;
};

// HLE variable information
struct ppu_static_variable
{
	const char* name;
	vm::gvar<void>* var; // Pointer to variable address storage
	void(*init)(); // Variable initialization function
	u32 size;
	u32 align;
};

// HLE module information
class ppu_static_module final
{
public:
	const std::string name;

	task_stack on_load;
	task_stack on_unload;

	std::unordered_map<u32, ppu_static_function> functions;
	std::unordered_map<u32, ppu_static_variable> variables;

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

public:
	static const ppu_static_module* get_module(const std::string& name);

	template<typename T, T Func>
	static void register_static_function(const char* module, const char* name, ppu_function_t func, u32 fnid, u32 flags)
	{
		auto& info = access_static_function(module, fnid);

		info.name  = name;
		info.index = ppu_function_manager::register_function<T, Func>(func);
		info.flags = flags;
	}

	template<typename T, T* Var>
	static void register_static_variable(const char* module, const char* name, u32 vnid, void(*init)())
	{
		static_assert(std::is_same<CV u32, CV typename T::addr_type>::value, "Static variable registration: vm::gvar<T> expected");

		auto& info = access_static_variable(module, vnid);

		info.name  = name;
		info.var   = reinterpret_cast<vm::gvar<void>*>(Var);
		info.init  = init ? init : [] {};
		info.size  = SIZE_32(typename T::type);
		info.align = ALIGN_32(typename T::type);
	}

	static const ppu_static_module cellAdec;
	static const ppu_static_module cellAtrac;
	static const ppu_static_module cellAtracMulti;
	static const ppu_static_module cellAudio;
	static const ppu_static_module cellAvconfExt;
	static const ppu_static_module cellBGDL;
	static const ppu_static_module cellCamera;
	static const ppu_static_module cellCelp8Enc;
	static const ppu_static_module cellCelpEnc;
	static const ppu_static_module cellDaisy;
	static const ppu_static_module cellDmux;
	static const ppu_static_module cellFiber;
	static const ppu_static_module cellFont;
	static const ppu_static_module cellFontFT;
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
	static const ppu_static_module cellMic;
	static const ppu_static_module cellMusic;
	static const ppu_static_module cellMusicDecode;
	static const ppu_static_module cellMusicExport;
	static const ppu_static_module cellNetCtl;
	static const ppu_static_module cellOskDialog;
	static const ppu_static_module cellOvis;
	static const ppu_static_module cellPamf;
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
	static const ppu_static_module cellSubdisplay;
	static const ppu_static_module cellSync;
	static const ppu_static_module cellSync2;
	static const ppu_static_module cellSysconf;
	static const ppu_static_module cellSysmodule;
	static const ppu_static_module cellSysutil;
	static const ppu_static_module cellSysutilAp;
	static const ppu_static_module cellSysutilAvc;
	static const ppu_static_module cellSysutilAvc2;
	static const ppu_static_module cellSysutilMisc;
	static const ppu_static_module cellUsbd;
	static const ppu_static_module cellUsbPspcm;
	static const ppu_static_module cellUserInfo;
	static const ppu_static_module cellVdec;
	static const ppu_static_module cellVideoExport;
	static const ppu_static_module cellVideoUpload;
	static const ppu_static_module cellVoice;
	static const ppu_static_module cellVpost;
	static const ppu_static_module libmixer;
	static const ppu_static_module libsnd3;
	static const ppu_static_module libsynth2;
	static const ppu_static_module sceNp;
	static const ppu_static_module sceNp2;
	static const ppu_static_module sceNpClans;
	static const ppu_static_module sceNpCommerce2;
	static const ppu_static_module sceNpSns;
	static const ppu_static_module sceNpTrophy;
	static const ppu_static_module sceNpTus;
	static const ppu_static_module sceNpUtil;
	static const ppu_static_module sys_io;
	static const ppu_static_module libnet;
	static const ppu_static_module sysPrxForUser;
	static const ppu_static_module sys_libc;
	static const ppu_static_module sys_lv2dbg;
};

// Call specified function directly if LLE is not available, call LLE equivalent in callback style otherwise
template<typename T, T Func, typename... Args, typename RT = std::result_of_t<T(Args...)>>
inline RT ppu_execute_function_or_callback(const char* name, PPUThread& ppu, Args&&... args)
{
	const auto previous_function = ppu.last_function; // TODO

	try
	{
		return Func(std::forward<Args>(args)...);
	}
	catch (const std::exception&)
	{
		LOG_ERROR(PPU, "Function '%s' aborted", ppu.last_function);
		ppu.last_function = previous_function;
		throw;
	}
	catch (...)
	{
		LOG_WARNING(PPU, "Function '%s' aborted", ppu.last_function);
		ppu.last_function = previous_function;
		throw;
	}

	ppu.last_function = previous_function;
}

#define CALL_FUNC(ppu, func, ...) ppu_execute_function_or_callback<decltype(&func), &func>(#func, ppu, __VA_ARGS__)

#define REG_FNID(module, nid, func, ...) ppu_module_manager::register_static_function<decltype(&func), &func>(#module, #func, BIND_FUNC(func), nid, {__VA_ARGS__})

#define REG_FUNC(module, func, ...) REG_FNID(module, ppu_generate_id(#func), func, __VA_ARGS__)

#define REG_VNID(module, nid, var, ...) ppu_module_manager::register_static_variable<decltype(var), &var>(#module, #var, nid, {__VA_ARGS__})

#define REG_VAR(module, var, ...) REG_VNID(module, ppu_generate_id(#var), var, __VA_ARGS__)

#define UNIMPLEMENTED_FUNC(module) module.todo("%s", __func__)
