#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceSsl.h"

extern psv_log_base sceHttp;

enum SceHttpHttpVersion : s32
{
	SCE_HTTP_VERSION_1_0 = 1,
	SCE_HTTP_VERSION_1_1
};

enum SceHttpProxyMode : s32
{
	SCE_HTTP_PROXY_AUTO,
	SCE_HTTP_PROXY_MANUAL
};

enum SceHttpAddHeaderMode : s32
{
	SCE_HTTP_HEADER_OVERWRITE,
	SCE_HTTP_HEADER_ADD
};

enum SceHttpAuthType : s32
{
	SCE_HTTP_AUTH_BASIC,
	SCE_HTTP_AUTH_DIGEST,
	SCE_HTTP_AUTH_RESERVED0,
	SCE_HTTP_AUTH_RESERVED1,
	SCE_HTTP_AUTH_RESERVED2
};

typedef vm::psv::ptr<s32(s32 request, SceHttpAuthType authType, vm::psv::ptr<const char> realm, vm::psv::ptr<char> username, vm::psv::ptr<char> password, s32 needEntity, vm::psv::pptr<u8> entityBody, vm::psv::ptr<u32> entitySize, vm::psv::ptr<s32> save, vm::psv::ptr<void> userArg)> SceHttpAuthInfoCallback;

typedef vm::psv::ptr<s32(s32 request, s32 statusCode, vm::psv::ptr<s32> method, vm::psv::ptr<const char> location, vm::psv::ptr<void> userArg)> SceHttpRedirectCallback;

struct SceHttpMemoryPoolStats
{
	u32 poolSize;
	u32 maxInuseSize;
	u32 currentInuseSize;
	s32 reserved;
};

struct SceHttpUriElement
{
	s32 opaque;
	vm::psv::ptr<char> scheme;
	vm::psv::ptr<char> username;
	vm::psv::ptr<char> password;
	vm::psv::ptr<char> hostname;
	vm::psv::ptr<char> path;
	vm::psv::ptr<char> query;
	vm::psv::ptr<char> fragment;
	u16 port;
	u8 reserved[10];
};

typedef vm::psv::ptr<s32(s32 request, vm::psv::ptr<const char> url, vm::psv::ptr<const char> cookieHeader, u32 headerLen, vm::psv::ptr<void> userArg)> SceHttpCookieRecvCallback;

typedef vm::psv::ptr<s32(s32 request, vm::psv::ptr<const char> url, vm::psv::ptr<const char> cookieHeader, vm::psv::ptr<void> userArg)> SceHttpCookieSendCallback;

struct SceHttpsData
{
	vm::psv::ptr<char> ptr;
	u32 size;
};

struct SceHttpsCaList
{
	vm::psv::lpptr<SceSslCert> caCerts;
	s32 caNum;
};

typedef vm::psv::ptr<s32(u32 verifyEsrr, vm::psv::ptr<const vm::psv::ptr<SceSslCert>> sslCert, s32 certNum, vm::psv::ptr<void> userArg)> SceHttpsCallback;

s32 sceHttpInit(u32 poolSize)
{
	throw __FUNCTION__;
}

s32 sceHttpTerm()
{
	throw __FUNCTION__;
}

s32 sceHttpGetMemoryPoolStats(vm::psv::ptr<SceHttpMemoryPoolStats> currentStat)
{
	throw __FUNCTION__;
}

s32 sceHttpCreateTemplate(vm::psv::ptr<const char> userAgent, s32 httpVer, s32 autoProxyConf)
{
	throw __FUNCTION__;
}

s32 sceHttpDeleteTemplate(s32 tmplId)
{
	throw __FUNCTION__;
}

s32 sceHttpCreateConnection(s32 tmplId, vm::psv::ptr<const char> serverName, vm::psv::ptr<const char> scheme, u16 port, s32 enableKeepalive)
{
	throw __FUNCTION__;
}

s32 sceHttpCreateConnectionWithURL(s32 tmplId, vm::psv::ptr<const char> url, s32 enableKeepalive)
{
	throw __FUNCTION__;
}

