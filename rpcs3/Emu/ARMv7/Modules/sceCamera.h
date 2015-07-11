#pragma once

struct SceCameraInfo
{
	le_t<u32> sizeThis;
	le_t<u32> wPriority;
	le_t<u32> wFormat;
	le_t<u32> wResolution;
	le_t<u32> wFramerate;
	le_t<u32> wWidth;
	le_t<u32> wHeight;
	le_t<u32> wRange;
	le_t<u32> _padding_0;
	le_t<u32> sizeIBase;
	le_t<u32> sizeUBase;
	le_t<u32> sizeVBase;
	vm::lptr<void> pvIBase;
	vm::lptr<void> pvUBase;
	vm::lptr<void> pvVBase;
	le_t<u32> wPitch;
	le_t<u32> wBuffer;
};

struct SceCameraRead
{
	le_t<u32> sizeThis;
	le_t<s32> dwMode;
	le_t<s32> _padding_0;
	le_t<s32> dwStatus;
	le_t<u32> qwFrame;
	le_t<u32> qwTimestamp;
	le_t<u32> sizeIBase;
	le_t<u32> sizeUBase;
	le_t<u32> sizeVBase;
	vm::lptr<void> pvIBase;
	vm::lptr<void> pvUBase;
	vm::lptr<void> pvVBase;
};

extern psv_log_base sceCamera;
