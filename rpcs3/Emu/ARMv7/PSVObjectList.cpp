#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"
#include "Modules/sceLibKernel.h"
#include "Modules/psv_sema.h"
#include "Modules/psv_event_flag.h"

psv_object_list_t<psv_sema_t, SCE_KERNEL_THREADMGR_UID_CLASS_SEMA> g_psv_sema_list;
psv_object_list_t<psv_event_flag_t, SCE_KERNEL_THREADMGR_UID_CLASS_EVENT_FLAG> g_psv_ef_list;

void clear_all_psv_objects()
{
	g_psv_sema_list.clear();
	g_psv_ef_list.clear();
}
