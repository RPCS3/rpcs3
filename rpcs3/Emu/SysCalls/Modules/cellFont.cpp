#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "stblib/stb_truetype.h"
#include "Emu/FS/vfsFile.h"
#include "cellFont.h"

Module *cellFont = nullptr;

CCellFontInternal* s_fontInternalInstance = nullptr;

// Functions
int cellFontInitializeWithRevision(u64 revisionFlags, vm::ptr<CellFontConfig> config)
{
	cellFont->Warning("cellFontInitializeWithRevision(revisionFlags=0x%llx, config=0x%x)", revisionFlags, config.addr());
	
	if (s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_ALREADY_INITIALIZED;
	if (config->FileCache.size < 24)
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	if (config->flags != 0)
		cellFont->Warning("cellFontInitializeWithRevision: Unknown flags (0x%x)", config->flags);

	s_fontInternalInstance->m_buffer_addr = config->FileCache.buffer_addr;
	s_fontInternalInstance->m_buffer_size = config->FileCache.size;
	s_fontInternalInstance->m_userFontEntrys_addr = config->userFontEntrys_addr;
	s_fontInternalInstance->m_userFontEntryMax    = config->userFontEntryMax;
	s_fontInternalInstance->m_bInitialized = true;
	return CELL_FONT_OK;
}

int cellFontGetRevisionFlags(vm::ptr<be_t<u64>> revisionFlags)
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontInit(vm::ptr<CellFontConfig> config)
{
	cellFont->Log("cellFontInit(config=0x%x)", config.addr());

	vm::var<be_t<u64>> revisionFlags;
	revisionFlags.value() = 0;
	cellFontGetRevisionFlags(revisionFlags);
	return cellFontInitializeWithRevision(revisionFlags.value(), config);
}

int cellFontEnd()
{
	cellFont->Log("cellFontEnd()");

	if (!s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_UNINITIALIZED;

	s_fontInternalInstance->m_bInitialized = false;
	return CELL_FONT_OK;
}

s32 cellFontSetFontsetOpenMode(u32 openMode)
{
	cellFont->Log("cellFontSetFontsetOpenMode(openMode=0x%x)", openMode);
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontOpenFontMemory(vm::ptr<CellFontLibrary> library, u32 fontAddr, u32 fontSize, u32 subNum, u32 uniqueId, vm::ptr<CellFont> font)
{
	cellFont->Warning("cellFontOpenFontMemory(library_addr=0x%x, fontAddr=0x%x, fontSize=%d, subNum=%d, uniqueId=%d, font_addr=0x%x)",
		library.addr(), fontAddr, fontSize, subNum, uniqueId, font.addr());

	if (!s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_UNINITIALIZED;

	font->stbfont = (stbtt_fontinfo*)((u8*)&(font->stbfont) + sizeof(void*)); // hack: use next bytes of the struct

	if (!stbtt_InitFont(font->stbfont, vm::get_ptr<unsigned char>(fontAddr), 0))
		return CELL_FONT_ERROR_FONT_OPEN_FAILED;

	font->renderer_addr = 0;
	font->fontdata_addr = fontAddr;
	font->origin = CELL_FONT_OPEN_MEMORY;
	return CELL_FONT_OK;
}

int cellFontOpenFontFile(vm::ptr<CellFontLibrary> library, vm::ptr<const char> fontPath, u32 subNum, s32 uniqueId, vm::ptr<CellFont> font)
{
	std::string fp(fontPath.get_ptr());
	cellFont->Warning("cellFontOpenFontFile(library_addr=0x%x, fontPath=\"%s\", subNum=%d, uniqueId=%d, font_addr=0x%x)",
		library.addr(), fp.c_str(), subNum, uniqueId, font.addr());

	vfsFile f(fp);
	if (!f.IsOpened())
		return CELL_FONT_ERROR_FONT_OPEN_FAILED;

	u32 fileSize = (u32)f.GetSize();
	u32 bufferAddr = (u32)Memory.Alloc(fileSize, 1); // Freed in cellFontCloseFont
	f.Read(vm::get_ptr<void>(bufferAddr), fileSize);
	int ret = cellFontOpenFontMemory(library, bufferAddr, fileSize, subNum, uniqueId, font);
	font->origin = CELL_FONT_OPEN_FONT_FILE;
	return ret;
}

int cellFontOpenFontset(vm::ptr<CellFontLibrary> library, vm::ptr<CellFontType> fontType, vm::ptr<CellFont> font)
{
	cellFont->Log("cellFontOpenFontset(library_addr=0x%x, fontType_addr=0x%x, font_addr=0x%x)",
		library.addr(), fontType.addr(), font.addr());

	if (!s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_UNINITIALIZED;
	if (fontType->map != CELL_FONT_MAP_UNICODE)
		cellFont->Warning("cellFontOpenFontset: Only Unicode is supported");
	
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
		cellFont->Warning("cellFontOpenFontset: fontType->type = %d not supported yet. RD-R-LATIN.TTF will be used instead.", fontType->type);
		file = "/dev_flash/data/font/SCE-PS3-RD-R-LATIN.TTF";
		break;

	default:
		cellFont->Warning("cellFontOpenFontset: fontType->type = %d not supported.", fontType->type);
		return CELL_FONT_ERROR_NO_SUPPORT_FONTSET;
	}

	vm::var<char> f((u32)file.length() + 1, 1);
	memcpy(f.get_ptr(), file.c_str(), file.size() + 1);
	int ret = cellFontOpenFontFile(library, f, 0, 0, font); //TODO: Find the correct values of subNum, uniqueId
	font->origin = CELL_FONT_OPEN_FONTSET;
	return ret;
}

int cellFontOpenFontInstance(vm::ptr<CellFont> openedFont, vm::ptr<CellFont> font)
{
	cellFont->Warning("cellFontOpenFontInstance(openedFont=0x%x, font=0x%x)", openedFont.addr(), font.addr());

	font->renderer_addr = openedFont->renderer_addr;
	font->scale_x = openedFont->scale_x;
	font->scale_y = openedFont->scale_y;
	font->slant = openedFont->slant;
	font->stbfont = openedFont->stbfont;
	font->origin = CELL_FONT_OPEN_FONT_INSTANCE;

	return CELL_FONT_OK;
}

s32 cellFontSetFontOpenMode(u32 openMode)
{
	cellFont->Log("cellFontSetFontOpenMode(openMode=0x%x)", openMode);
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontCreateRenderer(vm::ptr<CellFontLibrary> library, vm::ptr<CellFontRendererConfig> config, vm::ptr<CellFontRenderer> Renderer)
{
	cellFont->Warning("cellFontCreateRenderer(library_addr=0x%x, config_addr=0x%x, Renderer_addr=0x%x)",
		library.addr(), config.addr(), Renderer.addr());

	if (!s_fontInternalInstance->m_bInitialized)
		return CELL_FONT_ERROR_UNINITIALIZED;
	
	//Write data in Renderer

	return CELL_FONT_OK;
}

void cellFontRenderSurfaceInit(vm::ptr<CellFontRenderSurface> surface, u32 buffer_addr, s32 bufferWidthByte, s32 pixelSizeByte, s32 w, s32 h)
{
	cellFont->Warning("cellFontRenderSurfaceInit(surface_addr=0x%x, buffer_addr=0x%x, bufferWidthByte=%d, pixelSizeByte=%d, w=%d, h=%d)",
		surface.addr(), buffer_addr, bufferWidthByte, pixelSizeByte, w, h);

	surface->buffer_addr	= buffer_addr;
	surface->widthByte		= bufferWidthByte;
	surface->pixelSizeByte	= pixelSizeByte;
	surface->width			= w;
	surface->height			= h;

	if (!buffer_addr)
		surface->buffer_addr = (u32)Memory.Alloc(bufferWidthByte * h, 1); // TODO: Huge memory leak
}

void cellFontRenderSurfaceSetScissor(vm::ptr<CellFontRenderSurface> surface, s32 x0, s32 y0, s32 w, s32 h)
{
	cellFont->Warning("cellFontRenderSurfaceSetScissor(surface_addr=0x%x, x0=%d, y0=%d, w=%d, h=%d)",
		surface.addr(), x0, y0, w, h);

	surface->Scissor.x0 = x0;
	surface->Scissor.y0 = y0;
	surface->Scissor.x1 = w;
	surface->Scissor.y1 = h;
}

int cellFontSetScalePixel(vm::ptr<CellFont> font, float w, float h)
{
	cellFont->Log("cellFontSetScalePixel(font_addr=0x%x, w=%f, h=%f)", font.addr(), w, h);

	font->scale_x = w;
	font->scale_y = h;
	return CELL_FONT_OK;
}

int cellFontGetHorizontalLayout(vm::ptr<CellFont> font, vm::ptr<CellFontHorizontalLayout> layout)
{
	cellFont->Log("cellFontGetHorizontalLayout(font_addr=0x%x, layout_addr=0x%x)",
		font.addr(), layout.addr());

	int ascent, descent, lineGap;
	float scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	stbtt_GetFontVMetrics(font->stbfont, &ascent, &descent, &lineGap);

	layout->baseLineY = ascent * scale;
	layout->lineHeight = (ascent-descent+lineGap) * scale;
	layout->effectHeight = lineGap * scale;
	return CELL_FONT_OK;
}

int cellFontBindRenderer(vm::ptr<CellFont> font, vm::ptr<CellFontRenderer> renderer)
{
	cellFont->Warning("cellFontBindRenderer(font_addr=0x%x, renderer_addr=0x%x)",
		font.addr(), renderer.addr());

	if (font->renderer_addr)
		return CELL_FONT_ERROR_RENDERER_ALREADY_BIND;

	font->renderer_addr = renderer.addr();
	return CELL_FONT_OK;
}

int cellFontUnbindRenderer(vm::ptr<CellFont> font)
{
	cellFont->Warning("cellFontBindRenderer(font_addr=0x%x)", font.addr());
	
	if (!font->renderer_addr)
		return CELL_FONT_ERROR_RENDERER_UNBIND;

	font->renderer_addr = 0;
	return CELL_FONT_OK;
}

int cellFontDestroyRenderer()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetupRenderScalePixel(vm::ptr<CellFont> font, float w, float h)
{
	cellFont->Log("cellFontSetupRenderScalePixel(font_addr=0x%x, w=%f, h=%f)", font.addr(), w, h);

	if (!font->renderer_addr)
		return CELL_FONT_ERROR_RENDERER_UNBIND;

	// TODO: ?
	return CELL_FONT_OK;
}

int cellFontGetRenderCharGlyphMetrics(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont->Log("cellFontGetRenderCharGlyphMetrics(font_addr=0x%x, code=0x%x, metrics_addr=0x%x)",
		font.addr(), code, metrics.addr());

	if (!font->renderer_addr)
		return CELL_FONT_ERROR_RENDERER_UNBIND;

	// TODO: ?
	return CELL_FONT_OK;
}

int cellFontRenderCharGlyphImage(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontRenderSurface> surface, float x, float y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont->Log("cellFontRenderCharGlyphImage(font_addr=0x%x, code=0x%x, surface_addr=0x%x, x=%f, y=%f, metrics_addr=0x%x, trans_addr=0x%x)",
		font.addr(), code, surface.addr(), x, y, metrics.addr(), transInfo.addr());

	if (!font->renderer_addr)
		return CELL_FONT_ERROR_RENDERER_UNBIND;

	// Render the character
	int width, height, xoff, yoff;
	float scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	unsigned char* box = stbtt_GetCodepointBitmap(font->stbfont, scale, scale, code, &width, &height, &xoff, &yoff);
	if (!box) return CELL_OK;

	// Get the baseLineY value
	int baseLineY;
	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(font->stbfont, &ascent, &descent, &lineGap);
	baseLineY = (int)((float)ascent * scale); // ???

	// Move the rendered character to the surface
	unsigned char* buffer = vm::get_ptr<unsigned char>(surface->buffer_addr);
	for (u32 ypos = 0; ypos < (u32)height; ypos++){
		if ((u32)y + ypos + yoff + baseLineY >= surface->height)
			break;

		for (u32 xpos = 0; xpos < (u32)width; xpos++){
			if ((u32)x + xpos >= surface->width)
				break;

			// TODO: There are some oddities in the position of the character in the final buffer
			buffer[((int)y + ypos + yoff + baseLineY)*surface->width + (int)x+xpos] = box[ypos*width + xpos];
		}
	}
	stbtt_FreeBitmap(box, 0);
	return CELL_FONT_OK;
}

int cellFontEndLibrary()
{
	UNIMPLEMENTED_FUNC(cellFont);
	return CELL_FONT_OK;
}

int cellFontSetEffectSlant(vm::ptr<CellFont> font, float slantParam)
{
	cellFont->Log("cellFontSetEffectSlant(font_addr=0x%x, slantParam=%f)", font.addr(), slantParam);

	if (slantParam < -1.0 || slantParam > 1.0)
		return CELL_FONT_ERROR_INVALID_PARAMETER;

	font->slant = slantParam;
	return CELL_FONT_OK;
}

int cellFontGetEffectSlant(vm::ptr<CellFont> font, vm::ptr<be_t<float>> slantParam)
{
	cellFont->Warning("cellFontSetEffectSlant(font_addr=0x%x, slantParam_addr=0x%x)", font.addr(), slantParam.addr());

	*slantParam = font->slant;
	return CELL_FONT_OK;
}

int cellFontGetFontIdCode(vm::ptr<CellFont> font, u32 code, vm::ptr<be_t<u32>> fontId, vm::ptr<be_t<u32>> fontCode)
{
	cellFont->Todo("cellFontGetFontIdCode(font_addr=0x%x, code=0x%x, fontId_addr=0x%x, fontCode_addr=0x%x",
		font.addr(), code, fontId.addr(), fontCode.addr());

	// TODO: ?
	return CELL_FONT_OK;
}

int cellFontCloseFont(vm::ptr<CellFont> font)
{
	cellFont->Warning("cellFontCloseFont(font_addr=0x%x)", font.addr());

	if (font->origin == CELL_FONT_OPEN_FONTSET ||
		font->origin == CELL_FONT_OPEN_FONT_FILE ||
		font->origin == CELL_FONT_OPEN_MEMORY)
		Memory.Free(font->fontdata_addr);

	return CELL_FONT_OK;
}

int cellFontGetCharGlyphMetrics(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont->Log("cellFontGetCharGlyphMetrics(font_addr=0x%x, code=0x%x, metrics_addr=0x%x",
		font.addr(), code, metrics.addr());

	int x0, y0, x1, y1;
	int advanceWidth, leftSideBearing;
	float scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	stbtt_GetCodepointBox(font->stbfont, code, &x0, &y0, &x1, &y1);
	stbtt_GetCodepointHMetrics(font->stbfont, code, &advanceWidth, &leftSideBearing);
	
	// TODO: Add the rest of the information
	metrics->width = (x1-x0) * scale;
	metrics->height = (y1-y0) * scale;
	metrics->Horizontal.bearingX = (float)leftSideBearing * scale;
	metrics->Horizontal.bearingY = 0;
	metrics->Horizontal.advance = (float)advanceWidth * scale;
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
	cellFont->Warning("cellFontExtend(a1=0x%x, a2=0x%x, a3=0x%x)", a1, a2, a3);
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

void cellFont_init(Module *pxThis)
{
	cellFont = pxThis;

	cellFont->AddFunc(0x25c107e6, cellFontInit);
	cellFont->AddFunc(0x6bf6f832, cellFontSetFontsetOpenMode);
	cellFont->AddFunc(0x6cfada83, cellFontSetFontOpenMode);
	cellFont->AddFunc(0x042e74e3, cellFontCreateRenderer);
	cellFont->AddFunc(0x1387c45c, cellFontGetHorizontalLayout);
	cellFont->AddFunc(0x21ebb248, cellFontDestroyRenderer);
	cellFont->AddFunc(0x227e1e3c, cellFontSetupRenderScalePixel);
	cellFont->AddFunc(0x29329541, cellFontOpenFontInstance);
	cellFont->AddFunc(0x297f0e93, cellFontSetScalePixel);
	cellFont->AddFunc(0x2da9fd9d, cellFontGetRenderCharGlyphMetrics);
	cellFont->AddFunc(0x40d40544, cellFontEndLibrary);
	cellFont->AddFunc(0x66a23100, cellFontBindRenderer);
	cellFont->AddFunc(0x7ab47f7e, cellFontEnd);
	cellFont->AddFunc(0x8657c8f5, cellFontSetEffectSlant);
	cellFont->AddFunc(0xe16e679a, cellFontGetEffectSlant);
	cellFont->AddFunc(0x88be4799, cellFontRenderCharGlyphImage);
	cellFont->AddFunc(0x90b9465e, cellFontRenderSurfaceInit);
	cellFont->AddFunc(0x98ac5524, cellFontGetFontIdCode);
	cellFont->AddFunc(0xa885cc9b, cellFontOpenFontset);
	cellFont->AddFunc(0xb276f1f6, cellFontCloseFont);
	cellFont->AddFunc(0xb422b005, cellFontRenderSurfaceSetScissor);
	cellFont->AddFunc(0xd8eaee9f, cellFontGetCharGlyphMetrics);
	cellFont->AddFunc(0xf03dcc29, cellFontInitializeWithRevision);
	cellFont->AddFunc(0x061049ad, cellFontGraphicsSetFontRGBA);
	cellFont->AddFunc(0x073fa321, cellFontOpenFontsetOnMemory);
	cellFont->AddFunc(0x0a7306a4, cellFontOpenFontFile);
	cellFont->AddFunc(0x16322df1, cellFontGraphicsSetScalePixel);
	cellFont->AddFunc(0x2388186c, cellFontGraphicsGetScalePixel);
	cellFont->AddFunc(0x25253fe4, cellFontSetEffectWeight);
	cellFont->AddFunc(0x53f529fe, cellFontGlyphSetupVertexesGlyph);
	cellFont->AddFunc(0x698897f8, cellFontGetVerticalLayout);
	cellFont->AddFunc(0x700e6223, cellFontGetRenderCharGlyphMetricsVertical);
	cellFont->AddFunc(0x70f3e728, cellFontSetScalePoint);
	cellFont->AddFunc(0x78d05e08, cellFontSetupRenderEffectSlant);
	cellFont->AddFunc(0x7c83bc15, cellFontGraphicsSetLineRGBA);
	cellFont->AddFunc(0x87bd650f, cellFontGraphicsSetDrawType);
	cellFont->AddFunc(0x8a35c887, cellFontEndGraphics);
	cellFont->AddFunc(0x970d4c22, cellFontGraphicsSetupDrawContext);
	cellFont->AddFunc(0x9e19072b, cellFontOpenFontMemory);
	cellFont->AddFunc(0xa6dc25d1, cellFontSetupRenderEffectWeight);
	cellFont->AddFunc(0xa8fae920, cellFontGlyphGetOutlineControlDistance);
	cellFont->AddFunc(0xb4d112af, cellFontGlyphGetVertexesGlyphSize);
	cellFont->AddFunc(0xc17259de, cellFontGenerateCharGlyph);
	cellFont->AddFunc(0xd62f5d76, cellFontDeleteGlyph);
	cellFont->AddFunc(0xdee0836c, cellFontExtend);
	cellFont->AddFunc(0xe857a0ca, cellFontRenderCharGlyphImageVertical);
	cellFont->AddFunc(0xfb3341ba, cellFontSetResolutionDpi);
	cellFont->AddFunc(0xfe9a6dd7, cellFontGetCharGlyphMetricsVertical);
	cellFont->AddFunc(0xf16379fa, cellFontUnbindRenderer);
	cellFont->AddFunc(0xb015a84e, cellFontGetRevisionFlags);
}

void cellFont_load()
{
	s_fontInternalInstance = new CCellFontInternal();
}

void cellFont_unload()
{
	// s_fontInternalInstance->m_bInitialized = false;
	// s_fontInternalInstance->m_bFontGcmInitialized = false;
	delete s_fontInternalInstance;
}
