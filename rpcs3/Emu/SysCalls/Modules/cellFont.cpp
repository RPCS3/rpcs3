#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellFont.h"

void cellFont_init();
void cellFont_unload();
Module cellFont(0x0019, cellFont_init, nullptr, cellFont_unload);

// Font Set Types
enum
{
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LATIN                    = 0x00000000,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LIGHT_LATIN              = 0x00000001,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_BOLD_LATIN               = 0x00000002,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LATIN2                   = 0x00000018,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_LIGHT_LATIN2             = 0x00000019,
	CELL_FONT_TYPE_RODIN_SANS_SERIF_BOLD_LATIN2              = 0x0000001a,
	CELL_FONT_TYPE_MATISSE_SERIF_LATIN                       = 0x00000020,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_JAPANESE                  = 0x00000008,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_LIGHT_JAPANESE            = 0x00000009,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_BOLD_JAPANESE             = 0x0000000a,
	CELL_FONT_TYPE_YD_GOTHIC_KOREAN                          = 0x0000000c,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_LATIN                  = 0x00000040,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_LATIN2                 = 0x00000041,
	CELL_FONT_TYPE_VAGR_SANS_SERIF_ROUND                     = 0x00000043,
	CELL_FONT_TYPE_VAGR_SANS_SERIF_ROUND_LATIN2              = 0x00000044,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_JAPANESE               = 0x00000048,

	CELL_FONT_TYPE_NEWRODIN_GOTHIC_JP_SET                    = 0x00000100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_LATIN_SET                 = 0x00000101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_SET                 = 0x00000104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_SET                = 0x00000204,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_SET             = 0x00000201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_SET             = 0x00000108,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_RODIN_SET       = 0x00000109,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_RODIN2_SET      = 0x00000209,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_TCH_SET         = 0x0000010a,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN_TCH_SET   = 0x0000010b,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN2_TCH_SET  = 0x0000020b,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_SCH_SET         = 0x0000010c,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN_SCH_SET   = 0x0000010d,
	CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN2_SCH_SET  = 0x0000020d,
	
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_RSANS_SET                      = 0x00300104,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_RSANS_SET                = 0x00300105,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_JP_SET                   = 0x00300107,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_YG_DFHEI5_RSANS_SET            = 0x00300109,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS_SET      = 0x0030010F,
	CELL_FONT_TYPE_VAGR_SEURAT_CAPIE_MARU_GOTHIC_RSANS_SET           = 0x00300124,
	CELL_FONT_TYPE_VAGR_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS_SET = 0x00300129,

	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_LIGHT_SET                      = 0x00040100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN_LIGHT_SET                = 0x00040101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_LIGHT_SET               = 0x00040201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_LIGHT_SET                   = 0x00040104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_LIGHT_SET                  = 0x00040204,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_BOLD_SET                       = 0x00070100,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN_BOLD_SET                 = 0x00070101,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_BOLD_SET                = 0x00070201,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_BOLD_SET                    = 0x00070104,
	CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_BOLD_SET                   = 0x00070204,

	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_RSANS2_SET                     = 0x00300204,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_RSANS2_SET               = 0x00300205,
	CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_YG_DFHEI5_RSANS2_SET           = 0x00300209,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS2_SET     = 0x0030020F,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_VAGR2_SET      = 0x00300229,
	CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_VAGR2_SET                = 0x00300224,
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
	u32 SystemReserved_addr; //void *systemReserved[64];
};

struct CellFont
{
	//void* SystemReserved[64];
	be_t<float> scale_x;
	be_t<float> scale_y;
	be_t<float> slant;
	be_t<u32> renderer_addr;
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
		: m_buffer_addr(NULL)
		, m_buffer_size(0)
		, m_bInitialized(false)
		, m_bFontGcmInitialized(false)
	{
	}
};

CCellFontInternal* s_fontInternalInstance = new CCellFontInternal();

