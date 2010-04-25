/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009 zeydlitz@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2006
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Texture and avi saving to file functions

//------------------ Includes
#if defined(_WIN32)
#	include <windows.h>
#	include <aviUtil.h>
#	include "resource.h"
#endif
#include <stdlib.h>

#include "zerogs.h"
#include "targets.h"
#include "Mem.h"


extern "C" {
#ifdef _WIN32
#	define XMD_H
#	undef FAR
#define HAVE_BOOLEAN
#endif

#include "jpeglib.h" 	// This library want to be after zerogs.h
}

//------------------ Defines
#define 	TGA_FILE_NAME_MAX_LENGTH 	 20
#define		MAX_NUMBER_SAVED_TGA		200

//Windows have no snprintf
#if defined(_WIN32)
#	define snprintf sprintf_s
#endif
//------------------ Constants

//------------------ Global Variables
int TexNumber = 0;
int s_aviinit = 0;

//------------------ Code

// Set variables need to made a snapshoot when it's possible
void
ZeroGS::SaveSnapshot(const char* filename)
{
	g_bMakeSnapshot = 1;
	strSnapshot = filename;
}

// Save curent renderer in jpeg or TGA format
bool
ZeroGS::SaveRenderTarget(const char* filename, int width, int height, int jpeg)
{
	bool bflip = height < 0;
	height = abs(height);
	vector<u32> data(width*height);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	if (glGetError() != GL_NO_ERROR)
		return false;

	if (bflip) {
		// swap scanlines
		vector<u32> scanline(width);
		for (int i = 0; i < height/2; ++i) {
			memcpy(&scanline[0], &data[i * width], width * 4);
			memcpy(&data[i * width], &data[(height - i - 1) * width], width * 4);
			memcpy(&data[(height - i - 1) * width], &scanline[0], width * 4);
		}
	}

	if (jpeg)
		return SaveJPEG(filename, width, height, &data[0], 70);

	return SaveTGA(filename, width, height, &data[0]);
}

