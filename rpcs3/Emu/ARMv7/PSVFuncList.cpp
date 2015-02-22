#include "stdafx.h"
#include "ARMv7Thread.h"
#include "PSVFuncList.h"

std::vector<psv_func> g_psv_func_list;
std::vector<psv_log_base*> g_psv_modules;

u32 add_psv_func(psv_func data)
{
	for (auto& f : g_psv_func_list)
	{
		if (f.nid == data.nid)
		{
			const u32 index = (u32)(&f - g_psv_func_list.data());

			if (index < SFI_MAX)
			{
				continue;
			}

			if (data.func)
			{
				f.func = data.func;
			}
		
			return index;
		}
	}

	g_psv_func_list.push_back(data);
	return (u32)(g_psv_func_list.size() - 1);
}

psv_func* get_psv_func_by_nid(u32 nid, u32* out_index)
{
	for (auto& f : g_psv_func_list)
	{
		if (f.nid == nid && &f - g_psv_func_list.data())
		{
			const u32 index = (u32)(&f - g_psv_func_list.data());

			if (index < SFI_MAX)
			{
				continue;
			}

			if (out_index)
			{
				*out_index = index;
			}

			return &f;
		}
	}

	return nullptr;
}

psv_func* get_psv_func_by_index(u32 index)
{
	if (index >= g_psv_func_list.size())
	{
		return nullptr;
	}

	return &g_psv_func_list[index];
}

void execute_psv_func_by_index(ARMv7Context& context, u32 index)
{
	if (auto func = get_psv_func_by_index(index))
	{
		auto old_last_syscall = context.thread.m_last_syscall;
		context.thread.m_last_syscall = func->nid;

		if (func->func)
		{
			func->func(context);
		}
		else
		{
			throw "Unimplemented function";
		}

		context.thread.m_last_syscall = old_last_syscall;
	}
	else
	{
		throw "Invalid function index";
	}
}

extern psv_log_base sceAppMgr;
extern psv_log_base sceAppUtil;
extern psv_log_base sceAudio;
extern psv_log_base sceAudiodec;
extern psv_log_base sceAudioenc;
extern psv_log_base sceAudioIn;
extern psv_log_base sceCamera;
extern psv_log_base sceCodecEngine;
extern psv_log_base sceCommonDialog;
extern psv_log_base sceCtrl;
extern psv_log_base sceDbg;
extern psv_log_base sceDeci4p;
extern psv_log_base sceDeflt;
extern psv_log_base sceDisplay;
extern psv_log_base sceFiber;
extern psv_log_base sceFios;
extern psv_log_base sceFpu;
extern psv_log_base sceGxm;
extern psv_log_base sceHttp;
extern psv_log_base sceIme;
extern psv_log_base sceJpeg;
extern psv_log_base sceJpegEnc;
extern psv_log_base sceLibc;
extern psv_log_base sceLibKernel;
extern psv_log_base sceLibm;
extern psv_log_base sceLibstdcxx;
extern psv_log_base sceLiveArea;
extern psv_log_base sceLocation;
extern psv_log_base sceMd5;
extern psv_log_base sceMotion;
extern psv_log_base sceMt19937;
extern psv_log_base sceNet;
extern psv_log_base sceNetCtl;
extern psv_log_base sceNgs;
extern psv_log_base sceNpBasic;
extern psv_log_base sceNpCommon;
extern psv_log_base sceNpManager;
extern psv_log_base sceNpMatching;
extern psv_log_base sceNpScore;
extern psv_log_base sceNpUtility;
extern psv_log_base scePerf;
extern psv_log_base scePgf;
extern psv_log_base scePhotoExport;
extern psv_log_base sceRazorCapture;
extern psv_log_base sceRtc;
extern psv_log_base sceSas;
extern psv_log_base sceScreenShot;
extern psv_log_base sceSfmt;
extern psv_log_base sceSha;
extern psv_log_base sceSqlite;
extern psv_log_base sceSsl;
extern psv_log_base sceSulpha;
extern psv_log_base sceSysmodule;
extern psv_log_base sceSystemGesture;
extern psv_log_base sceTouch;
extern psv_log_base sceUlt;
extern psv_log_base sceVideodec;
extern psv_log_base sceVoice;
extern psv_log_base sceVoiceQoS;
extern psv_log_base sceXml;

