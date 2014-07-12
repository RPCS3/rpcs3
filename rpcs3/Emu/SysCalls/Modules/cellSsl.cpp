#include "stdafx.h"
#if 0
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellSsl.AddFunc(0xfb02c9d2, cellSslInit);
	cellSsl.AddFunc(0x1650aea4, cellSslEnd);

	cellSsl.AddFunc(0x571afaca, cellSslCertificateLoader);

	cellSsl.AddFunc(0x7b689ebc, cellSslCertGetSerialNumber);
	cellSsl.AddFunc(0xf8206492, cellSslCertGetPublicKey);
	cellSsl.AddFunc(0x8e505175, cellSslCertGetRsaPublicKeyModulus);
	cellSsl.AddFunc(0x033c4905, cellSslCertGetRsaPublicKeyExponent);
	cellSsl.AddFunc(0x31d9ba8d, cellSslCertGetNotBefore);
	cellSsl.AddFunc(0x218b64da, cellSslCertGetNotAfter);
	cellSsl.AddFunc(0x32c61bdf, cellSslCertGetSubjectName);
	cellSsl.AddFunc(0xae6eb491, cellSslCertGetIssuerName);
	cellSsl.AddFunc(0x766d3ca1, cellSslCertGetNameEntryCount);
	cellSsl.AddFunc(0x006c4900, cellSslCertGetNameEntryInfo);
	cellSsl.AddFunc(0x5e9253ca, cellSslCertGetMd5Fingerprint);
}
#endif
