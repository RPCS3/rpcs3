#pragma once
#include "aes.h"
#include "sha1.h"

// Auxiliary functions (endian swap and xor).
u16 swap16(u16 i);
u32 swap32(u32 i);
u64 swap64(u64 i);
void xor_(unsigned char *dest, unsigned char *src1, unsigned char *src2, int size);

// Hex string conversion auxiliary functions.
u64 hex_to_u64(const char* hex_str);
void hex_to_bytes(unsigned char *data, const char *hex_str);

// Crypto functions (AES128-CBC, AES128-ECB, SHA1-HMAC and AES-CMAC).
void aescbc128_decrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, int len);
void aesecb128_encrypt(unsigned char *key, unsigned char *in, unsigned char *out);
bool hmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash);
bool cmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash);

// Reverse-engineered custom Lempel–Ziv–Markov based compression (unknown variant of LZRC).
int lz_decompress(unsigned char *out, unsigned char *in, unsigned int size);