void initialize_psv_modules()
{
	assert(!g_psv_func_list.size() && !g_psv_modules.size());

	// fill module list
	g_psv_modules.push_back(&sceAppMgr);
	g_psv_modules.push_back(&sceAppUtil);
	g_psv_modules.push_back(&sceAudio);
	g_psv_modules.push_back(&sceAudiodec);
	g_psv_modules.push_back(&sceAudioenc);
	g_psv_modules.push_back(&sceAudioIn);
	g_psv_modules.push_back(&sceCamera);
	g_psv_modules.push_back(&sceCodecEngine);
	g_psv_modules.push_back(&sceCommonDialog);
	g_psv_modules.push_back(&sceCtrl);
	g_psv_modules.push_back(&sceDbg);
	g_psv_modules.push_back(&sceDeci4p);
	g_psv_modules.push_back(&sceDeflt);
	g_psv_modules.push_back(&sceDisplay);
	g_psv_modules.push_back(&sceFiber);
	g_psv_modules.push_back(&sceFios);
	g_psv_modules.push_back(&sceFpu);
	g_psv_modules.push_back(&sceGxm);
	g_psv_modules.push_back(&sceHttp);
	g_psv_modules.push_back(&sceIme);
	g_psv_modules.push_back(&sceJpeg);
	g_psv_modules.push_back(&sceJpegEnc);
	g_psv_modules.push_back(&sceLibc);
	g_psv_modules.push_back(&sceLibKernel);
	g_psv_modules.push_back(&sceLibm);
	g_psv_modules.push_back(&sceLibstdcxx);
	g_psv_modules.push_back(&sceLiveArea);
	g_psv_modules.push_back(&sceLocation);
	g_psv_modules.push_back(&sceMd5);
	g_psv_modules.push_back(&sceMotion);
	g_psv_modules.push_back(&sceMt19937);
	g_psv_modules.push_back(&sceNet);
	g_psv_modules.push_back(&sceNetCtl);
	g_psv_modules.push_back(&sceNgs);
	g_psv_modules.push_back(&sceNpBasic);
	g_psv_modules.push_back(&sceNpCommon);
	g_psv_modules.push_back(&sceNpManager);
	g_psv_modules.push_back(&sceNpMatching);
	g_psv_modules.push_back(&sceNpScore);
	g_psv_modules.push_back(&sceNpUtility);
	g_psv_modules.push_back(&scePerf);
	g_psv_modules.push_back(&scePgf);
	g_psv_modules.push_back(&scePhotoExport);
	g_psv_modules.push_back(&sceRazorCapture);
	g_psv_modules.push_back(&sceRtc);
	g_psv_modules.push_back(&sceSas);
	g_psv_modules.push_back(&sceScreenShot);
	g_psv_modules.push_back(&sceSfmt);
	g_psv_modules.push_back(&sceSha);
	g_psv_modules.push_back(&sceSqlite);
	g_psv_modules.push_back(&sceSsl);
	g_psv_modules.push_back(&sceSulpha);
	g_psv_modules.push_back(&sceSysmodule);
	g_psv_modules.push_back(&sceSystemGesture);
	g_psv_modules.push_back(&sceTouch);
	g_psv_modules.push_back(&sceUlt);
	g_psv_modules.push_back(&sceVideodec);
	g_psv_modules.push_back(&sceVoice);
	g_psv_modules.push_back(&sceVoiceQoS);
	g_psv_modules.push_back(&sceXml);

	// setup special functions (without NIDs)
	g_psv_func_list.resize(SFI_MAX);

	psv_func& hle_return = g_psv_func_list[SFI_HLE_RETURN];
	hle_return.nid = 0;
	hle_return.name = "HLE_RETURN";
	hle_return.func = [](ARMv7Context& context)
	{
		context.thread.FastStop();
	};

	// load functions
	for (auto module : g_psv_modules)
	{
		module->Init();
	}
}

void finalize_psv_modules()
{
	for (auto module : g_psv_modules)
	{
		if (module->on_stop)
		{
			module->on_stop();
		}
	}

	g_psv_func_list.clear();
	g_psv_modules.clear();
}
