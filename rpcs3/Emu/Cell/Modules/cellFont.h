#pragma once

#include "Emu/Memory/vm_ptr.h"

// Error codes
enum CellFontError : u32
{
	CELL_FONT_ERROR_FATAL                      = 0x80540001,
	CELL_FONT_ERROR_INVALID_PARAMETER          = 0x80540002,
	CELL_FONT_ERROR_UNINITIALIZED              = 0x80540003,
	CELL_FONT_ERROR_INITIALIZE_FAILED          = 0x80540004,
	CELL_FONT_ERROR_INVALID_CACHE_BUFFER       = 0x80540005,
	CELL_FONT_ERROR_ALREADY_INITIALIZED        = 0x80540006,
	CELL_FONT_ERROR_ALLOCATION_FAILED          = 0x80540007,
	CELL_FONT_ERROR_NO_SUPPORT_FONTSET         = 0x80540008,
	CELL_FONT_ERROR_OPEN_FAILED                = 0x80540009,
	CELL_FONT_ERROR_READ_FAILED                = 0x8054000a,
	CELL_FONT_ERROR_FONT_OPEN_FAILED           = 0x8054000b,
	CELL_FONT_ERROR_FONT_NOT_FOUND             = 0x8054000c,
	CELL_FONT_ERROR_FONT_OPEN_MAX              = 0x8054000d,
	CELL_FONT_ERROR_FONT_CLOSE_FAILED          = 0x8054000e,
	CELL_FONT_ERROR_ALREADY_OPENED             = 0x8054000f,
	CELL_FONT_ERROR_NO_SUPPORT_FUNCTION        = 0x80540010,
	CELL_FONT_ERROR_NO_SUPPORT_CODE            = 0x80540011,
	CELL_FONT_ERROR_NO_SUPPORT_GLYPH           = 0x80540012,
	CELL_FONT_ERROR_BUFFER_SIZE_NOT_ENOUGH     = 0x80540016,
	CELL_FONT_ERROR_RENDERER_ALREADY_BIND      = 0x80540020,
	CELL_FONT_ERROR_RENDERER_UNBIND            = 0x80540021,
	CELL_FONT_ERROR_RENDERER_INVALID           = 0x80540022,
	CELL_FONT_ERROR_RENDERER_ALLOCATION_FAILED = 0x80540023,
	CELL_FONT_ERROR_ENOUGH_RENDERING_BUFFER    = 0x80540024,
	CELL_FONT_ERROR_NO_SUPPORT_SURFACE         = 0x80540040,
};

// Font Set Types
enum
{
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LATIN         = 0x00000000,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LIGHT_LATIN   = 0x00000001,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_BOLD_LATIN    = 0x00000002,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LATIN2        = 0x00000018,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LIGHT_LATIN2  = 0x00000019,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_BOLD_LATIN2   = 0x0000001a,
	CELL_FONT_TYPE_MATISSE_SERIF_LATIN            = 0x00000020,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_JAPANESE       = 0x00000008,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_LIGHT_JAPANESE = 0x00000009,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_BOLD_JAPANESE  = 0x0000000a,
	CELL_FONT_TYPE_YD_GOTHIC_KOREAN               = 0x0000000c,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_LATIN       = 0x00000040,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_LATIN2      = 0x00000041,
	CELL_FONT_TYPE_VAGR_SANS_SERIF_ROUND          = 0x00000043,
	CELL_FONT_TYPE_VAGR_SANS_SERIF_ROUND_LATIN2   = 0x00000044,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_JAPANESE    = 0x00000048,

	CELL_FONT_TYPE_NEWRODIN_GOTHIC_JP_SET                   = 0x00000100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_LATIN_SET                = 0x00000101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_SET                = 0x00000104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_SET               = 0x00000204,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_SET            = 0x00000201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_SET            = 0x00000108,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_RODIN_SET      = 0x00000109,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_RODIN2_SET     = 0x00000209,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_TCH_SET        = 0x0000010a,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN_TCH_SET  = 0x0000010b,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN2_TCH_SET = 0x0000020b,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_SCH_SET        = 0x0000010c,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN_SCH_SET  = 0x0000010d,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN2_SCH_SET = 0x0000020d,

	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_RSANS_SET                      = 0x00300104,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_RSANS_SET                = 0x00300105,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_JP_SET                   = 0x00300107,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_YG_DFHEI5_RSANS_SET            = 0x00300109,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS_SET      = 0x0030010F,
	CELL_FONT_TYPE_VAGR_SEURAT_CAPIE_MARU_GOTHIC_RSANS_SET           = 0x00300124,
	CELL_FONT_TYPE_VAGR_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS_SET = 0x00300129,

	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_LIGHT_SET        = 0x00040100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN_LIGHT_SET  = 0x00040101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_LIGHT_SET = 0x00040201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_LIGHT_SET     = 0x00040104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_LIGHT_SET    = 0x00040204,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_BOLD_SET         = 0x00070100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN_BOLD_SET   = 0x00070101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_BOLD_SET  = 0x00070201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_BOLD_SET      = 0x00070104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_BOLD_SET     = 0x00070204,

	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_RSANS2_SET                 = 0x00300204,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_RSANS2_SET           = 0x00300205,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_YG_DFHEI5_RSANS2_SET       = 0x00300209,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS2_SET = 0x0030020F,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_VAGR2_SET  = 0x00300229,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_VAGR2_SET            = 0x00300224,
};

