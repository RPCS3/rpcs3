#pragma once

struct SceTouchPanelInfo
{
	s16 minAaX;
	s16 minAaY;
	s16 maxAaX;
	s16 maxAaY;
	s16 minDispX;
	s16 minDispY;
	s16 maxDispX;
	s16 maxDispY;
	u8 minForce;
	u8 maxForce;
	u8 rsv[30];
};

struct SceTouchReport
{
	u8 id;
	u8 force;
	s16 x;
	s16 y;
	s8 rsv[8];
	u16 info;
};

struct SceTouchData
{
	u64 timeStamp;
	u32 status;
	u32 reportNum;
	SceTouchReport report[8];
};

extern psv_log_base sceTouch;
