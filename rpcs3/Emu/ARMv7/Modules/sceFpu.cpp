#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceFpu.h"

float sceFpuSinf(float x)
{
	throw EXCEPTION("");
}

float sceFpuCosf(float x)
{
	throw EXCEPTION("");
}

float sceFpuTanf(float x)
{
	throw EXCEPTION("");
}

float sceFpuAtanf(float x)
{
	throw EXCEPTION("");
}

float sceFpuAtan2f(float y, float x)
{
	throw EXCEPTION("");
}

float sceFpuAsinf(float x)
{
	throw EXCEPTION("");
}

float sceFpuAcosf(float x)
{
	throw EXCEPTION("");
}


float sceFpuLogf(float x)
{
	throw EXCEPTION("");
}

float sceFpuLog2f(float fs)
{
	throw EXCEPTION("");
}

float sceFpuLog10f(float fs)
{
	throw EXCEPTION("");
}

float sceFpuExpf(float x)
{
	throw EXCEPTION("");
}

float sceFpuExp2f(float x)
{
	throw EXCEPTION("");
}

float sceFpuExp10f(float x)
{
	throw EXCEPTION("");
}

float sceFpuPowf(float x, float y)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceFpu, #name, name)

psv_log_base sceFpu("SceFpu", []()
{
	sceFpu.on_load = nullptr;
	sceFpu.on_unload = nullptr;
	sceFpu.on_stop = nullptr;
	sceFpu.on_error = nullptr;

	//REG_FUNC(0x33E1AC14, sceFpuSinf);
	//REG_FUNC(0xDB66BA89, sceFpuCosf);
	//REG_FUNC(0x6FBDA1C9, sceFpuTanf);
	//REG_FUNC(0x53FF26AF, sceFpuAtanf);
	//REG_FUNC(0xC8A4989B, sceFpuAtan2f);
	//REG_FUNC(0x4D1AE0F1, sceFpuAsinf);
	//REG_FUNC(0x64A8F9FE, sceFpuAcosf);
	//REG_FUNC(0x936F0D27, sceFpuLogf);
	//REG_FUNC(0x19881EC8, sceFpuLog2f);
	//REG_FUNC(0xABBB6168, sceFpuLog10f);
	//REG_FUNC(0xEFA16C6E, sceFpuExpf);
	//REG_FUNC(0xA3A88AD0, sceFpuExp2f);
	//REG_FUNC(0x35652326, sceFpuExp10f);
	//REG_FUNC(0xDF622E56, sceFpuPowf);
});
