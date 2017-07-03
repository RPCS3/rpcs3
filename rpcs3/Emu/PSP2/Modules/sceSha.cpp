#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

logs::channel sceSha("sceSha");

#define REG_FUNC(nid, name) REG_FNID(SceSha, nid, name)

DECLARE(arm_module_manager::SceSha)("SceSha", []()
{
	//REG_FUNC(0xD19A9AA8, sceSha0Digest);
	//REG_FUNC(0xBCF6DB3A, sceSha0BlockInit);
	//REG_FUNC(0x37EF2AFC, sceSha0BlockUpdate);
	//REG_FUNC(0xBF0158C4, sceSha0BlockResult);

	//REG_FUNC(0xE1215C9D, sceSha1Digest);
	//REG_FUNC(0xB13D65AA, sceSha1BlockInit);
	//REG_FUNC(0x9007205E, sceSha1BlockUpdate);
	//REG_FUNC(0x0195DADF, sceSha1BlockResult);

	//REG_FUNC(0x1346D270, sceSha224Digest);
	//REG_FUNC(0x538F04CE, sceSha224BlockInit);
	//REG_FUNC(0xB5FD0160, sceSha224BlockUpdate);
	//REG_FUNC(0xA36ECF65, sceSha224BlockResult);

	//REG_FUNC(0xA337079C, sceSha256Digest);
	//REG_FUNC(0xE281374F, sceSha256BlockInit);
	//REG_FUNC(0xDAECA1F8, sceSha256BlockUpdate);
	//REG_FUNC(0x9B5BB4BA, sceSha256BlockResult);

	//REG_FUNC(0xA602C694, sceSha384Digest);
	//REG_FUNC(0x037AABE7, sceSha384BlockInit);
	//REG_FUNC(0x4B99DBB8, sceSha384BlockUpdate);
	//REG_FUNC(0x30D5C919, sceSha384BlockResult);

	//REG_FUNC(0x5DC0B916, sceSha512Digest);
	//REG_FUNC(0xE017A9CD, sceSha512BlockInit);
	//REG_FUNC(0x669281E8, sceSha512BlockUpdate);
	//REG_FUNC(0x26146A16, sceSha512BlockResult);
});
