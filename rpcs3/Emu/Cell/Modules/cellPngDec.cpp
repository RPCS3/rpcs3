#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_fs.h"
#include "png.h"
#include "cellPngDec.h"

logs::channel cellPngDec("cellPngDec", logs::level::notice);

// cellPngDec aliases to improve readability
using PPHandle           = vm::pptr<PngHandle>;
using PHandle            = vm::ptr<PngHandle>;
using PThreadInParam     = vm::cptr<CellPngDecThreadInParam>;
using PThreadOutParam    = vm::ptr<CellPngDecThreadOutParam>;
using PExtThreadInParam  = vm::cptr<CellPngDecExtThreadInParam>;
using PExtThreadOutParam = vm::ptr<CellPngDecExtThreadOutParam>;
using PPStream           = vm::pptr<PngStream>;
using PStream            = vm::ptr<PngStream>;
using PSrc               = vm::cptr<CellPngDecSrc>;
using POpenInfo          = vm::ptr<CellPngDecOpnInfo>;
using POpenParam         = vm::cptr<CellPngDecOpnParam>;
using PInfo              = vm::ptr<CellPngDecInfo>;
using PExtInfo           = vm::ptr<CellPngDecExtInfo>;
using PInParam           = vm::cptr<CellPngDecInParam>;
using POutParam          = vm::ptr<CellPngDecOutParam>;
using PExtInParam        = vm::cptr<CellPngDecExtInParam>;
using PExtOutParam       = vm::ptr<CellPngDecExtOutParam>;
using PDataControlParam  = vm::cptr<CellPngDecDataCtrlParam>;
using PDataOutInfo       = vm::ptr<CellPngDecDataOutInfo>;
using PCbControlDisp     = vm::cptr<CellPngDecCbCtrlDisp>;
using PCbControlStream   = vm::cptr<CellPngDecCbCtrlStrm>;
using PDispParam         = vm::ptr<CellPngDecDispParam>;

// Custom read function for libpng, so we could decode images from a buffer
void pngDecReadBuffer(png_structp png_ptr, png_bytep out, png_size_t length)
{
	// Get the IO pointer
	png_voidp io_ptr = png_get_io_ptr(png_ptr);

	// Check if obtaining of the IO pointer failed
	if (!io_ptr)
	{
		cellPngDec.error("Failed to obtain the io_ptr failed.");
		return;
	}

	// Cast the IO pointer to our custom structure
	PngBuffer& buffer = *(PngBuffer*)io_ptr;

	// Read froma  file or a buffer
	if (buffer.file)
	{
		// Get the file
		auto file = idm::get<lv2_file_t>(buffer.fd);

		// Read the data
		file->file.read(out, length);
	}
	else
	{
		// Get the current data pointer, including the current cursor position
		void* data = static_cast<u8*>(buffer.data.get_ptr()) + buffer.cursor;

		// Copy the length of the current data pointer to the output
		memcpy(out, data, length);

		// Increment the cursor for the next time
		buffer.cursor += length;
	}
}

// Custom error handler for libpng
void pngDecError(png_structp png_ptr, png_const_charp error_message)
{
	cellPngDec.error(error_message);
}

// Custom warning handler for libpng
void pngDecWarning(png_structp png_ptr, png_const_charp error_message)
{
	cellPngDec.warning(error_message);
}

