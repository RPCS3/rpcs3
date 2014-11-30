#include "stdafx.h"
#include "key_vault.h"
#include "unedat.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"

void generate_key(int crypto_mode, int version, unsigned char *key_final, unsigned char *iv_final, unsigned char *key, unsigned char *iv)
{
	int mode = (int)(crypto_mode & 0xF0000000);
	switch (mode) {
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
	int mode = (int)(hash_mode & 0xF0000000);
	switch (mode) {
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
		LOG_ERROR(LOADER, "EDAT: Unknown crypto algorithm!");
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
		LOG_ERROR(LOADER, "EDAT: Unknown hashing algorithm!");
		return false;
	}
}

// EDAT/SDAT functions.
unsigned char* dec_section(unsigned char* metadata)
{
	unsigned char *dec = new unsigned char[0x10];
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
	return dec;
}

unsigned char* get_block_key(int block, NPD_HEADER *npd)
{
	unsigned char empty_key[0x10] = {};
	unsigned char *src_key = (npd->version <= 1) ? empty_key : npd->dev_hash;
	unsigned char *dest_key = new unsigned char[0x10];
	memcpy(dest_key, src_key, 0xC);
	dest_key[0xC] = (block >> 24 & 0xFF);
	dest_key[0xD] = (block >> 16 & 0xFF);
	dest_key[0xE] = (block >> 8 & 0xFF);
	dest_key[0xF] = (block & 0xFF);
	return dest_key;
}

// EDAT/SDAT decryption.
int decrypt_data(rFile *in, rFile *out, EDAT_HEADER *edat, NPD_HEADER *npd, unsigned char* crypt_key, bool verbose)
{
	// Get metadata info and setup buffers.
	int block_num = (int)((edat->file_size + edat->block_size - 1) / edat->block_size);
	int metadata_section_size = ((edat->flags & EDAT_COMPRESSED_FLAG) != 0 || (edat->flags & EDAT_FLAG_0x20) != 0) ? 0x20 : 0x10;
	int metadata_offset = 0x100;

	unsigned char *enc_data;
	unsigned char *dec_data;
	unsigned char *b_key;
	unsigned char *iv;

	unsigned char hash[0x10];
	unsigned char key_result[0x10];
	unsigned char hash_result[0x14];
	memset(hash, 0, 0x10);
	memset(key_result, 0, 0x10);
	memset(hash_result, 0, 0x14);

	unsigned long long offset = 0;
	unsigned long long metadata_sec_offset = 0;
	int length = 0;
	int compression_end = 0;
	unsigned char empty_iv[0x10] = {};

	// Decrypt the metadata.
	int i;
	for (i = 0; i < block_num; i++)
	{
		memset(hash_result, 0, 0x14);

		if ((edat->flags & EDAT_COMPRESSED_FLAG) != 0)
		{
			metadata_sec_offset = metadata_offset + (unsigned long long) i * metadata_section_size;
			in->Seek(metadata_sec_offset);

			unsigned char metadata[0x20];
			memset(metadata, 0, 0x20);
			in->Read(metadata, 0x20);

			// If the data is compressed, decrypt the metadata.
			// NOTE: For NPD version 1 the metadata is not encrypted.
			if (npd->version <= 1)
			{
				offset = swap64(*(unsigned long long*)&metadata[0x10]);
				length = swap32(*(int*)&metadata[0x18]);
				compression_end = swap32(*(int*)&metadata[0x1C]);
			}
			else
			{
				unsigned char *result = dec_section(metadata);
				offset = swap64(*(unsigned long long*)&result[0]);
				length = swap32(*(int*)&result[8]);
				compression_end = swap32(*(int*)&result[12]);
				delete[] result;
			}
			
			memcpy(hash_result, metadata, 0x10);
		}
		else if ((edat->flags & EDAT_FLAG_0x20) != 0)
		{
			// If FLAG 0x20, the metadata precedes each data block.
			metadata_sec_offset = metadata_offset + (unsigned long long) i * (metadata_section_size + length);
			in->Seek(metadata_sec_offset);

			unsigned char metadata[0x20];
			memset(metadata, 0, 0x20);
			in->Read(metadata, 0x20);
			memcpy(hash_result, metadata, 0x14);

			// If FLAG 0x20 is set, apply custom xor.
			int j;
			for (j = 0; j < 0x10; j++)
				hash_result[j] = (unsigned char)(metadata[j] ^ metadata[j + 0x10]);

			offset = metadata_sec_offset + 0x20;
			length = edat->block_size;

			if ((i == (block_num - 1)) && (edat->file_size % edat->block_size))
				length = (int)(edat->file_size % edat->block_size);
		}
		else
		{
			metadata_sec_offset = metadata_offset + (unsigned long long) i * metadata_section_size;
			in->Seek(metadata_sec_offset);

			in->Read(hash_result, 0x10);
			offset = metadata_offset + (unsigned long long) i * edat->block_size + (unsigned long long) block_num * metadata_section_size;
			length = edat->block_size;

			if ((i == (block_num - 1)) && (edat->file_size % edat->block_size))
				length = (int)(edat->file_size % edat->block_size);
		}

		// Locate the real data.
		int pad_length = length;
		length = (int)((pad_length + 0xF) & 0xFFFFFFF0);

		// Setup buffers for decryption and read the data.
		enc_data = new unsigned char[length];
		dec_data = new unsigned char[length];
		memset(enc_data, 0, length);
		memset(dec_data, 0, length);
		memset(hash, 0, 0x10);
		memset(key_result, 0, 0x10);

		in->Seek(offset);
		in->Read(enc_data, length);

		// Generate a key for the current block.
		b_key = get_block_key(i, npd);

		// Encrypt the block key with the crypto key.
		aesecb128_encrypt(crypt_key, b_key, key_result);
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
			memcpy(dec_data, enc_data, length);
		}
		else
		{
			// IV is null if NPD version is 1 or 0.
			iv = (npd->version <= 1) ? empty_iv : npd->digest;
			// Call main crypto routine on this data block.
			if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), enc_data, dec_data, length, key_result, iv, hash, hash_result))
			{
				if (verbose)
					LOG_WARNING(LOADER, "EDAT: Block at offset 0x%llx has invalid hash!", offset);
					
				return 1;
			}
		}

		// Apply additional compression if needed and write the decrypted data.
		if (((edat->flags & EDAT_COMPRESSED_FLAG) != 0) && compression_end)
		{
			int decomp_size = (int)edat->file_size;
			unsigned char *decomp_data = new unsigned char[decomp_size];
			memset(decomp_data, 0, decomp_size);

			if (verbose)
				LOG_NOTICE(LOADER, "EDAT: Decompressing data...");

			int res = decompress(decomp_data, dec_data, decomp_size);
			out->Write(decomp_data, res);

			if (verbose)
			{
				LOG_NOTICE(LOADER, "EDAT: Compressed block size: %d", pad_length);
				LOG_NOTICE(LOADER, "EDAT: Decompressed block size: %d", res);
			}

			edat->file_size -= res;

			if (edat->file_size == 0)
			{
				if (res < 0)
				{
					LOG_ERROR(LOADER, "EDAT: Decompression failed!");
					return 1;
				}
				else
					LOG_NOTICE(LOADER, "EDAT: Successfully decompressed!");
			}

			delete[] decomp_data;
		}
		else
		{
			out->Write(dec_data, pad_length);
		}

		delete[] enc_data;
		delete[] dec_data;
	}

	return 0;
}

