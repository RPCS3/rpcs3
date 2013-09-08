#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "lodepng/lodepng.h"
#include "lodepng/lodepng.cpp"

void cellPngDec_init();
Module cellPngDec(0x0018, cellPngDec_init);

//Return Codes
enum
{
	CELL_PNGDEC_ERROR_HEADER 			= 0x80611201,
	CELL_PNGDEC_ERROR_STREAM_FORMAT 	= 0x80611202,
	CELL_PNGDEC_ERROR_ARG 				= 0x80611203,
	CELL_PNGDEC_ERROR_SEQ 				= 0x80611204,
	CELL_PNGDEC_ERROR_BUSY 				= 0x80611205,
	CELL_PNGDEC_ERROR_FATAL 			= 0x80611206,
	CELL_PNGDEC_ERROR_OPEN_FILE 		= 0x80611207,
	CELL_PNGDEC_ERROR_SPU_UNSUPPORT 	= 0x80611208,
	CELL_PNGDEC_ERROR_SPU_ERROR 		= 0x80611209,
	CELL_PNGDEC_ERROR_CB_PARAM 			= 0x8061120a,
};

struct CellPngDecInfo
{
    u32 imageWidth;
    u32 imageHeight;
    u32 numComponents;
    u32 colorSpace;			// CellPngDecColorSpace
    u32 bitDepth;
	u32 interlaceMethod;	// CellPngDecInterlaceMode
	u32 chunkInformation;
};

struct CellPngDecSrc
{
    u32 srcSelect;			// CellPngDecStreamSrcSel
    u32 fileName_addr;		// const char*
    u64 fileOffset;			// int64_t
    u32 fileSize;
    u32 streamPtr;
    u32 streamSize;
    u32 spuThreadEnable;	// CellPngDecSpuThreadEna
};

CellPngDecInfo	current_info;
CellPngDecSrc	current_src;


int cellPngDecCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

int cellPngDecDestroy(u32 mainHandle)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

int cellPngDecOpen(u32 mainHandle, u32 subHandle_addr, u32 src_addr, u32 openInfo)
{
	//current_src.srcSelect       = Memory.Read32(src_addr);
	current_src.fileName_addr   = Memory.Read32(src_addr+4);
	//current_src.fileOffset      = Memory.Read32(src_addr+8);
	//current_src.fileSize        = Memory.Read32(src_addr+12);
	//current_src.streamPtr       = Memory.Read32(src_addr+16);
	//current_src.streamSize      = Memory.Read32(src_addr+20);
	//current_src.spuThreadEnable = Memory.Read32(src_addr+24);

	u32& fd_addr = subHandle_addr;						// Set file descriptor as sub handler of the decoder
	int ret = cellFsOpen(current_src.fileName_addr, 0, fd_addr, NULL, 0);
	if(ret != 0) return CELL_PNGDEC_ERROR_OPEN_FILE;

	return CELL_OK;
}

int cellPngDecClose(u32 mainHandle, u32 subHandle)
{
	u32& fd = subHandle;
	cellFsClose(fd);

	return CELL_OK;
}

int cellPngDecReadHeader(u32 mainHandle, u32 subHandle, u32 info_addr)
{
	u32& fd = subHandle;

	//Check size of file
	u32 sb_addr = Memory.Alloc(52,1);					// Alloc a CellFsStat struct
	cellFsFstat(fd, sb_addr);
	u64 fileSize = Memory.Read64(sb_addr+36);			// Get CellFsStat.st_size
	if(fileSize < 29) return CELL_PNGDEC_ERROR_HEADER;	// Error: The file is smaller than the length of a PNG header
	

	//Write the header to buffer
	u32 buffer = Memory.Alloc(34,1);					// Alloc buffer for PNG header
	u32 pos_addr = Memory.Alloc(8,1);
	cellFsLseek(fd, 0, 0, pos_addr);
	cellFsRead(fd, buffer, 34, NULL);

	if (Memory.Read32(buffer) != 0x89504E47 || Memory.Read32(buffer+4) != 0x0D0A1A0A)
		return CELL_PNGDEC_ERROR_HEADER; // Error: The first 8 bytes are not a valid PNG signature

	if (Memory.Read32(buffer+12) != 0x49484452)
		return CELL_PNGDEC_ERROR_HEADER; // Error: The PNG file does not start with an IHDR chunk

	current_info.imageWidth       = Memory.Read32(buffer+16);
	current_info.imageHeight      = Memory.Read32(buffer+20);
	current_info.numComponents    = 0;							// Unimplemented
	current_info.colorSpace       = Memory.Read8(buffer+25);
	current_info.bitDepth         = Memory.Read8(buffer+24);
	current_info.interlaceMethod  = Memory.Read8(buffer+28);
	current_info.chunkInformation = 0;							// Unimplemented

	mem_class_t info(info_addr);
	info += current_info.imageWidth;
	info += current_info.imageHeight;
	info += current_info.numComponents;
	info += current_info.colorSpace;
	info += current_info.bitDepth;
	info += current_info.interlaceMethod;
	info += current_info.chunkInformation;

	Memory.Free(sb_addr);
	Memory.Free(pos_addr);
	Memory.Free(buffer);
	
	return CELL_OK;
}

