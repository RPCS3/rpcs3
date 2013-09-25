#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "stblib/stb_image.h"

void cellJpgDec_init();
Module cellJpgDec(0x000f, cellJpgDec_init);

//Return Codes
enum
{
	CELL_JPGDEC_ERROR_HEADER			= 0x80611101,
	CELL_JPGDEC_ERROR_STREAM_FORMAT		= 0x80611102,
	CELL_JPGDEC_ERROR_ARG				= 0x80611103,
	CELL_JPGDEC_ERROR_SEQ				= 0x80611104,
	CELL_JPGDEC_ERROR_BUSY				= 0x80611105,
	CELL_JPGDEC_ERROR_FATAL				= 0x80611106,
	CELL_JPGDEC_ERROR_OPEN_FILE			= 0x80611107,
	CELL_JPGDEC_ERROR_SPU_UNSUPPORT		= 0x80611108,
	CELL_JPGDEC_ERROR_CB_PARAM			= 0x80611109,
};

enum CellJpgDecColorSpace
{
	CELL_JPG_UNKNOWN					= 0,
	CELL_JPG_GRAYSCALE					= 1,
	CELL_JPG_RGB						= 2,
	CELL_JPG_YCbCr						= 3,
	CELL_JPG_RGBA						= 10,
	CELL_JPG_UPSAMPLE_ONLY				= 11,
	CELL_JPG_ARGB						= 20,
	CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA	= 40,
	CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB	= 41,
};

struct CellJpgDecInfo
{
    u32 imageWidth;
    u32 imageHeight;
    u32 numComponents;
    u32 colorSpace;			// CellJpgDecColorSpace
};

struct CellJpgDecSrc
{
    u32 srcSelect;			// CellJpgDecStreamSrcSel
    u32 fileName;			// const char*
    u64 fileOffset;			// int64_t
    u32 fileSize;
    u32 streamPtr;
    u32 streamSize;
    u32 spuThreadEnable;	// CellJpgDecSpuThreadEna
};

struct CellJpgDecInParam
{
	u32 *commandPtr;
	u32 downScale;
	u32 method;				// CellJpgDecMethod
	u32 outputMode;			// CellJpgDecOutputMode
	u32 outputColorSpace;	// CellJpgDecColorSpace
	u8 outputColorAlpha;
	u8 reserved[3];
};

struct CellJpgDecOutParam
{
	u64 outputWidthByte;
	u32 outputWidth;
	u32 outputHeight;
	u32 outputComponents;
	u32 outputMode;			// CellJpgDecOutputMode
	u32 outputColorSpace;	// CellJpgDecColorSpace
	u32 downScale;
	u32 useMemorySpace;
};

struct CellJpgDecSubHandle	//Custom struct
{
	u32 fd;
	u64 fileSize;
	CellJpgDecInfo info;
	CellJpgDecOutParam outParam;
};


int cellJpgDecCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam)
{
	UNIMPLEMENTED_FUNC(cellJpgDec);
	return CELL_OK;
}

int cellJpgDecExtCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam, u32 extThreadInParam, u32 extThreadOutParam)
{
	UNIMPLEMENTED_FUNC(cellJpgDec);
	return CELL_OK;
}

int cellJpgDecDestroy(u32 mainHandle)
{
	UNIMPLEMENTED_FUNC(cellJpgDec);
	return CELL_OK;
}

int cellJpgDecOpen(u32 mainHandle, mem32_t subHandle, u32 src_addr, u32 openInfo)
{
	//u32 srcSelect       = Memory.Read32(src_addr);
	u32 fileName		  = Memory.Read32(src_addr+4);
	//u64 fileOffset      = Memory.Read32(src_addr+8);
	//u32 fileSize        = Memory.Read32(src_addr+12);
	//u32 streamPtr       = Memory.Read32(src_addr+16);
	//u32 streamSize      = Memory.Read32(src_addr+20);
	//u32 spuThreadEnable = Memory.Read32(src_addr+24);

	CellJpgDecSubHandle *current_subHandle = new CellJpgDecSubHandle;

	// Get file descriptor
	u32 fd_addr = Memory.Alloc(sizeof(u32), 1);
	int ret = cellFsOpen(fileName, 0, fd_addr, NULL, 0);
	current_subHandle->fd = Memory.Read32(fd_addr);
	Memory.Free(fd_addr);
	if(ret != 0) return CELL_JPGDEC_ERROR_OPEN_FILE;

	// Get size of file
	u32 sb_addr = Memory.Alloc(52,1);					// Alloc a CellFsStat struct
	cellFsFstat(current_subHandle->fd, sb_addr);
	current_subHandle->fileSize = Memory.Read64(sb_addr+36);	// Get CellFsStat.st_size
	Memory.Free(sb_addr);

	// From now, every u32 subHandle argument is a pointer to a CellPngDecSubHandle struct.
	subHandle += (u32)current_subHandle;

	return CELL_OK;
}