int check_data(unsigned char *key, EDAT_HEADER *edat, NPD_HEADER *npd, rFile *f, bool verbose)
{
	f->Seek(0);
	unsigned char header[0xA0];
	unsigned char empty_header[0xA0];
	unsigned char header_hash[0x10];
	unsigned char metadata_hash[0x10];
	memset(header, 0, 0xA0);
	memset(empty_header, 0, 0xA0);
	memset(header_hash, 0, 0x10);
	memset(metadata_hash, 0, 0x10);

	// Check NPD version and flags.
	if ((npd->version == 0) || (npd->version == 1))
	{
		if (edat->flags & 0x7EFFFFFE)
		{
			LOG_ERROR(LOADER, "EDAT: Bad header flags!");
			return 1;
		}
	}
	else if (npd->version == 2)
	{
		if (edat->flags & 0x7EFFFFE0)
		{
			LOG_ERROR(LOADER, "EDAT: Bad header flags!");
			return 1;
		}
	}
	else if ((npd->version == 3) || (npd->version == 4))
	{
		if (edat->flags & 0x7EFFFFC0)
		{
			LOG_ERROR(LOADER, "EDAT: Bad header flags!");
			return 1;
		}
	}
	else
	{
		LOG_ERROR(LOADER, "EDAT: Unknown version!");
		return 1;
	}

	// Read in the file header.
	f->Read(header, 0xA0);

	// Read in the header and metadata section hashes.
	f->Seek(0x90);
	f->Read(metadata_hash, 0x10);
	f->Read(header_hash, 0x10);

	// Setup the hashing mode and the crypto mode used in the file.
	int crypto_mode = 0x1;
	int hash_mode = ((edat->flags & EDAT_ENCRYPTED_KEY_FLAG) == 0) ? 0x00000002 : 0x10000002;
	if ((edat->flags & EDAT_DEBUG_DATA_FLAG) != 0)
	{
		hash_mode |= 0x01000000;

		if (verbose)
			LOG_WARNING(LOADER, "EDAT: DEBUG data detected!");
	}

	// Setup header key and iv buffers.
	unsigned char header_key[0x10];
	unsigned char header_iv[0x10];
	memset(header_key, 0, 0x10);
	memset(header_iv, 0, 0x10);

	// Test the header hash (located at offset 0xA0).
	if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), header, empty_header, 0xA0, header_key, header_iv, key, header_hash))
	{
		if (verbose)
			LOG_WARNING(LOADER, "EDAT: Header hash is invalid!");

		// If the header hash test fails and the data is not DEBUG, then RAP/RIF/KLIC key is invalid.
		if ((edat->flags & EDAT_DEBUG_DATA_FLAG) != EDAT_DEBUG_DATA_FLAG)
		{
			LOG_ERROR(LOADER, "EDAT: RAP/RIF/KLIC key is invalid!");
			return 1;
		}
	}

	// Parse the metadata info.
	int metadata_section_size = ((edat->flags & EDAT_COMPRESSED_FLAG) != 0 || (edat->flags & EDAT_FLAG_0x20) != 0) ? 0x20 : 0x10;
	if (((edat->flags & EDAT_COMPRESSED_FLAG) != 0))
	{
		if (verbose)
			LOG_WARNING(LOADER, "EDAT: COMPRESSED data detected!");
	}

	int block_num = (int)((edat->file_size + edat->block_size - 1) / edat->block_size);
	int metadata_offset = 0x100;
	int metadata_size = metadata_section_size * block_num;
	long long metadata_section_offset = metadata_offset;

	long bytes_read = 0;
	long bytes_to_read = metadata_size;
	unsigned char *metadata = new unsigned char[metadata_size];
	unsigned char *empty_metadata = new unsigned char[metadata_size];

	while (bytes_to_read > 0)
	{
		// Locate the metadata blocks.
		f->Seek(metadata_section_offset);

		// Read in the metadata.
		f->Read(metadata + bytes_read, metadata_section_size);

		// Adjust sizes.
		bytes_read += metadata_section_size;
		bytes_to_read -= metadata_section_size;

		if (((edat->flags & EDAT_FLAG_0x20) != 0)) // Metadata block before each data block.
			metadata_section_offset += (metadata_section_size + edat->block_size);
		else
			metadata_section_offset += metadata_section_size;
	}

	// Test the metadata section hash (located at offset 0x90).
	if (!decrypt(hash_mode, crypto_mode, (npd->version == 4), metadata, empty_metadata, metadata_size, header_key, header_iv, key, metadata_hash))
	{
		if (verbose)
			LOG_WARNING(LOADER, "EDAT: Metadata section hash is invalid!");
	}

	// Checking ECDSA signatures.
	if ((edat->flags & EDAT_DEBUG_DATA_FLAG) == 0)
	{
		LOG_NOTICE(LOADER, "EDAT: Checking signatures...");

		// Setup buffers.
		unsigned char metadata_signature[0x28];
		unsigned char header_signature[0x28];
		unsigned char signature_hash[20];
		unsigned char signature_r[0x15];
		unsigned char signature_s[0x15];
		unsigned char zero_buf[0x15];
		memset(metadata_signature, 0, 0x28);
		memset(header_signature, 0, 0x28);
		memset(signature_hash, 0, 20);
		memset(signature_r, 0, 0x15);
		memset(signature_s, 0, 0x15);
		memset(zero_buf, 0, 0x15);

		// Setup ECDSA curve and public key.
		ecdsa_set_curve(VSH_CURVE_P, VSH_CURVE_A, VSH_CURVE_B, VSH_CURVE_N, VSH_CURVE_GX, VSH_CURVE_GY);
		ecdsa_set_pub(VSH_PUB);


		// Read in the metadata and header signatures.
		f->Seek(0xB0);
		f->Read(metadata_signature, 0x28);
		f->Seek(0xD8);
		f->Read(header_signature, 0x28);

		// Checking metadata signature.
		// Setup signature r and s.
		memcpy(signature_r + 01, metadata_signature, 0x14);
		memcpy(signature_s + 01, metadata_signature + 0x14, 0x14);
		if ((!memcmp(signature_r, zero_buf, 0x15)) || (!memcmp(signature_s, zero_buf, 0x15)))
			LOG_WARNING(LOADER, "EDAT: Metadata signature is invalid!");
		else
		{
			// Setup signature hash.
			if ((edat->flags & EDAT_FLAG_0x20) != 0) //Sony failed again, they used buffer from 0x100 with half size of real metadata.
			{
				int metadata_buf_size = block_num * 0x10;
				unsigned char *metadata_buf = new unsigned char[metadata_buf_size];
				f->Seek(metadata_offset);
				f->Read(metadata_buf, metadata_buf_size);
				sha1(metadata_buf, metadata_buf_size, signature_hash);
				delete[] metadata_buf;
			}
			else
				sha1(metadata, metadata_size, signature_hash);

			if (!ecdsa_verify(signature_hash, signature_r, signature_s))
			{
				LOG_WARNING(LOADER, "EDAT: Metadata signature is invalid!");
				if (((unsigned long long)edat->block_size * block_num) > 0x100000000)
					LOG_WARNING(LOADER, "EDAT: *Due to large file size, metadata signature status may be incorrect!");
			}
			else
				LOG_NOTICE(LOADER, "EDAT: Metadata signature is valid!");
		}


		// Checking header signature.
		// Setup header signature r and s.
		memset(signature_r, 0, 0x15);
		memset(signature_s, 0, 0x15);
		memcpy(signature_r + 01, header_signature, 0x14);
		memcpy(signature_s + 01, header_signature + 0x14, 0x14);

		if ((!memcmp(signature_r, zero_buf, 0x15)) || (!memcmp(signature_s, zero_buf, 0x15)))
			LOG_WARNING(LOADER, "EDAT: Header signature is invalid!");
		else
		{
			// Setup header signature hash.
			memset(signature_hash, 0, 20);
			unsigned char *header_buf = new unsigned char[0xD8];
			f->Seek(0x00);
			f->Read(header_buf, 0xD8);
			sha1(header_buf, 0xD8, signature_hash );
			delete[] header_buf;

			if (ecdsa_verify(signature_hash, signature_r, signature_s))
				LOG_NOTICE(LOADER, "EDAT: Header signature is valid!");
			else
				LOG_WARNING(LOADER, "EDAT: Header signature is invalid!");
		}
	}

	// Cleanup.
	delete[] metadata;
	delete[] empty_metadata;

	return 0;
}

