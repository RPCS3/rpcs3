#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "stblib/stb_truetype.h"
#include "Emu/FS/vfsFile.h"
#include "cellFont.h"

extern Module cellFont;

std::unique_ptr<CellFontInternal> g_font;

// Functions
s32 cellFontInitializeWithRevision(u64 revisionFlags, vm::ptr<CellFontConfig> config)
{
	cellFont.Warning("cellFontInitializeWithRevision(revisionFlags=0x%llx, config=*0x%x)", revisionFlags, config);
	
	if (g_font->m_bInitialized)
	{
		return CELL_FONT_ERROR_ALREADY_INITIALIZED;
	}

	if (config->FileCache.size < 24)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (config->flags != 0)
	{
		cellFont.Error("cellFontInitializeWithRevision: Unknown flags (0x%x)", config->flags);
	}

	g_font->m_buffer_addr = config->FileCache.buffer_addr;
	g_font->m_buffer_size = config->FileCache.size;
	g_font->m_userFontEntrys_addr = config->userFontEntrys_addr;
	g_font->m_userFontEntryMax    = config->userFontEntryMax;
	g_font->m_bInitialized = true;
	return CELL_OK;
}

s32 cellFontGetRevisionFlags(vm::ptr<u64> revisionFlags)
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontInit(PPUThread& CPU, vm::ptr<CellFontConfig> config)
{
	cellFont.Warning("cellFontInit(config=*0x%x)", config);

	vm::stackvar<be_t<u64>> revisionFlags(CPU);
	revisionFlags.value() = 0;
	cellFontGetRevisionFlags(revisionFlags);

	return cellFontInitializeWithRevision(revisionFlags.value(), config);
}

s32 cellFontEnd()
{
	cellFont.Warning("cellFontEnd()");

	if (!g_font->m_bInitialized)
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	g_font->m_bInitialized = false;

	return CELL_OK;
}

s32 cellFontSetFontsetOpenMode(u32 openMode)
{
	cellFont.Todo("cellFontSetFontsetOpenMode(openMode=0x%x)", openMode);
	return CELL_OK;
}

s32 cellFontOpenFontMemory(vm::ptr<CellFontLibrary> library, u32 fontAddr, u32 fontSize, u32 subNum, u32 uniqueId, vm::ptr<CellFont> font)
{
	cellFont.Warning("cellFontOpenFontMemory(library=*0x%x, fontAddr=0x%x, fontSize=%d, subNum=%d, uniqueId=%d, font=*0x%x)", library, fontAddr, fontSize, subNum, uniqueId, font);

	if (!g_font->m_bInitialized)
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	font->stbfont = (stbtt_fontinfo*)((u8*)&(font->stbfont) + sizeof(void*)); // hack: use next bytes of the struct

	if (!stbtt_InitFont(font->stbfont, vm::get_ptr<unsigned char>(fontAddr), 0))
		return CELL_FONT_ERROR_FONT_OPEN_FAILED;

	font->renderer_addr = 0;
	font->fontdata_addr = fontAddr;
	font->origin = CELL_FONT_OPEN_MEMORY;

	return CELL_OK;
}

s32 cellFontOpenFontFile(vm::ptr<CellFontLibrary> library, vm::cptr<char> fontPath, u32 subNum, s32 uniqueId, vm::ptr<CellFont> font)
{
	cellFont.Warning("cellFontOpenFontFile(library=*0x%x, fontPath=*0x%x, subNum=%d, uniqueId=%d, font=*0x%x)", library, fontPath, subNum, uniqueId, font);

	vfsFile f(fontPath.get_ptr());
	if (!f.IsOpened())
	{
		return CELL_FONT_ERROR_FONT_OPEN_FAILED;
	}

	u32 fileSize = (u32)f.GetSize();
	u32 bufferAddr = vm::alloc(fileSize, vm::main); // Freed in cellFontCloseFont
	f.Read(vm::get_ptr<void>(bufferAddr), fileSize);
	s32 ret = cellFontOpenFontMemory(library, bufferAddr, fileSize, subNum, uniqueId, font);
	font->origin = CELL_FONT_OPEN_FONT_FILE;

	return ret;
}

