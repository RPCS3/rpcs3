#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/Cell/PPUModule.h"

#include <stb_truetype.h>

#include "cellFont.h"

LOG_CHANNEL(cellFont);

// Functions
s32 cellFontInitializeWithRevision(u64 revisionFlags, vm::ptr<CellFontConfig> config)
{
	cellFont.warning("cellFontInitializeWithRevision(revisionFlags=0x%llx, config=*0x%x)", revisionFlags, config);

	if (config->fc_size < 24)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (config->flags != 0u)
	{
		cellFont.error("cellFontInitializeWithRevision: Unknown flags (0x%x)", config->flags);
	}

	return CELL_OK;
}

s32 cellFontGetRevisionFlags(vm::ptr<u64> revisionFlags)
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontEnd()
{
	cellFont.warning("cellFontEnd()");

	return CELL_OK;
}

s32 cellFontSetFontsetOpenMode(u32 openMode)
{
	cellFont.todo("cellFontSetFontsetOpenMode(openMode=0x%x)", openMode);
	return CELL_OK;
}

s32 cellFontOpenFontMemory(vm::ptr<CellFontLibrary> library, u32 fontAddr, u32 fontSize, u32 subNum, u32 uniqueId, vm::ptr<CellFont> font)
{
	cellFont.warning("cellFontOpenFontMemory(library=*0x%x, fontAddr=0x%x, fontSize=%d, subNum=%d, uniqueId=%d, font=*0x%x)", library, fontAddr, fontSize, subNum, uniqueId, font);

	font->stbfont = vm::_ptr<stbtt_fontinfo>(font.addr() + font.size()); // hack: use next bytes of the struct

	if (!stbtt_InitFont(font->stbfont, vm::_ptr<unsigned char>(fontAddr), 0))
		return CELL_FONT_ERROR_FONT_OPEN_FAILED;

	font->renderer_addr = 0;
	font->fontdata_addr = fontAddr;
	font->origin = CELL_FONT_OPEN_MEMORY;

	return CELL_OK;
}

s32 cellFontOpenFontFile(vm::ptr<CellFontLibrary> library, vm::cptr<char> fontPath, u32 subNum, s32 uniqueId, vm::ptr<CellFont> font)
{
	cellFont.warning("cellFontOpenFontFile(library=*0x%x, fontPath=%s, subNum=%d, uniqueId=%d, font=*0x%x)", library, fontPath, subNum, uniqueId, font);

	fs::file f(vfs::get(fontPath.get_ptr()));
	if (!f)
	{
		return CELL_FONT_ERROR_FONT_OPEN_FAILED;
	}

	u32 fileSize = ::size32(f);
	u32 bufferAddr = vm::alloc(fileSize, vm::main); // Freed in cellFontCloseFont
	f.read(vm::base(bufferAddr), fileSize);
	s32 ret = cellFontOpenFontMemory(library, bufferAddr, fileSize, subNum, uniqueId, font);
	font->origin = CELL_FONT_OPEN_FONT_FILE;

	return ret;
}

s32 cellFontOpenFontset(ppu_thread& ppu, vm::ptr<CellFontLibrary> library, vm::ptr<CellFontType> fontType, vm::ptr<CellFont> font)
{
	cellFont.warning("cellFontOpenFontset(library=*0x%x, fontType=*0x%x, font=*0x%x)", library, fontType, font);

	if (fontType->map != CELL_FONT_MAP_UNICODE)
	{
		cellFont.warning("cellFontOpenFontset: Only Unicode is supported");
	}

	std::string file;
	switch (fontType->type)
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
		cellFont.warning("cellFontOpenFontset: fontType->type = %d not supported yet. RD-R-LATIN.TTF will be used instead.", fontType->type);
		file = "/dev_flash/data/font/SCE-PS3-RD-R-LATIN.TTF";
		break;

	default:
		cellFont.warning("cellFontOpenFontset: fontType->type = %d not supported.", fontType->type);
		return CELL_FONT_ERROR_NO_SUPPORT_FONTSET;
	}

	s32 ret = cellFontOpenFontFile(library, vm::make_str(file), 0, 0, font); //TODO: Find the correct values of subNum, uniqueId
	font->origin = CELL_FONT_OPEN_FONTSET;

	return ret;
}

