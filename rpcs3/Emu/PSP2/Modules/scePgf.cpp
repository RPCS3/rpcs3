#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "scePgf.h"

logs::channel scePgf("scePgf");

#define REG_FUNC(nid, name) REG_FNID(ScePgf, nid, name)

DECLARE(arm_module_manager::ScePgf)("ScePgf", []()
{
	//REG_FUNC(0x1055ABA3, sceFontNewLib);
	//REG_FUNC(0x07EE1733, sceFontDoneLib);
	//REG_FUNC(0xDE47674C, sceFontSetResolution);
	//REG_FUNC(0x9F842307, sceFontGetNumFontList);
	//REG_FUNC(0xD56DCCEA, sceFontGetFontList);
	//REG_FUNC(0x8DFBAE1B, sceFontFindOptimumFont);
	//REG_FUNC(0x51061D87, sceFontFindFont);
	//REG_FUNC(0xAB034738, sceFontGetFontInfoByIndexNumber);
	//REG_FUNC(0xBD2DFCFF, sceFontOpen);
	//REG_FUNC(0xE260E740, sceFontOpenUserFile);
	//REG_FUNC(0xB23ED47C, sceFontOpenUserMemory);
	//REG_FUNC(0x4A7293E9, sceFontClose);
	//REG_FUNC(0xF9414FA2, sceFontGetFontInfo);
	//REG_FUNC(0x6FD1BA65, sceFontGetCharInfo);
	//REG_FUNC(0x70C86B3E, sceFontGetCharImageRect);
	//REG_FUNC(0xAB45AAD3, sceFontGetCharGlyphImage);
	//REG_FUNC(0xEB589530, sceFontGetCharGlyphImage_Clip);
	//REG_FUNC(0x9E38F4D6, sceFontPixelToPointH);
	//REG_FUNC(0x7B45E2D1, sceFontPixelToPointV);
	//REG_FUNC(0x39B9AEFF, sceFontPointToPixelH);
	//REG_FUNC(0x03F10EC8, sceFontPointToPixelV);
	//REG_FUNC(0x8D5B44DF, sceFontSetAltCharacterCode);
	//REG_FUNC(0x7D8CB13B, sceFontFlush);
});