// Get the chunk information of the PNG file. IDAT is marked as existing, only after decoding or reading the header.
// Bits (if set indicates existence of the chunk):
// 0  - gAMA
// 1  - sBIT
// 2  - cHRM
// 3  - PLTE
// 4  - tRNS
// 5  - bKGD
// 6  - hIST
// 7  - pHYs
// 8  - oFFs
// 9  - tIME
// 10 - pCAL
// 11 - sRGB
// 12 - iCCP
// 13 - sPLT
// 14 - sCAL
// 15 - IDAT
// 16:30 - reserved
be_t<u32> pngDecGetChunkInformation(PStream stream, bool IDAT = false)
{
	// The end result of the chunk information (bigger-endian)
	be_t<u32> chunk_information = 0;

	// Needed pointers for getting the chunk information
	f64 gamma;
	f64 red_x;
	f64 red_y;
	f64 green_x;
	f64 green_y;
	f64 blue_x;
	f64 blue_y;
	f64 white_x;
	f64 white_y;
	f64 width;
	f64 height;
	s32 intent;
	s32 num_trans;
	s32 num_palette;
	s32 unit_type;
	s32 type;
	s32 nparams;
	s32 compression_type;
	s32 unit;
	u16* hist;
	u32 proflen;
	png_bytep profile;
	png_bytep trans_alpha;
	png_charp units;
	png_charp name;
	png_charp purpose;
	png_charpp params;
	png_int_32 X0;
	png_int_32 X1;
	png_int_32 offset_x;
	png_int_32 offset_y;
	png_uint_32 res_x;
	png_uint_32 res_y;
	png_colorp palette;
	png_color_8p sig_bit;
	png_color_16p background;
	png_color_16p trans_color;
	png_sPLT_tp entries;
	png_timep mod_time;

	// Get chunk information and set the appropriate bits
	if (png_get_gAMA(stream->png_ptr, stream->info_ptr, &gamma))
	{
		chunk_information |= 1 << 0; // gAMA
	}

	if (png_get_sBIT(stream->png_ptr, stream->info_ptr, &sig_bit))
	{
		chunk_information |= 1 << 1; // sBIT
	}

	if (png_get_cHRM(stream->png_ptr, stream->info_ptr, &white_x, &white_y, &red_x, &red_y, &green_x, &green_y, &blue_x, &blue_y))
	{
		chunk_information |= 1 << 2; // cHRM
	}

	if (png_get_PLTE(stream->png_ptr, stream->info_ptr, &palette, &num_palette))
	{
		chunk_information |= 1 << 3; // PLTE
	}

	if (png_get_tRNS(stream->png_ptr, stream->info_ptr, &trans_alpha, &num_trans, &trans_color))
	{
		chunk_information |= 1 << 4; // tRNS
	}

	if (png_get_bKGD(stream->png_ptr, stream->info_ptr, &background))
	{
		chunk_information |= 1 << 5; // bKGD
	}

	if (png_get_hIST(stream->png_ptr, stream->info_ptr, &hist))
	{
		chunk_information |= 1 << 6; // hIST
	}

	if (png_get_pHYs(stream->png_ptr, stream->info_ptr, &res_x, &res_y, &unit_type))
	{
		chunk_information |= 1 << 7; // pHYs
	}

	if (png_get_oFFs(stream->png_ptr, stream->info_ptr, &offset_x, &offset_y, &unit_type))
	{
		chunk_information |= 1 << 8; // oFFs
	}

	if (png_get_tIME(stream->png_ptr, stream->info_ptr, &mod_time))
	{
		chunk_information |= 1 << 9; // tIME
	}

	if (png_get_pCAL(stream->png_ptr, stream->info_ptr, &purpose, &X0, &X1, &type, &nparams, &units, &params))
	{
		chunk_information |= 1 << 10; // pCAL
	}

	if (png_get_sRGB(stream->png_ptr, stream->info_ptr, &intent))
	{
		chunk_information |= 1 << 11; // sRGB
	}

	if (png_get_iCCP(stream->png_ptr, stream->info_ptr, &name, &compression_type, &profile, &proflen))
	{
		chunk_information |= 1 << 12; // iCCP
	}

	if (png_get_sPLT(stream->png_ptr, stream->info_ptr, &entries))
	{
		chunk_information |= 1 << 13; // sPLT
	}

	if (png_get_sCAL(stream->png_ptr, stream->info_ptr, &unit, &width, &height))
	{
		chunk_information |= 1 << 14; // sCAL
	}

	if (IDAT)
	{
		chunk_information |= 1 << 15; // IDAT
	}

	return chunk_information;
}

s32 pngDecCreate(PPUThread& ppu, PPHandle png_handle, PThreadInParam thread_in_param, PThreadOutParam thread_out_param, PExtThreadInParam extra_thread_in_param = vm::null, PExtThreadOutParam extra_thread_out_param = vm::null)
{
	// Check if partial image decoding is used
	if (extra_thread_out_param)
	{
		throw EXCEPTION("Partial image decoding is not supported.");
	}

	// Allocate memory for the decoder handle
	auto handle = vm::ptr<PngHandle>::make(thread_in_param->cbCtrlMallocFunc(ppu, sizeof(PngHandle), thread_in_param->cbCtrlMallocArg).addr());

	// Check if the memory allocation for the handle failed
	if (!handle)
	{
		cellPngDec.error("PNG decoder creation failed.");
		return CELL_PNGDEC_ERROR_FATAL;
	}

	// Set the allocation functions in the handle
	handle->malloc = thread_in_param->cbCtrlMallocFunc;
	handle->malloc_arg = thread_in_param->cbCtrlMallocArg;
	handle->free = thread_in_param->cbCtrlFreeFunc;
	handle->free_arg = thread_in_param->cbCtrlFreeArg;

	// Set handle pointer
	*png_handle = handle;

	// Set the version information
	thread_out_param->pngCodecVersion = PNGDEC_CODEC_VERSION;

	return CELL_OK;
}