s32 cellFontOpenFontInstance(vm::ptr<CellFont> openedFont, vm::ptr<CellFont> font)
{
	cellFont.warning("cellFontOpenFontInstance(openedFont=*0x%x, font=*0x%x)", openedFont, font);

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
	cellFont.todo("cellFontSetFontOpenMode(openMode=0x%x)", openMode);
	return CELL_OK;
}

s32 cellFontCreateRenderer(vm::ptr<CellFontLibrary> library, vm::ptr<CellFontRendererConfig> config, vm::ptr<CellFontRenderer> Renderer)
{
	cellFont.todo("cellFontCreateRenderer(library=*0x%x, config=*0x%x, Renderer=*0x%x)", library, config, Renderer);

	//Write data in Renderer

	return CELL_OK;
}

void cellFontRenderSurfaceInit(vm::ptr<CellFontRenderSurface> surface, vm::ptr<void> buffer, s32 bufferWidthByte, s32 pixelSizeByte, s32 w, s32 h)
{
	cellFont.warning("cellFontRenderSurfaceInit(surface=*0x%x, buffer=*0x%x, bufferWidthByte=%d, pixelSizeByte=%d, w=%d, h=%d)", surface, buffer, bufferWidthByte, pixelSizeByte, w, h);

	surface->buffer         = buffer;
	surface->widthByte      = bufferWidthByte;
	surface->pixelSizeByte  = pixelSizeByte;
	surface->width          = w;
	surface->height         = h;
}

void cellFontRenderSurfaceSetScissor(vm::ptr<CellFontRenderSurface> surface, s32 x0, s32 y0, s32 w, s32 h)
{
	cellFont.warning("cellFontRenderSurfaceSetScissor(surface=*0x%x, x0=%d, y0=%d, w=%d, h=%d)", surface, x0, y0, w, h);

	surface->sc_x0 = x0;
	surface->sc_y0 = y0;
	surface->sc_x1 = w;
	surface->sc_y1 = h;
}

s32 cellFontSetScalePixel(vm::ptr<CellFont> font, float w, float h)
{
	cellFont.trace("cellFontSetScalePixel(font=*0x%x, w=%f, h=%f)", font, w, h);

	font->scale_x = w;
	font->scale_y = h;

	return CELL_OK;
}

s32 cellFontGetHorizontalLayout(vm::ptr<CellFont> font, vm::ptr<CellFontHorizontalLayout> layout)
{
	cellFont.trace("cellFontGetHorizontalLayout(font=*0x%x, layout=*0x%x)", font, layout);

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
	cellFont.warning("cellFontBindRenderer(font=*0x%x, renderer=*0x%x)", font, renderer);

	if (font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_ALREADY_BIND;
	}

	font->renderer_addr = renderer.addr();

	return CELL_OK;
}

s32 cellFontUnbindRenderer(vm::ptr<CellFont> font)
{
	cellFont.warning("cellFontBindRenderer(font=*0x%x)", font);

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
	cellFont.todo("cellFontSetupRenderScalePixel(font=*0x%x, w=%f, h=%f)", font, w, h);

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO: ?
	return CELL_OK;
}

s32 cellFontGetRenderCharGlyphMetrics(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont.todo("cellFontGetRenderCharGlyphMetrics(font=*0x%x, code=0x%x, metrics=*0x%x)", font, code, metrics);

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO: ?
	return CELL_OK;
}