// Functions
int cellFontInitializeWithRevision(u64 revisionFlags, mem_ptr_t<CellFontConfig> config)
{
	cellFont.Warning("cellFontInitializeWithRevision(revisionFlags=0x%llx, config=0x%x)", revisionFlags, config.GetAddr());
	
	if (s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_ALREADY_INITIALIZED;
	if (!config.IsGood())
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	if (!Memory.IsGoodAddr(config->FileCache.buffer_addr))
		return CELL_FONT_ERROR_INVALID_CACHE_BUFFER;
	if (config->FileCache.size < 24)
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	if (config->flags != 0)
		cellFont.Warning("cellFontInitializeWithRevision: Unknown flags (0x%x)", config->flags);

	s_fontInternalInstance->m_buffer_addr = config->FileCache.buffer_addr;
	s_fontInternalInstance->m_buffer_size = config->FileCache.size;
	s_fontInternalInstance->m_userFontEntrys_addr = config->userFontEntrys_addr;
	s_fontInternalInstance->m_userFontEntryMax    = config->userFontEntryMax;
	s_fontInternalInstance->m_bInitialized = true;
	return CELL_FONT_OK;
}

int cellFontGetRevisionFlags(mem64_t revisionFlags)
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontInit(mem_ptr_t<CellFontConfig> config)
{
	cellFont.Log("cellFontInit(config=0x%x)", config.GetAddr());

	MemoryAllocator<u64> revisionFlags = 0;
	cellFontGetRevisionFlags(revisionFlags.GetAddr());
	return cellFontInitializeWithRevision(revisionFlags, config.GetAddr());
}

int cellFontEnd()
{
	cellFont.Log("cellFontEnd()");

	if (!s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_UNINITIALIZED;

	s_fontInternalInstance->m_bInitialized = false;
	return CELL_FONT_OK;
}

s32 cellFontSetFontsetOpenMode(u32 openMode)
{
	cellFont.Log("cellFontSetFontsetOpenMode(openMode=0x%x)", openMode);
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontOpenFontset(mem_ptr_t<CellFontLibrary> library, mem_ptr_t<CellFontType> fontType, mem_ptr_t<CellFont> font)
{
	cellFont.Warning("cellFontOpenFontset(library_addr=0x%x, fontType_addr=0x%x, font_addr=0x%x)",
		library.GetAddr(), fontType.GetAddr(), font.GetAddr());

	if (!library.IsGood() || !fontType.IsGood() || !font.IsGood())
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	if (!s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_UNINITIALIZED;
	if (fontType->map != CELL_FONT_MAP_UNICODE)
		cellFont.Warning("cellFontOpenFontset: Only Unicode is supported");
	
	font->renderer_addr = NULL;
	//TODO: Write data in font

	return CELL_FONT_OK;
}

int cellFontOpenFontInstance(mem_ptr_t<CellFont> openedFont, mem_ptr_t<CellFont> font)
{
	cellFont.Warning("cellFontOpenFontInstance(openedFont=0x%x, font=0x%x)", openedFont.GetAddr(), font.GetAddr());
	return CELL_FONT_OK;
}

s32 cellFontSetFontOpenMode(u32 openMode)
{
	cellFont.Log("cellFontSetFontOpenMode(openMode=0x%x)", openMode);
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontCreateRenderer(mem_ptr_t<CellFontLibrary> library, mem_ptr_t<CellFontRendererConfig> config, mem_ptr_t<CellFontRenderer> Renderer)
{
	cellFont.Warning("cellFontCreateRenderer(library_addr=0x%x, config_addr=0x%x, Renderer_addr=0x%x)",
		library.GetAddr(), config.GetAddr(), Renderer.GetAddr());

	if (!library.IsGood() || !config.IsGood() || !Renderer.IsGood())
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	if (!s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_UNINITIALIZED;
	
	//Write data in Renderer

	return CELL_FONT_OK;
}

void cellFontRenderSurfaceInit(mem_ptr_t<CellFontRenderSurface> surface, u32 buffer_addr, s32 bufferWidthByte, s32 pixelSizeByte, s32 w, s32 h)
{
	cellFont.Warning("cellFontRenderSurfaceInit(surface_addr=0x%x, buffer_addr=0x%x, bufferWidthByte=%d, pixelSizeByte=%d, w=%d, h=%d)",
		surface.GetAddr(), buffer_addr, bufferWidthByte, pixelSizeByte, w, h);

	surface->buffer_addr	= buffer_addr;
	surface->widthByte		= bufferWidthByte;
	surface->pixelSizeByte	= pixelSizeByte;
	surface->width			= w;
	surface->height			= h;
}

void cellFontRenderSurfaceSetScissor(mem_ptr_t<CellFontRenderSurface> surface, s32 x0, s32 y0, s32 w, s32 h)
{
	cellFont.Warning("cellFontRenderSurfaceSetScissor(surface_addr=0x%x, x0=%d, y0=%d, w=%d, h=%d)",
		surface.GetAddr(), x0, y0, w, h);

	surface->Scissor.x0 = x0;
	surface->Scissor.y0 = y0;
	surface->Scissor.x1 = w;
	surface->Scissor.y1 = h;
}

int cellFontSetScalePixel(mem_ptr_t<CellFont> font, float w, float h)
{
	h = GetCurrentPPUThread().FPR[1]; // TODO: Something is wrong with the float arguments
	cellFont.Warning("cellFontSetScalePixel(font_addr=0x%x, w=%f, h=%f)", font.GetAddr(), w, h);

	if (!font.IsGood())
		return CELL_FONT_ERROR_INVALID_PARAMETER;

	font->scale_x = w;
	font->scale_y = h;
	return CELL_FONT_OK;
}

int cellFontGetHorizontalLayout(mem_ptr_t<CellFont> font, mem_ptr_t<CellFontHorizontalLayout> layout)
{
	cellFont.Warning("cellFontGetHorizontalLayout(font_addr=0x%x, layout_addr=0x%x)",
		font.GetAddr(), layout.GetAddr());

	if (!font.IsGood() || !layout.IsGood())
		return CELL_FONT_ERROR_INVALID_PARAMETER;

	//TODO: This values are (probably) wrong and just for testing purposes! Find the way of calculating them.
	layout->baseLineY = font->scale_y - 4;
	layout->lineHeight = font->scale_y;
	layout->effectHeight = 4;
	return CELL_FONT_OK;
}

int cellFontBindRenderer(mem_ptr_t<CellFont> font, mem_ptr_t<CellFontRenderer> renderer)
{
	cellFont.Warning("cellFontBindRenderer(font_addr=0x%x, renderer_addr=0x%x)",
		font.GetAddr(), renderer.GetAddr());
	
	if (!font.IsGood() || !renderer.GetAddr())
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	if (font->renderer_addr)
		return CELL_FONT_ERROR_RENDERER_ALREADY_BIND;

	font->renderer_addr = renderer.GetAddr();
	return CELL_FONT_OK;
}

int cellFontUnbindRenderer(mem_ptr_t<CellFont> font)
{
	cellFont.Warning("cellFontBindRenderer(font_addr=0x%x)", font.GetAddr());
	
	if (!font.IsGood())
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	if (!font->renderer_addr)
		return CELL_FONT_ERROR_RENDERER_UNBIND;

	font->renderer_addr = NULL;
	return CELL_FONT_OK;
}

int cellFontDestroyRenderer()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetupRenderScalePixel()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGetRenderCharGlyphMetrics()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontEndLibrary()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetEffectSlant(mem_ptr_t<CellFont> font, float slantParam)
{
	cellFont.Warning("cellFontSetEffectSlant(font_addr=0x%x, slantParam=%f)", font.GetAddr(), slantParam);

	if (!font.IsGood() || slantParam < -1.0 || slantParam > 1.0)
		return CELL_FONT_ERROR_INVALID_PARAMETER;

	font->slant = slantParam;
	return CELL_FONT_OK;
}

int cellFontGetEffectSlant(mem_ptr_t<CellFont> font, mem32_t slantParam)
{
	cellFont.Warning("cellFontSetEffectSlant(font_addr=0x%x, slantParam_addr=0x%x)", font.GetAddr(), slantParam.GetAddr());

	if (!font.IsGood() || !slantParam.IsGood())
		return CELL_FONT_ERROR_INVALID_PARAMETER;

	slantParam = font->slant;
	return CELL_FONT_OK;
}

int cellFontRenderCharGlyphImage()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGetFontIdCode(mem_ptr_t<CellFont> font, u32 code, mem32_t fontId, mem32_t fontCode)
{
	cellFont.Warning("cellFontGetFontIdCode(font_addr=0x%x, code=0x%x, fontId_addr=0x%x, fontCode_addr=0x%x",
		font.GetAddr(), code, fontId.GetAddr(), fontCode.GetAddr());

	if (!font.IsGood() || !fontId.IsGood()) //fontCode isn't used
		return CELL_FONT_ERROR_INVALID_PARAMETER;

	return CELL_FONT_OK;
}

int cellFontCloseFont()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGetCharGlyphMetrics(mem_ptr_t<CellFont> font, u32 code, mem_ptr_t<CellFontGlyphMetrics> metrics)
{
	cellFont.Warning("cellFontGetCharGlyphMetrics(font_addr=0x%x, code=0x%x, metrics_addr=0x%x",
		font.GetAddr(), code, metrics.GetAddr());

	if (!font.IsGood() || metrics.IsGood())
		return CELL_FONT_ERROR_INVALID_PARAMETER;

	//TODO: This values are (probably) wrong and just for testing purposes! Find the way of calculating them.
	metrics->width = 0;
	metrics->height = 0;
	metrics->Horizontal.bearingX = 0;
	metrics->Horizontal.bearingY = 0;
	metrics->Horizontal.advance = 0;
	metrics->Vertical.bearingX = 0;
	metrics->Vertical.bearingY = 0;
	metrics->Vertical.advance = 0;
	return CELL_FONT_OK;
}

int cellFontGraphicsSetFontRGBA()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontOpenFontsetOnMemory()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontOpenFontFile()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGraphicsSetScalePixel()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGraphicsGetScalePixel()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetEffectWeight()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGlyphSetupVertexesGlyph()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGetVerticalLayout()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGetRenderCharGlyphMetricsVertical()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetScalePoint()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetupRenderEffectSlant()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGraphicsSetLineRGBA()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGraphicsSetDrawType()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontEndGraphics()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGraphicsSetupDrawContext()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontOpenFontMemory()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetupRenderEffectWeight()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGlyphGetOutlineControlDistance()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGlyphGetVertexesGlyphSize()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGenerateCharGlyph()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontDeleteGlyph()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontExtend(u32 a1, u32 a2, u32 a3)
{
	cellFont.Warning("cellFontExtend(a1=0x%x, a2=0x%x, a3=0x%x)", a1, a2, a3);
	//In a test I did: a1=0xcfe00000, a2=0x0, a3=(pointer to something)
	if (a1 == 0xcfe00000)
		{
		if (a2 != 0 || a3 == 0)
		{
			//Something happens
		}
		if (Memory.Read32(a3) == 0)
		{
			//Something happens
		}
		//Something happens
	}
	//Something happens?
	return CELL_FONT_OK;
}

int cellFontRenderCharGlyphImageVertical()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetResolutionDpi()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGetCharGlyphMetricsVertical()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

void cellFont_init()
{
	cellFont.AddFunc(0x25c107e6, cellFontInit);
	cellFont.AddFunc(0x6bf6f832, cellFontSetFontsetOpenMode);
	cellFont.AddFunc(0x6cfada83, cellFontSetFontOpenMode);
	cellFont.AddFunc(0x042e74e3, cellFontCreateRenderer);
	cellFont.AddFunc(0x1387c45c, cellFontGetHorizontalLayout);
	cellFont.AddFunc(0x21ebb248, cellFontDestroyRenderer);
	cellFont.AddFunc(0x227e1e3c, cellFontSetupRenderScalePixel);
	cellFont.AddFunc(0x29329541, cellFontOpenFontInstance);
	cellFont.AddFunc(0x297f0e93, cellFontSetScalePixel);
	cellFont.AddFunc(0x2da9fd9d, cellFontGetRenderCharGlyphMetrics);
	cellFont.AddFunc(0x40d40544, cellFontEndLibrary);
	cellFont.AddFunc(0x66a23100, cellFontBindRenderer);
	cellFont.AddFunc(0x7ab47f7e, cellFontEnd);
	cellFont.AddFunc(0x8657c8f5, cellFontSetEffectSlant);
	cellFont.AddFunc(0xe16e679a, cellFontGetEffectSlant);
	cellFont.AddFunc(0x88be4799, cellFontRenderCharGlyphImage);
	cellFont.AddFunc(0x90b9465e, cellFontRenderSurfaceInit);
	cellFont.AddFunc(0x98ac5524, cellFontGetFontIdCode);
	cellFont.AddFunc(0xa885cc9b, cellFontOpenFontset);
	cellFont.AddFunc(0xb276f1f6, cellFontCloseFont);
	cellFont.AddFunc(0xb422b005, cellFontRenderSurfaceSetScissor);
	cellFont.AddFunc(0xd8eaee9f, cellFontGetCharGlyphMetrics);
	cellFont.AddFunc(0xf03dcc29, cellFontInitializeWithRevision);
	cellFont.AddFunc(0x061049ad, cellFontGraphicsSetFontRGBA);
	cellFont.AddFunc(0x073fa321, cellFontOpenFontsetOnMemory);
	cellFont.AddFunc(0x0a7306a4, cellFontOpenFontFile);
	cellFont.AddFunc(0x16322df1, cellFontGraphicsSetScalePixel);
	cellFont.AddFunc(0x2388186c, cellFontGraphicsGetScalePixel);
	cellFont.AddFunc(0x25253fe4, cellFontSetEffectWeight);
	cellFont.AddFunc(0x53f529fe, cellFontGlyphSetupVertexesGlyph);
	cellFont.AddFunc(0x698897f8, cellFontGetVerticalLayout);
	cellFont.AddFunc(0x700e6223, cellFontGetRenderCharGlyphMetricsVertical);
	cellFont.AddFunc(0x70f3e728, cellFontSetScalePoint);
	cellFont.AddFunc(0x78d05e08, cellFontSetupRenderEffectSlant);
	cellFont.AddFunc(0x7c83bc15, cellFontGraphicsSetLineRGBA);
	cellFont.AddFunc(0x87bd650f, cellFontGraphicsSetDrawType);
	cellFont.AddFunc(0x8a35c887, cellFontEndGraphics);
	cellFont.AddFunc(0x970d4c22, cellFontGraphicsSetupDrawContext);
	cellFont.AddFunc(0x9e19072b, cellFontOpenFontMemory);
	cellFont.AddFunc(0xa6dc25d1, cellFontSetupRenderEffectWeight);
	cellFont.AddFunc(0xa8fae920, cellFontGlyphGetOutlineControlDistance);
	cellFont.AddFunc(0xb4d112af, cellFontGlyphGetVertexesGlyphSize);
	cellFont.AddFunc(0xc17259de, cellFontGenerateCharGlyph);
	cellFont.AddFunc(0xd62f5d76, cellFontDeleteGlyph);
	cellFont.AddFunc(0xdee0836c, cellFontExtend);
	cellFont.AddFunc(0xe857a0ca, cellFontRenderCharGlyphImageVertical);
	cellFont.AddFunc(0xfb3341ba, cellFontSetResolutionDpi);
	cellFont.AddFunc(0xfe9a6dd7, cellFontGetCharGlyphMetricsVertical);
	cellFont.AddFunc(0xf16379fa, cellFontUnbindRenderer);
	cellFont.AddFunc(0xb015a84e, cellFontGetRevisionFlags);
}

void cellFont_unload()
{
	s_fontInternalInstance->m_bInitialized = false;
	s_fontInternalInstance->m_bFontGcmInitialized = false;
}