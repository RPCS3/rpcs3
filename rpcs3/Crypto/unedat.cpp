#include "stdafx.h"
#include "key_vault.h"
#include "unedat.h"
#include "sha1.h"
#include "lz.h"
#include "ec.h"
#include "utils.h"

#include "Emu/system_utils.hpp"

#include "util/asm.hpp"
#include <algorithm>
#include <span>

LOG_CHANNEL(edat_log, "EDAT");

void generate_key(int crypto_mode, int version, unsigned char *key_final, unsigned char *iv_final, unsigned char *key, unsigned char *iv)
{
	int mode = crypto_mode & 0xF0000000;
	uchar temp_iv[16]{};
	switch (mode)
	{
	case 0x10000000:
		// Encrypted ERK.
		// Decrypt the key with EDAT_KEY + EDAT_IV and copy the original IV.
		memcpy(temp_iv, EDAT_IV, 0x10);
		aescbc128_decrypt(const_cast<u8*>(version ? EDAT_KEY_1 : EDAT_KEY_0), temp_iv, key, key_final, 0x10);
		memcpy(iv_final, iv, 0x10);
		break;
	case 0x20000000:
		// Default ERK.
		// Use EDAT_KEY and EDAT_IV.
		memcpy(key_final, version ? EDAT_KEY_1 : EDAT_KEY_0, 0x10);
		memcpy(iv_final, EDAT_IV, 0x10);
		break;
	case 0x00000000:
		// Unencrypted ERK.
		// Use the original key and iv.
		memcpy(key_final, key, 0x10);
		memcpy(iv_final, iv, 0x10);
		break;
	}
}

void generate_hash(int hash_mode, int version, unsigned char *hash_final, unsigned char *hash)
{
	int mode = hash_mode & 0xF0000000;
	uchar temp_iv[16]{};
	switch (mode)
	{
	case 0x10000000:
		// Encrypted HASH.
		// Decrypt the hash with EDAT_KEY + EDAT_IV.
		memcpy(temp_iv, EDAT_IV, 0x10);
		aescbc128_decrypt(const_cast<u8*>(version ? EDAT_KEY_1 : EDAT_KEY_0), temp_iv, hash, hash_final, 0x10);
		break;
	case 0x20000000:
		// Default HASH.
		// Use EDAT_HASH.
		memcpy(hash_final, version ? EDAT_HASH_1 : EDAT_HASH_0, 0x10);
		break;
	case 0x00000000:
		// Unencrypted ERK.
		// Use the original hash.
		memcpy(hash_final, hash, 0x10);
		break;
	};
}

bool decrypt(int hash_mode, int crypto_mode, int version, unsigned char *in, unsigned char *out, usz length, unsigned char *key, unsigned char *iv, unsigned char *hash, unsigned char *test_hash)
{
	// Setup buffers for key, iv and hash.
	unsigned char key_final[0x10] = {};
	unsigned char iv_final[0x10] = {};
	unsigned char hash_final_10[0x10] = {};
	unsigned char hash_final_14[0x14] = {};

	// Generate crypto key and hash.
	generate_key(crypto_mode, version, key_final, iv_final, key, iv);
	if ((hash_mode & 0xFF) == 0x01)
		generate_hash(hash_mode, version, hash_final_14, hash);
	else
		generate_hash(hash_mode, version, hash_final_10, hash);

	if ((crypto_mode & 0xFF) == 0x01)  // No algorithm.
	{
		memcpy(out, in, length);
	}
	else if ((crypto_mode & 0xFF) == 0x02)  // AES128-CBC
	{
		aescbc128_decrypt(key_final, iv_final, in, out, length);
	}
	else
	{
		edat_log.error("Unknown crypto algorithm!");
		return false;
	}

	if ((hash_mode & 0xFF) == 0x01) // 0x14 SHA1-HMAC
	{
		return hmac_hash_compare(hash_final_14, 0x14, in, length, test_hash, 0x14);
	}
	else if ((hash_mode & 0xFF) == 0x02)  // 0x10 AES-CMAC
	{
		return cmac_hash_compare(hash_final_10, 0x10, in, length, test_hash, 0x10);
	}
	else if ((hash_mode & 0xFF) == 0x04) //0x10 SHA1-HMAC
	{
		return hmac_hash_compare(hash_final_10, 0x10, in, length, test_hash, 0x10);
	}
	else
	{
		edat_log.error("Unknown hashing algorithm!");
		return false;
	}
}