s32 pngDecDestroy(PPUThread& ppu, PHandle handle)
{
	// Deallocate the decoder handle memory
	if (handle->free(ppu, handle, handle->free_arg) != 0)
	{
		cellPngDec.error("PNG decoder deallocation failed.");
		return CELL_PNGDEC_ERROR_FATAL;
	}

	return CELL_OK;
}

s32 pngDecOpen(PPUThread& ppu, PHandle handle, PPStream png_stream, PSrc source, POpenInfo open_info, PCbControlStream control_stream = vm::null, POpenParam open_param = vm::null)
{
	// Check if partial image decoding is used
	if (control_stream || open_param)
	{
		throw EXCEPTION("Partial image decoding is not supported.");
	}

	// Allocate memory for the stream structure
	auto stream = vm::ptr<PngStream>::make(handle->malloc(ppu, sizeof(PngStream), handle->malloc_arg).addr());

	// Check if the allocation of memory for the stream structure failed
	if (!stream)
	{
		cellPngDec.error("PNG stream creation failed.");
		return CELL_PNGDEC_ERROR_FATAL;
	}

	// Set memory info
	open_info->initSpaceAllocated = sizeof(PngStream);

	// Set the stream source to the source give by the game
	stream->source = *source;

	// Indicate that a fixed alpha value isn't used, if not specified otherwise
	stream->fixed_alpha = false;

	// Use virtual memory address as a handle
	*png_stream = stream;

	// Allocate memory for the PNG buffer for decoding
	auto buffer = vm::ptr<PngBuffer>::make(handle->malloc(ppu, sizeof(PngBuffer), handle->malloc_arg).addr());

	// Check for if the buffer structure allocation failed
	if (!buffer)
	{
		throw EXCEPTION("Memory allocation for the PNG buffer structure failed.");
	}

	// We might not be reading from a file stream
	buffer->file = false;

	// Set the buffer pointer in the stream structure, so we can later deallocate it
	stream->buffer = buffer;

	// Open the buffer/file and check the header
	u8 header[8];

	// Need to test it somewhere
	if (stream->source.fileOffset != 0)
	{
		throw EXCEPTION("Non-0 file offset not supported.");
	}

	// Depending on the source type, get the first 8 bytes
	if (source->srcSelect == CELL_PNGDEC_FILE)
	{
		// Open a file stream
		fs::file file_stream(vfs::get(stream->source.fileName.get_ptr()));
		
		// Check if opening of the PNG file failed
		if (!file_stream)
		{
			cellPngDec.error("Opening of PNG failed. (%s)", stream->source.fileName.get_ptr());
			return CELL_PNGDEC_ERROR_OPEN_FILE;
		}

		// Read the header
		if (file_stream.read(header, 8) != 8)
		{
			cellPngDec.error("PNG header is too small.");
			return CELL_PNGDEC_ERROR_HEADER;
		}

		// Get the file descriptor
		buffer->fd = idm::make<lv2_file_t>(std::move(file_stream), 0, 0);

		// Indicate that we need to read from a file stream
		buffer->file = true;
	}
	else
	{
		// We can simply copy the first 8 bytes
		memcpy(header, stream->source.streamPtr.get_ptr(), 8);
	}

	// Check if the header indicates a valid PNG file
	if (png_sig_cmp(header, 0, 8))
	{
		cellPngDec.error("PNG signature is invalid.");
		return CELL_PNGDEC_ERROR_HEADER;
	}

	// Create a libpng structure, also pass our custom error/warning functions
	stream->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, pngDecError, pngDecWarning);

	// Check if the creation of the structure failed
	if (!stream->png_ptr)
	{
		cellPngDec.error("Creation of png_structp failed.");
		return CELL_PNGDEC_ERROR_FATAL;
	}

	// Create a libpng info structure
	stream->info_ptr = png_create_info_struct(stream->png_ptr);

	// Check if the creation of the structure failed
	if (!stream->info_ptr)
	{
		throw EXCEPTION("Creation of png_infop failed.");
	}

	// Set a point to return to when an error occurs in libpng
	if (setjmp(png_jmpbuf(stream->png_ptr)))
	{
		throw EXCEPTION("Fatal error in libpng.");
	}

	// We must indicate, that we allocated more memory
	open_info->initSpaceAllocated += sizeof(PngBuffer);

	// Init the IO for either reading from a file or a buffer
	if (source->srcSelect == CELL_PNGDEC_BUFFER)
	{
		// Set the data pointer and the file size
		buffer->length = stream->source.fileSize;
		buffer->data = stream->source.streamPtr;

		// Since we already read the header, we start reading from position 8
		buffer->cursor = 8;
	}

	// Set the custom read function for decoding
	png_set_read_fn(stream->png_ptr, buffer.get_ptr(), pngDecReadBuffer);

	// We need to tell libpng, that we already read 8 bytes
	png_set_sig_bytes(stream->png_ptr, 8);

	// Read the basic info of the PNG file
	png_read_info(stream->png_ptr, stream->info_ptr);

	// Read the header info for future use
	stream->info.imageWidth = png_get_image_width(stream->png_ptr, stream->info_ptr);
	stream->info.imageHeight = png_get_image_height(stream->png_ptr, stream->info_ptr);
	stream->info.numComponents = png_get_channels(stream->png_ptr, stream->info_ptr);
	stream->info.colorSpace = getPngDecColourType(png_get_color_type(stream->png_ptr, stream->info_ptr));
	stream->info.bitDepth = png_get_bit_depth(stream->png_ptr, stream->info_ptr);
	stream->info.interlaceMethod = png_get_interlace_type(stream->png_ptr, stream->info_ptr);
	stream->info.chunkInformation = pngDecGetChunkInformation(stream);

	return CELL_OK;
}

