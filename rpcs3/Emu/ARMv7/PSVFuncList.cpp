#include "stdafx.h"
#include "Emu/System.h"
#include "PSVFuncList.h"

std::vector<psv_func>& g_psv_func_list = []() -> std::vector<psv_func>&
{
	auto v = new std::vector<psv_func>;

	psv_func f =
	{
		0xdeadbeef,
		"INVALID FUNCTION",
		new psv_func_detail::func_binder<u32>([]() -> u32
		{
			LOG_ERROR(HLE, "Unimplemented function found");
			Emu.Pause();

			return 0xffffffffu;
		}),
		nullptr,
	};
	v->push_back(f);

	return *v;
}();

void add_psv_func(psv_func& data)
{
	g_psv_func_list.push_back(data);
}

psv_func* get_psv_func_by_nid(u32 nid)
{
	for (auto& f : g_psv_func_list)
	{
		if (f.nid == nid)
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

void execute_psv_func_by_index(ARMv7Thread& CPU, u32 index)
{
	assert(index < g_psv_func_list.size());

	(*g_psv_func_list[index].func)(CPU);
}

extern psv_log_base& sceLibc;
extern psv_log_base& sceLibm;
extern psv_log_base& sceLibstdcxx;
extern psv_log_base& sceLibKernel;

void list_known_psv_modules()
{
	sceLibc.Log("");
	sceLibm.Log("");
	sceLibstdcxx.Log("");
	sceLibKernel.Log("");
}
