#include "stdafx.h"
#include "unedat.h"

void generate_key(int crypto_mode, int version, unsigned char *key_final, unsigned char *iv_final, unsigned char *key, unsigned char *iv)
{
	int mode = (int) (crypto_mode & 0xF0000000);
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
	int mode = (int) (hash_mode & 0xF0000000);
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

bool crypto(int hash_mode, int crypto_mode, int version, unsigned char *in, unsigned char *out, int length, unsigned char *key, unsigned char *iv, unsigned char *hash, unsigned char *test_hash) 
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
		ConLog.Error("EDAT: Unknown crypto algorithm!\n");
		return false;
	}

	if ((hash_mode & 0xFF) == 0x01) // 0x14 SHA1-HMAC
	{
		return hmac_hash_compare(hash_final_14, 0x14, in, length, test_hash);
	}
	else if ((hash_mode & 0xFF) == 0x02)  // 0x10 AES-CMAC
	{
		return cmac_hash_compare(hash_final_10, 0x10, in, length, test_hash);
	}
	else if ((hash_mode & 0xFF) == 0x04) //0x10 SHA1-HMAC
	{
		return hmac_hash_compare(hash_final_10, 0x10, in, length, test_hash);
	}
	else
	{
		ConLog.Error("EDAT: Unknown hashing algorithm!\n");
		return false;
	}
}

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

