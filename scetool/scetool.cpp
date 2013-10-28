/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include "getopt.h"
#else
#include <unistd.h>
#include <getopt.h>
#endif

#include "types.h"
#include "config.h"
#include "aes.c"
#include "getopt.c"
#include "sha1.c"
#include "zlib.h"
#include "util.h"
#include "keys.h"
#include "sce.h"
#include "np.h"
#include "self.h"
#include "rvk.h"


/*! Verbose mode. */
BOOL _verbose = FALSE;
/*! Raw mode. */
BOOL _raw = FALSE;


/*! List keys. */
static BOOL _list_keys = FALSE;

/*! Parameters. */
scetool::s8 *_template = NULL;
scetool::s8 *_file_type = NULL;
scetool::s8 *_compress_data = NULL;
scetool::s8 *_skip_sections = NULL;
scetool::s8 *_key_rev = NULL;
scetool::s8 *_meta_info = NULL;
scetool::s8 *_keyset = NULL;
scetool::s8 *_auth_id = NULL;
scetool::s8 *_vendor_id = NULL;
scetool::s8 *_self_type = NULL;
scetool::s8 *_app_version = NULL;
scetool::s8 *_fw_version = NULL;
scetool::s8 *_add_shdrs = NULL;
scetool::s8 *_ctrl_flags = NULL;
scetool::s8 *_cap_flags = NULL;
#ifdef CONFIG_CUSTOM_INDIV_SEED
scetool::s8 *_indiv_seed = NULL;
#endif
scetool::s8 *_license_type = NULL;
scetool::s8 *_app_type = NULL;
scetool::s8 *_content_id = NULL;
scetool::s8 *_klicensee = NULL;
scetool::s8 *_real_fname = NULL;
scetool::s8 *_add_sig = NULL;

#include <time.h>


//FILE: LIST.CPP
list_t *list_create()
{
	list_t *res;
	
	if((res = (list_t *)malloc(sizeof(list_t))) == NULL)
		return NULL;
	
	res->head = NULL;
	res->count = 0;
	
	return res;
}

void list_destroy(list_t *l)
{
	if(l == NULL)
		return;
	
	lnode_t *iter = l->head, *tmp;
	
	while(iter != NULL)
	{
		tmp = iter;
		iter = iter->next;
		free(tmp);
	}
	
	free(l);
}

BOOL list_push(list_t *l, void *value)
{	
	if(l == NULL)
		return FALSE;
	
	lnode_t *_new;
	
	//Allocate new node.
	if((_new = (lnode_t *)malloc(sizeof(lnode_t))) == NULL)
		return FALSE;
	
	//Insert.
	_new->value = value;
	_new->next = l->head;
	l->head = _new;
	l->count++;
	
	return TRUE;
}

BOOL list_add_back(list_t *l, void *value)
{	
	if(l == NULL)
		return FALSE;
	
	lnode_t *n, *_new;
	
	//Allocate new node.
	if((_new = (lnode_t *)malloc(sizeof(lnode_t))) == NULL)
		return FALSE;
	
	_new->value = value;
	_new->next = NULL;
	
	if(l->head == NULL)
		l->head = _new;
	else
	{
		//Move to the list end.
		for(n = l->head; n->next != NULL; n = n->next);
		
		//Add.
		n->next = _new;
		l->count++;
	}
	
	return TRUE;
}

BOOL list_remove_node(list_t *l, lnode_t *node)
{	
	if(l == NULL)
		return FALSE;
	
	lnode_t *iter;
	
	if(l->head == node)
	{
		l->head = l->head->next;
		free(node);
		l->count--;
		
		return TRUE;
	}
	
	iter = l->head;
	while(iter->next != NULL)
	{
		if(iter->next == node)
		{
			iter->next = iter->next->next;
			free(node);
			l->count--;
			
			return TRUE;
		}
		iter = iter->next;
	}
	
	return FALSE;
}

////

//FILE: UTIL.CPP
scetool::u64 _x_to_u64(const scetool::s8 *hex)
{
	scetool::u64 t = 0, res = 0;
	scetool::u32 len = strlen(hex);
	char c;

	while(len--)
	{
		c = *hex++;
		if(c >= '0' && c <= '9')
			t = c - '0';
		else if(c >= 'a' && c <= 'f')
			t = c - 'a' + 10;
		else if(c >= 'A' && c <= 'F')
			t = c - 'A' + 10;
		else
			t = 0;
		res |= t << (len * 4);
	}

	return res;
}

scetool::u8 *_x_to_u8_buffer(const scetool::s8 *hex)
{
	scetool::u32 len = strlen(hex);
	scetool::s8 xtmp[3] = {0, 0, 0};

	//Must be aligned to 2.
	if(len % 2 != 0)
		return NULL;

	scetool::u8 *res = (scetool::u8 *)malloc(sizeof(scetool::u8) * len);
	scetool::u8 *ptr = res;

	while(len--)
	{
		xtmp[0] = *hex++;
		xtmp[1] = *hex++;

		*ptr++ = (scetool::u8)_x_to_u64(xtmp);
	}

	return res;
}

scetool::u8 *_read_buffer(const scetool::s8 *file, scetool::u32 *length)
{
	FILE *fp;
	scetool::u32 size;

	if((fp = fopen(file, "rb")) == NULL)
		return NULL;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	scetool::u8 *buffer = (scetool::u8 *)malloc(sizeof(scetool::u8) * size);
	fread(buffer, sizeof(scetool::u8), size, fp);

	if(length != NULL)
		*length = size;

	fclose(fp);

	return buffer;
}

int _write_buffer(const scetool::s8 *file, scetool::u8 *buffer, scetool::u32 length)
{
	FILE *fp;

	if((fp = fopen(file, "wb")) == NULL)
		return 0;

	/**/
	while(length > 0)
	{
		scetool::u32 wrlen = 1024;
		if(length < 1024)
			wrlen = length;
		fwrite(buffer, sizeof(scetool::u8), wrlen, fp);
		length -= wrlen;
		buffer += 1024;
	}
	/**/

	//fwrite(buffer, sizeof(scetool::u8), length, fp);

	fclose(fp);

	return 1;
}

