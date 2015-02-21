#include "stdafx.h"
#if 0

void cellSsl_init();
Module cellSsl(0x0003, cellSsl_init);

int cellSslInit()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslEnd()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertificateLoader()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetSerialNumber()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetPublicKey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetRsaPublicKeyModulus()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetRsaPublicKeyExponent()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetNotBefore()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetNotAfter()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetSubjectName()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetIssuerName()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetNameEntryCount()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetNameEntryInfo()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

int cellSslCertGetMd5Fingerprint()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

void cellSsl_init()
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
}
#endif