s32 pngDecClose(PPUThread& ppu, PHandle handle, PStream stream)
{
	// Remove the file descriptor, if a file descriptor was used for decoding
	if (stream->buffer->file)
	{
		idm::remove<lv2_file_t>(stream->buffer->fd);
	}

	// Deallocate the PNG buffer structure used to decode from memory, if we decoded from memory
	if (stream->buffer)
	{
		if (handle->free(ppu, stream->buffer, handle->free_arg) != 0)
		{
			cellPngDec.error("PNG buffer decoding structure deallocation failed.");
			return CELL_PNGDEC_ERROR_FATAL;
		}
	}

	// Free the memory allocated by libpng
	png_destroy_read_struct(&stream->png_ptr, &stream->info_ptr, nullptr);

	// Deallocate the stream memory
	if (handle->free(ppu, stream, handle->free_arg) != 0)
	{
		cellPngDec.error("PNG stream deallocation failed.");
		return CELL_PNGDEC_ERROR_FATAL;
	}

	return CELL_OK;
}

s32 pngReadHeader(PStream stream, PInfo info, PExtInfo extra_info = vm::null)
{
	// Set the pointer to stream info - we already get the header info, when opening the decoder
	*info = stream->info;

	// Set the reserved value to 0, if passed to the function.
	if (extra_info)
	{
		extra_info->reserved = 0;
	}

	return CELL_OK;
}

s32 pngDecSetParameter(PStream stream, PInParam in_param, POutParam out_param, PExtInParam extra_in_param = vm::null, PExtOutParam extra_out_param = vm::null)
{
	// Partial image decoding is not supported. Need to find games to test with.
	if (extra_in_param || extra_out_param)
	{
		throw EXCEPTION("Partial image decoding is not supported");
	}

	if (in_param->outputPackFlag == CELL_PNGDEC_1BYTE_PER_NPIXEL)
	{
		throw EXCEPTION("Packing not supported! (%d)", in_param->outputPackFlag);
	}

	// We already grab the basic info, when opening the stream, so we simply need to pass most of the values
	stream->out_param.outputWidth = stream->info.imageWidth;
	stream->out_param.outputHeight = stream->info.imageHeight;
	stream->out_param.outputColorSpace = in_param->outputColorSpace;
	stream->out_param.outputBitDepth = stream->info.bitDepth;
	stream->out_param.outputMode = in_param->outputMode;
	stream->out_param.outputWidthByte = png_get_rowbytes(stream->png_ptr, stream->info_ptr);
	stream->packing = in_param->outputPackFlag;

	// Check if a fixed alpha value is specified
	if (in_param->outputAlphaSelect == CELL_PNGDEC_FIX_ALPHA)
	{
		// We set the fixed alpha value in the stream structure, for padding while decoding
		stream->fixed_alpha = true;
		stream->fixed_alpha_colour = in_param->outputColorAlpha;
	}

	// Remap the number of output components based on the passed colorSpace value
	switch (stream->out_param.outputColorSpace)
	{
	case CELL_PNGDEC_RGBA:
	case CELL_PNGDEC_ARGB:
	{
		stream->out_param.outputComponents = 4;
		break;
	}

	case CELL_PNGDEC_RGB:
	{
		stream->out_param.outputComponents = 3;
		break;
	}

	case CELL_PNGDEC_GRAYSCALE_ALPHA:
	{
		stream->out_param.outputComponents = 2;
		break;
	}

	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE:
	{
		stream->out_param.outputComponents = 1;
	}
	}

	// Set the memory usage. We currently don't actually allocate memory for libpng through the callbacks, due to libpng needing a lot more memory compared to PS3 variant.
	stream->out_param.useMemorySpace = 0;

	// Set the pointer
	*out_param = stream->out_param;
	
	return CELL_OK;
}

