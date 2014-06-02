#include "stdafx.h"
#if 0
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void sys_http_init();
Module sys_http(0x0001, sys_http_init);

int cellHttpInit()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpEnd()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpsInit()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpsEnd()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpSetProxy()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpGetProxy()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpInitCookie()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpEndCookie()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpAddCookieWithClientId()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpSessionCookieFlush()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpCookieExportWithClientId()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpCookieImportWithClientId()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetCookieSendCallback()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetCookieRecvCallback()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpCreateClient()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpDestroyClient()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetAuthenticationCallback()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetTransactionStateCallback()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetRedirectCallback()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetProxy()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetProxy()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetVersion()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetVersion()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetPipeline()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetPipeline()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetKeepAlive()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetKeepAlive()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetAutoRedirect()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetAutoRedirect()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetAutoAuthentication()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetAutoAuthentication()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetAuthenticationCacheStatus()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetAuthenticationCacheStatus()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetCookieStatus()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetCookieStatus()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetUserAgent()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetUserAgent()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetResponseBufferMax()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetResponseBufferMax()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientCloseAllConnections()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientCloseConnections()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientPollConnections()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetRecvTimeout()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetRecvTimeout()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetSendTimeout()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetSendTimeout()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetConnTimeout()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetConnTimeout()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetTotalPoolSize()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetTotalPoolSize()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetPerHostPoolSize()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetPerHostPoolSize()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetPerHostKeepAliveMax()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetPerHostKeepAliveMax()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetPerPipelineMax()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetPerPipelineMax()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetRecvBufferSize()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetRecvBufferSize()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetAllHeaders()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientAddHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientDeleteHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetSslCallback()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetSslClientCertificate()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpCreateTransaction()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpDestroyTransaction()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionGetUri()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionCloseConnection()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionReleaseConnection()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionAbortConnection()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpSendRequest()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestSetContentLength()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestGetContentLength()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestSetChunkedTransferStatus()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestGetChunkedTransferStatus()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestGetAllHeaders()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestSetHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestGetHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestAddHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRequestDeleteHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpRecvResponse()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpResponseGetAllHeaders()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpResponseGetHeader()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpResponseGetContentLength()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpResponseGetStatusCode()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpResponseGetStatusLine()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionGetSslCipherName()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionGetSslCipherId()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionGetSslCipherVersion()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionGetSslCipherBits()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionGetSslCipherString()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionGetSslVersion()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpTransactionGetSslId()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetSslVersion()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientGetSslVersion()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

int cellHttpClientSetSslIdDestroyCallback()
{
	UNIMPLEMENTED_FUNC(sys_http);
	return CELL_OK;
}

