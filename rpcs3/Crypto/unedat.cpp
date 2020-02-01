#include "stdafx.h"
#include "key_vault.h"
#include "unedat.h"

#include <cmath>

LOG_CHANNEL(edat_log, "EDAT");

void generate_key(int crypto_mode, int version, unsigned char *key_final, unsigned char *iv_final, unsigned char *key, unsigned char *iv)
{
	int mode = crypto_mode & 0xF0000000;
	switch (mode)
	{
	case 0x10000000:
		// Encrypted ERK.
		// Decrypt the key with EDAT_KEY + EDAT_IV and copy the original IV.
		aescbc128_decrypt(version ? EDAT_KEY_1 : EDAT_KEY_0, EDAT_IV, key, key_final, 0x10);
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
	};
}

void generate_hash(int hash_mode, int version, unsigned char *hash_final, unsigned char *hash)
{
	int mode = hash_mode & 0xF0000000;
	switch (mode)
	{
	case 0x10000000:
		// Encrypted HASH.
		// Decrypt the hash with EDAT_KEY + EDAT_IV.
		aescbc128_decrypt(version ? EDAT_KEY_1 : EDAT_KEY_0, EDAT_IV, hash, hash_final, 0x10);
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

bool decrypt(int hash_mode, int crypto_mode, int version, unsigned char *in, unsigned char *out, int length, unsigned char *key, unsigned char *iv, unsigned char *hash, unsigned char *test_hash)
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
		edat_log.error("EDAT: Unknown crypto algorithm!");
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
		edat_log.error("EDAT: Unknown hashing algorithm!");
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

	u64 offset = swap64(*reinterpret_cast<u64*>(&dec[0]));
	s32 length = swap32(*reinterpret_cast<s32*>(&dec[8]));
	s32 compression_end = swap32(*reinterpret_cast<s32*>(&dec[12]));

	return std::make_tuple(offset, length, compression_end);
}

std::array<u8, 0x10> get_block_key(int block, NPD_HEADER *npd)
{
	unsigned char empty_key[0x10] = {};
	unsigned char *src_key = (npd->version <= 1) ? empty_key : npd->dev_hash;
	std::array<u8, 0x10> dest_key{};
	memcpy(dest_key.data(), src_key, 0xC);

	s32 swappedBlock = swap32(block);
	memcpy(&dest_key[0xC], &swappedBlock, sizeof(swappedBlock));
	return dest_key;
}

// for out data, allocate a buffer the size of 'edat->block_size'
// Also, set 'in file' to the beginning of the encrypted data, which may be offset if inside another file, but normally just reset to beginning of file
// returns number of bytes written, -1 for error
s64 decrypt_block(const fs::file* in, u8* out, EDAT_HEADER *edat, NPD_HEADER *npd, u8* crypt_key, u32 block_num, u32 total_blocks, u64 size_left)
{
	// Get metadata info and setup buffers.
	const int metadata_section_size = ((edat->flags & EDAT_COMPRESSED_FLAG) != 0 || (edat->flags & EDAT_FLAG_0x20) != 0) ? 0x20 : 0x10;
	const int metadata_offset = 0x100;

	std::unique_ptr<u8[]> enc_data;
	std::unique_ptr<u8[]> dec_data;
	u8 hash[0x10] = { 0 };
	u8 key_result[0x10] = { 0 };
	u8 hash_result[0x14] = { 0 };

	u64 offset = 0;
	u64 metadata_sec_offset = 0;
	s32 length = 0;
	s32 compression_end = 0;
	unsigned char empty_iv[0x10] = {};

	const u64 file_offset = in->pos();
	memset(hash_result, 0, 0x14);

	// Decrypt the metadata.
	if ((edat->flags & EDAT_COMPRESSED_FLAG) != 0)
	{
		metadata_sec_offset = metadata_offset + u64{block_num} * metadata_section_size;

		in->seek(file_offset + metadata_sec_offset);

		unsigned char metadata[0x20];
		memset(metadata, 0, 0x20);
		in->read(metadata, 0x20);

		// If the data is compressed, decrypt the metadata.
		// NOTE: For NPD version 1 the metadata is not encrypted.
		if (npd->version <= 1)
		{
			offset = swap64(*reinterpret_cast<u64*>(&metadata[0x10]));
			length = swap32(*reinterpret_cast<s32*>(&metadata[0x18]));
			compression_end = swap32(*reinterpret_cast<s32*>(&metadata[0x1C]));
		}
		else
		{
			std::tie(offset, length, compression_end) = dec_section(metadata);
		}

		memcpy(hash_result, metadata, 0x10);
	}
	else if ((edat->flags & EDAT_FLAG_0x20) != 0)
	{
		// If FLAG 0x20, the metadata precedes each data block.
		metadata_sec_offset = metadata_offset + u64{block_num} * (metadata_section_size + edat->block_size);
		in->seek(file_offset + metadata_sec_offset);

		unsigned char metadata[0x20];
		memset(metadata, 0, 0x20);
		in->read(metadata, 0x20);
		memcpy(hash_result, metadata, 0x14);

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
		in->seek(file_offset + metadata_sec_offset);

		in->read(hash_result, 0x10);
		offset = metadata_offset + u64{block_num} * edat->block_size + total_blocks * metadata_section_size;
		length = edat->block_size;

		if ((block_num == (total_blocks - 1)) && (edat->file_size % edat->block_size))
			length = static_cast<s32>(edat->file_size % edat->block_size);
	}

	// Locate the real data.
	const int pad_length = length;
	length = (pad_length + 0xF) & 0xFFFFFFF0;

	// Setup buffers for decryption and read the data.
	enc_data.reset(new u8[length]{ 0 });
	dec_data.reset(new u8[length]{ 0 });
	memset(hash, 0, 0x10);
	memset(key_result, 0, 0x10);

	in->seek(file_offset + offset);
	in->read(enc_data.get(), length);

	// Generate a key for the current block.
	std::array<u8, 0x10> b_key = get_block_key(block_num, npd);

	// Encrypt the block key with the crypto key.
	aesecb128_encrypt(crypt_key, b_key.data(), key_result);
	if ((edat->flags & EDAT_FLAG_0x10) != 0)
		aesecb128_encrypt(crypt_key, key_result, hash);  // If FLAG 0x10 is set, encrypt again to get the final hash.
	else
		memcpy(hash, key_result, 0x10);

	// Setup the crypto and hashing mode based on the extra flags.
	int crypto_mode = ((edat->flags & EDAT_FLAG_0x02) == 0) ? 0x2 : 0x1;
	int hash_mode;

	if ((edat->flags  & EDAT_FLAG_0x10) == 0)
		hash_mode = 0x02;
	else if ((edat->flags & EDAT_FLAG_0x20) == 0)
		hash_mode = 0x04;
	else
		hash_mode = 0x01;

	if ((edat->flags  & EDAT_ENCRYPTED_KEY_FLAG) != 0)
	{
		crypto_mode |= 0x10000000;
		hash_mode |= 0x10000000;
	}

	if ((edat->flags  & EDAT_DEBUG_DATA_FLAG) != 0)
	{
		// Reset the flags.
		crypto_mode |= 0x01000000;
		hash_mode |= 0x01000000;
		// Simply copy the data without the header or the footer.
		memcpy(dec_data.get(), enc_data.get(), length);
	}
	else
	{
		// IV is null if NPD version is 1 or 0.
		u8* iv = (npd->version <= 1) ? empty_iv : npd->digest;
		// Call main crypto routine on this data block.
		if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), enc_data.get(), dec_data.get(), length, key_result, iv, hash, hash_result))
		{
			edat_log.error("EDAT: Block at offset 0x%llx has invalid hash!", offset);
			return -1;
		}
	}

	// Apply additional de-compression if needed and write the decrypted data.
	if (((edat->flags & EDAT_COMPRESSED_FLAG) != 0) && compression_end)
	{
		const int res = decompress(out, dec_data.get(), edat->block_size);

		size_left -= res;

		if (size_left == 0)
		{
			if (res < 0)
			{
				edat_log.error("EDAT: Decompression failed!");
				return -1;
			}
		}
		return res;
	}
	else
	{
		memcpy(out, dec_data.get(), pad_length);
		return pad_length;
	}
}