s32 cellFontRenderCharGlyphImage(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontRenderSurface> surface, float x, float y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont.notice("cellFontRenderCharGlyphImage(font=*0x%x, code=0x%x, surface=*0x%x, x=%f, y=%f, metrics=*0x%x, trans=*0x%x)", font, code, surface, x, y, metrics, transInfo);

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
	baseLineY = static_cast<int>(ascent * scale); // ???

	// Move the rendered character to the surface
	unsigned char* buffer = vm::_ptr<unsigned char>(surface->buffer.addr());
	for (u32 ypos = 0; ypos < static_cast<u32>(height); ypos++)
	{
		if (static_cast<u32>(y) + ypos + yoff + baseLineY >= static_cast<u32>(surface->height))
			break;

		for (u32 xpos = 0; xpos < static_cast<u32>(width); xpos++)
		{
			if (static_cast<u32>(x) + xpos >= static_cast<u32>(surface->width))
				break;

			// TODO: There are some oddities in the position of the character in the final buffer
			buffer[(static_cast<s32>(y) + ypos + yoff + baseLineY) * surface->width + static_cast<s32>(x) + xpos] = box[ypos * width + xpos];
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
	cellFont.trace("cellFontSetEffectSlant(font=*0x%x, slantParam=%f)", font, slantParam);

	if (slantParam < -1.0 || slantParam > 1.0)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	font->slant = slantParam;

	return CELL_OK;
}

s32 cellFontGetEffectSlant(vm::ptr<CellFont> font, vm::ptr<float> slantParam)
{
	cellFont.warning("cellFontSetEffectSlant(font=*0x%x, slantParam=*0x%x)", font, slantParam);

	*slantParam = font->slant;

	return CELL_OK;
}

s32 cellFontGetFontIdCode(vm::ptr<CellFont> font, u32 code, vm::ptr<u32> fontId, vm::ptr<u32> fontCode)
{
	cellFont.todo("cellFontGetFontIdCode(font=*0x%x, code=%d, fontId=*0x%x, fontCode=*0x%x)", font, code, fontId, fontCode);

	// TODO: ?
	return CELL_OK;
}

s32 cellFontCloseFont(vm::ptr<CellFont> font)
{
	cellFont.warning("cellFontCloseFont(font=*0x%x)", font);

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
	cellFont.warning("cellFontGetCharGlyphMetrics(font=*0x%x, code=0x%x, metrics=*0x%x)", font, code, metrics);

	s32 x0, y0, x1, y1;
	s32 advanceWidth, leftSideBearing;
	float scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	stbtt_GetCodepointBox(font->stbfont, code, &x0, &y0, &x1, &y1);
	stbtt_GetCodepointHMetrics(font->stbfont, code, &advanceWidth, &leftSideBearing);

	// TODO: Add the rest of the information
	metrics->width = (x1-x0) * scale;
	metrics->height = (y1-y0) * scale;
	metrics->h_bearingX = leftSideBearing * scale;
	metrics->h_bearingY = 0.f;
	metrics->h_advance = advanceWidth * scale;
	metrics->v_bearingX = 0.f;
	metrics->v_bearingY = 0.f;
	metrics->v_advance = 0.f;

	return CELL_OK;
}

s32 cellFontGraphicsSetFontRGBA()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_OK;
}

s32 cellFontOpenFontsetOnMemory(ppu_thread& ppu, vm::ptr<CellFontLibrary> library, vm::ptr<CellFontType> fontType, vm::ptr<CellFont> font)
{
	cellFont.todo("cellFontOpenFontsetOnMemory(library=*0x%x, fontType=*0x%x, font=*0x%x)", library, fontType, font);

	if (fontType->map != CELL_FONT_MAP_UNICODE)
	{
		cellFont.warning("cellFontOpenFontsetOnMemory: Only Unicode is supported");
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
	cellFont.todo("cellFontExtend(a1=0x%x, a2=0x%x, a3=0x%x)", a1, a2, a3);
	//In a test I did: a1=0xcfe00000, a2=0x0, a3=(pointer to something)
	if (a1 == 0xcfe00000)
		{
		if (a2 != 0 || a3 == 0)
		{
			//Something happens
		}
		if (vm::read32(a3) == 0u)
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

s32 cellFontGetRenderEffectWeight()
{
	cellFont.todo("cellFontGetRenderEffectWeight()");
	return CELL_OK;
}

s32 cellFontGraphicsGetDrawType()
{
	cellFont.todo("cellFontGraphicsGetDrawType()");
	return CELL_OK;
}

s32 cellFontGetKerning()
{
	cellFont.todo("cellFontGetKerning()");
	return CELL_OK;
}

s32 cellFontGetRenderScaledKerning()
{
	cellFont.todo("cellFontGetRenderScaledKerning()");
	return CELL_OK;
}

s32 cellFontGetRenderScalePixel()
{
	cellFont.todo("cellFontGetRenderScalePixel()");
	return CELL_OK;
}

s32 cellFontGlyphGetScalePixel()
{
	cellFont.todo("cellFontGlyphGetScalePixel()");
	return CELL_OK;
}

s32 cellFontGlyphGetHorizontalShift()
{
	cellFont.todo("cellFontGlyphGetHorizontalShift()");
	return CELL_OK;
}

s32 cellFontRenderCharGlyphImageHorizontal()
{
	cellFont.todo("cellFontRenderCharGlyphImageHorizontal()");
	return CELL_OK;
}

s32 cellFontGetEffectWeight()
{
	cellFont.todo("cellFontGetEffectWeight()");
	return CELL_OK;
}

s32 cellFontGetScalePixel()
{
	cellFont.todo("cellFontGetScalePixel()");
	return CELL_OK;
}

s32 cellFontClearFileCache()
{
	cellFont.todo("cellFontClearFileCache()");
	return CELL_OK;
}

s32 cellFontAdjustFontScaling()
{
	cellFont.todo("cellFontAdjustFontScaling()");
	return CELL_OK;
}

s32 cellFontSetupRenderScalePoint()
{
	cellFont.todo("cellFontSetupRenderScalePoint()");
	return CELL_OK;
}

s32 cellFontGlyphGetVerticalShift()
{
	cellFont.todo("cellFontGlyphGetVerticalShift()");
	return CELL_OK;
}

s32 cellFontGetGlyphExpandBufferInfo()
{
	cellFont.todo("cellFontGetGlyphExpandBufferInfo()");
	return CELL_OK;
}

s32 cellFontGetLibrary()
{
	cellFont.todo("cellFontGetLibrary()");
	return CELL_OK;
}

s32 cellFontVertexesGlyphRelocate()
{
	cellFont.todo("cellFontVertexesGlyphRelocate()");
	return CELL_OK;
}

s32 cellFontGetInitializedRevisionFlags()
{
	cellFont.todo("cellFontGetInitializedRevisionFlags()");
	return CELL_OK;
}

s32 cellFontGetResolutionDpi()
{
	cellFont.todo("cellFontGetResolutionDpi()");
	return CELL_OK;
}

s32 cellFontGlyphRenderImageVertical()
{
	cellFont.todo("cellFontGlyphRenderImageVertical()");
	return CELL_OK;
}

s32 cellFontGlyphRenderImageHorizontal()
{
	cellFont.todo("cellFontGlyphRenderImageHorizontal()");
	return CELL_OK;
}

s32 cellFontAdjustGlyphExpandBuffer()
{
	cellFont.todo("cellFontAdjustGlyphExpandBuffer()");
	return CELL_OK;
}

s32 cellFontGetRenderScalePoint()
{
	cellFont.todo("cellFontGetRenderScalePoint()");
	return CELL_OK;
}

s32 cellFontGraphicsGetFontRGBA()
{
	cellFont.todo("cellFontGraphicsGetFontRGBA()");
	return CELL_OK;
}

s32 cellFontGlyphGetOutlineVertexes()
{
	cellFont.todo("cellFontGlyphGetOutlineVertexes()");
	return CELL_OK;
}

s32 cellFontDelete()
{
	cellFont.todo("cellFontDelete()");
	return CELL_OK;
}

s32 cellFontPatchWorks()
{
	cellFont.todo("cellFontPatchWorks()");
	return CELL_OK;
}

s32 cellFontGlyphRenderImage()
{
	cellFont.todo("cellFontGlyphRenderImage()");
	return CELL_OK;
}

s32 cellFontGetBindingRenderer()
{
	cellFont.todo("cellFontGetBindingRenderer()");
	return CELL_OK;
}

s32 cellFontGenerateCharGlyphVertical()
{
	cellFont.todo("cellFontGenerateCharGlyphVertical()");
	return CELL_OK;
}

s32 cellFontGetRenderEffectSlant()
{
	cellFont.todo("cellFontGetRenderEffectSlant()");
	return CELL_OK;
}

s32 cellFontGetScalePoint()
{
	cellFont.todo("cellFontGetScalePoint()");
	return CELL_OK;
}

s32 cellFontGraphicsGetLineRGBA()
{
	cellFont.todo("cellFontGraphicsGetLineRGBA()");
	return CELL_OK;
}

s32 cellFontControl()
{
	cellFont.todo("cellFontControl()");
	return CELL_OK;
}

s32 cellFontStatic()
{
	cellFont.todo("cellFontStatic()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellFont)("cellFont", []()
{

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
	REG_FUNC(cellFont, cellFontGetRenderEffectWeight);
	REG_FUNC(cellFont, cellFontGraphicsGetDrawType);
	REG_FUNC(cellFont, cellFontGetKerning);
	REG_FUNC(cellFont, cellFontGetRenderScaledKerning);
	REG_FUNC(cellFont, cellFontGetRenderScalePixel);
	REG_FUNC(cellFont, cellFontGlyphGetScalePixel);
	REG_FUNC(cellFont, cellFontGlyphGetHorizontalShift);
	REG_FUNC(cellFont, cellFontRenderCharGlyphImageHorizontal);
	REG_FUNC(cellFont, cellFontGetEffectWeight);
	REG_FUNC(cellFont, cellFontGetScalePixel);
	REG_FUNC(cellFont, cellFontClearFileCache);
	REG_FUNC(cellFont, cellFontAdjustFontScaling);
	REG_FUNC(cellFont, cellFontSetupRenderScalePoint);
	REG_FUNC(cellFont, cellFontGlyphGetVerticalShift);
	REG_FUNC(cellFont, cellFontGetGlyphExpandBufferInfo);
	REG_FUNC(cellFont, cellFontGetLibrary);
	REG_FUNC(cellFont, cellFontVertexesGlyphRelocate);
	REG_FUNC(cellFont, cellFontGetInitializedRevisionFlags);
	REG_FUNC(cellFont, cellFontGetResolutionDpi);
	REG_FUNC(cellFont, cellFontGlyphRenderImageVertical);
	REG_FUNC(cellFont, cellFontGlyphRenderImageHorizontal);
	REG_FUNC(cellFont, cellFontAdjustGlyphExpandBuffer);
	REG_FUNC(cellFont, cellFontGetRenderScalePoint);
	REG_FUNC(cellFont, cellFontGraphicsGetFontRGBA);
	REG_FUNC(cellFont, cellFontGlyphGetOutlineVertexes);
	REG_FUNC(cellFont, cellFontDelete);
	REG_FUNC(cellFont, cellFontPatchWorks);
	REG_FUNC(cellFont, cellFontGlyphRenderImage);
	REG_FUNC(cellFont, cellFontGetBindingRenderer);
	REG_FUNC(cellFont, cellFontGenerateCharGlyphVertical);
	REG_FUNC(cellFont, cellFontGetRenderEffectSlant);
	REG_FUNC(cellFont, cellFontGetScalePoint);
	REG_FUNC(cellFont, cellFontGraphicsGetLineRGBA);
	REG_FUNC(cellFont, cellFontControl);
	REG_FUNC(cellFont, cellFontStatic);
});
