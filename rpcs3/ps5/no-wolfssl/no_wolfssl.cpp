// SPDX-License-Identifier: MIT
// PS5_RPCS3: a build without RPCN (PS5_RPCN off, 3rdparty/CMakeLists.txt): what
// RPCN's client calls of wolfSSL (include/wolfssl/ssl.h), which has no TLS here.
// wolfSSL_Init fails, so RPCN stops at its wolfSSL error before it connects; the
// rest is never reached, and fails if it is. The clans client's base64 encoder is
// a real one.

#include "wolfssl/ssl.h"
#include "wolfssl/wolfcrypt/coding.h"

#include <cstring>

extern "C"
{
	int wolfSSL_Init(void) { return -1; }
	int wolfSSL_Cleanup(void) { return WOLFSSL_SUCCESS; }
	WOLFSSL_METHOD* wolfTLSv1_2_client_method(void) { return nullptr; }
	WOLFSSL_CTX* wolfSSL_CTX_new(WOLFSSL_METHOD*) { return nullptr; }
	void wolfSSL_CTX_free(WOLFSSL_CTX*) {}
	void wolfSSL_CTX_set_verify(WOLFSSL_CTX*, int, VerifyCallback) {}
	WOLFSSL* wolfSSL_new(WOLFSSL_CTX*) { return nullptr; }
	void wolfSSL_free(WOLFSSL*) {}
	WOLFSSL* wolfSSL_write_dup(WOLFSSL*) { return nullptr; }
	int wolfSSL_set_fd(WOLFSSL*, int) { return -1; }
	void wolfSSL_set_using_nonblock(WOLFSSL*, int) {}
	int wolfSSL_connect(WOLFSSL*) { return -1; }
	int wolfSSL_read(WOLFSSL*, void*, int) { return -1; }
	int wolfSSL_write(WOLFSSL*, const void*, int) { return -1; }
	int wolfSSL_want_read(WOLFSSL*) { return 0; }
	int wolfSSL_want_write(WOLFSSL*) { return 0; }
	int wolfSSL_get_error(WOLFSSL*, int ret) { return ret; }

	char* wolfSSL_ERR_error_string(unsigned long, char* buffer)
	{
		static char text[] = "RPCN is not in this build";
		if (buffer)
		{
			std::strncpy(buffer, text, WOLFSSL_MAX_ERROR_SZ - 1);
			buffer[WOLFSSL_MAX_ERROR_SZ - 1] = 0;
			return buffer;
		}
		return text;
	}

	int Base64_Encode_NoNl(const byte* in, word32 in_size, byte* out, word32* out_size)
	{
		static const char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		const word32 needed = (in_size + 2) / 3 * 4;
		if (!out_size || *out_size < needed || (in_size && (!in || !out)))
		{
			return -1;
		}
		word32 o = 0;
		for (word32 i = 0; i < in_size; i += 3)
		{
			const word32 left = in_size - i;
			const word32 v = (word32{in[i]} << 16) | (left > 1 ? word32{in[i + 1]} << 8 : 0) | (left > 2 ? word32{in[i + 2]} : 0);
			out[o++] = digits[(v >> 18) & 63];
			out[o++] = digits[(v >> 12) & 63];
			out[o++] = left > 1 ? digits[(v >> 6) & 63] : '=';
			out[o++] = left > 2 ? digits[v & 63] : '=';
		}
		*out_size = o;
		return 0;
	}
}