// EDAT/SDAT decryption.
// reset file to beginning of data before calling
int decrypt_data(const fs::file* in, const fs::file* out, EDAT_HEADER *edat, NPD_HEADER *npd, unsigned char* crypt_key, bool verbose)
{
	const int total_blocks = static_cast<int>((edat->file_size + edat->block_size - 1) / edat->block_size);
	u64 size_left = edat->file_size;
	std::unique_ptr<u8[]> data(new u8[edat->block_size]);

	for (int i = 0; i < total_blocks; i++)
	{
		in->seek(0);
		memset(data.get(), 0, edat->block_size);
		u64 res = decrypt_block(in, data.get(), edat, npd, crypt_key, i, total_blocks, size_left);
		if (res == -1)
		{
			edat_log.error("EDAT: Decrypt Block failed!");
			return 1;
		}
		size_left -= res;
		out->write(data.get(), res);
	}

	return 0;
}

// set file offset to beginning before calling
int check_data(unsigned char *key, EDAT_HEADER *edat, NPD_HEADER *npd, const fs::file* f, bool verbose)
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
			edat_log.error("EDAT: Bad header flags!");
			return 1;
		}
	}
	else if (npd->version == 2)
	{
		if (edat->flags & 0x7EFFFFE0)
		{
			edat_log.error("EDAT: Bad header flags!");
			return 1;
		}
	}
	else if ((npd->version == 3) || (npd->version == 4))
	{
		if (edat->flags & 0x7EFFFFC0)
		{
			edat_log.error("EDAT: Bad header flags!");
			return 1;
		}
	}
	else
	{
		edat_log.error("EDAT: Unknown version!");
		return 1;
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
			edat_log.warning("EDAT: DEBUG data detected!");
	}

	// Setup header key and iv buffers.
	unsigned char header_key[0x10] = { 0 };
	unsigned char header_iv[0x10] = { 0 };

	// Test the header hash (located at offset 0xA0).
	if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), header, empty_header, 0xA0, header_key, header_iv, key, header_hash))
	{
		if (verbose)
			edat_log.warning("EDAT: Header hash is invalid!");

		// If the header hash test fails and the data is not DEBUG, then RAP/RIF/KLIC key is invalid.
		if ((edat->flags & EDAT_DEBUG_DATA_FLAG) != EDAT_DEBUG_DATA_FLAG)
		{
			edat_log.error("EDAT: RAP/RIF/KLIC key is invalid!");
			return 1;
		}
	}

	// Parse the metadata info.
	const int metadata_section_size = ((edat->flags & EDAT_COMPRESSED_FLAG) != 0 || (edat->flags & EDAT_FLAG_0x20) != 0) ? 0x20 : 0x10;
	if (((edat->flags & EDAT_COMPRESSED_FLAG) != 0))
	{
		if (verbose)
			edat_log.warning("EDAT: COMPRESSED data detected!");
	}

	const int block_num = static_cast<int>((edat->file_size + edat->block_size - 1) / edat->block_size);
	const int metadata_offset = 0x100;
	const int metadata_size = metadata_section_size * block_num;
	u64 metadata_section_offset = metadata_offset;

	long bytes_read = 0;
	long bytes_to_read = metadata_size;
	std::unique_ptr<u8[]> metadata(new u8[metadata_size]);
	std::unique_ptr<u8[]> empty_metadata(new u8[metadata_size]);

	while (bytes_to_read > 0)
	{
		// Locate the metadata blocks.
		f->seek(file_offset + metadata_section_offset);

		// Read in the metadata.
		f->read(metadata.get() + bytes_read, metadata_section_size);

		// Adjust sizes.
		bytes_read += metadata_section_size;
		bytes_to_read -= metadata_section_size;

		if (((edat->flags & EDAT_FLAG_0x20) != 0)) // Metadata block before each data block.
			metadata_section_offset += (metadata_section_size + edat->block_size);
		else
			metadata_section_offset += metadata_section_size;
	}

	// Test the metadata section hash (located at offset 0x90).
	if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), metadata.get(), empty_metadata.get(), metadata_size, header_key, header_iv, key, metadata_hash))
	{
		if (verbose)
			edat_log.warning("EDAT: Metadata section hash is invalid!");
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
		f->seek(file_offset + 0xB0);
		f->read(metadata_signature, 0x28);
		f->read(header_signature, 0x28);

		// Checking metadata signature.
		// Setup signature r and s.
		memcpy(signature_r + 01, metadata_signature, 0x14);
		memcpy(signature_s + 01, metadata_signature + 0x14, 0x14);
		if ((!memcmp(signature_r, zero_buf, 0x15)) || (!memcmp(signature_s, zero_buf, 0x15)))
			edat_log.warning("EDAT: Metadata signature is invalid!");
		else
		{
			// Setup signature hash.
			if ((edat->flags & EDAT_FLAG_0x20) != 0) //Sony failed again, they used buffer from 0x100 with half size of real metadata.
			{
				int metadata_buf_size = block_num * 0x10;
				std::unique_ptr<u8[]> metadata_buf(new u8[metadata_buf_size]);
				f->seek(file_offset + metadata_offset);
				f->read(metadata_buf.get(), metadata_buf_size);
				sha1(metadata_buf.get(), metadata_buf_size, signature_hash);
			}
			else
				sha1(metadata.get(), metadata_size, signature_hash);

			if (!ecdsa_verify(signature_hash, signature_r, signature_s))
			{
				edat_log.warning("EDAT: Metadata signature is invalid!");
				if (((edat->block_size + 0ull) * block_num) > 0x100000000)
					edat_log.warning("EDAT: *Due to large file size, metadata signature status may be incorrect!");
			}
		}

		// Checking header signature.
		// Setup header signature r and s.
		memset(signature_r, 0, 0x15);
		memset(signature_s, 0, 0x15);
		memcpy(signature_r + 01, header_signature, 0x14);
		memcpy(signature_s + 01, header_signature + 0x14, 0x14);

		if ((!memcmp(signature_r, zero_buf, 0x15)) || (!memcmp(signature_s, zero_buf, 0x15)))
			edat_log.warning("EDAT: Header signature is invalid!");
		else
		{
			// Setup header signature hash.
			memset(signature_hash, 0, 20);
			std::unique_ptr<u8[]> header_buf(new u8[0xD8]);
			f->seek(file_offset);
			f->read(header_buf.get(), 0xD8);
			sha1(header_buf.get(), 0xD8, signature_hash );

			if (!ecdsa_verify(signature_hash, signature_r, signature_s))
				edat_log.warning("EDAT: Header signature is invalid!");
		}
	}

	return 0;
}

