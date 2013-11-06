/*************************************************************************
 *
 * Source file for Windows 95/Win32. 
 *
 * The function LoadTIFFinDIB in this source file let you load 
 * a TIFF file and build a memory DIB with it and return the 
 * HANDLE (HDIB) of the memory bloc containing the DIB.
 *
 *  Example : 
 * 
 *   HDIB   hDIB;
 *   hDIB = LoadTIFFinDIB("sample.tif");
 *
 *
 * To build this source file you must include the TIFF library   
 * in your project.
 *
 * 4/12/95   Philippe Tenenhaus   100423.3705@compuserve.com
 *
 ************************************************************************/


#include "tiffio.h" 

#define HDIB HANDLE
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define CVT(x)      (((x) * 255L) / ((1L<<16)-1))

static HDIB CreateDIB(DWORD dwWidth, DWORD dwHeight, WORD wBitCount);
static LPSTR FindDIBBits(LPSTR lpDIB);
static WORD PaletteSize(LPSTR lpDIB);
static WORD DIBNumColors(LPSTR lpDIB);
static int checkcmap(int n, uint16* r, uint16* g, uint16* b);



/*************************************************************************
 *
 * HDIB LoadTIFFinDIB(LPSTR lpFileName) 
 *
 * Parameter:
 *
 * LPSTR lpDIB      - File name of a tiff imag
 *
 * Return Value:
 *
 * LPSTR            - HANDLE of a DIB
 *
 * Description:
 *
 * This function load a TIFF file and build a memory DIB with it
 * and return the HANDLE (HDIB) of the memory bloc containing
 * the DIB.
 *
 * 4/12/95   Philippe Tenenhaus   100423.3705@compuserve.com
 *
 ************************************************************************/

HDIB LoadTIFFinDIB(LPSTR lpFileName)    
{
    TIFF          *tif;
    unsigned long imageLength; 
    unsigned long imageWidth; 
    unsigned int  BitsPerSample;
    unsigned long LineSize;
    unsigned int  SamplePerPixel;
    unsigned long RowsPerStrip;  
    int           PhotometricInterpretation;
    long          nrow;
	unsigned long row;
    char          *buf;          
    LPBITMAPINFOHEADER lpDIB; 
    HDIB          hDIB;
    char          *lpBits;
    HGLOBAL       hStrip;
    int           i,l;
    int           Align; 
    
    tif = TIFFOpen(lpFileName, "r");
    
    if (!tif)
        goto TiffOpenError;
    
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);  
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);
    TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);  
    TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);   
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &PhotometricInterpretation);
           
    LineSize = TIFFScanlineSize(tif); //Number of byte in ine line

    SamplePerPixel = (int) (LineSize/imageWidth);

    //Align = Number of byte to add at the end of each line of the DIB
    Align = 4 - (LineSize % 4);
    if (Align == 4)	Align = 0;

    
    //Create a new DIB
    hDIB = CreateDIB((DWORD) imageWidth, (DWORD) imageLength, (WORD)
(BitsPerSample*SamplePerPixel));
    lpDIB  = (LPBITMAPINFOHEADER) GlobalLock(hDIB);
    if (!lpDIB)
          goto OutOfDIBMemory;
          
    if (lpDIB)
       lpBits = FindDIBBits((LPSTR) lpDIB);

    //In the tiff file the lines are save from up to down 
	//In a DIB the lines must be save from down to up
    if (lpBits)
      {
        lpBits = FindDIBBits((LPSTR) lpDIB);
        lpBits+=((imageWidth*SamplePerPixel)+Align)*(imageLength-1);
		//now lpBits pointe on the bottom line
        
        hStrip = GlobalAlloc(GHND,TIFFStripSize(tif));
        buf = GlobalLock(hStrip);           
        
        if (!buf)
           goto OutOfBufMemory;
        
        //PhotometricInterpretation = 2 image is RGB
        //PhotometricInterpretation = 3 image have a color palette              
        if (PhotometricInterpretation == 3)
        {
          uint16* red;
          uint16* green;
          uint16* blue;
          int16 i;
          LPBITMAPINFO lpbmi;   
          int   Palette16Bits;          
           
          TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue); 

		  //Is the palette 16 or 8 bits ?
          if (checkcmap(1<<BitsPerSample, red, green, blue) == 16) 
             Palette16Bits = TRUE;
          else
             Palette16Bits = FALSE;
             
          lpbmi = (LPBITMAPINFO)lpDIB;                      
                
          //load the palette in the DIB
          for (i = (1<<BitsPerSample)-1; i >= 0; i--) 
            {             
             if (Palette16Bits)
                {
                  lpbmi->bmiColors[i].rgbRed =(BYTE) CVT(red[i]);
                  lpbmi->bmiColors[i].rgbGreen = (BYTE) CVT(green[i]);
                  lpbmi->bmiColors[i].rgbBlue = (BYTE) CVT(blue[i]);           
                }
             else
                {
                  lpbmi->bmiColors[i].rgbRed = (BYTE) red[i];
                  lpbmi->bmiColors[i].rgbGreen = (BYTE) green[i];
                  lpbmi->bmiColors[i].rgbBlue = (BYTE) blue[i];        
                }
            }  
                 
        }
        
        //read the tiff lines and save them in the DIB
		//with RGB mode, we have to change the order of the 3 samples RGB
