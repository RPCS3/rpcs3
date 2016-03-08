#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceSsl.h"

s32 sceSslInit(u32 poolSize)
{
	throw EXCEPTION("");
}

s32 sceSslTerm()
{
	throw EXCEPTION("");
}

s32 sceSslGetMemoryPoolStats(vm::ptr<SceSslMemoryPoolStats> currentStat)
{
	throw EXCEPTION("");
}

s32 sceSslGetSerialNumber(vm::ptr<SceSslCert> sslCert, vm::cpptr<u8> sboData, vm::ptr<u32> sboLen)
{
	throw EXCEPTION("");
}

vm::ptr<SceSslCertName> sceSslGetSubjectName(vm::ptr<SceSslCert> sslCert)
{
	throw EXCEPTION("");
}

vm::ptr<SceSslCertName> sceSslGetIssuerName(vm::ptr<SceSslCert> sslCert)
{
	throw EXCEPTION("");
}

s32 sceSslGetNotBefore(vm::ptr<SceSslCert> sslCert, vm::ptr<u64> begin)
{
	throw EXCEPTION("");
}

s32 sceSslGetNotAfter(vm::ptr<SceSslCert> sslCert, vm::ptr<u64> limit)
{
	throw EXCEPTION("");
}

s32 sceSslGetNameEntryCount(vm::ptr<SceSslCertName> certName)
{
	throw EXCEPTION("");
}

s32 sceSslGetNameEntryInfo(vm::ptr<SceSslCertName> certName, s32 entryNum, vm::ptr<char> oidname, u32 maxOidnameLen, vm::ptr<u8> value, u32 maxValueLen, vm::ptr<u32> valueLen)
{
	throw EXCEPTION("");
}

s32 sceSslFreeSslCertName(vm::ptr<SceSslCertName> certName)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceSsl, #name, name)

psv_log_base sceSsl("SceSsl", []()
{
	sceSsl.on_load = nullptr;
	sceSsl.on_unload = nullptr;
	sceSsl.on_stop = nullptr;
	sceSsl.on_error = nullptr;

	REG_FUNC(0x3C733316, sceSslInit);
	REG_FUNC(0x03CE6E3A, sceSslTerm);
	REG_FUNC(0xBD203262, sceSslGetMemoryPoolStats);
	REG_FUNC(0x901C5C15, sceSslGetSerialNumber);
	REG_FUNC(0x9B2F1BC1, sceSslGetSubjectName);
	REG_FUNC(0x412711E5, sceSslGetIssuerName);
	REG_FUNC(0x70DEA174, sceSslGetNotBefore);
	REG_FUNC(0xF5ED7B68, sceSslGetNotAfter);
	REG_FUNC(0x95E14CA6, sceSslGetNameEntryCount);
	REG_FUNC(0x2A857867, sceSslGetNameEntryInfo);
	REG_FUNC(0xC73687E4, sceSslFreeSslCertName);
});