// EDAT/SDAT functions.
std::tuple<u64, s32, s32> dec_section(unsigned char* metadata)
{
	std::array<u8, 0x10> dec;
	dec[0x00] = (metadata[0xC] ^ metadata[0x8] ^ metadata[0x10]);
	dec[0x01] = (metadata[0xD] ^ metadata[0x9] ^ metadata[0x11]);
	dec[0x02] = (metadata[0xE] ^ metadata[0xA] ^ metadata[0x12]);
	dec[0x03] = (metadata[0xF] ^ metadata[0xB] ^ metadata[0x13]);
	dec[0x04] = (metadata[0x4] ^ metadata[0x8] ^ metadata[0x14]);
	dec[0x05] = (metadata[0x5] ^ metadata[0x9] ^ metadata[0x15]);
	dec[0x06] = (metadata[0x6] ^ metadata[0xA] ^ metadata[0x16]);
	dec[0x07] = (metadata[0x7] ^ metadata[0xB] ^ metadata[0x17]);
	dec[0x08] = (metadata[0xC] ^ metadata[0x0] ^ metadata[0x18]);
	dec[0x09] = (metadata[0xD] ^ metadata[0x1] ^ metadata[0x19]);
	dec[0x0A] = (metadata[0xE] ^ metadata[0x2] ^ metadata[0x1A]);
	dec[0x0B] = (metadata[0xF] ^ metadata[0x3] ^ metadata[0x1B]);
	dec[0x0C] = (metadata[0x4] ^ metadata[0x0] ^ metadata[0x1C]);
	dec[0x0D] = (metadata[0x5] ^ metadata[0x1] ^ metadata[0x1D]);
	dec[0x0E] = (metadata[0x6] ^ metadata[0x2] ^ metadata[0x1E]);
	dec[0x0F] = (metadata[0x7] ^ metadata[0x3] ^ metadata[0x1F]);

	u64 offset = read_from_ptr<be_t<u64>>(dec, 0);
	s32 length = read_from_ptr<be_t<s32>>(dec, 8);
	s32 compression_end = read_from_ptr<be_t<s32>>(dec, 12);

	return std::make_tuple(offset, length, compression_end);
}

u128 get_block_key(int block, NPD_HEADER *npd)
{
	unsigned char empty_key[0x10] = {};
	unsigned char *src_key = (npd->version <= 1) ? empty_key : npd->dev_hash;
	u128 dest_key{};
	std::memcpy(&dest_key, src_key, 0xC);

	s32 swappedBlock = std::bit_cast<be_t<s32>>(block);
	std::memcpy(reinterpret_cast<uchar*>(&dest_key) + 0xC, &swappedBlock, sizeof(swappedBlock));
	return dest_key;
}