s32 cellFontOpenFontset(PPUThread& CPU, vm::ptr<CellFontLibrary> library, vm::ptr<CellFontType> fontType, vm::ptr<CellFont> font)
{
	cellFont.Warning("cellFontOpenFontset(library=*0x%x, fontType=*0x%x, font=*0x%x)", library, fontType, font);

	if (!g_font->m_bInitialized)
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}
	
	if (fontType->map != CELL_FONT_MAP_UNICODE)
	{
		cellFont.Warning("cellFontOpenFontset: Only Unicode is supported");
	}
	
	std::string file;
	switch((u32)fontType->type)
	{
	case CELL_FONT_TYPE_RODIN_SANS_SERIF_LATIN:         file = "/dev_flash/data/font/SCE-PS3-RD-R-LATIN.TTF";  break;
	case CELL_FONT_TYPE_RODIN_SANS_SERIF_LIGHT_LATIN:   file = "/dev_flash/data/font/SCE-PS3-RD-L-LATIN.TTF";  break;
	case CELL_FONT_TYPE_RODIN_SANS_SERIF_BOLD_LATIN:    file = "/dev_flash/data/font/SCE-PS3-RD-B-LATIN.TTF";  break;
	case CELL_FONT_TYPE_RODIN_SANS_SERIF_LATIN2:        file = "/dev_flash/data/font/SCE-PS3-RD-R-LATIN2.TTF"; break;
	case CELL_FONT_TYPE_RODIN_SANS_SERIF_LIGHT_LATIN2:  file = "/dev_flash/data/font/SCE-PS3-RD-L-LATIN2.TTF"; break;
	case CELL_FONT_TYPE_RODIN_SANS_SERIF_BOLD_LATIN2:   file = "/dev_flash/data/font/SCE-PS3-RD-B-LATIN2.TTF"; break;
	case CELL_FONT_TYPE_MATISSE_SERIF_LATIN:            file = "/dev_flash/data/font/SCE-PS3-MT-R-LATIN.TTF";  break;
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_JAPANESE:       file = "/dev_flash/data/font/SCE-PS3-NR-R-JPN.TTF";    break;
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_LIGHT_JAPANESE: file = "/dev_flash/data/font/SCE-PS3-NR-L-JPN.TTF";    break;
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_BOLD_JAPANESE:  file = "/dev_flash/data/font/SCE-PS3-NR-B-JPN.TTF";    break;
	case CELL_FONT_TYPE_YD_GOTHIC_KOREAN:               file = "/dev_flash/data/font/SCE-PS3-YG-R-KOR.TTF";    break;
	case CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_LATIN:       file = "/dev_flash/data/font/SCE-PS3-SR-R-LATIN.TTF";  break;
	case CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_LATIN2:      file = "/dev_flash/data/font/SCE-PS3-SR-R-LATIN2.TTF"; break;
	case CELL_FONT_TYPE_VAGR_SANS_SERIF_ROUND:          file = "/dev_flash/data/font/SCE-PS3-VR-R-LATIN.TTF";  break;
	case CELL_FONT_TYPE_VAGR_SANS_SERIF_ROUND_LATIN2:   file = "/dev_flash/data/font/SCE-PS3-VR-R-LATIN2.TTF"; break;
	case CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_JAPANESE:    file = "/dev_flash/data/font/SCE-PS3-SR-R-JPN.TTF";    break;

	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_JP_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_LATIN_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_RODIN_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_DFHEI5_RODIN2_SET:
	case CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_TCH_SET:
	case CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN_TCH_SET:
	case CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN2_TCH_SET:
	case CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_SCH_SET:
	case CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN_SCH_SET:
	case CELL_FONT_TYPE_DFHEI5_GOTHIC_YG_NEWRODIN_RODIN2_SCH_SET:
	case CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_RSANS_SET:
	case CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_RSANS_SET:
	case CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_JP_SET:
	case CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_YG_DFHEI5_RSANS_SET:
	case CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS_SET:
	case CELL_FONT_TYPE_VAGR_SEURAT_CAPIE_MARU_GOTHIC_RSANS_SET:
	case CELL_FONT_TYPE_VAGR_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_LIGHT_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN_LIGHT_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_LIGHT_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_LIGHT_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_LIGHT_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_BOLD_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN_BOLD_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_YG_RODIN2_BOLD_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN_BOLD_SET:
	case CELL_FONT_TYPE_NEWRODIN_GOTHIC_RODIN2_BOLD_SET:
	case CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_RSANS2_SET:
	case CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_RSANS2_SET:
	case CELL_FONT_TYPE_SEURAT_MARU_GOTHIC_YG_DFHEI5_RSANS2_SET:
	case CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_RSANS2_SET:
	case CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_YG_DFHEI5_VAGR2_SET:
	case CELL_FONT_TYPE_SEURAT_CAPIE_MARU_GOTHIC_VAGR2_SET:
		cellFont.Warning("cellFontOpenFontset: fontType->type = %d not supported yet. RD-R-LATIN.TTF will be used instead.", fontType->type);
		file = "/dev_flash/data/font/SCE-PS3-RD-R-LATIN.TTF";
		break;

	default:
		cellFont.Warning("cellFontOpenFontset: fontType->type = %d not supported.", fontType->type);
		return CELL_FONT_ERROR_NO_SUPPORT_FONTSET;
	}

	vm::stackvar<char> f(CPU, (u32)file.length() + 1, 1);
	memcpy(f.get_ptr(), file.c_str(), file.size() + 1);
	s32 ret = cellFontOpenFontFile(library, f, 0, 0, font); //TODO: Find the correct values of subNum, uniqueId
	font->origin = CELL_FONT_OPEN_FONTSET;

	return ret;
}