void _zlib_inflate(scetool::u8 *in, scetool::u64 len_in, scetool::u8 *out, scetool::u64 len_out)
{
	z_stream s;
	memset(&s, 0, sizeof(z_stream));

	s.zalloc = Z_NULL;
	s.zfree = Z_NULL;
	s.opaque = Z_NULL;

	inflateInit(&s);

	s.avail_in = len_in;
	s.next_in = in;
	s.avail_out = len_out;
	s.next_out = out;

	inflate(&s, Z_FINISH);

	inflateEnd(&s);
}

void *_memdup(void *ptr, scetool::u32 size)
{
	void *res = malloc(size);

	if(res != NULL)
		memcpy(res, ptr, size);
	
	return res;
}

void _print_align(FILE *fp, const scetool::s8 *str, scetool::s32 align, scetool::s32 len)
{
	scetool::s32 i, tmp;
	tmp = align - len;
	if(tmp < 0)
			tmp = 0;
	for(i = 0; i < tmp; i++)
		fputs(str, fp);
}

const scetool::s8 *_get_name(id_to_name_t *tab, scetool::u64 id)
{
	scetool::u32 i = 0;

	while(!(tab[i].name == NULL && tab[i].id == 0))
	{
		if(tab[i].id == id)
			return tab[i].name;
		i++;
	}

	return NULL;
}

//FILE: SCE.CPP
#include "sce_inlines.h"

sce_buffer_ctxt_t *sce_create_ctxt_from_buffer(scetool::u8 *scebuffer)
{
	sce_buffer_ctxt_t *res;

	if((res = (sce_buffer_ctxt_t *)malloc(sizeof(sce_buffer_ctxt_t))) == NULL)
		return NULL;

	memset(res, 0, sizeof(sce_buffer_ctxt_t));

	res->scebuffer = scebuffer;
	res->mdec = FALSE;

	//Set pointer to SCE header.
	res->sceh = (sce_header_t *)scebuffer;
	_es_sce_header(res->sceh);

	//Set pointers to file type specific headers.
	switch(res->sceh->header_type)
	{
		case SCE_HEADER_TYPE_SELF:
		{
			//SELF header.
			res->self.selfh = (self_header_t *)(res->scebuffer + sizeof(sce_header_t));
			_es_self_header(res->self.selfh);

			//Application info.
			res->self.ai = (app_info_t *)(res->scebuffer + res->self.selfh->app_info_offset);
			_es_app_info(res->self.ai);

			//Section infos.
			res->self.si = (section_info_t *)(res->scebuffer + res->self.selfh->section_info_offset);

			//SCE version.
			if(res->self.selfh->sce_version_offset != NULL)
			{
				res->self.sv = (sce_version_t *)(res->scebuffer + res->self.selfh->sce_version_offset);
				_es_sce_version(res->self.sv);
			}
			else
				res->self.sv = 0;

			//Get pointers to all control infos.
			scetool::u32 len = (scetool::u32)res->self.selfh->control_info_size;
			if(len > 0)
			{
				scetool::u8 *ptr = res->scebuffer + res->self.selfh->control_info_offset;
				res->self.cis = list_create();

				while(len > 0)
				{
					control_info_t *tci = (control_info_t *)ptr;
					_es_control_info(tci);
					ptr += tci->size;
					len -= tci->size;
					list_add_back(res->self.cis, tci);
				}
			}
			else
				res->self.cis = NULL;
		}
		break;
	case SCE_HEADER_TYPE_RVK:
		//TODO
		break;
	case SCE_HEADER_TYPE_PKG:
		//TODO
		break;
	case SCE_HEADER_TYPE_SPP:
		//TODO
		break;
	default:
		free(res);
		return NULL;
		break;
	}

	//Set pointers to metadata headers.
	res->metai = (metadata_info_t *)(scebuffer + sizeof(sce_header_t) + res->sceh->metadata_offset);
	res->metah = (metadata_header_t *)((scetool::u8 *)res->metai + sizeof(metadata_info_t));
	res->metash = (metadata_section_header_t *)((scetool::u8 *)res->metah + sizeof(metadata_header_t));

	return res;
}

static scetool::s8 _sce_tmp_vstr[16];
scetool::s8 *sce_version_to_str(scetool::u64 version)
{
	scetool::u32 v = version >> 32;
	sprintf(_sce_tmp_vstr, "%02X.%02X", (v & 0xFFFF0000) >> 16, v & 0x0000FFFF);
	return _sce_tmp_vstr;
}

BOOL sce_decrypt_header(sce_buffer_ctxt_t *ctxt, scetool::u8 *metadata_info, scetool::u8 *keyset)
{
	scetool::u32 i;
	size_t nc_off;
	scetool::u8 sblk[0x10], iv[0x10];
	keyset_t *ks;
	aes_context aes_ctxt;

	//Check if provided metadata info should be used.
	if(metadata_info == NULL)
	{
		//Check if a keyset is provided.
		if(keyset == NULL)
		{
			//Try to find keyset.
			if((ks = keyset_find(ctxt)) == NULL)
				return FALSE;

			_LOG_VERBOSE("Using keyset [%s 0x%04X %s]\n", ks->name, ks->key_revision, sce_version_to_str(ks->version));
		}
		else
		{
			//Use the provided keyset.
			ks = keyset_from_buffer(keyset);
		}

		//Remove NPDRM layer.
		if(ctxt->sceh->header_type == SCE_HEADER_TYPE_SELF && ctxt->self.ai->self_type == SELF_TYPE_NPDRM)
			if(np_decrypt_npdrm(ctxt) == FALSE)
				return FALSE;

		//Decrypt metadata info.
		aes_setkey_dec(&aes_ctxt, ks->erk, KEYBITS(ks->erklen));
		memcpy(iv, ks->riv, 0x10); //!!!
		aes_crypt_cbc(&aes_ctxt, AES_DECRYPT, sizeof(metadata_info_t), iv, (scetool::u8 *)ctxt->metai, (scetool::u8 *)ctxt->metai);
	}
	else
	{
		//Copy provided metadata info over SELF metadata.
		memcpy((scetool::u8 *)ctxt->metai, metadata_info, sizeof(metadata_info));
	}

	if(ctxt->metai->key_pad[0] != 0x00 || ctxt->metai->iv_pad[0] != 0x00)
		return FALSE;

	//Decrypt metadata header, metadata section headers and keys.
	nc_off = 0;
	aes_setkey_enc(&aes_ctxt, ctxt->metai->key, METADATA_INFO_KEYBITS);
	aes_crypt_ctr(&aes_ctxt, 
		ctxt->sceh->header_len - (sizeof(sce_header_t) + ctxt->sceh->metadata_offset + sizeof(metadata_info_t)), 
		&nc_off, ctxt->metai->iv, sblk, (scetool::u8 *)ctxt->metah, (scetool::u8 *)ctxt->metah);

	//Fixup headers.
	_es_metadata_header(ctxt->metah);
	for(i = 0; i < ctxt->metah->section_count; i++)
		_es_metadata_section_header(&ctxt->metash[i]);

	//Metadata decrypted.
	ctxt->mdec = TRUE;

	//Set start of SCE file keys.
	ctxt->keys = (scetool::u8 *)ctxt->metash + sizeof(metadata_section_header_t) * ctxt->metah->section_count;
	ctxt->keys_len = ctxt->metah->key_count * 0x10;

	//Set SELF only headers.
	if(ctxt->sceh->header_type == SCE_HEADER_TYPE_SELF)
	{
		//Get pointers to all optional headers.
		ctxt->self.ohs = list_create();
		opt_header_t *oh = (opt_header_t *)(ctxt->keys + ctxt->metah->key_count * 0x10);
		_es_opt_header(oh);
		list_add_back(ctxt->self.ohs, oh);
		while(oh->next != 0)
		{
			oh = (opt_header_t *)((scetool::u8 *)oh + oh->size);
			_es_opt_header(oh);
			list_add_back(ctxt->self.ohs, oh);
		}

		//Signature.
		ctxt->sig = (signature_t *)((scetool::u8 *)oh + oh->size);
	}
	else
		ctxt->sig = (signature_t *)(ctxt->keys + ctxt->metah->key_count * 0x10);

	return TRUE;
}

