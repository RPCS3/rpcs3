#pragma once

#include "ARMv7Function.h"
#include "ARMv7Callback.h"
#include "ErrorCodes.h"

namespace vm { using namespace psv; }

// HLE function information
struct arm_static_function
{
	const char* name;
	u32 index; // Index for arm_function_manager
	u32 flags;
};

// HLE variable information
struct arm_static_variable
{
	const char* name;
	vm::gvar<void>* var; // Pointer to variable address storage
	void(*init)(); // Variable initialization function
	u32 size;
	u32 align;
};

// HLE module information
class arm_static_module final
{
public:
	const std::string name;

	task_stack on_load;
	task_stack on_unload;

	std::unordered_map<u32, arm_static_function> functions;
	std::unordered_map<u32, arm_static_variable> variables;

public:
	arm_static_module(const char* name);

	arm_static_module(const char* name, void(*init)())
		: arm_static_module(name)
	{
		init();
	}

	arm_static_module(const char* name, void(*init)(arm_static_module* _this))
		: arm_static_module(name)
	{
		init(this);
	}
};

class arm_module_manager final
{
	friend class arm_static_module;

	static std::unordered_map<std::string, arm_static_module*>& access();

	static void register_module(arm_static_module* module);

	static arm_static_function& access_static_function(const char* module, u32 fnid);

	static arm_static_variable& access_static_variable(const char* module, u32 vnid);

public:
	static const arm_static_module* get_module(const std::string& name);

	template<typename T, T Func>
	static auto& register_static_function(const char* module, const char* name, arm_function_t func, u32 fnid)
	{
		auto& info = access_static_function(module, fnid);

		info.name  = name;
		info.index = arm_function_manager::register_function<T, Func>(func);
		info.flags = 0;

		return info;
	}

	template<typename T, T* Var>
	static auto& register_static_variable(const char* module, const char* name, u32 vnid)
	{
		static_assert(std::is_same<u32, typename T::addr_type>::value, "Static variable registration: vm::gvar<T> expected");

		auto& info = access_static_variable(module, vnid);

		info.name  = name;
		info.var   = reinterpret_cast<vm::gvar<void>*>(Var);
		info.init  = [] {};
		info.size  = SIZE_32(typename T::type);
		info.align = ALIGN_32(typename T::type);

		return info;
	}

	static const auto& get()
	{
		return access();
	}

	static const arm_static_module SceAppMgr;
	static const arm_static_module SceAppUtil;
	static const arm_static_module SceAudio;
	static const arm_static_module SceAudiodec;
	static const arm_static_module SceAudioenc;
	static const arm_static_module SceAudioIn;
	static const arm_static_module SceCamera;
	static const arm_static_module SceCodecEngine;
	static const arm_static_module SceCommonDialog;
	static const arm_static_module SceCpu;
	static const arm_static_module SceCtrl;
	static const arm_static_module SceDbg;
	static const arm_static_module SceDebugLed;
	static const arm_static_module SceDeci4p;
	static const arm_static_module SceDeflt;
	static const arm_static_module SceDipsw;
	static const arm_static_module SceDisplay;
	static const arm_static_module SceDisplayUser;
	static const arm_static_module SceFiber;
	static const arm_static_module SceFios;
	static const arm_static_module SceFpu;
	static const arm_static_module SceGxm;
	static const arm_static_module SceHttp;
	static const arm_static_module SceIme;
	static const arm_static_module SceIofilemgr;
	static const arm_static_module SceJpeg;
	static const arm_static_module SceJpegEnc;
	static const arm_static_module SceLibc;
	static const arm_static_module SceLibKernel;
	static const arm_static_module SceLibm;
	static const arm_static_module SceLibstdcxx;
	static const arm_static_module SceLibXml;
	static const arm_static_module SceLiveArea;
	static const arm_static_module SceLocation;
	static const arm_static_module SceMd5;
	static const arm_static_module SceModulemgr;
	static const arm_static_module SceMotion;
	static const arm_static_module SceMt19937;
	static const arm_static_module SceNet;
	static const arm_static_module SceNetCtl;
	static const arm_static_module SceNgs;
	static const arm_static_module SceNpBasic;
	static const arm_static_module SceNpCommon;
	static const arm_static_module SceNpManager;
	static const arm_static_module SceNpMatching;
	static const arm_static_module SceNpScore;
	static const arm_static_module SceNpUtility;
	static const arm_static_module ScePerf;
	static const arm_static_module ScePgf;
	static const arm_static_module ScePhotoExport;
	static const arm_static_module SceProcessmgr;
	static const arm_static_module SceRazorCapture;
	static const arm_static_module SceRtc;
	static const arm_static_module SceSas;
	static const arm_static_module SceScreenShot;
	static const arm_static_module SceSfmt;
	static const arm_static_module SceSha;
	static const arm_static_module SceSqlite;
	static const arm_static_module SceSsl;
	static const arm_static_module SceStdio;
	static const arm_static_module SceSulpha;
	static const arm_static_module SceSysmem;
	static const arm_static_module SceSysmodule;
	static const arm_static_module SceSystemGesture;
	static const arm_static_module SceThreadmgr;
	static const arm_static_module SceTouch;
	static const arm_static_module SceUlt;
	static const arm_static_module SceVideodec;
	static const arm_static_module SceVoice;
	static const arm_static_module SceVoiceQoS;
};

#define REG_FNID(module, nid, func) arm_module_manager::register_static_function<decltype(&func), &func>(#module, #func, BIND_FUNC(func), nid)

#define REG_VNID(module, nid, var) arm_module_manager::register_static_variable<decltype(var), &var>(#module, #var, nid)