// EDAT/SDAT functions.
int decrypt_data(wxFile *in, wxFile *out, EDAT_SDAT_HEADER *edat, NPD_HEADER *npd, unsigned char* crypt_key, bool verbose)
{
	// Get metadata info and setup buffers.
	int block_num = (int) ((edat->file_size + edat->block_size - 1) / edat->block_size);
	int metadata_section_size = ((edat->flags & EDAT_COMPRESSED_FLAG) != 0 || (edat->flags & EDAT_FLAG_0x20) != 0) ? 0x20 : 0x10;
	int metadata_offset = 0x100;

	unsigned char *enc_data;
	unsigned char *dec_data;
	unsigned char *b_key;
	unsigned char *iv;

	unsigned char empty_iv[0x10] = {};

	// Decrypt the metadata.
	for (int i = 0; i < block_num; i++)
	{
		in->Seek(metadata_offset + i * metadata_section_size);
		unsigned char hash_result[0x10];
		long offset;
		int length = 0;
		int compression_end = 0;

		if ((edat->flags & EDAT_COMPRESSED_FLAG) != 0)
		{
			unsigned char metadata[0x20];
			in->Read(metadata, 0x20);
			
			// If the data is compressed, decrypt the metadata.
			unsigned char *result = dec_section(metadata);
			offset = ((swap32(*(int*)&result[0]) << 4) | (swap32(*(int*)&result[4])));
			length = swap32(*(int*)&result[8]);
			compression_end = swap32(*(int*)&result[12]);
			delete[] result;

			memcpy(hash_result, metadata, 0x10);
		}
		else if ((edat->flags & EDAT_FLAG_0x20) != 0)
		{
			// If FLAG 0x20, the metadata precedes each data block.
			in->Seek(metadata_offset + i * metadata_section_size + length);

			unsigned char metadata[0x20];
			in->Read(metadata, 0x20);

			// If FLAG 0x20 is set, apply custom xor.
			for (int j = 0; j < 0x10; j++) {
				hash_result[j] = (unsigned char)(metadata[j] ^ metadata[j+0x10]);
			}

			offset = metadata_offset + i * edat->block_size + (i + 1) * metadata_section_size;
			length = edat->block_size;

			if ((i == (block_num - 1)) && (edat->file_size % edat->block_size))
				length = (int) (edat->file_size % edat->block_size);
		}
		else
		{
			in->Read(hash_result, 0x10);
			offset = metadata_offset + i * edat->block_size + block_num * metadata_section_size;
			length = edat->block_size;
			
			if ((i == (block_num - 1)) && (edat->file_size % edat->block_size))
				length = (int) (edat->file_size % edat->block_size);
		}

		// Locate the real data.
		int pad_length = length;
		length = (int) ((pad_length + 0xF) & 0xFFFFFFF0);
		in->Seek(offset);

		// Setup buffers for decryption and read the data.
		enc_data = new unsigned char[length];
		dec_data = new unsigned char[length];
		unsigned char key_result[0x10];
		unsigned char hash[0x10];
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
			crypto(hash_mode, crypto_mode, (npd->version == 4), enc_data, dec_data, length, key_result, iv, hash, hash_result);
		}

		// Apply additional compression if needed and write the decrypted data.
		if (((edat->flags & EDAT_COMPRESSED_FLAG) != 0) && compression_end)
		{
			int decomp_size = (int)edat->file_size;
			unsigned char *decomp_data = new unsigned char[decomp_size];
			memset(decomp_data, 0, decomp_size);

			if (verbose)
				ConLog.Write("EDAT: Decompressing...\n");
			
			int res = lz_decompress(decomp_data, dec_data, decomp_size);
			out->Write(decomp_data, res);
			
			if (verbose)
			{
				ConLog.Write("EDAT: Compressed block size: %d\n", pad_length);
				ConLog.Write("EDAT: Decompressed block size: %d\n", res);
			}

			edat->file_size -= res;

			if (edat->file_size == 0) 
			{
				if (res < 0)
				{
					ConLog.Error("EDAT: Decompression failed!\n");
					return 1;
				}
				else
					ConLog.Success("EDAT: Data successfully decompressed!\n");	
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

static bool check_flags(EDAT_SDAT_HEADER *edat, NPD_HEADER *npd)
{
	if (edat == nullptr || npd == nullptr)
		return false;

	if (npd->version == 0 || npd->version == 1)
	{
		if (edat->flags & 0x7EFFFFFE) 
		{
			ConLog.Error("EDAT: Bad header flags!\n");
			return false;
		}
	}
	else if (npd->version == 2) 
	{
		if (edat->flags & 0x7EFFFFE0) 
		{
			ConLog.Error("EDAT: Bad header flags!\n");
			return false;
		}
	}
	else if (npd->version == 3 || npd->version == 4)
	{
		if (edat->flags & 0x7EFFFFC0)
		{
			ConLog.Error("EDAT: Bad header flags!\n");
			return false;
		}
	}
	else if (npd->version > 4)
	{
		ConLog.Error("EDAT: Unknown version - %d\n", npd->version);
		return false;
	}

	return true;
}

int check_data(unsigned char *key, EDAT_SDAT_HEADER *edat, NPD_HEADER *npd, wxFile *f, bool verbose)
{
	f->Seek(0);
	unsigned char *header = new unsigned char[0xA0];
	unsigned char *tmp = new unsigned char[0xA0];
	unsigned char *hash_result = new unsigned char[0x10];

	// Check NPD version and EDAT flags.
	if (!check_flags(edat, npd))
	{
		delete[] header;
		delete[] tmp;
		delete[] hash_result;

		return 1;
	}

	// Read in the file header.
	f->Read(header, 0xA0);
	f->Read(hash_result, 0x10);

	// Setup the hashing mode and the crypto mode used in the file.
	int crypto_mode = 0x1;
	int hash_mode = ((edat->flags & EDAT_ENCRYPTED_KEY_FLAG) == 0) ? 0x00000002 : 0x10000002;
	if ((edat->flags & EDAT_DEBUG_DATA_FLAG) != 0)
	{
		ConLog.Warning("EDAT: DEBUG data detected!\n");
		hash_mode |= 0x01000000;
	}

	// Setup header key and iv buffers.
	unsigned char header_key[0x10] = {};
	unsigned char header_iv[0x10] = {};

	// Test the header hash (located at offset 0xA0).
	if (!crypto(hash_mode, crypto_mode, (npd->version == 4), header, tmp, 0xA0, header_key, header_iv, key, hash_result))
	{
		if (verbose)
			ConLog.Warning("EDAT: Header hash is invalid!\n");
	}

	// Parse the metadata info.
	int metadata_section_size = 0x10;
	if (((edat->flags & EDAT_COMPRESSED_FLAG) != 0))
	{
		ConLog.Warning("EDAT: COMPRESSED data detected!\n");
		metadata_section_size = 0x20;
	}

	int block_num = (int) ((edat->file_size + edat->block_size - 1) / edat->block_size);
	int bytes_read = 0;
	int metadata_offset = 0x100;

	long bytes_to_read = metadata_section_size * block_num;
	while (bytes_to_read > 0)
	{
		// Locate the metadata blocks.
		int block_size = (0x3C00 > bytes_to_read) ? (int) bytes_to_read : 0x3C00;  // 0x3C00 is the maximum block size.
		f->Seek(metadata_offset + bytes_read);
		unsigned char *data = new unsigned char[block_size];

		// Read in the metadata.
		tmp = new unsigned char[block_size];
		f->Read(data, block_size);

		// Check the generated hash against the metadata hash located at offset 0x90 in the header.
		memset(hash_result, 0, 0x10);
		f->Seek(0x90);
		f->Read(hash_result, 0x10);

		// Generate the hash for this block.
		if (!crypto(hash_mode, crypto_mode, (npd->version == 4), data, tmp, block_size, header_key, header_iv, key, hash_result))
		{
			if (verbose)
				ConLog.Warning("EDAT: Metadata hash from block 0x%08x is invalid!\n", metadata_offset + bytes_read);
		}

		// Adjust sizes.
		bytes_read += block_size;
		bytes_to_read -= block_size;

		delete[] data;
	}

	// Cleanup.
	delete[] header;
	delete[] tmp;
	delete[] hash_result;

	return 0;
}

void validate_data(const char* file_name, unsigned char *klicensee, NPD_HEADER *npd, bool verbose)
{
	int title_hash_result = 0;
	int dev_hash_result = 0;

	int file_name_length = strlen(file_name);
	unsigned char *buf = new unsigned char[0x30 + file_name_length];
	unsigned char key[0x10];

	// Build the buffer (content_id + file_name).
	memcpy(buf, npd->content_id, 0x30);
	memcpy(buf + 0x30, file_name, file_name_length);

	// Hash with NP_OMAC_KEY_3 and compare with title_hash.
	title_hash_result = cmac_hash_compare(NP_OMAC_KEY_3, 0x10, buf, 0x30 + file_name_length, npd->title_hash);

	if (verbose)
	{
		if (title_hash_result)
			ConLog.Success("EDAT: NPD title hash is valid!\n");
		else 
			ConLog.Warning("EDAT: NPD title hash is invalid!\n");
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
			ConLog.Warning("EDAT: NPD dev hash is empty!\n");
	}
	else
	{
		// Generate klicensee xor key.
		xor_(key, klicensee, NP_OMAC_KEY_2, 0x10);

		// Hash with generated key and compare with dev_hash.
		dev_hash_result = cmac_hash_compare(key, 0x10, (unsigned char *)npd, 0x60, npd->dev_hash);
	
		if (verbose)
		{
			if (dev_hash_result)
				ConLog.Success("EDAT: NPD dev hash is valid!\n");
			else 
				ConLog.Warning("EDAT: NPD dev hash is invalid!\n");
		}
	}

	delete[] buf;
}

bool extract_data(wxFile *input, wxFile *output, const char* input_file_name, unsigned char* devklic, unsigned char* rifkey, bool verbose)
{
	// Setup NPD and EDAT/SDAT structs.
	NPD_HEADER *NPD = new NPD_HEADER();
	EDAT_SDAT_HEADER *EDAT = new EDAT_SDAT_HEADER();

	// Read in the NPD and EDAT/SDAT headers.
	char npd_header[0x80];
	char edat_header[0x10];
	input->Read(npd_header, 0x80);
	input->Read(edat_header, 0x10);
	
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
	if(memcmp(NPD->magic, npd_magic, 4))
	{
		ConLog.Error("EDAT: File has invalid NPD header.");
		delete NPD;
		delete EDAT;
		return 1;
	}

	EDAT->flags = swap32(*(int*)&edat_header[0]);
	EDAT->block_size = swap32(*(int*)&edat_header[4]);
	EDAT->file_size = swap64(*(u64*)&edat_header[8]);

	if (verbose)
	{
		ConLog.Write("NPD HEADER\n");
		ConLog.Write("NPD version: %d\n", NPD->version);
		ConLog.Write("NPD license: %d\n", NPD->license);
		ConLog.Write("NPD type: %d\n", NPD->type);
		ConLog.Write("\n");
		ConLog.Write("EDAT HEADER\n");
		ConLog.Write("EDAT flags: 0x%08X\n", EDAT->flags);
		ConLog.Write("EDAT block size: 0x%08X\n", EDAT->block_size);
		ConLog.Write("EDAT file size: 0x%08X\n", EDAT->file_size);
		ConLog.Write("\n");
	}

	// Set decryption key.
	unsigned char key[0x10];
	memset(key, 0, 0x10);

	if((EDAT->flags & SDAT_FLAG) == SDAT_FLAG)
	{
		ConLog.Warning("EDAT: SDAT detected!\n");
		xor_(key, NPD->dev_hash, SDAT_KEY, 0x10);
	}
	else
	{
		// Perform header validation (optional step).
		validate_data(input_file_name, devklic, NPD, verbose);

		if ((NPD->license & 0x3) == 0x3)      // Type 3: Use supplied devklic.
			memcpy(key, devklic, 0x10);
		else if ((NPD->license & 0x2) == 0x2) // Type 2: Use key from RAP file (RIF key).
		{	
			memcpy(key, rifkey, 0x10);

			// Make sure we don't have an empty RIF key.
			int i, test = 0;
			for(i = 0; i < 0x10; i++)
			{
				if (key[i] != 0)
				{
					test = 1;
					break;
				}
			}
			
			if (!test)
			{
				ConLog.Error("EDAT: A valid RAP file is needed!");
				delete NPD;
				delete EDAT;
				return 1;
			}
		}
	}

	ConLog.Write("EDAT: Parsing data...\n");
	if (check_data(key, EDAT, NPD, input, verbose))
		ConLog.Error("EDAT: Data parsing failed!\n");
	else
		ConLog.Success("EDAT: Data successfully parsed!\n");

	printf("\n");

	ConLog.Write("EDAT: Decrypting data...\n");
	if (decrypt_data(input, output, EDAT, NPD, key, verbose))
		ConLog.Error("EDAT: Data decryption failed!");
	else
		ConLog.Success("EDAT: Data successfully decrypted!");

	delete NPD;
	delete EDAT;

	return 0;
}

int DecryptEDAT(const std::string& input_file_name, const std::string& output_file_name, int mode, const std::string& rap_file_name, unsigned char *custom_klic, bool verbose)
{
	// Prepare the files.
	wxFile input(input_file_name.c_str());
	wxFile output(output_file_name.c_str(), wxFile::write);
	wxFile rap(rap_file_name.c_str());

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
				ConLog.Error("EDAT: Invalid custom klic!\n");
				return -1;
			}

			break;
		}
	default:
		ConLog.Error("EDAT: Invalid mode!\n");
		return -1;
	}
	
	// Check the input/output files.
	if (!input.IsOpened() || !output.IsOpened())
	{
		ConLog.Error("EDAT: Failed to open files!\n");
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
		wxRemoveFile(output_file_name);
		return 0;
	}
	
	// Cleanup.
	input.Close();
	output.Close();
	return 0;
}
