#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceIme;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceIme, #name, name)

psv_log_base sceIme("SceIme", []()
{
	sceIme.on_load = nullptr;
	sceIme.on_unload = nullptr;
	sceIme.on_stop = nullptr;

	//REG_FUNC(0x0E050613, sceImeOpen);
	//REG_FUNC(0x71D6898A, sceImeUpdate);
	//REG_FUNC(0x889A8421, sceImeClose);
	//REG_FUNC(0xD8342D2A, sceImeSetCaret);
	//REG_FUNC(0x7B1EFAA5, sceImeSetPreeditGeometry);
});
