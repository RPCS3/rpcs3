// Copyright (C) 2014       Hykem <hykem@hotmail.com>
// Licensed under the terms of the GNU GPL, version 3
// http://www.gnu.org/licenses/gpl-3.0.txt

#include "stdafx.h"
#include "utils.h"
#include <stdio.h>
#include <time.h>

// Auxiliary functions (endian swap, xor and prng).
u16 swap16(u16 i)
{
	return ((i & 0xFF00) >>  8) | ((i & 0xFF) << 8);
}

u32 swap32(u32 i)
{
	return ((i & 0xFF000000) >> 24) | ((i & 0xFF0000) >>  8) | ((i & 0xFF00) <<  8) | ((i & 0xFF) << 24);
}

u64 swap64(u64 i)
{
	return ((i & 0x00000000000000ff) << 56) | ((i & 0x000000000000ff00) << 40) |
		((i & 0x0000000000ff0000) << 24) | ((i & 0x00000000ff000000) <<  8) |
		((i & 0x000000ff00000000) >>  8) | ((i & 0x0000ff0000000000) >> 24) |
		((i & 0x00ff000000000000) >> 40) | ((i & 0xff00000000000000) >> 56);
}

void xor_key(unsigned char *dest, unsigned char *src1, unsigned char *src2, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		dest[i] = src1[i] ^ src2[i];
	}
}

void prng(unsigned char *dest, int size)
{
    unsigned char *buffer = new unsigned char[size];
	srand((u32)time(0));

	int i;
	for(i = 0; i < size; i++)
      buffer[i] = (unsigned char)(rand() & 0xFF);

	memcpy(dest, buffer, size);

	delete[] buffer;
}

// Hex string conversion auxiliary functions.
u64 hex_to_u64(const char* hex_str)
{
	u32 length = (u32) strlen(hex_str);
	u64 tmp = 0;
	u64 result = 0;
	char c;

	while (length--)
	{
		c = *hex_str++;
		if((c >= '0') && (c <= '9'))
			tmp = c - '0';
		else if((c >= 'a') && (c <= 'f'))
			tmp = c - 'a' + 10;
		else if((c >= 'A') && (c <= 'F'))
			tmp = c - 'A' + 10;
		else
			tmp = 0;
		result |= (tmp << (length * 4));
	}

	return result;
}

void hex_to_bytes(unsigned char *data, const char *hex_str, unsigned int str_length)
{
	u32 strn_length = (str_length > 0) ? str_length : (u32) strlen(hex_str);
	u32 data_length = strn_length / 2;
	char tmp_buf[3] = {0, 0, 0};

	// Don't convert if the string length is odd.
	if (!(strn_length % 2))
	{
		u8 *out = (u8 *)malloc(strn_length * sizeof(u8));
		u8 *pos = out;

		while (strn_length--)
		{
			tmp_buf[0] = *hex_str++;
			tmp_buf[1] = *hex_str++;

			*pos++ = (u8)(hex_to_u64(tmp_buf) & 0xFF);
		}

		// Copy back to our array.
		memcpy(data, out, data_length);
	}
}

bool is_hex(const char* hex_str, unsigned int str_length)
{
    static const char hex_chars[] = "0123456789abcdefABCDEF";

    if (hex_str == NULL)
        return false;

    unsigned int i;
    for (i = 0; i < str_length; i++)
	{
		if (strchr(hex_chars, hex_str[i]) == 0)
			return false;
	}

    return true;
}

// Crypto functions (AES128-CBC, AES128-ECB, SHA1-HMAC and AES-CMAC).
void aescbc128_decrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, int len)
{
	aes_context ctx;
	aes_setkey_dec(&ctx, key, 128);
	aes_crypt_cbc(&ctx, AES_DECRYPT, len, iv, in, out);

	// Reset the IV.
	memset(iv, 0, 0x10);
}

void aescbc128_encrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, int len)
{
	aes_context ctx;
	aes_setkey_enc(&ctx, key, 128);
	aes_crypt_cbc(&ctx, AES_ENCRYPT, len, iv, in, out);

	// Reset the IV.
	memset(iv, 0, 0x10);
}

void aesecb128_encrypt(unsigned char *key, unsigned char *in, unsigned char *out)
{
	aes_context ctx;
	aes_setkey_enc(&ctx, key, 128);
	aes_crypt_ecb(&ctx, AES_ENCRYPT, in, out);
}

bool hmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash, int hash_len)
{
	unsigned char *out = new unsigned char[key_len];

	sha1_hmac(key, key_len, in, in_len, out);

	for (int i = 0; i < hash_len; i++)
	{
		if (out[i] != hash[i])
		{
			delete[] out;
			return false;
		}
	}

	delete[] out;

	return true;
}

void hmac_hash_forge(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash)
{
	sha1_hmac(key, key_len, in, in_len, hash);
}

bool cmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash, int hash_len)
{
	unsigned char *out = new unsigned char[key_len];

	aes_context ctx;
	aes_setkey_enc(&ctx, key, 128);
	aes_cmac(&ctx, in_len, in, out);

	for (int i = 0; i < hash_len; i++)
	{
		if (out[i] != hash[i])
		{
			delete[] out;
			return false;
		}
	}

	delete[] out;

	return true;
}

void cmac_hash_forge(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash)
{
	aes_context ctx;
	aes_setkey_enc(&ctx, key, 128);
	aes_cmac(&ctx, in_len, in, hash);
}

char* extract_file_name(const char* file_path, char real_file_name[MAX_PATH])
{
	size_t file_path_len = strlen(file_path);
	const char* p = strrchr(file_path, '/');
	if (!p) p = strrchr(file_path, '\\');
	if (p) file_path_len = file_path + file_path_len - p - 1;
	strncpy(real_file_name, p ? (p + 1) : file_path, file_path_len + 1);
	
	return real_file_name;
}