s32 pngDecodeData(PPUThread& ppu, PHandle handle, PStream stream, vm::ptr<u8> data, PDataControlParam data_control_param, PDataOutInfo data_out_info, PCbControlDisp cb_control_disp = vm::null, PDispParam disp_param = vm::null)
{
	if (cb_control_disp || disp_param)
	{
		throw EXCEPTION("Partial image decoding is not supported");
	}

	// Indicate, that the PNG decoding is stopped/failed. This is incase, we return an error code in the middle of decoding
	data_out_info->status = CELL_PNGDEC_DEC_STATUS_STOP;

	// Possibilities for decoding in different sizes aren't tested, so if anyone finds any of these cases, we'll know about it.
	if (stream->info.imageWidth != stream->out_param.outputWidth)
	{
		throw EXCEPTION("Image width doesn't match output width! (%d/%d)", stream->out_param.outputWidth, stream->info.imageWidth);
	}

	if (stream->info.imageHeight != stream->out_param.outputHeight)
	{
		throw EXCEPTION("Image width doesn't match output height! (%d/%d)", stream->out_param.outputHeight, stream->info.imageHeight);
	}

	// Get the amount of output bytes per line
	const u32 bytes_per_line = data_control_param->outputBytesPerLine;

	// Whether to recaculate bytes per row
	bool recalculate_bytes_per_row = false;

	// Check if the game is expecting the number of bytes per line to be lower, than the actual bytes per line on the image. (Arkedo Pixel for example)
	// In such case we strip the bit depth to be lower.
	if ((bytes_per_line < stream->out_param.outputWidthByte) && stream->out_param.outputBitDepth != 8)
	{
		// Check if the packing is really 1 byte per 1 pixel
		if (stream->packing != CELL_PNGDEC_1BYTE_PER_1PIXEL)
		{
			throw EXCEPTION("Unexpected packing value! (%d)", stream->packing);
		}

		// Scale 16 bit depth down to 8 bit depth. PS3 uses png_set_strip_16, since png_set_scale_16 wasn't available back then.
		png_set_strip_16(stream->png_ptr);
		recalculate_bytes_per_row = true;
	}
	// Check if the outputWidthByte is smaller than the intended output length of a line. For example an image might be in RGB, but we need to output 4 components, so we need to perform alpha padding.
	else if (stream->out_param.outputWidthByte < (stream->out_param.outputWidth * stream->out_param.outputComponents))
	{
		// If fixed alpha is not specified in such a case, the default value for the alpha is 0xFF (255)
		if (!stream->fixed_alpha)
		{
			stream->fixed_alpha_colour = 0xFF;
		}

		// We need to fill alpha (before or after, depending on the output colour format) using the fixed alpha value passed by the game.
		png_set_add_alpha(stream->png_ptr, stream->fixed_alpha_colour, stream->out_param.outputColorSpace == CELL_PNGDEC_RGBA ? PNG_FILLER_AFTER : PNG_FILLER_BEFORE);
		recalculate_bytes_per_row = true;
	}
	// We decode as RGBA, so we need to swap the alpha
	else if (stream->out_param.outputColorSpace == CELL_PNGDEC_ARGB)
	{
		// Swap the alpha channel for the ARGB output format, if the padding isn't needed
		png_set_swap_alpha(stream->png_ptr);
	}
	// Sometimes games pass in a RBG/RGBA image and want it as grayscale
	else if ((stream->out_param.outputColorSpace == CELL_PNGDEC_GRAYSCALE_ALPHA || stream->out_param.outputColorSpace == CELL_PNGDEC_GRAYSCALE)
		  && (stream->info.colorSpace == CELL_PNGDEC_RGB || stream->info.colorSpace == CELL_PNGDEC_RGBA))
	{
		// Tell libpng to convert it to grayscale
		png_set_rgb_to_gray(stream->png_ptr, PNG_ERROR_ACTION_NONE, PNG_RGB_TO_GRAY_DEFAULT, PNG_RGB_TO_GRAY_DEFAULT);
		recalculate_bytes_per_row = true;
	}

	if (recalculate_bytes_per_row)
	{
		// Update the info structure
		png_read_update_info(stream->png_ptr, stream->info_ptr);

		// Recalculate the bytes per row
		stream->out_param.outputWidthByte = png_get_rowbytes(stream->png_ptr, stream->info_ptr);
	}

	// Calculate the image size
	u32 image_size = stream->out_param.outputWidthByte * stream->out_param.outputHeight;

	// Buffer for storing the image
	std::vector<u8> png(image_size);
	
	// Make an unique pointer for the row pointers
	std::vector<u8*> row_pointers(stream->out_param.outputHeight);

	// Allocate memory for rows
	for (u32 y = 0; y < stream->out_param.outputHeight; y++)
	{
		row_pointers[y] = &png[y * stream->out_param.outputWidthByte];
	}

	// Decode the image
	png_read_image(stream->png_ptr, row_pointers.data());

	// Check if the image needs to be flipped
	const bool flip = stream->out_param.outputMode == CELL_PNGDEC_BOTTOM_TO_TOP;

	// Copy the result to the output buffer
	switch (stream->out_param.outputColorSpace)
	{
	case CELL_PNGDEC_RGB:
	case CELL_PNGDEC_RGBA:
	case CELL_PNGDEC_ARGB:
	case CELL_PNGDEC_GRAYSCALE_ALPHA:
	{
		// Check if we need to flip the image or need to leave empty bytes at the end of a line
		if ((bytes_per_line > stream->out_param.outputWidthByte) || flip)
		{
			// Get how many bytes per line we need to output - bytesPerLine is total amount of bytes per line, rest is unused and the game can do as it pleases.
			const u32 line_size = std::min(bytes_per_line, stream->out_param.outputWidth * 4);

			// If the game wants more bytes per line to be output, than the image has, then we simply copy what we have for each line,
			// and continue on the next line, thus leaving empty bytes at the end of the line.
			for (u32 i = 0; i < stream->out_param.outputHeight; i++)
			{
				const u32 dst = i * bytes_per_line;
				const u32 src = stream->out_param.outputWidth * 4 * (flip ? stream->out_param.outputHeight - i - 1 : i);
				memcpy(&data[dst], &png[src], line_size);
			}
		}
		else
		{
			// We can simply copy the output to the data pointer specified by the game, since we already do alpha channel transformations in libpng, if needed
			memcpy(data.get_ptr(), png.data(), image_size);
		}
		break;
	}

	default: throw EXCEPTION("Unsupported color space (%d)", stream->out_param.outputColorSpace);
	}

	// Get the number of iTXt, tEXt and zTXt chunks
	s32 text_chunks = 0;
	png_get_text(stream->png_ptr, stream->info_ptr, nullptr, &text_chunks);

	// Set the chunk information and the previously obtained number of text chunks
	data_out_info->numText = (u32)text_chunks;
	data_out_info->chunkInformation = pngDecGetChunkInformation(stream, true);
	data_out_info->numUnknownChunk = 0; // TODO: Get this somehow. Does anything even use or need this?

	// Indicate that the decoding succeeded
	data_out_info->status = CELL_PNGDEC_DEC_STATUS_FINISH;

	return CELL_OK;
}

