/******************************************************************************
 * tif_overview.c,v 1.9 2005/05/25 09:03:16 dron Exp
 *
 * Project:  TIFF Overview Builder
 * Purpose:  Library function for building overviews in a TIFF file.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 * Notes:
 *  o Currently only images with bits_per_sample of a multiple of eight
 *    will work.
 *
 *  o The downsampler currently just takes the top left pixel from the
 *    source rectangle.  Eventually sampling options of averaging, mode, and
 *    ``center pixel'' should be offered.
 *
 *  o The code will attempt to use the same kind of compression,
 *    photometric interpretation, and organization as the source image, but
 *    it doesn't copy geotiff tags to the reduced resolution images.
 *
 *  o Reduced resolution overviews for multi-sample files will currently
 *    always be generated as PLANARCONFIG_SEPARATE.  This could be fixed
 *    reasonable easily if needed to improve compatibility with other
 *    packages.  Many don't properly support PLANARCONFIG_SEPARATE. 
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
 */

/* TODO: update notes in header above */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tiffio.h"
#include "tif_ovrcache.h"

#ifndef FALSE
#  define FALSE 0
#  define TRUE 1
#endif

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

void TIFFBuildOverviews( TIFF *, int, int *, int, const char *,
                         int (*)(double,void*), void * );

/************************************************************************/
/*                         TIFF_WriteOverview()                         */
/*                                                                      */
/*      Create a new directory, without any image data for an overview. */
/*      Returns offset of newly created overview directory, but the     */
/*      current directory is reset to be the one in used when this      */
/*      function is called.                                             */
/************************************************************************/

uint32 TIFF_WriteOverview( TIFF *hTIFF, uint32 nXSize, uint32 nYSize,
                           int nBitsPerPixel, int nPlanarConfig, int nSamples, 
                           int nBlockXSize, int nBlockYSize,
                           int bTiled, int nCompressFlag, int nPhotometric,
                           int nSampleFormat,
                           unsigned short *panRed,
                           unsigned short *panGreen,
                           unsigned short *panBlue,
                           int bUseSubIFDs,
                           int nHorSubsampling, int nVerSubsampling )