int validate_dev_klic(const u8* klicensee, NPD_HEADER *npd)
{
	unsigned char dev[0x60] = { 0 };
	unsigned char key[0x10] = { 0 };

	// Build the dev buffer (first 0x60 bytes of NPD header in big-endian).
	memcpy(dev, npd, 0x60);

	// Fix endianness.
	int version = swap32(npd->version);
	int license = swap32(npd->license);
	int type = swap32(npd->type);
	memcpy(dev + 0x4, &version, 4);
	memcpy(dev + 0x8, &license, 4);
	memcpy(dev + 0xC, &type, 4);

	// Check for an empty dev_hash (can't validate if devklic is NULL);
	bool isDevklicEmpty = true;
	for (int i = 0; i < 0x10; i++)
	{
		if (klicensee[i] != 0)
		{
			isDevklicEmpty = false;
			break;
		}
	}

	if (isDevklicEmpty)
	{
		// Allow empty dev hash.
		return 1;
	}
	else
	{
		// Generate klicensee xor key.
		xor_key(key, klicensee, NP_OMAC_KEY_2);

		// Hash with generated key and compare with dev_hash.
		return cmac_hash_compare(key, 0x10, dev, 0x60, npd->dev_hash, 0x10);
	}
}

int validate_npd_hashes(const char* file_name, const u8* klicensee, NPD_HEADER *npd, bool verbose)
{
	int title_hash_result = 0;
	int dev_hash_result = 0;

	const auto file_name_length = std::strlen(file_name);
	std::unique_ptr<u8[]> buf(new u8[0x30 + file_name_length]);

	// Build the title buffer (content_id + file_name).
	memcpy(buf.get(), npd->content_id, 0x30);
	memcpy(buf.get() + 0x30, file_name, file_name_length);

	// Hash with NPDRM_OMAC_KEY_3 and compare with title_hash.
	title_hash_result = cmac_hash_compare(NP_OMAC_KEY_3, 0x10, buf.get(), 0x30 + file_name_length, npd->title_hash, 0x10);

	if (verbose)
	{
		if (title_hash_result)
			edat_log.notice("EDAT: NPD title hash is valid!");
		else
			edat_log.warning("EDAT: NPD title hash is invalid!");
	}


	dev_hash_result = validate_dev_klic(klicensee, npd);

	return (title_hash_result && dev_hash_result);
}

