#pragma once

#include "sceNpCommon.h"

struct SceNpOptParam
{
	le_t<u32> optParamSize;
};

using SceNpServiceStateCallback = void(SceNpServiceState state, vm::ptr<void> userdata);

extern psv_log_base sceNpManager;
