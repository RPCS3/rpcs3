#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"
#include "sceLibKernel.h"
#include "psv_sema.h"

psv_sema_t::psv_sema_t(const char* name, u32 attr, s32 init_value, s32 max_value)
	: attr(attr)
	, value(init_value)
	, max(max_value)
{
	strcpy_trunc(this->name, name);
}
