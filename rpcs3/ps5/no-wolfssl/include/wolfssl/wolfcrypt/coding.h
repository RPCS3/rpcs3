// SPDX-License-Identifier: MIT
// PS5_RPCS3: a build without RPCN (wolfssl/ssl.h here): the base64 encoder RPCN's
// clans client calls (Emu/NP/clans_client.cpp), implemented in ../../../no_wolfssl.cpp.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;
typedef unsigned int word32;

// Base64 of in[0..in_size) to out, without line breaks or a terminating NUL;
// *out_size is out's room, then what was written. 0, or -1 if out is too small
int Base64_Encode_NoNl(const byte* in, word32 in_size, byte* out, word32* out_size);

#ifdef __cplusplus
}
#endif
