#pragma once

#include "sceSsl.h"

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

using SceHttpAuthInfoCallback = s32(s32 request, SceHttpAuthType authType, vm::cptr<char> realm, vm::ptr<char> username, vm::ptr<char> password, s32 needEntity, vm::pptr<u8> entityBody, vm::ptr<u32> entitySize, vm::ptr<s32> save, vm::ptr<void> userArg);
using SceHttpRedirectCallback = s32(s32 request, s32 statusCode, vm::ptr<s32> method, vm::cptr<char> location, vm::ptr<void> userArg);

struct SceHttpMemoryPoolStats
{
	le_t<u32> poolSize;
	le_t<u32> maxInuseSize;
	le_t<u32> currentInuseSize;
	le_t<s32> reserved;
};

struct SceHttpUriElement
{
	le_t<s32> opaque;
	vm::lptr<char> scheme;
	vm::lptr<char> username;
	vm::lptr<char> password;
	vm::lptr<char> hostname;
	vm::lptr<char> path;
	vm::lptr<char> query;
	vm::lptr<char> fragment;
	le_t<u16> port;
	u8 reserved[10];
};

using SceHttpCookieRecvCallback = s32(s32 request, vm::cptr<char> url, vm::cptr<char> cookieHeader, u32 headerLen, vm::ptr<void> userArg);
using SceHttpCookieSendCallback = s32(s32 request, vm::cptr<char> url, vm::cptr<char> cookieHeader, vm::ptr<void> userArg);

struct SceHttpsData
{
	vm::lptr<char> ptr;
	le_t<u32> size;
};

struct SceHttpsCaList
{
	vm::lpptr<SceSslCert> caCerts;
	le_t<s32> caNum;
};

using SceHttpsCallback = s32(u32 verifyEsrr, vm::cptr<vm::ptr<SceSslCert>> sslCert, s32 certNum, vm::ptr<void> userArg);

extern psv_log_base sceHttp;
