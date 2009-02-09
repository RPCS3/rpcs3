/* This file is part of a Windows32 DLL Interface to:
   GNU gettext - internationalization aids
   Copyright (C) 1996, 1998 Free Software Foundation, Inc.

   This file was written by Franco Bez <franco.bez@gmx.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.  */

/* REPLACEMENT FOR ORIGINAL LIBINTL.H for use with Windows32 */

#if !defined(__LIBINTL_H_INCLUDED)
#define __LIBINTL_H_INCLUDED

#if defined(__cplusplus)
extern "C" { 
#endif

/* See if we allready know what we want static or dll linkage or none at all*/
#if defined DONT_USE_GETTEXT || ( defined USE_SAFE_GETTEXT_DLL && defined USE_GETTEXT_STATIC ) || ( defined USE_GETTEXT_DLL && defined USE_SAFE_GETTEXT_DLL ) || ( defined USE_GETTEXT_DLL && defined USE_GETTEXT_STATIC ) 
/* TWO IS HARDLY POSSIBLE */
#undef  USE_GETTEXT_DLL
#undef  USE_GETTEXT_STATIC
#undef USE_SAFE_GETTEXT_DLL
#endif  /* MORE THAN ONE - OR NONE AT ALL */

#if !defined USE_GETTEXT_DLL && !defined USE_SAFE_GETTEXT_DLL && !defined USE_GETTEXT_STATIC && !defined DONT_USE_GETTEXT
/* not explicitly defined so try to guess it - 
   if GNUC is used - we use static linkage by default
           because at the moment this is the only plattform
	   for which a static lib is available
   else we use the DLL built with GNUC */   
# if defined __GNUC__
#  define USE_GETTEXT_STATIC
# else
#  define USE_GETTEXT_DLL
# endif /* __GNUC__ */
#endif  /* NONE */

/* NOW ONLY ONE OF
   DONT_USE_GETTEXT , USE_GETTEXT_DLL , USE_SAFE_GETTEXT_DLL , USE_GETTEXT_STATIC 
   IS DEFINED */

#if defined USE_GETTEXT_DLL
/* exported functions in DLL gnu_gettext.dll 
   you should link with import library 
    -lgnu_gettext (for mingw32) OR  gnu_gettext.lib (MSVC) */
__declspec(dllimport) char *gettext(const char *__msgid);
__declspec(dllimport) char *dgettext(const char *__domainname,const char *__msgid);
__declspec(dllimport) char *dcgettext(const char *__domainname,const char *__msgid, int __category);
__declspec(dllimport) char *textdomain(const char *__domainname);
__declspec(dllimport) char *bindtextdomain(const char *__domainname,const char *__dirname);
/* calling _putenv from within the DLL */
__declspec(dllexport) int gettext_putenv(const char *envstring);
#endif /* DLL */

#if defined USE_SAFE_GETTEXT_DLL
/* Uses DLL gnu_gettext.dll ONLY if present, otherwise NO translation will take place
   you should link with "safe_gettext_dll.o -lstdc++" see README for safe_gettext_dll for Details  */
/* The safe gettext functions */
extern char *gettext(const char *szMsgId);
extern char *dgettext(const char *szDomain,const char *szMsgId);
extern char *dcgettext(const char *szDomain,const char *szMsgId,int iCategory);
extern char *textdomain(const char *szDomain);
extern char *bindtextdomain(const char *szDomain,const char *szDirectory);
/* calling _putenv from within the DLL */
extern int gettext_putenv(const char *envstring);
#endif /* SAFE DLL */

#if defined USE_GETTEXT_STATIC
/* exported functions in static library libintl.a 
   and supporting macros
   you should link with -lintl (mingw32) */
extern char *gettext__(const char *__msgid);
extern char *dgettext__(const char *__domainname,const char *__msgid);
extern char *dcgettext__(const char *__domainname,const char *__msgid, int __category);
extern char *textdomain__(const char *__domainname);
extern char *bindtextdomain__(const char *__domainname,const char *__dirname);
#define gettext(szMsgId) gettext__(szMsgId)
#define dgettext(szDomain,szMsgId) dgettext__(szDomain,szMsgId)
#define dcgettext(szDomain,szMsgId,iCategory) dcgettext__(szDomain,szMsgId,iCategory)
#define textdomain(szDomain) textdomain__(szDomain)
#define bindtextdomain(szDomain,szDirectory) bindtextdomain__(szDomain,szDirectory)
// dummy - for static linkage -  calling _putenv from within the DLL
#define gettext_putenv(a) _putenv(a)
#endif /* STATIC */

#if defined DONT_USE_GETTEXT
/* DON'T USE GETTEXT AT ALL
   MAKROS TO MAKE CODE COMPILE WELL, BUT GETTEXT WILL NOT BE USESD
*/
# define gettext(Msgid) (Msgid)
# define dgettext(Domainname, Msgid) (Msgid)
# define dcgettext(Domainname, Msgid, Category) (Msgid)
# define textdomain(Domainname) ((char *) Domainname)
# define bindtextdomain(Domainname, Dirname) ((char *) Dirname)
// dummy - for static linkage -  calling _putenv from within the DLL
# define gettext_putenv(a) _putenv(a)
#endif /* DON'T USE AT ALL */

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /*!defined(__LIBINTL_H_INCLUDED)*/
