#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"
#include "sceLibKernel.h"
#include "psv_event_flag.h"

psv_event_flag_t::psv_event_flag_t(const char* name, u32 attr, u32 pattern)
	: attr(attr)
	, pattern(pattern)
{
	strcpy_trunc(this->name, name);
}
