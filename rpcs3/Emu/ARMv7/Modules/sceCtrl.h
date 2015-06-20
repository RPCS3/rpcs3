#pragma once

struct SceCtrlData
{
	le_t<u64> timeStamp;
	le_t<u32> buttons;
	u8 lx;
	u8 ly;
	u8 rx;
	u8 ry;
	u8 reserved[16];
};

struct SceCtrlRapidFireRule
{
	le_t<u32> uiMask;
	le_t<u32> uiTrigger;
	le_t<u32> uiTarget;
	le_t<u32> uiDelay;
	le_t<u32> uiMake;
	le_t<u32> uiBreak;
};

extern psv_log_base sceCtrl;