s32 cellFontOpenFontInstance(vm::ptr<CellFont> openedFont, vm::ptr<CellFont> font)
{
	cellFont.Warning("cellFontOpenFontInstance(openedFont=*0x%x, font=*0x%x)", openedFont, font);

	font->renderer_addr = openedFont->renderer_addr;
	font->scale_x = openedFont->scale_x;
	font->scale_y = openedFont->scale_y;
	font->slant = openedFont->slant;
	font->stbfont = openedFont->stbfont;
	font->origin = CELL_FONT_OPEN_FONT_INSTANCE;

	return CELL_OK;
}

s32 cellFontSetFontOpenMode(u32 openMode)
{
	cellFont.Todo("cellFontSetFontOpenMode(openMode=0x%x)", openMode);
	return CELL_OK;
}

s32 cellFontCreateRenderer(vm::ptr<CellFontLibrary> library, vm::ptr<CellFontRendererConfig> config, vm::ptr<CellFontRenderer> Renderer)
{
	cellFont.Todo("cellFontCreateRenderer(library=*0x%x, config=*0x%x, Renderer=*0x%x)", library, config, Renderer);

	if (!g_font->m_bInitialized)
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}
	
	//Write data in Renderer

	return CELL_OK;
}

void cellFontRenderSurfaceInit(vm::ptr<CellFontRenderSurface> surface, vm::ptr<void> buffer, s32 bufferWidthByte, s32 pixelSizeByte, s32 w, s32 h)
{
	cellFont.Warning("cellFontRenderSurfaceInit(surface=*0x%x, buffer=*0x%x, bufferWidthByte=%d, pixelSizeByte=%d, w=%d, h=%d)", surface, buffer, bufferWidthByte, pixelSizeByte, w, h);

	surface->buffer_addr	= buffer.addr();
	surface->widthByte		= bufferWidthByte;
	surface->pixelSizeByte	= pixelSizeByte;
	surface->width			= w;
	surface->height			= h;

	if (!buffer)
	{
		surface->buffer_addr = vm::alloc(bufferWidthByte * h, vm::main); // TODO: Huge memory leak
	}
}

void cellFontRenderSurfaceSetScissor(vm::ptr<CellFontRenderSurface> surface, s32 x0, s32 y0, s32 w, s32 h)
{
	cellFont.Warning("cellFontRenderSurfaceSetScissor(surface=*0x%x, x0=%d, y0=%d, w=%d, h=%d)", surface, x0, y0, w, h);

	surface->Scissor.x0 = x0;
	surface->Scissor.y0 = y0;
	surface->Scissor.x1 = w;
	surface->Scissor.y1 = h;
}

