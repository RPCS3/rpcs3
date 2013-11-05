$!
$! OpenVMS configure procedure for libtiff
$! (c) Alexey Chupahin  22-NOV-2007
$! elvis_75@mail.ru
$!
$! Permission to use, copy, modify, distribute, and sell this software and 
$! its documentation for any purpose is hereby granted without fee, provided
$! that (i) the above copyright notices and this permission notice appear in
$! all copies of the software and related documentation, and (ii) the names of
$! Sam Leffler and Silicon Graphics may not be used in any advertising or
$! publicity relating to the software without the specific, prior written
$! permission of Sam Leffler and Silicon Graphics.
$! 
$! THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
$! EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
$! WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
$! 
$! IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
$! ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
$! OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
$! WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
$! LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
$! OF THIS SOFTWARE.
$!
$!
$ SET NOON
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT "Configuring libTIFF library"
$WRITE SYS$OUTPUT " "
$! Checking architecture
$DECC = F$SEARCH("SYS$SYSTEM:DECC$COMPILER.EXE") .NES. ""
$IF (.NOT. DECC) THEN $WRITE SYS$OUTPUT  "BAD compiler" GOTO EXIT
$    IF F$GETSYI("ARCH_TYPE").EQ.1 THEN CPU = "VAX"
$    IF F$GETSYI("ARCH_TYPE").EQ.2 THEN CPU = "Alpha"
$    IF F$GETSYI("ARCH_TYPE").EQ.3 THEN CPU = "I64"
$    OS = F$GETSYI("VERSION")
$WRITE SYS$OUTPUT "Checking architecture	...  ", CPU
$WRITE SYS$OUTPUT "Checking OS          	...  OpenVMS ",OS
$SHARED=0
$IF ( (CPU.EQS."Alpha").OR.(CPU.EQS."I64") )
$  THEN
$       SHARED=64
$  ELSE
$       SHARED=32
$ENDIF
$MMS = F$SEARCH("SYS$SYSTEM:MMS.EXE") .NES. ""
$MMK = F$TYPE(MMK) 
$IF (MMS .OR. MMK.NES."") THEN GOTO TEST_LIBRARIES
$! I cant find any make tool
$GOTO EXIT
$!
$!
$TEST_LIBRARIES:
$!   Setting as MAKE utility one of MMS or MMK. I prefer MMS.
$IF (MMK.NES."") THEN MAKE="MMK"
$IF (MMS) THEN MAKE="MMS"
$WRITE SYS$OUTPUT "Checking build utility	...  ''MAKE'"
$WRITE SYS$OUTPUT " "
$!
$!
$IF (P1.EQS."STATIC").OR.(P1.EQS."static") THEN SHARED=0
$!
$!
$!"Checking for strcasecmp "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
	#include  <strings.h>
	#include  <stdlib.h>

    int main()
	{
        if (strcasecmp("bla", "Bla")==0) exit(0);
	   else exit(2);
	}
$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$       HAVE_STRCASECMP=0
$       GOTO NEXT1
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_STRCASECMP=0
$       GOTO NEXT1
$ENDIF
$!
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$RUN TEST
$IF ($STATUS .NE. %X00000001)
$  THEN
$	HAVE_STRCASECMP=0
$  ELSE
$	 HAVE_STRCASECMP=1
$ENDIF
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$NEXT1:
$IF (HAVE_STRCASECMP.EQ.1)
$  THEN
$ 	WRITE SYS$OUTPUT "Checking for strcasecmp ...   Yes"	
$  ELSE
$	WRITE SYS$OUTPUT "Checking for strcasecmp ...   No"
$ENDIF
$!
$!

$!"Checking for lfind "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
        #include  <search.h>

    int main()
        {
        lfind((const void *)key, (const void *)NULL, (size_t *)NULL,
           (size_t) 0, NULL);
        }
