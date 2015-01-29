#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceSsl;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceSsl, #name, name)

psv_log_base sceSsl("SceSsl", []()
{
	sceSsl.on_load = nullptr;
	sceSsl.on_unload = nullptr;
	sceSsl.on_stop = nullptr;

	//REG_FUNC(0x3C733316, sceSslInit);
	//REG_FUNC(0x03CE6E3A, sceSslTerm);
	//REG_FUNC(0xBD203262, sceSslGetMemoryPoolStats);
	//REG_FUNC(0x901C5C15, sceSslGetSerialNumber);
	//REG_FUNC(0x9B2F1BC1, sceSslGetSubjectName);
	//REG_FUNC(0x412711E5, sceSslGetIssuerName);
	//REG_FUNC(0x70DEA174, sceSslGetNotBefore);
	//REG_FUNC(0xF5ED7B68, sceSslGetNotAfter);
	//REG_FUNC(0x95E14CA6, sceSslGetNameEntryCount);
	//REG_FUNC(0x2A857867, sceSslGetNameEntryInfo);
	//REG_FUNC(0xC73687E4, sceSslFreeSslCertName);
});
