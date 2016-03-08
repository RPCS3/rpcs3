#pragma once

enum SceDbgBreakOnErrorState : s32
{
	SCE_DBG_DISABLE_BREAK_ON_ERROR = 0,
	SCE_DBG_ENABLE_BREAK_ON_ERROR
};

extern psv_log_base sceDbg;