// for out data, allocate a buffer the size of 'edat->block_size'
// Also, set 'in file' to the beginning of the encrypted data, which may be offset if inside another file, but normally just reset to beginning of file
// returns number of bytes written, -1 for error
s64 decrypt_block(const fs::file* in, u8* out, EDAT_HEADER *edat, NPD_HEADER *npd, u8* crypt_key, u32 block_num, u32 total_blocks, u64 size_left, bool is_out_buffer_aligned = false)
{
	// Get metadata info and setup buffers.
	const int metadata_section_size = ((edat->flags & EDAT_COMPRESSED_FLAG) != 0 || (edat->flags & EDAT_FLAG_0x20) != 0) ? 0x20 : 0x10;
	const int metadata_offset = 0x100;

	u8 hash[0x10] = { 0 };
	u8 key_result[0x10] = { 0 };
	u8 hash_result[0x14] = { 0 };

	u64 offset = 0;
	u64 metadata_sec_offset = 0;
	u64 length = 0;
	s32 compression_end = 0;
	unsigned char empty_iv[0x10] = {};

	// Decrypt the metadata.
	if ((edat->flags & EDAT_COMPRESSED_FLAG) != 0)
	{
		metadata_sec_offset = metadata_offset + u64{block_num} * metadata_section_size;

		u8 metadata[0x20]{};

		in->read_at(metadata_sec_offset, metadata, 0x20);

		// If the data is compressed, decrypt the metadata.
		// NOTE: For NPD version 1 the metadata is not encrypted.
		if (npd->version <= 1)
		{
			offset = read_from_ptr<be_t<u64>>(metadata, 0x10);
			length = read_from_ptr<be_t<s32>>(metadata, 0x18);
			compression_end = read_from_ptr<be_t<s32>>(metadata, 0x1C);
		}
		else
		{
			std::tie(offset, length, compression_end) = dec_section(metadata);
		}

		std::memcpy(hash_result, metadata, 0x10);
	}
	else if ((edat->flags & EDAT_FLAG_0x20) != 0)
	{
		// If FLAG 0x20, the metadata precedes each data block.
		metadata_sec_offset = metadata_offset + u64{block_num} * (metadata_section_size + edat->block_size);

		u8 metadata[0x20]{};
		in->read_at(metadata_sec_offset, metadata, 0x20);

		std::memcpy(hash_result, metadata, 0x14);

		// If FLAG 0x20 is set, apply custom xor.
		for (int j = 0; j < 0x10; j++)
			hash_result[j] = metadata[j] ^ metadata[j + 0x10];

		offset = metadata_sec_offset + 0x20;
		length = edat->block_size;

		if ((block_num == (total_blocks - 1)) && (edat->file_size % edat->block_size))
			length = static_cast<s32>(edat->file_size % edat->block_size);
	}
	else
	{
		metadata_sec_offset = metadata_offset + u64{block_num} * metadata_section_size;

		in->read_at(metadata_sec_offset, hash_result, 0x10);

		offset = metadata_offset + u64{block_num} * edat->block_size + total_blocks * metadata_section_size;
		length = edat->block_size;

		if ((block_num == (total_blocks - 1)) && (edat->file_size % edat->block_size))
			length = static_cast<s32>(edat->file_size % edat->block_size);
	}

	// Locate the real data.
	const usz pad_length = length;
	length = utils::align<usz>(pad_length, 0x10);

	// Setup buffers for decryption and read the data.
	std::vector<u8> enc_data_buf(is_out_buffer_aligned || length == pad_length ? 0 : length);
	std::vector<u8> dec_data_buf(length);

	// Try to use out buffer for file reads if no padding is needed instead of a new buffer
	u8* enc_data = enc_data_buf.empty() ? out : enc_data_buf.data();

	// Variable to avoid copies when possible
	u8* dec_data = dec_data_buf.data();

	std::memset(hash, 0, 0x10);
	std::memset(key_result, 0, 0x10);

	in->read_at(offset, enc_data, length);

	// Generate a key for the current block.
	auto b_key = get_block_key(block_num, npd);

	// Encrypt the block key with the crypto key.
	aesecb128_encrypt(crypt_key, reinterpret_cast<uchar*>(&b_key), key_result);

	if ((edat->flags & EDAT_FLAG_0x10) != 0)
	{
		aesecb128_encrypt(crypt_key, key_result, hash);  // If FLAG 0x10 is set, encrypt again to get the final hash.
	}
	else
	{
		std::memcpy(hash, key_result, 0x10);
	}

	// Setup the crypto and hashing mode based on the extra flags.
	int crypto_mode = ((edat->flags & EDAT_FLAG_0x02) == 0) ? 0x2 : 0x1;
	int hash_mode;

	if ((edat->flags & EDAT_FLAG_0x10) == 0)
		hash_mode = 0x02;
	else if ((edat->flags & EDAT_FLAG_0x20) == 0)
		hash_mode = 0x04;
	else
		hash_mode = 0x01;

	if ((edat->flags & EDAT_ENCRYPTED_KEY_FLAG) != 0)
	{
		crypto_mode |= 0x10000000;
		hash_mode |= 0x10000000;
	}

	const bool should_decompress = ((edat->flags & EDAT_COMPRESSED_FLAG) != 0) && compression_end;

	if ((edat->flags & EDAT_DEBUG_DATA_FLAG) != 0)
	{
		// Reset the flags.
		crypto_mode |= 0x01000000;
		hash_mode |= 0x01000000;

		// Simply copy the data without the header or the footer.
		if (should_decompress)
		{
			std::memcpy(dec_data, enc_data, length);
		}
		else
		{
			// Optimize when decompression is not needed by avoiding 2 copies
			dec_data = enc_data;
		}
	}
	else
	{
		// IV is null if NPD version is 1 or 0.
		u8* iv = (npd->version <= 1) ? empty_iv : npd->digest;

		// Call main crypto routine on this data block.
		if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), enc_data, dec_data, length, key_result, iv, hash, hash_result))
		{
			edat_log.error("Block at offset 0x%llx has invalid hash!", offset);
			return -1;
		}
	}

	// Apply additional de-compression if needed and write the decrypted data.
	if (should_decompress)
	{
		const int res = decompress(out, dec_data, edat->block_size);

		size_left -= res;

		if (size_left == 0)
		{
			if (res < 0)
			{
				edat_log.error("Decompression failed!");
				return -1;
			}
		}

		return res;
	}

	if (dec_data != out)
	{
		std::memcpy(out, dec_data, pad_length);
	}

	return pad_length;
}

