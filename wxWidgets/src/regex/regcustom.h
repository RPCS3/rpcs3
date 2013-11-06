/*
 * Copyright (c) 1998, 1999 Henry Spencer.  All rights reserved.
 * 
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results.  The author
 * thanks all of them. 
 * 
 * Redistribution and use in source and binary forms -- with or without
 * modification -- are permitted for any purpose, provided that
 * redistributions in source form retain this entire copyright notice and
 * indicate the origin and nature of any modifications.
 * 
 * I'd appreciate being given credit for this package in the documentation
 * of software which uses it, but that is not a requirement.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * HENRY SPENCER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* overrides for regguts.h definitions, if any */
/* regguts only includes standard headers if NULL is not defined, so do it
 * ourselves here */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

/* must include this after ctype.h inclusion for CodeWarrior/Mac */
#include "wx/defs.h"
#include "wx/chartype.h"
#include "wx/wxcrtbase.h"

/*
 * Do not insert extras between the "begin" and "end" lines -- this
 * chunk is automatically extracted to be fitted into regex.h.
 */
/* --- begin --- */
/* ensure certain things don't sneak in from system headers */
#ifdef __REG_WIDE_T
#undef __REG_WIDE_T
#endif
#ifdef __REG_WIDE_COMPILE
#undef __REG_WIDE_COMPILE
#endif
#ifdef __REG_WIDE_EXEC
#undef __REG_WIDE_EXEC
#endif
#ifdef __REG_REGOFF_T
#undef __REG_REGOFF_T
#endif
#ifdef __REG_VOID_T
#undef __REG_VOID_T
#endif
#ifdef __REG_CONST
#undef __REG_CONST
#endif
#ifdef __REG_NOFRONT
#undef __REG_NOFRONT
#endif
#ifdef __REG_NOCHAR
#undef __REG_NOCHAR
#endif
#if wxUSE_UNICODE
#   define  __REG_WIDE_T        wxChar
#   define  __REG_WIDE_COMPILE  wx_re_comp
#   define  __REG_WIDE_EXEC     wx_re_exec
#   define  __REG_NOCHAR        /* don't want the char versions */
#endif
#define __REG_NOFRONT           /* don't want regcomp() and regexec() */
#define _ANSI_ARGS_(x)          x
/* --- end --- */

/* internal character type and related */
typedef wxChar chr;             /* the type itself */
typedef int pchr;               /* what it promotes to */
typedef unsigned uchr;          /* unsigned type that will hold a chr */
typedef int celt;               /* type to hold chr, MCCE number, or NOCELT */
#define NOCELT  (-1)            /* celt value which is not valid chr or MCCE */
#define UCHAR(c) ((unsigned char) (c))
#define CHR(c)  (UCHAR(c))      /* turn char literal into chr literal */
#define DIGITVAL(c) ((c)-'0')   /* turn chr digit into its value */
#if !wxUSE_UNICODE
#   define CHRBITS 8            /* bits in a chr; must not use sizeof */
#   define CHR_MIN 0x00         /* smallest and largest chr; the value */
#   define CHR_MAX 0xff         /*  CHR_MAX-CHR_MIN+1 should fit in uchr */
#elif SIZEOF_WCHAR_T == 4
#   define CHRBITS 32           /* bits in a chr; must not use sizeof */
#   define CHR_MIN 0x00000000   /* smallest and largest chr; the value */
#   define CHR_MAX 0xffffffff   /*  CHR_MAX-CHR_MIN+1 should fit in uchr */
#else
#   define CHRBITS 16           /* bits in a chr; must not use sizeof */
#   define CHR_MIN 0x0000       /* smallest and largest chr; the value */
#   define CHR_MAX 0xffff       /*  CHR_MAX-CHR_MIN+1 should fit in uchr */
#endif

/*
 * I'm using isalpha et al. instead of wxIsalpha since BCC 5.5's iswalpha
 * seems not to work on Windows 9x? Note that these are only used by the
 * lexer, and although they must work for wxChars, they need only return
 * true for characters within the ascii range.
 */
#define iscalnum(x)     ((wxUChar)(x) < 128 && isalnum(x))
#define iscalpha(x)     ((wxUChar)(x) < 128 && isalpha(x))
#define iscdigit(x)     ((wxUChar)(x) < 128 && isdigit(x))
#define iscspace(x)     ((wxUChar)(x) < 128 && isspace(x))

/* name the external functions */
#define compile         wx_re_comp
#define exec            wx_re_exec

/* enable/disable debugging code (by whether REG_DEBUG is defined or not) */
#if 0           /* no debug unless requested by makefile */
#define REG_DEBUG       /* */
#endif

/* and pick up the standard header */
#include "regex.h"