s32 cellFontSetScalePixel(vm::ptr<CellFont> font, float w, float h)
{
	cellFont.Log("cellFontSetScalePixel(font=*0x%x, w=%f, h=%f)", font, w, h);

	font->scale_x = w;
	font->scale_y = h;

	return CELL_OK;
}

s32 cellFontGetHorizontalLayout(vm::ptr<CellFont> font, vm::ptr<CellFontHorizontalLayout> layout)
{
	cellFont.Log("cellFontGetHorizontalLayout(font=*0x%x, layout=*0x%x)", font, layout);

	s32 ascent, descent, lineGap;
	float scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	stbtt_GetFontVMetrics(font->stbfont, &ascent, &descent, &lineGap);

	layout->baseLineY = ascent * scale;
	layout->lineHeight = (ascent-descent+lineGap) * scale;
	layout->effectHeight = lineGap * scale;

	return CELL_OK;
}

s32 cellFontBindRenderer(vm::ptr<CellFont> font, vm::ptr<CellFontRenderer> renderer)
{
	cellFont.Warning("cellFontBindRenderer(font=*0x%x, renderer=*0x%x)", font, renderer);

	if (font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_ALREADY_BIND;
	}

	font->renderer_addr = renderer.addr();

	return CELL_OK;
}

s32 cellFontUnbindRenderer(vm::ptr<CellFont> font)
{
	cellFont.Warning("cellFontBindRenderer(font=*0x%x)", font);
	
	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	font->renderer_addr = 0;

	return CELL_OK;
}

s32 cellFontDestroyRenderer()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontSetupRenderScalePixel(vm::ptr<CellFont> font, float w, float h)
{
	cellFont.Todo("cellFontSetupRenderScalePixel(font=*0x%x, w=%f, h=%f)", font, w, h);

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO: ?
	return CELL_OK;
}

s32 cellFontGetRenderCharGlyphMetrics(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont.Todo("cellFontGetRenderCharGlyphMetrics(font=*0x%x, code=0x%x, metrics=*0x%x)", font, code, metrics);

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO: ?
	return CELL_OK;
}

s32 cellFontRenderCharGlyphImage(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontRenderSurface> surface, float x, float y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont.Log("cellFontRenderCharGlyphImage(font=*0x%x, code=0x%x, surface=*0x%x, x=%f, y=%f, metrics=*0x%x, trans=*0x%x)", font, code, surface, x, y, metrics, transInfo);

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// Render the character
	s32 width, height, xoff, yoff;
	float scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	unsigned char* box = stbtt_GetCodepointBitmap(font->stbfont, scale, scale, code, &width, &height, &xoff, &yoff);

	if (!box)
	{
		return CELL_OK;
	}

	// Get the baseLineY value
	s32 baseLineY;
	s32 ascent, descent, lineGap;
	stbtt_GetFontVMetrics(font->stbfont, &ascent, &descent, &lineGap);
	baseLineY = (int)((float)ascent * scale); // ???

	// Move the rendered character to the surface
	unsigned char* buffer = vm::get_ptr<unsigned char>(surface->buffer_addr);
	for (u32 ypos = 0; ypos < (u32)height; ypos++)
	{
		if ((u32)y + ypos + yoff + baseLineY >= surface->height)
			break;

		for (u32 xpos = 0; xpos < (u32)width; xpos++)
		{
			if ((u32)x + xpos >= surface->width)
				break;

			// TODO: There are some oddities in the position of the character in the final buffer
			buffer[((s32)y + ypos + yoff + baseLineY)*surface->width + (s32)x + xpos] = box[ypos * width + xpos];
		}
	}
	stbtt_FreeBitmap(box, 0);
	return CELL_OK;
}

s32 cellFontEndLibrary()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontSetEffectSlant(vm::ptr<CellFont> font, float slantParam)
{
	cellFont.Log("cellFontSetEffectSlant(font=*0x%x, slantParam=%f)", font, slantParam);

	if (slantParam < -1.0 || slantParam > 1.0)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	font->slant = slantParam;

	return CELL_OK;
}

