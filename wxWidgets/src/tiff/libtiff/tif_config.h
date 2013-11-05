#ifdef __APPLE__

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `floor' function. */
#define HAVE_FLOOR 1

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the <GLUT/glut.h> header file. */
#define HAVE_GLUT_GLUT_H 1

/* Define to 1 if you have the <GL/glut.h> header file. */
#define HAVE_GL_GLUT_H 1

/* Define to 1 if you have the <GL/glu.h> header file. */
#define HAVE_GL_GLU_H 1

/* Define to 1 if you have the <GL/gl.h> header file. */
#define HAVE_GL_GL_H 1

/* Define as 0 or 1 according to the floating point format suported by the
   machine */
#define HAVE_IEEEFP 1

/* Define to 1 if the system has the type `int16'. */
/* #undef HAVE_INT16 */

/* Define to 1 if the system has the type `int32'. */
/* #undef HAVE_INT32 */

/* Define to 1 if the system has the type `int8'. */
/* #undef HAVE_INT8 */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
/* #undef HAVE_IO_H */

/* Define to 1 if you have the `isascii' function. */
#define HAVE_ISASCII 1

/* Define to 1 if you have the `jbg_newlen' function. */
/* #undef HAVE_JBG_NEWLEN */

/* Define to 1 if you have the `lfind' function. */
#define HAVE_LFIND 1

/* Define to 1 if you have the `m' library (-lm). */
/* #undef HAVE_LIBM */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mmap' function. */
#define HAVE_MMAP 1

/* Define to 1 if you have the <OpenGL/glu.h> header file. */
#define HAVE_OPENGL_GLU_H 1

/* Define to 1 if you have the <OpenGL/gl.h> header file. */
#define HAVE_OPENGL_GL_H 1

/* Define to 1 if you have the `pow' function. */
#define HAVE_POW 1

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Define to 1 if you have the <search.h> header file. */
#define HAVE_SEARCH_H 1

/* Define to 1 if you have the `setmode' function. */
#define HAVE_SETMODE 1

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the `strtol' function. */
#define HAVE_STRTOL 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if you have the `strtoull' function. */
#define HAVE_STRTOULL 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Use nonstandard varargs form for the GLU tesselator callback */
/* #undef HAVE_VARARGS_GLU_TESSCB */

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

#ifdef __BIG_ENDIAN__
    /* Set the native cpu bit order */
    #define HOST_FILLORDER FILLORDER_MSB2LSB
    
    /* Define to 1 if your processor stores words with the most significant byte
    first (like Motorola and SPARC, unlike Intel and VAX). */
    #define WORDS_BIGENDIAN 1
#else
    /* Set the native cpu bit order */
    #define HOST_FILLORDER FILLORDER_LSB2MSB
    
    /* Define to 1 if your processor stores words with the most significant byte
        first (like Motorola and SPARC, unlike Intel and VAX). */
    /* #undef WORDS_BIGENDIAN */
#endif

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
# ifndef inline
#  define inline __inline
# endif
#endif

/* Support ISO JBIG compression (requires JBIG-KIT library) */
/* #undef JBIG_SUPPORT */

/* 8/12 bit libjpeg dual mode enabled */
/* #undef JPEG_DUAL_MODE_8_12 */

/* Support JPEG compression (requires IJG JPEG library) */
/* #undef JPEG_SUPPORT */

/* 12bit libjpeg primary include file with path */
/* #undef LIBJPEG_12_PATH */

/* Support LogLuv high dynamic range encoding */
#define LOGLUV_SUPPORT 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Support LZMA2 compression */
/* #undef LZMA_SUPPORT */

/* Support LZW algorithm */
#define LZW_SUPPORT 1

/* Support Microsoft Document Imaging format */
#define MDI_SUPPORT 1

/* Support NeXT 2-bit RLE algorithm */
#define NEXT_SUPPORT 1

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Support Old JPEG compresson (read-only) */
/* #undef OJPEG_SUPPORT */

/* Support Macintosh PackBits algorithm */
#define PACKBITS_SUPPORT 1

/* Support Pixar log-format algorithm (requires Zlib) */
#define PIXARLOG_SUPPORT 1

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* The size of `signed int', as computed by sizeof. */
#define SIZEOF_SIGNED_INT 4

/* The size of `signed long', as computed by sizeof. */
#define SIZEOF_SIGNED_LONG 4

