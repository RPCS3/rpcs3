///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/net.cpp
// Purpose:
// Author:
// Modified by:
// Created:
// RCS-ID:      $Id: net.cpp 58995 2009-02-18 15:58:25Z JS $
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

Subject: Re: Could you provide a license for your wxWidgets v2.6.2 files
From: Ben Goetter <goetter@mazama.net>
Date: Wed, 22 Mar 2006 09:58:32 -0800
To: Julian Smart <julian@anthemion.co.uk>
Return-Path: <goetter@mazama.net>
Delivered-To: jsmart@gotadsl.co.uk
Envelope-To: jsmart@gotadsl.co.uk
Received: (qmail 6858 invoked from network); 22 Mar 2006 17:58:46 -0000
Received: from unknown (HELO as004.apm-internet.net) (85.119.248.22) by mail001.apm-internet.net with SMTP; 22 Mar 2006 17:58:46 -0000
Received: (qmail 47565 invoked from network); 22 Mar 2006 17:58:46 -0000
X-Spam-Score: 0.1
X-Spam-Checker-Version: SpamAssassin 3.1.0 (2005-09-13) on as004.apm-internet.net
X-Spam-Report: * 0.1 AWL AWL: From: address is in the auto white-list
Received: from unknown (HELO av004.apm-internet.net) (85.119.248.18) by as004.apm-internet.net with SMTP; 22 Mar 2006 17:58:46 -0000
Received: (qmail 33684 invoked by uid 1013); 22 Mar 2006 17:58:44 -0000
Received: from unknown (HELO relay002.apm-internet.net) (85.119.248.12) by av004.apm-internet.net with SMTP; 22 Mar 2006 17:58:44 -0000
Received: (qmail 4919 invoked from network); 22 Mar 2006 17:58:43 -0000
Received: from unknown (HELO mini-131.dolphin-server.co.uk) (80.87.138.131) by relay002.apm-internet.net with SMTP; 22 Mar 2006 17:58:44 -0000
Received: (qmail 28982 invoked by uid 64020); 22 Mar 2006 09:59:50 -0000
Delivered-To: anthemion.co.uk-julian@anthemion.co.uk
Received: (qmail 28980 invoked from network); 22 Mar 2006 09:59:50 -0000
Received: from unknown (HELO bgkh-household.seattle.mazama.net) (216.231.59.183) by mini-131.dolphin-server.co.uk with SMTP; 22 Mar 2006 09:59:50 -0000
Received: from [192.168.0.128] (dhcp128.seattle.mazama.net [192.168.0.128]) (using TLSv1 with cipher DHE-RSA-AES256-SHA (256/256 bits)) (Client did not present a certificate) by bgkh-household.seattle.mazama.net (Postfix) with ESMTP id 5AE2417020 for <julian@anthemion.co.uk>; Wed, 22 Mar 2006 09:58:42 -0800 (PST)
Message-ID: <44219048.9020000@mazama.net>
User-Agent: Thunderbird 1.5 (Windows/20051201)
MIME-Version: 1.0
References: <8C8EF65853BB6842809053B779B8CAB70E416E95@emss07m14.us.lmco.com> <4420DD8C.6090405@mazama.net> <6.2.1.2.2.20060322091301.0315cc90@pop3.gotadsl.co.uk>
In-Reply-To: <6.2.1.2.2.20060322091301.0315cc90@pop3.gotadsl.co.uk>
Content-Type: text/plain; charset=ISO-8859-1; format=flowed
Content-Transfer-Encoding: 8bit
X-Anti-Virus: Kaspersky Anti-Virus for MailServers 5.5.3/RELEASE, bases: 22032006 #172373, status: clean

Certainly.

I hereby release the text of those three functions -- getprotobynumber, getservbyport, and getservbyname, as found in wxWidgets-2.6.2\include\wx\msw\wince\net.h  and wxWidgets-2.6.2\src\msw\wince\net.cpp -- under the wxWindows license.  Please feel free to replace "All rights reserved." with "Licensed under the wxWindows License" or equivalent boilerplate.

Absolutely no problem,
Ben


Julian Smart wrote:
> Hi Ben,
>
> Thanks a lot for the clarification, and indeed for the code! Apologies if
> we didn't ask for permission to use those functions.
>
> Would it be possible to have these 3 functions licensed under
> the wxWindows License as well? This would simplify the legal
> position for us rather than having multiple licenses. The wxWindows
> License is here:
>
> http://www.wxwidgets.org/newlicen.htm
>
> Best regards,
>
> Julian


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
