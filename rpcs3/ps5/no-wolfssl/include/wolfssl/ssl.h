// SPDX-License-Identifier: MIT
// PS5_RPCS3: a build without RPCN (PS5_RPCN off, 3rdparty/CMakeLists.txt). wolfSSL,
// RPCN's TLS, is GPL-3.0-or-later, and a title with it in cannot be shared.
// These are the parts of its interface RPCN's client calls (Emu/NP/rpcn_client.*),
// declared here and implemented in ../../no_wolfssl.cpp: wolfSSL_Init fails, and
// RPCN reports its wolfSSL error and stays offline.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WOLFSSL WOLFSSL;
typedef struct WOLFSSL_CTX WOLFSSL_CTX;
typedef struct WOLFSSL_METHOD WOLFSSL_METHOD;
typedef struct WOLFSSL_X509_STORE_CTX WOLFSSL_X509_STORE_CTX;
typedef int (*VerifyCallback)(int, WOLFSSL_X509_STORE_CTX*);

#define WOLFSSL_SUCCESS 1
#define SSL_SUCCESS WOLFSSL_SUCCESS
#define SSL_VERIFY_NONE 0
#define WOLFSSL_MAX_ERROR_SZ 80

int wolfSSL_Init(void);
int wolfSSL_Cleanup(void);
WOLFSSL_METHOD* wolfTLSv1_2_client_method(void);
WOLFSSL_CTX* wolfSSL_CTX_new(WOLFSSL_METHOD* method);
void wolfSSL_CTX_free(WOLFSSL_CTX* ctx);
void wolfSSL_CTX_set_verify(WOLFSSL_CTX* ctx, int mode, VerifyCallback callback);
WOLFSSL* wolfSSL_new(WOLFSSL_CTX* ctx);
void wolfSSL_free(WOLFSSL* ssl);
WOLFSSL* wolfSSL_write_dup(WOLFSSL* ssl);
int wolfSSL_set_fd(WOLFSSL* ssl, int fd);
void wolfSSL_set_using_nonblock(WOLFSSL* ssl, int nonblock);
int wolfSSL_connect(WOLFSSL* ssl);
int wolfSSL_read(WOLFSSL* ssl, void* data, int size);
int wolfSSL_write(WOLFSSL* ssl, const void* data, int size);
int wolfSSL_want_read(WOLFSSL* ssl);
int wolfSSL_want_write(WOLFSSL* ssl);
int wolfSSL_get_error(WOLFSSL* ssl, int ret);
char* wolfSSL_ERR_error_string(unsigned long error, char* buffer);

#ifdef __cplusplus
}
#endif
