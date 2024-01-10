#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/Cell/PPUModule.h"

#include <stb_truetype.h>

#include "cellFont.h"

LOG_CHANNEL(cellFont);

template <>
void fmt_class_string<CellFontError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_FONT_ERROR_FATAL);
			STR_CASE(CELL_FONT_ERROR_INVALID_PARAMETER);
			STR_CASE(CELL_FONT_ERROR_UNINITIALIZED);
			STR_CASE(CELL_FONT_ERROR_INITIALIZE_FAILED);
			STR_CASE(CELL_FONT_ERROR_INVALID_CACHE_BUFFER);
			STR_CASE(CELL_FONT_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_FONT_ERROR_ALLOCATION_FAILED);
			STR_CASE(CELL_FONT_ERROR_NO_SUPPORT_FONTSET);
			STR_CASE(CELL_FONT_ERROR_OPEN_FAILED);
			STR_CASE(CELL_FONT_ERROR_READ_FAILED);
			STR_CASE(CELL_FONT_ERROR_FONT_OPEN_FAILED);
			STR_CASE(CELL_FONT_ERROR_FONT_NOT_FOUND);
			STR_CASE(CELL_FONT_ERROR_FONT_OPEN_MAX);
			STR_CASE(CELL_FONT_ERROR_FONT_CLOSE_FAILED);
			STR_CASE(CELL_FONT_ERROR_ALREADY_OPENED);
			STR_CASE(CELL_FONT_ERROR_NO_SUPPORT_FUNCTION);
			STR_CASE(CELL_FONT_ERROR_NO_SUPPORT_CODE);
			STR_CASE(CELL_FONT_ERROR_NO_SUPPORT_GLYPH);
			STR_CASE(CELL_FONT_ERROR_BUFFER_SIZE_NOT_ENOUGH);
			STR_CASE(CELL_FONT_ERROR_RENDERER_ALREADY_BIND);
			STR_CASE(CELL_FONT_ERROR_RENDERER_UNBIND);
			STR_CASE(CELL_FONT_ERROR_RENDERER_INVALID);
			STR_CASE(CELL_FONT_ERROR_RENDERER_ALLOCATION_FAILED);
			STR_CASE(CELL_FONT_ERROR_ENOUGH_RENDERING_BUFFER);
			STR_CASE(CELL_FONT_ERROR_NO_SUPPORT_SURFACE);
		}

		return unknown;
	});
}

// Functions
error_code cellFontInitializeWithRevision(u64 revisionFlags, vm::ptr<CellFontConfig> config)
{
	cellFont.todo("cellFontInitializeWithRevision(revisionFlags=0x%llx, config=*0x%x)", revisionFlags, config);

	if (config->fc_size < 24)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (config->flags != 0u)
	{
		cellFont.error("cellFontInitializeWithRevision: Unknown flags (0x%x)", config->flags);
	}

	// TODO

	return CELL_OK;
}

void cellFontGetRevisionFlags(vm::ptr<u64> revisionFlags)
{
	cellFont.notice("cellFontGetRevisionFlags(*0x%x)", revisionFlags);

	if (revisionFlags)
	{
		*revisionFlags = 100;
	}
}

error_code cellFontEnd()
{
	cellFont.todo("cellFontEnd()");

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	return CELL_OK;
}