void read_npd_edat_header(const fs::file* input, NPD_HEADER& NPD, EDAT_HEADER& EDAT)
{
	char npd_header[0x80];
	char edat_header[0x10];
	input->read(npd_header, sizeof(npd_header));
	input->read(edat_header, sizeof(edat_header));

	memcpy(&NPD.magic, npd_header, 4);
	NPD.version = swap32(*reinterpret_cast<s32*>(&npd_header[4]));
	NPD.license = swap32(*reinterpret_cast<s32*>(&npd_header[8]));
	NPD.type = swap32(*reinterpret_cast<s32*>(&npd_header[12]));
	memcpy(NPD.content_id, &npd_header[16], 0x30);
	memcpy(NPD.digest, &npd_header[64], 0x10);
	memcpy(NPD.title_hash, &npd_header[80], 0x10);
	memcpy(NPD.dev_hash, &npd_header[96], 0x10);
	NPD.unk1 = swap64(*reinterpret_cast<u64*>(&npd_header[112]));
	NPD.unk2 = swap64(*reinterpret_cast<u64*>(&npd_header[120]));

	EDAT.flags = swap32(*reinterpret_cast<s32*>(&edat_header[0]));
	EDAT.block_size = swap32(*reinterpret_cast<s32*>(&edat_header[4]));
	EDAT.file_size = swap64(*reinterpret_cast<u64*>(&edat_header[8]));
}

