#pragma once

struct SceAppMgrEvent
{
	le_t<s32> event;
	le_t<s32> appId;
	char param[56];
};
