/*
 * maketif.c -- creates a little TIFF file, with
 *   the XTIFF extended tiff example tags.
 */

#include <stdlib.h>
#include "xtiffio.h"


void SetUpTIFFDirectory(TIFF *tif);
void WriteImage(TIFF *tif);

#define WIDTH 20
#define HEIGHT 20

void main()
{
	TIFF *tif=(TIFF*)0;  /* TIFF-level descriptor */
	
	tif=XTIFFOpen("newtif.tif","w");
	if (!tif) goto failure;
	
	SetUpTIFFDirectory(tif);
	WriteImage(tif);
	
	XTIFFClose(tif);
	exit (0);
	
failure:
	printf("failure in maketif\n");
	if (tif) XTIFFClose(tif);
	exit (-1);
}


void SetUpTIFFDirectory(TIFF *tif)
{
	double mymulti[6]={0.0,1.0,2.0,  3.1415926, 5.0,1.0};
	uint32 mysingle=3456;
	char *ascii="This file was produced by Steven Spielberg. NOT";

	TIFFSetField(tif,TIFFTAG_IMAGEWIDTH,WIDTH);
	TIFFSetField(tif,TIFFTAG_IMAGELENGTH,HEIGHT);
	TIFFSetField(tif,TIFFTAG_COMPRESSION,COMPRESSION_NONE);
	TIFFSetField(tif,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_MINISBLACK);
	TIFFSetField(tif,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
	TIFFSetField(tif,TIFFTAG_BITSPERSAMPLE,8);
	TIFFSetField(tif,TIFFTAG_ROWSPERSTRIP,20);

	/* Install the extended TIFF tag examples */
	TIFFSetField(tif,TIFFTAG_EXAMPLE_MULTI,6,mymulti);
	TIFFSetField(tif,TIFFTAG_EXAMPLE_SINGLE,mysingle);
	TIFFSetField(tif,TIFFTAG_EXAMPLE_ASCII,ascii);
}


void WriteImage(TIFF *tif)
{
	int i;
	char buffer[WIDTH];
	
	memset(buffer,0,sizeof(buffer));
	for (i=0;i<HEIGHT;i++)
		if (!TIFFWriteScanline(tif, buffer, i, 0))
			TIFFErrorExt(tif->tif_clientdata, "WriteImage","failure in WriteScanline\n");
}




/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
