/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#ifndef _SPP_H_
#define _SPP_H_

#include "types.h"
#include "sce.h"

/*! SPP entry types. */
#define SPP_ENTRY_TYPE_1 1
#define SPP_ENTRY_TYPE_2 2

/*! SPP header. */
typedef struct _spp_header
{
	scetool::u16 unk1;
	scetool::u16 unk2;
	scetool::u32 spp_size;
	scetool::u32 unk3;
	scetool::u32 unk4;
	scetool::u64 unk5;
	scetool::u32 entcnt;
	scetool::u32 unk7;
} spp_header_t;

static inline void _es_spp_header(spp_header_t *h)
{
	h->unk1 = _Es16(h->unk1);
	h->unk2 = _Es16(h->unk2);
	h->spp_size = _Es32(h->spp_size);
	h->unk3 = _Es32(h->unk3);
	h->unk4 = _Es32(h->unk4);
	h->unk5 = _Es64(h->unk5);
	h->entcnt = _Es32(h->entcnt);
	h->unk7 = _Es32(h->unk7);
}

/*! SPP entry header. */
typedef struct _spp_entry_header
{
	scetool::u32 entry_size;
	scetool::u32 type;
	scetool::u64 laid;
	scetool::u64 paid;
	scetool::u8 name[0x20];
} spp_entry_header_t;

static inline void _es_spp_entry_header(spp_entry_header_t *h)
{
	h->entry_size = _Es32(h->entry_size);
	h->type = _Es32(h->type);
	h->laid = _Es64(h->laid);
	h->paid = _Es64(h->paid);
}

/*! Print SPP infos. */
void spp_print(FILE *fp, sce_buffer_ctxt_t *ctxt);

#endif
