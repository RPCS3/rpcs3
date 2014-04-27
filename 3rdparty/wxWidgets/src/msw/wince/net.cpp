///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/net.cpp
// Purpose:
// Author:
// Modified by:
// Created:
// RCS-ID:      $Id: net.cpp 41054 2006-09-07 19:01:45Z ABX $
// Copyright:   Copyright 1998, Ben Goetter.  All rights reserved.
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

/*
 patch holes in winsock

  WCE 2.0 lacks many of the 'database' winsock routines.
  Stub just enough them for ss.dll.

  getprotobynumber
  getservbyport
  getservbyname

*/

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/msw/wrapwin.h"
#endif

#include <tchar.h>
#include <winsock.h>
#include <string.h>
#include "wx/msw/wince/net.h"


#define CCH_MAX_PROTO 4

static struct protoent RgProtoEnt[] =
{
 { "tcp", {NULL}, 6 },
 { "udp", {NULL}, 17 },
 { "icmp", {NULL}, 1 },
 { "ip", {NULL}, 0 },
 { NULL, {NULL}, 0 }
};


#define CCH_MAX_SERV 8

// Ordered by most likely to be requested.
// Assumes that a service available on different protocols
// will use the same port number on each protocol.
// Should that be no longer the case,
// remove the fFoundOnce code from getservbyXxx fcns.

// This table keeps port numbers in host byte order.

static struct servent RgServEnt[] =
{
 { "ftp", {NULL}, 21, "tcp" },
 { "ftp-data", {NULL}, 20, "tcp" },
 { "telnet", {NULL}, 23, "tcp" },
 { "smtp", {NULL}, 25, "tcp" },
 { "http", {NULL}, 80, "tcp" },
 { "http", {NULL}, 80, "udp" },
 { "pop", {NULL}, 109, "tcp" },
 { "pop2", {NULL}, 109, "tcp" },
 { "pop3", {NULL}, 110, "tcp" },
 { "nntp", {NULL}, 119, "tcp" },
 { "finger", {NULL}, 79, "tcp" },
 /* include most of the simple TCP services for testing */
 { "echo", {NULL}, 7, "tcp" },
 { "echo", {NULL}, 7, "udp" },
 { "discard", {NULL}, 9, "tcp" },
 { "discard", {NULL}, 9, "udp" },
 { "chargen", {NULL}, 19, "tcp" },
 { "chargen", {NULL}, 19, "udp" },
 { "systat", {NULL}, 11, "tcp" },
 { "systat", {NULL}, 11, "udp" },
 { "daytime", {NULL}, 13, "tcp" },
 { "daytime", {NULL}, 13, "udp" },
 { "netstat", {NULL}, 15, "tcp" },
 { "qotd", {NULL}, 17, "tcp" },
 { "qotd", {NULL}, 17, "udp" },
 { NULL, {NULL}, 0, NULL }
};

// Since table kept in host byte order,
// return this element to callers

static struct servent ServEntReturn = {0};

// Because CE doesn't have _stricmp - that's why.

static void strcpyLC(char* szDst, const char* szSrc, int cch)
{
 int i;
 char ch;
 for (i = 0, ch = szSrc[i]; i < cch && ch != 0; ch = szSrc[++i])
 {
  szDst[i] = (ch >= 'A' && ch <= 'Z') ? (ch + ('a'-'A')) : ch;
 } szDst[i] = 0;
}


struct servent * WINSOCKAPI getservbyport(int port, const char * proto)
{

 port = ntohs((unsigned short)port); // arrives in network byte order
 struct servent *ps = &RgServEnt[0];
 BOOL fFoundOnce = FALSE; // flag to short-circuit search through rest of db

 // Make a lowercase version for comparison
 // truncate to 1 char longer than any value in table
 char szProtoLC[CCH_MAX_PROTO+2];
 if (NULL != proto)
  strcpyLC(szProtoLC, proto, CCH_MAX_PROTO+1);

 while (NULL != ps->s_name)
 {
  if (port == ps->s_port)
  {
   fFoundOnce = TRUE;
   if (NULL == proto || !strcmp(szProtoLC, ps->s_proto))
   {
    ServEntReturn = *ps;
    ServEntReturn.s_port = htons(ps->s_port);
    return &ServEntReturn;
   }
  }
  else if (fFoundOnce)
   break;
  ++ps;
 } return NULL;
}


struct servent * WINSOCKAPI getservbyname(const char * name,
                                          const char * proto)
{
 struct servent *ps = &RgServEnt[0];
 BOOL fFoundOnce = FALSE; // flag to short-circuit search through rest of db

 // Make lowercase versions for comparisons
 // truncate to 1 char longer than any value in table
 char szNameLC[CCH_MAX_SERV+2];
 strcpyLC(szNameLC, name, CCH_MAX_SERV+1);
 char szProtoLC[CCH_MAX_PROTO+2];
 if (NULL != proto)
  strcpyLC(szProtoLC, proto, CCH_MAX_PROTO+1);

 while (NULL != ps->s_name)
 {
  if (!strcmp(szNameLC, ps->s_name))
  {
   fFoundOnce = TRUE;
   if (NULL == proto || !strcmp(szProtoLC, ps->s_proto))
   {
    ServEntReturn = *ps;
    ServEntReturn.s_port = htons(ps->s_port);
    return &ServEntReturn;
   }
  }
  else if (fFoundOnce)
   break;
  ++ps;
 } return NULL;
}


struct protoent * WINSOCKAPI getprotobynumber(int proto)
{
 struct protoent *pr = &RgProtoEnt[0];
 while (NULL != pr->p_name)
 {
  if (proto == pr->p_proto)
   return pr;
  ++pr;
 } return NULL;
}