s32 cellPngDecCreate(PPUThread& ppu, PPHandle handle, PThreadInParam threadInParam, PThreadOutParam threadOutParam)
{
	cellPngDec.warning("cellPngDecCreate(handle=**0x%x, threadInParam=*0x%x, threadOutParam=*0x%x)", handle, threadInParam, threadOutParam);
	return pngDecCreate(ppu, handle, threadInParam, threadOutParam);
}

s32 cellPngDecExtCreate(PPUThread& ppu, PPHandle handle, PThreadInParam threadInParam, PThreadOutParam threadOutParam, PExtThreadInParam extThreadInParam, PExtThreadOutParam extThreadOutParam)
{
	cellPngDec.warning("cellPngDecCreate(mainHandle=**0x%x, threadInParam=*0x%x, threadOutParam=*0x%x, extThreadInParam=*0x%x, extThreadOutParam=*0x%x)", handle, threadInParam, threadOutParam, extThreadInParam, extThreadOutParam);
	return pngDecCreate(ppu, handle, threadInParam, threadOutParam, extThreadInParam, extThreadOutParam);
}

s32 cellPngDecDestroy(PPUThread& ppu, PHandle handle)
{
	cellPngDec.warning("cellPngDecDestroy(mainHandle=*0x%x)", handle);
	return pngDecDestroy(ppu, handle);
}

