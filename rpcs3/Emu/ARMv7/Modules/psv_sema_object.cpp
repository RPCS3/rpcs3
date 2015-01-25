#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"
#include "sceLibKernel.h"
#include "psv_sema_object.h"

psv_object_list_t<psv_sema_t, SCE_KERNEL_THREADMGR_UID_CLASS_SEMA> g_psv_sema_list;
