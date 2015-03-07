#include "stdafx.h"
#if 0

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

	REG_FUNC(sys_http, cellHttpInit);
	REG_FUNC(sys_http, cellHttpEnd);
	REG_FUNC(sys_http, cellHttpsInit);
	REG_FUNC(sys_http, cellHttpsEnd);
	REG_FUNC(sys_http, cellHttpSetProxy);
	REG_FUNC(sys_http, cellHttpGetProxy);

	REG_FUNC(sys_http, cellHttpInitCookie);
	REG_FUNC(sys_http, cellHttpEndCookie);
	REG_FUNC(sys_http, cellHttpAddCookieWithClientId);
	REG_FUNC(sys_http, cellHttpSessionCookieFlush);
	REG_FUNC(sys_http, cellHttpCookieExportWithClientId);
	REG_FUNC(sys_http, cellHttpCookieImportWithClientId);
	REG_FUNC(sys_http, cellHttpClientSetCookieSendCallback);
	REG_FUNC(sys_http, cellHttpClientSetCookieRecvCallback);

	REG_FUNC(sys_http, cellHttpCreateClient);
	REG_FUNC(sys_http, cellHttpDestroyClient);
	REG_FUNC(sys_http, cellHttpClientSetAuthenticationCallback);
	REG_FUNC(sys_http, cellHttpClientSetTransactionStateCallback);
	REG_FUNC(sys_http, cellHttpClientSetRedirectCallback);

	REG_FUNC(sys_http, cellHttpClientSetProxy);
	REG_FUNC(sys_http, cellHttpClientGetProxy);
	REG_FUNC(sys_http, cellHttpClientSetVersion);
	REG_FUNC(sys_http, cellHttpClientGetVersion);
	REG_FUNC(sys_http, cellHttpClientSetPipeline);
	REG_FUNC(sys_http, cellHttpClientGetPipeline);
	REG_FUNC(sys_http, cellHttpClientSetKeepAlive);
	REG_FUNC(sys_http, cellHttpClientGetKeepAlive);
	REG_FUNC(sys_http, cellHttpClientSetAutoRedirect);
	REG_FUNC(sys_http, cellHttpClientGetAutoRedirect);
	REG_FUNC(sys_http, cellHttpClientSetAutoAuthentication);
	REG_FUNC(sys_http, cellHttpClientGetAutoAuthentication);
	REG_FUNC(sys_http, cellHttpClientSetAuthenticationCacheStatus);
	REG_FUNC(sys_http, cellHttpClientGetAuthenticationCacheStatus);
	REG_FUNC(sys_http, cellHttpClientSetCookieStatus);
	REG_FUNC(sys_http, cellHttpClientGetCookieStatus);
	REG_FUNC(sys_http, cellHttpClientSetUserAgent);
	REG_FUNC(sys_http, cellHttpClientGetUserAgent);
	REG_FUNC(sys_http, cellHttpClientSetResponseBufferMax);
	REG_FUNC(sys_http, cellHttpClientGetResponseBufferMax);

	REG_FUNC(sys_http, cellHttpClientCloseAllConnections);
	REG_FUNC(sys_http, cellHttpClientCloseConnections);
	REG_FUNC(sys_http, cellHttpClientPollConnections);
	REG_FUNC(sys_http, cellHttpClientSetRecvTimeout);
	REG_FUNC(sys_http, cellHttpClientGetRecvTimeout);
	REG_FUNC(sys_http, cellHttpClientSetSendTimeout);
	REG_FUNC(sys_http, cellHttpClientGetSendTimeout);
	REG_FUNC(sys_http, cellHttpClientSetConnTimeout);
	REG_FUNC(sys_http, cellHttpClientGetConnTimeout);
	REG_FUNC(sys_http, cellHttpClientSetTotalPoolSize);
	REG_FUNC(sys_http, cellHttpClientGetTotalPoolSize);
	REG_FUNC(sys_http, cellHttpClientSetPerHostPoolSize);
	REG_FUNC(sys_http, cellHttpClientGetPerHostPoolSize);
	REG_FUNC(sys_http, cellHttpClientSetPerHostKeepAliveMax);
	REG_FUNC(sys_http, cellHttpClientGetPerHostKeepAliveMax);
	REG_FUNC(sys_http, cellHttpClientSetPerPipelineMax);
	REG_FUNC(sys_http, cellHttpClientGetPerPipelineMax);
	REG_FUNC(sys_http, cellHttpClientSetRecvBufferSize);
	REG_FUNC(sys_http, cellHttpClientGetRecvBufferSize);
	//sys_http.AddFunc(, cellHttpClientSetSendBufferSize);
	//sys_http.AddFunc(, cellHttpClientGetSendBufferSize);

	REG_FUNC(sys_http, cellHttpClientGetAllHeaders);
	REG_FUNC(sys_http, cellHttpClientSetHeader);
	REG_FUNC(sys_http, cellHttpClientGetHeader);
	REG_FUNC(sys_http, cellHttpClientAddHeader);
	REG_FUNC(sys_http, cellHttpClientDeleteHeader);

	REG_FUNC(sys_http, cellHttpClientSetSslCallback);
	REG_FUNC(sys_http, cellHttpClientSetSslClientCertificate);

	REG_FUNC(sys_http, cellHttpCreateTransaction);
	REG_FUNC(sys_http, cellHttpDestroyTransaction);
	REG_FUNC(sys_http, cellHttpTransactionGetUri);
	REG_FUNC(sys_http, cellHttpTransactionCloseConnection);
	REG_FUNC(sys_http, cellHttpTransactionReleaseConnection);
	REG_FUNC(sys_http, cellHttpTransactionAbortConnection);

	REG_FUNC(sys_http, cellHttpSendRequest);
	REG_FUNC(sys_http, cellHttpRequestSetContentLength);
	REG_FUNC(sys_http, cellHttpRequestGetContentLength);
	REG_FUNC(sys_http, cellHttpRequestSetChunkedTransferStatus);
	REG_FUNC(sys_http, cellHttpRequestGetChunkedTransferStatus);
	REG_FUNC(sys_http, cellHttpRequestGetAllHeaders);
	REG_FUNC(sys_http, cellHttpRequestSetHeader);
	REG_FUNC(sys_http, cellHttpRequestGetHeader);
	REG_FUNC(sys_http, cellHttpRequestAddHeader);
	REG_FUNC(sys_http, cellHttpRequestDeleteHeader);

	REG_FUNC(sys_http, cellHttpRecvResponse);
	REG_FUNC(sys_http, cellHttpResponseGetAllHeaders);
	REG_FUNC(sys_http, cellHttpResponseGetHeader);
	REG_FUNC(sys_http, cellHttpResponseGetContentLength);
	REG_FUNC(sys_http, cellHttpResponseGetStatusCode);
	REG_FUNC(sys_http, cellHttpResponseGetStatusLine);

	REG_FUNC(sys_http, cellHttpTransactionGetSslCipherName);
	REG_FUNC(sys_http, cellHttpTransactionGetSslCipherId);
	REG_FUNC(sys_http, cellHttpTransactionGetSslCipherVersion);
	REG_FUNC(sys_http, cellHttpTransactionGetSslCipherBits);
	REG_FUNC(sys_http, cellHttpTransactionGetSslCipherString);
	REG_FUNC(sys_http, cellHttpTransactionGetSslVersion);
	REG_FUNC(sys_http, cellHttpTransactionGetSslId);

	REG_FUNC(sys_http, cellHttpClientSetSslVersion);
	REG_FUNC(sys_http, cellHttpClientGetSslVersion);
	REG_FUNC(sys_http, cellHttpClientSetSslIdDestroyCallback);
}
#endif
