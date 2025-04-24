#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_fs.h"
#include "png.h"
#include "cellPng.h"
#include "cellPngDec.h"

#if PNG_LIBPNG_VER_MAJOR >= 1 && (PNG_LIBPNG_VER_MINOR < 5 \
|| (PNG_LIBPNG_VER_MINOR == 5 && PNG_LIBPNG_VER_RELEASE < 7))
#define PNG_ERROR_ACTION_NONE 1
#define PNG_RGB_TO_GRAY_DEFAULT (-1)
#endif

#if PNG_LIBPNG_VER_MAJOR >= 1 && PNG_LIBPNG_VER_MINOR >= 5
typedef png_bytep iCCP_profile_type;
#else
typedef png_charp iCCP_profile_type;
#endif

LOG_CHANNEL(cellPngDec);

template <>
void fmt_class_string<CellPngDecError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellPngDecError value)
	{
		switch (value)
		{
		STR_CASE(CELL_PNGDEC_ERROR_HEADER);
		STR_CASE(CELL_PNGDEC_ERROR_STREAM_FORMAT);
		STR_CASE(CELL_PNGDEC_ERROR_ARG);
		STR_CASE(CELL_PNGDEC_ERROR_SEQ);
		STR_CASE(CELL_PNGDEC_ERROR_BUSY);
		STR_CASE(CELL_PNGDEC_ERROR_FATAL);
		STR_CASE(CELL_PNGDEC_ERROR_OPEN_FILE);
		STR_CASE(CELL_PNGDEC_ERROR_SPU_UNSUPPORT);
		STR_CASE(CELL_PNGDEC_ERROR_SPU_ERROR);
		STR_CASE(CELL_PNGDEC_ERROR_CB_PARAM);
		}

		return unknown;
	});
}

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
	PngBuffer& buffer = *static_cast<PngBuffer*>(io_ptr);

	// Read froma  file or a buffer
	if (buffer.file)
	{
		// Get the file
		auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(buffer.fd);

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

void pngDecRowCallback(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
{
	PngStream* stream = static_cast<PngStream*>(png_get_progressive_ptr(png_ptr));
	if (!stream)
	{
		cellPngDec.error("Failed to obtain streamPtr in rowCallback.");
		return;
	}

	// we have to check this everytime as this func can be called multiple times per row, and/or only once per row
	if (stream->nextRow + stream->outputCounts == row_num)
		stream->nextRow = row_num;

	if (stream->ppuContext && (stream->nextRow == row_num  || pass > 0))
	{
		if (pass > 0 )
		{
			stream->cbDispInfo->scanPassCount = pass;
			stream->cbDispInfo->nextOutputStartY = row_num;
		}
		else {
			stream->cbDispInfo->scanPassCount = 0;
			stream->cbDispInfo->nextOutputStartY = 0;
		}

		stream->cbDispInfo->outputImage = stream->cbDispParam->nextOutputImage;
		stream->cbCtrlDisp.cbCtrlDispFunc(*stream->ppuContext, stream->cbDispInfo, stream->cbDispParam, stream->cbCtrlDisp.cbCtrlDispArg);
		stream->cbDispInfo->outputStartY = row_num;
	}
	u8* data;
	if (pass > 0)
		data = static_cast<u8*>(stream->cbDispParam->nextOutputImage.get_ptr());
	else
		data = static_cast<u8*>(stream->cbDispParam->nextOutputImage.get_ptr()) + ((row_num - stream->cbDispInfo->outputStartY) * stream->cbDispInfo->outputFrameWidthByte);

	png_progressive_combine_row(png_ptr, data, new_row);
}

void pngDecInfoCallback(png_structp png_ptr, png_infop /*info*/)
{
	PngStream* stream = static_cast<PngStream*>(png_get_progressive_ptr(png_ptr));
	if (!stream)
	{
		cellPngDec.error("Failed to obtain streamPtr in rowCallback.");
		return;
	}

	const usz remaining = png_process_data_pause(png_ptr, false);
	stream->buffer->cursor += (stream->buffer->length - remaining);
}

void pngDecEndCallback(png_structp png_ptr, png_infop /*info*/)
{
	PngStream* stream = static_cast<PngStream*>(png_get_progressive_ptr(png_ptr));
	if (!stream)
	{
		cellPngDec.error("Failed to obtain streamPtr in endCallback.");
		return;
	}

	stream->endOfFile = true;
}

// Custom error handler for libpng
[[noreturn]] void pngDecError(png_structp /*png_ptr*/, png_const_charp error_message)
{
	cellPngDec.error("pngDecError: %s", error_message);
	// we can't return here or libpng blows up
	fmt::throw_exception("Fatal Error in libpng: %s", error_message);
}

// Custom warning handler for libpng
void pngDecWarning(png_structp /*png_ptr*/, png_const_charp error_message)
{
	cellPngDec.warning("pngDecWarning: %s", error_message);
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
be_t<u32> pngDecGetChunkInformation(PngStream* stream, bool IDAT = false)
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
	png_uint_32 proflen;
	iCCP_profile_type profile;
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

error_code pngDecCreate(ppu_thread& ppu, PPHandle png_handle, PThreadInParam thread_in_param, PThreadOutParam thread_out_param, PExtThreadInParam /*extra_thread_in_param*/ = vm::null, PExtThreadOutParam extra_thread_out_param = vm::null)
{
	// Check if partial image decoding is used
	if (extra_thread_out_param)
	{
		fmt::throw_exception("Partial image decoding is not supported.");
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
	handle->malloc_ = thread_in_param->cbCtrlMallocFunc;
	handle->malloc_arg = thread_in_param->cbCtrlMallocArg;
	handle->free_ = thread_in_param->cbCtrlFreeFunc;
	handle->free_arg = thread_in_param->cbCtrlFreeArg;

	// Set handle pointer
	*png_handle = handle;

	// Set the version information
	thread_out_param->pngCodecVersion = PNGDEC_CODEC_VERSION;

	return CELL_OK;
}

error_code pngDecDestroy(ppu_thread& ppu, PHandle handle)
{
	// Deallocate the decoder handle memory
	if (handle->free_(ppu, handle, handle->free_arg) != 0)
	{
		cellPngDec.error("PNG decoder deallocation failed.");
		return CELL_PNGDEC_ERROR_FATAL;
	}

	return CELL_OK;
}

error_code pngDecOpen(ppu_thread& ppu, PHandle handle, PPStream png_stream, PSrc source, POpenInfo open_info, PCbControlStream control_stream = vm::null, POpenParam open_param = vm::null)
{
	// partial decoding only supported with buffer type
	if (source->srcSelect != CELL_PNGDEC_BUFFER && control_stream)
	{
		cellPngDec.error("Attempted partial image decode with file.");
		return CELL_PNGDEC_ERROR_STREAM_FORMAT;
	}

	// Allocate memory for the stream structure
	auto stream = vm::ptr<PngStream>::make(handle->malloc_(ppu, sizeof(PngStream), handle->malloc_arg).addr());

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

	// Use virtual memory address as a handle
	*png_stream = stream;

	// Allocate memory for the PNG buffer for decoding
	auto buffer = vm::ptr<PngBuffer>::make(handle->malloc_(ppu, sizeof(PngBuffer), handle->malloc_arg).addr());

	// Check for if the buffer structure allocation failed
	if (!buffer)
	{
		fmt::throw_exception("Memory allocation for the PNG buffer structure failed.");
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
		fmt::throw_exception("Non-0 file offset not supported.");
	}

	// Depending on the source type, get the first 8 bytes
	if (source->srcSelect == CELL_PNGDEC_FILE)
	{
		const auto real_path = vfs::get(stream->source.fileName.get_ptr());

		// Open a file stream
		fs::file file_stream(real_path);

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
		buffer->fd = idm::make<lv2_fs_object, lv2_file>(stream->source.fileName.get_ptr(), std::move(file_stream), 0, 0, real_path);

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
		fmt::throw_exception("Creation of png_infop failed.");
	}

	// We must indicate, that we allocated more memory
	open_info->initSpaceAllocated += u32{sizeof(PngBuffer)};

	if (source->srcSelect == CELL_PNGDEC_BUFFER)
	{
		buffer->length = stream->source.streamSize;
		buffer->data = stream->source.streamPtr;
		buffer->cursor = 8;
	}

	// Set the custom read function for decoding
	if (control_stream)
	{
		if (open_param && open_param->selectChunk != 0u)
			fmt::throw_exception("Partial Decoding with selectChunk not supported yet.");

		stream->cbCtrlStream.cbCtrlStrmArg = control_stream->cbCtrlStrmArg;
		stream->cbCtrlStream.cbCtrlStrmFunc = control_stream->cbCtrlStrmFunc;

		png_set_progressive_read_fn(stream->png_ptr, stream.get_ptr(), pngDecInfoCallback, pngDecRowCallback, pngDecEndCallback);

		// push header tag to libpng to keep us in sync
		png_process_data(stream->png_ptr, stream->info_ptr, header, 8);
	}
	else
	{
		png_set_read_fn(stream->png_ptr, buffer.get_ptr(), pngDecReadBuffer);

		// We need to tell libpng, that we already read 8 bytes
		png_set_sig_bytes(stream->png_ptr, 8);
	}

	return CELL_OK;
}

error_code pngDecClose(ppu_thread& ppu, PHandle handle, PStream stream)
{
	// Remove the file descriptor, if a file descriptor was used for decoding
	if (stream->buffer->file)
	{
		idm::remove<lv2_fs_object, lv2_file>(stream->buffer->fd);
	}

	// Deallocate the PNG buffer structure used to decode from memory, if we decoded from memory
	if (stream->buffer)
	{
		if (handle->free_(ppu, stream->buffer, handle->free_arg) != 0)
		{
			cellPngDec.error("PNG buffer decoding structure deallocation failed.");
			return CELL_PNGDEC_ERROR_FATAL;
		}
	}

	// Free the memory allocated by libpng
	png_destroy_read_struct(&stream->png_ptr, &stream->info_ptr, nullptr);

	// Deallocate the stream memory
	if (handle->free_(ppu, stream, handle->free_arg) != 0)
	{
		cellPngDec.error("PNG stream deallocation failed.");
		return CELL_PNGDEC_ERROR_FATAL;
	}

	return CELL_OK;
}

void pngSetHeader(PngStream* stream)
{
	stream->info.imageWidth = png_get_image_width(stream->png_ptr, stream->info_ptr);
	stream->info.imageHeight = png_get_image_height(stream->png_ptr, stream->info_ptr);
	stream->info.numComponents = png_get_channels(stream->png_ptr, stream->info_ptr);
	stream->info.colorSpace = getPngDecColourType(png_get_color_type(stream->png_ptr, stream->info_ptr));
	stream->info.bitDepth = png_get_bit_depth(stream->png_ptr, stream->info_ptr);
	stream->info.interlaceMethod = png_get_interlace_type(stream->png_ptr, stream->info_ptr);
	stream->info.chunkInformation = pngDecGetChunkInformation(stream);
}

error_code pngDecSetParameter(PStream stream, PInParam in_param, POutParam out_param, PExtInParam extra_in_param = vm::null, PExtOutParam extra_out_param = vm::null)
{
	if (in_param->outputPackFlag == CELL_PNGDEC_1BYTE_PER_NPIXEL)
	{
		fmt::throw_exception("Packing not supported! (%d)", in_param->outputPackFlag);
	}

	// flag to keep unknown chunks
	png_set_keep_unknown_chunks(stream->png_ptr, PNG_HANDLE_CHUNK_IF_SAFE, nullptr, 0);

	// Scale 16 bit depth down to 8 bit depth.
	if (stream->info.bitDepth == 16u && in_param->outputBitDepth == 8u)
	{
		// PS3 uses png_set_strip_16, since png_set_scale_16 wasn't available back then.
		png_set_strip_16(stream->png_ptr);
	}

	// This shouldnt ever happen, but not sure what to do if it does, just want it logged for now
	if (stream->info.bitDepth != 16u && in_param->outputBitDepth == 16u)
		cellPngDec.error("Output depth of 16 with non input depth of 16 specified!");
	if (in_param->commandPtr)
		cellPngDec.warning("Ignoring CommandPtr.");

	if (stream->info.colorSpace != in_param->outputColorSpace)
	{
		// check if we need to set alpha
		const bool inputHasAlpha = cellPngColorSpaceHasAlpha(stream->info.colorSpace);
		const bool outputWantsAlpha = cellPngColorSpaceHasAlpha(in_param->outputColorSpace);

		if (outputWantsAlpha && !inputHasAlpha)
		{
			if (in_param->outputAlphaSelect == CELL_PNGDEC_FIX_ALPHA)
				png_set_add_alpha(stream->png_ptr, in_param->outputColorAlpha, in_param->outputColorSpace == CELL_PNGDEC_ARGB ? PNG_FILLER_BEFORE : PNG_FILLER_AFTER);
			else
			{
				// Check if we can steal the alpha from a trns block
				if (png_get_valid(stream->png_ptr, stream->info_ptr, PNG_INFO_tRNS))
					png_set_tRNS_to_alpha(stream->png_ptr);
				// if not, just set default of 0xff
				else
					png_set_add_alpha(stream->png_ptr, 0xff, in_param->outputColorSpace == CELL_PNGDEC_ARGB ? PNG_FILLER_BEFORE : PNG_FILLER_AFTER);
			}
		}
		else if (inputHasAlpha && !outputWantsAlpha)
			png_set_strip_alpha(stream->png_ptr);
		else if (in_param->outputColorSpace == CELL_PNGDEC_ARGB && stream->info.colorSpace == CELL_PNGDEC_RGBA)
			png_set_swap_alpha(stream->png_ptr);

		// Handle gray<->rgb colorspace conversions
		// rgb output
		if (in_param->outputColorSpace == CELL_PNGDEC_ARGB
			|| in_param->outputColorSpace == CELL_PNGDEC_RGBA
			|| in_param->outputColorSpace == CELL_PNGDEC_RGB)
		{

			if (stream->info.colorSpace == CELL_PNGDEC_PALETTE)
				png_set_palette_to_rgb(stream->png_ptr);
			if ((stream->info.colorSpace == CELL_PNGDEC_GRAYSCALE || stream->info.colorSpace == CELL_PNGDEC_GRAYSCALE_ALPHA)
				&& stream->info.bitDepth < 8)
				png_set_expand_gray_1_2_4_to_8(stream->png_ptr);
		}
		// grayscale output
		else
		{
			if (stream->info.colorSpace == CELL_PNGDEC_ARGB
				|| stream->info.colorSpace == CELL_PNGDEC_RGBA
				|| stream->info.colorSpace == CELL_PNGDEC_RGB)
			{

				png_set_rgb_to_gray(stream->png_ptr, PNG_ERROR_ACTION_NONE, PNG_RGB_TO_GRAY_DEFAULT, PNG_RGB_TO_GRAY_DEFAULT);
			}
			else {
				// not sure what to do here
				cellPngDec.error("Grayscale / Palette to Grayscale / Palette conversion currently unsupported.");
			}
		}
	}

	stream->passes = png_set_interlace_handling(stream->png_ptr);

	// Update the info structure
	png_read_update_info(stream->png_ptr, stream->info_ptr);

	stream->out_param.outputWidth = stream->info.imageWidth;
	stream->out_param.outputHeight = stream->info.imageHeight;
	stream->out_param.outputBitDepth = in_param->outputBitDepth;
	stream->out_param.outputColorSpace = in_param->outputColorSpace;
	stream->out_param.outputMode = in_param->outputMode;

	stream->out_param.outputWidthByte = png_get_rowbytes(stream->png_ptr, stream->info_ptr);
	stream->out_param.outputComponents = png_get_channels(stream->png_ptr, stream->info_ptr);

	stream->packing = in_param->outputPackFlag;

	// Set the memory usage. We currently don't actually allocate memory for libpng through the callbacks, due to libpng needing a lot more memory compared to PS3 variant.
	stream->out_param.useMemorySpace = 0;

	if (extra_in_param)
	{
		if (extra_in_param->bufferMode != CELL_PNGDEC_LINE_MODE)
		{
			cellPngDec.error("Invalid Buffermode specified.");
			return CELL_PNGDEC_ERROR_ARG;
		}

		if (stream->passes > 1)
		{
			stream->outputCounts = 1;
		}
		else
			stream->outputCounts = extra_in_param->outputCounts;

		if (extra_out_param)
		{
			if (stream->outputCounts == 0)
				extra_out_param->outputHeight = stream->out_param.outputHeight;
			else
				extra_out_param->outputHeight = std::min(stream->outputCounts, stream->out_param.outputHeight.value());
			extra_out_param->outputWidthByte = stream->out_param.outputWidthByte;
		}
	}

	*out_param = stream->out_param;

	return CELL_OK;
}

error_code pngDecodeData(ppu_thread& ppu, PHandle handle, PStream stream, vm::ptr<u8> data, PDataControlParam data_control_param, PDataOutInfo data_out_info, PCbControlDisp cb_control_disp = vm::null, PDispParam disp_param = vm::null)
{
	// Indicate, that the PNG decoding is stopped/failed. This is incase, we return an error code in the middle of decoding
	data_out_info->status = CELL_PNGDEC_DEC_STATUS_STOP;

	const u32 bytes_per_line = ::narrow<u32>(data_control_param->outputBytesPerLine);

	// Log this for now
	if (bytes_per_line < stream->out_param.outputWidthByte)
	{
		fmt::throw_exception("Bytes per line less than expected output! Got: %d, expected: %d", bytes_per_line, stream->out_param.outputWidthByte);
	}

	// partial decoding
	if (cb_control_disp && stream->outputCounts > 0)
	{
		// get data from cb
		auto streamInfo = vm::ptr<CellPngDecStrmInfo>::make(handle->malloc_(ppu, sizeof(CellPngDecStrmInfo), handle->malloc_arg).addr());
		auto streamParam = vm::ptr<CellPngDecStrmParam>::make(handle->malloc_(ppu, sizeof(CellPngDecStrmParam), handle->malloc_arg).addr());
		stream->cbDispInfo = vm::ptr<CellPngDecDispInfo>::make(handle->malloc_(ppu, sizeof(CellPngDecDispInfo), handle->malloc_arg).addr());
		stream->cbDispParam = vm::ptr<CellPngDecDispParam>::make(handle->malloc_(ppu, sizeof(CellPngDecDispParam), handle->malloc_arg).addr());

		auto freeMem = [&]()
		{
			handle->free_(ppu, streamInfo, handle->free_arg);
			handle->free_(ppu, streamParam, handle->free_arg);
			handle->free_(ppu, stream->cbDispInfo, handle->free_arg);
			handle->free_(ppu, stream->cbDispParam, handle->free_arg);
		};

		// set things that won't change between callbacks
		stream->cbDispInfo->outputFrameWidthByte = bytes_per_line;
		stream->cbDispInfo->outputFrameHeight = stream->out_param.outputHeight;
		stream->cbDispInfo->outputWidthByte = stream->out_param.outputWidthByte;
		stream->cbDispInfo->outputBitDepth = stream->out_param.outputBitDepth;
		stream->cbDispInfo->outputComponents = stream->out_param.outputComponents;
		stream->cbDispInfo->outputHeight = stream->outputCounts;
		stream->cbDispInfo->outputStartXByte = 0;
		stream->cbDispInfo->outputStartY = 0;
		stream->cbDispInfo->scanPassCount = 0;
		stream->cbDispInfo->nextOutputStartY = 0;

		stream->ppuContext = &ppu;
		stream->nextRow = stream->cbDispInfo->outputHeight;
		stream->cbCtrlDisp.cbCtrlDispArg = cb_control_disp->cbCtrlDispArg;
		stream->cbCtrlDisp.cbCtrlDispFunc = cb_control_disp->cbCtrlDispFunc;

		stream->cbDispParam->nextOutputImage = disp_param->nextOutputImage;

		streamInfo->decodedStrmSize = ::narrow<u32>(stream->buffer->cursor);
		// push the rest of the buffer we have
		if (stream->buffer->length > stream->buffer->cursor)
		{
			u8* data = static_cast<u8*>(stream->buffer->data.get_ptr()) + stream->buffer->cursor;
			png_process_data(stream->png_ptr, stream->info_ptr, data, stream->buffer->length - stream->buffer->cursor);
			streamInfo->decodedStrmSize = ::narrow<u32>(stream->buffer->length);
		}

		// todo: commandPtr
		// then just loop until the end, the callbacks should take care of the rest
		while (stream->endOfFile != true)
		{
			stream->cbCtrlStream.cbCtrlStrmFunc(ppu, streamInfo, streamParam, stream->cbCtrlStream.cbCtrlStrmArg);
			streamInfo->decodedStrmSize += streamParam->strmSize;
			png_process_data(stream->png_ptr, stream->info_ptr, static_cast<u8*>(streamParam->strmPtr.get_ptr()), streamParam->strmSize);
		}

		freeMem();
	}
	else
	{
		// Check if the image needs to be flipped
		const bool flip = stream->out_param.outputMode == CELL_PNGDEC_BOTTOM_TO_TOP;

		// Decode the image
		// todo: commandptr
		{
			for (u32 j = 0; j < stream->passes; j++)
			{
				for (u32 i = 0; i < stream->out_param.outputHeight; ++i)
				{
					const u32 line = flip ? stream->out_param.outputHeight - i - 1 : i;
					png_read_row(stream->png_ptr, &data[line*bytes_per_line], nullptr);
				}
			}
			png_read_end(stream->png_ptr, stream->info_ptr);
		}
	}

	// Get the number of iTXt, tEXt and zTXt chunks
	const s32 text_chunks = png_get_text(stream->png_ptr, stream->info_ptr, nullptr, nullptr);

	// Set the chunk information and the previously obtained number of text chunks
	data_out_info->numText = static_cast<u32>(text_chunks);
	data_out_info->chunkInformation = pngDecGetChunkInformation(stream.get_ptr(), true);
	png_unknown_chunkp unknowns;
	const int num_unknowns = png_get_unknown_chunks(stream->png_ptr, stream->info_ptr, &unknowns);
	data_out_info->numUnknownChunk = num_unknowns;

	// Indicate that the decoding succeeded
	data_out_info->status = CELL_PNGDEC_DEC_STATUS_FINISH;

	return CELL_OK;
}

error_code cellPngDecCreate(ppu_thread& ppu, PPHandle handle, PThreadInParam threadInParam, PThreadOutParam threadOutParam)
{
	cellPngDec.warning("cellPngDecCreate(handle=**0x%x, threadInParam=*0x%x, threadOutParam=*0x%x)", handle, threadInParam, threadOutParam);
	return pngDecCreate(ppu, handle, threadInParam, threadOutParam);
}

error_code cellPngDecExtCreate(ppu_thread& ppu, PPHandle handle, PThreadInParam threadInParam, PThreadOutParam threadOutParam, PExtThreadInParam extThreadInParam, PExtThreadOutParam extThreadOutParam)
{
	cellPngDec.warning("cellPngDecExtCreate(mainHandle=**0x%x, threadInParam=*0x%x, threadOutParam=*0x%x, extThreadInParam=*0x%x, extThreadOutParam=*0x%x)", handle, threadInParam, threadOutParam, extThreadInParam, extThreadOutParam);
	return pngDecCreate(ppu, handle, threadInParam, threadOutParam, extThreadInParam, extThreadOutParam);
}

error_code cellPngDecDestroy(ppu_thread& ppu, PHandle handle)
{
	cellPngDec.warning("cellPngDecDestroy(mainHandle=*0x%x)", handle);
	return pngDecDestroy(ppu, handle);
}

error_code cellPngDecOpen(ppu_thread& ppu, PHandle handle, PPStream stream, PSrc src, POpenInfo openInfo)
{
	cellPngDec.warning("cellPngDecOpen(handle=*0x%x, stream=**0x%x, src=*0x%x, openInfo=*0x%x)", handle, stream, src, openInfo);
	return pngDecOpen(ppu, handle, stream, src, openInfo);
}

error_code cellPngDecExtOpen(ppu_thread& ppu, PHandle handle, PPStream stream, PSrc src, POpenInfo openInfo, PCbControlStream cbCtrlStrm, POpenParam opnParam)
{
	cellPngDec.warning("cellPngDecExtOpen(handle=*0x%x, stream=**0x%x, src=*0x%x, openInfo=*0x%x, cbCtrlStrm=*0x%x, opnParam=*0x%x)", handle, stream, src, openInfo, cbCtrlStrm, opnParam);
	return pngDecOpen(ppu, handle, stream, src, openInfo, cbCtrlStrm, opnParam);
}

error_code cellPngDecClose(ppu_thread& ppu, PHandle handle, PStream stream)
{
	cellPngDec.warning("cellPngDecClose(handle=*0x%x, stream=*0x%x)", handle, stream);
	return pngDecClose(ppu, handle, stream);
}

error_code cellPngDecReadHeader(PHandle handle, PStream stream, PInfo info)
{
	cellPngDec.warning("cellPngDecReadHeader(handle=*0x%x, stream=*0x%x, info=*0x%x)", handle, stream, info);

	// Read the header info
	png_read_info(stream->png_ptr, stream->info_ptr);

	pngSetHeader(stream.get_ptr());

	// Set the pointer to stream info
	*info = stream->info;
	return CELL_OK;
}

error_code cellPngDecExtReadHeader(PHandle handle, PStream stream, PInfo info, PExtInfo extInfo)
{
	cellPngDec.warning("cellPngDecExtReadHeader(handle=*0x%x, stream=*0x%x, info=*0x%x, extInfo=*0x%x)", handle, stream, info, extInfo);
	// Set the reserved value to 0, if passed to the function. (Should this be arg error if they dont pass?)
	if (extInfo)
	{
		extInfo->reserved = 0;
	}

	// lets push what we have so far
	u8* data = static_cast<u8*>(stream->buffer->data.get_ptr()) + stream->buffer->cursor;
	png_process_data(stream->png_ptr, stream->info_ptr, data, stream->buffer->length);

	// lets hope we pushed enough for callback
	pngSetHeader(stream.get_ptr());

	// png doesnt allow empty image, so quick check for 0 verifys if we got the header
	// not sure exactly what should happen if we dont have header, ask for more data with callback?
	if (stream->info.imageWidth == 0u)
	{
		fmt::throw_exception("Invalid or not enough data sent to get header");
		return CELL_PNGDEC_ERROR_HEADER;
	}

	// Set the pointer to stream info
	*info = stream->info;
	return CELL_OK;
}

error_code cellPngDecSetParameter(PHandle handle, PStream stream, PInParam inParam, POutParam outParam)
{
	cellPngDec.warning("cellPngDecSetParameter(handle=*0x%x, stream=*0x%x, inParam=*0x%x, outParam=*0x%x)", handle, stream, inParam, outParam);
	return pngDecSetParameter(stream, inParam, outParam);
}

error_code cellPngDecExtSetParameter(PHandle handle, PStream stream, PInParam inParam, POutParam outParam, PExtInParam extInParam, PExtOutParam extOutParam)
{
	cellPngDec.warning("cellPngDecExtSetParameter(handle=*0x%x, stream=*0x%x, inParam=*0x%x, outParam=*0x%x, extInParam=*0x%x, extOutParam=*0x%x", handle, stream, inParam, outParam, extInParam, extOutParam);
	return pngDecSetParameter(stream, inParam, outParam, extInParam, extOutParam);
}

error_code cellPngDecDecodeData(ppu_thread& ppu, PHandle handle, PStream stream, vm::ptr<u8> data, PDataControlParam dataCtrlParam, PDataOutInfo dataOutInfo)
{
	cellPngDec.warning("cellPngDecDecodeData(handle=*0x%x, stream=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x)", handle, stream, data, dataCtrlParam, dataOutInfo);
	return pngDecodeData(ppu, handle, stream, data, dataCtrlParam, dataOutInfo);
}

error_code cellPngDecExtDecodeData(ppu_thread& ppu, PHandle handle, PStream stream, vm::ptr<u8> data, PDataControlParam dataCtrlParam, PDataOutInfo dataOutInfo, PCbControlDisp cbCtrlDisp, PDispParam dispParam)
{
	cellPngDec.warning("cellPngDecExtDecodeData(handle=*0x%x, stream=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x, cbCtrlDisp=*0x%x, dispParam=*0x%x)", handle, stream, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);
	return pngDecodeData(ppu, handle, stream, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);
}

error_code cellPngDecGetUnknownChunks(PHandle handle, PStream stream, vm::pptr<CellPngUnknownChunk> unknownChunk, vm::ptr<u32> unknownChunkNumber)
{
	cellPngDec.todo("cellPngDecGetUnknownChunks(handle=*0x%x, stream=*0x%x, unknownChunk=*0x%x, unknownChunkNumber=*0x%x)", handle, stream, unknownChunk, unknownChunkNumber);
	return CELL_OK;
}

error_code cellPngDecGetpCAL(PHandle handle, PStream stream, vm::ptr<CellPngPCAL> pcal)
{
	cellPngDec.todo("cellPngDecGetpCAL(handle=*0x%x, stream=*0x%x, pcal=*0x%x)", handle, stream, pcal);
	return CELL_OK;
}

error_code cellPngDecGetcHRM(PHandle handle, PStream stream, vm::ptr<CellPngCHRM> chrm)
{
	cellPngDec.todo("cellPngDecGetcHRM(handle=*0x%x, stream=*0x%x, chrm=*0x%x)", handle, stream, chrm);
	return CELL_OK;
}

error_code cellPngDecGetsCAL(PHandle handle, PStream stream, vm::ptr<CellPngSCAL> scal)
{
	cellPngDec.todo("cellPngDecGetsCAL(handle=*0x%x, stream=*0x%x, scal=*0x%x)", handle, stream, scal);
	return CELL_OK;
}

error_code cellPngDecGetpHYs(PHandle handle, PStream stream, vm::ptr<CellPngPHYS> phys)
{
	cellPngDec.todo("cellPngDecGetpHYs(handle=*0x%x, stream=*0x%x, phys=*0x%x)", handle, stream, phys);
	return CELL_OK;
}

error_code cellPngDecGetoFFs(PHandle handle, PStream stream, vm::ptr<CellPngOFFS> offs)
{
	cellPngDec.todo("cellPngDecGetoFFs(handle=*0x%x, stream=*0x%x, offs=*0x%x)", handle, stream, offs);
	return CELL_OK;
}

error_code cellPngDecGetsPLT(PHandle handle, PStream stream, vm::ptr<CellPngSPLT> splt)
{
	cellPngDec.todo("cellPngDecGetsPLT(handle=*0x%x, stream=*0x%x, splt=*0x%x)", handle, stream, splt);
	return CELL_OK;
}

error_code cellPngDecGetbKGD(PHandle handle, PStream stream, vm::ptr<CellPngBKGD> bkgd)
{
	cellPngDec.todo("cellPngDecGetbKGD(handle=*0x%x, stream=*0x%x, bkgd=*0x%x)", handle, stream, bkgd);
	return CELL_OK;
}

error_code cellPngDecGettIME(PHandle handle, PStream stream, vm::ptr<CellPngTIME> time)
{
	cellPngDec.todo("cellPngDecGettIME(handle=*0x%x, stream=*0x%x, time=*0x%x)", handle, stream, time);
	return CELL_OK;
}

error_code cellPngDecGethIST(PHandle handle, PStream stream, vm::ptr<CellPngHIST> hist)
{
	cellPngDec.todo("cellPngDecGethIST(handle=*0x%x, stream=*0x%x, hist=*0x%x)", handle, stream, hist);
	return CELL_OK;
}

error_code cellPngDecGettRNS(PHandle handle, PStream stream, vm::ptr<CellPngTRNS> trns)
{
	cellPngDec.todo("cellPngDecGettRNS(handle=*0x%x, stream=*0x%x, trns=*0x%x)", handle, stream, trns);
	return CELL_OK;
}

error_code cellPngDecGetsBIT(PHandle handle, PStream stream, vm::ptr<CellPngSBIT> sbit)
{
	cellPngDec.todo("cellPngDecGetsBIT(handle=*0x%x, stream=*0x%x, sbit=*0x%x)", handle, stream, sbit);
	return CELL_OK;
}

error_code cellPngDecGetiCCP(PHandle handle, PStream stream, vm::ptr<CellPngICCP> iccp)
{
	cellPngDec.todo("cellPngDecGetiCCP(handle=*0x%x, stream=*0x%x, iccp=*0x%x)", handle, stream, iccp);
	return CELL_OK;
}

error_code cellPngDecGetsRGB(PHandle handle, PStream stream, vm::ptr<CellPngSRGB> srgb)
{
	cellPngDec.todo("cellPngDecGetsRGB(handle=*0x%x, stream=*0x%x, srgb=*0x%x)", handle, stream, srgb);
	return CELL_OK;
}

error_code cellPngDecGetgAMA(PHandle handle, PStream stream, vm::ptr<CellPngGAMA> gama)
{
	cellPngDec.todo("cellPngDecGetgAMA(handle=*0x%x, stream=*0x%x, gama=*0x%x)", handle, stream, gama);
	return CELL_OK;
}

error_code cellPngDecGetPLTE(PHandle handle, PStream stream, vm::ptr<CellPngPLTE> plte)
{
	cellPngDec.todo("cellPngDecGetPLTE(handle=*0x%x, stream=*0x%x, plte=*0x%x)", handle, stream, plte);
	return CELL_OK;
}

error_code cellPngDecGetTextChunk(PHandle handle, PStream stream, vm::ptr<u32> textInfoNum, vm::pptr<CellPngTextInfo> textInfo)
{
	cellPngDec.todo("cellPngDecGetTextChunk(handle=*0x%x, stream=*0x%x, textInfoNum=*0x%x, textInfo=*0x%x)", handle, stream, textInfoNum, textInfo);
	return CELL_OK;
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
