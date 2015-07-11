#pragma once

struct SceCodecEnginePmonProcessorLoad
{
	le_t<u32> size;
	le_t<u32> average;
};

extern psv_log_base sceCodecEngine;