s32 cellFontGetEffectSlant(vm::ptr<CellFont> font, vm::ptr<float> slantParam)
{
	cellFont.Warning("cellFontSetEffectSlant(font=*0x%x, slantParam=*0x%x)", font, slantParam);

	*slantParam = font->slant;

	return CELL_OK;
}

s32 cellFontGetFontIdCode(vm::ptr<CellFont> font, u32 code, vm::ptr<u32> fontId, vm::ptr<u32> fontCode)
{
	cellFont.Todo("cellFontGetFontIdCode(font=*0x%x, code=0x%x, fontId=*0x%x, fontCode=*0x%x)", font, code, fontId, fontCode);

	// TODO: ?
	return CELL_OK;
}

s32 cellFontCloseFont(vm::ptr<CellFont> font)
{
	cellFont.Warning("cellFontCloseFont(font=*0x%x)", font);

	if (font->origin == CELL_FONT_OPEN_FONTSET ||
		font->origin == CELL_FONT_OPEN_FONT_FILE ||
		font->origin == CELL_FONT_OPEN_MEMORY)
	{
		vm::dealloc(font->fontdata_addr, vm::main);
	}

	return CELL_OK;
}

s32 cellFontGetCharGlyphMetrics(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont.Warning("cellFontGetCharGlyphMetrics(font=*0x%x, code=0x%x, metrics=*0x%x)", font, code, metrics);

	s32 x0, y0, x1, y1;
	s32 advanceWidth, leftSideBearing;
	float scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	stbtt_GetCodepointBox(font->stbfont, code, &x0, &y0, &x1, &y1);
	stbtt_GetCodepointHMetrics(font->stbfont, code, &advanceWidth, &leftSideBearing);
	
	// TODO: Add the rest of the information
	metrics->width = (x1-x0) * scale;
	metrics->height = (y1-y0) * scale;
	metrics->Horizontal.bearingX = (float)leftSideBearing * scale;
	metrics->Horizontal.bearingY = 0.f;
	metrics->Horizontal.advance = (float)advanceWidth * scale;
	metrics->Vertical.bearingX = 0.f;
	metrics->Vertical.bearingY = 0.f;
	metrics->Vertical.advance = 0.f;

	return CELL_OK;
}

s32 cellFontGraphicsSetFontRGBA()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontOpenFontsetOnMemory(PPUThread& CPU, vm::ptr<CellFontLibrary> library, vm::ptr<CellFontType> fontType, vm::ptr<CellFont> font)
{
	cellFont.Todo("cellFontOpenFontsetOnMemory(library=*0x%x, fontType=*0x%x, font=*0x%x)", library, fontType, font);

	if (!g_font->m_bInitialized)
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (fontType->map != CELL_FONT_MAP_UNICODE)
	{
		cellFont.Warning("cellFontOpenFontsetOnMemory: Only Unicode is supported");
	}

	return CELL_OK;
}

s32 cellFontGraphicsSetScalePixel()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGraphicsGetScalePixel()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontSetEffectWeight()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGlyphSetupVertexesGlyph()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGetVerticalLayout()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGetRenderCharGlyphMetricsVertical()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontSetScalePoint()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontSetupRenderEffectSlant()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGraphicsSetLineRGBA()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGraphicsSetDrawType()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontEndGraphics()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGraphicsSetupDrawContext()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontSetupRenderEffectWeight()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGlyphGetOutlineControlDistance()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGlyphGetVertexesGlyphSize()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGenerateCharGlyph()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontDeleteGlyph()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontExtend(u32 a1, u32 a2, u32 a3)
{
	cellFont.Todo("cellFontExtend(a1=0x%x, a2=0x%x, a3=0x%x)", a1, a2, a3);
	//In a test I did: a1=0xcfe00000, a2=0x0, a3=(pointer to something)
	if (a1 == 0xcfe00000)
		{
		if (a2 != 0 || a3 == 0)
		{
			//Something happens
		}
		if (vm::read32(a3) == 0)
		{
			//Something happens
		}
		//Something happens
	}
	//Something happens?
	return CELL_OK;
}

