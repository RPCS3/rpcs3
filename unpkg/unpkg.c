/*
 * Copyright 2011 Andrey Tolstoy <avtolstoy@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "unpkg.h"
#include "ps3_common.h"
#include "oddkeys.h"

static void hash_tostring(char *str, u8 *hash, u32 len)
{
	u8 *p;
	memset(str, 0, 2*len+1);
	for (p = hash; p-hash < len; p++)
	{
		str += 2;
	}
}

static void *pkg_open(const char *fname)
{
	FILE *f;
	
	f = fopen(fname, "rb");
	if (f == NULL)
	{
		ConLog.Error ("UnPkg: Could not open package file!");
		return NULL;
	}
	
	return f;
}

static int pkg_sanity_check(FILE *f, FILE *g, pkg_header **h_ptr, const char *fname)
{
	pkg_header *header = (pkg_header*)malloc(sizeof(pkg_header));
	u64 tmp;
	
	if (!fread(header, sizeof(pkg_header), 1, f))
	{
		ConLog.Error("UnPkg: Package file is too short!");
		return 1;
	}
	
	// some sanity checks
	
	if (ntohl(header->magic) != PKG_MAGIC)
	{
		ConLog.Error("UnPkg: Not a package file!");
		return 1;
	}
	
	switch (ntohl(header->rel_type) >> 16 & (0xffff))
	{
		case PKG_RELEASE_TYPE_DEBUG: 
			{
			ConLog.Warning ("UnPkg: Debug PKG detected.");
			u8* data;
			u8 sha_key[0x40];
			int i;
			f= fopen(fname, "rb");
			fseek(f, 0, SEEK_END);
			int nlen = ftell(f);
			fseek(f, 0, SEEK_SET);
			data = (u8*)malloc(nlen);
			fread(data, 1, nlen, f);
			fclose(f); 

			pkg_header2 *header = (pkg_header2 *)data;
			int data_offset = get_u64(&(header->dataOffset));
			int data_size = get_u64(&(header->dataSize));
			
			// decrypt debug
			u8 sha_crap[0x40];
			memset(sha_crap, 0, 0x40);
			memcpy(sha_crap, &data[0x60], 8);
			memcpy(sha_crap+0x8, &data[0x60], 8);
			memcpy(sha_crap+0x10, &data[0x68], 8);
			memcpy(sha_crap+0x18, &data[0x68], 8);

			int dptr;
			for(dptr = data_offset; dptr < (data_offset+data_size); dptr+=0x10) {
			u8 hash[0x14];
			sha1(sha_crap, 0x40, hash);
			for(i=0;i<0x10;i++) data[dptr+i] ^= hash[i];
			set_u64(sha_crap+0x38, get_u64(sha_crap+0x38)+1);
			}
 
			// recrypt retail
			u8 pkg_key[0x10];
			memcpy(pkg_key, &data[0x70], 0x10);

			//AES_KEY aes_key;
			aes_context aes_key;
			aes_setkey_enc(&aes_key, retail_pkg_aes_key, 128);

			size_t num=0; u8 ecount_buf[0x10]; memset(ecount_buf, 0, 0x10);
			aes_crypt_ctr(&aes_key, data_size, &num, pkg_key, ecount_buf, &data[data_offset], &data[data_offset]);
			
			// write back
			g = fopen(fname, "wb");
			data[4] = 0x80;   // set finalize flag
			memset(&data[(data_offset+data_size)], 0, 0x60);

			// add hash
			sha1(data, nlen-0x20, &data[nlen-0x20]);
			fwrite(data, 1, nlen, g);
			//fclose(g); // not close the file for continuing

			fseek(g, 0, SEEK_END);
			tmp = ftell(g);
			}
			break;
			
		
		case PKG_RELEASE_TYPE_RELEASE:
			{
				ConLog.Warning ("UnPkg: Retail PKG detected.");
				fseek(f, 0, SEEK_END);
				tmp = ftell(f);
			}
			break;

		default:
			ConLog.Error("UnPkg: Unknown release type.");
			return 1;
	

	}
	switch (ntohl(header->rel_type) & (0xffff))
	{
		case PKG_PLATFORM_TYPE_PS3:
		case PKG_PLATFORM_TYPE_PSP:
			break;
			
		default:
			ConLog.Error("UnPkg: Unknown platform type.");
			return 1;
	}
	
	if (ntohl(header->header_size) != PKG_HEADER_SIZE)
	{
		ConLog.Error("UnPkg: Wrong header size: ");
		return 1;
	}

	//fseek(g, 0, SEEK_END);
	//tmp = ftell(g);
	if (ntohll(header->pkg_size) != tmp)
	{
		ConLog.Error("UnPkg: File size mismatch.");
		return 1;
	}
	
	tmp -= ntohll(header->data_offset) + 0x60;
	if (ntohll(header->data_size) != tmp)
	{
		ConLog.Error("UnPkg: Data size mismatch.");
		return 1;
	}
	
	if (h_ptr != NULL)
	{
		(*h_ptr) = (pkg_header*) malloc(sizeof(pkg_header));
		memcpy(h_ptr, &header, sizeof(pkg_header*));
	}
	
	return 0;
}

static void print_pkg_header(pkg_header *header)
{
	char qa[33], kl[33];
	
	if (header == NULL)
		return;
	
	hash_tostring(qa, header->qa_digest, sizeof(header->qa_digest));
	hash_tostring(kl, header->klicensee, sizeof(header->klicensee));	
	
	ConLog.Write("Magic: %x\n", ntohl(header->magic));
	ConLog.Write("Release Type: %x\n", ntohl(header->rel_type) >> 16 & (0xffff));
	ConLog.Write("Platform Type: %x\n", ntohl(header->rel_type) & (0xffff));
	ConLog.Write("Header size: %x\n", ntohl(header->header_size));
	ConLog.Write("Unk1: %x\n", ntohl(header->unk1));
	ConLog.Write("Metadata size: %x\n", ntohl(header->meta_size));
	ConLog.Write("File count: %u\n", ntohl(header->file_count));
	ConLog.Write("Pkg size: %llu\n", ntohll(header->pkg_size));
	ConLog.Write("Data offset: %llx\n", ntohll(header->data_offset));
	ConLog.Write("Data size: %llu\n", ntohll(header->data_size));
	ConLog.Write("TitleID: %s\n", header->title_id);
	ConLog.Write("QA Digest: %s\n", qa);
	ConLog.Write( "KLicensee: %s\n", kl);
}

static void *pkg_info(const char *fname,  pkg_header **h_ptr)
{
	FILE *f;
	pkg_header *header;
	
	f = (FILE*) pkg_open(fname);
	if (f == NULL)
		return NULL;
	
	if (pkg_sanity_check(f, NULL, &header, fname))
		return NULL;
	
	print_pkg_header(header);
	
	if (h_ptr != NULL)
	{
		(*h_ptr) = header;
	} 
	else 
	{
		free(header);
	}

	
	return f;
}


static void pkg_crypt(const u8 *key, const u8 *kl, FILE *f, 
					  u64 len, FILE *out)
{
	aes_context c;
	u32 parts, bits;
	u32 i, j;
	u8 iv[HASH_LEN];
	u8 buf[BUF_SIZE];
	u8 ctr[BUF_SIZE];
	u8 out_buf[BUF_SIZE];
	u32 l;
	u64 hi, lo;
	
	parts = len / BUF_SIZE;
	if (len % BUF_SIZE != 0)
		parts++;

	memcpy(iv, kl, sizeof(iv));
	aes_setkey_enc(&c, key, 128);
	
	for (i = 0; i<parts; i++)
	{
		l = fread(buf, 1, BUF_SIZE, f);
		bits = l / HASH_LEN;
		if (bits % HASH_LEN != 0)
			bits++;
		
		for (j = 0; j<bits; j++)
		{
			aes_crypt_ecb(&c, AES_ENCRYPT, iv, ctr+j*HASH_LEN);
			
			hi = unpack64(iv);
			lo = unpack64(iv+8) + 1;
			if (lo == 0)
				hi++;
			pack64(iv, hi);
			pack64(iv + 8, lo);
		}
		
		
		memset(out_buf, 0, sizeof(out_buf));
		for (j=0; j<l; j++)
		{
			out_buf[j] = buf[j] ^ ctr[j];
		}
		
		fwrite(out_buf, 1, l, out);
	}
	
}

static void pkg_unpack_file(pkg_file_entry *fentry, FILE *dec)
{
	FILE *out = NULL;
	u64 size;
	u32 tmp;
	u8 buf[BUF_SIZE];
	
	fseek(dec, fentry->name_offset, SEEK_SET);
	
	memset(buf, 0, sizeof(buf));
	fread(buf, fentry->name_size, 1, dec);
	
	switch (fentry->type & (0xffff))
	{
		case PKG_FILE_ENTRY_NPDRM:
		case PKG_FILE_ENTRY_NPDRMEDAT:
		case PKG_FILE_ENTRY_SDAT:
		case PKG_FILE_ENTRY_REGULAR:
			out = fopen((char *)buf, "wb");
			fseek(dec, fentry->file_offset, SEEK_SET);
			for (size = 0; size < fentry->file_size; )
			{
				size += fread(buf, sizeof(u8), BUF_SIZE, dec);
				if (size > fentry->file_size)
					tmp = size - fentry->file_size;
				else
					tmp = 0;

				fwrite(buf, sizeof(u8), BUF_SIZE - tmp, out);
			}
			
			fclose(out);
			break;
			
		case PKG_FILE_ENTRY_FOLDER:
			mkdir ((char *)buf);
			break;
	}
}

static void pkg_unpack_data(u32 file_count, FILE *dec)
{
	u32 i;
	pkg_file_entry *file_table = NULL;
	
	fseek(dec, 0, SEEK_SET);
	
	file_table = (pkg_file_entry *)malloc(sizeof(pkg_file_entry)*file_count);
	i = fread(file_table, sizeof(pkg_file_entry), file_count, dec);
	
	if (ntohl(file_table->name_offset) / sizeof(pkg_file_entry) != file_count)
	{
		ConLog.Error("UnPkg: ERROR. Impossiburu!");
		return;
	}
	
	for (i = 0; i<file_count; i++)
	{
		(file_table+i)->name_offset = ntohl((file_table+i)->name_offset);
		(file_table+i)->name_size = ntohl((file_table+i)->name_size);
		(file_table+i)->file_offset = ntohll((file_table+i)->file_offset);
		(file_table+i)->file_size = ntohll((file_table+i)->file_size);
		(file_table+i)->type = ntohl((file_table+i)->type);
		
		pkg_unpack_file(file_table+i, dec);
		
	}
	
	free(file_table);
}

static void pkg_unpack(const char *fname)
{
	FILE *f, *dec;
	char *dec_fname;
	pkg_header *header;
	int ret;
	struct stat sb;
	
	f = (FILE*) pkg_info(fname, &header);
	
	if (f == NULL)
		return;
	
	fseek(f, ntohll(header->data_offset), SEEK_SET);
	
	dec_fname = (char*)malloc(strlen(fname)+4);
	memset(dec_fname, 0, strlen(fname)+4);
	sprintf(dec_fname, "%s.dec", fname);
	
	dec = fopen(dec_fname, "wb+");
	if (dec == NULL)
	{
		ConLog.Error("UnPkg: Could not create temp file for decrypted data.");
		free(header);
		return;
	}
	unlink(dec_fname);
	
	pkg_crypt(PKG_AES_KEY, header->klicensee, f, ntohll(header->data_size), 
			  dec);
	fseek(dec, 0, SEEK_SET);
	
	fclose(f);
	
	if (stat(header->title_id, &sb) != 0)
	{
		ret = mkdir(header->title_id);
		if (ret < 0)
		{
			ConLog.Error("UnPkg: Could not mkdir.");
			free(header);
			return;
		}
	}
	
	chdir(header->title_id);
	
	pkg_unpack_data(ntohl(header->file_count), dec);
	fclose(dec);
}