void sys_http_init()
{
	// (TODO: Find addresses for cellHttpClientSetSendBufferSize and cellHttpClientGetSendBufferSize)

	sys_http.AddFunc(0x250c386c, cellHttpInit);
	sys_http.AddFunc(0xd276ff1f, cellHttpEnd);
	sys_http.AddFunc(0x522180bc, cellHttpsInit);
	sys_http.AddFunc(0xe6d4202f, cellHttpsEnd);
	sys_http.AddFunc(0x0d896b97, cellHttpSetProxy);
	sys_http.AddFunc(0x2a87603a, cellHttpGetProxy);

	sys_http.AddFunc(0x9638f766, cellHttpInitCookie);
	sys_http.AddFunc(0x61b2bade, cellHttpEndCookie);
	sys_http.AddFunc(0x1b5bdcc6, cellHttpAddCookieWithClientId);
	sys_http.AddFunc(0xad6a2e5b, cellHttpSessionCookieFlush);
	sys_http.AddFunc(0xf972c733, cellHttpCookieExportWithClientId);
	sys_http.AddFunc(0x0d846d63, cellHttpCookieImportWithClientId);
	sys_http.AddFunc(0x4d915204, cellHttpClientSetCookieSendCallback);
	sys_http.AddFunc(0x13fe767b, cellHttpClientSetCookieRecvCallback);

	sys_http.AddFunc(0x4e4ee53a, cellHttpCreateClient);
	sys_http.AddFunc(0x980855ac, cellHttpDestroyClient);
	sys_http.AddFunc(0x660d42a9, cellHttpClientSetAuthenticationCallback);
	sys_http.AddFunc(0xb6feb84b, cellHttpClientSetTransactionStateCallback);
	sys_http.AddFunc(0x473cd9f1, cellHttpClientSetRedirectCallback);

	sys_http.AddFunc(0xd7d3cd5d, cellHttpClientSetProxy);
	sys_http.AddFunc(0x4d40cf98, cellHttpClientGetProxy);
	sys_http.AddFunc(0x40547d8b, cellHttpClientSetVersion);
	sys_http.AddFunc(0xdc405507, cellHttpClientGetVersion);
	sys_http.AddFunc(0x296a46cf, cellHttpClientSetPipeline);
	sys_http.AddFunc(0x2a1f28f6, cellHttpClientGetPipeline);
	sys_http.AddFunc(0x5d473170, cellHttpClientSetKeepAlive);
	sys_http.AddFunc(0x591c21a8, cellHttpClientGetKeepAlive);
	sys_http.AddFunc(0x211d8ba3, cellHttpClientSetAutoRedirect);
	sys_http.AddFunc(0x2960e309, cellHttpClientGetAutoRedirect);
	sys_http.AddFunc(0x8eaf47a3, cellHttpClientSetAutoAuthentication);
	sys_http.AddFunc(0x5980a293, cellHttpClientGetAutoAuthentication);
	sys_http.AddFunc(0x6eed4999, cellHttpClientSetAuthenticationCacheStatus);
	sys_http.AddFunc(0xfce39343, cellHttpClientGetAuthenticationCacheStatus);
	sys_http.AddFunc(0x434419c8, cellHttpClientSetCookieStatus);
	sys_http.AddFunc(0xeb9c1e5e, cellHttpClientGetCookieStatus);
	sys_http.AddFunc(0xcac9fc34, cellHttpClientSetUserAgent);
	sys_http.AddFunc(0xee05b0c1, cellHttpClientGetUserAgent);
	sys_http.AddFunc(0xadd66b5c, cellHttpClientSetResponseBufferMax);
	sys_http.AddFunc(0x6884cdb7, cellHttpClientGetResponseBufferMax);

	sys_http.AddFunc(0x2033b878, cellHttpClientCloseAllConnections);
	sys_http.AddFunc(0x27f86d70, cellHttpClientCloseConnections);
	sys_http.AddFunc(0xadc0a4b2, cellHttpClientPollConnections);
	sys_http.AddFunc(0x224e1610, cellHttpClientSetRecvTimeout);
	sys_http.AddFunc(0xba78e51f, cellHttpClientGetRecvTimeout);
	sys_http.AddFunc(0x71714cdc, cellHttpClientSetSendTimeout);
	sys_http.AddFunc(0x271a0b06, cellHttpClientGetSendTimeout);
	sys_http.AddFunc(0xd7471088, cellHttpClientSetConnTimeout);
	sys_http.AddFunc(0x14bfc765, cellHttpClientGetConnTimeout);
	sys_http.AddFunc(0x8aa5fcd3, cellHttpClientSetTotalPoolSize);
	sys_http.AddFunc(0x070f1020, cellHttpClientGetTotalPoolSize);
	sys_http.AddFunc(0xab1c55ab, cellHttpClientSetPerHostPoolSize);
	sys_http.AddFunc(0xffc74003, cellHttpClientGetPerHostPoolSize);
	sys_http.AddFunc(0x595adee9, cellHttpClientSetPerHostKeepAliveMax);
	sys_http.AddFunc(0x46bcc9ff, cellHttpClientGetPerHostKeepAliveMax);
	sys_http.AddFunc(0xdc7ed599, cellHttpClientSetPerPipelineMax);
	sys_http.AddFunc(0xd06c90a4, cellHttpClientGetPerPipelineMax);
	sys_http.AddFunc(0xbf6e3659, cellHttpClientSetRecvBufferSize);
	sys_http.AddFunc(0x130150ea, cellHttpClientGetRecvBufferSize);
	//sys_http.AddFunc(, cellHttpClientSetSendBufferSize);
	//sys_http.AddFunc(, cellHttpClientGetSendBufferSize);

	sys_http.AddFunc(0x0d9c65be, cellHttpClientGetAllHeaders);
	sys_http.AddFunc(0xa34c4b6f, cellHttpClientSetHeader);
	sys_http.AddFunc(0xd1ec0b25, cellHttpClientGetHeader);
	sys_http.AddFunc(0x4b33942a, cellHttpClientAddHeader);
	sys_http.AddFunc(0x617eec02, cellHttpClientDeleteHeader);

	sys_http.AddFunc(0x1395d8d1, cellHttpClientSetSslCallback);
	sys_http.AddFunc(0xd8352a40, cellHttpClientSetSslClientCertificate);

	sys_http.AddFunc(0x052a80d9, cellHttpCreateTransaction);
	sys_http.AddFunc(0x32f5cae2, cellHttpDestroyTransaction);
	sys_http.AddFunc(0x0ef17399, cellHttpTransactionGetUri);
	sys_http.AddFunc(0xa0d9223c, cellHttpTransactionCloseConnection);
	sys_http.AddFunc(0xd47cc666, cellHttpTransactionReleaseConnection);
	sys_http.AddFunc(0x2d52848b, cellHttpTransactionAbortConnection);

	sys_http.AddFunc(0xa755b005, cellHttpSendRequest);
	sys_http.AddFunc(0xaf73a64e, cellHttpRequestSetContentLength);
	sys_http.AddFunc(0x958323cf, cellHttpRequestGetContentLength);
	sys_http.AddFunc(0x8e3f7ee1, cellHttpRequestSetChunkedTransferStatus);
	sys_http.AddFunc(0x4137a1f6, cellHttpRequestGetChunkedTransferStatus);
	sys_http.AddFunc(0x42205fe0, cellHttpRequestGetAllHeaders);
	sys_http.AddFunc(0x54f2a4de, cellHttpRequestSetHeader);
	sys_http.AddFunc(0x0b9fea5f, cellHttpRequestGetHeader);
	sys_http.AddFunc(0xed993147, cellHttpRequestAddHeader);
	sys_http.AddFunc(0x16214411, cellHttpRequestDeleteHeader);

	sys_http.AddFunc(0x61c90691, cellHttpRecvResponse);
	sys_http.AddFunc(0xbea17389, cellHttpResponseGetAllHeaders);
	sys_http.AddFunc(0x4f5d8d20, cellHttpResponseGetHeader);
	sys_http.AddFunc(0x464ff889, cellHttpResponseGetContentLength);
	sys_http.AddFunc(0x10d0d7fc, cellHttpResponseGetStatusCode);
	sys_http.AddFunc(0x6a81b5e4, cellHttpResponseGetStatusLine);

	sys_http.AddFunc(0x895c604c, cellHttpTransactionGetSslCipherName);
	sys_http.AddFunc(0x34061e49, cellHttpTransactionGetSslCipherId);
	sys_http.AddFunc(0x93e938e5, cellHttpTransactionGetSslCipherVersion);
	sys_http.AddFunc(0x38954133, cellHttpTransactionGetSslCipherBits);
	sys_http.AddFunc(0xe3c424b3, cellHttpTransactionGetSslCipherString);
	sys_http.AddFunc(0xad1c6f02, cellHttpTransactionGetSslVersion);
	sys_http.AddFunc(0x2a78ff04, cellHttpTransactionGetSslId);

	sys_http.AddFunc(0x65691795, cellHttpClientSetSslVersion);
	sys_http.AddFunc(0xccf57336, cellHttpClientGetSslVersion);
	sys_http.AddFunc(0x7313c78d, cellHttpClientSetSslIdDestroyCallback);
}
#endif