s32 cellFontRenderCharGlyphImageVertical()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontSetResolutionDpi()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontGetCharGlyphMetricsVertical()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

Module cellFont("cellFont", []()
{
	g_font = std::make_unique<CellFontInternal>();

	REG_FUNC(cellFont, cellFontInit);
	REG_FUNC(cellFont, cellFontSetFontsetOpenMode);
	REG_FUNC(cellFont, cellFontSetFontOpenMode);
	REG_FUNC(cellFont, cellFontCreateRenderer);
	REG_FUNC(cellFont, cellFontGetHorizontalLayout);
	REG_FUNC(cellFont, cellFontDestroyRenderer);
	REG_FUNC(cellFont, cellFontSetupRenderScalePixel);
	REG_FUNC(cellFont, cellFontOpenFontInstance);
	REG_FUNC(cellFont, cellFontSetScalePixel);
	REG_FUNC(cellFont, cellFontGetRenderCharGlyphMetrics);
	REG_FUNC(cellFont, cellFontEndLibrary);
	REG_FUNC(cellFont, cellFontBindRenderer);
	REG_FUNC(cellFont, cellFontEnd);
	REG_FUNC(cellFont, cellFontSetEffectSlant);
	REG_FUNC(cellFont, cellFontGetEffectSlant);
	REG_FUNC(cellFont, cellFontRenderCharGlyphImage);
	REG_FUNC(cellFont, cellFontRenderSurfaceInit);
	REG_FUNC(cellFont, cellFontGetFontIdCode);
	REG_FUNC(cellFont, cellFontOpenFontset);
	REG_FUNC(cellFont, cellFontCloseFont);
	REG_FUNC(cellFont, cellFontRenderSurfaceSetScissor);
	REG_FUNC(cellFont, cellFontGetCharGlyphMetrics);
	REG_FUNC(cellFont, cellFontInitializeWithRevision);
	REG_FUNC(cellFont, cellFontGraphicsSetFontRGBA);
	REG_FUNC(cellFont, cellFontOpenFontsetOnMemory);
	REG_FUNC(cellFont, cellFontOpenFontFile);
	REG_FUNC(cellFont, cellFontGraphicsSetScalePixel);
	REG_FUNC(cellFont, cellFontGraphicsGetScalePixel);
	REG_FUNC(cellFont, cellFontSetEffectWeight);
	REG_FUNC(cellFont, cellFontGlyphSetupVertexesGlyph);
	REG_FUNC(cellFont, cellFontGetVerticalLayout);
	REG_FUNC(cellFont, cellFontGetRenderCharGlyphMetricsVertical);
	REG_FUNC(cellFont, cellFontSetScalePoint);
	REG_FUNC(cellFont, cellFontSetupRenderEffectSlant);
	REG_FUNC(cellFont, cellFontGraphicsSetLineRGBA);
	REG_FUNC(cellFont, cellFontGraphicsSetDrawType);
	REG_FUNC(cellFont, cellFontEndGraphics);
	REG_FUNC(cellFont, cellFontGraphicsSetupDrawContext);
	REG_FUNC(cellFont, cellFontOpenFontMemory);
	REG_FUNC(cellFont, cellFontSetupRenderEffectWeight);
	REG_FUNC(cellFont, cellFontGlyphGetOutlineControlDistance);
	REG_FUNC(cellFont, cellFontGlyphGetVertexesGlyphSize);
	REG_FUNC(cellFont, cellFontGenerateCharGlyph);
	REG_FUNC(cellFont, cellFontDeleteGlyph);
	REG_FUNC(cellFont, cellFontExtend);
	REG_FUNC(cellFont, cellFontRenderCharGlyphImageVertical);
	REG_FUNC(cellFont, cellFontSetResolutionDpi);
	REG_FUNC(cellFont, cellFontGetCharGlyphMetricsVertical);
	REG_FUNC(cellFont, cellFontUnbindRenderer);
	REG_FUNC(cellFont, cellFontGetRevisionFlags);	
});