// set file offset to beginning before calling
bool check_data(u8* key, EDAT_HEADER* edat, NPD_HEADER* npd, const fs::file* f, bool verbose)
{
	u8 header[0xA0] = { 0 };
	u8 empty_header[0xA0] = { 0 };
	u8 header_hash[0x10] = { 0 };
	u8 metadata_hash[0x10] = { 0 };

	const u64 file_offset = f->pos();

	// Check NPD version and flags.
	if ((npd->version == 0) || (npd->version == 1))
	{
		if (edat->flags & 0x7EFFFFFE)
		{
			edat_log.error("Bad header flags!");
			return false;
		}
	}
	else if (npd->version == 2)
	{
		if (edat->flags & 0x7EFFFFE0)
		{
			edat_log.error("Bad header flags!");
			return false;
		}
	}
	else if ((npd->version == 3) || (npd->version == 4))
	{
		if (edat->flags & 0x7EFFFFC0)
		{
			edat_log.error("Bad header flags!");
			return false;
		}
	}
	else
	{
		edat_log.error("Unknown version!");
		return false;
	}

	// Read in the file header.
	f->read(header, 0xA0);

	// Read in the header and metadata section hashes.
	f->seek(file_offset + 0x90);
	f->read(metadata_hash, 0x10);
	f->read(header_hash, 0x10);

	// Setup the hashing mode and the crypto mode used in the file.
	const int crypto_mode = 0x1;
	int hash_mode = ((edat->flags & EDAT_ENCRYPTED_KEY_FLAG) == 0) ? 0x00000002 : 0x10000002;
	if ((edat->flags & EDAT_DEBUG_DATA_FLAG) != 0)
	{
		hash_mode |= 0x01000000;

		if (verbose)
			edat_log.warning("DEBUG data detected!");
	}

	// Setup header key and iv buffers.
	unsigned char header_key[0x10] = { 0 };
	unsigned char header_iv[0x10] = { 0 };

	// Test the header hash (located at offset 0xA0).
	if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), header, empty_header, 0xA0, header_key, header_iv, key, header_hash))
	{
		if (verbose)
			edat_log.warning("Header hash is invalid!");

		// If the header hash test fails and the data is not DEBUG, then RAP/RIF/KLIC key is invalid.
		if ((edat->flags & EDAT_DEBUG_DATA_FLAG) != EDAT_DEBUG_DATA_FLAG)
		{
			edat_log.error("RAP/RIF/KLIC key is invalid!");
			return false;
		}
	}

	// Parse the metadata info.
	const int metadata_section_size = ((edat->flags & EDAT_COMPRESSED_FLAG) != 0 || (edat->flags & EDAT_FLAG_0x20) != 0) ? 0x20 : 0x10;
	if (((edat->flags & EDAT_COMPRESSED_FLAG) != 0))
	{
		if (verbose)
			edat_log.warning("COMPRESSED data detected!");
	}

	if (!edat->block_size)
	{
		return false;
	}

	const usz block_num = utils::aligned_div<u64>(edat->file_size, edat->block_size);
	constexpr usz metadata_offset = 0x100;
	const usz metadata_size = utils::mul_saturate<u64>(metadata_section_size, block_num);
	u64 metadata_section_offset = metadata_offset;

	if (utils::add_saturate<u64>(utils::add_saturate<u64>(file_offset, metadata_section_offset), metadata_size) > f->size())
	{
		return false;
	}

	u64 bytes_read = 0;
	const auto metadata = std::make_unique<u8[]>(metadata_size);
	const auto empty_metadata = std::make_unique<u8[]>(metadata_size);

	while (bytes_read < metadata_size)
	{
		// Locate the metadata blocks.
		const usz offset = file_offset + metadata_section_offset;

		// Read in the metadata.
		f->read_at(offset, metadata.get() + bytes_read, metadata_section_size);

		// Adjust sizes.
		bytes_read += metadata_section_size;

		if (((edat->flags & EDAT_FLAG_0x20) != 0)) // Metadata block before each data block.
			metadata_section_offset += (metadata_section_size + edat->block_size);
		else
			metadata_section_offset += metadata_section_size;
	}

	// Test the metadata section hash (located at offset 0x90).
	if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), metadata.get(), empty_metadata.get(), metadata_size, header_key, header_iv, key, metadata_hash))
	{
		if (verbose)
			edat_log.warning("Metadata section hash is invalid!");
	}

	// Checking ECDSA signatures.
	if ((edat->flags & EDAT_DEBUG_DATA_FLAG) == 0)
	{
		// Setup buffers.
		unsigned char metadata_signature[0x28] = { 0 };
		unsigned char header_signature[0x28] = { 0 };
		unsigned char signature_hash[20] = { 0 };
		unsigned char signature_r[0x15] = { 0 };
		unsigned char signature_s[0x15] = { 0 };
		unsigned char zero_buf[0x15] = { 0 };

		// Setup ECDSA curve and public key.
		ecdsa_set_curve(VSH_CURVE_P, VSH_CURVE_A, VSH_CURVE_B, VSH_CURVE_N, VSH_CURVE_GX, VSH_CURVE_GY);
		ecdsa_set_pub(VSH_PUB);

		// Read in the metadata and header signatures.
		f->seek(0xB0);
		f->read(metadata_signature, 0x28);
		f->read(header_signature, 0x28);

		// Checking metadata signature.
		// Setup signature r and s.
		signature_r[0] = 0;
		signature_s[0] = 0;
		std::memcpy(signature_r + 1, metadata_signature, 0x14);
		std::memcpy(signature_s + 1, metadata_signature + 0x14, 0x14);
		if ((!std::memcmp(signature_r, zero_buf, 0x15)) || (!std::memcmp(signature_s, zero_buf, 0x15)))
		{
			edat_log.warning("Metadata signature is invalid!");
		}
		else
		{
			// Setup signature hash.
			if ((edat->flags & EDAT_FLAG_0x20) != 0) //Sony failed again, they used buffer from 0x100 with half size of real metadata.
			{
				const usz metadata_buf_size = block_num * 0x10;

				std::vector<u8> metadata_buf(metadata_buf_size);

				f->read_at(file_offset + metadata_offset, metadata_buf.data(), metadata_buf_size);

				sha1(metadata_buf.data(), metadata_buf_size, signature_hash);
			}
			else
				sha1(metadata.get(), metadata_size, signature_hash);

			if (!ecdsa_verify(signature_hash, signature_r, signature_s))
			{
				edat_log.warning("Metadata signature is invalid!");
				if (((edat->block_size + 0ull) * block_num) > 0x100000000)
					edat_log.warning("*Due to large file size, metadata signature status may be incorrect!");
			}
		}

		// Checking header signature.
		// Setup header signature r and s.
		signature_r[0] = 0;
		signature_s[0] = 0;
		std::memcpy(signature_r + 1, header_signature, 0x14);
		std::memcpy(signature_s + 1, header_signature + 0x14, 0x14);

		if ((!std::memcmp(signature_r, zero_buf, 0x15)) || (!std::memcmp(signature_s, zero_buf, 0x15)))
		{
			edat_log.warning("Header signature is invalid!");
		}
		else
		{
			// Setup header signature hash.
			std::memset(signature_hash, 0, 20);

			u8 header_buf[0xD8]{};

			f->read_at(file_offset, header_buf, 0xD8);
			sha1(header_buf, 0xD8, signature_hash);

			if (!ecdsa_verify(signature_hash, signature_r, signature_s))
				edat_log.warning("Header signature is invalid!");
		}
	}

	return true;
}