BOOL sce_decrypt_data(sce_buffer_ctxt_t *ctxt)
{
	scetool::u32 i;
	aes_context aes_ctxt;

	//Decrypt sections.
	for(i = 0; i < ctxt->metah->section_count; i++)
	{
		size_t nc_off = 0;
		scetool::u8 buf[16];
		scetool::u8 iv[16];

		//Only decrypt encrypted sections.
		if(ctxt->metash[i].encrypted == METADATA_SECTION_ENCRYPTED)
		{
			if(ctxt->metash[i].key_index > ctxt->metah->key_count - 1 || ctxt->metash[i].iv_index > ctxt->metah->key_count)
				printf("[*] Warning: Skipped decryption of section %03d (marked encrypted but key/iv index out of range)\n", i);
			else
			{
				memcpy(iv, ctxt->keys + ctxt->metash[i].iv_index * 0x10, 0x10);
				aes_setkey_enc(&aes_ctxt, ctxt->keys + ctxt->metash[i].key_index * 0x10, 128);
				scetool::u8 *ptr = ctxt->scebuffer + ctxt->metash[i].data_offset;
				aes_crypt_ctr(&aes_ctxt, ctxt->metash[i].data_size, &nc_off, iv, buf, ptr, ptr);
			}
		}
	}

	return TRUE;
}

//FILE: SELF.CPP
#include "elf_inlines.h"

//TODO: maybe implement better.
BOOL self_write_to_elf(sce_buffer_ctxt_t *ctxt, const scetool::s8 *elf_out)
{
	FILE *fp;
	scetool::u32 i, self_type;

	const scetool::u8 *eident;

	//Check for SELF.
	if(ctxt->sceh->header_type != SCE_HEADER_TYPE_SELF)
		return FALSE;

	if((fp = fopen(elf_out, "wb")) == NULL)
		return FALSE;

	self_type = ctxt->self.ai->self_type;
	eident = ctxt->scebuffer + ctxt->self.selfh->elf_offset;

	//SPU is 32 bit.
	if(self_type == SELF_TYPE_LDR || self_type == SELF_TYPE_ISO || eident[EI_CLASS] == ELFCLASs32)
	{
#ifdef CONFIG_DUMP_INDIV_SEED
		/*
		//Print individuals seed.
		if(self_type == SELF_TYPE_ISO)
		{
			scetool::u8 *indiv_seed = (scetool::u8 *)ctxt->self.ish + sizeof(iseed_header_t);
			scetool::s8 ifile[256];
			sprintf(ifile, "%s.indiv_seed.bin", elf_out);
			FILE *ifp = fopen(ifile, "wb");
			fwrite(indiv_seed, sizeof(scetool::u8), ctxt->self.ish->size - sizeof(iseed_header_t), ifp);
			fclose(ifp);
		}
		*/
#endif

		//32 bit ELF.
		Elf32_Ehdr ceh, *eh = (Elf32_Ehdr *)(ctxt->scebuffer + ctxt->self.selfh->elf_offset);
		_copy_es_elf32_ehdr(&ceh, eh);

		//Write ELF header.
		fwrite(eh, sizeof(Elf32_Ehdr), 1, fp);

		//Write program headers.
		Elf32_Phdr *ph = (Elf32_Phdr *)(ctxt->scebuffer + ctxt->self.selfh->phdr_offset);
		fwrite(ph, sizeof(Elf32_Phdr), ceh.e_phnum, fp);

		//Write program data.
		metadata_section_header_t *msh = ctxt->metash;
		for(i = 0; i < ctxt->metah->section_count; i++)
		{
			if(msh[i].type == METADATA_SECTION_TYPE_PHDR)
			{
				_es_elf32_phdr(&ph[msh[i].index]);
				fseek(fp, ph[msh[i].index].p_offset, SEEK_SET);
				fwrite(ctxt->scebuffer + msh[i].data_offset, sizeof(scetool::u8), msh[i].data_size, fp);
			}
		}		

		//Write section headers.
		if(ctxt->self.selfh->shdr_offset != 0)
		{
			Elf32_Shdr *sh = (Elf32_Shdr *)(ctxt->scebuffer + ctxt->self.selfh->shdr_offset);
			fseek(fp, ceh.e_shoff, SEEK_SET);
			fwrite(sh, sizeof(Elf32_Shdr), ceh.e_shnum, fp);
		}
	}
	else
	{
		//64 bit ELF.
		Elf64_Ehdr ceh, *eh = (Elf64_Ehdr *)(ctxt->scebuffer + ctxt->self.selfh->elf_offset);
		_copy_es_elf64_ehdr(&ceh, eh);

		//Write ELF header.
		fwrite(eh, sizeof(Elf64_Ehdr), 1, fp);

		//Write program headers.
		Elf64_Phdr *ph = (Elf64_Phdr *)(ctxt->scebuffer + ctxt->self.selfh->phdr_offset);
		fwrite(ph, sizeof(Elf64_Phdr), ceh.e_phnum, fp);

		//Write program data.
		metadata_section_header_t *msh = ctxt->metash;
		for(i = 0; i < ctxt->metah->section_count; i++)
		{
			if(msh[i].type == METADATA_SECTION_TYPE_PHDR)
			{
				if(msh[i].compressed == METADATA_SECTION_COMPRESSED)
				{
					_es_elf64_phdr(&ph[msh[i].index]);
					scetool::u8 *data = (scetool::u8 *)malloc(ph[msh[i].index].p_filesz);

					_zlib_inflate(ctxt->scebuffer + msh[i].data_offset, msh[i].data_size, data, ph[msh[i].index].p_filesz);
					fseek(fp, ph[msh[i].index].p_offset, SEEK_SET);
					fwrite(data, sizeof(scetool::u8), ph[msh[i].index].p_filesz, fp);

					free(data);
				}
				else
				{
					_es_elf64_phdr(&ph[msh[i].index]);
					fseek(fp, ph[msh[i].index].p_offset, SEEK_SET);
					fwrite(ctxt->scebuffer + msh[i].data_offset, sizeof(scetool::u8), msh[i].data_size, fp);
				}
			}
		}		

		//Write section headers.
		if(ctxt->self.selfh->shdr_offset != 0)
		{
			Elf64_Shdr *sh = (Elf64_Shdr *)(ctxt->scebuffer + ctxt->self.selfh->shdr_offset);
			fseek(fp, ceh.e_shoff, SEEK_SET);
			fwrite(sh, sizeof(Elf64_Shdr), ceh.e_shnum, fp);
		}
	}

	fclose(fp);

	return TRUE;
}

