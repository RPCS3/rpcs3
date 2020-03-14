#pragma once

#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Utilities/geometry.h"

namespace rsx
{
	namespace overlays
	{
		using callback_t = std::function<void(const std::u32string&)>;

		enum button_flags
		{
			_default = 0,
			_return = 1,
			_space = 2,
			_shift = 3,
			_layer = 4
		};

		struct grid_entry_ctor
		{
			// TODO: change to array with layer_mode::layer_count
			std::vector<std::vector<std::u32string>> outputs;
			color4f color;
			u32 num_cell_hz;
			button_flags type_flags;
			callback_t callback;
		};

		struct osk_panel
		{
			u32 osk_panel_mode = 0;
			u32 num_rows = 0;
			u32 num_columns = 0;
			u32 cell_size_x = 0;
			u32 cell_size_y = 0;

			std::vector<grid_entry_ctor> layout;

			osk_panel(u32 panel_mode = 0);
		};

		struct osk_panel_latin : public osk_panel
		{
			osk_panel_latin(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb, u32 osk_panel_mode = CELL_OSKDIALOG_PANELMODE_LATIN);
		};

		struct osk_panel_english : public osk_panel_latin
		{
			osk_panel_english(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_japanese : public osk_panel
		{
			osk_panel_japanese(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_japanese_hiragana : public osk_panel
		{
			osk_panel_japanese_hiragana(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_japanese_katakana : public osk_panel
		{
			osk_panel_japanese_katakana(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_alphabet_half_width : public osk_panel
		{
			osk_panel_alphabet_half_width(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb, u32 osk_panel_mode = CELL_OSKDIALOG_PANELMODE_ALPHABET);
		};

		struct osk_panel_alphabet_full_width : public osk_panel
		{
			osk_panel_alphabet_full_width(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_numeral_half_width : public osk_panel
		{
			osk_panel_numeral_half_width(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_numeral_full_width : public osk_panel
		{
			osk_panel_numeral_full_width(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_url : public osk_panel_alphabet_half_width
		{
			osk_panel_url(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_password : public osk_panel_alphabet_half_width
		{
			osk_panel_password(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};
	}
}