s32 cellPngDecOpen(PPUThread& ppu, PHandle handle, PPStream stream, PSrc src, POpenInfo openInfo)
{
	cellPngDec.warning("cellPngDecOpen(handle=*0x%x, stream=**0x%x, src=*0x%x, openInfo=*0x%x)", handle, stream, src, openInfo);
	return pngDecOpen(ppu, handle, stream, src, openInfo);
}

s32 cellPngDecExtOpen(PPUThread& ppu, PHandle handle, PPStream stream, PSrc src, POpenInfo openInfo, PCbControlStream cbCtrlStrm, POpenParam opnParam)
{
	cellPngDec.warning("cellPngDecExtOpen(handle=*0x%x, stream=**0x%x, src=*0x%x, openInfo=*0x%x, cbCtrlStrm=*0x%x, opnParam=*0x%x)", handle, stream, src, openInfo, cbCtrlStrm, opnParam);
	return pngDecOpen(ppu, handle, stream, src, openInfo, cbCtrlStrm, opnParam);
}

s32 cellPngDecClose(PPUThread& ppu, PHandle handle, PStream stream)
{
	cellPngDec.warning("cellPngDecClose(handle=*0x%x, stream=*0x%x)", handle, stream);
	return pngDecClose(ppu, handle, stream);
}

s32 cellPngDecReadHeader(PHandle handle, PStream stream, PInfo info)
{
	cellPngDec.warning("cellPngDecReadHeader(handle=*0x%x, stream=*0x%x, info=*0x%x)", handle, stream, info);
	return pngReadHeader(stream, info);
}

s32 cellPngDecExtReadHeader(PHandle handle, PStream stream, PInfo info, PExtInfo extInfo)
{
	cellPngDec.warning("cellPngDecExtReadHeader(handle=*0x%x, stream=*0x%x, info=*0x%x, extInfo=*0x%x)", handle, stream, info, extInfo);
	return pngReadHeader(stream, info, extInfo);
}

s32 cellPngDecSetParameter(PHandle handle, PStream stream, PInParam inParam, POutParam outParam)
{
	cellPngDec.warning("cellPngDecSetParameter(handle=*0x%x, stream=*0x%x, inParam=*0x%x, outParam=*0x%x)", handle, stream, inParam, outParam);
	return pngDecSetParameter(stream, inParam, outParam);
}

s32 cellPngDecExtSetParameter(PHandle handle, PStream stream, PInParam inParam, POutParam outParam, PExtInParam extInParam, PExtOutParam extOutParam)
{
	cellPngDec.warning("cellPngDecExtSetParameter(handle=*0x%x, stream=*0x%x, inParam=*0x%x, outParam=*0x%x, extInParam=*0x%x, extOutParam=*0x%x", handle, stream, inParam, outParam, extInParam, extOutParam);
	return pngDecSetParameter(stream, inParam, outParam, extInParam, extOutParam);
}

s32 cellPngDecDecodeData(PPUThread& ppu, PHandle handle, PStream stream, vm::ptr<u8> data, PDataControlParam dataCtrlParam, PDataOutInfo dataOutInfo)
{
	cellPngDec.warning("cellPngDecDecodeData(handle=*0x%x, stream=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x)", handle, stream, data, dataCtrlParam, dataOutInfo);
	return pngDecodeData(ppu, handle, stream, data, dataCtrlParam, dataOutInfo);
}

s32 cellPngDecExtDecodeData(PPUThread& ppu, PHandle handle, PStream stream, vm::ptr<u8> data, PDataControlParam dataCtrlParam, PDataOutInfo dataOutInfo, PCbControlDisp cbCtrlDisp, PDispParam dispParam)
{
	cellPngDec.warning("cellPngDecExtDecodeData(handle=*0x%x, stream=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x, cbCtrlDisp=*0x%x, dispParam=*0x%x)", handle, stream, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);
	return pngDecodeData(ppu, handle, stream, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);
}

