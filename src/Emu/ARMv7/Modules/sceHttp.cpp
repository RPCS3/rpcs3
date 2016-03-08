#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceHttp.h"

s32 sceHttpInit(u32 poolSize)
{
	throw EXCEPTION("");
}

s32 sceHttpTerm()
{
	throw EXCEPTION("");
}

s32 sceHttpGetMemoryPoolStats(vm::ptr<SceHttpMemoryPoolStats> currentStat)
{
	throw EXCEPTION("");
}

s32 sceHttpCreateTemplate(vm::cptr<char> userAgent, s32 httpVer, s32 autoProxyConf)
{
	throw EXCEPTION("");
}

s32 sceHttpDeleteTemplate(s32 tmplId)
{
	throw EXCEPTION("");
}

s32 sceHttpCreateConnection(s32 tmplId, vm::cptr<char> serverName, vm::cptr<char> scheme, u16 port, s32 enableKeepalive)
{
	throw EXCEPTION("");
}

s32 sceHttpCreateConnectionWithURL(s32 tmplId, vm::cptr<char> url, s32 enableKeepalive)
{
	throw EXCEPTION("");
}

s32 sceHttpDeleteConnection(s32 connId)
{
	throw EXCEPTION("");
}

s32 sceHttpCreateRequest(s32 connId, s32 method, vm::cptr<char> path, u64 contentLength)
{
	throw EXCEPTION("");
}

s32 sceHttpCreateRequestWithURL(s32 connId, s32 method, vm::cptr<char> url, u64 contentLength)
{
	throw EXCEPTION("");
}

s32 sceHttpDeleteRequest(s32 reqId)
{
	throw EXCEPTION("");
}

s32 sceHttpSetResponseHeaderMaxSize(s32 id, u32 headerSize)
{
	throw EXCEPTION("");
}

s32 sceHttpSetRecvBlockSize(s32 id, u32 blockSize)
{
	throw EXCEPTION("");
}

s32 sceHttpSetRequestContentLength(s32 id, u64 contentLength)
{
	throw EXCEPTION("");
}

s32 sceHttpSendRequest(s32 reqId, vm::cptr<void> postData, u32 size)
{
	throw EXCEPTION("");
}

s32 sceHttpAbortRequest(s32 reqId)
{
	throw EXCEPTION("");
}

s32 sceHttpGetResponseContentLength(s32 reqId, vm::ptr<u64> contentLength)
{
	throw EXCEPTION("");
}

s32 sceHttpGetStatusCode(s32 reqId, vm::ptr<s32> statusCode)
{
	throw EXCEPTION("");
}

s32 sceHttpGetAllResponseHeaders(s32 reqId, vm::pptr<char> header, vm::ptr<u32> headerSize)
{
	throw EXCEPTION("");
}

s32 sceHttpReadData(s32 reqId, vm::ptr<void> data, u32 size)
{
	throw EXCEPTION("");
}

s32 sceHttpAddRequestHeader(s32 id, vm::cptr<char> name, vm::cptr<char> value, u32 mode)
{
	throw EXCEPTION("");
}

s32 sceHttpRemoveRequestHeader(s32 id, vm::cptr<char> name)
{
	throw EXCEPTION("");
}

s32 sceHttpParseResponseHeader(vm::cptr<char> header, u32 headerLen, vm::cptr<char> fieldStr, vm::cpptr<char> fieldValue, vm::ptr<u32> valueLen)
{
	throw EXCEPTION("");
}

s32 sceHttpParseStatusLine(vm::cptr<char> statusLine, u32 lineLen, vm::ptr<s32> httpMajorVer, vm::ptr<s32> httpMinorVer, vm::ptr<s32> responseCode, vm::cpptr<char> reasonPhrase, vm::ptr<u32> phraseLen)
{
	throw EXCEPTION("");
}

s32 sceHttpSetAuthInfoCallback(s32 id, vm::ptr<SceHttpAuthInfoCallback> cbfunc, vm::ptr<void> userArg)
{
	throw EXCEPTION("");
}

s32 sceHttpSetAuthEnabled(s32 id, s32 enable)
{
	throw EXCEPTION("");
}

s32 sceHttpGetAuthEnabled(s32 id, vm::ptr<s32> enable)
{
	throw EXCEPTION("");
}

s32 sceHttpSetRedirectCallback(s32 id, vm::ptr<SceHttpRedirectCallback> cbfunc, vm::ptr<void> userArg)
{
	throw EXCEPTION("");
}

s32 sceHttpSetAutoRedirect(s32 id, s32 enable)
{
	throw EXCEPTION("");
}

s32 sceHttpGetAutoRedirect(s32 id, vm::ptr<s32> enable)
{
	throw EXCEPTION("");
}

s32 sceHttpSetResolveTimeOut(s32 id, u32 usec)
{
	throw EXCEPTION("");
}

