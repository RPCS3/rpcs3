/******************************************************************************
 *
 * Project:  GeoTIFF Overview Builder
 * Purpose:  Mainline for building overviews in a TIFF file.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log: addtiffo.c,v $
 * Revision 1.7  2010-06-08 18:55:15  bfriesen
 * * contrib: Add an emacs formatting mode footer to all source files
 * so that emacs can be effectively used.
 *
 * Revision 1.6  2005/12/16 05:59:55  fwarmerdam
 * Major upgrade to support YCbCr subsampled jpeg images
 *
 * Revision 1.4  2004/09/21 13:31:23  dron
 * Add missed include string.h.
 *
 * Revision 1.3  2000/04/18 22:48:31  warmerda
 * Added support for averaging resampling
 *
 * Revision 1.2  2000/01/28 15:36:38  warmerda
 * pass TIFF handle instead of filename to overview builder
 *
 * Revision 1.1  1999/08/17 01:47:59  warmerda
 * New
 *
 * Revision 1.1  1999/03/12 17:46:32  warmerda
 * New
 *
 * Revision 1.2  1999/02/11 22:27:12  warmerda
 * Added multi-sample support
 *
 * Revision 1.1  1999/02/11 18:12:30  warmerda
 * New
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiffio.h"

void TIFFBuildOverviews( TIFF *, int, int *, int, const char *,
                         int (*)(double,void*), void * );

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int argc, char ** argv )

{
    int		anOverviews[100];   /* TODO: un-hardwire array length, flexible allocate */
    int		nOverviewCount = 0;
    int		bUseSubIFD = 0;
    TIFF	*hTIFF;
    const char  *pszResampling = "nearest";

/* -------------------------------------------------------------------- */
/*      Usage:                                                          */
/* -------------------------------------------------------------------- */
    if( argc < 2 )
    {
        printf( "Usage: addtiffo [-r {nearest,average,mode}]\n"
                "                tiff_filename [resolution_reductions]\n"
                "\n"
                "Example:\n"
                " %% addtiffo abc.tif 2 4 8 16\n" );
        return( 1 );
    }

    while( argv[1][0] == '-' )
    {
        if( strcmp(argv[1],"-subifd") == 0 )
        {
            bUseSubIFD = 1;
            argv++;
            argc--;
        }
        else if( strcmp(argv[1],"-r") == 0 )
        {
            argv += 2;
            argc -= 2;
            pszResampling = *argv;
        }
        else
        {
            fprintf( stderr, "Incorrect parameters\n" );
            return( 1 );
        }
    }

    /* TODO: resampling mode parameter needs to be encoded in an integer from this point on */

/* -------------------------------------------------------------------- */
/*      Collect the user requested reduction factors.                   */
/* -------------------------------------------------------------------- */
    while( nOverviewCount < argc - 2 && nOverviewCount < 100 )
    {
        anOverviews[nOverviewCount] = atoi(argv[nOverviewCount+2]);
        if( anOverviews[nOverviewCount] <= 0)
        {
            fprintf( stderr, "Incorrect parameters\n" );
            return(1);
        }
        nOverviewCount++;
    }

/* -------------------------------------------------------------------- */
/*      Default to four overview levels.  It would be nicer if it       */
/*      defaulted based on the size of the source image.                */
/* -------------------------------------------------------------------- */
    /* TODO: make it default based on the size of the source image */
    if( nOverviewCount == 0 )
    {
        nOverviewCount = 4;

        anOverviews[0] = 2;
        anOverviews[1] = 4;
        anOverviews[2] = 8;
        anOverviews[3] = 16;
    }

/* -------------------------------------------------------------------- */
/*      Build the overview.                                             */
/* -------------------------------------------------------------------- */
    hTIFF = TIFFOpen( argv[1], "r+" );
    if( hTIFF == NULL )
    {
        fprintf( stderr, "TIFFOpen(%s) failed.\n", argv[1] );
        return( 1 );
    }

    TIFFBuildOverviews( hTIFF, nOverviewCount, anOverviews, bUseSubIFD,
                        pszResampling, NULL, NULL );

    TIFFClose( hTIFF );

/* -------------------------------------------------------------------- */
/*      Optionally test for memory leaks.                               */
/* -------------------------------------------------------------------- */
#ifdef DBMALLOC
    malloc_dump(1);
#endif

    return( 0 );
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
