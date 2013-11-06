/*
 *  xtiffio.h -- Public interface to Extended TIFF tags
 *
 *    This is a template for defining a client module
 *    which supports tag extensions to the standard libtiff
 *    set. Only portions of the code marked "XXX" need to
 *    be changed to support your tag set.
 *
 *    written by: Niles D. Ritter
 */

#ifndef __xtiffio_h
#define __xtiffio_h

#include "tiffio.h"

/* 
 *  XXX Define your private Tag names and values here 
 */

/* These tags are not valid, but are provided for example */
#define TIFFTAG_EXAMPLE_MULTI      61234
#define TIFFTAG_EXAMPLE_SINGLE     61235
#define TIFFTAG_EXAMPLE_ASCII      61236

/* 
 *  XXX Define Printing method flags. These
 *  flags may be passed in to TIFFPrintDirectory() to
 *  indicate that those particular field values should
 *  be printed out in full, rather than just an indicator
 *  of whether they are present or not.
 */
#define	TIFFPRINT_MYMULTIDOUBLES	0x80000000

/**********************************************************************
 *    Nothing below this line should need to be changed by the user.
 **********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

extern TIFF* XTIFFOpen(const char* name, const char* mode);
extern TIFF* XTIFFFdOpen(int fd, const char* name, const char* mode);
extern void  XTIFFClose(TIFF *tif);

#if defined(__cplusplus)
}
#endif

#endif /* __xtiffio_h */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