bool validate_dev_klic(const u8* klicensee, NPD_HEADER *npd)
{
	if ((npd->license & 0x3) != 0x3)
	{
		return true;
	}

	unsigned char dev[0x60]{};

	// Build the dev buffer (first 0x60 bytes of NPD header in big-endian).
	std::memcpy(dev, npd, 0x60);

	// Fix endianness.
	s32 version = std::bit_cast<be_t<s32>>(npd->version);
	s32 license = std::bit_cast<be_t<s32>>(npd->license);
	s32 type = std::bit_cast<be_t<s32>>(npd->type);
	std::memcpy(dev + 0x4, &version, 4);
	std::memcpy(dev + 0x8, &license, 4);
	std::memcpy(dev + 0xC, &type, 4);

	// Check for an empty dev_hash (can't validate if devklic is NULL);
	u128 klic;
	std::memcpy(&klic, klicensee, sizeof(klic));

	// Generate klicensee xor key.
	u128 key = klic ^ std::bit_cast<u128>(NP_OMAC_KEY_2);

	// Hash with generated key and compare with dev_hash.
	return cmac_hash_compare(reinterpret_cast<uchar*>(&key), 0x10, dev, 0x60, npd->dev_hash, 0x10);
}

bool validate_npd_hashes(std::string_view file_name, const u8* klicensee, NPD_HEADER* npd, EDAT_HEADER* edat, bool verbose)
{
	// Ignore header validation in DEBUG data.
	if (edat->flags & EDAT_DEBUG_DATA_FLAG)
	{
		return true;
	}

	if (!validate_dev_klic(klicensee, npd))
	{
		return false;
	}

	if (file_name.empty())
	{
		return true;
	}

	const usz buf_len = 0x30 + file_name.size();

	std::unique_ptr<u8[]> buf(new u8[buf_len]);
	std::unique_ptr<u8[]> buf_lower(new u8[buf_len]);
	std::unique_ptr<u8[]> buf_upper(new u8[buf_len]);

	// Build the title buffer (content_id + file_name).
	std::memcpy(buf.get(), npd->content_id, 0x30);
	std::memcpy(buf.get() + 0x30, file_name.data(), file_name.size());

	std::memcpy(buf_lower.get(), buf.get(), buf_len);
	std::memcpy(buf_upper.get(), buf.get(), buf_len);

	const auto buf_span = std::span(buf.get(), buf.get() + buf_len);
	const auto it = std::find(buf_span.rbegin(), buf_span.rend() - 0x30, '.');
	for (usz i = std::distance(it, buf_span.rend()) - 1; i < buf_len; ++i)
	{
		const u8 c = buf[i];
		buf_upper[i] = std::toupper(c);
		buf_lower[i] = std::tolower(c);
	}

	// Hash with NPDRM_OMAC_KEY_3 and compare with title_hash.
	// Try to ignore case sensivity with file extension
	const bool title_hash_result =
		cmac_hash_compare(const_cast<u8*>(NP_OMAC_KEY_3), 0x10, buf.get(), buf_len, npd->title_hash, 0x10) ||
		cmac_hash_compare(const_cast<u8*>(NP_OMAC_KEY_3), 0x10, buf_lower.get(), buf_len, npd->title_hash, 0x10) ||
		cmac_hash_compare(const_cast<u8*>(NP_OMAC_KEY_3), 0x10, buf_upper.get(), buf_len, npd->title_hash, 0x10);

	if (verbose)
	{
		if (title_hash_result)
			edat_log.notice("NPD title hash is valid!");
		else
			edat_log.warning("NPD title hash is invalid!");
	}

	return title_hash_result;
}

