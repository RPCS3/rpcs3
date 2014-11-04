#include "stdafx.h"
#include "PSVFuncList.h"

std::vector<psv_func> g_psv_func_list;

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
extern psv_log_base& sceLibstdcxx;

void list_known_psv_modules()
{
	sceLibc.Log("");
	sceLibstdcxx.Log("");
}
