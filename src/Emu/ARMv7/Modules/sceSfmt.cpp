#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceSfmt;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceSfmt, #name, name)

psv_log_base sceSfmt("SceSfmt", []()
{
	sceSfmt.on_load = nullptr;
	sceSfmt.on_unload = nullptr;
	sceSfmt.on_stop = nullptr;
	sceSfmt.on_error = nullptr;

	//REG_FUNC(0x8FF464C9, sceSfmt11213InitGenRand);
	//REG_FUNC(0xBAF5F058, sceSfmt11213InitByArray);
	//REG_FUNC(0xFB281CD7, sceSfmt11213GenRand32);
	//REG_FUNC(0xAFEDD6E1, sceSfmt11213GenRand64);
	//REG_FUNC(0xFD696585, sceSfmt11213FillArray32);
	//REG_FUNC(0x7A412A29, sceSfmt11213FillArray64);

	//REG_FUNC(0x02E8D906, sceSfmt1279InitGenRand);
	//REG_FUNC(0xC25D9ACE, sceSfmt1279InitByArray);
	//REG_FUNC(0x9B4A48DF, sceSfmt1279GenRand32);
	//REG_FUNC(0xA2C5EE14, sceSfmt1279GenRand64);
	//REG_FUNC(0xE7F63838, sceSfmt1279FillArray32);
	//REG_FUNC(0xDB3832EB, sceSfmt1279FillArray64);

	//REG_FUNC(0xDC6B23B0, sceSfmt132049InitGenRand);
	//REG_FUNC(0xDC69294A, sceSfmt132049InitByArray);
	//REG_FUNC(0x795F9644, sceSfmt132049GenRand32);
	//REG_FUNC(0xBBD80AC4, sceSfmt132049GenRand64);
	//REG_FUNC(0xD891A99F, sceSfmt132049FillArray32);
	//REG_FUNC(0x68AD7866, sceSfmt132049FillArray64);

	//REG_FUNC(0x2AFACB0B, sceSfmt19937InitGenRand);
	//REG_FUNC(0xAC496C8C, sceSfmt19937InitByArray);
	//REG_FUNC(0xF0557157, sceSfmt19937GenRand32);
	//REG_FUNC(0xE66F2502, sceSfmt19937GenRand64);
	//REG_FUNC(0xA1C654D8, sceSfmt19937FillArray32);
	//REG_FUNC(0xE74BA81C, sceSfmt19937FillArray64);

	//REG_FUNC(0x86DDE4A7, sceSfmt216091InitGenRand);
	//REG_FUNC(0xA9CF6616, sceSfmt216091InitByArray);
	//REG_FUNC(0x4A972DCD, sceSfmt216091GenRand32);
	//REG_FUNC(0x23369ABF, sceSfmt216091GenRand64);
	//REG_FUNC(0xDD4256F0, sceSfmt216091FillArray32);
	//REG_FUNC(0xA1CE5628, sceSfmt216091FillArray64);

	//REG_FUNC(0xB8E5A0BB, sceSfmt2281InitGenRand);
	//REG_FUNC(0xAB3AD459, sceSfmt2281InitByArray);
	//REG_FUNC(0x84BB4ADB, sceSfmt2281GenRand32);
	//REG_FUNC(0x3CC47146, sceSfmt2281GenRand64);
	//REG_FUNC(0xBB89D8F0, sceSfmt2281FillArray32);
	//REG_FUNC(0x17C10E2D, sceSfmt2281FillArray64);

	//REG_FUNC(0xE9F8CB9A, sceSfmt4253InitGenRand);
	//REG_FUNC(0xC4D7AA2D, sceSfmt4253InitByArray);
	//REG_FUNC(0x8791E2EF, sceSfmt4253GenRand32);
	//REG_FUNC(0x6C0E5E3C, sceSfmt4253GenRand64);
	//REG_FUNC(0x59A1B9FC, sceSfmt4253FillArray32);
	//REG_FUNC(0x01683CDD, sceSfmt4253FillArray64);

	//REG_FUNC(0xCF1C8C38, sceSfmt44497InitGenRand);
	//REG_FUNC(0x16D8AA5E, sceSfmt44497InitByArray);
	//REG_FUNC(0xF869DFDC, sceSfmt44497GenRand32);
	//REG_FUNC(0xD411A9A6, sceSfmt44497GenRand64);
	//REG_FUNC(0x1C38322A, sceSfmt44497FillArray32);
	//REG_FUNC(0x908F1122, sceSfmt44497FillArray64);

	//REG_FUNC(0x76A5D8CA, sceSfmt607InitGenRand);
	//REG_FUNC(0xCC6DABA0, sceSfmt607InitByArray);
	//REG_FUNC(0x8A0BF859, sceSfmt607GenRand32);
	//REG_FUNC(0x5E880862, sceSfmt607GenRand64);
	//REG_FUNC(0xA288ADB9, sceSfmt607FillArray32);
	//REG_FUNC(0x1520D408, sceSfmt607FillArray64);

	//REG_FUNC(0x2FF42588, sceSfmt86243InitGenRand);
	//REG_FUNC(0x81B67AB5, sceSfmt86243InitByArray);
	//REG_FUNC(0x569BF903, sceSfmt86243GenRand32);
	//REG_FUNC(0x8E25CBA8, sceSfmt86243GenRand64);
	//REG_FUNC(0xC297E6B1, sceSfmt86243FillArray32);
	//REG_FUNC(0xF7FFE87C, sceSfmt86243FillArray64);
});