int validate_npd_hashes(const char* file_name, unsigned char *klicensee, NPD_HEADER *npd, bool verbose)
{
	int title_hash_result = 0;
	int dev_hash_result = 0;

	int file_name_length = (int) strlen(file_name);
	unsigned char *buf = new unsigned char[0x30 + file_name_length];
	unsigned char dev[0x60];
	unsigned char key[0x10];
	memset(dev, 0, 0x60);
	memset(key, 0, 0x10);

	// Build the title buffer (content_id + file_name).
	memcpy(buf, npd->content_id, 0x30);
	memcpy(buf + 0x30, file_name, file_name_length);

	// Build the dev buffer (first 0x60 bytes of NPD header in big-endian).
	memcpy(dev, npd, 0x60);

	// Fix endianness.
	int version = swap32(npd->version);
	int license = swap32(npd->license);
	int type = swap32(npd->type);
	memcpy(dev + 0x4, &version, 4);
	memcpy(dev + 0x8, &license, 4);
	memcpy(dev + 0xC, &type, 4);

	// Hash with NPDRM_OMAC_KEY_3 and compare with title_hash.
	title_hash_result = cmac_hash_compare(NP_OMAC_KEY_3, 0x10, buf, 0x30 + file_name_length, npd->title_hash, 0x10);

	if (verbose)
	{
		if (title_hash_result)
			LOG_NOTICE(LOADER, "EDAT: NPD title hash is valid!");
		else
			LOG_WARNING(LOADER, "EDAT: NPD title hash is invalid!");
	}

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
		if (verbose)
			LOG_WARNING(LOADER, "EDAT: NPD dev hash is empty!");

		// Allow empty dev hash.
		dev_hash_result = 1;
	}
	else
	{
		// Generate klicensee xor key.
		xor_key(key, klicensee, NP_OMAC_KEY_2, 0x10);

		// Hash with generated key and compare with dev_hash.
		dev_hash_result = cmac_hash_compare(key, 0x10, dev, 0x60, npd->dev_hash, 0x10);

		if (verbose)
		{
			if (dev_hash_result)
				LOG_NOTICE(LOADER, "EDAT: NPD dev hash is valid!");
			else
				LOG_WARNING(LOADER, "EDAT: NPD dev hash is invalid!");
		}
	}

	delete[] buf;

	return (title_hash_result && dev_hash_result);
}

