#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceFpu.h"

logs::channel sceFpu("sceFpu");

float sceFpuSinf(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuCosf(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuTanf(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuAtanf(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuAtan2f(float y, float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuAsinf(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuAcosf(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}


float sceFpuLogf(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuLog2f(float fs)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuLog10f(float fs)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuExpf(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuExp2f(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuExp10f(float x)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceFpuPowf(float x, float y)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceFpu, nid, name)

DECLARE(arm_module_manager::SceFpu)("SceFpu", []()
{
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
