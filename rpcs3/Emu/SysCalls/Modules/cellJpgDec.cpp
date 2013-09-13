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
	CellJpgDecInParam inParam;
};

CellJpgDecInfo	current_info;
CellJpgDecSrc	current_src;


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

int cellJpgDecOpen(u32 mainHandle, u32 subHandle_addr, u32 src_addr, u32 openInfo)
{
	//current_src.srcSelect       = Memory.Read32(src_addr);
	current_src.fileName		  = Memory.Read32(src_addr+4);
	//current_src.fileOffset      = Memory.Read32(src_addr+8);
	//current_src.fileSize        = Memory.Read32(src_addr+12);
	//current_src.streamPtr       = Memory.Read32(src_addr+16);
	//current_src.streamSize      = Memory.Read32(src_addr+20);
	//current_src.spuThreadEnable = Memory.Read32(src_addr+24);

	CellJpgDecSubHandle *subHandle = new CellJpgDecSubHandle;

	// Get file descriptor
	u32 fd_addr = Memory.Alloc(sizeof(u32), 1);
	int ret = cellFsOpen(current_src.fileName, 0, fd_addr, NULL, 0);
	subHandle->fd = Memory.Read32(fd_addr);
	Memory.Free(fd_addr);
	if(ret != 0) return CELL_JPGDEC_ERROR_OPEN_FILE;

	// Get size of file
	u32 sb_addr = Memory.Alloc(52,1);					// Alloc a CellFsStat struct
	cellFsFstat(subHandle->fd, sb_addr);
	subHandle->fileSize = Memory.Read64(sb_addr+36);	// Get CellFsStat.st_size
	Memory.Free(sb_addr);

	// From now, every u32 subHandle argument is a pointer to a CellPngDecSubHandle struct.
	Memory.Write32(subHandle_addr, (u32)subHandle);

	return CELL_OK;
}

int cellJpgDecClose(u32 mainHandle, u32 subHandle)
{
	cellFsClose( ((CellJpgDecSubHandle*)subHandle)->fd );
	delete (CellJpgDecSubHandle*)subHandle;

	return CELL_OK;
}

int cellJpgDecReadHeader(u32 mainHandle, u32 subHandle, u32 info_addr)
{
	const u32& fd = ((CellJpgDecSubHandle*)subHandle)->fd;
	const u64& fileSize = ((CellJpgDecSubHandle*)subHandle)->fileSize;

	//Copy the JPG file to a buffer
	u32 buffer = Memory.Alloc(fileSize,1);
	u32 pos_addr = Memory.Alloc(8,1);
	cellFsLseek(fd, 0, 0, pos_addr);
	cellFsRead(fd, buffer, fileSize, NULL);
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
	current_info.numComponents    = 0;							// Unimplemented
	current_info.colorSpace       = 3;							// Unimplemented

	mem_class_t info(info_addr);
	info += current_info.imageWidth;
	info += current_info.imageHeight;
	info += current_info.numComponents;
	info += current_info.colorSpace;
	Memory.Free(buffer);
	
	return CELL_OK;
}

int cellJpgDecDecodeData(u32 mainHandle, u32 subHandle, u32 data_addr, u32 dataCtrlParam_addr, u32 dataOutInfo_addr)
{
	const u32& fd = ((CellJpgDecSubHandle*)subHandle)->fd;
	const u64& fileSize = ((CellJpgDecSubHandle*)subHandle)->fileSize;
	const CellJpgDecInParam& inParam = ((CellJpgDecSubHandle*)subHandle)->inParam; // (TODO: We should use the outParam)

	//Copy the JPG file to a buffer
	u32 buffer = Memory.Alloc(fileSize,1);
	u32 pos_addr = Memory.Alloc(8,1);
	cellFsLseek(fd, 0, 0, pos_addr);
	cellFsRead(fd, buffer, fileSize, NULL);
	Memory.Free(pos_addr);

	//Decode JPG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	unsigned char *jpg =  new unsigned char [fileSize];
	for(u32 i = 0; i < fileSize; i++){
		jpg[i] = Memory.Read8(buffer+i);
	}

	unsigned char *image = stbi_load_from_memory((const unsigned char*)jpg, fileSize, &width, &height, &actual_components, 4);
	if (!image)
	{
		Memory.Free(buffer);
		return CELL_JPGDEC_ERROR_STREAM_FORMAT; 
	}
	u32 image_size = width * height * 4;
	if (inParam.outputColorSpace == CELL_JPG_RGBA){
		for(u32 i = 0; i < image_size; i+=4){
			Memory.Write8(data_addr+i+0, image[i+0]);
			Memory.Write8(data_addr+i+1, image[i+1]);
			Memory.Write8(data_addr+i+2, image[i+2]);
			Memory.Write8(data_addr+i+3, image[i+3]);
		}
	}
	else if (inParam.outputColorSpace == CELL_JPG_ARGB){
		for(u32 i = 0; i < image_size; i+=4){
			Memory.Write8(data_addr+i+0, image[i+3]);
			Memory.Write8(data_addr+i+1, image[i+0]);
			Memory.Write8(data_addr+i+2, image[i+1]);
			Memory.Write8(data_addr+i+3, image[i+2]);
		}
	}
	Memory.Free(buffer);

	return CELL_OK;
}

int cellJpgDecSetParameter(u32 mainHandle, u32 subHandle, u32 inParam_addr, u32 outParam_addr)
{
	CellJpgDecInParam& inParam = ((CellJpgDecSubHandle*)subHandle)->inParam;
	inParam.outputColorSpace = Memory.Read32(inParam_addr+16);

	// (TODO)

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