enum
{
	CELL_FONT_MAP_FONT    = 0,
	CELL_FONT_MAP_UNICODE = 1,
};

//Custom enum to determine the origin of a CellFont object
enum
{
	CELL_FONT_OPEN_FONTSET,
	CELL_FONT_OPEN_FONT_FILE,
	CELL_FONT_OPEN_FONT_INSTANCE,
	CELL_FONT_OPEN_MEMORY,
};


using CellFontMallocCallback = vm::ptr<void>(vm::ptr<void> arg, u32 size);
using CellFontFreeCallback = void(vm::ptr<void> arg, vm::ptr<void> ptr);
using CellFontReallocCallback = vm::ptr<void>(vm::ptr<void> arg, vm::ptr<void> p, u32 reallocSize);
using CellFontCallocCallback = vm::ptr<void>(vm::ptr<void> arg, u32 num, u32 size);

struct CellFontMemoryInterface
{
	vm::bptr<void> arg;
	vm::bptr<CellFontMallocCallback>  malloc;
	vm::bptr<CellFontFreeCallback>    free;
	vm::bptr<CellFontReallocCallback> realloc;
	vm::bptr<CellFontCallocCallback>  calloc;
};

struct CellFontEntry
{
	be_t<u32> lock;
	be_t<u32> uniqueId;
	vm::bcptr<void> fontLib;
	vm::bptr<void> fontH;
};

struct CellFontConfig
{
	// FileCache
	vm::bptr<u32> fc_buffer;
	be_t<u32> fc_size;

	be_t<u32> userFontEntryMax;
	vm::bptr<CellFontEntry> userFontEntrys;

	be_t<u32> flags;
};

struct CellFontLibrary
{
	be_t<u32> libraryType;
	be_t<u32> libraryVersion;
	// ...
};

struct CellFontType
{
	be_t<u32> type;
	be_t<u32> map;
};

struct CellFontHorizontalLayout
{
	be_t<f32> baseLineY;
	be_t<f32> lineHeight;
	be_t<f32> effectHeight;
};

struct CellFontVerticalLayout
{
	be_t<f32> baseLineX;
	be_t<f32> lineWidth;
	be_t<f32> effectWidth;
};

struct CellFontGlyphMetrics
{
	be_t<f32> width;
	be_t<f32> height;

	be_t<f32> h_bearingX;
	be_t<f32> h_bearingY;
	be_t<f32> h_advance;

	be_t<f32> v_bearingX;
	be_t<f32> v_bearingY;
	be_t<f32> v_advance;
};

struct CellFontRenderSurface
{
	vm::bptr<void> buffer;
	be_t<s32> widthByte;
	be_t<s32> pixelSizeByte;
	be_t<s32> width;
	be_t<s32> height;

	// Scissor
	be_t<u32> sc_x0;
	be_t<u32> sc_y0;
	be_t<u32> sc_x1;
	be_t<u32> sc_y1;
};

struct CellFontImageTransInfo
{
	vm::bptr<u8> image;
	be_t<u32> imageWidthByte;
	be_t<u32> imageWidth;
	be_t<u32> imageHeight;
	vm::bptr<void> surface;
	be_t<u32> surfWidthByte;
};

struct CellFont
{
	be_t<float> scale_x;
	be_t<float> scale_y;
	be_t<float> slant;
	be_t<u32> renderer_addr;

	be_t<u32> fontdata_addr;
	be_t<u32> origin;
	struct stbtt_fontinfo* stbfont;
	// hack: don't place anything after pointer
};

struct CellFontRendererConfig
{
	// Buffering Policy
	vm::bptr<void> buffer;
	be_t<u32> initSize;
	be_t<u32> maxSize;
	be_t<u32> expandSize;
	be_t<u32> resetSize;
};

struct CellFontRenderer
{
	void *systemReserved[64];
};

struct CellFontGraphics
{
	be_t<u32> graphicsType;
	// ...
};