bool extract_all_data(const fs::file* input, const fs::file* output, const char* input_file_name, unsigned char* devklic, unsigned char* rifkey, bool verbose)
{
	// Setup NPD and EDAT/SDAT structs.
	NPD_HEADER NPD;
	EDAT_HEADER EDAT;

	// Read in the NPD and EDAT/SDAT headers.
	read_npd_edat_header(input, NPD, EDAT);

	unsigned char npd_magic[4] = {0x4E, 0x50, 0x44, 0x00};  //NPD0
	if (memcmp(&NPD.magic, npd_magic, 4))
	{
		edat_log.error("EDAT: %s has invalid NPD header or already decrypted.", input_file_name);
		return 1;
	}

	if (verbose)
	{
		edat_log.notice("NPD HEADER");
		edat_log.notice("NPD version: %d", NPD.version);
		edat_log.notice("NPD license: %d", NPD.license);
		edat_log.notice("NPD type: %d", NPD.type);
	}

	// Set decryption key.
	u8 key[0x10] = { 0 };

	// Check EDAT/SDAT flag.
	if ((EDAT.flags & SDAT_FLAG) == SDAT_FLAG)
	{
		if (verbose)
		{
			edat_log.notice("SDAT HEADER");
			edat_log.notice("SDAT flags: 0x%08X", EDAT.flags);
			edat_log.notice("SDAT block size: 0x%08X", EDAT.block_size);
			edat_log.notice("SDAT file size: 0x%08X", EDAT.file_size);
		}

		// Generate SDAT key.
		xor_key(key, NPD.dev_hash, SDAT_KEY);
	}
	else
	{
		if (verbose)
		{
			edat_log.notice("EDAT HEADER");
			edat_log.notice("EDAT flags: 0x%08X", EDAT.flags);
			edat_log.notice("EDAT block size: 0x%08X", EDAT.block_size);
			edat_log.notice("EDAT file size: 0x%08X", EDAT.file_size);
		}

		// Perform header validation (EDAT only).
		char real_file_name[MAX_PATH];
		extract_file_name(input_file_name, real_file_name);
		if (!validate_npd_hashes(real_file_name, devklic, &NPD, verbose))
		{
			// Ignore header validation in DEBUG data.
			if ((EDAT.flags & EDAT_DEBUG_DATA_FLAG) != EDAT_DEBUG_DATA_FLAG)
			{
				edat_log.error("EDAT: NPD hash validation failed!");
				return 1;
			}
		}

		// Select EDAT key.
		if ((NPD.license & 0x3) == 0x3)           // Type 3: Use supplied devklic.
			memcpy(key, devklic, 0x10);
		else if ((NPD.license & 0x2) == 0x2)      // Type 2: Use key from RAP file (RIF key).
		{
			memcpy(key, rifkey, 0x10);

			// Make sure we don't have an empty RIF key.
			int i, test = 0;
			for (i = 0; i < 0x10; i++)
			{
				if (key[i] != 0)
				{
					test = 1;
					break;
				}
			}

			if (!test)
			{
				edat_log.error("EDAT: A valid RAP file is needed for this EDAT file!");
				return 1;
			}
		}
		else if ((NPD.license & 0x1) == 0x1)      // Type 1: Use network activation.
		{
			edat_log.error("EDAT: Network license not supported!");
			return 1;
		}

		if (verbose)
		{
			int i;
			edat_log.notice("DEVKLIC: ");
			for (i = 0; i < 0x10; i++)
				edat_log.notice("%02X", devklic[i]);

			edat_log.notice("RIF KEY: ");
			for (i = 0; i < 0x10; i++)
				edat_log.notice("%02X", rifkey[i]);
		}
	}

	if (verbose)
	{
		int i;
		edat_log.notice("DECRYPTION KEY: ");
		for (i = 0; i < 0x10; i++)
			edat_log.notice("%02X", key[i]);
	}

	input->seek(0);
	if (check_data(key, &EDAT, &NPD, input, verbose))
	{
		edat_log.error("EDAT: Data parsing failed!");
		return 1;
	}

	input->seek(0);
	if (decrypt_data(input, output, &EDAT, &NPD, key, verbose))
	{
		edat_log.error("EDAT: Data decryption failed!");
		return 1;
	}

	return 0;
}