//FILE: NP.CPP
/*! klicensee key. */
static scetool::u8 *_klicensee_key;

static ci_data_npdrm_t *_sce_find_ci_npdrm(sce_buffer_ctxt_t *ctxt)
{
	if(ctxt->self.cis != NULL)
	{
		LIST_FOREACH(iter, ctxt->self.cis)
		{
			control_info_t *ci = (control_info_t *)iter->value;

			if(ci->type == CONTROL_INFO_TYPE_NPDRM)
			{
				ci_data_npdrm_t *np = (ci_data_npdrm_t *)((scetool::u8 *)ci + sizeof(control_info_t));
				//Fixup.
				_es_ci_data_npdrm(np);
				return np;
			}
		}
	}

	return NULL;
}

void np_set_klicensee(scetool::u8 *klicensee)
{
	_klicensee_key = klicensee;
}

BOOL np_decrypt_npdrm(sce_buffer_ctxt_t *ctxt)
{
	aes_context aes_ctxt;
	keyset_t *ks_np_klic_free, *ks_klic_key;
	scetool::u8 npdrm_key[0x10];
	scetool::u8 npdrm_iv[0x10];
	ci_data_npdrm_t *np;

	if((np = _sce_find_ci_npdrm(ctxt)) == NULL)
		return FALSE;

	//Try to find keysets.
	ks_klic_key = keyset_find_by_name(CONFIG_NP_KLIC_KEY_KNAME);
	if(ks_klic_key == NULL)
		return FALSE;
	if(_klicensee_key != NULL)
		memcpy(npdrm_key, _klicensee_key, 0x10);
	else if(np->license_type == NP_LICENSE_FREE)
	{
		ks_np_klic_free = keyset_find_by_name(CONFIG_NP_KLIC_FREE_KNAME);
		if(ks_np_klic_free == NULL)
			return FALSE;
		memcpy(npdrm_key, ks_np_klic_free->erk, 0x10);
	}
	else if(np->license_type == NP_LICENSE_LOCAL)
	{
		if ((klicensee_by_content_id((scetool::s8 *)np->content_id, npdrm_key)) == FALSE)
			return FALSE;
	}
	else
		return FALSE;

	aes_setkey_dec(&aes_ctxt, ks_klic_key->erk, METADATA_INFO_KEYBITS);
	aes_crypt_ecb(&aes_ctxt, AES_DECRYPT, npdrm_key, npdrm_key);

	memset(npdrm_iv, 0, 0x10);
	aes_setkey_dec(&aes_ctxt, npdrm_key, METADATA_INFO_KEYBITS);
	aes_crypt_cbc(&aes_ctxt, AES_DECRYPT, sizeof(metadata_info_t), npdrm_iv, (scetool::u8 *)ctxt->metai, (scetool::u8 *)ctxt->metai);

	return TRUE;
}

//FILE: TABLES.CPP
/*! SCE header types. */
id_to_name_t _sce_header_types[] = 
{
	{SCE_HEADER_TYPE_SELF, "SELF"},
	{SCE_HEADER_TYPE_RVK, "RVK"},
	{SCE_HEADER_TYPE_PKG, "PKG"},
	{SCE_HEADER_TYPE_SPP, "SPP"},
	{0, NULL}
};

/*! SELF types. */
id_to_name_t _self_types[] = 
{
	{SELF_TYPE_LV0, "lv0"},
	{SELF_TYPE_LV1, "lv1"},
	{SELF_TYPE_LV2, "lv2"},
	{SELF_TYPE_APP, "Application"},
	{SELF_TYPE_ISO, "Isolated SPU Module"},
	{SELF_TYPE_LDR, "Secure Loader"},
	{SELF_TYPE_UNK_7, "Unknown 7"},
	{SELF_TYPE_NPDRM, "NPDRM Application"},
	{0, NULL}
};

/*! Key types. */
id_to_name_t _key_types[] = 
{
	{KEYTYPE_SELF, "SELF"},
	{KEYTYPE_RVK, "RVK"},
	{KEYTYPE_PKG, "PKG"},
	{KEYTYPE_SPP, "SPP"},
	{KEYTYPE_OTHER, "OTHER"},
	{0, NULL}
};

//FILE: KEYS.CPP
/*! Loaded keysets. */
list_t *_keysets;
/*! Loaded curves. */
curve_t *_curves;
/*! Loaded VSH curves. */
vsh_curve_t *_vsh_curves;

static scetool::u8 rap_init_key[0x10] = 
{
	0x86, 0x9F, 0x77, 0x45, 0xC1, 0x3F, 0xD8, 0x90, 0xCC, 0xF2, 0x91, 0x88, 0xE3, 0xCC, 0x3E, 0xDF
};