int cellJpgDecClose(u32 mainHandle, u32 subHandle)
{
	cellFsClose( ((CellJpgDecSubHandle*)subHandle)->fd );
	delete (CellJpgDecSubHandle*)subHandle;

	return CELL_OK;
}

int cellJpgDecReadHeader(u32 mainHandle, u32 subHandle, mem_class_t info)
{
	const u32& fd = ((CellJpgDecSubHandle*)subHandle)->fd;
	const u64& fileSize = ((CellJpgDecSubHandle*)subHandle)->fileSize;
	CellJpgDecInfo& current_info = ((CellJpgDecSubHandle*)subHandle)->info;

	//Copy the JPG file to a buffer
	u32 buffer = Memory.Alloc(fileSize,1);
	u32 nread = Memory.Alloc(8,1);
	u32 pos_addr = Memory.Alloc(8,1);
	cellFsLseek(fd, 0, 0, pos_addr);
	cellFsRead(fd, buffer, fileSize, nread);
	Memory.Free(nread);
	Memory.Free(pos_addr);

	if (Memory.Read32(buffer) != 0xFFD8FFE0 ||			// Error: Not a valid SOI header
		Memory.Read32(buffer+6) != 0x4A464946)			// Error: Not a valid JFIF string
	{
		Memory.Free(buffer);
		return CELL_JPGDEC_ERROR_HEADER; 
	}

	u32 i = 4;
	u16 block_length = Memory.Read8(buffer+i)*0xFF + Memory.Read8(buffer+i+1);
	while(i < fileSize)
	{
            i += block_length;															// Increase the file index to get to the next block
            if (i >= fileSize){
				Memory.Free(buffer);
				return CELL_JPGDEC_ERROR_HEADER;										// Check to protect against segmentation faults
			}
            if(Memory.Read8(buffer+i) != 0xFF){
				Memory.Free(buffer);
				return CELL_JPGDEC_ERROR_HEADER;										// Check that we are truly at the start of another block
			}																			
            if(Memory.Read8(buffer+i+1) == 0xC0){
				break;																	// 0xFFC0 is the "Start of frame" marker which contains the file size
			}
            i += 2;                                                                     // Skip the block marker
            block_length = Memory.Read8(buffer+i)*0xFF + Memory.Read8(buffer+i+1);      // Go to the next block
    } 

	current_info.imageWidth       = Memory.Read8(buffer+i+7)*256 + Memory.Read8(buffer+i+8);
	current_info.imageHeight      = Memory.Read8(buffer+i+5)*256 + Memory.Read8(buffer+i+6);
	current_info.numComponents    = 3;							// Unimplemented
	current_info.colorSpace       = CELL_JPG_RGB;				// Unimplemented

	info += current_info.imageWidth;
	info += current_info.imageHeight;
	info += current_info.numComponents;
	info += current_info.colorSpace;
	Memory.Free(buffer);
	
	return CELL_OK;
}