bool extract_data(rFile *input, rFile *output, const char* input_file_name, unsigned char* devklic, unsigned char* rifkey, bool verbose)
{
	// Setup NPD and EDAT/SDAT structs.
	NPD_HEADER *NPD = new NPD_HEADER();
	EDAT_HEADER *EDAT = new EDAT_HEADER();

	// Read in the NPD and EDAT/SDAT headers.
	char npd_header[0x80];
	char edat_header[0x10];
	input->Read(npd_header, sizeof(npd_header));
	input->Read(edat_header, sizeof(edat_header));

	memcpy(NPD->magic, npd_header, 4);
	NPD->version = swap32(*(int*)&npd_header[4]);
	NPD->license = swap32(*(int*)&npd_header[8]);
	NPD->type = swap32(*(int*)&npd_header[12]);
	memcpy(NPD->content_id, (unsigned char*)&npd_header[16], 0x30);
	memcpy(NPD->digest, (unsigned char*)&npd_header[64], 0x10);
	memcpy(NPD->title_hash, (unsigned char*)&npd_header[80], 0x10);
	memcpy(NPD->dev_hash, (unsigned char*)&npd_header[96], 0x10);
	NPD->unk1 = swap64(*(u64*)&npd_header[112]);
	NPD->unk2 = swap64(*(u64*)&npd_header[120]);

	unsigned char npd_magic[4] = {0x4E, 0x50, 0x44, 0x00};  //NPD0
	if (memcmp(NPD->magic, npd_magic, 4))
	{
		LOG_ERROR(LOADER, "EDAT: %s has invalid NPD header or already decrypted.", input_file_name);
		delete NPD;
		delete EDAT;
		return 1;
	}

	EDAT->flags = swap32(*(int*)&edat_header[0]);
	EDAT->block_size = swap32(*(int*)&edat_header[4]);
	EDAT->file_size = swap64(*(u64*)&edat_header[8]);

	if (verbose)
	{
		LOG_NOTICE(LOADER, "NPD HEADER");
		LOG_NOTICE(LOADER, "NPD version: %d", NPD->version);
		LOG_NOTICE(LOADER, "NPD license: %d", NPD->license);
		LOG_NOTICE(LOADER, "NPD type: %d", NPD->type);
	}

	// Set decryption key.
	unsigned char key[0x10];
	memset(key, 0, 0x10);

	// Check EDAT/SDAT flag.
	if ((EDAT->flags & SDAT_FLAG) == SDAT_FLAG)
	{
		if (verbose)
		{
			LOG_NOTICE(LOADER, "SDAT HEADER");
			LOG_NOTICE(LOADER, "SDAT flags: 0x%08X", EDAT->flags);
			LOG_NOTICE(LOADER, "SDAT block size: 0x%08X", EDAT->block_size);
			LOG_NOTICE(LOADER, "SDAT file size: 0x%08X", EDAT->file_size);
		}

		// Generate SDAT key.
		xor_key(key, NPD->dev_hash, SDAT_KEY, 0x10);
	}
	else
	{
		if (verbose)
		{
			LOG_NOTICE(LOADER, "EDAT HEADER");
			LOG_NOTICE(LOADER, "EDAT flags: 0x%08X", EDAT->flags);
			LOG_NOTICE(LOADER, "EDAT block size: 0x%08X", EDAT->block_size);
			LOG_NOTICE(LOADER, "EDAT file size: 0x%08X", EDAT->file_size);
		}

		// Perform header validation (EDAT only).
		char real_file_name[MAX_PATH];
		extract_file_name(input_file_name, real_file_name);
		if (!validate_npd_hashes(real_file_name, devklic, NPD, verbose))
		{
			// Ignore header validation in DEBUG data.
			if ((EDAT->flags & EDAT_DEBUG_DATA_FLAG) != EDAT_DEBUG_DATA_FLAG)
			{
				LOG_ERROR(LOADER, "EDAT: NPD hash validation failed!");
				delete NPD;
				delete EDAT;
				return 1;
			}
		}

		// Select EDAT key.
		if ((NPD->license & 0x3) == 0x3)           // Type 3: Use supplied devklic.
			memcpy(key, devklic, 0x10);
		else if ((NPD->license & 0x2) == 0x2)      // Type 2: Use key from RAP file (RIF key).
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
				LOG_ERROR(LOADER, "EDAT: A valid RAP file is needed for this EDAT file!");
				delete NPD;
				delete EDAT;
				return 1;
			}
		}
		else if ((NPD->license & 0x1) == 0x1)      // Type 1: Use network activation.
		{
			LOG_ERROR(LOADER, "EDAT: Network license not supported!");
			delete NPD;
			delete EDAT;
			return 1;
		}

		if (verbose)
		{
			int i;
			LOG_NOTICE(LOADER, "DEVKLIC: ");
			for (i = 0; i < 0x10; i++)
				LOG_NOTICE(LOADER, "%02X", devklic[i]);

			LOG_NOTICE(LOADER, "RIF KEY: ");
			for (i = 0; i < 0x10; i++)
				LOG_NOTICE(LOADER, "%02X", rifkey[i]);
		}
	}

	if (verbose)
	{
		int i;
		LOG_NOTICE(LOADER, "DECRYPTION KEY: ");
		for (i = 0; i < 0x10; i++)
			LOG_NOTICE(LOADER, "%02X", key[i]);
	}

	LOG_NOTICE(LOADER, "EDAT: Parsing data...");
	if (check_data(key, EDAT, NPD, input, verbose))
	{
		LOG_ERROR(LOADER, "EDAT: Data parsing failed!");
		delete NPD;
		delete EDAT;
		return 1;
	}
	else
		LOG_NOTICE(LOADER, "EDAT: Data successfully parsed!");

	LOG_NOTICE(LOADER, "EDAT: Decrypting data...");
	if (decrypt_data(input, output, EDAT, NPD, key, verbose))
	{
		LOG_ERROR(LOADER, "EDAT: Data decryption failed!");
		delete NPD;
		delete EDAT;
		return 1;
	}
	else
		LOG_NOTICE(LOADER, "EDAT: Data successfully decrypted!");

	delete NPD;
	delete EDAT;

	return 0;
}