int cellPngDecDecodeData(u32 mainHandle, u32 subHandle, u32 data_addr, u32 dataCtrlParam_addr, u32 dataOutInfo_addr)
{
	u32& fd = subHandle;

	//Check size of file
	u32 sb_addr = Memory.Alloc(52,1);					// Alloc a CellFsStat struct
	cellFsFstat(fd, sb_addr);
	u64 fileSize = Memory.Read64(sb_addr+36);			// Get CellFsStat.st_size
	if(fileSize < 29) return CELL_PNGDEC_ERROR_HEADER;	// Error: The file is smaller than the length of a PNG header 

	//Write the header to buffer
	u32 buffer = Memory.Alloc(34,1);					// Alloc buffer for PNG header
	u32 pos_addr = Memory.Alloc(8,1);
	cellFsLseek(fd, 0, 0, pos_addr);
	cellFsRead(fd, buffer, 34, NULL);
	
	if (Memory.Read32(buffer) != 0x89504E47 || Memory.Read32(buffer+4) != 0x0D0A1A0A)
		return CELL_PNGDEC_ERROR_HEADER; // Error: The first 8 bytes are not a valid PNG signature

	if (Memory.Read32(buffer+12) != 0x49484452)
		return CELL_PNGDEC_ERROR_HEADER; // Error: The PNG file does not start with an IHDR chunk

	current_info.imageWidth       = Memory.Read32(buffer+16);
	current_info.imageHeight      = Memory.Read32(buffer+20);
	Memory.Free(buffer);

	//Decode PNG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	buffer = Memory.Alloc(fileSize,1);
	cellFsLseek(fd,0,0,pos_addr);
	cellFsRead(fd, buffer, fileSize, NULL);

	std::vector<unsigned char> png;    // PNG buffer
	std::vector<unsigned char> image;  // Raw buffer
	
	//Load contents in png buffer
	png.resize(size_t(fileSize));
	for(u32 i = 0; i < fileSize; i++){
		png[i] = Memory.Read8(buffer+i);
	}

	//Decode
	lodepng::decode(image, current_info.imageWidth, current_info.imageHeight, png);
	
	u32 image_size = image.size();
	for(u32 i = 0; i < image_size; i+=4){
		Memory.Write8(data_addr+i+0, image[i+3]);
		Memory.Write8(data_addr+i+1, image[i+0]);
		Memory.Write8(data_addr+i+2, image[i+1]);
		Memory.Write8(data_addr+i+3, image[i+2]);
	}

	Memory.Free(sb_addr);
	Memory.Free(pos_addr);
	Memory.Free(buffer);

	return CELL_OK;
}

int cellPngDecSetParameter(u32 mainHandle, u32 subHandle, u32 inParam, u32 outParam)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}


void cellPngDec_init()
{
	cellPngDec.AddFunc(0x157d30c5, cellPngDecCreate);
	cellPngDec.AddFunc(0x820dae1a, cellPngDecDestroy);
	cellPngDec.AddFunc(0xd2bc5bfd, cellPngDecOpen);
	cellPngDec.AddFunc(0x5b3d1ff1, cellPngDecClose);
	cellPngDec.AddFunc(0x9ccdcc95, cellPngDecReadHeader);
	cellPngDec.AddFunc(0x2310f155, cellPngDecDecodeData);
	cellPngDec.AddFunc(0xe97c9bd4, cellPngDecSetParameter);

	/*cellPngDec.AddFunc(0x48436b2d, cellPngDecExtCreate);
	cellPngDec.AddFunc(0x0c515302, cellPngDecExtOpen);
	cellPngDec.AddFunc(0x8b33f863, cellPngDecExtReadHeader);
	cellPngDec.AddFunc(0x726fc1d0, cellPngDecExtDecodeData);
	cellPngDec.AddFunc(0x9e9d7d42, cellPngDecExtSetParameter);
	cellPngDec.AddFunc(0x7585a275, cellPngDecGetbKGD);
	cellPngDec.AddFunc(0x7a062d26, cellPngDecGetcHRM);
	cellPngDec.AddFunc(0xb153629c, cellPngDecGetgAMA);
	cellPngDec.AddFunc(0xb905ebb7, cellPngDecGethIST);
	cellPngDec.AddFunc(0xf44b6c30, cellPngDecGetiCCP);
	cellPngDec.AddFunc(0x27c921b5, cellPngDecGetoFFs);
	cellPngDec.AddFunc(0xb4fe75e1, cellPngDecGetpCAL);
	cellPngDec.AddFunc(0x3d50016a, cellPngDecGetpHYs);
	cellPngDec.AddFunc(0x30cb334a, cellPngDecGetsBIT);
	cellPngDec.AddFunc(0xc41e1198, cellPngDecGetsCAL);
	cellPngDec.AddFunc(0xa5cdf57e, cellPngDecGetsPLT);
	cellPngDec.AddFunc(0xe4416e82, cellPngDecGetsRGB);
	cellPngDec.AddFunc(0x35a6846c, cellPngDecGettIME);
	cellPngDec.AddFunc(0xb96fb26e, cellPngDecGettRNS);
	cellPngDec.AddFunc(0xe163977f, cellPngDecGetPLTE);
	cellPngDec.AddFunc(0x609ec7d5, cellPngDecUnknownChunks);
	cellPngDec.AddFunc(0xb40ca175, cellPngDecGetTextChunk);*/
}