static scetool::u8 rap_pbox[0x10] = 
{
	0x0C, 0x03, 0x06, 0x04, 0x01, 0x0B, 0x0F, 0x08, 0x02, 0x07, 0x00, 0x05, 0x0A, 0x0E, 0x0D, 0x09
};

static scetool::u8 rap_e1[0x10] = 
{
	0xA9, 0x3E, 0x1F, 0xD6, 0x7C, 0x55, 0xA3, 0x29, 0xB7, 0x5F, 0xDD, 0xA6, 0x2A, 0x95, 0xC7, 0xA5
};

static scetool::u8 rap_e2[0x10] = 
{
	0x67, 0xD4, 0x5D, 0xA3, 0x29, 0x6D, 0x00, 0x6A, 0x4E, 0x7C, 0x53, 0x7B, 0xF5, 0x53, 0x8C, 0x74
};

static act_dat_t *act_dat_load()
{
	scetool::s8 *ps3 = NULL, path[256];
	act_dat_t *act_dat;
	scetool::u32 len = 0;
	
	if((ps3 = getenv(CONFIG_ENV_PS3)) != NULL)
		if(_access(ps3, 0) != 0)
			ps3 = NULL;

	if(ps3 != NULL)
	{
		sprintf(path, "%s/%s", ps3, CONFIG_ACT_DAT_FILE);
		if(_access(path, 0) != 0)
			sprintf(path, "%s/%s", CONFIG_ACT_DAT_PATH, CONFIG_ACT_DAT_FILE);
	}
	else
		sprintf(path, "%s/%s", CONFIG_ACT_DAT_PATH, CONFIG_ACT_DAT_FILE);

	act_dat = (act_dat_t *)_read_buffer(path, &len);
	
	if(act_dat == NULL)
		return NULL;
	
	if(len != ACT_DAT_LENGTH)
	{
		free(act_dat);
		return NULL;
	}
	
	return act_dat;
}

static rif_t *rif_load(const scetool::s8 *content_id)
{
	scetool::s8 *ps3 = NULL, path[256];
	rif_t *rif;
	scetool::u32 len = 0;
	
	if((ps3 = getenv(CONFIG_ENV_PS3)) != NULL)
		if(_access(ps3, 0) != 0)
			ps3 = NULL;

	if(ps3 != NULL)
	{
		sprintf(path, "%s/%s%s", ps3, content_id, CONFIG_RIF_FILE_EXT);
		if(_access(path, 0) != 0)
			sprintf(path, "%s/%s%s", CONFIG_RIF_PATH, content_id, CONFIG_RIF_FILE_EXT);
	}
	else
		sprintf(path, "%s/%s%s", CONFIG_RIF_PATH, content_id, CONFIG_RIF_FILE_EXT);

	rif = (rif_t *)_read_buffer(path, &len);
	if(rif == NULL)
		return NULL;
	
	if(len < RIF_LENGTH)
	{
		free(rif);
		return NULL;
	}
	
	return rif;
}

static scetool::u8 *rap_load(const scetool::s8 *content_id)
{
	scetool::s8 *ps3 = NULL, path[256];
	scetool::u8 *rap;
	scetool::u32 len = 0;
	
	if((ps3 = getenv(CONFIG_ENV_PS3)) != NULL)
		if(_access(ps3, 0) != 0)
			ps3 = NULL;

	if(ps3 != NULL)
	{
		sprintf(path, "%s/%s%s", ps3, content_id, CONFIG_RAP_FILE_EXT);
		if(_access(path, 0) != 0)
			sprintf(path, "%s/%s%s", CONFIG_RAP_PATH, content_id, CONFIG_RAP_FILE_EXT);
	}
	else
		sprintf(path, "%s/%s%s", CONFIG_RAP_PATH, content_id, CONFIG_RAP_FILE_EXT);

	rap = (scetool::u8 *)_read_buffer(path, &len);
	
	if(rap == NULL)
		return NULL;
	
	if(len != RAP_LENGTH)
	{
		free(rap);
		return NULL;
	}
	
	return rap;
}

static BOOL rap_to_klicensee(const scetool::s8 *content_id, scetool::u8 *klicensee)
{
	scetool::u8 *rap;
	aes_context aes_ctxt;
	int round_num;
	int i;

	rap = rap_load(content_id);
	if(rap == NULL)
		return FALSE;

	aes_setkey_dec(&aes_ctxt, rap_init_key, RAP_KEYBITS);
	aes_crypt_ecb(&aes_ctxt, AES_DECRYPT, rap, rap);

	for (round_num = 0; round_num < 5; ++round_num)
	{
		for (i = 0; i < 16; ++i)
		{
			int p = rap_pbox[i];
			rap[p] ^= rap_e1[p];
		}
		for (i = 15; i >= 1; --i)
		{
			int p = rap_pbox[i];
			int pp = rap_pbox[i - 1];
			rap[p] ^= rap[pp];
		}
		int o = 0;
		for (i = 0; i < 16; ++i)
		{
			int p = rap_pbox[i];
			scetool::u8 kc = rap[p] - o;
			scetool::u8 ec2 = rap_e2[p];
			if (o != 1 || kc != 0xFF)
			{
				o = kc < ec2 ? 1 : 0;
				rap[p] = kc - ec2;
			}
			else if (kc == 0xFF)
				rap[p] = kc - ec2;
			else
				rap[p] = kc;
		}
	}

	memcpy(klicensee, rap, RAP_LENGTH);
	free(rap);

	return TRUE;
}

static scetool::u8 *idps_load()
{
	scetool::s8 *ps3 = NULL, path[256];
	scetool::u8 *idps;
	scetool::u32 len = 0;

	if((ps3 = getenv(CONFIG_ENV_PS3)) != NULL)
		if(_access(ps3, 0) != 0)
			ps3 = NULL;

	if(ps3 != NULL)
	{
		sprintf(path, "%s/%s", ps3, CONFIG_IDPS_FILE);
		if(_access(path, 0) != 0)
			sprintf(path, "%s/%s", CONFIG_IDPS_PATH, CONFIG_IDPS_FILE);
	}
	else
		sprintf(path, "%s/%s", CONFIG_IDPS_PATH, CONFIG_IDPS_FILE);

	idps = (scetool::u8 *)_read_buffer(path, &len);
	
	if(idps == NULL)
		return NULL;
	
	if(len != IDPS_LENGTH)
	{
		free(idps);
		return NULL;
	}
	
	return idps;
}

