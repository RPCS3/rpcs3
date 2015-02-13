#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"
#include "sceLibKernel.h"
#include "psv_mutex.h"

psv_mutex_t::psv_mutex_t(const char* name, u32 attr, s32 count)
	: attr(attr)
	, count(count)
{
	strcpy_trunc(this->name, name);
}
