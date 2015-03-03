#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"
#include "Modules/sceLibKernel.h"
#include "Modules/psv_sema.h"
#include "Modules/psv_event_flag.h"
#include "Modules/psv_mutex.h"
#include "Modules/psv_cond.h"

psv_sema_list_t g_psv_sema_list;
psv_ef_list_t g_psv_ef_list;
psv_mutex_list_t g_psv_mutex_list;
psv_cond_list_t g_psv_cond_list;

void clear_all_psv_objects()
{
	g_psv_sema_list.clear();
	g_psv_ef_list.clear();
	g_psv_mutex_list.clear();
	g_psv_cond_list.clear();
}