BOOL klicensee_by_content_id(const scetool::s8 *content_id, scetool::u8 *klicensee)
{
	aes_context aes_ctxt;

	if(rap_to_klicensee(content_id, klicensee) == FALSE)
	{
		keyset_t *ks_np_idps_const, *ks_np_rif_key;
		rif_t *rif;
		scetool::u8 idps_const[0x10];
		scetool::u8 act_dat_key[0x10];
		scetool::u32 act_dat_key_index;
		scetool::u8 *idps;
		act_dat_t *act_dat;

		if((idps = idps_load()) == NULL)
		{
			printf("[*] Error: Could not load IDPS.\n");
			return FALSE;
		}
		else
			_LOG_VERBOSE("IDPS loaded.\n");

		if((act_dat = act_dat_load()) == NULL)
		{
			printf("[*] Error: Could not load act.dat.\n");
			return FALSE;
		}
		else
			_LOG_VERBOSE("act.dat loaded.\n");

		ks_np_idps_const = keyset_find_by_name(CONFIG_NP_IDPS_CONST_KNAME);
		if(ks_np_idps_const == NULL)
			return FALSE;
		memcpy(idps_const, ks_np_idps_const->erk, 0x10);

		ks_np_rif_key = keyset_find_by_name(CONFIG_NP_RIF_KEY_KNAME);
		if(ks_np_rif_key == NULL)
			return FALSE;

		rif = rif_load(content_id);
		if(rif == NULL)
		{
			printf("[*] Error: Could not obtain klicensee for '%s'.\n", content_id);
			return FALSE;
		}

		aes_setkey_dec(&aes_ctxt, ks_np_rif_key->erk, RIF_KEYBITS);
		aes_crypt_ecb(&aes_ctxt, AES_DECRYPT, rif->act_key_index, rif->act_key_index);

		act_dat_key_index = _Es32(*(scetool::u32 *)(rif->act_key_index + 12));
		if(act_dat_key_index > 127)
		{
			printf("[*] Error: act.dat key index out of bounds.\n");
			return FALSE;
		}

		memcpy(act_dat_key, act_dat->primary_key_table + act_dat_key_index * BITS2BYTES(ACT_DAT_KEYBITS), BITS2BYTES(ACT_DAT_KEYBITS));

		aes_setkey_enc(&aes_ctxt, idps, IDPS_KEYBITS);
		aes_crypt_ecb(&aes_ctxt, AES_ENCRYPT, idps_const, idps_const);

		aes_setkey_dec(&aes_ctxt, idps_const, IDPS_KEYBITS);
		aes_crypt_ecb(&aes_ctxt, AES_DECRYPT, act_dat_key, act_dat_key);

		aes_setkey_dec(&aes_ctxt, act_dat_key, ACT_DAT_KEYBITS);
		aes_crypt_ecb(&aes_ctxt, AES_DECRYPT, rif->klicensee, klicensee);

		free(rif);

		_LOG_VERBOSE("klicensee decrypted.\n");
	}
	else
		_LOG_VERBOSE("klicensee converted from %s.rap.\n", content_id);

	return TRUE;
}

keyset_t *keyset_from_buffer(scetool::u8 *keyset)
{
	keyset_t *ks;

	if((ks = (keyset_t *)malloc(sizeof(keyset_t))) == NULL)
		return NULL;

	ks->erk = (scetool::u8 *)_memdup(keyset, 0x20);
	ks->erklen = 0x20;
	ks->riv = (scetool::u8 *)_memdup(keyset + 0x20, 0x10);
	ks->rivlen = 0x10;
	ks->pub = (scetool::u8 *)_memdup(keyset + 0x20 + 0x10, 0x28);
	ks->priv = (scetool::u8 *)_memdup(keyset + 0x20 + 0x10 + 0x28, 0x15);
	ks->ctype = (scetool::u8)*(keyset + 0x20 + 0x10 + 0x28 + 0x15);

	return ks;
}

keyset_t *keyset_find_by_name(const scetool::s8 *name)
{
	LIST_FOREACH(iter, _keysets)
	{
		keyset_t *ks = (keyset_t *)iter->value;
		if(strcmp(ks->name, name) == 0)
			return ks;
	}

	printf("[*] Error: Could not find keyset '%s'.\n", name);

	return NULL;
}

static keyset_t *_keyset_find_for_self(scetool::u32 self_type, scetool::u16 key_revision, scetool::u64 version)
{
	LIST_FOREACH(iter, _keysets)
	{
		keyset_t *ks = (keyset_t *)iter->value;

		if(ks->self_type == self_type)
		{
			switch(self_type)
			{
			case SELF_TYPE_LV0:
				return ks;
				break;
			case SELF_TYPE_LV1:
				if(version <= ks->version)
					return ks;
				break;
			case SELF_TYPE_LV2:
				if(version <= ks->version)
					return ks;
				break;
			case SELF_TYPE_APP:
				if(key_revision == ks->key_revision)
					return ks;
				break;
			case SELF_TYPE_ISO:
				if(version <= ks->version && key_revision == ks->key_revision)
					return ks;
				break;
			case SELF_TYPE_LDR:
				return ks;
				break;
			case SELF_TYPE_NPDRM:
				if(key_revision == ks->key_revision)
					return ks;
				break;
			}
		}
	}

	return NULL;
}

static keyset_t *_keyset_find_for_rvk(scetool::u32 key_revision)
{
	LIST_FOREACH(iter, _keysets)
	{
		keyset_t *ks = (keyset_t *)iter->value;

		if(ks->type == KEYTYPE_RVK && key_revision <= ks->key_revision)
			return ks;
	}

	return NULL;
}

static keyset_t *_keyset_find_for_pkg(scetool::u16 key_revision)
{
	LIST_FOREACH(iter, _keysets)
	{
		keyset_t *ks = (keyset_t *)iter->value;

		if(ks->type == KEYTYPE_PKG && key_revision <= ks->key_revision)
			return ks;
	}

	return NULL;
}

static keyset_t *_keyset_find_for_spp(scetool::u16 key_revision)
{
	LIST_FOREACH(iter, _keysets)
	{
		keyset_t *ks = (keyset_t *)iter->value;

		if(ks->type == KEYTYPE_SPP && key_revision <= ks->key_revision)
			return ks;
	}

	return NULL;
}