s32 sceHttpSetResolveRetry(s32 id, s32 retry)
{
	throw EXCEPTION("");
}

s32 sceHttpSetConnectTimeOut(s32 id, u32 usec)
{
	throw EXCEPTION("");
}

s32 sceHttpSetSendTimeOut(s32 id, u32 usec)
{
	throw EXCEPTION("");
}

s32 sceHttpSetRecvTimeOut(s32 id, u32 usec)
{
	throw EXCEPTION("");
}

s32 sceHttpUriEscape(vm::ptr<char> out, vm::ptr<u32> require, u32 prepare, vm::cptr<char> in)
{
	throw EXCEPTION("");
}

s32 sceHttpUriUnescape(vm::ptr<char> out, vm::ptr<u32> require, u32 prepare, vm::cptr<char> in)
{
	throw EXCEPTION("");
}

s32 sceHttpUriParse(vm::ptr<SceHttpUriElement> out, vm::cptr<char> srcUrl, vm::ptr<void> pool, vm::ptr<u32> require, u32 prepare)
{
	throw EXCEPTION("");
}

s32 sceHttpUriBuild(vm::ptr<char> out, vm::ptr<u32> require, u32 prepare, vm::cptr<SceHttpUriElement> srcElement, u32 option)
{
	throw EXCEPTION("");
}

s32 sceHttpUriMerge(vm::ptr<char> mergedUrl, vm::cptr<char> url, vm::cptr<char> relativeUrl, vm::ptr<u32> require, u32 prepare, u32 option)
{
	throw EXCEPTION("");
}

s32 sceHttpUriSweepPath(vm::ptr<char> dst, vm::cptr<char> src, u32 srcSize)
{
	throw EXCEPTION("");
}

s32 sceHttpSetCookieEnabled(s32 id, s32 enable)
{
	throw EXCEPTION("");
}

s32 sceHttpGetCookieEnabled(s32 id, vm::ptr<s32> enable)
{
	throw EXCEPTION("");
}

s32 sceHttpGetCookie(vm::cptr<char> url, vm::ptr<char> cookie, vm::ptr<u32> cookieLength, u32 prepare, s32 secure)
{
	throw EXCEPTION("");
}

s32 sceHttpAddCookie(vm::cptr<char> url, vm::cptr<char> cookie, u32 cookieLength)
{
	throw EXCEPTION("");
}

s32 sceHttpSetCookieRecvCallback(s32 id, vm::ptr<SceHttpCookieRecvCallback> cbfunc, vm::ptr<void> userArg)
{
	throw EXCEPTION("");
}

s32 sceHttpSetCookieSendCallback(s32 id, vm::ptr<SceHttpCookieSendCallback> cbfunc, vm::ptr<void> userArg)
{
	throw EXCEPTION("");
}

s32 sceHttpsLoadCert(s32 caCertNum, vm::cpptr<SceHttpsData> caList, vm::cptr<SceHttpsData> cert, vm::cptr<SceHttpsData> privKey)
{
	throw EXCEPTION("");
}

s32 sceHttpsUnloadCert()
{
	throw EXCEPTION("");
}

s32 sceHttpsEnableOption(u32 sslFlags)
{
	throw EXCEPTION("");
}

s32 sceHttpsDisableOption(u32 sslFlags)
{
	throw EXCEPTION("");
}

s32 sceHttpsGetSslError(s32 id, vm::ptr<s32> errNum, vm::ptr<u32> detail)
{
	throw EXCEPTION("");
}

s32 sceHttpsSetSslCallback(s32 id, vm::ptr<SceHttpsCallback> cbfunc, vm::ptr<void> userArg)
{
	throw EXCEPTION("");
}

s32 sceHttpsGetCaList(vm::ptr<SceHttpsCaList> caList)
{
	throw EXCEPTION("");
}

s32 sceHttpsFreeCaList(vm::ptr<SceHttpsCaList> caList)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceHttp, #name, name)

