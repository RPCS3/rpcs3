/* config.h  */

#ifndef PTW32_CONFIG_H
#define PTW32_CONFIG_H

/*********************************************************************
 * Defaults: see target specific redefinitions below.
 *********************************************************************/

/* We're building the pthreads-win32 library */
#define PTW32_BUILD

/* Do we know about the C type sigset_t? */
#undef HAVE_SIGSET_T

/* Define if you have the <signal.h> header file.  */
#define HAVE_SIGNAL_H

/* Define if you have the Borland TASM32 or compatible assembler.  */
#undef HAVE_TASM32

/* Define if you don't have Win32 DuplicateHandle. (eg. WinCE) */
#undef NEED_DUPLICATEHANDLE

/* Define if you don't have Win32 _beginthreadex. (eg. WinCE) */
// PCSX2 : This is preferred when using the shared CRT, which we do! -- air
#define NEED_CREATETHREAD

/* Define if you don't have Win32 errno. (eg. WinCE) */
#undef NEED_ERRNO

/* Define if you don't have Win32 calloc. (eg. WinCE)  */
#undef NEED_CALLOC

/* Define if you don't have Win32 ftime. (eg. WinCE)  */
#undef NEED_FTIME

/* Define if you don't have Win32 semaphores. (eg. WinCE 2.1 or earlier)  */
#undef NEED_SEM

/* Define if you need to convert string parameters to unicode. (eg. WinCE)  */
#undef NEED_UNICODE_CONSTS

/* Define if your C (not C++) compiler supports "inline" functions. */
#define HAVE_C_INLINE

/* Do we know about type mode_t? */
#undef HAVE_MODE_T

/* Define if you have the timespec struct */
#undef HAVE_STRUCT_TIMESPEC

/* Define if you don't have the GetProcessAffinityMask() */
#undef NEED_PROCESS_AFFINITY_MASK

/*
# ----------------------------------------------------------------------
# The library can be built with some alternative behaviour to better
# facilitate development of applications on Win32 that will be ported
# to other POSIX systems.
#
# Nothing described here will make the library non-compliant and strictly
# compliant applications will not be affected in any way, but
# applications that make assumptions that POSIX does not guarantee are
# not strictly compliant and may fail or misbehave with some settings.
#
# PTW32_THREAD_ID_REUSE_INCREMENT
# Purpose:
# POSIX says that applications should assume that thread IDs can be
# recycled. However, Solaris (and some other systems) use a [very large]
# sequence number as the thread ID, which provides virtual uniqueness.
# This provides a very high but finite level of safety for applications
# that are not meticulous in tracking thread lifecycles e.g. applications
# that call functions which target detached threads without some form of
# thread exit synchronisation.
#
# Usage:
# Set to any value in the range: 0 <= value < 2^wordsize.
# Set to 0 to emulate reusable thread ID behaviour like Linux or *BSD.
# Set to 1 for unique thread IDs like Solaris (this is the default).
# Set to some factor of 2^wordsize to emulate smaller word size types
# (i.e. will wrap sooner). This might be useful to emulate some embedded
# systems.
#
# define PTW32_THREAD_ID_REUSE_INCREMENT 0
#
# ----------------------------------------------------------------------
 */
#undef PTW32_THREAD_ID_REUSE_INCREMENT


/*********************************************************************
 * Target specific groups
 *
 * If you find that these are incorrect or incomplete please report it
 * to the pthreads-win32 maintainer. Thanks.
 *********************************************************************/
#ifdef WINCE
#define NEED_DUPLICATEHANDLE
#define NEED_CREATETHREAD
#define NEED_ERRNO
#define NEED_CALLOC
#define NEED_FTIME
//#define NEED_SEM
#define NEED_UNICODE_CONSTS
#define NEED_PROCESS_AFFINITY_MASK
#endif

#ifdef _UWIN
#define HAVE_MODE_T
#define HAVE_STRUCT_TIMESPEC
#endif

#ifdef __GNUC__
#define HAVE_C_INLINE
#endif

#ifdef __MINGW32__
#define HAVE_MODE_T
#endif

#ifdef __BORLANDC__
#endif

#ifdef __WATCOMC__
#endif

#ifdef __DMC__
#define HAVE_SIGNAL_H
#define HAVE_C_INLINE
#endif



#endif