void read_npd_edat_header(const fs::file* input, NPD_HEADER& NPD, EDAT_HEADER& EDAT)
{
	char npd_header[0x80]{};
	char edat_header[0x10]{};

	usz pos = input->pos();
	pos += input->read_at(pos, npd_header, sizeof(npd_header));
	input->read_at(pos, edat_header, sizeof(edat_header));

	std::memcpy(&NPD.magic, npd_header, 4);
	NPD.version = read_from_ptr<be_t<s32>>(npd_header, 4);
	NPD.license = read_from_ptr<be_t<s32>>(npd_header, 8);
	NPD.type = read_from_ptr<be_t<s32>>(npd_header, 12);
	std::memcpy(NPD.content_id, &npd_header[16], 0x30);
	std::memcpy(NPD.digest, &npd_header[64], 0x10);
	std::memcpy(NPD.title_hash, &npd_header[80], 0x10);
	std::memcpy(NPD.dev_hash, &npd_header[96], 0x10);
	NPD.activate_time = read_from_ptr<be_t<s64>>(npd_header, 112);
	NPD.expire_time = read_from_ptr<be_t<s64>>(npd_header, 120);

	EDAT.flags = read_from_ptr<be_t<s32>>(edat_header, 0);
	EDAT.block_size = read_from_ptr<be_t<s32>>(edat_header, 4);
	EDAT.file_size = read_from_ptr<be_t<u64>>(edat_header, 8);
}

