#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cell_FreeType2.h"

logs::channel cell_FreeType2("cell_FreeType2", logs::level::notice);

// Functions
s32 cellFreeType2Ex()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Add_Default_Modules()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Bitmap_Copy()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Bitmap_Embolden()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Bitmap_New()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Done_Face()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Done_Glyph()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Get_Char_Index()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Get_Glyph()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Get_Sfnt_Table()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Glyph_Copy()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Glyph_Get_CBox()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Glyph_To_Bitmap()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Glyph_Transform()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Init_FreeType()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Load_Glyph()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_MulFix()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_New_Face()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_New_Library()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_New_Memory_Face()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Open_Face()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Outline_Embolden()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Render_Glyph()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}


s32 FT_RoundFix()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Select_Charmap()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Set_Char_Size()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Set_Transform()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

s32 FT_Vector_Transform()
{
	UNIMPLEMENTED_FUNC(cell_FreeType2);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cell_FreeType2)("cell_FreeType2", []()
{
	REG_FUNC(cell_FreeType2, cellFreeType2Ex);

	REG_FUNC(cell_FreeType2, FT_Add_Default_Modules);

	REG_FUNC(cell_FreeType2, FT_Bitmap_Copy);
	REG_FUNC(cell_FreeType2, FT_Bitmap_Embolden);
	REG_FUNC(cell_FreeType2, FT_Bitmap_New);

	REG_FUNC(cell_FreeType2, FT_Done_Face);
	REG_FUNC(cell_FreeType2, FT_Done_Glyph);

	REG_FUNC(cell_FreeType2, FT_Get_Char_Index);
	REG_FUNC(cell_FreeType2, FT_Get_Glyph);
	REG_FUNC(cell_FreeType2, FT_Get_Sfnt_Table);

	REG_FUNC(cell_FreeType2, FT_Glyph_Copy);
	REG_FUNC(cell_FreeType2, FT_Glyph_Get_CBox);
	REG_FUNC(cell_FreeType2, FT_Glyph_To_Bitmap);
	REG_FUNC(cell_FreeType2, FT_Glyph_Transform);

	REG_FUNC(cell_FreeType2, FT_Init_FreeType);
	REG_FUNC(cell_FreeType2, FT_Load_Glyph);
	REG_FUNC(cell_FreeType2, FT_MulFix);

	REG_FUNC(cell_FreeType2, FT_New_Face);
	REG_FUNC(cell_FreeType2, FT_New_Library);
	REG_FUNC(cell_FreeType2, FT_New_Memory_Face);

	REG_FUNC(cell_FreeType2, FT_Open_Face);
	REG_FUNC(cell_FreeType2, FT_Outline_Embolden);
	REG_FUNC(cell_FreeType2, FT_Render_Glyph);
	REG_FUNC(cell_FreeType2, FT_RoundFix);
	REG_FUNC(cell_FreeType2, FT_Select_Charmap);

	REG_FUNC(cell_FreeType2, FT_Set_Char_Size);
	REG_FUNC(cell_FreeType2, FT_Set_Transform);

	REG_FUNC(cell_FreeType2, FT_Vector_Transform);
});