// Copyright (C) 2014       Hykem <hykem@hotmail.com>
// Licensed under the terms of the GNU GPL, version 2.0 or later versions.
// http://www.gnu.org/licenses/gpl-2.0.txt

#include "utils.h"
#include "aes.h"
#include "sha1.h"
#include "sha256.h"
#include "key_vault.h"
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include "Utilities/StrUtil.h"
#include "Utilities/File.h"

#include <memory>
#include <string>
#include <string_view>
#include <span>

// Auxiliary functions (endian swap, xor).

// Hex string conversion auxiliary functions.
void hex_to_bytes(unsigned char* data, std::string_view hex_str, unsigned int str_length)
{
	const auto strn_length = (str_length > 0) ? str_length : hex_str.size();

	// Don't convert if the string length is odd.
	if ((strn_length % 2) == 0)
	{
		for (size_t i = 0; i < strn_length; i += 2)
		{
			const auto [ptr, err] = std::from_chars(hex_str.data() + i, hex_str.data() + i + 2, *data++, 16);
			if (err != std::errc())
			{
				fmt::throw_exception("Failed to read hex string: %s", std::make_error_code(err).message());
			}
		}
	}
}

// Crypto functions (AES128-CBC, AES128-ECB, SHA1-HMAC and AES-CMAC).
void aescbc128_decrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, usz len)
{
	aes_context ctx;
	aes_setkey_dec(&ctx, key, 128);
	aes_crypt_cbc(&ctx, AES_DECRYPT, len, iv, in, out);

	// Reset the IV.
	memset(iv, 0, 0x10);
}

void aescbc128_encrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, usz len)
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

bool hmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, usz in_len, unsigned char *hash, usz hash_len)
{
	const std::unique_ptr<u8[]> out(new u8[key_len]);

	sha1_hmac(key, key_len, in, in_len, out.get());

	return std::memcmp(out.get(), hash, hash_len) == 0;
}

void hmac_hash_forge(unsigned char *key, int key_len, unsigned char *in, usz in_len, unsigned char *hash)
{
	sha1_hmac(key, key_len, in, in_len, hash);
}

bool cmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, usz in_len, unsigned char *hash, usz hash_len)
{
	const std::unique_ptr<u8[]> out(new u8[key_len]);

	aes_context ctx;
	aes_setkey_enc(&ctx, key, 128);
	aes_cmac(&ctx, in_len, in, out.get());

	return std::memcmp(out.get(), hash, hash_len) == 0;
}

void cmac_hash_forge(unsigned char *key, int /*key_len*/, unsigned char *in, usz in_len, unsigned char *hash)
{
	aes_context ctx;
	aes_setkey_enc(&ctx, key, 128);
	aes_cmac(&ctx, in_len, in, hash);
}

char* extract_file_name(const char* file_path, char real_file_name[CRYPTO_MAX_PATH])
{
	std::string_view v(file_path);

	if (const auto pos = v.find_last_of(fs::delim); pos != umax)
	{
		v.remove_prefix(pos + 1);
	}

	std::span r(real_file_name, CRYPTO_MAX_PATH);
	strcpy_trunc(r, v);
	return real_file_name;
}

std::string sha256_get_hash(const char* data, usz size, bool lower_case)
{
	u8 res_hash[32];
	mbedtls_sha256_context ctx;
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts_ret(&ctx, 0);
	mbedtls_sha256_update_ret(&ctx, reinterpret_cast<const unsigned char*>(data), size);
	mbedtls_sha256_finish_ret(&ctx, res_hash);

	std::string res_hash_string("0000000000000000000000000000000000000000000000000000000000000000");

	for (usz index = 0; index < 32; index++)
	{
		const auto pal                   = lower_case ? "0123456789abcdef" : "0123456789ABCDEF";
		res_hash_string[index * 2]       = pal[res_hash[index] >> 4];
		res_hash_string[(index * 2) + 1] = pal[res_hash[index] & 15];
	}

	return res_hash_string;
}

void mbedtls_zeroize(void *v, size_t n)
{
	static void *(*const volatile unop_memset)(void *, int, size_t) = &memset;
	(void)unop_memset(v, 0, n);
}


// SC passphrase crypto

void sc_form_key(const u8* sc_key, const std::array<u8, PASSPHRASE_KEY_LEN>& laid_paid, u8* key)
{
	for (u32 i = 0; i < PASSPHRASE_KEY_LEN; i++)
	{
		key[i] = static_cast<u8>(sc_key[i] ^ laid_paid[i]);
	}
}

std::array<u8, PASSPHRASE_KEY_LEN> sc_combine_laid_paid(s64 laid, s64 paid)
{
	const std::string paid_laid = fmt::format("%016llx%016llx", laid, paid);
	std::array<u8, PASSPHRASE_KEY_LEN> out{};
	hex_to_bytes(out.data(), paid_laid.c_str(), PASSPHRASE_KEY_LEN * 2);
	return out;
}

std::array<u8, PASSPHRASE_KEY_LEN> vtrm_get_laid_paid_from_type(int type)
{
	// No idea what this type stands for
	switch (type)
	{
	case 0: return sc_combine_laid_paid(0xFFFFFFFFFFFFFFFFL, 0xFFFFFFFFFFFFFFFFL);
	case 1: return sc_combine_laid_paid(LAID_2, 0x1070000000000001L);
	case 2: return sc_combine_laid_paid(LAID_2, 0x0000000000000000L);
	case 3: return sc_combine_laid_paid(LAID_2, PAID_69);
	default:
		fmt::throw_exception("vtrm_get_laid_paid_from_type: Wrong type specified (type=%d)", type);
	}
}

std::array<u8, PASSPHRASE_KEY_LEN> vtrm_portability_laid_paid()
{
	// 107000002A000001
	return sc_combine_laid_paid(0x0000000000000000L, 0x0000000000000000L);
}

int sc_decrypt(const u8* sc_key, const std::array<u8, PASSPHRASE_KEY_LEN>& laid_paid, u8* iv, u8* input, u8* output)
{
	aes_context ctx;
	u8 key[PASSPHRASE_KEY_LEN];
	sc_form_key(sc_key, laid_paid, key);
	aes_setkey_dec(&ctx, key, 128);
	return aes_crypt_cbc(&ctx, AES_DECRYPT, PASSPHRASE_OUT_LEN, iv, input, output);
}

int vtrm_decrypt(int type, u8* iv, u8* input, u8* output)
{
	return sc_decrypt(SC_ISO_SERIES_KEY_2, vtrm_get_laid_paid_from_type(type), iv, input, output);
}

int vtrm_decrypt_master(s64 laid, s64 paid, u8* iv, u8* input, u8* output)
{
	return sc_decrypt(SC_ISO_SERIES_INTERNAL_KEY_3, sc_combine_laid_paid(laid, paid), iv, input, output);
}

const u8* vtrm_portability_type_mapper(int type)
{
	// No idea what this type stands for
	switch (type)
	{
	//case 0: return key_for_type_1;
	case 1: return SC_ISO_SERIES_KEY_2;
	case 2: return SC_ISO_SERIES_KEY_1;
	case 3: return SC_KEY_FOR_MASTER_2;
	default:
		fmt::throw_exception("vtrm_portability_type_mapper: Wrong type specified (type=%d)", type);
	}
}

int vtrm_decrypt_with_portability(int type, u8* iv, u8* input, u8* output)
{
	return sc_decrypt(vtrm_portability_type_mapper(type), vtrm_portability_laid_paid(), iv, input, output);
}