u128 GetEdatRifKeyFromRapFile(const fs::file& rap_file)
{
	u128 rapkey{};
	u128 rifkey{};

	rap_file.read<u128>(rapkey);

	rap_to_rif(reinterpret_cast<const uchar*>(&rapkey), reinterpret_cast<uchar*>(&rifkey));

	return rifkey;
}

bool VerifyEDATHeaderWithKLicense(const fs::file& input, const std::string& input_file_name, const u8* custom_klic, NPD_HEADER* npd_out)
{
	// Setup NPD and EDAT/SDAT structs.
	NPD_HEADER NPD;
	EDAT_HEADER EDAT;

	// Read in the NPD and EDAT/SDAT headers.
	read_npd_edat_header(&input, NPD, EDAT);

	if (NPD.magic != "NPD\0"_u32)
	{
		edat_log.error("%s has invalid NPD header or already decrypted.", input_file_name);
		return false;
	}

	if ((EDAT.flags & SDAT_FLAG) == SDAT_FLAG)
	{
		edat_log.error("SDATA file given to edat function");
		return false;
	}

	// Perform header validation (EDAT only).
	char real_file_name[CRYPTO_MAX_PATH]{};
	extract_file_name(input_file_name.c_str(), real_file_name);
	if (!validate_npd_hashes(real_file_name, custom_klic, &NPD, &EDAT, false))
	{
		edat_log.error("NPD hash validation failed!");
		return false;
	}

	if (npd_out)
	{
		*npd_out = NPD;
	}

	return true;
}

// Decrypts full file
fs::file DecryptEDAT(const fs::file& input, const std::string& input_file_name, int mode, u8 *custom_klic)
{
	if (!input)
	{
		return {};
	}

	// Prepare the files.
	input.seek(0);

	// Set DEVKLIC
	u128 devklic{};

	// Select the EDAT key mode.
	switch (mode)
	{
	case 0:
		break;
	case 1:
		memcpy(&devklic, NP_KLIC_FREE, 0x10);
		break;
	case 2:
		memcpy(&devklic, NP_OMAC_KEY_2, 0x10);
		break;
	case 3:
		memcpy(&devklic, NP_OMAC_KEY_3, 0x10);
		break;
	case 4:
		memcpy(&devklic, NP_KLIC_KEY, 0x10);
		break;
	case 5:
		memcpy(&devklic, NP_PSX_KEY, 0x10);
		break;
	case 6:
		memcpy(&devklic, NP_PSP_KEY_1, 0x10);
		break;
	case 7:
		memcpy(&devklic, NP_PSP_KEY_2, 0x10);
		break;
	case 8:
		{
			if (custom_klic != NULL)
				memcpy(&devklic, custom_klic, 0x10);
			else
			{
				edat_log.error("Invalid custom klic!");
				return fs::file{};
			}
			break;
		}
	default:
		edat_log.error("Invalid mode!");
		return fs::file{};
	}

	// Delete the bad output file if any errors arise.
	auto data = std::make_unique<EDATADecrypter>(input, devklic, input_file_name, false);

	if (!data->ReadHeader())
	{
		return fs::file{};
	}

	fs::file output;
	output.reset(std::move(data));

	return output;
}