// Save selected texture as TGA
bool
ZeroGS::SaveTexture(const char* filename, u32 textarget, u32 tex, int width, int height)
{
	vector<u32> data(width*height);
	glBindTexture(textarget, tex);
	glGetTexImage(textarget, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	if (glGetError() != GL_NO_ERROR)
		return false;

	return SaveTGA(filename, width, height, &data[0]);
}

// save image as JPEG
bool
ZeroGS::SaveJPEG(const char* filename, int image_width, int image_height, const void* pdata, int quality)
{
	u8* image_buffer = new u8[image_width * image_height * 3];
	u8* psrc = (u8*)pdata;

	// input data is rgba format, so convert to rgb
	u8* p = image_buffer;
	for(int i = 0; i < image_height; ++i) {
		for(int j = 0; j < image_width; ++j) {
			p[0] = psrc[0];
			p[1] = psrc[1];
			p[2] = psrc[2];
			p += 3;
			psrc += 4;
		}
	}

	/* This struct contains the JPEG compression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	* It is possible to have several such structures, representing multiple
	* compression/decompression processes, in existence at once.  We refer
	* to any one struct (and its associated working data) as a "JPEG object".
	*/
	struct jpeg_compress_struct cinfo;
	/* This struct represents a JPEG error handler.  It is declared separately
	* because applications often want to supply a specialized error handler
	* (see the second half of this file for an example).  But here we just
	* take the easy way out and use the standard error handler, which will
	* print a message on stderr and call exit() if compression fails.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct jpeg_error_mgr jerr;
	/* More stuff */
	FILE * outfile;	 /* target file */
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int row_stride;	 /* physical row width in image buffer */

	/* Step 1: allocate and initialize JPEG compression object */

	/* We have to set up the error handler first, in case the initialization
	* step fails.  (Unlikely, but it could happen if you are out of memory.)
	* This routine fills in the contents of struct jerr, and returns jerr's
	* address which we place into the link field in cinfo.
	*/
	cinfo.err = jpeg_std_error(&jerr);
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */

	/* Here we use the library-supplied code to send compressed data to a
	* stdio stream.  You can also write your own code to do something else.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to write binary files.
	*/
	if ((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	/* Step 3: set parameters for compression */

	/* First we supply a description of the input image.
	* Four fields of the cinfo struct must be filled in:
	*/
	cinfo.image_width = image_width;	/* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 3;	 /* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB;	 /* colorspace of input image */
	/* Now use the library's routine to set default compression parameters.
	* (You must set at least cinfo.in_color_space before calling this,
	* since the defaults depend on the source color space.)
	*/
	jpeg_set_defaults(&cinfo);
	/* Now you can set any non-default parameters you wish to.
	* Here we just illustrate the use of quality (quantization table) scaling:
	*/
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	/* Step 4: Start compressor */

	/* TRUE ensures that we will write a complete interchange-JPEG file.
	* Pass TRUE unless you are very sure of what you're doing.
	*/
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*		   jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	* To keep things simple, we pass one scanline per call; you can pass
	* more if you wish, though.
	*/
	row_stride = image_width * 3;   /* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
  		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could pass
		* more than one scanline at a time if that's more convenient.
		*/
		row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	/* Step 6: Finish compression */

	jpeg_finish_compress(&cinfo);
	/* After finish_compress, we can close the output file. */
	fclose(outfile);

	/* Step 7: release JPEG compression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_compress(&cinfo);

	delete image_buffer;
	/* And we're done! */
	return true;
}

#if defined(_MSC_VER)
#	pragma pack(push, 1)
#endif

// This is the defenition of TGA header. We need it to function bellow
struct TGA_HEADER
{
	u8  identsize;		  	// size of ID field that follows 18 u8 header (0 usually)
	u8  colourmaptype;	  	// type of colour map 0=none, 1=has palette
	u8  imagetype;		  	// type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	s16 colourmapstart;	 	// first colour map entry in palette
	s16 colourmaplength;		// number of colours in palette
	u8  colourmapbits;	  	// number of bits per palette entry 15,16,24,32

	s16 xstart;			// image x origin
	s16 ystart;			// image y origin
	s16 width;			// image width in pixels
	s16 height;			// image height in pixels
	u8  bits;			// image bits per pixel 8,16,24,32
	u8  descriptor;		 	// image descriptor bits (vh flip bits)

					// pixel data follows header
#if defined(_MSC_VER)
};
#	pragma pack(pop)
#	else
} __attribute__((packed));
#endif

// Save image as TGA
bool
ZeroGS::SaveTGA(const char* filename, int width, int height, void* pdata)
{
	int err = 0;
	TGA_HEADER hdr;
	FILE* f = fopen(filename, "wb");
	if (f == NULL)
		return false;

	assert( sizeof(TGA_HEADER) == 18 && sizeof(hdr) == 18 );

	memset(&hdr, 0, sizeof(hdr));
	hdr.imagetype = 2;
	hdr.bits = 32;
	hdr.width = width;
	hdr.height = height;
	hdr.descriptor |= 8|(1<<5); 	// 8bit alpha, flip vertical

	err = fwrite(&hdr, sizeof(hdr), 1, f);
	err = fwrite(pdata, width * height * 4, 1, f);
	fclose(f);
	return true;
}

// AVI capture stuff
// AVI start -- set needed glabal variables
void ZeroGS::StartCapture()
{
	if( !s_aviinit ) {

#ifdef _WIN32
		START_AVI("zerogs.avi");
#else // linux
		//TODO
#endif
		s_aviinit = 1;
	}
	else {
		ERROR_LOG("Continuing from previous capture");
	}

	s_avicapturing = 1;
}

// Stop.
void ZeroGS::StopCapture()
{
	s_avicapturing = 0;
}

// And capture frame
// Does not work on linux
void ZeroGS::CaptureFrame()
{
	assert( s_avicapturing && s_aviinit );

	vector<u32> data(nBackbufferWidth*nBackbufferHeight);
	glReadPixels(0, 0, nBackbufferWidth, nBackbufferHeight, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	if (glGetError() != GL_NO_ERROR) return;

#ifdef _WIN32
	int fps = SMODE1->CMOD == 3 ? 50 : 60;

	bool bSuccess = ADD_FRAME_FROM_DIB_TO_AVI("AAAA", fps, nBackbufferWidth, nBackbufferHeight, 32, &data[0]);

	if( !bSuccess ) {
		s_avicapturing = 0;
		STOP_AVI();
		ZeroGS::AddMessage("Failed to create avi");
		return;
	}
#else // linux
	//TODO
#endif // _WIN32
}

// It's nearly the same as save texture
void
ZeroGS::SaveTex(tex0Info* ptex, int usevid)
{
	vector<u32> data(ptex->tw*ptex->th);
	vector<u8> srcdata;

	u32* dst = &data[0];
	u8* psrc = g_pbyGSMemory;

	CMemoryTarget* pmemtarg = NULL;

	if (usevid) {

		pmemtarg = g_MemTargs.GetMemoryTarget(*ptex, 0);
		assert( pmemtarg != NULL );

		glBindTexture(GL_TEXTURE_RECTANGLE_NV, pmemtarg->ptex->tex);
		srcdata.resize(pmemtarg->realheight * GPU_TEXWIDTH * pmemtarg->widthmult * 4 * 8); // max of 8 cannels

		glGetTexImage(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, pmemtarg->fmt, &srcdata[0]);

		u32 offset = pmemtarg->realy * 4 * GPU_TEXWIDTH;

		if (ptex->psm == PSMT8)
			offset *= PSMT_IS32BIT(ptex->cpsm) ? 4 : 2;
		else if (ptex->psm == PSMT4)
			offset *= PSMT_IS32BIT(ptex->cpsm) ? 8 : 4;

		psrc = &srcdata[0] - offset;
	}

	for (int i = 0; i < ptex->th; ++i) {
		for (int j = 0; j < ptex->tw; ++j) {
			u32 u = 0;
		        u32 addr;
			switch (ptex->psm) {
				case PSMCT32:
					addr = getPixelAddress32(j, i, ptex->tbp0, ptex->tbw);
					if (addr * 4 < 0x00400000)
						u = readPixel32(psrc, j, i, ptex->tbp0, ptex->tbw);
					else
						u = 0;
					break;
				case PSMCT24:
					addr = getPixelAddress24(j, i, ptex->tbp0, ptex->tbw);
					if (addr * 4 < 0x00400000)
						u = readPixel24(psrc, j, i, ptex->tbp0, ptex->tbw);
					else
						u = 0;
					break;
				case PSMCT16:
					addr = getPixelAddress16(j, i, ptex->tbp0, ptex->tbw);
					if (addr * 2 < 0x00400000) {
						u = readPixel16(psrc, j, i, ptex->tbp0, ptex->tbw);
						u = RGBA16to32(u);
					}
					else
						u = 0;
					break;
				case PSMCT16S:
					addr = getPixelAddress16(j, i, ptex->tbp0, ptex->tbw);
					if (addr * 2 < 0x00400000) {
						u = readPixel16S(psrc, j, i, ptex->tbp0, ptex->tbw);
						u = RGBA16to32(u);
					}
					else u = 0;
					break;

				case PSMT8:
					addr = getPixelAddress8(j, i, ptex->tbp0, ptex->tbw);
					if (addr < 0x00400000) {
						if (usevid) {
							if (PSMT_IS32BIT(ptex->cpsm))
								u = *(u32*)(psrc+4*addr);
							else
								u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel8(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else
						u = 0;
					break;

				case PSMT4:
					addr = getPixelAddress4(j, i, ptex->tbp0, ptex->tbw);

					if( addr < 2*0x00400000 ) {
						if( usevid ) {
							if (PSMT_IS32BIT(ptex->cpsm))
								u = *(u32*)(psrc+4*addr);
							else
							u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel4(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;
					break;

				case PSMT8H:
					addr = getPixelAddress8H(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 ) {
						if( usevid ) {
							if (PSMT_IS32BIT(ptex->cpsm))
								u = *(u32*)(psrc+4*addr);
							else
								u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel8H(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;

					break;

				case PSMT4HL:
					addr = getPixelAddress4HL(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 ) {
						if( usevid ) {
							if (PSMT_IS32BIT(ptex->cpsm))
								u = *(u32*)(psrc+4*addr);
							else
								u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel4HL(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;
					break;

				case PSMT4HH:
					addr = getPixelAddress4HH(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 ) {
						if( usevid ) {
							if (PSMT_IS32BIT(ptex->cpsm))
								u = *(u32*)(psrc+4*addr);
							else
								u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel4HH(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;
					break;

				case PSMT32Z:
					addr = getPixelAddress32Z(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 )
						u = readPixel32Z(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;
					break;

				case PSMT24Z:
					addr = getPixelAddress24Z(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 )
						u = readPixel24Z(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;
					break;

				case PSMT16Z:
					addr = getPixelAddress16Z(j, i, ptex->tbp0, ptex->tbw);

					if( 2*addr < 0x00400000 )
						u = readPixel16Z(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;
					break;

				case PSMT16SZ:
					addr = getPixelAddress16SZ(j, i, ptex->tbp0, ptex->tbw);

					if( 2*addr < 0x00400000 )
						u = readPixel16SZ(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;
					break;

				default:
					assert(0);
			}

			*dst++ = u;
		}
	}

	char Name[TGA_FILE_NAME_MAX_LENGTH];
	snprintf( Name, TGA_FILE_NAME_MAX_LENGTH, "Tex.%d.tga", TexNumber );
	SaveTGA(Name, ptex->tw, ptex->th, &data[0]);
}


// Do the save texture and return file name of it
// Do not forget to call free(), other wise there would be memory leak!
char*
ZeroGS::NamedSaveTex(tex0Info* ptex, int usevid){
	SaveTex(ptex, usevid);
	char* Name = (char*)malloc(TGA_FILE_NAME_MAX_LENGTH);
	snprintf( Name, TGA_FILE_NAME_MAX_LENGTH, "Tex.%d.tga", TexNumber );

	TexNumber++;
	if (TexNumber > MAX_NUMBER_SAVED_TGA)
		TexNumber = 0;

	return Name;
}

// Special function, wich is safe to call from any other file, without aviutils problems.
void
ZeroGS::Stop_Avi(){
#ifdef _WIN32
	STOP_AVI();
#else
// Does not support yet
#endif
}
