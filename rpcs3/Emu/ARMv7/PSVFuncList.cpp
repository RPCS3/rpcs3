#include "stdafx.h"
#include <unordered_map>
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "ARMv7Thread.h"
#include "PSVFuncList.h"

std::vector<psv_func> g_psv_func_list;

void add_psv_func(psv_func& data)
{
	// setup special functions (without NIDs)
	if (!g_psv_func_list.size())
	{
		psv_func unimplemented;
		unimplemented.nid = 0;
		unimplemented.name = "Special function (unimplemented stub)";
		unimplemented.func.reset(new psv_func_detail::func_binder<void, ARMv7Context&>([](ARMv7Context& context)
		{
			context.thread.m_last_syscall = vm::psv::read32(context.thread.PC + 4);
			throw "Unimplemented function executed";
		}));
		g_psv_func_list.push_back(unimplemented);

		psv_func hle_return;
		hle_return.nid = 1;
		hle_return.name = "Special function (return from HLE)";
		hle_return.func.reset(new psv_func_detail::func_binder<void, ARMv7Context&>([](ARMv7Context& context)
		{
			context.thread.FastStop();
		}));
		g_psv_func_list.push_back(hle_return);
	}

	g_psv_func_list.push_back(data);
}

psv_func* get_psv_func_by_nid(u32 nid)
{
	for (auto& f : g_psv_func_list)
	{
		if (f.nid == nid && &f - g_psv_func_list.data() >= 2 /* special functions count */)
		{
			return &f;
		}
	}

	return nullptr;
}

u32 get_psv_func_index(psv_func* func)
{
	auto res = func - g_psv_func_list.data();

	assert((size_t)res < g_psv_func_list.size());

	return (u32)res;
}

void execute_psv_func_by_index(ARMv7Context& context, u32 index)
{
	assert(index < g_psv_func_list.size());
	
	auto old_last_syscall = context.thread.m_last_syscall;
	context.thread.m_last_syscall = g_psv_func_list[index].nid;

	(*g_psv_func_list[index].func)(context);

	context.thread.m_last_syscall = old_last_syscall;
}

extern psv_log_base sceLibc;
extern psv_log_base sceLibm;
extern psv_log_base sceLibstdcxx;
extern psv_log_base sceLibKernel;

void list_known_psv_modules()
{
	sceLibc.Log("");
	sceLibm.Log("");
	sceLibstdcxx.Log("");
	sceLibKernel.Log("");
}