bool EDATADecrypter::ReadHeader()
{
	edata_file.seek(0);

	// Read in the NPD and EDAT/SDAT headers.
	read_npd_edat_header(&edata_file, npdHeader, edatHeader);

	if (npdHeader.magic != "NPD\0"_u32)
	{
		edat_log.error("Not an NPDRM file");
		return false;
	}

	// Check for SDAT flag.
	if ((edatHeader.flags & SDAT_FLAG) == SDAT_FLAG)
	{
		// Generate SDAT key.
		dec_key = std::bit_cast<u128>(npdHeader.dev_hash) ^ std::bit_cast<u128>(SDAT_KEY);
	}
	else
	{
		// extract key from RIF
		char real_file_name[CRYPTO_MAX_PATH]{};
		extract_file_name(m_file_name.c_str(), real_file_name);

		if (!validate_npd_hashes(real_file_name, reinterpret_cast<const u8*>(&dec_key), &npdHeader, &edatHeader, false))
		{
			edat_log.error("NPD hash validation failed!");
			return true;
		}

		// Select EDAT key.
		if (m_is_key_final)
		{
			// Already provided
		}
		// Type 3: Use supplied dec_key.
		else if ((npdHeader.license & 0x3) == 0x3)
		{
			//
		}
		// Type 2: Use key from RAP file (RIF key). (also used for type 1 at the moment)
		else 
		{
			const std::string rap_path = rpcs3::utils::get_rap_file_path(npdHeader.content_id);

			if (fs::file rap{rap_path}; rap && rap.size() >= sizeof(dec_key))
			{
				dec_key = GetEdatRifKeyFromRapFile(rap);
			}

			// Make sure we don't have an empty RIF key.
			if (!dec_key)
			{
				edat_log.error("A valid RAP file is needed for this EDAT file! (license=%d)", npdHeader.license);
				return true;
			}

			edat_log.trace("RIFKEY: %s", std::bit_cast<be_t<u128>>(dec_key));
		}
	}

	edata_file.seek(0);

	// k the ecdsa_verify function in this check_data function takes a ridiculous amount of time
	// like it slows down load time by a factor of x20, at least, so its ignored for now
	//if (!check_data(reinterpret_cast<u8*>(&dec_key), &edatHeader, &npdHeader, &edata_file, false))
	//{
	//	edat_log.error("NPDRM check_data() failed!");
	//	return false;
	//}

	file_size = edatHeader.file_size;
	total_blocks = ::narrow<u32>(utils::aligned_div(edatHeader.file_size, edatHeader.block_size));

	// Try decrypting the first block instead
	u8 data_sample[1];

	if (file_size && !ReadData(0, data_sample, 1))
	{
		edat_log.error("NPDRM ReadData() failed!");
		return false;
	}

	return true;
}

u64 EDATADecrypter::ReadData(u64 pos, u8* data, u64 size)
{
	size = std::min<u64>(size, pos > edatHeader.file_size ? 0 : edatHeader.file_size - pos);

	if (!size)
	{
		return 0;
	}

	// Now we need to offset things to account for the actual 'range' requested
	const u64 startOffset = pos % edatHeader.block_size;

	const u64 num_blocks = utils::aligned_div(startOffset + size, edatHeader.block_size);

	// Find and decrypt block range covering pos + size
	const u32 starting_block = ::narrow<u32>(pos / edatHeader.block_size);
	const u32 ending_block = ::narrow<u32>(std::min<u64>(starting_block + num_blocks, total_blocks));

	u64 writeOffset = 0;

	std::vector<u8> data_buf(edatHeader.block_size + 16);

	for (u32 i = starting_block; i < ending_block; i++)
	{
		u64 res = decrypt_block(&edata_file, data_buf.data(), &edatHeader, &npdHeader, reinterpret_cast<uchar*>(&dec_key), i, total_blocks, edatHeader.file_size, true);

		if (res == umax)
		{
			edat_log.error("Error Decrypting data");
			return 0;
		}

		const usz skip_start = (i == starting_block ? startOffset : 0);

		if (skip_start >= res)
		{
			break;
		}

		const usz end_pos = (i != total_blocks - 1 ? edatHeader.block_size : (edatHeader.file_size - 1) % edatHeader.block_size + 1);
		const usz read_end = std::min<usz>(res, i == ending_block - 1 ? std::min<usz>(end_pos, (startOffset + size - 1) % edatHeader.block_size + 1) : end_pos);

		std::memcpy(data + writeOffset, data_buf.data() + skip_start, read_end - skip_start);
		std::memset(data_buf.data(), 0, read_end - skip_start);

		writeOffset += read_end - skip_start;
	}

	return writeOffset;
}
