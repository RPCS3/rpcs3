#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSsl;

s32 cellSslInit()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslEnd()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertificateLoader()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetSerialNumber()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetPublicKey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetRsaPublicKeyModulus()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetRsaPublicKeyExponent()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetNotBefore()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetNotAfter()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetSubjectName()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetIssuerName()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetNameEntryCount()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetNameEntryInfo()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetMd5Fingerprint()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

Module cellSsl("cellSsl", []()
{
	REG_FUNC(cellSsl, cellSslInit);
	REG_FUNC(cellSsl, cellSslEnd);

	REG_FUNC(cellSsl, cellSslCertificateLoader);

	REG_FUNC(cellSsl, cellSslCertGetSerialNumber);
	REG_FUNC(cellSsl, cellSslCertGetPublicKey);
	REG_FUNC(cellSsl, cellSslCertGetRsaPublicKeyModulus);
	REG_FUNC(cellSsl, cellSslCertGetRsaPublicKeyExponent);
	REG_FUNC(cellSsl, cellSslCertGetNotBefore);
	REG_FUNC(cellSsl, cellSslCertGetNotAfter);
	REG_FUNC(cellSsl, cellSslCertGetSubjectName);
	REG_FUNC(cellSsl, cellSslCertGetIssuerName);
	REG_FUNC(cellSsl, cellSslCertGetNameEntryCount);
	REG_FUNC(cellSsl, cellSslCertGetNameEntryInfo);
	REG_FUNC(cellSsl, cellSslCertGetMd5Fingerprint);
});