psv_log_base sceHttp("SceHttp", []()
{
	sceHttp.on_load = nullptr;
	sceHttp.on_unload = nullptr;
	sceHttp.on_stop = nullptr;
	sceHttp.on_error = nullptr;

	REG_FUNC(0x214926D9, sceHttpInit);
	REG_FUNC(0xC9076666, sceHttpTerm);
	REG_FUNC(0xF98CDFA9, sceHttpGetMemoryPoolStats);
	REG_FUNC(0x62241DAB, sceHttpCreateTemplate);
	REG_FUNC(0xEC85ECFB, sceHttpDeleteTemplate);
	REG_FUNC(0xC616C200, sceHttpCreateConnectionWithURL);
	REG_FUNC(0xAEB3307E, sceHttpCreateConnection);
	REG_FUNC(0xF0F65C15, sceHttpDeleteConnection);
	REG_FUNC(0xBD5DA1D0, sceHttpCreateRequestWithURL);
	REG_FUNC(0xB0284270, sceHttpCreateRequest);
	REG_FUNC(0x3D3D29AD, sceHttpDeleteRequest);
	REG_FUNC(0x9CA58B99, sceHttpSendRequest);
	REG_FUNC(0x7EDE3979, sceHttpReadData);
	REG_FUNC(0xF580D304, sceHttpGetResponseContentLength);
	REG_FUNC(0x27071691, sceHttpGetStatusCode);
	REG_FUNC(0xEA61662F, sceHttpAbortRequest);
	REG_FUNC(0x7B51B122, sceHttpAddRequestHeader);
	REG_FUNC(0x5EB5F548, sceHttpRemoveRequestHeader);
	REG_FUNC(0x11F6C27F, sceHttpGetAllResponseHeaders);
	REG_FUNC(0x03A6C89E, sceHttpParseResponseHeader);
	REG_FUNC(0x179C56DB, sceHttpParseStatusLine);
	REG_FUNC(0x1DA2A673, sceHttpUriEscape);
	REG_FUNC(0x1274D318, sceHttpUriUnescape);
	REG_FUNC(0x1D45F24E, sceHttpUriParse);
	REG_FUNC(0x47664424, sceHttpUriBuild);
	REG_FUNC(0x75027D1D, sceHttpUriMerge);
	REG_FUNC(0x50737A3F, sceHttpUriSweepPath);
	REG_FUNC(0x37C30C90, sceHttpSetRequestContentLength);
	REG_FUNC(0x11EC42D0, sceHttpSetAuthEnabled);
	REG_FUNC(0x6727874C, sceHttpGetAuthEnabled);
	REG_FUNC(0x34891C3F, sceHttpSetAutoRedirect);
	REG_FUNC(0x6EAD73EB, sceHttpGetAutoRedirect);
	REG_FUNC(0xE0A3A88D, sceHttpSetAuthInfoCallback);
	REG_FUNC(0x4E08167D, sceHttpSetRedirectCallback);
	REG_FUNC(0x8455B5B3, sceHttpSetResolveTimeOut);
	REG_FUNC(0x9AB56EA7, sceHttpSetResolveRetry);
	REG_FUNC(0x237CA86E, sceHttpSetConnectTimeOut);
	REG_FUNC(0x8AE3F008, sceHttpSetSendTimeOut);
	REG_FUNC(0x94BF196E, sceHttpSetRecvTimeOut);
	//REG_FUNC(0x27A98BDA, sceHttpSetNonblock);
	//REG_FUNC(0xD65746BC, sceHttpGetNonblock);
	//REG_FUNC(0x5CEB6554, sceHttpSetEpollId);
	//REG_FUNC(0x9E031D7C, sceHttpGetEpollId);
	//REG_FUNC(0x94F7256A, sceHttpWaitRequest);
	//REG_FUNC(0x7C99AF67, sceHttpCreateEpoll);
	//REG_FUNC(0x0F1FD1B3, sceHttpSetEpoll);
	//REG_FUNC(0xCFB1DA4B, sceHttpUnsetEpoll);
	//REG_FUNC(0x65FE983F, sceHttpGetEpoll);
	//REG_FUNC(0x07D9F8BB, sceHttpDestroyEpoll);
	REG_FUNC(0xAEE573A3, sceHttpSetCookieEnabled);
	REG_FUNC(0x1B6EF66E, sceHttpGetCookieEnabled);
	REG_FUNC(0x70220BFA, sceHttpGetCookie);
	REG_FUNC(0xBEDB988D, sceHttpAddCookie);
	//REG_FUNC(0x4259FB9E, sceHttpCookieExport);
	//REG_FUNC(0x9DF48282, sceHttpCookieImport);
	REG_FUNC(0xD4F32A23, sceHttpSetCookieRecvCallback);
	REG_FUNC(0x11C03867, sceHttpSetCookieSendCallback);
	REG_FUNC(0xAE8D7C33, sceHttpsLoadCert);
	REG_FUNC(0x8577833F, sceHttpsUnloadCert);
	REG_FUNC(0x9FBE2869, sceHttpsEnableOption);
	REG_FUNC(0xC6D60403, sceHttpsDisableOption);
	//REG_FUNC(0x72CB0741, sceHttpsEnableOptionPrivate);
	//REG_FUNC(0x00659635, sceHttpsDisableOptionPrivate);
	REG_FUNC(0x2B79BDE0, sceHttpsGetSslError);
	REG_FUNC(0xA0926037, sceHttpsSetSslCallback);
	REG_FUNC(0xF71AA58D, sceHttpsGetCaList);
	REG_FUNC(0x56C95D94, sceHttpsFreeCaList);
});