keyset_t *keyset_find(sce_buffer_ctxt_t *ctxt)
{
	keyset_t *res = NULL;

	switch(ctxt->sceh->header_type)
	{
	case SCE_HEADER_TYPE_SELF:
		res = _keyset_find_for_self(ctxt->self.ai->self_type, ctxt->sceh->key_revision, ctxt->self.ai->version);
		break;
	case SCE_HEADER_TYPE_RVK:
		res = _keyset_find_for_rvk(ctxt->sceh->key_revision);
		break;
	case SCE_HEADER_TYPE_PKG:
		res = _keyset_find_for_pkg(ctxt->sceh->key_revision);
		break;
	case SCE_HEADER_TYPE_SPP:
		res = _keyset_find_for_spp(ctxt->sceh->key_revision);
		break;
	}

	if(res == NULL)
		printf("[*] Error: Could not find keyset for %s.\n", _get_name(_sce_header_types, ctxt->sceh->header_type));

	return res;
}

static void _fill_property(keyset_t *ks, scetool::s8 *prop, scetool::s8 *value)
{
	if(strcmp(prop, "type") == 0)
	{
		if(strcmp(value, "SELF") == 0)
			ks->type = KEYTYPE_SELF;
		else if(strcmp(value, "RVK") == 0)
			ks->type = KEYTYPE_RVK;
		else if(strcmp(value, "PKG") == 0)
			ks->type = KEYTYPE_PKG;
		else if(strcmp(value, "SPP") == 0)
			ks->type = KEYTYPE_SPP;
		else if(strcmp(value, "OTHER") == 0)
			ks->type = KEYTYPE_OTHER;
		else
			printf("[*] Error: Unknown type '%s'.\n", value);
	}
	else if(strcmp(prop, "revision") == 0)
		ks->key_revision = (scetool::u16)_x_to_u64(value);
	else if(strcmp(prop, "version") == 0)
		ks->version = _x_to_u64(value);
	else if(strcmp(prop, "self_type") == 0)
	{
		if(strcmp(value, "LV0") == 0)
			ks->self_type = SELF_TYPE_LV0;
		else if(strcmp(value, "LV1") == 0)
			ks->self_type = SELF_TYPE_LV1;
		else if(strcmp(value, "LV2") == 0)
			ks->self_type = SELF_TYPE_LV2;
		else if(strcmp(value, "APP") == 0)
			ks->self_type = SELF_TYPE_APP;
		else if(strcmp(value, "ISO") == 0)
			ks->self_type = SELF_TYPE_ISO;
		else if(strcmp(value, "LDR") == 0)
			ks->self_type = SELF_TYPE_LDR;
		else if(strcmp(value, "UNK_7") == 0)
			ks->self_type = SELF_TYPE_UNK_7;
		else if(strcmp(value, "NPDRM") == 0)
			ks->self_type = SELF_TYPE_NPDRM;
		else
			printf("[*] Error: unknown SELF type '%s'.\n", value);
	}
	else if(strcmp(prop, "erk") == 0 || strcmp(prop, "key") == 0)
	{
		ks->erk = _x_to_u8_buffer(value);
		ks->erklen = strlen(value) / 2;
	}
	else if(strcmp(prop, "riv") == 0)
	{
		ks->riv = _x_to_u8_buffer(value);
		ks->rivlen = strlen(value) / 2;
	}
	else if(strcmp(prop, "pub") == 0)
		ks->pub = _x_to_u8_buffer(value);
	else if(strcmp(prop, "priv") == 0)
		ks->priv = _x_to_u8_buffer(value);
	else if(strcmp(prop, "ctype") == 0)
		ks->ctype = (scetool::u8)_x_to_u64(value);
	else
		printf("[*] Error: Unknown keyfile property '%s'.\n", prop);
}

BOOL curves_load(const scetool::s8 *cfile)
{
	scetool::u32 len = 0;

	_curves = (curve_t *)_read_buffer(cfile, &len);
	
	if(_curves == NULL)
		return FALSE;
	
	if(len != CURVES_LENGTH)
	{
		free(_curves);
		return FALSE;
	}
	
	return TRUE;
}

BOOL vsh_curves_load(const scetool::s8 *cfile)
{
	scetool::u32 len = 0;

	_vsh_curves = (vsh_curve_t *)_read_buffer(cfile, &len);
	
	if(_vsh_curves == NULL)
		return FALSE;
	
	if(len != VSH_CURVES_LENGTH)
	{
		free(_vsh_curves);
		return FALSE;
	}
	
	return TRUE;
}

static scetool::s64 _compare_keysets(keyset_t *ks1, keyset_t *ks2)
{
	scetool::s64 res;

	if((res = (scetool::s64)ks1->version - (scetool::s64)ks2->version) == 0)
		res = (scetool::s64)ks1->key_revision - (scetool::s64)ks2->key_revision;

	return res;
}

static void _sort_keysets()
{
	scetool::u32 i, to = _keysets->count;
	lnode_t *max;

	list_t *tmp = list_create();

	for(i = 0; i < to; i++)
	{
		max = _keysets->head;
		LIST_FOREACH(iter, _keysets)
		{
			if(_compare_keysets((keyset_t *)max->value, (keyset_t *)iter->value) < 0)
				max = iter;
		}
		list_push(tmp, max->value);
		list_remove_node(_keysets, max);
	}

	list_destroy(_keysets);
	_keysets = tmp;
}

#define LINEBUFSIZE 512
BOOL keys_load(const scetool::s8 *kfile)
{
	scetool::u32 i = 0, lblen;
	FILE *fp;
	scetool::s8 lbuf[LINEBUFSIZE];
	keyset_t *cks = NULL;

	if((_keysets = list_create()) == NULL)
		return FALSE;

	if((fp = fopen(kfile, "r")) == NULL)
	{
		list_destroy(_keysets);
		return FALSE;
	}

	do
	{
		//Get next line.
		lbuf[0] = 0;
		fgets(lbuf, LINEBUFSIZE, fp);
		lblen = strlen(lbuf);

		//Don't parse empty lines (ignore '\n') and comment lines (starting with '#').
		if(lblen > 1 && lbuf[0] != '#')
		{
			//Remove '\n'.
			lbuf[lblen-1] = 0;

			//Check for keyset entry.
			if(lblen > 2 && lbuf[0] == '[')
			{
				if(cks != NULL)
				{
					//Add to keyset list.
					list_push(_keysets, cks);
					cks = NULL;
				}

				//Find name end.
				for(i = 0; lbuf[i] != ']' && lbuf[i] != '\n' && i < lblen; i++);
				lbuf[i] = 0;

				//Allocate keyset and fill name.
				cks = (keyset_t *)malloc(sizeof(keyset_t));
				memset(cks, 0, sizeof(keyset_t));
				cks->name = _strdup(&lbuf[1]);
			}
			else if(cks != NULL)
			{
				//Find property name end.
				for(i = 0; lbuf[i] != '=' && lbuf[i] != '\n' && i < lblen; i++);
				lbuf[i] = 0;

				//Fill property.
				_fill_property(cks, &lbuf[0], &lbuf[i+1]);
			}
		}
	} while(!feof(fp));

	//Add last keyset to keyset list.
	if(cks != NULL)
		list_push(_keysets, cks);

	//Sort keysets.
	_sort_keysets();

	return TRUE;
}
#undef LINEBUFSIZE

