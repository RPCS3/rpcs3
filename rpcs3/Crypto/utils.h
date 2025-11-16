#pragma once

// Copyright (C) 2014       Hykem <hykem@hotmail.com>
// Licensed under the terms of the GNU GPL, version 2.0 or later versions.
// http://www.gnu.org/licenses/gpl-2.0.txt

#include "util/types.hpp"

#include <cstdlib>
#include <string_view>

enum { CRYPTO_MAX_PATH = 4096 };

char* extract_file_name(const char* file_path, char real_file_name[CRYPTO_MAX_PATH]);

std::string sha256_get_hash(const char* data, usz size, bool lower_case);

// Hex string conversion auxiliary functions.
void hex_to_bytes(unsigned char* data, std::string_view hex_str, unsigned int str_length);

// Crypto functions (AES128-CBC, AES128-ECB, SHA1-HMAC and AES-CMAC).
void aescbc128_decrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, usz len);
void aescbc128_encrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, usz len);
void aesecb128_encrypt(unsigned char *key, unsigned char *in, unsigned char *out);
bool hmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, usz in_len, unsigned char *hash, usz hash_len);
void hmac_hash_forge(unsigned char *key, int key_len, unsigned char *in, usz in_len, unsigned char *hash);
bool cmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, usz in_len, unsigned char *hash, usz hash_len);
void cmac_hash_forge(unsigned char *key, int key_len, unsigned char *in, usz in_len, unsigned char *hash);
void mbedtls_zeroize(void *v, size_t n);

// SC passphrase crypto

int vtrm_decrypt(int type, u8* iv, u8* input, u8* output);
int vtrm_decrypt_master(s64 laid, s64 paid, u8* iv, u8* input, u8* output);
int vtrm_decrypt_with_portability(int type, u8* iv, u8* input, u8* output);
