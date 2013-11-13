/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>

#include "types.h"

/*! Verbose. */
extern BOOL _verbose;
#define _LOG_VERBOSE(...) _IF_VERBOSE(printf("[*] " __VA_ARGS__))
#define _IF_VERBOSE(code) \
	do \
	{ \
		if(_verbose == TRUE) \
		{ \
			code; \
		} \
	} while(0)

/*! Raw. */
extern BOOL _raw;
#define _PRINT_RAW(fp, ...) _IF_RAW(fprintf(fp, __VA_ARGS__))
#define _IF_RAW(code) \
	do \
	{ \
		if(_raw == TRUE) \
		{ \
			code; \
		} \
	} while(0)

/*! ID to name entry. */
typedef struct _id_to_name
{
	scetool::u64 id;
	const scetool::s8 *name;
} id_to_name_t;

/*! Utility functions. */
void _hexdump(FILE *fp, const char *name, scetool::u32 offset, scetool::u8 *buf, int len, BOOL print_addr);
void _print_align(FILE *fp, const scetool::s8 *str, scetool::s32 align, scetool::s32 len);
scetool::u8 *_read_buffer(const scetool::s8 *file, scetool::u32 *length);
int _write_buffer(const scetool::s8 *file, scetool::u8 *buffer, scetool::u32 length);
const scetool::s8 *_get_name(id_to_name_t *tab, scetool::u64 id);
scetool::u64 _get_id(id_to_name_t *tab, const scetool::s8 *name);
void _zlib_inflate(scetool::u8 *in, scetool::u64 len_in, scetool::u8 *out, scetool::u64 len_out);
void _zlib_deflate(scetool::u8 *in, scetool::u64 len_in, scetool::u8 *out, scetool::u64 len_out);
scetool::u8 _get_rand_byte();
void _fill_rand_bytes(scetool::u8 *dst, scetool::u32 len);
void _memcpy_inv(scetool::u8 *dst, scetool::u8 *src, scetool::u32 len);
void *_memdup(void *ptr, scetool::u32 size);
scetool::u64 _x_to_u64(const scetool::s8 *hex);
scetool::u8 *_x_to_u8_buffer(const scetool::s8 *hex);

#endif
