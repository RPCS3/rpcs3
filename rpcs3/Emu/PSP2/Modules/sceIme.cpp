#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceIme.h"

logs::channel sceIme("sceIme", logs::level::notice);

s32 sceImeOpen(vm::ptr<SceImeParam> param)
{
	throw EXCEPTION("");
}

s32 sceImeUpdate()
{
	throw EXCEPTION("");
}

s32 sceImeSetCaret(vm::cptr<SceImeCaret> caret)
{
	throw EXCEPTION("");
}

s32 sceImeSetPreeditGeometry(vm::cptr<SceImePreeditGeometry> preedit)
{
	throw EXCEPTION("");
}

s32 sceImeClose()
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) REG_FNID(SceIme, nid, name)

DECLARE(arm_module_manager::SceIme)("SceIme", []()
{
	REG_FUNC(0x0E050613, sceImeOpen);
	REG_FUNC(0x71D6898A, sceImeUpdate);
	REG_FUNC(0x889A8421, sceImeClose);
	REG_FUNC(0xD8342D2A, sceImeSetCaret);
	REG_FUNC(0x7B1EFAA5, sceImeSetPreeditGeometry);
});