std::array<u8, 0x10> GetEdatRifKeyFromRapFile(const fs::file& rap_file)
{
	std::array<u8, 0x10> rapkey{ 0 };
	std::array<u8, 0x10> rifkey{ 0 };

	rap_file.read<std::array<u8, 0x10>>(rapkey);

	rap_to_rif(rapkey.data(), rifkey.data());

	return rifkey;
}

bool VerifyEDATHeaderWithKLicense(const fs::file& input, const std::string& input_file_name, const std::array<u8, 0x10>& custom_klic, std::string* contentID)
{
	// Setup NPD and EDAT/SDAT structs.
	NPD_HEADER NPD;
	EDAT_HEADER EDAT;

	// Read in the NPD and EDAT/SDAT headers.
	read_npd_edat_header(&input, NPD, EDAT);

	unsigned char npd_magic[4] = { 0x4E, 0x50, 0x44, 0x00 };  //NPD0
	if (memcmp(&NPD.magic, npd_magic, 4))
	{
		edat_log.error("EDAT: %s has invalid NPD header or already decrypted.", input_file_name);
		return false;
	}

	if ((EDAT.flags & SDAT_FLAG) == SDAT_FLAG)
	{
		edat_log.error("EDAT: SDATA file given to edat function");
		return false;
	}

	// Perform header validation (EDAT only).
	char real_file_name[MAX_PATH];
	extract_file_name(input_file_name.c_str(), real_file_name);
	if (!validate_npd_hashes(real_file_name, custom_klic.data(), &NPD, false))
	{
		// Ignore header validation in DEBUG data.
		if ((EDAT.flags & EDAT_DEBUG_DATA_FLAG) != EDAT_DEBUG_DATA_FLAG)
		{
			edat_log.error("EDAT: NPD hash validation failed!");
			return false;
		}
	}

	*contentID = std::string(reinterpret_cast<const char*>(NPD.content_id));
	return true;
}

// Decrypts full file
fs::file DecryptEDAT(const fs::file& input, const std::string& input_file_name, int mode, const std::string& rap_file_name, u8 *custom_klic, bool verbose)
{
	// Prepare the files.
	input.seek(0);

	// Set keys (RIF and DEVKLIC).
	std::array<u8, 0x10> rifKey{ 0 };
	unsigned char devklic[0x10] = { 0 };

	// Select the EDAT key mode.
	switch (mode)
	{
	case 0:
		break;
	case 1:
		memcpy(devklic, NP_KLIC_FREE, 0x10);
		break;
	case 2:
		memcpy(devklic, NP_OMAC_KEY_2, 0x10);
		break;
	case 3:
		memcpy(devklic, NP_OMAC_KEY_3, 0x10);
		break;
	case 4:
		memcpy(devklic, NP_KLIC_KEY, 0x10);
		break;
	case 5:
		memcpy(devklic, NP_PSX_KEY, 0x10);
		break;
	case 6:
		memcpy(devklic, NP_PSP_KEY_1, 0x10);
		break;
	case 7:
		memcpy(devklic, NP_PSP_KEY_2, 0x10);
		break;
	case 8:
		{
			if (custom_klic != NULL)
				memcpy(devklic, custom_klic, 0x10);
			else
			{
				edat_log.error("EDAT: Invalid custom klic!");
				return fs::file{};
			}
			break;
		}
	default:
		edat_log.error("EDAT: Invalid mode!");
		return fs::file{};
	}

	// Read the RAP file, if provided.
	if (rap_file_name.size())
	{
		fs::file rap(rap_file_name);

		rifKey = GetEdatRifKeyFromRapFile(rap);
	}

	// Delete the bad output file if any errors arise.
	fs::file output = fs::make_stream<std::vector<u8>>();
	if (extract_all_data(&input, &output, input_file_name.c_str(), devklic, rifKey.data(), verbose))
	{
		output.release();
		return fs::file{};
	}

	output.seek(0);
	return output;
}

