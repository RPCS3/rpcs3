#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceSsl.h"

s32 sceSslInit(u32 poolSize)
{
	throw __FUNCTION__;
}

s32 sceSslTerm()
{
	throw __FUNCTION__;
}

s32 sceSslGetMemoryPoolStats(vm::psv::ptr<SceSslMemoryPoolStats> currentStat)
{
	throw __FUNCTION__;
}

s32 sceSslGetSerialNumber(vm::psv::ptr<SceSslCert> sslCert, vm::psv::ptr<vm::psv::ptr<const u8>> sboData, vm::psv::ptr<u32> sboLen)
{
	throw __FUNCTION__;
}

vm::psv::ptr<SceSslCertName> sceSslGetSubjectName(vm::psv::ptr<SceSslCert> sslCert)
{
	throw __FUNCTION__;
}

vm::psv::ptr<SceSslCertName> sceSslGetIssuerName(vm::psv::ptr<SceSslCert> sslCert)
{
	throw __FUNCTION__;
}

s32 sceSslGetNotBefore(vm::psv::ptr<SceSslCert> sslCert, vm::psv::ptr<u64> begin)
{
	throw __FUNCTION__;
}

s32 sceSslGetNotAfter(vm::psv::ptr<SceSslCert> sslCert, vm::psv::ptr<u64> limit)
{
	throw __FUNCTION__;
}

s32 sceSslGetNameEntryCount(vm::psv::ptr<SceSslCertName> certName)
{
	throw __FUNCTION__;
}

s32 sceSslGetNameEntryInfo(vm::psv::ptr<SceSslCertName> certName, s32 entryNum, vm::psv::ptr<char> oidname, u32 maxOidnameLen, vm::psv::ptr<u8> value, u32 maxValueLen, vm::psv::ptr<u32> valueLen)
{
	throw __FUNCTION__;
}

s32 sceSslFreeSslCertName(vm::psv::ptr<SceSslCertName> certName)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceSsl, #name, name)

psv_log_base sceSsl("SceSsl", []()
{
	sceSsl.on_load = nullptr;
	sceSsl.on_unload = nullptr;
	sceSsl.on_stop = nullptr;

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