int cellJpgDecDecodeData(u32 mainHandle, u32 subHandle, mem8_ptr_t data, u32 dataCtrlParam_addr, u32 dataOutInfo_addr)
{
	const u32& fd = ((CellJpgDecSubHandle*)subHandle)->fd;
	const u64& fileSize = ((CellJpgDecSubHandle*)subHandle)->fileSize;
	const CellJpgDecOutParam& current_outParam = ((CellJpgDecSubHandle*)subHandle)->outParam; // (TODO: We should use the outParam)

	//Copy the JPG file to a buffer
	u32 buffer = Memory.Alloc(fileSize,1);
	u32 nread = Memory.Alloc(8,1);
	u32 pos_addr = Memory.Alloc(8,1);
	cellFsLseek(fd, 0, 0, pos_addr);
	cellFsRead(fd, buffer, fileSize, nread);
	Memory.Free(nread);
	Memory.Free(pos_addr);

	//Decode JPG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	unsigned char *jpg = (unsigned char*)Memory.VirtualToRealAddr(buffer);
	unsigned char *image = stbi_load_from_memory(jpg, fileSize, &width, &height, &actual_components, 4);
	Memory.Free(buffer);
	if (!image) return CELL_JPGDEC_ERROR_STREAM_FORMAT;

	u32 image_size = width * height * 4;
	if (current_outParam.outputColorSpace == CELL_JPG_RGBA){
		for(u32 i = 0; i < image_size; i+=4){
			data += image[i+0];
			data += image[i+1];
			data += image[i+2];
			data += image[i+3];
		}
	}
	else if (current_outParam.outputColorSpace == CELL_JPG_ARGB){
		for(u32 i = 0; i < image_size; i+=4){
			data += image[i+3];
			data += image[i+0];
			data += image[i+1];
			data += image[i+2];
		}
	}
	delete[] image;

	return CELL_OK;
}

int cellJpgDecSetParameter(u32 mainHandle, u32 subHandle, u32 inParam_addr, mem_class_t outParam)
{
	CellJpgDecInfo& current_info = ((CellJpgDecSubHandle*)subHandle)->info;
	CellJpgDecOutParam& current_outParam = ((CellJpgDecSubHandle*)subHandle)->outParam;

	current_outParam.outputWidthByte	= (current_info.imageWidth * current_info.numComponents);
	current_outParam.outputWidth		= current_info.imageWidth;
	current_outParam.outputHeight		= current_info.imageHeight;
	current_outParam.outputColorSpace	= Memory.Read32(inParam_addr+16);
	switch (current_outParam.outputColorSpace)
	{
	case CELL_JPG_GRAYSCALE:				current_outParam.outputComponents = 1; break;
	case CELL_JPG_RGB:						current_outParam.outputComponents = 3; break;
	case CELL_JPG_YCbCr:					current_outParam.outputComponents = 3; break;
	case CELL_JPG_RGBA:						current_outParam.outputComponents = 4; break;
	case CELL_JPG_UPSAMPLE_ONLY:			current_outParam.outputComponents = current_info.numComponents; break;
	case CELL_JPG_ARGB:						current_outParam.outputComponents = 4; break;
	case CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA:	current_outParam.outputComponents = 4; break;
	case CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB:	current_outParam.outputComponents = 4; break;
	default: return CELL_JPGDEC_ERROR_ARG;		// Not supported color space
	}
	current_outParam.outputMode			= Memory.Read32(inParam_addr+12);
	current_outParam.downScale			= Memory.Read32(inParam_addr+4);
	current_outParam.useMemorySpace		= 0;	// Unimplemented

	outParam += current_outParam.outputWidthByte;
	outParam += current_outParam.outputWidth;
	outParam += current_outParam.outputHeight;
	outParam += current_outParam.outputComponents;
	outParam += current_outParam.outputMode;
	outParam += current_outParam.outputColorSpace;
	outParam += current_outParam.downScale;
	outParam += current_outParam.useMemorySpace;

	return CELL_OK;
}


void cellJpgDec_init()
{
	cellJpgDec.AddFunc(0xa7978f59, cellJpgDecCreate);
	cellJpgDec.AddFunc(0x8b300f66, cellJpgDecExtCreate);
	cellJpgDec.AddFunc(0x976ca5c2, cellJpgDecOpen);
	cellJpgDec.AddFunc(0x6d9ebccf, cellJpgDecReadHeader);
	cellJpgDec.AddFunc(0xe08f3910, cellJpgDecSetParameter);
	cellJpgDec.AddFunc(0xaf8bb012, cellJpgDecDecodeData);
	cellJpgDec.AddFunc(0x9338a07a, cellJpgDecClose);
	cellJpgDec.AddFunc(0xd8ea91f8, cellJpgDecDestroy);

	/*cellJpgDec.AddFunc(0xa9f703e3, cellJpgDecExtOpen);
	cellJpgDec.AddFunc(0xb91eb3d2, cellJpgDecExtReadHeader);
	cellJpgDec.AddFunc(0x65cbbb16, cellJpgDecExtSetParameter);
	cellJpgDec.AddFunc(0x716f8792, cellJpgDecExtDecodeData);*/
}