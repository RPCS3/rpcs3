/*================================================================
** Copyright 2000, Clark Cooper
** All rights reserved.
**
** This is free software. You are permitted to copy, distribute, or modify
** it under the terms of the MIT/X license (contained in the COPYING file
** with this distribution.)
*/

#ifndef OS2CONFIG_H
#define OS2CONFIG_H

#include <memory.h>
#include <string.h>

#define XML_NS 1
#define XML_DTD 1
#define XML_CONTEXT_BYTES 1024

/* we will assume all OS2 platforms are little endian */
#define BYTEORDER 1234

/* OS2 has memmove() available. */
#define HAVE_MEMMOVE

#endif /* ndef OS2CONFIG_H */
