#include "stdafx.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceLibc;

namespace sce_libc_func
{
	void __cxa_atexit()
	{
		sceLibc.Todo(__FUNCTION__);
	}

	void exit()
	{
		sceLibc.Todo(__FUNCTION__);
	}

	void printf()
	{
		sceLibc.Todo(__FUNCTION__);
	}

	void __cxa_set_dso_handle_main()
	{
		sceLibc.Todo(__FUNCTION__);
	}
}

psv_log_base sceLibc = []()
{
	psv_log_base* module = new psv_log_base("sceLibc");

#define REG_FUNC(nid, name) reg_psv_func(nid, module, sce_libc_func::name)

	REG_FUNC(0x33b83b70, __cxa_atexit);
	REG_FUNC(0x826bbbaf, exit);
	REG_FUNC(0x9a004680, printf);
	REG_FUNC(0xbfe02b3a, __cxa_set_dso_handle_main);

	return std::move(*module);
}();
