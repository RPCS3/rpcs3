/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <tamtypes.h>

#ifdef BIG_ENDIAN
inline unsigned int   ntohl(x) { return x; }
inline unsigned short ntohs(x) { return x; }
inline unsigned int   ntohl(x) { return x; }
inline unsigned short ntohs(x) { return x; }
#else
// LITTLE_ENDIAN
inline unsigned int
htonl(unsigned int x)
{
    return ((x & 0xff) << 24 ) |
        ((x & 0xff00) << 8 ) |
        ((x & 0xff0000) >> 8 ) |
        ((x & 0xff000000) >> 24 );
}

inline unsigned short 
htons(unsigned short x)
{
    return ((x & 0xff) << 8 ) | ((x & 0xff00) >> 8 );
}

#define ntohl	htonl
#define ntohs	htons

#endif

#define IP4_ADDR(ipaddr, a,b,c,d) (ipaddr)->s_addr = htonl(((u32)(a & 0xff) << 24) | ((u32)(b & 0xff) << 16 ) | ((u32)(c & 0xff) << 8) | (u32)(d & 0xff))

#endif /* BYTEORDER_H */
