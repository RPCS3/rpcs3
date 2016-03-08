#pragma once

struct SceDisplayFrameBuf
{
	le_t<u32> size;
	vm::lptr<void> base;
	le_t<u32> pitch;
	le_t<u32> pixelformat;
	le_t<u32> width;
	le_t<u32> height;
};

extern psv_log_base sceDisplay;