$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$       HAVE_LFIND=0
$       GOTO NEXT2
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_LFIND=0
$       GOTO NEXT2
$  ELSE
$        HAVE_LFIND=1
$ENDIF
$!
$NEXT2:
$IF (HAVE_LFIND.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for lfind ...   Yes"
$  ELSE
$       WRITE SYS$OUTPUT "Checking for lfind ...   No"
$ENDIF
$!
$!
$!"Checking for correct zlib library    "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
      #include <stdlib.h>
      #include <stdio.h>
      #include <zlib.h>
   int main()
     {
	printf("checking version zlib:  %s\n",zlibVersion());
     }
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10B90001) 
$  THEN 
$	HAVE_ZLIB=0
$	GOTO EXIT
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST,ZLIB:LIBZ/LIB 
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001) 
$  THEN 
$	HAVE_ZLIB=0
$       GOTO EXIT
$  ELSE
$	HAVE_ZLIB=1
$ENDIF
$IF (HAVE_ZLIB.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for correct zlib library ...   Yes"
$  ELSE
$	WRITE SYS$OUTPUT "Checking for correct zlib library ...   No"
$       WRITE SYS$OUTPUT "This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html"
$ENDIF
$RUN TEST
$!

$DEL TEST.OBJ;*
$! Checking for JPEG ...
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(JPEG) SYS$INPUT
      #include <stdlib.h>
      #include <stdio.h>
      #include <jpeglib.h>
      #include <jversion.h>	
   int main()
     {
	printf("checking version jpeg:  %s\n",JVERSION);
	jpeg_quality_scaling(0);
        return 0;
     }
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10B90001)
$  THEN
$       WRITE SYS$OUTPUT "Checking for static jpeg library ...   No"
$	HAVE_JPEG=0
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST,JPEG:LIBJPEG/LIB
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$	HAVE_JPEG=0
$  ELSE
$	HAVE_JPEG=1
$ENDIF
$IF (HAVE_JPEG.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for static jpeg library ...   Yes"
$       JPEG_LIBRARY_PATH="JPEG:LIBJPEG/LIB"
$       RUN TEST
$  ELSE
$       WRITE SYS$OUTPUT "Checking for static jpeg library ...   No"
$ENDIF
$!
$!"Checking for SHARED JPEG library    "
$OPEN/WRITE OUT TEST.OPT
$WRITE OUT "SYS$SHARE:LIBJPEG$SHR/SHARE"
$WRITE OUT "ZLIB:LIBZ/LIB"
$CLOSE OUT
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST,TEST/OPT
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_JPEG_SHARED=0
$  ELSE
$       HAVE_JPEG_SHARED=1
$ENDIF
$IF (HAVE_JPEG_SHARED.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for shared jpeg library ...   Yes"
$       JPEG_LIBRARY_PATH="SYS$SHARE:LIBJPEG$SHR/SHARE"
$  ELSE
$       WRITE SYS$OUTPUT "Checking for shared jpeg library ...   No"
$ENDIF
$!
$ IF ( (HAVE_JPEG_SHARED.EQ.0).AND.(HAVE_JPEG.EQ.0) )
$    THEN
$       WRITE SYS$OUTPUT "No JPEG library installed. This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html"
$       GOTO EXIT
$ ENDIF
$!
$!
$!
$! Checking for X11 ...
$IF F$TRNLNM("DECW$INCLUDE") .NES. ""
$  THEN
$	WRITE SYS$OUTPUT "Checking for X11 ...   Yes"
$  ELSE
$	WRITE SYS$OUTPUT "Checking for X11 ...   No"
$	WRITE SYS$OUTPUT "This is fatal. Please install X11 software"
$	GOTO EXIT
$ENDIF
$!
$!WRITING BUILD FILES
$OPEN/WRITE OUT BUILD.COM
$ WRITE OUT "$set def [.port]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-.libtiff]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-.tools]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-]"
$ WRITE OUT "$cop [.PORT]LIBPORT.OLB [.LIBTIFF]LIBPORT.OLB"
$ WRITE OUT "$ CURRENT = F$ENVIRONMENT (""DEFAULT"") "
$ WRITE OUT "$TIFF=CURRENT"
$ WRITE OUT "$OPEN/WRITE OUTT LIBTIFF$STARTUP.COM"
$ WRITE OUT "$TIFF[F$LOCATE(""]"",TIFF),9]:="".LIBTIFF]"""
$ WRITE OUT "$WRITE OUTT ""DEFINE TIFF ","'","'","TIFF'"" "
$ WRITE OUT "$TIFF=CURRENT"
$ WRITE OUT "$TIFF[F$LOCATE(""]"",TIFF),7]:="".TOOLS]"""
$ WRITE OUT "$WRITE OUTT ""BMP2TIFF:==$", "'","'","TIFF'BMP2TIFF"""
$ WRITE OUT "$WRITE OUTT ""FAX2PS:==$", "'","'","TIFF'FAX2PS"""
$ WRITE OUT "$WRITE OUTT ""FAX2TIFF:==$", "'","'","TIFF'FAX2TIFF"""
$ WRITE OUT "$WRITE OUTT ""GIF2TIFF:==$", "'","'","TIFF'GIF2TIFF"""
$ WRITE OUT "$WRITE OUTT ""PAL2RGB:==$", "'","'","TIFF'PAL2RGB"""
$ WRITE OUT "$WRITE OUTT ""PPM2TIFF:==$", "'","'","TIFF'PPM2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RAS2TIFF:==$", "'","'","TIFF'RAS2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RAW2TIFF:==$", "'","'","TIFF'RAW2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RGB2YCBCR:==$", "'","'","TIFF'RGB2YCBCR"""
$ WRITE OUT "$WRITE OUTT ""THUMBNAIL:==$", "'","'","TIFF'THUMBNAIL"""
$ WRITE OUT "$WRITE OUTT ""TIFF2BW:==$", "'","'","TIFF'TIFF2BW"""
$ WRITE OUT "$WRITE OUTT ""TIFF2PDF:==$", "'","'","TIFF'TIFF2PDF"""
$ WRITE OUT "$WRITE OUTT ""TIFF2PS:==$", "'","'","TIFF'TIFF2PS"""
$ WRITE OUT "$WRITE OUTT ""TIFF2RGBA:==$", "'","'","TIFF'TIFF2RGBA"""
$ WRITE OUT "$WRITE OUTT ""TIFFCMP:==$", "'","'","TIFF'TIFFCMP"""
$ WRITE OUT "$WRITE OUTT ""TIFFCP:==$", "'","'","TIFF'TIFFCP"""
$ WRITE OUT "$WRITE OUTT ""TIFFDITHER:==$", "'","'","TIFF'TIFFDITHER"""
$ WRITE OUT "$WRITE OUTT ""TIFFDUMP:==$", "'","'","TIFF'TIFFDUMP"""
$ WRITE OUT "$WRITE OUTT ""TIFFINFO:==$", "'","'","TIFF'TIFFINFO"""
$ WRITE OUT "$WRITE OUTT ""TIFFMEDIAN:==$", "'","'","TIFF'TIFFMEDIAN"""
$ WRITE OUT "$WRITE OUTT ""TIFFCROP:==$", "'","'","TIFF'TIFFCROP"""
$ WRITE OUT "$WRITE OUTT ""TIFFSET:==$", "'","'","TIFF'TIFFSET"""
$ WRITE OUT "$CLOSE OUTT"
$ WRITE OUT "$OPEN/WRITE OUTT [.LIBTIFF]LIBTIFF.OPT"
$ WRITE OUT "$WRITE OUTT ""TIFF:TIFF/LIB""
$ WRITE OUT "$WRITE OUTT ""TIFF:LIBPORT/LIB""
$ WRITE OUT "$WRITE OUTT ""JPEG:LIBJPEG/LIB""
$ WRITE OUT "$WRITE OUTT ""ZLIB:LIBZ/LIB""
$ WRITE OUT "$CLOSE OUTT"
$!
$ WRITE OUT "$WRITE SYS$OUTPUT "" "" "
$ WRITE OUT "$WRITE SYS$OUTPUT ""***************************************************************************** "" "
$ WRITE OUT "$WRITE SYS$OUTPUT ""LIBTIFF$STARTUP.COM has been created. "" "
$ WRITE OUT "$WRITE SYS$OUTPUT ""This file setups all logicals needed. It should be execute before using LibTIFF "" "
$ WRITE OUT "$WRITE SYS$OUTPUT ""Nice place to call it - LOGIN.COM "" "
$ WRITE OUT "$WRITE SYS$OUTPUT """" "
$ WRITE OUT "$WRITE SYS$OUTPUT ""Using the library:"" "
$ WRITE OUT "$WRITE SYS$OUTPUT ""CC/INC=TIFF ASCII_TAG.C"" "
$ WRITE OUT "$WRITE SYS$OUTPUT ""LINK ASCII_TAG,TIFF:LIBTIFF/OPT"" "
$ WRITE OUT "$WRITE SYS$OUTPUT ""***************************************************************************** "" "
$CLOSE OUT
$!
$! DESCRIP.MMS in [.PORT]
$OBJ="dummy.obj"
$IF HAVE_STRCASECMP.NE.1 
$  THEN 
$     OBJ=OBJ+",strcasecmp.obj"
$ENDIF
$IF HAVE_LFIND.NE.1   
$   THEN 
$       OBJ=OBJ+",lfind.obj"
$ENDIF
$OPEN/WRITE OUT [.PORT]DESCRIP.MMS
$WRITE OUT "OBJ=",OBJ
$WRITE OUT ""
$WRITE OUT "LIBPORT.OLB : $(OBJ)"
$WRITE OUT "	LIB/CREA LIBPORT $(OBJ)"
$WRITE OUT ""
$WRITE OUT ""
$WRITE OUT "dummy.obj : dummy.c"
$WRITE OUT "         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)"
$WRITE OUT ""
$WRITE OUT ""
$WRITE OUT "strcasecmp.obj : strcasecmp.c"
$WRITE OUT "         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)"
$WRITE OUT ""
$WRITE OUT ""
$WRITE OUT "lfind.obj : lfind.c"
$WRITE OUT "         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)"
$WRITE OUT ""
$WRITE OUT ""
$CLOSE OUT
$!
$!
$WRITE SYS$OUTPUT "Creating LIBTIFF$DEF.OPT"
$IF (SHARED.EQ.64)
$ THEN
$       COPY SYS$INPUT TIFF$DEF.OPT
SYMBOL_VECTOR= (-
TIFFOpen=PROCEDURE,-
TIFFGetVersion=PROCEDURE,-
TIFFCleanup=PROCEDURE,-
TIFFClose=PROCEDURE,-
TIFFFlush=PROCEDURE,-
TIFFFlushData=PROCEDURE,-
TIFFGetField=PROCEDURE,-
TIFFVGetField=PROCEDURE,-
TIFFGetFieldDefaulted=PROCEDURE,-
TIFFVGetFieldDefaulted=PROCEDURE,-
TIFFGetTagListEntry=PROCEDURE,-
TIFFGetTagListCount=PROCEDURE,-
TIFFReadDirectory=PROCEDURE,-
TIFFScanlineSize=PROCEDURE,-
TIFFStripSize=PROCEDURE,-
TIFFVStripSize=PROCEDURE,-
TIFFRawStripSize=PROCEDURE,-
TIFFTileRowSize=PROCEDURE,-
TIFFTileSize=PROCEDURE,-
TIFFVTileSize=PROCEDURE,-
TIFFFileno=PROCEDURE,-
TIFFSetFileno=PROCEDURE,-
TIFFGetMode=PROCEDURE,-
TIFFIsTiled=PROCEDURE,-
TIFFIsByteSwapped=PROCEDURE,-
TIFFIsBigEndian=PROCEDURE,-
TIFFIsMSB2LSB=PROCEDURE,-
TIFFIsUpSampled=PROCEDURE,-
TIFFCIELabToRGBInit=PROCEDURE,-
TIFFCIELabToXYZ=PROCEDURE,-
TIFFXYZToRGB=PROCEDURE,-
TIFFYCbCrToRGBInit=PROCEDURE,-
TIFFYCbCrtoRGB=PROCEDURE,-
TIFFCurrentRow=PROCEDURE,-
TIFFCurrentDirectory=PROCEDURE,-
TIFFCurrentStrip=PROCEDURE,-
TIFFCurrentTile=PROCEDURE,-
TIFFDataWidth=PROCEDURE,-
TIFFReadBufferSetup=PROCEDURE,-
TIFFWriteBufferSetup=PROCEDURE,-
TIFFSetupStrips=PROCEDURE,-
TIFFLastDirectory=PROCEDURE,-
TIFFSetDirectory=PROCEDURE,-
TIFFSetSubDirectory=PROCEDURE,-
TIFFUnlinkDirectory=PROCEDURE,-
TIFFSetField=PROCEDURE,-
TIFFVSetField=PROCEDURE,-
TIFFCheckpointDirectory=PROCEDURE,-
TIFFWriteDirectory=PROCEDURE,-
TIFFRewriteDirectory=PROCEDURE,-
TIFFPrintDirectory=PROCEDURE,-
TIFFReadScanline=PROCEDURE,-
TIFFWriteScanline=PROCEDURE,-
TIFFReadRGBAImage=PROCEDURE,-
TIFFReadRGBAImageOriented=PROCEDURE,-
TIFFFdOpen=PROCEDURE,-
TIFFClientOpen=PROCEDURE,-
TIFFFileName=PROCEDURE,-
TIFFError=PROCEDURE,-
TIFFErrorExt=PROCEDURE,-
TIFFWarning=PROCEDURE,-
TIFFWarningExt=PROCEDURE,-
TIFFSetErrorHandler=PROCEDURE,-
TIFFSetErrorHandlerExt=PROCEDURE,-
TIFFSetWarningHandler=PROCEDURE,-
TIFFSetWarningHandlerExt=PROCEDURE,-
TIFFComputeTile=PROCEDURE,-
TIFFCheckTile=PROCEDURE,-
TIFFNumberOfTiles=PROCEDURE,-
TIFFReadTile=PROCEDURE,-
TIFFWriteTile=PROCEDURE,-
TIFFComputeStrip=PROCEDURE,-
TIFFNumberOfStrips=PROCEDURE,-
TIFFRGBAImageBegin=PROCEDURE,-
TIFFRGBAImageGet=PROCEDURE,-
TIFFRGBAImageEnd=PROCEDURE,-
TIFFReadEncodedStrip=PROCEDURE,-
TIFFReadRawStrip=PROCEDURE,-
TIFFReadEncodedTile=PROCEDURE,-
TIFFReadRawTile=PROCEDURE,-
TIFFReadRGBATile=PROCEDURE,-
TIFFReadRGBAStrip=PROCEDURE,-
TIFFWriteEncodedStrip=PROCEDURE,-
TIFFWriteRawStrip=PROCEDURE,-
TIFFWriteEncodedTile=PROCEDURE,-
TIFFWriteRawTile=PROCEDURE,-
TIFFSetWriteOffset=PROCEDURE,-
TIFFSwabDouble=PROCEDURE,-
TIFFSwabShort=PROCEDURE,-
TIFFSwabLong=PROCEDURE,-
TIFFSwabArrayOfShort=PROCEDURE,-
TIFFSwabArrayOfLong=PROCEDURE,-
TIFFSwabArrayOfDouble=PROCEDURE,-
TIFFSwabArrayOfTriples=PROCEDURE,-
TIFFReverseBits=PROCEDURE,-
TIFFGetBitRevTable=PROCEDURE,-
TIFFDefaultStripSize=PROCEDURE,-
TIFFDefaultTileSize=PROCEDURE,-
TIFFRasterScanlineSize=PROCEDURE,-
_TIFFmalloc=PROCEDURE,-
_TIFFrealloc=PROCEDURE,-
_TIFFfree=PROCEDURE,-
_TIFFmemset=PROCEDURE,-
_TIFFmemcpy=PROCEDURE,-
_TIFFmemcmp=PROCEDURE,-
TIFFCreateDirectory=PROCEDURE,-
TIFFSetTagExtender=PROCEDURE,-
TIFFMergeFieldInfo=PROCEDURE,-
TIFFFindFieldInfo=PROCEDURE,-
TIFFFindFieldInfoByName=PROCEDURE,-
TIFFFieldWithName=PROCEDURE,-
TIFFFieldWithTag=PROCEDURE,-
TIFFFieldTag=PROCEDURE,-
TIFFFieldName=PROCEDURE,-
TIFFFieldDataType=PROCEDURE,-
TIFFFieldPassCount=PROCEDURE,-
TIFFFieldReadCount=PROCEDURE,-
TIFFFieldWriteCount=PROCEDURE,-
TIFFCurrentDirOffset=PROCEDURE,-
TIFFWriteCheck=PROCEDURE,-
TIFFRGBAImageOK=PROCEDURE,-
TIFFNumberOfDirectories=PROCEDURE,-
TIFFSetFileName=PROCEDURE,-
TIFFSetClientdata=PROCEDURE,-
TIFFSetMode=PROCEDURE,-
TIFFClientdata=PROCEDURE,-
TIFFGetReadProc=PROCEDURE,-
TIFFGetWriteProc=PROCEDURE,-
TIFFGetSeekProc=PROCEDURE,-
TIFFGetCloseProc=PROCEDURE,-
TIFFGetSizeProc=PROCEDURE,-
TIFFGetMapFileProc=PROCEDURE,-
TIFFGetUnmapFileProc=PROCEDURE,-
TIFFIsCODECConfigured=PROCEDURE,-
TIFFGetConfiguredCODECs=PROCEDURE,-
TIFFFindCODEC=PROCEDURE,-
TIFFRegisterCODEC=PROCEDURE,-
TIFFUnRegisterCODEC=PROCEDURE,-
TIFFFreeDirectory=PROCEDURE,-
TIFFReadCustomDirectory=PROCEDURE,-
TIFFReadEXIFDirectory=PROCEDURE,-
TIFFAccessTagMethods=PROCEDURE,-
TIFFGetClientInfo=PROCEDURE,-
TIFFSetClientInfo=PROCEDURE,-
TIFFReassignTagToIgnore=PROCEDURE-
)

$ENDIF
$IF (SHARED.EQ.32)
$ THEN
$       COPY SYS$INPUT TIFF$DEF.OPT
UNIVERSAL=TIFFOpen
UNIVERSAL=TIFFGetVersion
UNIVERSAL=TIFFCleanup
UNIVERSAL=TIFFClose
UNIVERSAL=TIFFFlush
UNIVERSAL=TIFFFlushData
UNIVERSAL=TIFFGetField
UNIVERSAL=TIFFVGetField
UNIVERSAL=TIFFGetFieldDefaulted
UNIVERSAL=TIFFVGetFieldDefaulted
UNIVERSAL=TIFFGetTagListEntry
UNIVERSAL=TIFFGetTagListCount
UNIVERSAL=TIFFReadDirectory
UNIVERSAL=TIFFScanlineSize
UNIVERSAL=TIFFStripSize
UNIVERSAL=TIFFVStripSize
UNIVERSAL=TIFFRawStripSize
UNIVERSAL=TIFFTileRowSize
UNIVERSAL=TIFFTileSize
UNIVERSAL=TIFFVTileSize
UNIVERSAL=TIFFFileno
UNIVERSAL=TIFFSetFileno
UNIVERSAL=TIFFGetMode
UNIVERSAL=TIFFIsTiled
UNIVERSAL=TIFFIsByteSwapped
UNIVERSAL=TIFFIsBigEndian
UNIVERSAL=TIFFIsMSB2LSB
UNIVERSAL=TIFFIsUpSampled
UNIVERSAL=TIFFCIELabToRGBInit
UNIVERSAL=TIFFCIELabToXYZ
UNIVERSAL=TIFFXYZToRGB
UNIVERSAL=TIFFYCbCrToRGBInit
UNIVERSAL=TIFFYCbCrtoRGB
UNIVERSAL=TIFFCurrentRow
UNIVERSAL=TIFFCurrentDirectory
UNIVERSAL=TIFFCurrentStrip
UNIVERSAL=TIFFCurrentTile
UNIVERSAL=TIFFDataWidth
UNIVERSAL=TIFFReadBufferSetup
UNIVERSAL=TIFFWriteBufferSetup
UNIVERSAL=TIFFSetupStrips
UNIVERSAL=TIFFLastDirectory
UNIVERSAL=TIFFSetDirectory
UNIVERSAL=TIFFSetSubDirectory
UNIVERSAL=TIFFUnlinkDirectory
UNIVERSAL=TIFFSetField
UNIVERSAL=TIFFVSetField
UNIVERSAL=TIFFCheckpointDirectory
UNIVERSAL=TIFFWriteDirectory
UNIVERSAL=TIFFRewriteDirectory
UNIVERSAL=TIFFPrintDirectory
UNIVERSAL=TIFFReadScanline
UNIVERSAL=TIFFWriteScanline
UNIVERSAL=TIFFReadRGBAImage
UNIVERSAL=TIFFReadRGBAImageOriented
UNIVERSAL=TIFFFdOpen
UNIVERSAL=TIFFClientOpen
UNIVERSAL=TIFFFileName
UNIVERSAL=TIFFError
UNIVERSAL=TIFFErrorExt
UNIVERSAL=TIFFWarning
UNIVERSAL=TIFFWarningExt
UNIVERSAL=TIFFSetErrorHandler
UNIVERSAL=TIFFSetErrorHandlerExt
UNIVERSAL=TIFFSetWarningHandler
UNIVERSAL=TIFFSetWarningHandlerExt
UNIVERSAL=TIFFComputeTile
UNIVERSAL=TIFFCheckTile
UNIVERSAL=TIFFNumberOfTiles
UNIVERSAL=TIFFReadTile
UNIVERSAL=TIFFWriteTile
UNIVERSAL=TIFFComputeStrip
UNIVERSAL=TIFFNumberOfStrips
UNIVERSAL=TIFFRGBAImageBegin
UNIVERSAL=TIFFRGBAImageGet
UNIVERSAL=TIFFRGBAImageEnd
UNIVERSAL=TIFFReadEncodedStrip
UNIVERSAL=TIFFReadRawStrip
UNIVERSAL=TIFFReadEncodedTile
UNIVERSAL=TIFFReadRawTile
UNIVERSAL=TIFFReadRGBATile
UNIVERSAL=TIFFReadRGBAStrip
UNIVERSAL=TIFFWriteEncodedStrip
UNIVERSAL=TIFFWriteRawStrip
UNIVERSAL=TIFFWriteEncodedTile
UNIVERSAL=TIFFWriteRawTile
UNIVERSAL=TIFFSetWriteOffset
UNIVERSAL=TIFFSwabDouble
UNIVERSAL=TIFFSwabShort
UNIVERSAL=TIFFSwabLong
UNIVERSAL=TIFFSwabArrayOfShort
UNIVERSAL=TIFFSwabArrayOfLong
UNIVERSAL=TIFFSwabArrayOfDouble
UNIVERSAL=TIFFSwabArrayOfTriples
UNIVERSAL=TIFFReverseBits
UNIVERSAL=TIFFGetBitRevTable
UNIVERSAL=TIFFDefaultStripSize
UNIVERSAL=TIFFDefaultTileSize
UNIVERSAL=TIFFRasterScanlineSize
UNIVERSAL=_TIFFmalloc
UNIVERSAL=_TIFFrealloc
UNIVERSAL=_TIFFfree
UNIVERSAL=_TIFFmemset
UNIVERSAL=_TIFFmemcpy
UNIVERSAL=_TIFFmemcmp
UNIVERSAL=TIFFCreateDirectory
UNIVERSAL=TIFFSetTagExtender
UNIVERSAL=TIFFMergeFieldInfo
UNIVERSAL=TIFFFindFieldInfo
UNIVERSAL=TIFFFindFieldInfoByName
UNIVERSAL=TIFFFieldWithName
UNIVERSAL=TIFFFieldWithTag
UNIVERSAL=TIFFFieldTag
UNIVERSAL=TIFFFieldName
UNIVERSAL=TIFFFieldDataType
UNIVERSAL=TIFFFieldPassCount
UNIVERSAL=TIFFFieldReadCount
UNIVERSAL=TIFFFieldWriteCount
UNIVERSAL=TIFFCurrentDirOffset
UNIVERSAL=TIFFWriteCheck
UNIVERSAL=TIFFRGBAImageOK
UNIVERSAL=TIFFNumberOfDirectories
UNIVERSAL=TIFFSetFileName
UNIVERSAL=TIFFSetClientdata
UNIVERSAL=TIFFSetMode
UNIVERSAL=TIFFClientdata
UNIVERSAL=TIFFGetReadProc
UNIVERSAL=TIFFGetWriteProc
UNIVERSAL=TIFFGetSeekProc
UNIVERSAL=TIFFGetCloseProc
UNIVERSAL=TIFFGetSizeProc
UNIVERSAL=TIFFGetMapFileProc
UNIVERSAL=TIFFGetUnmapFileProc
UNIVERSAL=TIFFIsCODECConfigured
UNIVERSAL=TIFFGetConfiguredCODECs
UNIVERSAL=TIFFFindCODEC
UNIVERSAL=TIFFRegisterCODEC
UNIVERSAL=TIFFUnRegisterCODEC
UNIVERSAL=TIFFFreeDirectory
UNIVERSAL=TIFFReadCustomDirectory
UNIVERSAL=TIFFReadEXIFDirectory
UNIVERSAL=TIFFAccessTagMethods
UNIVERSAL=TIFFGetClientInfo
UNIVERSAL=TIFFSetClientInfo
UNIVERSAL=TIFFReassignTagToIgnore
 
$ENDIF
$!
$!
$! Writing TIFF$SHR.OPT file to build TOOLS
$ IF (SHARED.GT.0)
$   THEN
$       OPEN/WRITE OUT TIFF$SHR.OPT
$       WRITE OUT "[]TIFF/LIB"
$       WRITE OUT "[-.PORT]LIBPORT/LIB"
$       WRITE OUT JPEG_LIBRARY_PATH
$       WRITE OUT "ZLIB:LIBZ/LIB"
$       CLOSE OUT
$ ENDIF
$!
$!
$! Writing OPT.OPT file to build TOOLS
$OPEN/WRITE OUT OPT.OPT
$ IF (SHARED.GT.0)
$   THEN
$       WRITE OUT "[-.LIBTIFF]TIFF$SHR/SHARE"
$       WRITE OUT JPEG_LIBRARY_PATH
$   ELSE
$       WRITE OUT "[-.LIBTIFF]TIFF/LIB"
$       WRITE OUT "[-.PORT]LIBPORT/LIB"
$       WRITE OUT JPEG_LIBRARY_PATH
$ ENDIF
$ WRITE OUT "ZLIB:LIBZ/LIB"
$CLOSE OUT
$!
$!
$COPY SYS$INPUT [.LIBTIFF]DESCRIP.MMS
# (c) Alexey Chupahin 22-NOV-2007
# OpenVMS 7.3-1, DEC 2000 mod.300
# OpenVMS 8.3,   HP rx1620
# Makefile for DEC C compilers.
#

INCL    = /INCLUDE=(JPEG,ZLIB,[])

CFLAGS =  $(INCL)

OBJ_SYSDEP_MODULE = tif_vms.obj

OBJ     = \
tif_aux.obj,\
tif_close.obj,\
tif_codec.obj,\
tif_color.obj,\
tif_compress.obj,\
tif_dir.obj,\
tif_dirinfo.obj,\
tif_dirread.obj,\
tif_dirwrite.obj,\
tif_dumpmode.obj,\
tif_error.obj,\
tif_extension.obj,\
tif_fax3.obj,\
tif_fax3sm.obj,\
tif_flush.obj,\
tif_getimage.obj,\
tif_jbig.obj,\
tif_jpeg.obj,\
tif_luv.obj,\
tif_lzw.obj,\
tif_next.obj,\
tif_ojpeg.obj,\
tif_open.obj,\
tif_packbits.obj,\
tif_pixarlog.obj,\
tif_predict.obj,\
tif_print.obj,\
tif_read.obj,\
tif_strip.obj,\
tif_swab.obj,\
tif_thunder.obj,\
tif_tile.obj,\
tif_version.obj,\
tif_warning.obj,\
tif_write.obj,\
tif_zip.obj, $(OBJ_SYSDEP_MODULE)

$IF (SHARED.GT.0)
$ THEN
$       APP SYS$INPUT [.LIBTIFF]DESCRIP.MMS
ALL : tiff.olb, tiff$shr.exe
        $WRITE SYS$OUTPUT "Done"

tiff$shr.exe : tiff.olb
        LINK/SHARE=TIFF$SHR.EXE TIF_AUX,[-]TIFF$DEF/OPT, [-]TIFF$SHR/OPT
        COPY TIFF$SHR.EXE SYS$SHARE
        PURGE SYS$SHARE:TIFF$SHR.EXE

$ ELSE
$       APP SYS$INPUT [.LIBTIFF]DESCRIP.MMS
ALL : tiff.olb
        $WRITE SYS$OUTPUT "Done"

$ENDIF
$!
$!
$ APP SYS$INPUT [.LIBTIFF]DESCRIP.MMS

tiff.olb :  $(OBJ)
        lib/crea tiff.olb $(OBJ)

#tif_config.h : tif_config.h-vms
#        copy tif_config.h-vms tif_config.h
#
#tiffconf.h : tiffconf.h-vms
#        copy tiffconf.h-vms tiffconf.h

tif_aux.obj : tif_aux.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_close.obj : tif_close.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_codec.obj : tif_codec.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_color.obj : tif_color.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_compress.obj : tif_compress.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_dir.obj : tif_dir.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_dirinfo.obj : tif_dirinfo.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_dirread.obj : tif_dirread.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_dirwrite.obj : tif_dirwrite.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_dumpmode.obj : tif_dumpmode.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_error.obj : tif_error.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_extension.obj : tif_extension.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_fax3.obj : tif_fax3.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_fax3sm.obj : tif_fax3sm.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_flush.obj : tif_flush.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_getimage.obj : tif_getimage.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_jbig.obj : tif_jbig.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_jpeg.obj : tif_jpeg.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_luv.obj : tif_luv.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_lzw.obj : tif_lzw.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_next.obj : tif_next.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_ojpeg.obj : tif_ojpeg.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_open.obj : tif_open.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_packbits.obj : tif_packbits.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_pixarlog.obj : tif_pixarlog.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_predict.obj : tif_predict.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_print.obj : tif_print.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_read.obj : tif_read.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_strip.obj : tif_strip.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_swab.obj : tif_swab.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_thunder.obj : tif_thunder.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_tile.obj : tif_tile.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_unix.obj : tif_unix.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_version.obj : tif_version.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_warning.obj : tif_warning.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_write.obj : tif_write.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tif_zip.obj : tif_zip.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)


clean :
        del *.obj;*
        del *.olb;*
$!
$!
$!
$COPY SYS$INPUT [.TOOLS]DESCRIP.MMS
# (c) Alexey Chupahin 22-NOV-2007
# OpenVMS 7.3-1, DEC 2000 mod.300
# OpenVMS 8.3,   HP rx1620
 
INCL            = /INCL=([],[-.LIBTIFF])
CFLAGS = $(INCL)
LIBS = [-]OPT/OPT

OBJ=\
bmp2tiff.exe,\
fax2ps.exe,\
fax2tiff.exe,\
gif2tiff.exe,\
pal2rgb.exe,\
ppm2tiff.exe,\
ras2tiff.exe,\
raw2tiff.exe,\
rgb2ycbcr.exe,\
thumbnail.exe,\
tiff2bw.exe,\
tiff2pdf.exe,\
tiff2ps.exe,\
tiff2rgba.exe,\
tiffcmp.exe,\
tiffcp.exe,\
tiffcrop.exe,\
tiffdither.exe,\
tiffdump.exe,\
tiffinfo.exe,\
tiffmedian.exe,\
tiffset.exe,\
tiffsplit.exe,\
ycbcr.exe
 

all : $(OBJ)
	$!

bmp2tiff.obj : bmp2tiff.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

bmp2tiff.exe : bmp2tiff.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

fax2ps.obj : fax2ps.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

fax2ps.exe : fax2ps.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

fax2tiff.obj : fax2tiff.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

fax2tiff.exe : fax2tiff.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

gif2tiff.obj : gif2tiff.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

gif2tiff.exe : gif2tiff.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

pal2rgb.obj : pal2rgb.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

pal2rgb.exe : pal2rgb.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

ppm2tiff.obj : ppm2tiff.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

ppm2tiff.exe : ppm2tiff.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

ras2tiff.obj : ras2tiff.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

ras2tiff.exe : ras2tiff.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

raw2tiff.obj : raw2tiff.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

raw2tiff.exe : raw2tiff.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

rgb2ycbcr.obj : rgb2ycbcr.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

rgb2ycbcr.exe : rgb2ycbcr.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

sgi2tiff.obj : sgi2tiff.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

sgi2tiff.exe : sgi2tiff.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

sgisv.obj : sgisv.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

sgisv.exe : sgisv.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

thumbnail.obj : thumbnail.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

thumbnail.exe : thumbnail.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiff2bw.obj : tiff2bw.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiff2bw.exe : tiff2bw.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiff2pdf.obj : tiff2pdf.c
         $(CC) $(CFLAGS) /NOWARN $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiff2pdf.exe : tiff2pdf.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiff2ps.obj : tiff2ps.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiff2ps.exe : tiff2ps.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiff2rgba.obj : tiff2rgba.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiff2rgba.exe : tiff2rgba.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffcmp.obj : tiffcmp.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffcmp.exe : tiffcmp.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffcp.obj : tiffcp.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffcp.exe : tiffcp.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffcrop.obj : tiffcrop.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffcrop.exe : tiffcrop.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffdither.obj : tiffdither.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffdither.exe : tiffdither.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffdump.obj : tiffdump.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffdump.exe : tiffdump.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffgt.obj : tiffgt.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffgt.exe : tiffgt.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffinfo.obj : tiffinfo.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffinfo.exe : tiffinfo.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffmedian.obj : tiffmedian.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffmedian.exe : tiffmedian.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffset.obj : tiffset.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffset.exe : tiffset.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

tiffsplit.obj : tiffsplit.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

tiffsplit.exe : tiffsplit.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)

ycbcr.obj : ycbcr.c
         $(CC) $(CFLAGS) $(MMS$SOURCE) /OBJ=$(MMS$TARGET)

ycbcr.exe : ycbcr.obj
         LINK/EXE=$(MMS$TARGET)  $(MMS$SOURCE), $(LIBS)
 

CLEAN :
	DEL ALL.;*
	DEL *.OBJ;*
	DEL *.EXE;*

$!
$!
$!
$!copiing and patching TIFF_CONF.H, TIF_CONFIG.H
$!
$CURRENT = F$ENVIRONMENT (""DEFAULT"")
$CURRENT[F$LOCATE("]",CURRENT),9]:=".LIBTIFF]"
$WRITE SYS$OUTPUT "Creating TIFFCONF.H and TIF_CONFIG.H"
$COPY SYS$INPUT 'CURRENT'TIFFCONF.H
/*
  Configuration defines for installed libtiff.
  This file maintained for backward compatibility. Do not use definitions
  from this file in your programs.
*/

#ifndef _TIFFCONF_
#define _TIFFCONF_

/* Define to 1 if the system has the type `int16'. */
//#define HAVE_INT16

/* Define to 1 if the system has the type `int32'. */
//#define  HAVE_INT32

/* Define to 1 if the system has the type `int8'. */
//#define HAVE_INT8

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Compatibility stuff. */

/* Define as 0 or 1 according to the floating point format suported by the
   machine */

#ifdef __IEEE_FLOAT
#define HAVE_IEEEFP 1
#endif

#define HAVE_GETOPT 1

/* Set the native cpu bit order (FILLORDER_LSB2MSB or FILLORDER_MSB2LSB) */
#define HOST_FILLORDER FILLORDER_LSB2MSB

/* Native cpu byte order: 1 if big-endian (Motorola) or 0 if little-endian
   (Intel) */
#define HOST_BIGENDIAN 0

/* Support CCITT Group 3 & 4 algorithms */
#define CCITT_SUPPORT 1

/* Support LogLuv high dynamic range encoding */
#define LOGLUV_SUPPORT 1

/* Support LZW algorithm */
#define LZW_SUPPORT 1

/* Support NeXT 2-bit RLE algorithm */
#define NEXT_SUPPORT 1

/* Support Old JPEG compresson (read contrib/ojpeg/README first! Compilation
   fails with unpatched IJG JPEG library) */

/* Support Macintosh PackBits algorithm */
#define PACKBITS_SUPPORT 1

/* Support Pixar log-format algorithm (requires Zlib) */
#define PIXARLOG_SUPPORT 1

/* Support ThunderScan 4-bit RLE algorithm */
#define THUNDER_SUPPORT 1

/* Support Deflate compression */
/* #undef ZIP_SUPPORT */

/* Support strip chopping (whether or not to convert single-strip uncompressed
   images to mutiple strips of ~8Kb to reduce memory usage) */
#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP

/* Enable SubIFD tag (330) support */
#define SUBIFD_SUPPORT 1

/* Treat extra sample as alpha (default enabled). The RGBA interface will
   treat a fourth sample with no EXTRASAMPLE_ value as being ASSOCALPHA. Many
   packages produce RGBA files but don't mark the alpha properly. */
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1

/* Pick up YCbCr subsampling info from the JPEG data stream to support files
   lacking the tag (default enabled). */
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1

/*
 * Feature support definitions.
 * XXX: These macros are obsoleted. Don't use them in your apps!
 * Macros stays here for backward compatibility and should be always defined.
 */
#define COLORIMETRY_SUPPORT
#define YCBCR_SUPPORT
#define CMYK_SUPPORT
#define ICC_SUPPORT
#define PHOTOSHOP_SUPPORT
#define IPTC_SUPPORT

#endif /* _TIFFCONF_ */
 

$COPY SYS$INPUT 'CURRENT'TIF_CONFIG.H
/* Define to 1 if you have the <assert.h> header file. */

#ifndef HAVE_GETOPT
#  define HAVE_GETOPT 1
#endif

#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define as 0 or 1 according to the floating point format suported by the
   machine */

#ifdef __IEEE_FLOAT
#define HAVE_IEEEFP 1
#endif

#define HAVE_UNISTD_H 1

#define HAVE_STRING_H 1
/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
//#define HAVE_IO_H 1

/* Define to 1 if you have the <search.h> header file. */
//#define HAVE_SEARCH_H 1

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Set the native cpu bit order */
#define HOST_FILLORDER FILLORDER_LSB2MSB

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
/*
#ifndef __cplusplus
# ifndef inline
#  define inline __inline
# endif
#endif
*/

/* Support CCITT Group 3 & 4 algorithms */
#define CCITT_SUPPORT 1

/* Pick up YCbCr subsampling info from the JPEG data stream to support files
   lacking the tag (default enabled). */
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1
/* Support C++ stream API (requires C++ compiler) */
#define CXX_SUPPORT 1

/* Treat extra sample as alpha (default enabled). The RGBA interface will
   treat a fourth sample with no EXTRASAMPLE_ value as being ASSOCALPHA. Many
      packages produce RGBA files but don't mark the alpha properly. */
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1

/* little Endian */
#define HOST_BIGENDIAN 0
#define JPEG_SUPPORT 1
#define LOGLUV_SUPPORT 1
/* Support LZW algorithm */
#define LZW_SUPPORT 1

/* Support Microsoft Document Imaging format */
#define MDI_SUPPORT 1

/* Support NeXT 2-bit RLE algorithm */
#define NEXT_SUPPORT 1
#define OJPEG_SUPPORT 1

/* Name of package */
#define PACKAGE "tiff"


/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "tiff@lists.maptools.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "LibTIFF Software"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "LibTIFF Software 3.9.0beta for VMS"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "tiff"

$PURGE 'CURRENT'TIFFCONF.H
$PURGE 'CURRENT'TIF_CONFIG.H
$OPEN/APPEND OUT 'CURRENT'TIF_CONFIG.H
$IF HAVE_LFIND.EQ.1
$   THEN
$       WRITE OUT "#define HAVE_SEARCH_H 1"
$   ELSE
$       WRITE OUT "#undef HAVE_SEARCH_H"
$ENDIF
$CLOSE OUT
$!
$!
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT "Now you can type @BUILD "
$!
$EXIT:
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$DEL TEST.OBJ;*
$DEL TEST.C;*
$DEL TEST.EXE;*
$DEAS SYS$ERROR
$DEAS SYS$OUTPUT