<=> BGR
        for (row = 0; row < imageLength; row += RowsPerStrip) 
          {     
            nrow = (row + RowsPerStrip > imageLength ? imageLength - row :
RowsPerStrip);
            if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 0),
                buf, nrow*LineSize)==-1)
                  {
                     goto TiffReadError;
                  } 
            else
                  {  
                    for (l = 0; l < nrow; l++) 
                      {
                         if (SamplePerPixel  == 3)
                           for (i=0;i< (int) (imageWidth);i++)
                              {
                               lpBits[i*SamplePerPixel+0]=buf[l*LineSize+i*Sample
PerPixel+2]; 
                               lpBits[i*SamplePerPixel+1]=buf[l*LineSize+i*Sample
PerPixel+1];
                               lpBits[i*SamplePerPixel+2]=buf[l*LineSize+i*Sample
PerPixel+0];
                              }
                         else
                           memcpy(lpBits, &buf[(int) (l*LineSize)], (int)
imageWidth*SamplePerPixel); 
                          
                         lpBits-=imageWidth*SamplePerPixel+Align;

                      }
                 }
          }
        GlobalUnlock(hStrip);
        GlobalFree(hStrip);
        GlobalUnlock(hDIB); 
        TIFFClose(tif);
      }
      
    return hDIB;
    
    OutOfBufMemory:
       
    TiffReadError:
       GlobalUnlock(hDIB); 
       GlobalFree(hStrip);
    OutOfDIBMemory:
       TIFFClose(tif);
    TiffOpenError:
       return (HANDLE) 0;
       
         
}


static int checkcmap(int n, uint16* r, uint16* g, uint16* b)
{
    while (n-- > 0)
        if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256)
        return (16);
    
    return (8);
}



/*************************************************************************
 * All the following functions were created by microsoft, they are
 * parts of the sample project "wincap" given with the SDK Win32.
 *
 * Microsoft says that :
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ************************************************************************/

HDIB CreateDIB(DWORD dwWidth, DWORD dwHeight, WORD wBitCount)
{
   BITMAPINFOHEADER bi;         // bitmap header
   LPBITMAPINFOHEADER lpbi;     // pointer to BITMAPINFOHEADER
   DWORD dwLen;                 // size of memory block
   HDIB hDIB;
   DWORD dwBytesPerLine;        // Number of bytes per scanline


   // Make sure bits per pixel is valid
   if (wBitCount <= 1)
      wBitCount = 1;
   else if (wBitCount <= 4)
      wBitCount = 4;
   else if (wBitCount <= 8)
      wBitCount = 8;
   else if (wBitCount <= 24)
      wBitCount = 24;
   else
      wBitCount = 4;  // set default value to 4 if parameter is bogus

   // initialize BITMAPINFOHEADER
   bi.biSize = sizeof(BITMAPINFOHEADER);
   bi.biWidth = dwWidth;         // fill in width from parameter
   bi.biHeight = dwHeight;       // fill in height from parameter
   bi.biPlanes = 1;              // must be 1
   bi.biBitCount = wBitCount;    // from parameter
   bi.biCompression = BI_RGB;    
   bi.biSizeImage = (dwWidth*dwHeight*wBitCount)/8; //0;           // 0's here
mean "default"
   bi.biXPelsPerMeter = 2834; //0;
   bi.biYPelsPerMeter = 2834; //0;
   bi.biClrUsed = 0;
   bi.biClrImportant = 0;

   // calculate size of memory block required to store the DIB.  This
   // block should be big enough to hold the BITMAPINFOHEADER, the color
   // table, and the bits

   dwBytesPerLine =   (((wBitCount * dwWidth) + 31) / 32 * 4);
   dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + (dwBytesPerLine * dwHeight);

   // alloc memory block to store our bitmap
   hDIB = GlobalAlloc(GHND, dwLen);

   // major bummer if we couldn't get memory block
   if (!hDIB)
   {
      return NULL;
   }

   // lock memory and get pointer to it
   lpbi = (VOID FAR *)GlobalLock(hDIB);

   // use our bitmap info structure to fill in first part of
   // our DIB with the BITMAPINFOHEADER
   *lpbi = bi;

   // Since we don't know what the colortable and bits should contain,
   // just leave these blank.  Unlock the DIB and return the HDIB.

   GlobalUnlock(hDIB);

   /* return handle to the DIB */
   return hDIB;
}


LPSTR FAR FindDIBBits(LPSTR lpDIB)
{
   return (lpDIB + *(LPDWORD)lpDIB + PaletteSize(lpDIB));
}


WORD FAR PaletteSize(LPSTR lpDIB)
{
   /* calculate the size required by the palette */
   if (IS_WIN30_DIB (lpDIB))
      return (DIBNumColors(lpDIB) * sizeof(RGBQUAD));
   else
      return (DIBNumColors(lpDIB) * sizeof(RGBTRIPLE));
}


WORD DIBNumColors(LPSTR lpDIB)
{
   WORD wBitCount;  // DIB bit count

   /*  If this is a Windows-style DIB, the number of colors in the
    *  color table can be less than the number of bits per pixel
    *  allows for (i.e. lpbi->biClrUsed can be set to some value).
    *  If this is the case, return the appropriate value.
    */

   if (IS_WIN30_DIB(lpDIB))
   {
      DWORD dwClrUsed;

      dwClrUsed = ((LPBITMAPINFOHEADER)lpDIB)->biClrUsed;
      if (dwClrUsed)
     return (WORD)dwClrUsed;
   }

   /*  Calculate the number of colors in the color table based on
    *  the number of bits per pixel for the DIB.
    */
   if (IS_WIN30_DIB(lpDIB))
      wBitCount = ((LPBITMAPINFOHEADER)lpDIB)->biBitCount;
   else
      wBitCount = ((LPBITMAPCOREHEADER)lpDIB)->bcBitCount;

   /* return number of colors based on bits per pixel */
   switch (wBitCount)
      {
   case 1:
      return 2;

   case 4:
      return 16;

   case 8:
      return 256;

   default:
      return 0;
      }
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
