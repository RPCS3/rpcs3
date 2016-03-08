#pragma once

struct SceTouchPanelInfo
{
	le_t<s16> minAaX;
	le_t<s16> minAaY;
	le_t<s16> maxAaX;
	le_t<s16> maxAaY;
	le_t<s16> minDispX;
	le_t<s16> minDispY;
	le_t<s16> maxDispX;
	le_t<s16> maxDispY;
	u8 minForce;
	u8 maxForce;
	u8 rsv[30];
};

struct SceTouchReport
{
	u8 id;
	u8 force;
	le_t<s16> x;
	le_t<s16> y;
	s8 rsv[8];
	le_t<u16> info;
};

struct SceTouchData
{
	le_t<u64> timeStamp;
	le_t<u32> status;
	le_t<u32> reportNum;
	SceTouchReport report[8];
};

extern psv_log_base sceTouch;
