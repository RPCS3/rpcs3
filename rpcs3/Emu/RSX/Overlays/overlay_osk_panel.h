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
			const color4f default_bg = { 0.7f, 0.7f, 0.7f, 1.f };
			const color4f special_bg = { 0.2f, 0.7f, 0.7f, 1.f };
			const color4f special2_bg = { 0.83f, 0.81f, 0.57f, 1.f };

			u32 osk_panel_mode = 0;
			u16 num_rows = 0;
			u16 num_columns = 0;
			u16 cell_size_x = 0;
			u16 cell_size_y = 0;

			std::vector<grid_entry_ctor> layout;

			osk_panel(u32 panel_mode = 0);

		protected:
			std::u32string space;
			std::u32string backspace;
			std::u32string enter; // Return key. Named 'enter' because 'return' is a reserved statement in cpp.
		};

		struct osk_panel_latin : public osk_panel
		{
			osk_panel_latin(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb, u32 osk_panel_mode = CELL_OSKDIALOG_PANELMODE_LATIN);
		};

		struct osk_panel_english : public osk_panel_latin
		{
			osk_panel_english(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_spanish : public osk_panel_latin
		{
			osk_panel_spanish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_italian : public osk_panel_latin
		{
			osk_panel_italian(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_danish : public osk_panel_latin
		{
			osk_panel_danish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_norwegian : public osk_panel_latin
		{
			osk_panel_norwegian(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_dutch : public osk_panel_latin
		{
			osk_panel_dutch(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_swedish : public osk_panel_latin
		{
			osk_panel_swedish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_finnish : public osk_panel_latin
		{
			osk_panel_finnish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_portuguese_pt : public osk_panel_latin
		{
			osk_panel_portuguese_pt(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_portuguese_br : public osk_panel_latin
		{
			osk_panel_portuguese_br(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_french : public osk_panel
		{
			osk_panel_french(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_german : public osk_panel
		{
			osk_panel_german(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_turkey : public osk_panel
		{
			osk_panel_turkey(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_polish : public osk_panel
		{
			osk_panel_polish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_russian : public osk_panel
		{
			osk_panel_russian(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_korean : public osk_panel
		{
			osk_panel_korean(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_chinese : public osk_panel
		{
			osk_panel_chinese(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb, u32 osk_panel_mode);
		};

		struct osk_panel_simplified_chinese : public osk_panel_chinese
		{
			osk_panel_simplified_chinese(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_traditional_chinese : public osk_panel_chinese
		{
			osk_panel_traditional_chinese(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
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

		struct osk_panel_url : public osk_panel
		{
			osk_panel_url(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};

		struct osk_panel_password : public osk_panel_alphabet_half_width
		{
			osk_panel_password(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb);
		};
	}
}