/* The size of `signed long long', as computed by sizeof. */
#define SIZEOF_SIGNED_LONG_LONG 8

/* The size of `signed short', as computed by sizeof. */
#define SIZEOF_SIGNED_SHORT 2

/* The size of `unsigned char *', as computed by sizeof. */
#define SIZEOF_UNSIGNED_CHAR_P 4

/* The size of `unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT 4

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG 4

/* The size of `unsigned long long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* The size of `unsigned short', as computed by sizeof. */
#define SIZEOF_UNSIGNED_SHORT 2

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Support strip chopping (whether or not to convert single-strip uncompressed
   images to mutiple strips of specified size to reduce memory usage) */
#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP

/* Default size of the strip in bytes (when strip chopping enabled) */
#define STRIP_SIZE_DEFAULT 8192

/* Enable SubIFD tag (330) support */
#define SUBIFD_SUPPORT 1

/* Support ThunderScan 4-bit RLE algorithm */
#define THUNDER_SUPPORT 1

/* Signed 16-bit type */
#define TIFF_INT16_T signed short

/* Signed 32-bit type formatter */
#define TIFF_INT32_FORMAT "%d"

/* Signed 32-bit type */
#define TIFF_INT32_T signed int

/* Signed 64-bit type formatter */
#define TIFF_INT64_FORMAT "%lld"

/* Signed 64-bit type */
#define TIFF_INT64_T signed long long

/* Signed 8-bit type */
#define TIFF_INT8_T signed char

/* Pointer difference type formatter */
#define TIFF_PTRDIFF_FORMAT "%ld"

/* Pointer difference type */
#define TIFF_PTRDIFF_T ptrdiff_t

/* Signed size type formatter */
#define TIFF_SSIZE_FORMAT "%ld"

/* Signed size type */
#define TIFF_SSIZE_T signed long

/* Unsigned 16-bit type */
#define TIFF_UINT16_T unsigned short

/* Unsigned 32-bit type formatter */
#define TIFF_UINT32_FORMAT "%u"

/* Unsigned 32-bit type */
#define TIFF_UINT32_T unsigned int

/* Unsigned 64-bit type formatter */
#define TIFF_UINT64_FORMAT "%llu"

/* Unsigned 64-bit type */
#define TIFF_UINT64_T unsigned long long

/* Unsigned 8-bit type */
#define TIFF_UINT8_T unsigned char

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

#else /* __APPLE__ */

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define as 0 or 1 according to the floating point format suported by the
   machine */
#define HAVE_IEEEFP 1

/* Define to 1 if you have the `jbg_newlen' function. */
#define HAVE_JBG_NEWLEN 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
#define HAVE_IO_H 1

/* Define to 1 if you have the <search.h> header file. */
#define HAVE_SEARCH_H 1

/* Define to 1 if you have the `setmode' function. */
#define HAVE_SETMODE 1

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Signed 64-bit type formatter */
#define TIFF_INT64_FORMAT "%I64d"

/* Signed 64-bit type */
#define TIFF_INT64_T signed __int64

/* Unsigned 64-bit type formatter */
#define TIFF_UINT64_FORMAT "%I64u"

/* Unsigned 64-bit type */
#define TIFF_UINT64_T unsigned __int64

/* Set the native cpu bit order */
#define HOST_FILLORDER FILLORDER_LSB2MSB

#define snprintf _snprintf

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
# ifndef inline
#  define inline __inline
# endif
#endif

#if !defined (__BORLANDC__) && !defined (__WATCOMC__)
   #define lfind _lfind
#endif

#ifdef __DMC__
#define HAVE_INT32
typedef	long int32;
#endif

#ifdef _WIN32_WCE
#   undef HAVE_FCNTL_H
#   undef HAVE_SYS_TYPES_H

    /*
       CE headers don't define these standard constants (not even underscored
       versions), provide our own replacements as they seem to be only used in
       libtiff own code anyhow.
     */
#   define   O_RDONLY    0x0000
#   define   O_WRONLY    0x0001
#   define   O_RDWR      0x0002
#   define   O_CREAT     0x0100
#   define   O_TRUNC     0x0200
#   define   O_EXCL      0x0400
#endif /* _WIN32_WCE */

#endif /* __APPLE__/!__APPLE__ */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
