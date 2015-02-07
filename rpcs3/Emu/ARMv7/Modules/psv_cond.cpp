#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"
#include "sceLibKernel.h"
#include "psv_cond.h"

psv_cond_t::psv_cond_t(const char* name, u32 attr, s32 mutexId)
	: attr(attr)
	, mutexId(mutexId)
{
	strcpy_trunc(this->name, name);
}