s32 sceHttpDeleteConnection(s32 connId)
{
	throw __FUNCTION__;
}

s32 sceHttpCreateRequest(s32 connId, s32 method, vm::psv::ptr<const char> path, u64 contentLength)
{
	throw __FUNCTION__;
}

s32 sceHttpCreateRequestWithURL(s32 connId, s32 method, vm::psv::ptr<const char> url, u64 contentLength)
{
	throw __FUNCTION__;
}

s32 sceHttpDeleteRequest(s32 reqId)
{
	throw __FUNCTION__;
}

s32 sceHttpSetResponseHeaderMaxSize(s32 id, u32 headerSize)
{
	throw __FUNCTION__;
}

s32 sceHttpSetRecvBlockSize(s32 id, u32 blockSize)
{
	throw __FUNCTION__;
}

s32 sceHttpSetRequestContentLength(s32 id, u64 contentLength)
{
	throw __FUNCTION__;
}

s32 sceHttpSendRequest(s32 reqId, vm::psv::ptr<const void> postData, u32 size)
{
	throw __FUNCTION__;
}

s32 sceHttpAbortRequest(s32 reqId)
{
	throw __FUNCTION__;
}

s32 sceHttpGetResponseContentLength(s32 reqId, vm::psv::ptr<u64> contentLength)
{
	throw __FUNCTION__;
}

s32 sceHttpGetStatusCode(s32 reqId, vm::psv::ptr<s32> statusCode)
{
	throw __FUNCTION__;
}

s32 sceHttpGetAllResponseHeaders(s32 reqId, vm::psv::pptr<char> header, vm::psv::ptr<u32> headerSize)
{
	throw __FUNCTION__;
}

s32 sceHttpReadData(s32 reqId, vm::psv::ptr<void> data, u32 size)
{
	throw __FUNCTION__;
}

s32 sceHttpAddRequestHeader(s32 id, vm::psv::ptr<const char> name, vm::psv::ptr<const char> value, u32 mode)
{
	throw __FUNCTION__;
}

s32 sceHttpRemoveRequestHeader(s32 id, vm::psv::ptr<const char> name)
{
	throw __FUNCTION__;
}

s32 sceHttpParseResponseHeader(vm::psv::ptr<const char> header, u32 headerLen, vm::psv::ptr<const char> fieldStr, vm::psv::pptr<const char> fieldValue, vm::psv::ptr<u32> valueLen)
{
	throw __FUNCTION__;
}

s32 sceHttpParseStatusLine(vm::psv::ptr<const char> statusLine, u32 lineLen, vm::psv::ptr<s32> httpMajorVer, vm::psv::ptr<s32> httpMinorVer, vm::psv::ptr<s32> responseCode, vm::psv::pptr<const char> reasonPhrase, vm::psv::ptr<u32> phraseLen)
{
	throw __FUNCTION__;
}

s32 sceHttpSetAuthInfoCallback(s32 id, SceHttpAuthInfoCallback cbfunc, vm::psv::ptr<void> userArg)
{
	throw __FUNCTION__;
}

s32 sceHttpSetAuthEnabled(s32 id, s32 enable)
{
	throw __FUNCTION__;
}

s32 sceHttpGetAuthEnabled(s32 id, vm::psv::ptr<s32> enable)
{
	throw __FUNCTION__;
}

s32 sceHttpSetRedirectCallback(s32 id, SceHttpRedirectCallback cbfunc, vm::psv::ptr<void> userArg)
{
	throw __FUNCTION__;
}

s32 sceHttpSetAutoRedirect(s32 id, s32 enable)
{
	throw __FUNCTION__;
}

s32 sceHttpGetAutoRedirect(s32 id, vm::psv::ptr<s32> enable)
{
	throw __FUNCTION__;
}

s32 sceHttpSetResolveTimeOut(s32 id, u32 usec)
{
	throw __FUNCTION__;
}

s32 sceHttpSetResolveRetry(s32 id, s32 retry)
{
	throw __FUNCTION__;
}

s32 sceHttpSetConnectTimeOut(s32 id, u32 usec)
{
	throw __FUNCTION__;
}

