#pragma once

namespace vm { using namespace ps3; }

// Error codes
enum
{
	CELL_FONT_OK                               = 0, 
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

struct CellFontLibrary
{
	u32 libraryType, libraryVersion;
	//u32 SystemClosed[];	
};

struct CellFontMemoryInterface
{
	u32 Object_addr; //void*
	//CellFontMallocCallback  Malloc;
	//CellFontFreeCallback    Free;
	//CellFontReallocCallback Realloc;
	//CellFontCallocCallback  Calloc;
};

// Font Set Types
enum
{
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LATIN = 0x00000000,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LIGHT_LATIN = 0x00000001,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_BOLD_LATIN = 0x00000002,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LATIN2 = 0x00000018,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LIGHT_LATIN2 = 0x00000019,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_BOLD_LATIN2 = 0x0000001a,
	CELL_FONT_TYPE_MATISSE_SERIF_LATIN = 0x00000020,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_JAPANESE = 0x00000008,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_LIGHT_JAPANESE = 0x00000009,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_BOLD_JAPANESE = 0x0000000a,
	CELL_FONT_TYPE_YD_GOTHIC_KOREAN = 0x0000000c,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_LATIN = 0x00000040,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_LATIN2 = 0x00000041,
	CELL_FONT_TYPE_VAGR_SANS_SERIF_ROUND = 0x00000043,
	CELL_FONT_TYPE_VAGR_SANS_SERIF_ROUND_LATIN2 = 0x00000044,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_JAPANESE = 0x00000048,

	CELL_FONT_TYPE_NEWRODIN_GOTHIC_JP_SET = 0x00000100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_LATIN_SET = 0x00000101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_SET = 0x00000104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_SET = 0x00000204,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_SET = 0x00000201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_SET = 0x00000108,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_RODIN_SET = 0x00000109,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_RODIN2_SET = 0x00000209,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_TCH_SET = 0x0000010a,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN_TCH_SET = 0x0000010b,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN2_TCH_SET = 0x0000020b,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_SCH_SET = 0x0000010c,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN_SCH_SET = 0x0000010d,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN2_SCH_SET = 0x0000020d,

	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_RSANS_SET = 0x00300104,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_RSANS_SET = 0x00300105,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_JP_SET = 0x00300107,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_YG_DFHEI5_RSANS_SET = 0x00300109,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS_SET = 0x0030010F,
	CELL_FONT_TYPE_VAGR_SEURAT_CAPIE_MARU_GOTHIC_RSANS_SET = 0x00300124,
	CELL_FONT_TYPE_VAGR_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS_SET = 0x00300129,

	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_LIGHT_SET = 0x00040100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN_LIGHT_SET = 0x00040101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_LIGHT_SET = 0x00040201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_LIGHT_SET = 0x00040104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_LIGHT_SET = 0x00040204,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_BOLD_SET = 0x00070100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN_BOLD_SET = 0x00070101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_BOLD_SET = 0x00070201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_BOLD_SET = 0x00070104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_BOLD_SET = 0x00070204,

	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_RSANS2_SET = 0x00300204,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_RSANS2_SET = 0x00300205,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_YG_DFHEI5_RSANS2_SET = 0x00300209,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS2_SET = 0x0030020F,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_VAGR2_SET = 0x00300229,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_VAGR2_SET = 0x00300224,
};

enum
{
	CELL_FONT_MAP_FONT = 0,
	CELL_FONT_MAP_UNICODE = 1,
};

struct CellFontConfig
{
	struct {
		be_t<u32> buffer_addr;
		be_t<u32> size;
	} FileCache;

	be_t<u32> userFontEntryMax;
	be_t<u32> userFontEntrys_addr;
	be_t<u32> flags;
};

struct CellFontRenderer
{
	void *systemReserved[64];
};

//Custom enum to determine the origin of a CellFont object
enum
{
	CELL_FONT_OPEN_FONTSET,
	CELL_FONT_OPEN_FONT_FILE,
	CELL_FONT_OPEN_FONT_INSTANCE,
	CELL_FONT_OPEN_MEMORY,
};

struct stbtt_fontinfo;

struct CellFont
{
	//void* SystemReserved[64];
	be_t<float> scale_x;
	be_t<float> scale_y;
	be_t<float> slant;
	be_t<u32> renderer_addr;

	be_t<u32> fontdata_addr;
	be_t<u32> origin;
	stbtt_fontinfo* stbfont;
	// hack: don't place anything after pointer
};

struct CellFontType
{
	be_t<u32> type;
	be_t<u32> map;
};

struct CellFontInitGraphicsConfigGcm
{
	be_t<u32> configType;
	struct {
		be_t<u32> address;
		be_t<u32> size;
	} GraphicsMemory;
	struct {
		be_t<u32> address;
		be_t<u32> size;
	} MappedMainMemory;
	struct {
		be_t<s16> slotNumber;
		be_t<s16> slotCount;
	} VertexShader;
};

struct CellFontGraphics
{
	u32 graphicsType;
	u32 SystemClosed_addr;
};

struct CellFontHorizontalLayout
{
	be_t<float> baseLineY;
	be_t<float> lineHeight;
	be_t<float> effectHeight;
};

struct CellFontVerticalLayout
{
	be_t<float> baseLineX;
	be_t<float> lineWidth;
	be_t<float> effectWidth;
};

struct CellFontGlyphMetrics
{
	be_t<float> width;
	be_t<float> height;
	struct {
		be_t<float> bearingX;
		be_t<float> bearingY;
		be_t<float> advance;
	} Horizontal;
	struct {
		be_t<float> bearingX;
		be_t<float> bearingY;
		be_t<float> advance;
	} Vertical;
};

struct CellFontImageTransInfo
{
	be_t<u32> Image_addr;
	be_t<u32> imageWidthByte;
	be_t<u32> imageWidth;
	be_t<u32> imageHeight;
	be_t<u32> Surface_addr;
	be_t<u32> surfWidthByte;
};

struct CellFontRendererConfig
{
	struct BufferingPolicy
	{
		be_t<u32> buffer;
		be_t<u32> initSize;
		be_t<u32> maxSize;
		be_t<u32> expandSize;
		be_t<u32> resetSize;
	};
};

struct CellFontRenderSurface
{
	be_t<u32> buffer_addr;
	be_t<u32> widthByte;
	be_t<u32> pixelSizeByte;
	be_t<u32> width, height;
	struct {
		be_t<u32> x0, y0;
		be_t<u32> x1, y1;
	} Scissor;
};

// Internal Datatypes
struct CCellFontInternal      //Module cellFont
{
	u32 m_buffer_addr, m_buffer_size;
	u32 m_userFontEntrys_addr, m_userFontEntryMax;

	bool m_bInitialized;
	bool m_bFontGcmInitialized;

	CCellFontInternal()
		: m_buffer_addr(0)
		, m_buffer_size(0)
		, m_bInitialized(false)
		, m_bFontGcmInitialized(false)
	{
	}
};