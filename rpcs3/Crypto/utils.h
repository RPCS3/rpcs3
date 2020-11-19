#pragma once

// Copyright (C) 2014       Hykem <hykem@hotmail.com>
// Licensed under the terms of the GNU GPL, version 3
// http://www.gnu.org/licenses/gpl-3.0.txt

#include "../../Utilities/types.h"

#include <stdlib.h>
#include "aes.h"
#include "sha1.h"
#include "lz.h"
#include "ec.h"

enum { CRYPTO_MAX_PATH = 4096 };

// Auxiliary functions (endian swap, xor, and file name).
inline u16 swap16(u16 i)
{
#if defined(__GNUG__)
	return __builtin_bswap16(i);
#else
	return _byteswap_ushort(i);
#endif
}

inline u32 swap32(u32 i)
{
#if defined(__GNUG__)
	return __builtin_bswap32(i);
#else
	return _byteswap_ulong(i);
#endif
}

inline u64 swap64(u64 i)
{
#if defined(__GNUG__)
	return __builtin_bswap64(i);
#else
	return _byteswap_uint64(i);
#endif
}

char* extract_file_name(const char* file_path, char real_file_name[CRYPTO_MAX_PATH]);

// Hex string conversion auxiliary functions.
u64 hex_to_u64(const char* hex_str);
void hex_to_bytes(unsigned char *data, const char *hex_str, unsigned int str_length);

// Crypto functions (AES128-CBC, AES128-ECB, SHA1-HMAC and AES-CMAC).
void aescbc128_decrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, int len);
void aescbc128_encrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, int len);
void aesecb128_encrypt(unsigned char *key, unsigned char *in, unsigned char *out);
bool hmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash, int hash_len);
void hmac_hash_forge(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash);
bool cmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash, int hash_len);
void cmac_hash_forge(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash);
