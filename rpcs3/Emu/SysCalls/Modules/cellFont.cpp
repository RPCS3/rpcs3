#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellFont_init();
Module cellFont(0x0019, cellFont_init);

// error codes

enum
{
	CELL_FONT_OK										= 0, 
	CELL_FONT_ERROR_FATAL								= 0x80540001,
	CELL_FONT_ERROR_INVALID_PARAMETER					= 0x80540002,
	CELL_FONT_ERROR_UNINITIALIZED						= 0x80540003,
	CELL_FONT_ERROR_INITIALIZE_FAILED					= 0x80540004,
	CELL_FONT_ERROR_INVALID_CACHE_BUFFER				= 0x80540005,
	CELL_FONT_ERROR_ALREADY_INITIALIZED					= 0x80540006,
	CELL_FONT_ERROR_ALLOCATION_FAILED					= 0x80540007,
	CELL_FONT_ERROR_NO_SUPPORT_FONTSET					= 0x80540008,
	CELL_FONT_ERROR_OPEN_FAILED							= 0x80540009,
	CELL_FONT_ERROR_READ_FAILED							= 0x8054000a,
	CELL_FONT_ERROR_FONT_OPEN_FAILED					= 0x8054000b,
	CELL_FONT_ERROR_FONT_NOT_FOUND						= 0x8054000c,
	CELL_FONT_ERROR_FONT_OPEN_MAX						= 0x8054000d,
	CELL_FONT_ERROR_FONT_CLOSE_FAILED					= 0x8054000e,
	CELL_FONT_ERROR_ALREADY_OPENED						= 0x8054000f,
	CELL_FONT_ERROR_NO_SUPPORT_FUNCTION					= 0x80540010,
	CELL_FONT_ERROR_NO_SUPPORT_CODE						= 0x80540011,
	CELL_FONT_ERROR_NO_SUPPORT_GLYPH					= 0x80540012,
	CELL_FONT_ERROR_BUFFER_SIZE_NOT_ENOUGH				= 0x80540016,
	CELL_FONT_ERROR_RENDERER_ALREADY_BIND				= 0x80540020,
	CELL_FONT_ERROR_RENDERER_UNBIND						= 0x80540021,
	CELL_FONT_ERROR_RENDERER_INVALID					= 0x80540022,
	CELL_FONT_ERROR_RENDERER_ALLOCATION_FAILED			= 0x80540023,
	CELL_FONT_ERROR_ENOUGH_RENDERING_BUFFER				= 0x80540024,
	CELL_FONT_ERROR_NO_SUPPORT_SURFACE					= 0x80540040,
};

struct CellFontConfig
{ 
	u32 buffer; 
	u32 size; 
	u32 userFontEntryMax; 
	u32 userFontEntrys; 
	u32 flags;
};

struct CellFontType
{ 
	u32 type; 
	u32 map; 
}; 

struct CellFontHorizontalLayout
{ 
	float baseLineY; 
	float lineHeight; 
	float effectHeight; 
}; 

struct CellFontVerticalLayout
{ 
	float baseLineX; 
	float lineWidth; 
	float effectWidth; 
};

struct CellFontGlyphMetrics
{ 
float width; 
float height; 
	struct Horizontal
	{ 
	float bearingX; 
	float bearingY; 
	float advance; 
	}; 
	struct Vertical
	{ 
	float bearingX; 
	float bearingY; 
	float advance; 
	}; 
};

struct CellFontRendererConfig
{
	struct BufferingPolicy
	{
	u32 buffer;
	u32 initSize;
	u32 maxSize;
	u32 expandSize;
	u32 resetSize;
	};
}; 

s32 cellFontInit (u32 buffer, u32 size, u32 userFontEntryMax, u32 userFontEntrys, u32 flags)
{
	cellFont.Log("cellFontInit(buffer=0x%x,size=0x%x,userFontEntryMax=0x%x,flags=0x%x)", buffer, size, userFontEntryMax, flags);
	return CELL_FONT_OK;
}

s32 cellFontSetFontsetOpenMode(u32 openMode)
{
	cellFont.Log("cellFontSetFontsetOpenMode(openMode=0x%x)", openMode);
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

s32 cellFontSetFontOpenMode(u32 openMode)
{
	cellFont.Log("cellFontSetFontOpenMode(openMode=0x%x)", openMode);
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontCreateRenderer()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontGetHorizontalLayout()
{
	UNIMPLEMENTED_FUNC(cellFont);
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

int cellFontOpenFontInstance()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetScalePixel()
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

int cellFontBindRenderer()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontEnd()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetEffectSlant ()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontRenderCharGlyphImage()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

void cellFontRenderSurfaceInit()
{
	UNIMPLEMENTED_FUNC(cellFont);
}

int cellFontGetFontIdCode()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontOpenFontset()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontCloseFont()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

void cellFontRenderSurfaceSetScissor()
{
	UNIMPLEMENTED_FUNC(cellFont);
}

int cellFontGetCharGlyphMetrics()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontUnbindRenderer()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontInitializeWithRevision()
{
	UNIMPLEMENTED_FUNC(cellFont);
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
int cellFontExtend()
{
	UNIMPLEMENTED_FUNC(cellFont);
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
}