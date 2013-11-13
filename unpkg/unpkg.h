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


#ifndef UNPKG_H_
#define UNPKG_H_

#include "stdafx.h"
#include "scetool/aes.h"
#include "scetool/sha1.h"

#define ntohll(x) (((u64) ntohl (x) << 32) | (u64) ntohl (x >> 32) )
#define htonll(x) (((u64) htonl (x) << 32) | (u64) htonl (x >> 32) )
#define conv_ntohl(x) { x = ntohl(x); }
#define conv_ntohll(x) { x = ntohll(x); }
#define conv_htonl(x) { x = htonl(x); }
#define conv_htonll(x) { x = htonll(x); }

#define unpack32(x) ((u32) ((u32)*(x) << 24 |	\
						(u32)*(x+1) << 16 | 	\
						(u32)*(x+2) << 8 | 		\
						(u32)*(x+3) << 0))

#define unpack64(x) ((u64)unpack32(x) << 32 | (u64)unpack32(x+4))

#define pack32(x, p) 								\
					{								\
						*(x) = (u8)(p >> 24);		\
						*((x)+1) = (u8)(p >> 16);	\
						*((x)+2) = (u8)(p >> 8);	\
						*((x)+3) = (u8)(p >> 0);	\
					}

#define pack64(x, p) { pack32((x + 4), p); pack32((x), p >> 32); }

#define HASH_LEN 16
#define BUF_SIZE 4096

typedef struct {
	u32 magic; // magic 0x7f504b47
	u32 rel_type; // release type
	u32 header_size; // 0xc0
	u32 unk1; //some pkg version maybe
	u32 meta_size; //size of metadata (block after header & hashes) 
	u32 file_count; // number of files
	u64 pkg_size; // pkg size in bytes
	u64 data_offset; // encrypted data offset
	u64 data_size; // encrypted data size in bytes
	char title_id[48]; // title id
	u8 qa_digest[16]; // this should be the hash of "files + attribs"
	u8 klicensee[16]; // nonce
} pkg_header;

typedef struct {
	u8 hash1[16];
	u8 hash2[16];
	u8 hash3[16];
	u8 hash4[16];
} pkg_unk_checksum;

/*
 is it in meta or sfo?
 # CATEGORY       : HG
 # BOOTABLE       : YES
 # VERSION        : 01.00
 # APP_VER        : 01.00
 # PS3_SYSTEM_VER : 03.0000
 */

/* meta hell structure */
typedef struct {
	u32 unk1;
	u32 unk2;
	u32 drm_type;
	u32 unk3;
	
	u32 unk4;
	u32 unk5;
	u32 unk6;
	u32 unk7;
	
	u32 unk8;
	u32 unk9;
	u32 unk10;
	u32 unk11;
	
	u32 data_size;
	u32 unk12;
	u32 unk13;
	u32 packager;
	
	u8 unk14[64];
} pkg_meta;

typedef struct {
	u32 name_offset; // file name offset
	u32 name_size; // file name size
	u64 file_offset; // file offset
	u64 file_size; // file size
	u32 type; // file type
	/*
	 0x80000003 - regular file
	 0x80000001 - npdrm
	 0x80000004 - folder
	 0x80000009 - sdat ?
	 0x80000002 - npdrm.edat ?
	 */
	u32 pad; // padding (zeros)
} pkg_file_entry;

typedef struct {
	pkg_file_entry fe;
	char *name;
	char *path;
} file_table_tr;

#define PKG_MAGIC 0x7f504b47 // \x7fPKG
#define PKG_HEADER_SIZE sizeof(pkg_header) + sizeof(pkg_unk_checksum)
#define PKG_RELEASE_TYPE_RELEASE 0x8000
#define PKG_RELEASE_TYPE_DEBUG 0x0000
#define PKG_PLATFORM_TYPE_PS3 0x0001
#define PKG_PLATFORM_TYPE_PSP 0x0002

#define PKG_FILE_ENTRY_OVERWRITE 0x80000000
#define PKG_FILE_ENTRY_NPDRM 0x0001
#define PKG_FILE_ENTRY_NPDRMEDAT 0x0002 // npdrm.edat
#define PKG_FILE_ENTRY_REGULAR 0x0003
#define PKG_FILE_ENTRY_FOLDER 0x0004
#define PKG_FILE_ENTRY_SDAT 0x0009 // .sdat ?

static const u8 PKG_AES_KEY[16] = {
	0x2e, 0x7b, 0x71, 0xd7,
	0xc9, 0xc9, 0xa1, 0x4e,
	0xa3, 0x22, 0x1f, 0x18,
	0x88, 0x28, 0xb8, 0xf8
};

static void hash_tostring(char *str, u8 *hash, u32 len);

static void *pkg_open(const char *fname);

static int pkg_sanity_check(FILE *f, FILE *g, pkg_header **h_ptr, const char *fname);

static void print_pkg_header(pkg_header *header);

static void *pkg_info(const char *fname, pkg_header **h_ptr);

static void pkg_crypt(const u8 *key, const u8 *kl, FILE *f, 
					  u64 len, FILE *out);

static void pkg_unpack(const char *fname);

static void pkg_unpack_data(u32 file_count, FILE *dec);

static void pkg_unpack_file(pkg_file_entry *fentry, FILE *dec);;

static int pkg_pack_data(file_table_tr *ftr, pkg_file_entry *table,
						 int file_count, sha1_context *ctx, FILE *out);


static void *pkg_pack_create_filetable(file_table_tr *tr, int file_count,
									   char **n_table, u32 *n_table_len);

#endif