bool frontend_decrypt(scetool::s8 *file_in, scetool::s8 *file_out)
{
	scetool::u8 *buf = _read_buffer(file_in, NULL);
	if(buf != NULL)
	{
		sce_buffer_ctxt_t *ctxt = sce_create_ctxt_from_buffer(buf);
		if(ctxt != NULL)
		{
			scetool::u8 *meta_info = NULL;
			if(_meta_info != NULL)
			{
				if(strlen(_meta_info) != 0x40*2)
				{
					ConLog.Error("scetool: Metadata info needs to be 64 bytes.\n");
					return false;
				}
				meta_info = _x_to_u8_buffer(_meta_info);
			}

			scetool::u8 *keyset = NULL;
			if(_keyset != NULL)
			{
				if(strlen(_keyset) != (0x20 + 0x10 + 0x15 + 0x28 + 0x01)*2)
				{
					ConLog.Error("scetool: Keyset has a wrong length");
					return false;
				}
				keyset = _x_to_u8_buffer(_keyset);
			}

			if(sce_decrypt_header(ctxt, meta_info, keyset))
			{
				if(sce_decrypt_data(ctxt))
				{
					if(ctxt->sceh->header_type == SCE_HEADER_TYPE_SELF)
					{
						if(self_write_to_elf(ctxt, file_out) == TRUE)
							ConLog.Write("scetool: ELF written to %s", file_out);
						else
							ConLog.Error("scetool: Could not write ELF");
					}
					else if(ctxt->sceh->header_type == SCE_HEADER_TYPE_RVK)
					{
						if(_write_buffer(file_out, ctxt->scebuffer + ctxt->metash[0].data_offset, 
							ctxt->metash[0].data_size + ctxt->metash[1].data_size))
							ConLog.Write("scetool: RVK written to %s", file_out);
						else
							ConLog.Error("scetool: Could not write RVK");
					}
					else if(ctxt->sceh->header_type == SCE_HEADER_TYPE_PKG)
					{
						/*if(_write_buffer(file_out, ctxt->scebuffer + ctxt->metash[0].data_offset, 
							ctxt->metash[0].data_size + ctxt->metash[1].data_size + ctxt->metash[2].data_size))
							printf("[*] PKG written to %s.\n", file_out);
						else
							printf("[*] Error: Could not write PKG.\n");*/
						ConLog.Warning("scetool: Not yet supported");
					}
					else if(ctxt->sceh->header_type == SCE_HEADER_TYPE_SPP)
					{
						if(_write_buffer(file_out, ctxt->scebuffer + ctxt->metash[0].data_offset, 
							ctxt->metash[0].data_size + ctxt->metash[1].data_size))
							ConLog.Write("scetool: SPP written to %s", file_out);
						else
							ConLog.Error("scetool: Could not write SPP");
					}
				}
				else
					ConLog.Error("scetool: Could not decrypt data");
			}
			else
				ConLog.Error("scetool: Could not decrypt header");
			free(ctxt);
		}
		else
			ConLog.Error("scetool: Could not process %s", file_in);
		free(buf);
	}
	else
		ConLog.Error("scetool: Could not load %s", file_in);
	return true;
}


bool scetool_decrypt(scetool::s8 *_file_in, scetool::s8 *_file_out)
{
	scetool::s8 *ps3 = NULL, path[256];

	//Try to get path from env:PS3.
	if((ps3 = getenv(CONFIG_ENV_PS3)) != NULL)
		if(_access(ps3, 0) != 0)
			ps3 = NULL;

	//Load keysets.
	if(ps3 != NULL)
	{
		sprintf(path, "%s/%s", ps3, CONFIG_KEYS_FILE);
		if(_access(path, 0) != 0)
			sprintf(path, "%s/%s", CONFIG_KEYS_PATH, CONFIG_KEYS_FILE);
	}
	else
		sprintf(path, "%s/%s", CONFIG_KEYS_PATH, CONFIG_KEYS_FILE);
	if(!keys_load(path))
	{
		ConLog.Error("scetool: Could not load keys");
		return false;
	}

	//Load curves.
	if(ps3 != NULL)
	{
		sprintf(path, "%s/%s", ps3, CONFIG_CURVES_FILE);
		if(_access(path, 0) != 0)
			sprintf(path, "%s/%s", CONFIG_CURVES_PATH, CONFIG_CURVES_FILE);
	}
	else
		sprintf(path, "%s/%s", CONFIG_CURVES_PATH, CONFIG_CURVES_FILE);
	if(!curves_load(path))
	{
		ConLog.Error("scetool: Could not load loader curves");
		return false;
	}

	//Load curves.
	if(ps3 != NULL)
	{
		sprintf(path, "%s/%s", ps3, CONFIG_VSH_CURVES_FILE);
		if(_access(path, 0) != 0)
			sprintf(path, "%s/%s", CONFIG_VSH_CURVES_PATH, CONFIG_VSH_CURVES_FILE);
	}
	else
		sprintf(path, "%s/%s", CONFIG_VSH_CURVES_PATH, CONFIG_VSH_CURVES_FILE);
	if(!vsh_curves_load(path))
	{
		ConLog.Error("scetool: Could not load vsh curves");
		return false;
	}

	//Set klicensee.
	if(_klicensee != NULL)
	{
		if(strlen(_klicensee) != 0x10*2)
		{
			ConLog.Error("scetool: klicensee needs to be 16 bytes.");
			return false;
		}
		np_set_klicensee(_x_to_u8_buffer(_klicensee));
	}

	
	return frontend_decrypt(_file_in, _file_out);;
}
