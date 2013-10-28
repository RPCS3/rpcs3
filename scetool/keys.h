/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#ifndef _KEYS_H_
#define _KEYS_H_

#include "types.h"
#include "sce.h"

#define KEYBITS(klen) BYTES2BITS(klen)

#define KEYTYPE_SELF 1
#define KEYTYPE_RVK 2
#define KEYTYPE_PKG 3
#define KEYTYPE_SPP 4
#define KEYTYPE_OTHER 5

/*! Flag to use VSH curve. */
#define USE_VSH_CURVE 0x40

/*! Length of whole curves file. */
#define CURVES_LENGTH 0x1E40
#define CTYPE_MIN 0
#define CTYPE_MAX 63

/*! Length of the whole VSH curves file. */
#define VSH_CURVES_LENGTH 0x168
#define VSH_CTYPE_MIN 0
#define VSH_CTYPE_MAX 2

/*! Length of the idps, act.dat, .rif and .rap files. */
#define IDPS_LENGTH 0x10
#define ACT_DAT_LENGTH 0x1038
#define RIF_LENGTH 0x98
#define RAP_LENGTH 0x10

/*! IDPS, RIF, act.dat key lengths. */
#define IDPS_KEYBITS 128
#define ACT_DAT_KEYBITS 128
#define RIF_KEYBITS 128
#define RAP_KEYBITS 128

/*! Keyset. */
typedef struct _keyset
{
	/*! Name. */
	scetool::s8 *name;
	/*! Type. */
	scetool::u32 type;
	/*! Key revision. */
	scetool::u16 key_revision;
	/*! Version. */
	scetool::u64 version;
	/*! SELF type. */
	scetool::u32 self_type;
	/*! Key length. */
	scetool::u32 erklen;
	/*! Key. */
	scetool::u8 *erk;
	/*! IV length. */
	scetool::u32 rivlen;
	/*! IV. */
	scetool::u8 *riv;
	/*! Pub. */
	scetool::u8 *pub;
	/*! Priv. */
	scetool::u8 *priv;
	/*! Curve type. */
	scetool::u8 ctype;
} keyset_t;

/*! Curve entry. */
typedef struct _curve
{
	scetool::u8 p[20];
	scetool::u8 a[20];
	scetool::u8 b[20];
	scetool::u8 N[21];
	scetool::u8 Gx[20];
	scetool::u8 Gy[20];
} curve_t;

/*! VSH Curve entry. */
typedef struct _vsh_curve
{
	scetool::u8 a[20];
	scetool::u8 b[20];
	scetool::u8 N[20];
	scetool::u8 p[20];
	scetool::u8 Gx[20];
	scetool::u8 Gy[20];
} vsh_curve_t;

/*! act.dat. */
typedef struct _act_dat
{
	scetool::u8 account_info[16];
    scetool::u8 primary_key_table[2048];
    scetool::u8 secondary_key_table[2048];
    scetool::u8 signature[40];
} act_dat_t;

/*! RIF. */
typedef struct _rif
{
	scetool::u8 account_info[16];
	scetool::u8 content_id[48];
	scetool::u8 act_key_index[16];
	scetool::u8 klicensee[16];
	scetool::u64 timestamp;
	scetool::u64 zero;
	scetool::u8 signature[40];
} rif_t;

BOOL keys_load(const scetool::s8 *kfile);
keyset_t *keyset_find(sce_buffer_ctxt_t *ctxt);
keyset_t *keyset_find_by_name(const scetool::s8 *name);

BOOL curves_load(const scetool::s8 *cfile);
curve_t *curve_find(scetool::u8 ctype);

BOOL vsh_curves_load(const scetool::s8 *cfile);
curve_t *vsh_curve_find(scetool::u8 ctype);

BOOL klicensee_by_content_id(const scetool::s8 *content_id, scetool::u8 *klicensee);

keyset_t *keyset_from_buffer(scetool::u8 *keyset);

#endif