int DecryptEDAT(const std::string& input_file_name, const std::string& output_file_name, int mode, const std::string& rap_file_name, unsigned char *custom_klic, bool verbose)
{
	// Prepare the files.
	rFile input(input_file_name.c_str());
	rFile output(output_file_name.c_str(), rFile::write);
	rFile rap(rap_file_name.c_str());

	// Set keys (RIF and DEVKLIC).
	unsigned char rifkey[0x10];
	unsigned char devklic[0x10];
	memset(rifkey, 0, 0x10);
	memset(devklic, 0, 0x10);

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
				LOG_ERROR(LOADER, "EDAT: Invalid custom klic!");
				return -1;
			}

			break;
		}
	default:
		LOG_ERROR(LOADER, "EDAT: Invalid mode!");
		return -1;
	}
	
	// Check the input/output files.
	if (!input.IsOpened() || !output.IsOpened())
	{
		LOG_ERROR(LOADER, "EDAT: Failed to open files!");
		return -1;
	}

	// Read the RAP file, if provided.
	if (rap.IsOpened())
	{
		unsigned char rapkey[0x10];
		memset(rapkey, 0, 0x10);
		
		rap.Read(rapkey, 0x10);
		
		rap_to_rif(rapkey, rifkey);
		
		rap.Close();
	}

	// Delete the bad output file if any errors arise.
	if (extract_data(&input, &output, input_file_name.c_str(), devklic, rifkey, verbose))
	{
		input.Close();
		output.Close();
		rRemoveFile(output_file_name);
		return -1;
	}
	
	// Cleanup.
	input.Close();
	output.Close();
	return 0;
}