s32 cellPngDecGetUnknownChunks(PHandle handle, PStream stream, vm::pptr<CellPngUnknownChunk> unknownChunk, vm::ptr<u32> unknownChunkNumber)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetpCAL(PHandle handle, PStream stream, vm::ptr<CellPngPCAL> pcal)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetcHRM(PHandle handle, PStream stream, vm::ptr<CellPngCHRM> chrm)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetsCAL(PHandle handle, PStream stream, vm::ptr<CellPngSCAL> scal)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetpHYs(PHandle handle, PStream stream, vm::ptr<CellPngPHYS> phys)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetoFFs(PHandle handle, PStream stream, vm::ptr<CellPngOFFS> offs)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetsPLT(PHandle handle, PStream stream, vm::ptr<CellPngSPLT> splt)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetbKGD(PHandle handle, PStream stream, vm::ptr<CellPngBKGD> bkgd)
{
	throw EXCEPTION("");
}

s32 cellPngDecGettIME(PHandle handle, PStream stream, vm::ptr<CellPngTIME> time)
{
	throw EXCEPTION("");
}

s32 cellPngDecGethIST(PHandle handle, PStream stream, vm::ptr<CellPngHIST> hist)
{
	throw EXCEPTION("");
}

s32 cellPngDecGettRNS(PHandle handle, PStream stream, vm::ptr<CellPngTRNS> trns)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetsBIT(PHandle handle, PStream stream, vm::ptr<CellPngSBIT> sbit)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetiCCP(PHandle handle, PStream stream, vm::ptr<CellPngICCP> iccp)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetsRGB(PHandle handle, PStream stream, vm::ptr<CellPngSRGB> srgb)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetgAMA(PHandle handle, PStream stream, vm::ptr<CellPngGAMA> gama)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetPLTE(PHandle handle, PStream stream, vm::ptr<CellPngPLTE> plte)
{
	throw EXCEPTION("");
}

s32 cellPngDecGetTextChunk(PHandle handle, PStream stream, vm::ptr<u32> textInfoNum, vm::pptr<CellPngTextInfo> textInfo)
{
	throw EXCEPTION("");
}

DECLARE(ppu_module_manager::cellPngDec)("cellPngDec", []()
{
	REG_FUNC(cellPngDec, cellPngDecGetUnknownChunks);
	REG_FUNC(cellPngDec, cellPngDecClose);
	REG_FUNC(cellPngDec, cellPngDecGetpCAL);
	REG_FUNC(cellPngDec, cellPngDecGetcHRM);
	REG_FUNC(cellPngDec, cellPngDecGetsCAL);
	REG_FUNC(cellPngDec, cellPngDecGetpHYs);
	REG_FUNC(cellPngDec, cellPngDecGetoFFs);
	REG_FUNC(cellPngDec, cellPngDecGetsPLT);
	REG_FUNC(cellPngDec, cellPngDecGetbKGD);
	REG_FUNC(cellPngDec, cellPngDecGettIME);
	REG_FUNC(cellPngDec, cellPngDecGethIST);
	REG_FUNC(cellPngDec, cellPngDecGettRNS);
	REG_FUNC(cellPngDec, cellPngDecGetsBIT);
	REG_FUNC(cellPngDec, cellPngDecGetiCCP);
	REG_FUNC(cellPngDec, cellPngDecGetsRGB);
	REG_FUNC(cellPngDec, cellPngDecGetgAMA);
	REG_FUNC(cellPngDec, cellPngDecGetPLTE);
	REG_FUNC(cellPngDec, cellPngDecGetTextChunk);
	REG_FUNC(cellPngDec, cellPngDecDestroy);
	REG_FUNC(cellPngDec, cellPngDecCreate);
	REG_FUNC(cellPngDec, cellPngDecExtCreate);
	REG_FUNC(cellPngDec, cellPngDecExtSetParameter);
	REG_FUNC(cellPngDec, cellPngDecSetParameter);
	REG_FUNC(cellPngDec, cellPngDecExtReadHeader);
	REG_FUNC(cellPngDec, cellPngDecReadHeader);
	REG_FUNC(cellPngDec, cellPngDecExtOpen);
	REG_FUNC(cellPngDec, cellPngDecOpen);
	REG_FUNC(cellPngDec, cellPngDecExtDecodeData);
	REG_FUNC(cellPngDec, cellPngDecDecodeData);
});
