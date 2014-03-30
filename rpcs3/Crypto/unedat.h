#pragma once
#include "utils.h"
#include "key_vault.h"

#define SDAT_FLAG 0x01000000
#define EDAT_COMPRESSED_FLAG 0x00000001
#define EDAT_FLAG_0x02 0x00000002
#define EDAT_ENCRYPTED_KEY_FLAG 0x00000008
#define EDAT_FLAG_0x10 0x00000010
#define EDAT_FLAG_0x20 0x00000020
#define EDAT_DEBUG_DATA_FLAG 0x80000000

typedef struct 
{
	unsigned char magic[4];
    int version;
    int license;
    int type;
    unsigned char content_id[0x30];
    unsigned char digest[0x10];
    unsigned char title_hash[0x10];
    unsigned char dev_hash[0x10];
    unsigned long long unk1;
    unsigned long long unk2;
} NPD_HEADER;

typedef struct 
{
	int flags;
	int block_size;
	unsigned long long file_size;
} EDAT_SDAT_HEADER;

int DecryptEDAT(const std::string& input_file_name, const std::string& output_file_name, int mode, const std::string& rap_file_name, unsigned char *custom_klic, bool verbose);