error_code cellFontSetFontsetOpenMode(vm::cptr<CellFontLibrary> library, u32 openMode)
{
	cellFont.todo("cellFontSetFontsetOpenMode(library=*0x%x, openMode=0x%x)", library, openMode);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (!library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: set openMode

	return CELL_OK;
}

error_code cellFontOpenFontMemory(vm::ptr<CellFontLibrary> library, u32 fontAddr, u32 fontSize, u32 subNum, u32 uniqueId, vm::ptr<CellFont> font)
{
	cellFont.todo("cellFontOpenFontMemory(library=*0x%x, fontAddr=0x%x, fontSize=%d, subNum=%d, uniqueId=%d, font=*0x%x)", library, fontAddr, fontSize, subNum, uniqueId, font);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: reset some font fields here

	if (!library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (!fontAddr || uniqueId < 0)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	font->stbfont = vm::_ptr<stbtt_fontinfo>(font.addr() + font.size()); // hack: use next bytes of the struct

	if (!stbtt_InitFont(font->stbfont, vm::_ptr<unsigned char>(fontAddr), 0))
	{
		return CELL_FONT_ERROR_FONT_OPEN_FAILED;
	}

	font->renderer_addr = 0;
	font->fontdata_addr = fontAddr;
	font->origin = CELL_FONT_OPEN_MEMORY;

	return CELL_OK;
}

error_code cellFontOpenFontFile(vm::ptr<CellFontLibrary> library, vm::cptr<char> fontPath, u32 subNum, s32 uniqueId, vm::ptr<CellFont> font)
{
	cellFont.todo("cellFontOpenFontFile(library=*0x%x, fontPath=%s, subNum=%d, uniqueId=%d, font=*0x%x)", library, fontPath, subNum, uniqueId, font);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: reset some font fields here

	if (!library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (!fontPath || uniqueId < 0)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

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

error_code cellFontOpenFontset(vm::ptr<CellFontLibrary> library, vm::ptr<CellFontType> fontType, vm::ptr<CellFont> font)
{
	cellFont.warning("cellFontOpenFontset(library=*0x%x, fontType=*0x%x, font=*0x%x)", library, fontType, font);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: reset some font fields here

	if (!library || !fontType)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	// TODO

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
		return { CELL_FONT_ERROR_NO_SUPPORT_FONTSET, fontType->type };
	}

	error_code ret = cellFontOpenFontFile(library, vm::make_str(file), 0, 0, font); //TODO: Find the correct values of subNum, uniqueId
	font->origin = CELL_FONT_OPEN_FONTSET;

	return ret;
}

error_code cellFontOpenFontInstance(vm::ptr<CellFont> openedFont, vm::ptr<CellFont> font)
{
	cellFont.todo("cellFontOpenFontInstance(openedFont=*0x%x, font=*0x%x)", openedFont, font);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: reset some font fields

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (!openedFont)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO: check field 0x10
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	font->renderer_addr = openedFont->renderer_addr;
	font->scale_x = openedFont->scale_x;
	font->scale_y = openedFont->scale_y;
	font->slant = openedFont->slant;
	font->stbfont = openedFont->stbfont;
	font->origin = CELL_FONT_OPEN_FONT_INSTANCE;

	return CELL_OK;
}

error_code cellFontSetFontOpenMode(vm::cptr<CellFontLibrary> library, u32 openMode)
{
	cellFont.todo("cellFontSetFontOpenMode(library=*0x%x, openMode=0x%x)", library, openMode);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (!library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: set openMode

	return CELL_OK;
}

error_code cellFontCreateRenderer(vm::cptr<CellFontLibrary> library, vm::ptr<CellFontRendererConfig> config, vm::ptr<CellFontRenderer> renderer)
{
	cellFont.todo("cellFontCreateRenderer(library=*0x%x, config=*0x%x, Renderer=*0x%x)", library, config, renderer);

	// Write data in Renderer

	if (!library || !config || !renderer)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO: check lib_func
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	return CELL_OK;
}

void cellFontRenderSurfaceInit(vm::ptr<CellFontRenderSurface> surface, vm::ptr<void> buffer, s32 bufferWidthByte, s32 pixelSizeByte, s32 w, s32 h)
{
	cellFont.notice("cellFontRenderSurfaceInit(surface=*0x%x, buffer=*0x%x, bufferWidthByte=%d, pixelSizeByte=%d, w=%d, h=%d)", surface, buffer, bufferWidthByte, pixelSizeByte, w, h);

	if (!surface)
	{
		return;
	}

	const u32 width = w & ~w >> 0x1f;
	const u32 height = h & ~h >> 0x1f;

	surface->buffer        = buffer;
	surface->widthByte     = bufferWidthByte;
	surface->pixelSizeByte = pixelSizeByte;
	surface->width         = width;
	surface->height        = height;
	surface->sc_x0         = 0;
	surface->sc_y0         = 0;
	surface->sc_x1         = width;
	surface->sc_y1         = height;
}

void cellFontRenderSurfaceSetScissor(vm::ptr<CellFontRenderSurface> surface, s32 x0, s32 y0, u32 w, u32 h)
{
	cellFont.warning("cellFontRenderSurfaceSetScissor(surface=*0x%x, x0=%d, y0=%d, w=%d, h=%d)", surface, x0, y0, w, h);

	if (!surface)
	{
		return;
	}

	// TODO: sdk check ?
	if (true)
	{
		if (surface->width != 0)
		{
			u32 sc_x0 = 0;
			u32 sc_x1 = 0;

			if (x0 < 0)
			{
				if (static_cast<u32>(-x0) < w)
				{
					sc_x1 = surface->width;

					if (x0 + w < sc_x1)
					{
						sc_x1 = x0 + w;
					}
				}
			}
			else
			{
				const u32 width = surface->width;

				sc_x0 = width;
				sc_x1 = width;

				if (static_cast<u32>(x0) <= sc_x0)
				{
					sc_x0 = x0;

					const u32 w_min = std::min(w, width);

					if (x0 + w_min <= width)
					{
						sc_x1 = x0 + w_min;
					}
				}
			}

			surface->sc_x0 = sc_x0;
			surface->sc_x1 = sc_x1;
		}

		if (surface->height != 0)
		{
			u32 sc_y0 = 0;
			u32 sc_y1 = 0;

			if (y0 < 0)
			{
				if (static_cast<u32>(-y0) < h)
				{
					sc_y1 = surface->height;

					if (y0 + h < sc_y1)
					{
						sc_y1 = y0 + h;
					}
				}
			}
			else
			{
				const u32 height = surface->height;

				sc_y0 = height;
				sc_y1 = height;

				if (static_cast<u32>(y0) <= sc_y0)
				{
					sc_y0 = y0;

					const u32 h_min = std::min(h, height);

					if (y0 + h_min <= sc_y1)
					{
						sc_y1 = y0 + h_min;
					}
				}
			}

			surface->sc_y0 = sc_y0;
			surface->sc_y1 = sc_y1;
		}
	}
	else
	{
		if (surface->width != 0)
		{
			if (static_cast<s32>(surface->sc_x0) < x0)
			{
				surface->sc_x0 = x0;
			}

			if (static_cast<s32>(w + x0) < static_cast<s32>(surface->sc_x1))
			{
				surface->sc_x1 = w + x0;
			}
		}

		if (surface->height != 0)
		{
			if (static_cast<s32>(surface->sc_y0) < y0)
			{
				surface->sc_y0 = y0;
			}

			if (static_cast<s32>(h + y0) < static_cast<s32>(surface->sc_y1))
			{
				surface->sc_y1 = h + y0;
			}
		}
	}
}

error_code cellFontSetScalePixel(vm::ptr<CellFont> font, f32 w, f32 h)
{
	cellFont.todo("cellFontSetScalePixel(font=*0x%x, w=%f, h=%f)", font, w, h);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	font->scale_x = w;
	font->scale_y = h;

	// TODO:
	//font->scalePointH = (h * something) / font->resolutionDpiV;
	//font->scalePointW = (w * something) / font->resolutionDpiH;

	return CELL_OK;
}

error_code cellFontGetHorizontalLayout(vm::ptr<CellFont> font, vm::ptr<CellFontHorizontalLayout> layout)
{
	cellFont.todo("cellFontGetHorizontalLayout(font=*0x%x, layout=*0x%x)", font, layout);

	if (!layout)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	*layout = {};

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	s32 ascent, descent, lineGap;
	const f32 scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	stbtt_GetFontVMetrics(font->stbfont, &ascent, &descent, &lineGap);

	layout->baseLineY = ascent * scale;
	layout->lineHeight = (ascent-descent+lineGap) * scale;
	layout->effectHeight = lineGap * scale;

	return CELL_OK;
}

error_code cellFontBindRenderer(vm::ptr<CellFont> font, vm::ptr<CellFontRenderer> renderer)
{
	cellFont.warning("cellFontBindRenderer(font=*0x%x, renderer=*0x%x)", font, renderer);

	if (!font || !renderer || !renderer->systemReserved[0x10])
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_ALREADY_BIND;
	}

	// TODO
	//renderer->systemReserved[2] = (void *)font->resolutionDpiH;
	//renderer->systemReserved[3] = (void *)font->resolutionDpiV;
	//renderer->systemReserved[4] = (void *)font->scalePointW;
	//renderer->systemReserved[5] = (void *)font->scalePointH;
	//renderer->systemReserved[6] = (void *)font->scalePixelW;
	//renderer->systemReserved[7] = (void *)font->scalePixelH;
	//renderer->systemReserved[8] = *(void **)&font->font_weight;
	//renderer->systemReserved[9] = *(void **)&font->field_0x5c;

	font->renderer_addr = renderer.addr();

	return CELL_OK;
}

error_code cellFontUnbindRenderer(vm::ptr<CellFont> font)
{
	cellFont.warning("cellFontBindRenderer(font=*0x%x)", font);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	font->renderer_addr = 0;

	return CELL_OK;
}

error_code cellFontDestroyRenderer(vm::ptr<CellFontRenderer> renderer)
{
	cellFont.todo("cellFontDestroyRenderer(renderer=*0x%x)", renderer);

	if (!renderer || !renderer->systemReserved[0x10])
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO: check func
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (false) // TODO: call func
	{
		renderer->systemReserved[0x10] = vm::null;
	}

	return CELL_OK;
}

error_code cellFontSetupRenderScalePixel(vm::ptr<CellFont> font, f32 w, f32 h)
{
	cellFont.todo("cellFontSetupRenderScalePixel(font=*0x%x, w=%f, h=%f)", font, w, h);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO:

	//if (w == something)
	//{
	//	w = font->scalePixelW;
	//}

	//if (h == something)
	//{
	//	h = font->scalePixelH;
	//}

	//font->field_0x14 + 0x10 = (w * something) / font->resolutionDpiH;
	//font->field_0x14 + 0x14 = (h * something) / font->resolutionDpiV;
	//font->field_0x14 + 0x18 = w;
	//font->field_0x14 + 0x1c = h;
	//font->field_0x78 = 0;

	return CELL_OK;
}

error_code cellFontGetRenderCharGlyphMetrics(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont.todo("cellFontGetRenderCharGlyphMetrics(font=*0x%x, code=0x%x, metrics=*0x%x)", font, code, metrics);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	if (!metrics)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: ?
	return CELL_OK;
}

error_code cellFontRenderCharGlyphImage(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontRenderSurface> surface, f32 x, f32 y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont.notice("cellFontRenderCharGlyphImage(font=*0x%x, code=0x%x, surface=*0x%x, x=%f, y=%f, metrics=*0x%x, transInfo=*0x%x)", font, code, surface, x, y, metrics, transInfo);

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// Render the character
	s32 width, height, xoff, yoff;
	const f32 scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
	unsigned char* box = stbtt_GetCodepointBitmap(font->stbfont, scale, scale, code, &width, &height, &xoff, &yoff);

	if (!box)
	{
		return CELL_OK;
	}

	// Get the baseLineY value
	s32 ascent, descent, lineGap;
	stbtt_GetFontVMetrics(font->stbfont, &ascent, &descent, &lineGap);
	const s32 baseLineY = static_cast<int>(ascent * scale); // ???

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
	stbtt_FreeBitmap(box, nullptr);
	return CELL_OK;
}

error_code cellFontEndLibrary(vm::cptr<CellFontLibrary> library)
{
	cellFont.todo("cellFontEndLibrary(library=*0x%x)", library);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	return CELL_OK;
}

error_code cellFontSetEffectSlant(vm::ptr<CellFont> font, f32 slantParam)
{
	cellFont.trace("cellFontSetEffectSlant(font=*0x%x, slantParam=%f)", font, slantParam);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: I think this clamps instead of returning an error
	if (slantParam < -1.0f || slantParam > 1.0f)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	font->slant = slantParam;

	return CELL_OK;
}

error_code cellFontGetEffectSlant(vm::ptr<CellFont> font, vm::ptr<f32> slantParam)
{
	cellFont.trace("cellFontSetEffectSlant(font=*0x%x, slantParam=*0x%x)", font, slantParam);

	if (!font || !slantParam)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	*slantParam = font->slant;

	return CELL_OK;
}

error_code cellFontGetFontIdCode(vm::ptr<CellFont> font, u32 code, vm::ptr<u32> fontId, vm::ptr<u32> fontCode)
{
	cellFont.todo("cellFontGetFontIdCode(font=*0x%x, code=%d, fontId=*0x%x, fontCode=*0x%x)", font, code, fontId, fontCode);
	
	if (fontId)
	{
		*fontId = 0;
	}

	if (fontCode)
	{
		*fontCode = 0;
	}

	if (!font) // TODO || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: some other code goes here

	if (false) // TODO (!font->field_0x2)
	{
		if (false) // TODO (font->field_0x60 != code)
		{
			if (false) // TODO
			{
				return CELL_FONT_ERROR_NO_SUPPORT_CODE;
			}
			// TODO: some other stuff

			return CELL_OK;
		}

		if (fontId)
		{
			//*fontId = font->field_0x64; // TODO
		}
		if (fontCode)
		{
			//*fontCode = font->field_0x68; // TODO
		}
	}
	else
	{
		if (fontId)
		{
			//*fontId = font->field_0x8 & 0x7fffffff; // TODO
		}
		if (fontCode)
		{
			*fontCode = code;
		}
	}

	return CELL_OK;
}

error_code cellFontCloseFont(vm::ptr<CellFont> font)
{
	cellFont.todo("cellFontCloseFont(font=*0x%x)", font);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (font->origin == CELL_FONT_OPEN_FONTSET ||
		font->origin == CELL_FONT_OPEN_FONT_FILE ||
		font->origin == CELL_FONT_OPEN_MEMORY)
	{
		vm::dealloc(font->fontdata_addr, vm::main);
	}

	return CELL_OK;
}

error_code cellFontGetCharGlyphMetrics(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont.todo("cellFontGetCharGlyphMetrics(font=*0x%x, code=0x%x, metrics=*0x%x)", font, code, metrics);

	if (!font || !metrics)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_CODE;
	}

	s32 x0, y0, x1, y1;
	s32 advanceWidth, leftSideBearing;
	const f32 scale = stbtt_ScaleForPixelHeight(font->stbfont, font->scale_y);
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

error_code cellFontGraphicsSetFontRGBA(vm::ptr<CellFontGraphicsDrawContext> context, vm::ptr<f32> fontRGBA)
{
	cellFont.todo("cellFontGraphicsSetFontRGBA(context=*0x%x, fontRGBA=*0x%x)", context, fontRGBA);

	if (!context || !fontRGBA) // TODO || (context->magic != 0xcf50))
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//context->field_0x70 = fontRGBA[0];
	//context->field_0x74 = fontRGBA[1];
	//context->field_0x78 = fontRGBA[2];
	//context->field_0x7c = fontRGBA[3];

	return CELL_OK;
}

error_code cellFontOpenFontsetOnMemory(vm::ptr<CellFontLibrary> library, vm::ptr<CellFontType> fontType, vm::ptr<CellFont> font)
{
	cellFont.todo("cellFontOpenFontsetOnMemory(library=*0x%x, fontType=*0x%x, font=*0x%x)", library, fontType, font);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: reset some font fields here

	if (!library || !fontType)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (fontType->map != CELL_FONT_MAP_UNICODE)
	{
		cellFont.warning("cellFontOpenFontsetOnMemory: Only Unicode is supported");
	}

	return CELL_OK;
}

error_code cellFontGraphicsSetScalePixel(vm::ptr<CellFontGraphicsDrawContext> context, f32 w, f32 h)
{
	cellFont.todo("cellFontGraphicsSetScalePixel(context=*0x%x, w=%f, h=%f)", context, w, h);

	if (!context) // TODO || (context->magic != 0xcf50))
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//context->field_0x10 = (w * something) / context->field_0x8;
	//context->field_0x14 = (h * something) / context->field_0xc;
	//context->field_0x18 = w;
	//context->field_0x1c = h;

	return CELL_OK;
}

error_code cellFontGraphicsGetScalePixel(vm::ptr<CellFontGraphicsDrawContext> context, vm::ptr<f32> w, vm::ptr<f32> h)
{
	cellFont.todo("cellFontGraphicsGetScalePixel(context=*0x%x, w=*0x%x, h=*0x%x)", context, w, h);

	if (w)
	{
		*w = 0.0;
	}

	if (h)
	{
		*h = 0.0;
	}

	if (!context) // TODO || context->magic != 0xcf50)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (w)
	{
		//*w = context->field_0x18; // TODO
	}
	if (h)
	{
		//*h = context->field_0x1c; // TODO
	}

	return CELL_OK;
}

error_code cellFontSetEffectWeight(vm::ptr<CellFont> font, f32 effectWeight)
{
	cellFont.warning("cellFontSetEffectWeight(font=*0x%x, effectWeight=%f)", font, effectWeight);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO: set effect weight (probably clamped)

	return CELL_OK;
}

error_code cellFontGlyphSetupVertexesGlyph(vm::ptr<CellFontGlyph> glyph, f32 controlDistance, vm::ptr<u32> mappedBuf, u32 mappedBufSize, vm::ptr<CellFontVertexesGlyph> vGlyph, vm::ptr<u32> dataSize)
{
	cellFont.todo("cellFontGlyphSetupVertexesGlyph(glyph=*0x%x, controlDistance=%f, mappedBuf=*0x%x, mappedBufSize=0x%x, vGlyph=*0x%x, dataSize=*0x%x)", glyph, controlDistance, mappedBuf, mappedBufSize, vGlyph, dataSize);

	//if (in_r8)
	//{
	//	*in_r8 = 0; // ???
	//}

	if (!dataSize)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	dataSize[0] = 0;
	dataSize[1] = 0;

	if (mappedBufSize == 0)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGetVerticalLayout(vm::ptr<CellFont> font, vm::ptr<CellFontVerticalLayout> layout)
{
	cellFont.todo("cellFontGetVerticalLayout(font=*0x%x, layout=*0x%x)", font, layout);
	
	if (!layout)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	*layout = {};

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGetRenderCharGlyphMetricsVertical(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont.todo("cellFontGetRenderCharGlyphMetricsVertical(font=*0x%x, code=0x%x, metrics=*0x%x)", font, code, metrics);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0x14)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontSetScalePoint(vm::ptr<CellFont> font, f32 w, f32 h)
{
	cellFont.todo("cellFontSetScalePoint(font=*0x%x, w=%f, h=%f)", font, w, h);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//font->scalePointW = w;
	//font->scalePointH = h;
	//font->scalePixelW = (w * font->resolutionDpiH) / something;
	//font->scalePixelH = (h * font->resolutionDpiV) / something;

	return CELL_OK;
}

error_code cellFontSetupRenderEffectSlant(vm::ptr<CellFont> font, f32 effectSlant)
{
	cellFont.todo("cellFontSetupRenderEffectSlant(font=*0x%x, effectSlant=%f)", font, effectSlant);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0x14)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO:
	//font->effectSlant = effectSlant; // TODO: probably clamped
	//font->field_0x78 = 0;

	return CELL_OK;
}

error_code cellFontGraphicsSetLineRGBA(vm::ptr<CellFontGraphicsDrawContext> context, vm::ptr<f32> lineRGBA)
{
	cellFont.todo("cellFontGraphicsSetLineRGBA(context=*0x%x, lineRGBA=*0x%x)", context, lineRGBA);

	if (!context || !lineRGBA) // TODO || (context->magic != 0xcf50))
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//context->lineColorR = lineRGBA[0];
	//context->lineColorG = lineRGBA[1];
	//context->lineColorB = lineRGBA[2];
	//context->lineColorA = lineRGBA[3];

	return CELL_OK;
}

error_code cellFontGraphicsSetDrawType(vm::ptr<CellFontGraphicsDrawContext> context, u32 type)
{
	cellFont.todo("cellFontGraphicsSetDrawType(context=*0x%x, type=0x%x)", context, type);

	if (!context) // TODO || (context->magic != 0xcf50))
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

    //context->field_0x42 = type; // TODO

	return CELL_OK;
}

error_code cellFontEndGraphics(vm::cptr<CellFontGraphics> graphics)
{
	cellFont.todo("cellFontEndGraphics(graphics=*0x%x)", graphics);

	if (!graphics)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	return CELL_OK;
}

error_code cellFontGraphicsSetupDrawContext(vm::cptr<CellFontGraphics> graphics, vm::ptr<CellFontGraphicsDrawContext> context)
{
	cellFont.todo("cellFontGraphicsSetupDrawContext(graphics=*0x%x, context=*0x%x)", graphics, context);

	if (!graphics && !context)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//context->magic = 0xcf50;
	//context->field_0x50 = something;
	//context->field_0x54 = something;
	//context->field_0x58 = something;
	//context->field_0x5c = something;
	//context->field_0x2 = 0;
	//context->field_0x8 = 72;
	//context->field_0x42 = 1;
	//context->field_0x4c = 0;
	//context->field_0x4 = 0;
	//context->field_0xc = 72;
	//context->field_0x14 = 0;
	//context->field_0x10 = 0;
	//context->field_0x1c = 0;
	//context->field_0x18 = 0;
	//context->field_0x20 = 0;
	//context->field_0x24 = 0;
	//context->field_0x28 = 0;
	//context->field_0x2c = 0;
	//context->field_0x30 = 0;
	//context->field_0x34 = 0;
	//context->field_0x38 = 0;
	//context->field_0x3c = 0;
	//context->field_0x40 = 0;
	//context->field_0x44 = 0;
	//context->field_0x48 = 0;
	//context->field_0x7c = something;
	//context->field_0x78 = something;
	//context->field_0x74 = something;
	//context->field_0x70 = something;
	//context->lineColorB = 0.0;
	//context->lineColorG = 0.0;
	//context->lineColorR = 0.0;
	//context->lineColorA = something;
	//context->field_0xc0 = 0;
	//context->field_0x80 = something;
	//context->field_0xbc = something;
	//context->field_0xa8 = something;
	//context->field_0x94 = something;
	//context->field_0xb8 = 0;
	//context->field_0xb4 = 0;
	//context->field_0xb0 = 0;
	//context->field_0xac = 0;
	//context->field_0xa4 = 0;
	//context->field_0xa0 = 0;
	//context->field_0x9c = 0;
	//context->field_0x98 = 0;
	//context->field_0x90 = 0;
	//context->field_0x8c = 0;
	//context->field_0x88 = 0;
	//context->field_0x84 = 0;
	//context->field_0xfc = 0;
	//context->field_0xf8 = 0;
	//context->field_0xf4 = 0;
	//context->field_0xf0 = 0;
	//context->field_0xec = 0;
	//context->field_0xe8 = 0;
	//context->field_0xe4 = 0;
	//context->field_0xe0 = 0;
	//context->field_0xdc = 0;
	//context->field_0xd8 = 0;
	//context->field_0xd4 = 0;
	//context->field_0xd0 = 0;
	//context->field_0xcc = 0;
	//context->field_0xc8 = 0;
	//context->field_0xc4 = 0;

	return CELL_OK;
}

error_code cellFontSetupRenderEffectWeight(vm::ptr<CellFont> font, f32 additionalWeight)
{
	cellFont.todo("cellFontSetupRenderEffectWeight(font=*0x%x, additionalWeight=%f)", font, additionalWeight);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0x14)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO: set weight

	return CELL_OK;
}

error_code cellFontGlyphGetOutlineControlDistance(vm::ptr<CellFontGlyph> glyph, f32 maxScale, f32 baseControlDistance, vm::ptr<f32> controlDistance)
{
	cellFont.todo("cellFontGlyphGetOutlineControlDistance(glyph=*0x%x, maxScale=%f, baseControlDistance=%f, controlDistance=*0x%x)", glyph, maxScale, baseControlDistance, controlDistance);

	if (!glyph || !controlDistance || maxScale <= 0.0f)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//*controlDistance = (something / maxScale) * baseControlDistance;

	return CELL_OK;
}

error_code cellFontGlyphGetVertexesGlyphSize(vm::ptr<CellFontGlyph> glyph, f32 controlDistance, vm::ptr<u32> useSize)
{
	cellFont.todo("cellFontGlyphGetVertexesGlyphSize(glyph=*0x%x, controlDistance=%f, useSize=*0x%x)", glyph, controlDistance, useSize);

	if (false) // TODO (!in_r5)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	//*in_r5 = 0; // TODO

	if (!glyph)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGenerateCharGlyph(vm::ptr<CellFont> font, u32 code, vm::pptr<CellFontGlyph> glyph)
{
	cellFont.todo("cellFontGenerateCharGlyph(font=*0x%x, code=0x%x, glyph=*0x%x)", font, code, glyph);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontDeleteGlyph(vm::ptr<CellFont> font, vm::ptr<CellFontGlyph> glyph)
{
	cellFont.todo("cellFontDeleteGlyph(font=*0x%x, glyph=*0x%x)", font, glyph);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false && !glyph) // TODO
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontExtend(u32 a1, u32 a2, u32 a3)
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

error_code cellFontRenderCharGlyphImageVertical(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontRenderSurface> surface, f32 x, f32 y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont.todo("cellFontRenderCharGlyphImageVertical(font=*0x%x, code=0x%x, surface=*0x%x, x=%f, y=%f, metrics=*0x%x, transInfo=*0x%x)", font, code, surface, x, y, metrics, transInfo);
	return CELL_OK;
}

error_code cellFontSetResolutionDpi(vm::ptr<CellFont> font, u32 hDpi, u32 vDpi)
{
	cellFont.todo("cellFontSetResolutionDpi(font=*0x%x, hDpi=0x%x, vDpi=0x%x)", font, hDpi, vDpi);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	// font->resolutionDpiH = hDpi == 0 ? 72 : hDpi;
	// font->resolutionDpiV = vDpi == 0 ? 72 : vDpi;

	return CELL_OK;
}

error_code cellFontGetCharGlyphMetricsVertical(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontGlyphMetrics> metrics)
{
	cellFont.todo("cellFontGetCharGlyphMetricsVertical(font=*0x%x, code=0x%x, metrics=*0x%x)", font, code, metrics);

	if (!font || !metrics)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_CODE;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGetRenderEffectWeight(vm::ptr<CellFont> font, vm::ptr<f32> effectWeight)
{
	cellFont.todo("cellFontGetRenderEffectWeight(font=*0x%x, effectWeight=*0x%x)", font, effectWeight);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0x14)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	if (effectWeight) // Technically unchecked in firmware
	{
		// TODO
		//*effectWeight = font->field_0x14 + 0x20;
	}

	return CELL_OK;
}

error_code cellFontGraphicsGetDrawType(vm::ptr<CellFontGraphicsDrawContext> context, vm::ptr<u32> type)
{
	cellFont.todo("cellFontGraphicsGetDrawType(context=*0x%x, type=*0x%x)", context, type);

	if (!context || !type)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (context->magic != 0xcf50)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	//*type = context->field_0x42; // TODO

	return CELL_OK;
}

error_code cellFontGetKerning(vm::ptr<CellFont> font, u32 preCode, u32 code, vm::ptr<CellFontKerning> kerning)
{
	cellFont.todo("cellFontGetKerning(font=*0x%x, preCode=0x%x, code=0x%x, kerning=*0x%x)", font, preCode, code, kerning);

	if (!kerning)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	*kerning = {};

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (preCode == 0 || code == 0)
	{
		return CELL_OK;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGetRenderScaledKerning(vm::ptr<CellFont> font, u32 preCode, u32 code, vm::ptr<CellFontKerning> kerning)
{
	cellFont.todo("cellFontGetRenderScaledKerning(font=*0x%x, preCode=0x%x, code=0x%x, kerning=*0x%x)", font, preCode, code, kerning);

	if (!kerning)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	*kerning = {};

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO: !font->field_0x14
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (preCode == 0 || code == 0)
	{
		return CELL_OK;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGetRenderScalePixel(vm::ptr<CellFont> font, vm::ptr<f32> w, vm::ptr<f32> h)
{
	cellFont.todo("cellFontGetRenderScalePixel(font=*0x%x, w=*0x%x, h=*0x%x)", font, w, h);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO: !font->field_0x14
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	if (w)
	{
		//*w = font->field_0x14 + 0x18; // TODO
	}

	if (h)
	{
		//*h = font->field_0x14 + 0x1c; // TODO
	}

	return CELL_OK;
}

error_code cellFontGlyphGetScalePixel(vm::ptr<CellFontGlyph> glyph, vm::ptr<f32> w, vm::ptr<f32> h)
{
	cellFont.todo("cellFontGlyphGetScalePixel(glyph=*0x%x, w=*0x%x, h=*0x%x)", glyph, w, h);

	if (!glyph)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (w)
	{
		*w = 0.0f;
	}

	if (h)
	{
		*h = 0.0f;
	}

	if (!glyph->Outline.generateEnv)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (w)
	{
		//*w = glyph->Outline.generateEnv[0]; // TODO
	}

	if (h)
	{
		//*h = glyph->Outline.generateEnv[1]; // TODO
	}

	return CELL_OK;
}

error_code cellFontGlyphGetHorizontalShift(vm::ptr<CellFontGlyph> glyph, vm::ptr<f32> shiftX, vm::ptr<f32> shiftY)
{
	cellFont.todo("cellFontGlyphGetHorizontalShift(glyph=*0x%x, shiftX=*0x%x, shiftY=*0x%x)", glyph, shiftX, shiftY);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (!glyph)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (glyph->Outline.generateEnv)
	{
		if (shiftX)
		{
			*shiftX = 0.0;
		}

		if (shiftY)
		{
			//*shiftY = glyph->Outline.generateEnv + 0x48; // TODO
		}
	}

	return CELL_OK;
}

error_code cellFontRenderCharGlyphImageHorizontal(vm::ptr<CellFont> font, u32 code, vm::ptr<CellFontRenderSurface> surface, f32 x, f32 y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont.todo("cellFontRenderCharGlyphImageHorizontal(font=*0x%x, code=0x%x, surface=*0x%x, x=%f, y=%f, metrics=*0x%x, transInfo=*0x%x)", font, code, surface, x, y, metrics, transInfo);
	return CELL_OK;
}

error_code cellFontGetEffectWeight(vm::ptr<CellFont> font, vm::ptr<f32> effectWeight)
{
	cellFont.todo("cellFontGetEffectWeight(font=*0x%x, effectWeight=*0x%x)", font, effectWeight);

	if (!font || !effectWeight)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	//*effectWeight = font->weight * something; // TODO

	return CELL_OK;
}

error_code cellFontGetScalePixel(vm::ptr<CellFont> font, vm::ptr<f32> w, vm::ptr<f32> h)
{
	cellFont.todo("cellFontGetScalePixel(font=*0x%x, w=*0x%x, h=*0x%x)", font, w, h);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (w)
	{
		//*w = font->scalePixelW; // TODO
	}

	if (h)
	{
		//*h = font->scalePixelH; // TODO
	}

	return CELL_OK;
}

error_code cellFontClearFileCache()
{
	cellFont.todo("cellFontClearFileCache()");
	return CELL_OK;
}

error_code cellFontAdjustFontScaling(vm::ptr<CellFont> font, f32 fontScale)
{
	cellFont.todo("cellFontAdjustFontScaling(font=*0x%x, fontScale=%f)", font, fontScale);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0x2)
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	// TODO: set font scale (probably clamped)

	return CELL_OK;
}

error_code cellFontSetupRenderScalePoint(vm::ptr<CellFont> font, f32 w, f32 h)
{
	cellFont.todo("cellFontSetupRenderScalePoint(font=*0x%x, w=%f, h=%f)", font, w, h);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (!font->renderer_addr)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	// TODO:

	//if (w == something)
	//{
	//	w = font->scalePointW;
	//}

	//if (h == something)
	//{
	//	h = font->scalePointH;
	//}

	//font->field_0x14 + 0x10 = w;
	//font->field_0x14 + 0x14 = h;
	//font->field_0x14 + 0x18 = (w * font->resolutionDpiH / something;
	//font->field_0x14 + 0x1c = (h * font->resolutionDpiV / something;
	//font->field_0x78 = 0;

	return CELL_OK;
}

error_code cellFontGlyphGetVerticalShift(vm::ptr<CellFontGlyph> glyph, vm::ptr<f32> shiftX, vm::ptr<f32> shiftY)
{
	cellFont.todo("cellFontGlyphGetVerticalShift(glyph=*0x%x, shiftX=*0x%x, shiftY=*0x%x)", glyph, shiftX, shiftY);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (!glyph || !glyph->Outline.generateEnv)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	//fVar1 = glyph->Outline.generateEnv + 0x4c;
	//fVar2 = glyph->Outline.generateEnv + 0x48;

	//if (fVar1 == something)
	//{
	//	fVar1 = (glyph->Outline.generateEnv + 0x4) / (glyph->Outline.generateEnv + 0x14) * (glyph->Outline.generateEnv + 0x18);
	//}

	//if (shiftX)
	//{
	//	*shiftX = glyph->Outline.generateEnv + 0x38;
	//}

	//if (shiftY)
	//{
	//	*shiftY = -fVar2 + fVar1 + (glyph->Outline.generateEnv + 0x3c);
	//}

	return CELL_OK;
}

error_code cellFontGetGlyphExpandBufferInfo(vm::ptr<CellFont> font, vm::ptr<s32> pointN, vm::ptr<s32> contourN)
{
	cellFont.todo("cellFontGetGlyphExpandBufferInfo(font=*0x%x, pointN=*0x%x, contourN=*0x%x)", font, pointN, contourN);

	if (pointN)
	{
		*pointN = 0;
	}

	if (contourN)
	{
		*contourN = 0;
	}

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!something || !font->library)
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (false) // TODO (!font->field_0xc)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGetLibrary(vm::ptr<CellFont> font, vm::cpptr<CellFontLibrary> library, vm::ptr<u32> type)
{
	cellFont.todo("cellFontGetLibrary(font=*0x%x, library=*0x%x, type=*0x%x)", font, library, type);

	if (!font || !library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//**library = font->library;

	if (!(*library))
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (type)
	{
		*type = (*library)->libraryType;
	}

	return CELL_OK;
}

error_code cellFontVertexesGlyphRelocate(vm::ptr<CellFontVertexesGlyph> vGlyph, vm::ptr<CellFontVertexesGlyph> vGlyph2, vm::ptr<CellFontVertexesGlyphSubHeader> subHeader, vm::ptr<u32> localBuf, u32 copySize)
{
	cellFont.todo("cellFontVertexesGlyphRelocate(vGlyph=*0x%x, vGlyph2=*0x%x, subHeader=*0x%x, localBuf=*0x%x, copySize=0x%x)", vGlyph, vGlyph2, subHeader, localBuf, copySize);

	if (!vGlyph2)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (vGlyph2 != vGlyph)
	{
		vGlyph2->subHeader = vm::null;
		vGlyph2->data = vm::null;
	}

	if (!vGlyph || !subHeader)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}
	return CELL_OK;
}

error_code cellFontGetInitializedRevisionFlags(vm::ptr<u64> revisionFlags)
{
	cellFont.todo("cellFontGetInitializedRevisionFlags(revisionFlags=*0x%x)", revisionFlags);

	if (revisionFlags)
	{
		*revisionFlags = 0;
	}

	if (false) // TODO
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (revisionFlags)
	{
		//*revisionFlags = something; // TODO
	}

	return CELL_OK;
}

error_code cellFontGetResolutionDpi(vm::ptr<CellFont> font, vm::ptr<u32> hDpi, vm::ptr<u32> vDpi)
{
	cellFont.todo("cellFontGetResolutionDpi(font=*0x%x, hDpi=*0x%x, vDpi=*0x%x)", font, hDpi, vDpi);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (hDpi)
	{
		//*hDpi = font->resolutionDpiH; // TODO
	}

	if (vDpi)
	{
		//*vDpi = font->resolutionDpiV; // TODO
	}

	return CELL_OK;
}

error_code cellFontGlyphRenderImageVertical(vm::ptr<CellFontGlyph> font, vm::ptr<CellFontGlyphStyle> style, vm::ptr<CellFontRenderer> renderer, vm::ptr<CellFontRenderSurface> surf, f32 x, f32 y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont.todo("cellFontGlyphRenderImageVertical(font=*0x%x, style=*0x%x, renderer=*0x%x, surf=*0x%x, x=%f, y=%f, metrics=*0x%x, transInfo=*0x%x)", font, style, renderer, surf, x, y, metrics, transInfo);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (!font || !renderer || !renderer->systemReserved[0x10])
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGlyphRenderImageHorizontal(vm::ptr<CellFontGlyph> font, vm::ptr<CellFontGlyphStyle> style, vm::ptr<CellFontRenderer> renderer, vm::ptr<CellFontRenderSurface> surf, f32 x, f32 y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont.todo("cellFontGlyphRenderImageHorizontal(font=*0x%x, style=*0x%x, renderer=*0x%x, surf=*0x%x, x=%f, y=%f, metrics=*0x%x, transInfo=*0x%x)", font, style, renderer, surf, x, y, metrics, transInfo);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (!font || !renderer || !renderer->systemReserved[0x10])
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontAdjustGlyphExpandBuffer(vm::ptr<CellFont> font, s32 pointN, s32 contourN)
{
	cellFont.todo("cellFontAdjustGlyphExpandBuffer(font=*0x%x, pointN=%d, contourN=%d)", font, pointN, contourN);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!something || !font->library)
	{
		return CELL_FONT_ERROR_UNINITIALIZED;
	}

	if (false) // TODO (!font->field_0xc)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	return CELL_OK;
}

error_code cellFontGetRenderScalePoint(vm::ptr<CellFont> font, vm::ptr<f32> w, vm::ptr<f32> h)
{
	cellFont.todo("cellFontGetRenderScalePoint(font=*0x%x, w=*0x%x, h=*0x%x)", font, w, h);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO: !font->field_0x14
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	if (w)
	{
		//*w = font->field_0x14 + 0x10; // TODO
	}

	if (h)
	{
		//*h = font->field_0x14 + 0x14; // TODO
	}

	return CELL_OK;
}

error_code cellFontGraphicsGetFontRGBA(vm::ptr<CellFontGraphicsDrawContext> context, vm::ptr<f32> fontRGBA)
{
	cellFont.todo("cellFontGraphicsGetFontRGBA(context=*0x%x, fontRGBA=*0x%x)", context, fontRGBA);

	if (!context || !fontRGBA)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO: (context->magic != 0xcf50)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//fontRGBA[0] = context->field_0x70;
	//fontRGBA[1] = context->field_0x74;
	//fontRGBA[2] = context->field_0x78;
	//fontRGBA[3] = context->field_0x7c;

	return CELL_OK;
}

error_code cellFontGlyphGetOutlineVertexes(vm::ptr<CellFontGlyph> glyph, f32 controlDistance, vm::ptr<CellFontGetOutlineVertexesIF> getIF, vm::ptr<CellFontGlyphBoundingBox> bbox, vm::ptr<u32> vcount)
{
	cellFont.todo("cellFontGlyphGetOutlineVertexes(glyph=*0x%x, controlDistance=%f, getIF=*0x%x, bbox=*0x%x, vcount=*0x%x)", glyph, controlDistance, getIF, bbox, vcount);

	if (!glyph)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (!bbox)
	{
		//bbox = something; // TODO
	}

	if (glyph->Outline.contoursCount == 0)
	{
		if (vcount)
		{
			vcount[0] = 0;
			vcount[1] = 0;
			vcount[2] = 0;
			vcount[3] = 0;
		}

		//if (in_r7)
		//{
		//	*in_r7 = 0; // ???
		//}

		return CELL_OK;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontDelete(vm::cptr<CellFontLibrary> library, vm::ptr<void> p)
{
	cellFont.todo("cellFontDelete(library=*0x%x, p=*0x%x)", library, p);

	if (!library || !p)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontPatchWorks(s32 param_1, u64 param_2)
{
	cellFont.todo("cellFontPatchWorks(param_1=0x%x, param_2=0x%x)", param_1, param_2);
	return CELL_OK;
}

error_code cellFontGlyphRenderImage(vm::ptr<CellFontGlyph> font, vm::ptr<CellFontGlyphStyle> style, vm::ptr<CellFontRenderer> renderer, vm::ptr<CellFontRenderSurface> surf, f32 x, f32 y, vm::ptr<CellFontGlyphMetrics> metrics, vm::ptr<CellFontImageTransInfo> transInfo)
{
	cellFont.todo("cellFontGlyphRenderImage(font=*0x%x, style=*0x%x, renderer=*0x%x, surf=*0x%x, x=%f, y=%f, metrics=*0x%x, transInfo=*0x%x)", font, style, renderer, surf, x, y, metrics, transInfo);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (!font || !renderer || !renderer->systemReserved[0x10])
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGetBindingRenderer(vm::ptr<CellFont> font, vm::pptr<CellFontRenderer> renderer)
{
	cellFont.todo("cellFontGetBindingRenderer(font=*0x%x, renderer=*0x%x)", font, renderer);

	if (!renderer)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (font)
	{
		// TODO: may return CELL_FONT_ERROR_RENDERER_UNBIND
		//*renderer = font->field_0x14;
	}
	else
	{
		*renderer = vm::null;
	}

	return CELL_OK;
}

error_code cellFontGenerateCharGlyphVertical(vm::ptr<CellFont> font, u32 code, vm::pptr<CellFontGlyph> glyph)
{
	cellFont.todo("cellFontGenerateCharGlyphVertical(font=*0x%x, code=0x%x, glyph=*0x%x)", font, code, glyph);

	if (false) // TODO
	{
		return CELL_FONT_ERROR_NO_SUPPORT_FUNCTION;
	}

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO

	return CELL_OK;
}

error_code cellFontGetRenderEffectSlant(vm::ptr<CellFont> font, vm::ptr<f32> effectSlant)
{
	cellFont.todo("cellFontGetRenderEffectSlant(font=*0x%x, effectSlant=*0x%x)", font, effectSlant);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0xc || !font->library)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO (!font->field_0x14)
	{
		return CELL_FONT_ERROR_RENDERER_UNBIND;
	}

	if (effectSlant) // Technically unchecked in firmware
	{
		// TODO
		//*effectSlant = font->field_0x14 + 0x24;
	}

	return CELL_OK;
}


error_code cellFontGetScalePoint(vm::ptr<CellFont> font, vm::ptr<f32> w, vm::ptr<f32> h)
{
	cellFont.todo("cellFontGetScalePoint(font=*0x%x, w=*0x%x, h=*0x%x)", font, w, h);

	if (!font)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (w)
	{
		//*w = font->scalePointW; // TODO
	}

	if (h)
	{
		//*h = font->scalePointH; // TODO
	}

	return CELL_OK;
}


error_code cellFontGraphicsGetLineRGBA(vm::ptr<CellFontGraphicsDrawContext> context, vm::ptr<f32> lineRGBA)
{
	cellFont.todo("cellFontGraphicsGetLineRGBA(context=*0x%x, lineRGBA=*0x%x)", context, lineRGBA);

	if (!context || !lineRGBA)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	if (false) // TODO: (context->magic != 0xcf50)
	{
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	}

	// TODO
	//lineRGBA[0] = context->lineColorR;
	//lineRGBA[1] = context->lineColorG;
	//lineRGBA[2] = context->lineColorB;
	//lineRGBA[3] = context->lineColorA;

	return CELL_OK;
}

error_code cellFontControl()
{
	cellFont.todo("cellFontControl()");
	return CELL_OK;
}

error_code cellFontStatic()
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