bool EDATADecrypter::ReadHeader()
{
	edata_file.seek(0);
	// Read in the NPD and EDAT/SDAT headers.
	read_npd_edat_header(&edata_file, npdHeader, edatHeader);

	unsigned char npd_magic[4] = { 0x4E, 0x50, 0x44, 0x00 };  //NPD0
	if (memcmp(&npdHeader.magic, npd_magic, 4))
	{
		return false;
	}

	// Check for SDAT flag.
	if ((edatHeader.flags & SDAT_FLAG) == SDAT_FLAG)
	{
		// Generate SDAT key.
		xor_key(dec_key.data(), npdHeader.dev_hash, SDAT_KEY);
	}
	else
	{
		// verify key
		if (validate_dev_klic(dev_key.data(), &npdHeader) == 0)
		{
			edat_log.error("EDAT: Failed validating klic");
			return false;
		}

		// Select EDAT key.
		if ((npdHeader.license & 0x3) == 0x3)           // Type 3: Use supplied devklic.
			dec_key = std::move(dev_key);
		else if ((npdHeader.license & 0x2) == 0x2)      // Type 2: Use key from RAP file (RIF key).
		{
			dec_key = std::move(rif_key);

			if (dec_key == std::array<u8, 0x10>{0})
			{
				edat_log.warning("EDAT: Empty Dec key!");
			}
		}
		else if ((npdHeader.license & 0x1) == 0x1)      // Type 1: Use network activation.
		{
			edat_log.error("EDAT: Network license not supported!");
			return false;
		}
	}

	edata_file.seek(0);

	// k the ecdsa_verify function in this check_data function takes a ridiculous amount of time
	// like it slows down load time by a factor of x20, at least, so its ignored for now

	/*if (check_data(dec_key.data(), &edatHeader, &npdHeader, &sdata_file, false))
	{
		return false;
	}*/

	file_size = edatHeader.file_size;
	total_blocks = static_cast<u32>((edatHeader.file_size + edatHeader.block_size - 1) / edatHeader.block_size);

	return true;
}

u64 EDATADecrypter::ReadData(u64 pos, u8* data, u64 size)
{
	if (pos > edatHeader.file_size)
		return 0;

	// now we need to offset things to account for the actual 'range' requested
	const u64 startOffset = pos % edatHeader.block_size;

	const u32 num_blocks = static_cast<u32>(std::ceil((startOffset + size) / (0. + edatHeader.block_size)));
	const u64 bufSize = num_blocks*edatHeader.block_size;
	if (data_buf_size < (bufSize))
	{
		data_buf.reset(new u8[bufSize]);
		data_buf_size = bufSize;
	}

	// find and decrypt block range covering pos + size
	const u32 starting_block = static_cast<u32>(pos / edatHeader.block_size);
	const u32 ending_block = std::min(starting_block + num_blocks, total_blocks);
	u64 writeOffset = 0;
	for (u32 i = starting_block; i < ending_block; ++i)
	{
		edata_file.seek(0);
		u64 res = decrypt_block(&edata_file, &data_buf[writeOffset], &edatHeader, &npdHeader, dec_key.data(), i, total_blocks, edatHeader.file_size);
		if (res == -1)
		{
			edat_log.error("Error Decrypting data");
			return 0;
		}
		writeOffset += res;
	}

	const u64 bytesWrote = std::min<u64>(writeOffset - startOffset, size);

	memcpy(data, &data_buf[startOffset], bytesWrote);
	return bytesWrote;
}