s32 sceHttpSetSendTimeOut(s32 id, u32 usec)
{
	throw __FUNCTION__;
}

s32 sceHttpSetRecvTimeOut(s32 id, u32 usec)
{
	throw __FUNCTION__;
}

s32 sceHttpUriEscape(vm::psv::ptr<char> out, vm::psv::ptr<u32> require, u32 prepare, vm::psv::ptr<const char> in)
{
	throw __FUNCTION__;
}

s32 sceHttpUriUnescape(vm::psv::ptr<char> out, vm::psv::ptr<u32> require, u32 prepare, vm::psv::ptr<const char> in)
{
	throw __FUNCTION__;
}

s32 sceHttpUriParse(vm::psv::ptr<SceHttpUriElement> out, vm::psv::ptr<const char> srcUrl, vm::psv::ptr<void> pool, vm::psv::ptr<u32> require, u32 prepare)
{
	throw __FUNCTION__;
}

s32 sceHttpUriBuild(vm::psv::ptr<char> out, vm::psv::ptr<u32> require, u32 prepare, vm::psv::ptr<const SceHttpUriElement> srcElement, u32 option)
{
	throw __FUNCTION__;
}

s32 sceHttpUriMerge(vm::psv::ptr<char> mergedUrl, vm::psv::ptr<const char> url, vm::psv::ptr<const char> relativeUrl, vm::psv::ptr<u32> require, u32 prepare, u32 option)
{
	throw __FUNCTION__;
}

s32 sceHttpUriSweepPath(vm::psv::ptr<char> dst, vm::psv::ptr<const char> src, u32 srcSize)
{
	throw __FUNCTION__;
}

s32 sceHttpSetCookieEnabled(s32 id, s32 enable)
{
	throw __FUNCTION__;
}

s32 sceHttpGetCookieEnabled(s32 id, vm::psv::ptr<s32> enable)
{
	throw __FUNCTION__;
}

s32 sceHttpGetCookie(vm::psv::ptr<const char> url, vm::psv::ptr<char> cookie, vm::psv::ptr<u32> cookieLength, u32 prepare, s32 secure)
{
	throw __FUNCTION__;
}

s32 sceHttpAddCookie(vm::psv::ptr<const char> url, vm::psv::ptr<const char> cookie, u32 cookieLength)
{
	throw __FUNCTION__;
}

s32 sceHttpSetCookieRecvCallback(s32 id, SceHttpCookieRecvCallback cbfunc, vm::psv::ptr<void> userArg)
{
	throw __FUNCTION__;
}

s32 sceHttpSetCookieSendCallback(s32 id, SceHttpCookieSendCallback cbfunc, vm::psv::ptr<void> userArg)
{
	throw __FUNCTION__;
}

s32 sceHttpsLoadCert(s32 caCertNum, vm::psv::pptr<const SceHttpsData> caList, vm::psv::ptr<const SceHttpsData> cert, vm::psv::ptr<const SceHttpsData> privKey)
{
	throw __FUNCTION__;
}

s32 sceHttpsUnloadCert()
{
	throw __FUNCTION__;
}

s32 sceHttpsEnableOption(u32 sslFlags)
{
	throw __FUNCTION__;
}

s32 sceHttpsDisableOption(u32 sslFlags)
{
	throw __FUNCTION__;
}

s32 sceHttpsGetSslError(s32 id, vm::psv::ptr<s32> errNum, vm::psv::ptr<u32> detail)
{
	throw __FUNCTION__;
}

s32 sceHttpsSetSslCallback(s32 id, SceHttpsCallback cbfunc, vm::psv::ptr<void> userArg)
{
	throw __FUNCTION__;
}

s32 sceHttpsGetCaList(vm::psv::ptr<SceHttpsCaList> caList)
{
	throw __FUNCTION__;
}

s32 sceHttpsFreeCaList(vm::psv::ptr<SceHttpsCaList> caList)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceHttp, #name, name)

psv_log_base sceHttp("SceHttp", []()
{
	sceHttp.on_load = nullptr;
	sceHttp.on_unload = nullptr;
	sceHttp.on_stop = nullptr;

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