{
    toff_t	nBaseDirOffset;
    toff_t	nOffset;

    (void) bUseSubIFDs;

    nBaseDirOffset = TIFFCurrentDirOffset( hTIFF );

    TIFFCreateDirectory( hTIFF );

/* -------------------------------------------------------------------- */
/*      Setup TIFF fields.                                              */
/* -------------------------------------------------------------------- */
    TIFFSetField( hTIFF, TIFFTAG_IMAGEWIDTH, nXSize );
    TIFFSetField( hTIFF, TIFFTAG_IMAGELENGTH, nYSize );
    if( nSamples == 1 )
        TIFFSetField( hTIFF, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
    else
        TIFFSetField( hTIFF, TIFFTAG_PLANARCONFIG, nPlanarConfig );

    TIFFSetField( hTIFF, TIFFTAG_BITSPERSAMPLE, nBitsPerPixel );
    TIFFSetField( hTIFF, TIFFTAG_SAMPLESPERPIXEL, nSamples );
    TIFFSetField( hTIFF, TIFFTAG_COMPRESSION, nCompressFlag );
    TIFFSetField( hTIFF, TIFFTAG_PHOTOMETRIC, nPhotometric );
    TIFFSetField( hTIFF, TIFFTAG_SAMPLEFORMAT, nSampleFormat );

    if( bTiled )
    {
        TIFFSetField( hTIFF, TIFFTAG_TILEWIDTH, nBlockXSize );
        TIFFSetField( hTIFF, TIFFTAG_TILELENGTH, nBlockYSize );
    }
    else
        TIFFSetField( hTIFF, TIFFTAG_ROWSPERSTRIP, nBlockYSize );

    TIFFSetField( hTIFF, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE );

    if( nPhotometric == PHOTOMETRIC_YCBCR || nPhotometric == PHOTOMETRIC_ITULAB )
    {
        TIFFSetField( hTIFF, TIFFTAG_YCBCRSUBSAMPLING, nHorSubsampling, nVerSubsampling);
        /* TODO: also write YCbCrPositioning and YCbCrCoefficients tag identical to source IFD */
    }
    /* TODO: add command-line parameter for selecting jpeg compression quality
     * that gets ignored when compression isn't jpeg */

/* -------------------------------------------------------------------- */
/*	Write color table if one is present.				*/
/* -------------------------------------------------------------------- */
    if( panRed != NULL )
    {
        TIFFSetField( hTIFF, TIFFTAG_COLORMAP, panRed, panGreen, panBlue );
    }

/* -------------------------------------------------------------------- */
/*      Write directory, and return byte offset.                        */
/* -------------------------------------------------------------------- */
    if( TIFFWriteCheck( hTIFF, bTiled, "TIFFBuildOverviews" ) == 0 )
        return 0;

    TIFFWriteDirectory( hTIFF );
    TIFFSetDirectory( hTIFF, (tdir_t) (TIFFNumberOfDirectories(hTIFF)-1) );

    nOffset = TIFFCurrentDirOffset( hTIFF );

    TIFFSetSubDirectory( hTIFF, nBaseDirOffset );

    return nOffset;
}

/************************************************************************/
/*                       TIFF_GetSourceSamples()                        */
/************************************************************************/

static void 
TIFF_GetSourceSamples( double * padfSamples, unsigned char *pabySrc, 
                       int nPixelBytes, int nSampleFormat, 
                       uint32 nXSize, uint32 nYSize, 
                       int nPixelOffset, int nLineOffset )
{
    uint32  iXOff, iYOff;
    int     iSample;

    iSample = 0;

    for( iYOff = 0; iYOff < nYSize; iYOff++ )
    {
        for( iXOff = 0; iXOff < nXSize; iXOff++ )
        {
            unsigned char *pabyData;

            pabyData = pabySrc + iYOff * nLineOffset + iXOff * nPixelOffset;

            if( nSampleFormat == SAMPLEFORMAT_UINT && nPixelBytes == 1 )
            {
                padfSamples[iSample++] = *pabyData;
            }
            else if( nSampleFormat == SAMPLEFORMAT_UINT && nPixelBytes == 2 )
            {
                padfSamples[iSample++] = ((uint16 *) pabyData)[0];
            }
            else if( nSampleFormat == SAMPLEFORMAT_UINT && nPixelBytes == 4 )
            {
                padfSamples[iSample++] = ((uint32 *) pabyData)[0];
            }
            else if( nSampleFormat == SAMPLEFORMAT_INT && nPixelBytes == 2 )
            {
                padfSamples[iSample++] = ((int16 *) pabyData)[0];
            }
            else if( nSampleFormat == SAMPLEFORMAT_INT && nPixelBytes == 32 )
            {
                padfSamples[iSample++] = ((int32 *) pabyData)[0];
            }
            else if( nSampleFormat == SAMPLEFORMAT_IEEEFP && nPixelBytes == 4 )
            {
                padfSamples[iSample++] = ((float *) pabyData)[0];
            }
            else if( nSampleFormat == SAMPLEFORMAT_IEEEFP && nPixelBytes == 8 )
            {
                padfSamples[iSample++] = ((double *) pabyData)[0];
            }
        }
    }
} 

/************************************************************************/
/*                           TIFF_SetSample()                           */
/************************************************************************/

static void 
TIFF_SetSample( unsigned char * pabyData, int nPixelBytes, int nSampleFormat, 
                double dfValue )

{
    if( nSampleFormat == SAMPLEFORMAT_UINT && nPixelBytes == 1 )
    {
        *pabyData = (unsigned char) MAX(0,MIN(255,dfValue));
    }
    else if( nSampleFormat == SAMPLEFORMAT_UINT && nPixelBytes == 2 )
    {
        *((uint16 *)pabyData) = (uint16) MAX(0,MIN(65535,dfValue));
    }
    else if( nSampleFormat == SAMPLEFORMAT_UINT && nPixelBytes == 4 )
    {
        *((uint32 *)pabyData) = (uint32) dfValue;
    }
    else if( nSampleFormat == SAMPLEFORMAT_INT && nPixelBytes == 2 )
    {
        *((int16 *)pabyData) = (int16) MAX(-32768,MIN(32767,dfValue));
    }
    else if( nSampleFormat == SAMPLEFORMAT_INT && nPixelBytes == 32 )
    {
        *((int32 *)pabyData) = (int32) dfValue;
    }
    else if( nSampleFormat == SAMPLEFORMAT_IEEEFP && nPixelBytes == 4 )
    {
        *((float *)pabyData) = (float) dfValue;
    }
    else if( nSampleFormat == SAMPLEFORMAT_IEEEFP && nPixelBytes == 8 )
    {
        *((double *)pabyData) = dfValue;
    }
}

/************************************************************************/
/*                          TIFF_DownSample()                           */
/*                                                                      */
/*      Down sample a tile of full res data into a window of a tile     */
/*      of downsampled data.                                            */
/************************************************************************/

static
void TIFF_DownSample( unsigned char *pabySrcTile,
                      uint32 nBlockXSize, uint32 nBlockYSize,
                      int nPixelSkewBits, int nBitsPerPixel,
                      unsigned char * pabyOTile,
                      uint32 nOBlockXSize, uint32 nOBlockYSize,
                      uint32 nTXOff, uint32 nTYOff, int nOMult,
                      int nSampleFormat, const char * pszResampling )

{
    uint32	i, j;
    int         k, nPixelBytes = (nBitsPerPixel) / 8;
    int		nPixelGroupBytes = (nBitsPerPixel+nPixelSkewBits)/8;
    unsigned char *pabySrc, *pabyDst;
    double      *padfSamples;

    assert( nBitsPerPixel >= 8 );

    padfSamples = (double *) malloc(sizeof(double) * nOMult * nOMult);

/* ==================================================================== */
/*      Loop over scanline chunks to process, establishing where the    */
/*      data is going.                                                  */
/* ==================================================================== */
    for( j = 0; j*nOMult < nBlockYSize; j++ )
    {
        if( j + nTYOff >= nOBlockYSize )
            break;

        pabyDst = pabyOTile + ((j+nTYOff)*nOBlockXSize + nTXOff)
            * nPixelBytes * nPixelGroupBytes;

/* -------------------------------------------------------------------- */
/*      Handler nearest resampling ... we don't even care about the     */
/*      data type, we just do a bytewise copy.                          */
/* -------------------------------------------------------------------- */
        if( strncmp(pszResampling,"nearest",4) == 0
            || strncmp(pszResampling,"NEAR",4) == 0 )
        {
            pabySrc = pabySrcTile + j*nOMult*nBlockXSize * nPixelGroupBytes;

            for( i = 0; i*nOMult < nBlockXSize; i++ )
            {
                if( i + nTXOff >= nOBlockXSize )
                    break;
            
                /*
                 * For now use simple subsampling, from the top left corner
                 * of the source block of pixels.
                 */

                for( k = 0; k < nPixelBytes; k++ )
                    pabyDst[k] = pabySrc[k];

                pabyDst += nPixelBytes * nPixelGroupBytes;
                pabySrc += nOMult * nPixelGroupBytes;
            }
        }

/* -------------------------------------------------------------------- */
/*      Handle the case of averaging.  For this we also have to         */
/*      handle each sample format we are concerned with.                */
/* -------------------------------------------------------------------- */
        else if( strncmp(pszResampling,"averag",6) == 0
                 || strncmp(pszResampling,"AVERAG",6) == 0 )
        {
            pabySrc = pabySrcTile + j*nOMult*nBlockXSize * nPixelGroupBytes;

            for( i = 0; i*nOMult < nBlockXSize; i++ )
            {
                double   dfTotal;
                uint32   nXSize, nYSize, iSample;

                if( i + nTXOff >= nOBlockXSize )
                    break;

                nXSize = MIN((uint32)nOMult,nBlockXSize-i);
                nYSize = MIN((uint32)nOMult,nBlockYSize-j);

                TIFF_GetSourceSamples( padfSamples, pabySrc,
                                       nPixelBytes, nSampleFormat,
                                       nXSize, nYSize,
                                       nPixelGroupBytes,
                                       nPixelGroupBytes * nBlockXSize );

                dfTotal = 0;
                for( iSample = 0; iSample < nXSize*nYSize; iSample++ )
                {
                    dfTotal += padfSamples[iSample];
                }

                TIFF_SetSample( pabyDst, nPixelBytes, nSampleFormat, 
                                dfTotal / (nXSize*nYSize) );

                pabySrc += nOMult * nPixelGroupBytes;
                pabyDst += nPixelBytes;
            }
        }
    }

    free( padfSamples );
}

/************************************************************************/
/*                     TIFF_DownSample_Subsampled()                     */
/************************************************************************/
static
void TIFF_DownSample_Subsampled( unsigned char *pabySrcTile, int nSample,
                                 uint32 nBlockXSize, uint32 nBlockYSize,
                                 unsigned char * pabyOTile,
                                 uint32 nOBlockXSize, uint32 nOBlockYSize,
                                 uint32 nTXOff, uint32 nTYOff, int nOMult,
                                 const char *pszResampling,
                                 int nHorSubsampling, int nVerSubsampling )
{
    /* TODO: test with variety of subsampling values, and incovinient tile/strip sizes */
    int nSampleBlockSize;
    int nSourceSampleRowSize;
    int nDestSampleRowSize;
    uint32  nSourceX, nSourceY;
    uint32  nSourceXSec, nSourceYSec;
    uint32  nSourceXSecEnd, nSourceYSecEnd;
    uint32  nDestX, nDestY;
    int nSampleOffsetInSampleBlock;
    unsigned int nCummulator;
    unsigned int nCummulatorCount;

    nSampleBlockSize = nHorSubsampling * nVerSubsampling + 2;
    nSourceSampleRowSize = 
        ( ( nBlockXSize + nHorSubsampling - 1 ) / nHorSubsampling ) * nSampleBlockSize;
    nDestSampleRowSize = 
        ( ( nOBlockXSize + nHorSubsampling - 1 ) / nHorSubsampling ) * nSampleBlockSize;

    if( strncmp(pszResampling,"nearest",4) == 0
        || strncmp(pszResampling,"NEAR",4) == 0 )
    {
    	if( nSample == 0 )
        {
            for( nSourceY = 0, nDestY = nTYOff; 
                 nSourceY < nBlockYSize; 
                 nSourceY += nOMult, nDestY ++)
            {
                if( nDestY >= nOBlockYSize )
                    break;

                for( nSourceX = 0, nDestX = nTXOff; 
                     nSourceX < nBlockXSize; 
                     nSourceX += nOMult, nDestX ++)
                {
                    if( nDestX >= nOBlockXSize )
                        break;

                    * ( pabyOTile + ( nDestY / nVerSubsampling ) * nDestSampleRowSize
                        + ( nDestY % nVerSubsampling ) * nHorSubsampling
                        + ( nDestX / nHorSubsampling ) * nSampleBlockSize
                        + ( nDestX % nHorSubsampling ) ) =
                        * ( pabySrcTile + ( nSourceY / nVerSubsampling ) * nSourceSampleRowSize
                            + ( nSourceY % nVerSubsampling ) * nHorSubsampling
                            + ( nSourceX / nHorSubsampling ) * nSampleBlockSize
                            + ( nSourceX % nHorSubsampling ) );
                }
            }
        }
        else
        {
            nSampleOffsetInSampleBlock = nHorSubsampling * nVerSubsampling + nSample - 1;
            for( nSourceY = 0, nDestY = ( nTYOff / nVerSubsampling ); 
                 nSourceY < ( nBlockYSize / nVerSubsampling );
                 nSourceY += nOMult, nDestY ++)
            {
                if( nDestY*nVerSubsampling >= nOBlockYSize )
                    break;

            	for( nSourceX = 0, nDestX = ( nTXOff / nHorSubsampling ); 
                     nSourceX < ( nBlockXSize / nHorSubsampling );
                     nSourceX += nOMult, nDestX ++)
                {
                    if( nDestX*nHorSubsampling >= nOBlockXSize )
                        break;

                    * ( pabyOTile + nDestY * nDestSampleRowSize
                        + nDestX * nSampleBlockSize
                        + nSampleOffsetInSampleBlock ) =
                    	* ( pabySrcTile + nSourceY * nSourceSampleRowSize
                            + nSourceX * nSampleBlockSize
                            + nSampleOffsetInSampleBlock );
                }
            }
        }
    }
    else if( strncmp(pszResampling,"averag",6) == 0
             || strncmp(pszResampling,"AVERAG",6) == 0 )
    {
    	if( nSample == 0 )
        {
            for( nSourceY = 0, nDestY = nTYOff; nSourceY < nBlockYSize; nSourceY += nOMult, nDestY ++)
            {
                if( nDestY >= nOBlockYSize )
                    break;

                for( nSourceX = 0, nDestX = nTXOff; nSourceX < nBlockXSize; nSourceX += nOMult, nDestX ++)
                {
                    if( nDestX >= nOBlockXSize )
                        break;

                    nSourceXSecEnd = nSourceX + nOMult;
                    if( nSourceXSecEnd > nBlockXSize )
                        nSourceXSecEnd = nBlockXSize;
                    nSourceYSecEnd = nSourceY + nOMult;
                    if( nSourceYSecEnd > nBlockYSize )
                        nSourceYSecEnd = nBlockYSize;
                    nCummulator = 0;
                    for( nSourceYSec = nSourceY; nSourceYSec < nSourceYSecEnd; nSourceYSec ++)
                    {
                        for( nSourceXSec = nSourceX; nSourceXSec < nSourceXSecEnd; nSourceXSec ++)
                        {
                            nCummulator += * ( pabySrcTile + ( nSourceYSec / nVerSubsampling ) * nSourceSampleRowSize
                                               + ( nSourceYSec % nVerSubsampling ) * nHorSubsampling
                                               + ( nSourceXSec / nHorSubsampling ) * nSampleBlockSize
                                               + ( nSourceXSec % nHorSubsampling ) );
                        }
                    }
                    nCummulatorCount = ( nSourceXSecEnd - nSourceX ) * ( nSourceYSecEnd - nSourceY );
                    * ( pabyOTile + ( nDestY / nVerSubsampling ) * nDestSampleRowSize
                        + ( nDestY % nVerSubsampling ) * nHorSubsampling
                        + ( nDestX / nHorSubsampling ) * nSampleBlockSize
                        + ( nDestX % nHorSubsampling ) ) =
                        ( ( nCummulator + ( nCummulatorCount >> 1 ) ) / nCummulatorCount );
                }
            }
        }
        else
        {
            nSampleOffsetInSampleBlock = nHorSubsampling * nVerSubsampling + nSample - 1;
            for( nSourceY = 0, nDestY = ( nTYOff / nVerSubsampling ); nSourceY < ( nBlockYSize / nVerSubsampling );
                 nSourceY += nOMult, nDestY ++)
            {
                if( nDestY*nVerSubsampling >= nOBlockYSize )
                    break;

                for( nSourceX = 0, nDestX = ( nTXOff / nHorSubsampling ); nSourceX < ( nBlockXSize / nHorSubsampling );
                     nSourceX += nOMult, nDestX ++)
                {
                    if( nDestX*nHorSubsampling >= nOBlockXSize )
                        break;

                    nSourceXSecEnd = nSourceX + nOMult;
                    if( nSourceXSecEnd > ( nBlockXSize / nHorSubsampling ) )
                        nSourceXSecEnd = ( nBlockXSize / nHorSubsampling );
                    nSourceYSecEnd = nSourceY + nOMult;
                    if( nSourceYSecEnd > ( nBlockYSize / nVerSubsampling ) )
                        nSourceYSecEnd = ( nBlockYSize / nVerSubsampling );
                    nCummulator = 0;
                    for( nSourceYSec = nSourceY; nSourceYSec < nSourceYSecEnd; nSourceYSec ++)
                    {
                        for( nSourceXSec = nSourceX; nSourceXSec < nSourceXSecEnd; nSourceXSec ++)
                        {
                            nCummulator += * ( pabySrcTile + nSourceYSec * nSourceSampleRowSize
                                               + nSourceXSec * nSampleBlockSize
                                               + nSampleOffsetInSampleBlock );
                        }
                    }
                    nCummulatorCount = ( nSourceXSecEnd - nSourceX ) * ( nSourceYSecEnd - nSourceY );
                    * ( pabyOTile + nDestY * nDestSampleRowSize
                        + nDestX * nSampleBlockSize
                        + nSampleOffsetInSampleBlock ) =
                        ( ( nCummulator + ( nCummulatorCount >> 1 ) ) / nCummulatorCount );
                }
            }
        }
    }
}

/************************************************************************/
/*                      TIFF_ProcessFullResBlock()                      */
/*                                                                      */
/*      Process one block of full res data, downsampling into each      */
/*      of the overviews.                                               */
/************************************************************************/

void TIFF_ProcessFullResBlock( TIFF *hTIFF, int nPlanarConfig,
                               int bSubsampled,
                               int nHorSubsampling, int nVerSubsampling,
                               int nOverviews, int * panOvList,
                               int nBitsPerPixel,
                               int nSamples, TIFFOvrCache ** papoRawBIs,
                               uint32 nSXOff, uint32 nSYOff,
                               unsigned char *pabySrcTile,
                               uint32 nBlockXSize, uint32 nBlockYSize,
                               int nSampleFormat, const char * pszResampling )
    
{
    int		iOverview, iSample;

    for( iSample = 0; iSample < nSamples; iSample++ )
    {
        /*
         * We have to read a tile/strip for each sample for
         * PLANARCONFIG_SEPARATE.  Otherwise, we just read all the samples
         * at once when handling the first sample.
         */
        if( nPlanarConfig == PLANARCONFIG_SEPARATE || iSample == 0 )
        {
            if( TIFFIsTiled(hTIFF) )
            {
                TIFFReadEncodedTile( hTIFF,
                                     TIFFComputeTile(hTIFF, nSXOff, nSYOff,
                                                     0, (tsample_t)iSample ),
                                     pabySrcTile,
                                     TIFFTileSize(hTIFF));
            }
            else
            {
                TIFFReadEncodedStrip( hTIFF,
                                      TIFFComputeStrip(hTIFF, nSYOff,
                                                       (tsample_t) iSample),
                                      pabySrcTile,
                                      TIFFStripSize(hTIFF) );
            }
        }

        /*        
         * Loop over destination overview layers
         */
        for( iOverview = 0; iOverview < nOverviews; iOverview++ )
        {
            TIFFOvrCache *poRBI = papoRawBIs[iOverview];
            unsigned char *pabyOTile;
            uint32  nTXOff, nTYOff, nOXOff, nOYOff, nOMult;
            uint32  nOBlockXSize = poRBI->nBlockXSize;
            uint32  nOBlockYSize = poRBI->nBlockYSize;
            int     nSkewBits, nSampleByteOffset; 

            /*
             * Fetch the destination overview tile
             */
            nOMult = panOvList[iOverview];
            nOXOff = (nSXOff/nOMult) / nOBlockXSize;
            nOYOff = (nSYOff/nOMult) / nOBlockYSize;

            if( bSubsampled )
            {
                pabyOTile = TIFFGetOvrBlock_Subsampled( poRBI, nOXOff, nOYOff );

                /*
                 * Establish the offset into this tile at which we should
                 * start placing data.
                 */
                nTXOff = (nSXOff - nOXOff*nOMult*nOBlockXSize) / nOMult;
                nTYOff = (nSYOff - nOYOff*nOMult*nOBlockYSize) / nOMult;


#ifdef DBMALLOC
                malloc_chain_check( 1 );
#endif
                TIFF_DownSample_Subsampled( pabySrcTile, iSample,
                                            nBlockXSize, nBlockYSize,
                                            pabyOTile,
                                            poRBI->nBlockXSize, poRBI->nBlockYSize,
                                            nTXOff, nTYOff,
                                            nOMult, pszResampling,
                                            nHorSubsampling, nVerSubsampling );
#ifdef DBMALLOC
                malloc_chain_check( 1 );
#endif

            }
            else
            {

                pabyOTile = TIFFGetOvrBlock( poRBI, nOXOff, nOYOff, iSample );

                /*
                 * Establish the offset into this tile at which we should
                 * start placing data.
                 */
                nTXOff = (nSXOff - nOXOff*nOMult*nOBlockXSize) / nOMult;
                nTYOff = (nSYOff - nOYOff*nOMult*nOBlockYSize) / nOMult;

                /*
                 * Figure out the skew (extra space between ``our samples'') and
                 * the byte offset to the first sample.
                 */
                assert( (nBitsPerPixel % 8) == 0 );
                if( nPlanarConfig == PLANARCONFIG_SEPARATE )
                {
                    nSkewBits = 0;
                    nSampleByteOffset = 0;
                }
                else
                {
                    nSkewBits = nBitsPerPixel * (nSamples-1);
                    nSampleByteOffset = (nBitsPerPixel/8) * iSample;
                }

                /*
                 * Perform the downsampling.
                 */
#ifdef DBMALLOC
                malloc_chain_check( 1 );
#endif
                TIFF_DownSample( pabySrcTile + nSampleByteOffset,
                               nBlockXSize, nBlockYSize,
                               nSkewBits, nBitsPerPixel, pabyOTile,
                               poRBI->nBlockXSize, poRBI->nBlockYSize,
                               nTXOff, nTYOff,
                               nOMult, nSampleFormat, pszResampling );
#ifdef DBMALLOC
                malloc_chain_check( 1 );
#endif
            }
        }
    }
}

/************************************************************************/
/*                        TIFF_BuildOverviews()                         */
/*                                                                      */
/*      Build the requested list of overviews.  Overviews are           */
/*      maintained in a bunch of temporary files and then these are     */
/*      written back to the TIFF file.  Only one pass through the       */
/*      source TIFF file is made for any number of output               */
/*      overviews.                                                      */
/************************************************************************/

void TIFFBuildOverviews( TIFF *hTIFF, int nOverviews, int * panOvList,
                         int bUseSubIFDs, const char *pszResampleMethod,
                         int (*pfnProgress)( double, void * ),
                         void * pProgressData )

{
    TIFFOvrCache	**papoRawBIs;
    uint32		nXSize, nYSize, nBlockXSize, nBlockYSize;
    uint16		nBitsPerPixel, nPhotometric, nCompressFlag, nSamples,
        nPlanarConfig, nSampleFormat;
    int         bSubsampled;
    uint16      nHorSubsampling, nVerSubsampling;
    int			bTiled, nSXOff, nSYOff, i;
    unsigned char	*pabySrcTile;
    uint16		*panRedMap, *panGreenMap, *panBlueMap;
    TIFFErrorHandler    pfnWarning;

    (void) pfnProgress;
    (void) pProgressData;

/* -------------------------------------------------------------------- */
/*      Get the base raster size.                                       */
/* -------------------------------------------------------------------- */
    TIFFGetField( hTIFF, TIFFTAG_IMAGEWIDTH, &nXSize );
    TIFFGetField( hTIFF, TIFFTAG_IMAGELENGTH, &nYSize );

    TIFFGetField( hTIFF, TIFFTAG_BITSPERSAMPLE, &nBitsPerPixel );
    /* TODO: nBitsPerPixel seems misnomer and may need renaming to nBitsPerSample */
    TIFFGetField( hTIFF, TIFFTAG_SAMPLESPERPIXEL, &nSamples );
    TIFFGetFieldDefaulted( hTIFF, TIFFTAG_PLANARCONFIG, &nPlanarConfig );

    TIFFGetFieldDefaulted( hTIFF, TIFFTAG_PHOTOMETRIC, &nPhotometric );
    TIFFGetFieldDefaulted( hTIFF, TIFFTAG_COMPRESSION, &nCompressFlag );
    TIFFGetFieldDefaulted( hTIFF, TIFFTAG_SAMPLEFORMAT, &nSampleFormat );

    if( nPhotometric == PHOTOMETRIC_YCBCR || nPhotometric == PHOTOMETRIC_ITULAB )
    {
        if( nBitsPerPixel != 8 || nSamples != 3 || nPlanarConfig != PLANARCONFIG_CONTIG ||
            nSampleFormat != SAMPLEFORMAT_UINT)
        {
            /* TODO: use of TIFFError is inconsistent with use of fprintf in addtiffo.c, sort out */
            TIFFErrorExt( TIFFClientdata(hTIFF), "TIFFBuildOverviews",
                          "File `%s' has an unsupported subsampling configuration.\n",
                          TIFFFileName(hTIFF) );
            /* If you need support for this particular flavor, please contact either
             * Frank Warmerdam warmerdam@pobox.com
             * Joris Van Damme info@awaresystems.be
             */
            return;
        }
        bSubsampled = 1;
        TIFFGetField( hTIFF, TIFFTAG_YCBCRSUBSAMPLING, &nHorSubsampling, &nVerSubsampling );
        /* TODO: find out if maybe TIFFGetFieldDefaulted is better choice for YCbCrSubsampling tag */
    }
    else
    {
        if( nBitsPerPixel < 8 )
        {
            /* TODO: use of TIFFError is inconsistent with use of fprintf in addtiffo.c, sort out */
            TIFFErrorExt( TIFFClientdata(hTIFF), "TIFFBuildOverviews",
                          "File `%s' has samples of %d bits per sample.  Sample\n"
                          "sizes of less than 8 bits per sample are not supported.\n",
                          TIFFFileName(hTIFF), nBitsPerPixel );
            return;
        }
        bSubsampled = 0;
        nHorSubsampling = 1;
        nVerSubsampling = 1;
    }

/* -------------------------------------------------------------------- */
/*      Turn off warnings to avoid alot of repeated warnings while      */
/*      rereading directories.                                          */
/* -------------------------------------------------------------------- */
    pfnWarning = TIFFSetWarningHandler( NULL );

/* -------------------------------------------------------------------- */
/*      Get the base raster block size.                                 */
/* -------------------------------------------------------------------- */
    if( TIFFGetField( hTIFF, TIFFTAG_ROWSPERSTRIP, &(nBlockYSize) ) )
    {
        nBlockXSize = nXSize;
        bTiled = FALSE;
    }
    else
    {
        TIFFGetField( hTIFF, TIFFTAG_TILEWIDTH, &nBlockXSize );
        TIFFGetField( hTIFF, TIFFTAG_TILELENGTH, &nBlockYSize );
        bTiled = TRUE;
    }

/* -------------------------------------------------------------------- */
/*	Capture the pallette if there is one.				*/
/* -------------------------------------------------------------------- */
    if( TIFFGetField( hTIFF, TIFFTAG_COLORMAP,
                      &panRedMap, &panGreenMap, &panBlueMap ) )
    {
        uint16		*panRed2, *panGreen2, *panBlue2;
        int             nColorCount = 1 << nBitsPerPixel;

        panRed2 = (uint16 *) _TIFFmalloc(2*nColorCount);
        panGreen2 = (uint16 *) _TIFFmalloc(2*nColorCount);
        panBlue2 = (uint16 *) _TIFFmalloc(2*nColorCount);

        memcpy( panRed2, panRedMap, 2 * nColorCount );
        memcpy( panGreen2, panGreenMap, 2 * nColorCount );
        memcpy( panBlue2, panBlueMap, 2 * nColorCount );

        panRedMap = panRed2;
        panGreenMap = panGreen2;
        panBlueMap = panBlue2;
    }
    else
    {
        panRedMap = panGreenMap = panBlueMap = NULL;
    }

/* -------------------------------------------------------------------- */
/*      Initialize overviews.                                           */
/* -------------------------------------------------------------------- */
    papoRawBIs = (TIFFOvrCache **) _TIFFmalloc(nOverviews*sizeof(void*));

    for( i = 0; i < nOverviews; i++ )
    {
        uint32  nOXSize, nOYSize, nOBlockXSize, nOBlockYSize;
        toff_t  nDirOffset;

        nOXSize = (nXSize + panOvList[i] - 1) / panOvList[i];
        nOYSize = (nYSize + panOvList[i] - 1) / panOvList[i];

        nOBlockXSize = MIN(nBlockXSize,nOXSize);
        nOBlockYSize = MIN(nBlockYSize,nOYSize);

        if( bTiled )
        {
            if( (nOBlockXSize % 16) != 0 )
                nOBlockXSize = nOBlockXSize + 16 - (nOBlockXSize % 16);

            if( (nOBlockYSize % 16) != 0 )
                nOBlockYSize = nOBlockYSize + 16 - (nOBlockYSize % 16);
        }

        nDirOffset = TIFF_WriteOverview( hTIFF, nOXSize, nOYSize,
                                         nBitsPerPixel, nPlanarConfig,
                                         nSamples, nOBlockXSize, nOBlockYSize,
                                         bTiled, nCompressFlag, nPhotometric,
                                         nSampleFormat,
                                         panRedMap, panGreenMap, panBlueMap,
                                         bUseSubIFDs,
                                         nHorSubsampling, nVerSubsampling );
        
        papoRawBIs[i] = TIFFCreateOvrCache( hTIFF, nDirOffset );
    }

    if( panRedMap != NULL )
    {
        _TIFFfree( panRedMap );
        _TIFFfree( panGreenMap );
        _TIFFfree( panBlueMap );
    }
    
/* -------------------------------------------------------------------- */
/*      Allocate a buffer to hold a source block.                       */
/* -------------------------------------------------------------------- */
    if( bTiled )
        pabySrcTile = (unsigned char *) _TIFFmalloc(TIFFTileSize(hTIFF));
    else
        pabySrcTile = (unsigned char *) _TIFFmalloc(TIFFStripSize(hTIFF));
    
/* -------------------------------------------------------------------- */
/*      Loop over the source raster, applying data to the               */
/*      destination raster.                                             */
/* -------------------------------------------------------------------- */
    for( nSYOff = 0; nSYOff < (int) nYSize; nSYOff += nBlockYSize )
    {
        for( nSXOff = 0; nSXOff < (int) nXSize; nSXOff += nBlockXSize )
        {
            /*
             * Read and resample into the various overview images.
             */
            
            TIFF_ProcessFullResBlock( hTIFF, nPlanarConfig,
                                      bSubsampled,nHorSubsampling,nVerSubsampling,
                                      nOverviews, panOvList,
                                      nBitsPerPixel, nSamples, papoRawBIs,
                                      nSXOff, nSYOff, pabySrcTile,
                                      nBlockXSize, nBlockYSize,
                                      nSampleFormat, pszResampleMethod );
        }
    }

    _TIFFfree( pabySrcTile );

/* -------------------------------------------------------------------- */
/*      Cleanup the rawblockedimage files.                              */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nOverviews; i++ )
    {
        TIFFDestroyOvrCache( papoRawBIs[i] );
    }

    if( papoRawBIs != NULL )
        _TIFFfree( papoRawBIs );

    TIFFSetWarningHandler( pfnWarning );
}


/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
