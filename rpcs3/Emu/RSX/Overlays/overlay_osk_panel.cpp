#include "stdafx.h"
#include "overlay_osk_panel.h"

namespace rsx
{
	namespace overlays
	{
		osk_panel::osk_panel(u32 panel_mode)
		{
			osk_panel_mode = panel_mode;

			// TODO: Use proper translations for Space/Backspace/Return etc. and make sure they fit in the grid.
			switch (panel_mode)
			{
			case CELL_OSKDIALOG_PANELMODE_DEFAULT:
			case CELL_OSKDIALOG_PANELMODE_GERMAN:
			case CELL_OSKDIALOG_PANELMODE_ENGLISH:
			case CELL_OSKDIALOG_PANELMODE_SPANISH:
			case CELL_OSKDIALOG_PANELMODE_FRENCH:
			case CELL_OSKDIALOG_PANELMODE_ITALIAN:
			case CELL_OSKDIALOG_PANELMODE_DUTCH:
			case CELL_OSKDIALOG_PANELMODE_PORTUGUESE:
			case CELL_OSKDIALOG_PANELMODE_RUSSIAN:
			case CELL_OSKDIALOG_PANELMODE_JAPANESE:
			case CELL_OSKDIALOG_PANELMODE_DEFAULT_NO_JAPANESE:
			case CELL_OSKDIALOG_PANELMODE_POLISH:
			case CELL_OSKDIALOG_PANELMODE_KOREAN:
			case CELL_OSKDIALOG_PANELMODE_TURKEY:
			case CELL_OSKDIALOG_PANELMODE_TRADITIONAL_CHINESE:
			case CELL_OSKDIALOG_PANELMODE_SIMPLIFIED_CHINESE:
			case CELL_OSKDIALOG_PANELMODE_PORTUGUESE_BRAZIL:
			case CELL_OSKDIALOG_PANELMODE_DANISH:
			case CELL_OSKDIALOG_PANELMODE_SWEDISH:
			case CELL_OSKDIALOG_PANELMODE_NORWEGIAN:
			case CELL_OSKDIALOG_PANELMODE_FINNISH:
			case CELL_OSKDIALOG_PANELMODE_JAPANESE_HIRAGANA:
			case CELL_OSKDIALOG_PANELMODE_JAPANESE_KATAKANA:
			case CELL_OSKDIALOG_PANELMODE_ALPHABET_FULL_WIDTH:
			case CELL_OSKDIALOG_PANELMODE_ALPHABET:
			case CELL_OSKDIALOG_PANELMODE_LATIN:
			case CELL_OSKDIALOG_PANELMODE_NUMERAL_FULL_WIDTH:
			case CELL_OSKDIALOG_PANELMODE_NUMERAL:
			case CELL_OSKDIALOG_PANELMODE_URL:
			case CELL_OSKDIALOG_PANELMODE_PASSWORD:
			default:
			{
				space = U"Space";
				backspace = U"Backspace";
				enter = U"Return";
			}
			}
		}

		// Language specific implementations

		// TODO: Check and adjust special characters for each panel

		osk_panel_latin::osk_panel_latin(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb, u32 osk_panel_mode)
			: osk_panel(osk_panel_mode)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"à", U"À"}, {U"!", U"¡"}}, default_bg, 1},
				{{{U"2", U"@"}, {U"á", U"Á"}, {U"?", U"¿"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"â", U"Â"}, {U"#", U"~"}}, default_bg, 1},
				{{{U"4", U"$"}, {U"ã", U"Ã"}, {U"$", U"„"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"ä", U"Ä"}, {U"%", U"´"}}, default_bg, 1},
				{{{U"6", U"^"}, {U"å", U"Å"}, {U"&", U"‘"}}, default_bg, 1},
				{{{U"7", U"&"}, {U"æ", U"Æ"}, {U"'", U"’"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"ç", U"Ç"}, {U"(", U"‚"}}, default_bg, 1},
				{{{U"9", U"("}, {U"[", U"<"}, {U")", U"“"}}, default_bg, 1},
				{{{U"0", U")"}, {U"]", U">"}, {U"*", U"”"}}, default_bg, 1},

				// Row 2
				{{{U"q", U"Q"}, {U"è", U"È"}, {U"/", U"¤"}}, default_bg, 1},
				{{{U"w", U"W"}, {U"é", U"É"}, {U"\\", U"¢"}}, default_bg, 1},
				{{{U"e", U"E"}, {U"ê", U"Ê"}, {U"[", U"€"}}, default_bg, 1},
				{{{U"r", U"R"}, {U"ë", U"Ë"}, {U"]", U"£"}}, default_bg, 1},
				{{{U"t", U"T"}, {U"ì", U"Ì"}, {U"^", U"¥"}}, default_bg, 1},
				{{{U"y", U"Y"}, {U"í", U"Í"}, {U"_", U"§"}}, default_bg, 1},
				{{{U"u", U"U"}, {U"î", U"Î"}, {U"`", U"¦"}}, default_bg, 1},
				{{{U"i", U"I"}, {U"ï", U"Ï"}, {U"{", U"µ"}}, default_bg, 1},
				{{{U"o", U"O"}, {U";", U"="}, {U"}", U""}}, default_bg, 1},
				{{{U"p", U"P"}, {U":", U"+"}, {U"|", U""}}, default_bg, 1},

				// Row 3
				{{{U"a", U"A"}, {U"ñ", U"Ñ"}, {U"@", U""}}, default_bg, 1},
				{{{U"s", U"S"}, {U"ò", U"Ò"}, {U"°", U""}}, default_bg, 1},
				{{{U"d", U"D"}, {U"ó", U"Ó"}, {U"‹", U""}}, default_bg, 1},
				{{{U"f", U"F"}, {U"ô", U"Ô"}, {U"›", U""}}, default_bg, 1},
				{{{U"g", U"G"}, {U"õ", U"Õ"}, {U"«", U""}}, default_bg, 1},
				{{{U"h", U"H"}, {U"ö", U"Ö"}, {U"»", U""}}, default_bg, 1},
				{{{U"j", U"J"}, {U"ø", U"Ø"}, {U"ª", U""}}, default_bg, 1},
				{{{U"k", U"K"}, {U"œ", U"Œ"}, {U"º", U""}}, default_bg, 1},
				{{{U"l", U"L"}, {U"`", U"~"}, {U"×", U""}}, default_bg, 1},
				{{{U"'", U"\""}, {U"¡", U"\""}, {U"÷", U""}}, default_bg, 1},

				// Row 4
				{{{U"z", U"Z"}, {U"ß", U"ß"}, {U"+", U""}}, default_bg, 1},
				{{{U"x", U"X"}, {U"ù", U"Ù"}, {U",", U""}}, default_bg, 1},
				{{{U"c", U"C"}, {U"ú", U"Ú"}, {U"-", U""}}, default_bg, 1},
				{{{U"v", U"V"}, {U"û", U"Û"}, {U".", U""}}, default_bg, 1},
				{{{U"b", U"B"}, {U"ü", U"Ü"}, {U"\"", U""}}, default_bg, 1},
				{{{U"n", U"N"}, {U"ý", U"Ý"}, {U":", U""}}, default_bg, 1},
				{{{U"m", U"M"}, {U"ÿ", U"Ÿ"}, {U";", U""}}, default_bg, 1},
				{{{U",", U"-"}, {U",", U"-"}, {U"<", U""}}, default_bg, 1},
				{{{U".", U"_"}, {U".", U"_"}, {U"=", U""}}, default_bg, 1},
				{{{U"?", U"/"}, {U"¿", U"/"}, {U">", U""}}, default_bg, 1},

				// Control
				{{{U"A/a"}, {U"À/à"}, {U"!/¡"}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"ÖÑß"}, {U"@#:"}, {U"ABC"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_english::osk_panel_english(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_ENGLISH)
		{
			// English and latin should be mostly the same.
		}

		osk_panel_spanish::osk_panel_spanish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_SPANISH)
		{
			// Spanish and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_italian::osk_panel_italian(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_ITALIAN)
		{
			// Italian and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_danish::osk_panel_danish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_DANISH)
		{
			// Danish and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_norwegian::osk_panel_norwegian(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_NORWEGIAN)
		{
			// Norwegian and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_dutch::osk_panel_dutch(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_DUTCH)
		{
			// Dutch and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_swedish::osk_panel_swedish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_SWEDISH)
		{
			// Swedish and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_finnish::osk_panel_finnish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_FINNISH)
		{
			// Finnish and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_portuguese_pt::osk_panel_portuguese_pt(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_PORTUGUESE)
		{
			// Portuguese (Portugal) and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_portuguese_br::osk_panel_portuguese_br(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_PORTUGUESE_BRAZIL)
		{
			// Portuguese (Brazil) and latin should be mostly the same. Only the translation for the controls should be different.
		}

		osk_panel_french::osk_panel_french(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_FRENCH)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"à", U"À"}, {U"!", U"¡"}}, default_bg, 1},
				{{{U"2", U"@"}, {U"á", U"Á"}, {U"?", U"¿"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"â", U"Â"}, {U"#", U"~"}}, default_bg, 1},
				{{{U"4", U"$"}, {U"ã", U"Ã"}, {U"$", U"„"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"ä", U"Ä"}, {U"%", U"´"}}, default_bg, 1},
				{{{U"6", U"^"}, {U"å", U"Å"}, {U"&", U"‘"}}, default_bg, 1},
				{{{U"7", U"&"}, {U"æ", U"Æ"}, {U"'", U"’"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"ç", U"Ç"}, {U"(", U"‚"}}, default_bg, 1},
				{{{U"9", U"("}, {U"[", U"<"}, {U")", U"“"}}, default_bg, 1},
				{{{U"0", U")"}, {U"]", U">"}, {U"*", U"”"}}, default_bg, 1},

				// Row 2
				{{{U"a", U"A"}, {U"è", U"È"}, {U"/", U"¤"}}, default_bg, 1},
				{{{U"z", U"Z"}, {U"é", U"É"}, {U"\\", U"¢"}}, default_bg, 1},
				{{{U"e", U"E"}, {U"ê", U"Ê"}, {U"[", U"€"}}, default_bg, 1},
				{{{U"r", U"R"}, {U"ë", U"Ë"}, {U"]", U"£"}}, default_bg, 1},
				{{{U"t", U"T"}, {U"ì", U"Ì"}, {U"^", U"¥"}}, default_bg, 1},
				{{{U"y", U"Y"}, {U"í", U"Í"}, {U"_", U"§"}}, default_bg, 1},
				{{{U"u", U"U"}, {U"î", U"Î"}, {U"`", U"¦"}}, default_bg, 1},
				{{{U"i", U"I"}, {U"ï", U"Ï"}, {U"{", U"µ"}}, default_bg, 1},
				{{{U"o", U"O"}, {U";", U"="}, {U"}", U""}}, default_bg, 1},
				{{{U"p", U"P"}, {U":", U"+"}, {U"|", U""}}, default_bg, 1},

				// Row 3
				{{{U"q", U"Q"}, {U"ñ", U"Ñ"}, {U"@", U""}}, default_bg, 1},
				{{{U"s", U"S"}, {U"ò", U"Ò"}, {U"°", U""}}, default_bg, 1},
				{{{U"d", U"D"}, {U"ó", U"Ó"}, {U"‹", U""}}, default_bg, 1},
				{{{U"f", U"F"}, {U"ô", U"Ô"}, {U"›", U""}}, default_bg, 1},
				{{{U"g", U"G"}, {U"õ", U"Õ"}, {U"«", U""}}, default_bg, 1},
				{{{U"h", U"H"}, {U"ö", U"Ö"}, {U"»", U""}}, default_bg, 1},
				{{{U"j", U"J"}, {U"ø", U"Ø"}, {U"ª", U""}}, default_bg, 1},
				{{{U"k", U"K"}, {U"œ", U"Œ"}, {U"º", U""}}, default_bg, 1},
				{{{U"l", U"L"}, {U"`", U"~"}, {U"×", U""}}, default_bg, 1},
				{{{U"'", U"\""}, {U"¡", U"\""}, {U"÷", U""}}, default_bg, 1},

				// Row 4
				{{{U"w", U"W"}, {U"ß", U"ß"}, {U"+", U""}}, default_bg, 1},
				{{{U"x", U"X"}, {U"ù", U"Ù"}, {U",", U""}}, default_bg, 1},
				{{{U"c", U"C"}, {U"ú", U"Ú"}, {U"-", U""}}, default_bg, 1},
				{{{U"v", U"V"}, {U"û", U"Û"}, {U".", U""}}, default_bg, 1},
				{{{U"b", U"B"}, {U"ü", U"Ü"}, {U"\"", U""}}, default_bg, 1},
				{{{U"n", U"N"}, {U"ý", U"Ý"}, {U":", U""}}, default_bg, 1},
				{{{U"m", U"M"}, {U"ÿ", U"Ÿ"}, {U";", U""}}, default_bg, 1},
				{{{U",", U"-"}, {U",", U"-"}, {U"<", U""}}, default_bg, 1},
				{{{U".", U"_"}, {U".", U"_"}, {U"=", U""}}, default_bg, 1},
				{{{U"?", U"/"}, {U"¿", U"/"}, {U">", U""}}, default_bg, 1},

				// Control
				{{{U"A/a"}, {U"À/à"}, {U"!/¡"}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"ÖÑß"}, {U"@#:"}, {U"ABC"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_german::osk_panel_german(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_GERMAN)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"à", U"À"}, {U"!", U"¡"}}, default_bg, 1},
				{{{U"2", U"@"}, {U"á", U"Á"}, {U"?", U"¿"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"â", U"Â"}, {U"#", U"~"}}, default_bg, 1},
				{{{U"4", U"$"}, {U"ã", U"Ã"}, {U"$", U"„"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"ä", U"Ä"}, {U"%", U"´"}}, default_bg, 1},
				{{{U"6", U"^"}, {U"å", U"Å"}, {U"&", U"‘"}}, default_bg, 1},
				{{{U"7", U"&"}, {U"æ", U"Æ"}, {U"'", U"’"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"ç", U"Ç"}, {U"(", U"‚"}}, default_bg, 1},
				{{{U"9", U"("}, {U"[", U"<"}, {U")", U"“"}}, default_bg, 1},
				{{{U"0", U")"}, {U"]", U">"}, {U"*", U"”"}}, default_bg, 1},

				// Row 2
				{{{U"q", U"Q"}, {U"è", U"È"}, {U"/", U"¤"}}, default_bg, 1},
				{{{U"w", U"W"}, {U"é", U"É"}, {U"\\", U"¢"}}, default_bg, 1},
				{{{U"e", U"E"}, {U"ê", U"Ê"}, {U"[", U"€"}}, default_bg, 1},
				{{{U"r", U"R"}, {U"ë", U"Ë"}, {U"]", U"£"}}, default_bg, 1},
				{{{U"t", U"T"}, {U"ì", U"Ì"}, {U"^", U"¥"}}, default_bg, 1},
				{{{U"z", U"Z"}, {U"í", U"Í"}, {U"_", U"§"}}, default_bg, 1},
				{{{U"u", U"U"}, {U"î", U"Î"}, {U"`", U"¦"}}, default_bg, 1},
				{{{U"i", U"I"}, {U"ï", U"Ï"}, {U"{", U"µ"}}, default_bg, 1},
				{{{U"o", U"O"}, {U";", U"="}, {U"}", U""}}, default_bg, 1},
				{{{U"p", U"P"}, {U":", U"+"}, {U"|", U""}}, default_bg, 1},

				// Row 3
				{{{U"a", U"A"}, {U"ñ", U"Ñ"}, {U"@", U""}}, default_bg, 1},
				{{{U"s", U"S"}, {U"ò", U"Ò"}, {U"°", U""}}, default_bg, 1},
				{{{U"d", U"D"}, {U"ó", U"Ó"}, {U"‹", U""}}, default_bg, 1},
				{{{U"f", U"F"}, {U"ô", U"Ô"}, {U"›", U""}}, default_bg, 1},
				{{{U"g", U"G"}, {U"õ", U"Õ"}, {U"«", U""}}, default_bg, 1},
				{{{U"h", U"H"}, {U"ö", U"Ö"}, {U"»", U""}}, default_bg, 1},
				{{{U"j", U"J"}, {U"ø", U"Ø"}, {U"ª", U""}}, default_bg, 1},
				{{{U"k", U"K"}, {U"œ", U"Œ"}, {U"º", U""}}, default_bg, 1},
				{{{U"l", U"L"}, {U"`", U"~"}, {U"×", U""}}, default_bg, 1},
				{{{U"'", U"\""}, {U"¡", U"\""}, {U"÷", U""}}, default_bg, 1},

				// Row 4
				{{{U"y", U"Y"}, {U"ß", U"ß"}, {U"+", U""}}, default_bg, 1},
				{{{U"x", U"X"}, {U"ù", U"Ù"}, {U",", U""}}, default_bg, 1},
				{{{U"c", U"C"}, {U"ú", U"Ú"}, {U"-", U""}}, default_bg, 1},
				{{{U"v", U"V"}, {U"û", U"Û"}, {U".", U""}}, default_bg, 1},
				{{{U"b", U"B"}, {U"ü", U"Ü"}, {U"\"", U""}}, default_bg, 1},
				{{{U"n", U"N"}, {U"ý", U"Ý"}, {U":", U""}}, default_bg, 1},
				{{{U"m", U"M"}, {U"ÿ", U"Ÿ"}, {U";", U""}}, default_bg, 1},
				{{{U",", U"-"}, {U",", U"-"}, {U"<", U""}}, default_bg, 1},
				{{{U".", U"_"}, {U".", U"_"}, {U"=", U""}}, default_bg, 1},
				{{{U"?", U"/"}, {U"¿", U"/"}, {U">", U""}}, default_bg, 1},

				// Control
				{{{U"A/a"}, {U"À/à"}, {U"!/¡"}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"ÖÑß"}, {U"@#:"}, {U"ABC"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_turkey::osk_panel_turkey(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_TURKEY)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"1", U"!"}, {U"!", U"¡"}}, default_bg, 1},
				{{{U"2", U"@"}, {U"2", U"@"}, {U"?", U"¿"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"3", U"#"}, {U"#", U"~"}}, default_bg, 1},
				{{{U"4", U"$"}, {U"4", U"$"}, {U"$", U"„"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"5", U"%"}, {U"%", U"´"}}, default_bg, 1},
				{{{U"6", U"^"}, {U"6", U"^"}, {U"&", U"‘"}}, default_bg, 1},
				{{{U"7", U"&"}, {U"7", U"&"}, {U"'", U"’"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"8", U"*"}, {U"(", U"‚"}}, default_bg, 1},
				{{{U"9", U"("}, {U"9", U"("}, {U")", U"“"}}, default_bg, 1},
				{{{U"0", U")"}, {U"0", U")"}, {U"*", U"”"}}, default_bg, 1},

				// Row 2
				{{{U"q", U"Q"}, {U"q", U"Q"}, {U"/", U"¤"}}, default_bg, 1},
				{{{U"w", U"W"}, {U"w", U"W"}, {U"\\", U"¢"}}, default_bg, 1},
				{{{U"e", U"€"}, {U"e", U"€"}, {U"[", U"€"}}, default_bg, 1},
				{{{U"r", U"R"}, {U"r", U"R"}, {U"]", U"£"}}, default_bg, 1},
				{{{U"t", U"T"}, {U"t", U"T"}, {U"^", U"¥"}}, default_bg, 1},
				{{{U"y", U"Y"}, {U"y", U"Y"}, {U"_", U"§"}}, default_bg, 1},
				{{{U"u", U"U"}, {U"ü", U"Ü"}, {U"`", U"¦"}}, default_bg, 1},
				{{{U"i", U"I"}, {U"ı", U"İ"}, {U"{", U"µ"}}, default_bg, 1}, // I couldn't find ı and İ in the PS3 OSK, but I'll put them here anyway
				{{{U"o", U"O"}, {U"ö", U"Ö"}, {U"}", U""}}, default_bg, 1},
				{{{U"p", U"P"}, {U"p", U"P"}, {U"|", U""}}, default_bg, 1},

				// Row 3
				{{{U"a", U"A"}, {U"a", U"A"}, {U"@", U""}}, default_bg, 1},
				{{{U"s", U"S"}, {U"ş", U"Ş"}, {U"°", U""}}, default_bg, 1},
				{{{U"d", U"D"}, {U"d", U"D"}, {U"‹", U""}}, default_bg, 1},
				{{{U"f", U"F"}, {U"f", U"F"}, {U"›", U""}}, default_bg, 1},
				{{{U"g", U"G"}, {U"ğ", U"Ğ"}, {U"«", U""}}, default_bg, 1},
				{{{U"h", U"H"}, {U"h", U"H"}, {U"»", U""}}, default_bg, 1},
				{{{U"j", U"J"}, {U"j", U"J"}, {U"ª", U""}}, default_bg, 1},
				{{{U"k", U"K"}, {U"k", U"K"}, {U"º", U""}}, default_bg, 1},
				{{{U"l", U"L"}, {U"l", U"L"}, {U"×", U""}}, default_bg, 1},
				{{{U"'", U"\""}, {U"'", U"\""}, {U"÷", U""}}, default_bg, 1},

				// Row 4
				{{{U"z", U"Z"}, {U"z", U"Z"}, {U"+", U""}}, default_bg, 1},
				{{{U"x", U"X"}, {U"x", U"X"}, {U",", U""}}, default_bg, 1},
				{{{U"c", U"C"}, {U"ç", U"Ç"}, {U"-", U""}}, default_bg, 1},
				{{{U"v", U"V"}, {U"v", U"V"}, {U".", U""}}, default_bg, 1},
				{{{U"b", U"B"}, {U"b", U"B"}, {U"\"", U""}}, default_bg, 1},
				{{{U"n", U"N"}, {U"n", U"N"}, {U":", U""}}, default_bg, 1},
				{{{U"m", U"M"}, {U"m", U"M"}, {U";", U""}}, default_bg, 1},
				{{{U",", U"-"}, {U",", U"-"}, {U"<", U""}}, default_bg, 1},
				{{{U".", U"_"}, {U".", U"_"}, {U"=", U""}}, default_bg, 1},
				{{{U"?", U"/"}, {U"?", U"/"}, {U">", U""}}, default_bg, 1},

				// Control
				{{{U"A/a"}, {U"A/a"}, {U"!/¡"}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"AltGr"}, {U"@#:"}, {U"ABC"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_polish::osk_panel_polish(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_POLISH)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"1", U"!"}, {U"!", U"¡"}}, default_bg, 1},
				{{{U"2", U"@"}, {U"2", U"@"}, {U"?", U"¿"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"3", U"#"}, {U"#", U"~"}}, default_bg, 1},
				{{{U"4", U"$"}, {U"4", U"$"}, {U"$", U"„"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"5", U"%"}, {U"%", U"´"}}, default_bg, 1},
				{{{U"6", U"^"}, {U"6", U"^"}, {U"&", U"‘"}}, default_bg, 1},
				{{{U"7", U"&"}, {U"7", U"&"}, {U"'", U"’"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"8", U"*"}, {U"(", U"‚"}}, default_bg, 1},
				{{{U"9", U"("}, {U"9", U"("}, {U")", U"“"}}, default_bg, 1},
				{{{U"0", U")"}, {U"0", U")"}, {U"*", U"”"}}, default_bg, 1},

				// Row 2
				{{{U"q", U"Q"}, {U"q", U"Q"}, {U"/", U"¤"}}, default_bg, 1},
				{{{U"w", U"W"}, {U"w", U"W"}, {U"\\", U"¢"}}, default_bg, 1},
				{{{U"e", U"E"}, {U"ę", U"Ę"}, {U"[", U"€"}}, default_bg, 1},
				{{{U"r", U"R"}, {U"r", U"R"}, {U"]", U"£"}}, default_bg, 1},
				{{{U"t", U"T"}, {U"t", U"T"}, {U"^", U"¥"}}, default_bg, 1},
				{{{U"y", U"Y"}, {U"y", U"Y"}, {U"_", U"§"}}, default_bg, 1},
				{{{U"u", U"U"}, {U"€", U"€"}, {U"`", U"¦"}}, default_bg, 1},
				{{{U"i", U"I"}, {U"i", U"I"}, {U"{", U"µ"}}, default_bg, 1},
				{{{U"o", U"O"}, {U"ó", U"Ó"}, {U"}", U""}}, default_bg, 1},
				{{{U"p", U"P"}, {U"p", U"P"}, {U"|", U""}}, default_bg, 1},

				// Row 3
				{{{U"a", U"A"}, {U"ą", U"Ą"}, {U"@", U""}}, default_bg, 1},
				{{{U"s", U"S"}, {U"ś", U"Ś"}, {U"°", U""}}, default_bg, 1},
				{{{U"d", U"D"}, {U"d", U"D"}, {U"‹", U""}}, default_bg, 1},
				{{{U"f", U"F"}, {U"f", U"F"}, {U"›", U""}}, default_bg, 1},
				{{{U"g", U"G"}, {U"g", U"G"}, {U"«", U""}}, default_bg, 1},
				{{{U"h", U"H"}, {U"h", U"H"}, {U"»", U""}}, default_bg, 1},
				{{{U"j", U"J"}, {U"j", U"J"}, {U"ª", U""}}, default_bg, 1},
				{{{U"k", U"K"}, {U"k", U"K"}, {U"º", U""}}, default_bg, 1},
				{{{U"l", U"L"}, {U"ł", U"Ł"}, {U"×", U""}}, default_bg, 1},
				{{{U"'", U"\""}, {U"'", U"\""}, {U"÷", U""}}, default_bg, 1},

				// Row 4
				{{{U"z", U"Z"}, {U"ż", U"Ż"}, {U"+", U""}}, default_bg, 1},
				{{{U"x", U"X"}, {U"ź", U"Ź"}, {U",", U""}}, default_bg, 1},
				{{{U"c", U"C"}, {U"ć", U"Ć"}, {U"-", U""}}, default_bg, 1},
				{{{U"v", U"V"}, {U"v", U"V"}, {U".", U""}}, default_bg, 1},
				{{{U"b", U"B"}, {U"b", U"B"}, {U"\"", U""}}, default_bg, 1},
				{{{U"n", U"N"}, {U"ń", U"Ń"}, {U":", U""}}, default_bg, 1},
				{{{U"m", U"M"}, {U"m", U"M"}, {U";", U""}}, default_bg, 1},
				{{{U",", U"-"}, {U",", U"-"}, {U"<", U""}}, default_bg, 1},
				{{{U".", U"_"}, {U".", U"_"}, {U"=", U""}}, default_bg, 1},
				{{{U"?", U"/"}, {U"?", U"/"}, {U">", U""}}, default_bg, 1},

				// Control
				{{{U"A/a"}, {U"Ą/ą"}, {U"!/¡"}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"AltGr"}, {U"@#:"}, {U"ABC"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_russian::osk_panel_russian(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_RUSSIAN)
		{
			num_rows = 6;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"1", U"!"}, {U"à", U"À"}, {U"!", U"¡"}}, default_bg, 1},
				{{{U"2", U"\""}, {U"2", U"@"}, {U"á", U"Á"}, {U"?", U"¿"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"3", U"#"}, {U"â", U"Â"}, {U"#", U"~"}}, default_bg, 1},
				{{{U"4", U";"}, {U"4", U"$"}, {U"ã", U"Ã"}, {U"$", U"„"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"5", U"%"}, {U"ä", U"Ä"}, {U"%", U"´"}}, default_bg, 1},
				{{{U"6", U":"}, {U"6", U"^"}, {U"å", U"Å"}, {U"&", U"‘"}}, default_bg, 1},
				{{{U"7", U"?"}, {U"7", U"&"}, {U"æ", U"Æ"}, {U"'", U"’"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"8", U"*"}, {U"ç", U"Ç"}, {U"(", U"‚"}}, default_bg, 1},
				{{{U"9", U"("}, {U"9", U"("}, {U"[", U"<"}, {U")", U"“"}}, default_bg, 1},
				{{{U"0", U")"}, {U"0", U")"}, {U"]", U">"}, {U"*", U"”"}}, default_bg, 1},

				// Row 2
				{{{U"ё", U"Ё"}, {U"q", U"Q"}, {U"è", U"È"}, {U"/", U"¤"}}, default_bg, 1},
				{{{U"'", U"@"}, {U"w", U"W"}, {U"é", U"É"}, {U"\\", U"¢"}}, default_bg, 1},
				{{{U",", U"$"}, {U"e", U"E"}, {U"ê", U"Ê"}, {U"[", U"€"}}, default_bg, 1},
				{{{U".", U"^"}, {U"r", U"R"}, {U"ë", U"Ë"}, {U"]", U"£"}}, default_bg, 1},
				{{{U"?", U"&"}, {U"t", U"T"}, {U"ì", U"Ì"}, {U"^", U"¥"}}, default_bg, 1},
				{{{U"!", U"/"}, {U"y", U"Y"}, {U"í", U"Í"}, {U"_", U"§"}}, default_bg, 1},
				{{{U"-", U"_"}, {U"u", U"U"}, {U"î", U"Î"}, {U"`", U"¦"}}, default_bg, 1},
				{{{U"=", U"+"}, {U"i", U"I"}, {U"ï", U"Ï"}, {U"{", U"µ"}}, default_bg, 1},
				{{{U"х", U"Х"}, {U"o", U"O"}, {U";", U"="}, {U"}", U""}}, default_bg, 1},
				{{{U"ъ", U"Ъ"}, {U"p", U"P"}, {U":", U"+"}, {U"|", U""}}, default_bg, 1},

				// Row 3
				{{{U"й", U"Й"}, {U"a", U"A"}, {U"ñ", U"Ñ"}, {U"@", U""}}, default_bg, 1},
				{{{U"ц", U"Ц"}, {U"s", U"S"}, {U"ò", U"Ò"}, {U"°", U""}}, default_bg, 1},
				{{{U"у", U"У"}, {U"d", U"D"}, {U"ó", U"Ó"}, {U"‹", U""}}, default_bg, 1},
				{{{U"к", U"К"}, {U"f", U"F"}, {U"ô", U"Ô"}, {U"›", U""}}, default_bg, 1},
				{{{U"е", U"Е"}, {U"g", U"G"}, {U"õ", U"Õ"}, {U"«", U""}}, default_bg, 1},
				{{{U"н", U"Н"}, {U"h", U"H"}, {U"ö", U"Ö"}, {U"»", U""}}, default_bg, 1},
				{{{U"г", U"Г"}, {U"j", U"J"}, {U"ø", U"Ø"}, {U"ª", U""}}, default_bg, 1},
				{{{U"ш", U"Ш"}, {U"k", U"K"}, {U"œ", U"Œ"}, {U"º", U""}}, default_bg, 1},
				{{{U"щ", U"Щ"}, {U"l", U"L"}, {U"`", U"~"}, {U"×", U""}}, default_bg, 1},
				{{{U"з", U"З"}, {U"'", U"\""}, {U"¡", U"\""}, {U"÷", U""}}, default_bg, 1},

				// Row 4
				{{{U"ф", U"Ф"}, {U"z", U"Z"}, {U"ß", U"ß"}, {U"+", U""}}, default_bg, 1},
				{{{U"ы", U"Ы"}, {U"x", U"X"}, {U"ù", U"Ù"}, {U",", U""}}, default_bg, 1},
				{{{U"в", U"В"}, {U"c", U"C"}, {U"ú", U"Ú"}, {U"-", U""}}, default_bg, 1},
				{{{U"а", U"А"}, {U"v", U"V"}, {U"û", U"Û"}, {U".", U""}}, default_bg, 1},
				{{{U"п", U"П"}, {U"b", U"B"}, {U"ü", U"Ü"}, {U"\"", U""}}, default_bg, 1},
				{{{U"р", U"Р"}, {U"n", U"N"}, {U"ý", U"Ý"}, {U":", U""}}, default_bg, 1},
				{{{U"о", U"О"}, {U"m", U"M"}, {U"ÿ", U"Ÿ"}, {U";", U""}}, default_bg, 1},
				{{{U"л", U"Л"}, {U",", U"-"}, {U",", U"-"}, {U"<", U""}}, default_bg, 1},
				{{{U"д", U"Д"}, {U".", U"_"}, {U".", U"_"}, {U"=", U""}}, default_bg, 1},
				{{{U"Ж", U"ж"}, {U"?", U"/"}, {U"¿", U"/"}, {U">", U""}}, default_bg, 1},

				// Row 5
				{{{U"я", U"Я"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"ч", U"Ч"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"с", U"С"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"м", U"М"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"и", U"И"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"т", U"Т"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"ь", U"Ь"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"б", U"Б"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"ю", U"Ю"}, {U""}, {U""}, {U""}}, default_bg, 1},
				{{{U"э", U"Э"}, {U""}, {U""}, {U""}}, default_bg, 1},

				// Control
				{{{U"У/у"}, {U"A/a"}, {U"À/à"}, {U"!/¡"}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"ABC"}, {U"ÖÑß"}, {U"@#:"}, {U"РУ"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_korean::osk_panel_korean(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_KOREAN)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"ㅃ"}, {U"1", U"!"}, {U"!"}}, default_bg, 1},
				{{{U"ㅉ"}, {U"2", U"@"}, {U"\""}}, default_bg, 1},
				{{{U"ㄸ"}, {U"3", U"#"}, {U"#"}}, default_bg, 1},
				{{{U"ㄲ"}, {U"4", U"$"}, {U"$"}}, default_bg, 1},
				{{{U"ㅆ"}, {U"5", U"%"}, {U"%"}}, default_bg, 1},
				{{{U"^"}, {U"6", U"^"}, {U"&"}}, default_bg, 1},
				{{{U"*"}, {U"7", U"&"}, {U"'"}}, default_bg, 1},
				{{{U"-"}, {U"8", U"*"}, {U"("}}, default_bg, 1},
				{{{U"ㅒ"}, {U"9", U"("}, {U")"}}, default_bg, 1},
				{{{U"ㅖ"}, {U"0", U")"}, {U"*"}}, default_bg, 1},

				// Row 2
				{{{U"ㅂ"}, {U"q", U"Q"}, {U"+"}}, default_bg, 1},
				{{{U"ㅈ"}, {U"w", U"W"}, {U","}}, default_bg, 1},
				{{{U"ㄷ"}, {U"e", U"E"}, {U"-"}}, default_bg, 1},
				{{{U"ㄱ"}, {U"r", U"R"}, {U"."}}, default_bg, 1},
				{{{U"ㅅ"}, {U"t", U"T"}, {U"/"}}, default_bg, 1},
				{{{U"ㅛ"}, {U"y", U"Y"}, {U":"}}, default_bg, 1},
				{{{U"ㅕ"}, {U"u", U"U"}, {U";"}}, default_bg, 1},
				{{{U"ㅑ"}, {U"i", U"I"}, {U"<"}}, default_bg, 1},
				{{{U"ㅐ"}, {U"o", U"O"}, {U"="}}, default_bg, 1},
				{{{U"ㅔ"}, {U"p", U"P"}, {U">"}}, default_bg, 1},

				// Row 3
				{{{U"ㅁ"}, {U"a", U"A"}, {U"?"}}, default_bg, 1},
				{{{U"ㄴ"}, {U"s", U"S"}, {U"@"}}, default_bg, 1},
				{{{U"ㅇ"}, {U"d", U"D"}, {U"["}}, default_bg, 1},
				{{{U"ㄹ"}, {U"f", U"F"}, {U"\\"}}, default_bg, 1},
				{{{U"ㅎ"}, {U"g", U"G"}, {U"]"}}, default_bg, 1},
				{{{U"ㅗ"}, {U"h", U"H"}, {U"^"}}, default_bg, 1},
				{{{U"ㅓ"}, {U"j", U"J"}, {U"_"}}, default_bg, 1},
				{{{U"ㅏ"}, {U"k", U"K"}, {U"`"}}, default_bg, 1},
				{{{U"ㅣ"}, {U"l", U"L"}, {U"{"}}, default_bg, 1},
				{{{U";"}, {U"'", U"\""}, {U"}"}}, default_bg, 1},

				// Row 4
				{{{U"ㅋ"}, {U"z", U"Z"}, {U"|"}}, default_bg, 1},
				{{{U"ㅌ"}, {U"x", U"X"}, {U"~"}}, default_bg, 1},
				{{{U"ㅊ"}, {U"c", U"C"}, {U"₩"}}, default_bg, 1},
				{{{U"ㅍ"}, {U"v", U"V"}, {U""}}, default_bg, 1},
				{{{U"ㅠ"}, {U"b", U"B"}, {U""}}, default_bg, 1},
				{{{U"ㅜ"}, {U"n", U"N"}, {U""}}, default_bg, 1},
				{{{U"ㅡ"}, {U"m", U"M"}, {U""}}, default_bg, 1},
				{{{U","}, {U",", U"-"}, {U""}}, default_bg, 1},
				{{{U"."}, {U".", U"_"}, {U""}}, default_bg, 1},
				{{{U"?"}, {U"?", U"/"}, {U""}}, default_bg, 1},

				// Control
				{{{U""}, {U"A/a"}, {U""}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"ABC"}, {U"@#:"}, {U"가"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_chinese::osk_panel_chinese(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb, u32 osk_panel_mode)
			: osk_panel(osk_panel_mode)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			// TODO: add mode for chinese dictionaries

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"１", U"！"}, {U"，"}}, default_bg, 1},
				{{{U"2", U"@"}, {U"２", U"＠"}, {U"．"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"３", U"＃"}, {U"："}}, default_bg, 1},
				{{{U"4", U"$"}, {U"４", U"＄"}, {U"；"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"５", U"％"}, {U"！"}}, default_bg, 1},
				{{{U"6", U"^"}, {U"６", U"＾"}, {U"？"}}, default_bg, 1},
				{{{U"7", U"&"}, {U"７", U"＆"}, {U"＂"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"８", U"＊"}, {U"＇"}}, default_bg, 1},
				{{{U"9", U"("}, {U"９", U"（"}, {U"｀"}}, default_bg, 1},
				{{{U"0", U")"}, {U"０", U"）"}, {U"＾"}}, default_bg, 1},

				// Row 2
				{{{U"q", U"Q"}, {U"ｑ", U"Ｑ"}, {U"～"}}, default_bg, 1},
				{{{U"w", U"W"}, {U"ｗ", U"Ｗ"}, {U"＿"}}, default_bg, 1},
				{{{U"e", U"E"}, {U"ｅ", U"Ｅ"}, {U"＆"}}, default_bg, 1},
				{{{U"r", U"R"}, {U"ｒ", U"Ｒ"}, {U"＠"}}, default_bg, 1},
				{{{U"t", U"T"}, {U"ｔ", U"Ｔ"}, {U"＃"}}, default_bg, 1},
				{{{U"y", U"Y"}, {U"ｙ", U"Ｙ"}, {U"％"}}, default_bg, 1},
				{{{U"u", U"U"}, {U"ｕ", U"Ｕ"}, {U"＋"}}, default_bg, 1},
				{{{U"i", U"I"}, {U"ｉ", U"Ｉ"}, {U"－"}}, default_bg, 1},
				{{{U"o", U"O"}, {U"ｏ", U"Ｏ"}, {U"＊"}}, default_bg, 1},
				{{{U"p", U"P"}, {U"ｐ", U"Ｐ"}, {U"･"}}, default_bg, 1},

				// Row 3
				{{{U"a", U"A"}, {U"ａ", U"Ａ"}, {U"＜"}}, default_bg, 1},
				{{{U"s", U"S"}, {U"ｓ", U"Ｓ"}, {U"＞"}}, default_bg, 1},
				{{{U"d", U"D"}, {U"ｄ", U"Ｄ"}, {U"（"}}, default_bg, 1},
				{{{U"f", U"F"}, {U"ｆ", U"Ｆ"}, {U"）"}}, default_bg, 1},
				{{{U"g", U"G"}, {U"ｇ", U"Ｇ"}, {U"［"}}, default_bg, 1},
				{{{U"h", U"H"}, {U"ｈ", U"Ｈ"}, {U"］"}}, default_bg, 1},
				{{{U"j", U"J"}, {U"ｊ", U"Ｊ"}, {U"｛"}}, default_bg, 1},
				{{{U"k", U"K"}, {U"ｋ", U"Ｋ"}, {U"｝"}}, default_bg, 1},
				{{{U"l", U"L"}, {U"ｌ", U"Ｌ"}, {U"｢"}}, default_bg, 1},
				{{{U"'", U"\""}, {U"＇", U"＂"}, {U"｣"}}, default_bg, 1},

				// Row 4
				{{{U"z", U"Z"}, {U"ｚ", U"Ｚ"}, {U"＝"}}, default_bg, 1},
				{{{U"x", U"X"}, {U"ｘ", U"Ｘ"}, {U"｜"}}, default_bg, 1},
				{{{U"c", U"C"}, {U"ｃ", U"Ｃ"}, {U"｡"}}, default_bg, 1},
				{{{U"v", U"V"}, {U"ｖ", U"Ｖ"}, {U"／"}}, default_bg, 1},
				{{{U"b", U"B"}, {U"ｂ", U"Ｂ"}, {U"＼"}}, default_bg, 1},
				{{{U"n", U"N"}, {U"ｎ", U"Ｎ"}, {U"￢"}}, default_bg, 1},
				{{{U"m", U"M"}, {U"ｍ", U"Ｍ"}, {U"＄"}}, default_bg, 1},
				{{{U",", U"-"}, {U"，", U"－"}, {U"￥"}}, default_bg, 1},
				{{{U".", U"_"}, {U"．", U"＿"}, {U"､"}}, default_bg, 1},
				{{{U"?", U"/"}, {U"？", U"／"}, {U""}}, default_bg, 1},

				// Control
				{{{U"A/a"}, {U"Ａ/ａ"}, {U""}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"全半"}, {U"＠％"}, {U"abc"}}, special2_bg, 2, button_flags::_layer, layer_cb},
				{{{space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb},
				{{{backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_simplified_chinese::osk_panel_simplified_chinese(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_chinese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_SIMPLIFIED_CHINESE)
		{
			// Simplified chinese uses osk_panel_chinese. Only the translation for the controls and the dictionary should be different than for traditional chinese.
		}

		osk_panel_traditional_chinese::osk_panel_traditional_chinese(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_chinese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_TRADITIONAL_CHINESE)
		{
			// Traditional chinese uses osk_panel_chinese. Only the translation for the controls and the dictionary should be different than for simplified chinese.
		}

		osk_panel_japanese::osk_panel_japanese(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_JAPANESE)
		{
			num_rows = 6;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"あ", U"ア"}, {U"1", U"!"}, {U"１", U"！"}, {U"，"}}, default_bg, 1},
				{{{U"か", U"カ"}, {U"2", U"@"}, {U"２", U"＠"}, {U"．"}}, default_bg, 1},
				{{{U"さ", U"サ"}, {U"3", U"#"}, {U"３", U"＃"}, {U"："}}, default_bg, 1},
				{{{U"た", U"タ"}, {U"4", U"$"}, {U"４", U"＄"}, {U"；"}}, default_bg, 1},
				{{{U"な", U"ナ"}, {U"5", U"%"}, {U"５", U"％"}, {U"！"}}, default_bg, 1},
				{{{U"は", U"ハ"}, {U"6", U"^"}, {U"６", U"＾"}, {U"？"}}, default_bg, 1},
				{{{U"ま", U"マ"}, {U"7", U"&"}, {U"７", U"＆"}, {U"＂"}}, default_bg, 1},
				{{{U"や", U"ヤ"}, {U"8", U"*"}, {U"８", U"＊"}, {U"＇"}}, default_bg, 1},
				{{{U"ら", U"ラ"}, {U"9", U"("}, {U"９", U"（"}, {U"｀"}}, default_bg, 1},
				{{{U"わ", U"ワ"}, {U"0", U")"}, {U"０", U"）"}, {U"＾"}}, default_bg, 1},

				// Row 2
				{{{U"い", U"イ"}, {U"q", U"Q"}, {U"ｑ", U"Ｑ"}, {U"～"}}, default_bg, 1},
				{{{U"き", U"キ"}, {U"w", U"W"}, {U"ｗ", U"Ｗ"}, {U"＿"}}, default_bg, 1},
				{{{U"し", U"シ"}, {U"e", U"E"}, {U"ｅ", U"Ｅ"}, {U"＆"}}, default_bg, 1},
				{{{U"ち", U"チ"}, {U"r", U"R"}, {U"ｒ", U"Ｒ"}, {U"＠"}}, default_bg, 1},
				{{{U"に", U"ニ"}, {U"t", U"T"}, {U"ｔ", U"Ｔ"}, {U"＃"}}, default_bg, 1},
				{{{U"ひ", U"ヒ"}, {U"y", U"Y"}, {U"ｙ", U"Ｙ"}, {U"％"}}, default_bg, 1},
				{{{U"み", U"ミ"}, {U"u", U"U"}, {U"ｕ", U"Ｕ"}, {U"＋"}}, default_bg, 1},
				{{{U"ゆ", U"ユ"}, {U"i", U"I"}, {U"ｉ", U"Ｉ"}, {U"－"}}, default_bg, 1},
				{{{U"り", U"リ"}, {U"o", U"O"}, {U"ｏ", U"Ｏ"}, {U"＊"}}, default_bg, 1},
				{{{U"を", U"ヲ"}, {U"p", U"P"}, {U"ｐ", U"Ｐ"}, {U"･"}}, default_bg, 1},

				// Row 3
				{{{U"う", U"ウ"}, {U"a", U"A"}, {U"ａ", U"Ａ"}, {U"＜"}}, default_bg, 1},
				{{{U"く", U"ク"}, {U"s", U"S"}, {U"ｓ", U"Ｓ"}, {U"＞"}}, default_bg, 1},
				{{{U"す", U"ス"}, {U"d", U"D"}, {U"ｄ", U"Ｄ"}, {U"（"}}, default_bg, 1},
				{{{U"つ", U"ツ"}, {U"f", U"F"}, {U"ｆ", U"Ｆ"}, {U"）"}}, default_bg, 1},
				{{{U"ぬ", U"ヌ"}, {U"g", U"G"}, {U"ｇ", U"Ｇ"}, {U"［"}}, default_bg, 1},
				{{{U"ふ", U"フ"}, {U"h", U"H"}, {U"ｈ", U"Ｈ"}, {U"］"}}, default_bg, 1},
				{{{U"む", U"ム"}, {U"j", U"J"}, {U"ｊ", U"Ｊ"}, {U"｛"}}, default_bg, 1},
				{{{U"よ", U"ヨ"}, {U"k", U"K"}, {U"ｋ", U"Ｋ"}, {U"｝"}}, default_bg, 1},
				{{{U"る", U"ル"}, {U"l", U"L"}, {U"ｌ", U"Ｌ"}, {U"｢"}}, default_bg, 1},
				{{{U"ん", U"ン"}, {U"'", U"\""}, {U"＇", U"＂"}, {U"｣"}}, default_bg, 1},

				// Row 4
				{{{U"え", U"エ"}, {U"z", U"Z"}, {U"ｚ", U"Ｚ"}, {U"＝"}}, default_bg, 1},
				{{{U"け", U"ケ"}, {U"x", U"X"}, {U"ｘ", U"Ｘ"}, {U"｜"}}, default_bg, 1},
				{{{U"せ", U"セ"}, {U"c", U"C"}, {U"ｃ", U"Ｃ"}, {U"｡"}}, default_bg, 1},
				{{{U"て", U"テ"}, {U"v", U"V"}, {U"ｖ", U"Ｖ"}, {U"／"}}, default_bg, 1},
				{{{U"ね", U"ネ"}, {U"b", U"B"}, {U"ｂ", U"Ｂ"}, {U"＼"}}, default_bg, 1},
				{{{U"へ", U"ヘ"}, {U"n", U"N"}, {U"ｎ", U"Ｎ"}, {U"￢"}}, default_bg, 1},
				{{{U"め", U"メ"}, {U"m", U"M"}, {U"ｍ", U"Ｍ"}, {U"＄"}}, default_bg, 1},
				{{{U"゛", U"゛"}, {U",", U"-"}, {U"，", U"－"}, {U"￥"}}, default_bg, 1},
				{{{U"れ", U"レ"}, {U".", U"_"}, {U"．", U"＿"}, {U"､"}}, default_bg, 1},
				{{{U"ー", U"ー"}, {U"?", U"/"}, {U"？", U"／"}, {U""}}, default_bg, 1},

				// Row 5
				{{{U"お", U"オ"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"こ", U"コ"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"そ", U"ソ"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"と", U"ト"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"の", U"ノ"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"ほ", U"ホ"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"も", U"モ"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"゜", U"゜"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"ろ", U"ロ"}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},
				{{{U"", U""}, {U"", U""}, {U"", U""}, {U""}}, default_bg, 1},

				// Control
				{{{U"あ/ア"}, {U"A/a"}, {U"Ａ/ａ"}, {U""}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"abc"}, {U"全半"}, {U"＠％"}, {U"あア"}}, special2_bg, 2, button_flags::_layer, layer_cb},
				{{{space}, {space}, {space}, {space}}, special_bg, 2, button_flags::_space, space_cb},
				{{{backspace}, {backspace}, {backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}, {enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_japanese_hiragana::osk_panel_japanese_hiragana(callback_t /*shift_cb*/, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_JAPANESE_HIRAGANA)
		{
			num_rows = 6;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"あ"}, {U"，"}}, default_bg, 1},
				{{{U"か"}, {U"．"}}, default_bg, 1},
				{{{U"さ"}, {U"："}}, default_bg, 1},
				{{{U"た"}, {U"；"}}, default_bg, 1},
				{{{U"な"}, {U"！"}}, default_bg, 1},
				{{{U"は"}, {U"？"}}, default_bg, 1},
				{{{U"ま"}, {U"＂"}}, default_bg, 1},
				{{{U"や"}, {U"＇"}}, default_bg, 1},
				{{{U"ら"}, {U"｀"}}, default_bg, 1},
				{{{U"わ"}, {U"＾"}}, default_bg, 1},

				// Row 2
				{{{U"い"}, {U"～"}}, default_bg, 1},
				{{{U"き"}, {U"＿"}}, default_bg, 1},
				{{{U"し"}, {U"＆"}}, default_bg, 1},
				{{{U"ち"}, {U"＠"}}, default_bg, 1},
				{{{U"に"}, {U"＃"}}, default_bg, 1},
				{{{U"ひ"}, {U"％"}}, default_bg, 1},
				{{{U"み"}, {U"＋"}}, default_bg, 1},
				{{{U"ゆ"}, {U"－"}}, default_bg, 1},
				{{{U"り"}, {U"＊"}}, default_bg, 1},
				{{{U"を"}, {U"･"}}, default_bg, 1},

				// Row 3
				{{{U"う"}, {U"＜"}}, default_bg, 1},
				{{{U"く"}, {U"＞"}}, default_bg, 1},
				{{{U"す"}, {U"（"}}, default_bg, 1},
				{{{U"つ"}, {U"）"}}, default_bg, 1},
				{{{U"ぬ"}, {U"［"}}, default_bg, 1},
				{{{U"ふ"}, {U"］"}}, default_bg, 1},
				{{{U"む"}, {U"｛"}}, default_bg, 1},
				{{{U"よ"}, {U"｝"}}, default_bg, 1},
				{{{U"る"}, {U"｢"}}, default_bg, 1},
				{{{U"ん"}, {U"｣"}}, default_bg, 1},

				// Row 4
				{{{U"え"}, {U"＝"}}, default_bg, 1},
				{{{U"け"}, {U"｜"}}, default_bg, 1},
				{{{U"せ"}, {U"｡"}}, default_bg, 1},
				{{{U"て"}, {U"／"}}, default_bg, 1},
				{{{U"ね"}, {U"＼"}}, default_bg, 1},
				{{{U"へ"}, {U"￢"}}, default_bg, 1},
				{{{U"め"}, {U"＄"}}, default_bg, 1},
				{{{U"゛"}, {U"￥"}}, default_bg, 1},
				{{{U"れ"}, {U"､"}}, default_bg, 1},
				{{{U"ー"}, {U""}}, default_bg, 1},

				// Row 5
				{{{U"お"}, {U""}}, default_bg, 1},
				{{{U"こ"}, {U""}}, default_bg, 1},
				{{{U"そ"}, {U""}}, default_bg, 1},
				{{{U"と"}, {U""}}, default_bg, 1},
				{{{U"の"}, {U""}}, default_bg, 1},
				{{{U"ほ"}, {U""}}, default_bg, 1},
				{{{U"も"}, {U""}}, default_bg, 1},
				{{{U"゜"}, {U""}}, default_bg, 1},
				{{{U"ろ"}, {U""}}, default_bg, 1},
				{{{U""}, {U""}}, default_bg, 1},

				// Control
				{{{U""}, {U""}}, special2_bg, 2, button_flags::_shift, nullptr },
				{{{U"＠％"}, {U"あ"}}, special2_bg, 2, button_flags::_layer, layer_cb},
				{{{space}, {space}}, special_bg, 2, button_flags::_space, space_cb},
				{{{backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_japanese_katakana::osk_panel_japanese_katakana(callback_t /*shift_cb*/, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_JAPANESE_KATAKANA)
		{
			num_rows = 6;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"ア"}, {U"，"}}, default_bg, 1},
				{{{U"カ"}, {U"．"}}, default_bg, 1},
				{{{U"サ"}, {U"："}}, default_bg, 1},
				{{{U"タ"}, {U"；"}}, default_bg, 1},
				{{{U"ナ"}, {U"！"}}, default_bg, 1},
				{{{U"ハ"}, {U"？"}}, default_bg, 1},
				{{{U"マ"}, {U"＂"}}, default_bg, 1},
				{{{U"ヤ"}, {U"＇"}}, default_bg, 1},
				{{{U"ラ"}, {U"｀"}}, default_bg, 1},
				{{{U"ワ"}, {U"＾"}}, default_bg, 1},

				// Row 2
				{{{U"イ"}, {U"～"}}, default_bg, 1},
				{{{U"キ"}, {U"＿"}}, default_bg, 1},
				{{{U"シ"}, {U"＆"}}, default_bg, 1},
				{{{U"チ"}, {U"＠"}}, default_bg, 1},
				{{{U"ニ"}, {U"＃"}}, default_bg, 1},
				{{{U"ヒ"}, {U"％"}}, default_bg, 1},
				{{{U"ミ"}, {U"＋"}}, default_bg, 1},
				{{{U"ユ"}, {U"－"}}, default_bg, 1},
				{{{U"リ"}, {U"＊"}}, default_bg, 1},
				{{{U"ヲ"}, {U"･"}}, default_bg, 1},

				// Row 3
				{{{U"ウ"}, {U"＜"}}, default_bg, 1},
				{{{U"ク"}, {U"＞"}}, default_bg, 1},
				{{{U"ス"}, {U"（"}}, default_bg, 1},
				{{{U"ツ"}, {U"）"}}, default_bg, 1},
				{{{U"ヌ"}, {U"［"}}, default_bg, 1},
				{{{U"フ"}, {U"］"}}, default_bg, 1},
				{{{U"ム"}, {U"｛"}}, default_bg, 1},
				{{{U"ヨ"}, {U"｝"}}, default_bg, 1},
				{{{U"ル"}, {U"｢"}}, default_bg, 1},
				{{{U"ン"}, {U"｣"}}, default_bg, 1},

				// Row 4
				{{{U"エ"}, {U"＝"}}, default_bg, 1},
				{{{U"ケ"}, {U"｜"}}, default_bg, 1},
				{{{U"セ"}, {U"｡"}}, default_bg, 1},
				{{{U"テ"}, {U"／"}}, default_bg, 1},
				{{{U"ネ"}, {U"＼"}}, default_bg, 1},
				{{{U"ヘ"}, {U"￢"}}, default_bg, 1},
				{{{U"メ"}, {U"＄"}}, default_bg, 1},
				{{{U"゛"}, {U"￥"}}, default_bg, 1},
				{{{U"レ"}, {U"､"}}, default_bg, 1},
				{{{U"ー"}, {U""}}, default_bg, 1},

				// Row 5
				{{{U"オ"}, {U""}}, default_bg, 1},
				{{{U"コ"}, {U""}}, default_bg, 1},
				{{{U"ソ"}, {U""}}, default_bg, 1},
				{{{U"ト"}, {U""}}, default_bg, 1},
				{{{U"ノ"}, {U""}}, default_bg, 1},
				{{{U"ホ"}, {U""}}, default_bg, 1},
				{{{U"モ"}, {U""}}, default_bg, 1},
				{{{U"゜"}, {U""}}, default_bg, 1},
				{{{U"ロ"}, {U""}}, default_bg, 1},
				{{{U""}, {U""}}, default_bg, 1},

				// Control
				{{{U""}, {U""}}, special2_bg, 2, button_flags::_shift, nullptr },
				{{{U"＠％"}, {U"ア"}}, special2_bg, 2, button_flags::_layer, layer_cb},
				{{{space}, {space}}, special_bg, 2, button_flags::_space, space_cb},
				{{{backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_alphabet_half_width::osk_panel_alphabet_half_width(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb, u32 osk_panel_mode)
			: osk_panel(osk_panel_mode)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"!"}}, default_bg, 1},
				{{{U"2", U"@"}, {U"?"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"#"}}, default_bg, 1},
				{{{U"4", U"$"}, {U"$"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"%"}}, default_bg, 1},
				{{{U"6", U"^"}, {U"&"}}, default_bg, 1},
				{{{U"7", U"&"}, {U"'"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"("}}, default_bg, 1},
				{{{U"9", U"("}, {U")"}}, default_bg, 1},
				{{{U"0", U")"}, {U"*"}}, default_bg, 1},

				// Row 2
				{{{U"q", U"Q"}, {U"/"}}, default_bg, 1},
				{{{U"w", U"W"}, {U"\\"}}, default_bg, 1},
				{{{U"e", U"E"}, {U"["}}, default_bg, 1},
				{{{U"r", U"R"}, {U"]"}}, default_bg, 1},
				{{{U"t", U"T"}, {U"^"}}, default_bg, 1},
				{{{U"y", U"Y"}, {U"_"}}, default_bg, 1},
				{{{U"u", U"U"}, {U"`"}}, default_bg, 1},
				{{{U"i", U"I"}, {U"{"}}, default_bg, 1},
				{{{U"o", U"O"}, {U"}"}}, default_bg, 1},
				{{{U"p", U"P"}, {U"|"}}, default_bg, 1},

				// Row 3
				{{{U"a", U"A"}, {U"@"}}, default_bg, 1},
				{{{U"s", U"S"}, {U"°"}}, default_bg, 1},
				{{{U"d", U"D"}, {U"‹"}}, default_bg, 1},
				{{{U"f", U"F"}, {U"›"}}, default_bg, 1},
				{{{U"g", U"G"}, {U"«"}}, default_bg, 1},
				{{{U"h", U"H"}, {U"»"}}, default_bg, 1},
				{{{U"j", U"J"}, {U"ª"}}, default_bg, 1},
				{{{U"k", U"K"}, {U"º"}}, default_bg, 1},
				{{{U"l", U"L"}, {U"×"}}, default_bg, 1},
				{{{U"'", U"\""}, {U"÷"}}, default_bg, 1},

				// Row 4
				{{{U"z", U"Z"}, {U"+"}}, default_bg, 1},
				{{{U"x", U"X"}, {U","}}, default_bg, 1},
				{{{U"c", U"C"}, {U"-"}}, default_bg, 1},
				{{{U"v", U"V"}, {U"."}}, default_bg, 1},
				{{{U"b", U"B"}, {U"\""}}, default_bg, 1},
				{{{U"n", U"N"}, {U":"}}, default_bg, 1},
				{{{U"m", U"M"}, {U";"}}, default_bg, 1},
				{{{U",", U"-"}, {U"<"}}, default_bg, 1},
				{{{U".", U"_"}, {U"="}}, default_bg, 1},
				{{{U"?", U"/"}, {U">"}}, default_bg, 1},

				// Control
				{{{U"A/a"}, {U""}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"@#:"}, {U"ABC"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_alphabet_full_width::osk_panel_alphabet_full_width(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_ALPHABET_FULL_WIDTH)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"１", U"！"}, {U"！"}}, default_bg, 1},
				{{{U"２", U"＠"}, {U"＠"}}, default_bg, 1},
				{{{U"３", U"＃"}, {U"＃"}}, default_bg, 1},
				{{{U"４", U"＄"}, {U"＄"}}, default_bg, 1},
				{{{U"５", U"％"}, {U"％"}}, default_bg, 1},
				{{{U"６", U"＾"}, {U"＾"}}, default_bg, 1},
				{{{U"７", U"＆"}, {U"＆"}}, default_bg, 1},
				{{{U"８", U"＊"}, {U"＊"}}, default_bg, 1},
				{{{U"９", U"（"}, {U"（"}}, default_bg, 1},
				{{{U"０", U"）"}, {U"）"}}, default_bg, 1},

				// Row 2
				{{{U"ｑ", U"Ｑ"}, {U"～"}}, default_bg, 1},
				{{{U"ｗ", U"Ｗ"}, {U"＿"}}, default_bg, 1},
				{{{U"ｅ", U"Ｅ"}, {U"＆"}}, default_bg, 1},
				{{{U"ｒ", U"Ｒ"}, {U"＠"}}, default_bg, 1},
				{{{U"ｔ", U"Ｔ"}, {U"＃"}}, default_bg, 1},
				{{{U"ｙ", U"Ｙ"}, {U"％"}}, default_bg, 1},
				{{{U"ｕ", U"Ｕ"}, {U"＋"}}, default_bg, 1},
				{{{U"ｉ", U"Ｉ"}, {U"－"}}, default_bg, 1},
				{{{U"ｏ", U"Ｏ"}, {U"＊"}}, default_bg, 1},
				{{{U"ｐ", U"Ｐ"}, {U"･"}}, default_bg, 1},

				// Row 3
				{{{U"ａ", U"Ａ"}, {U"＜"}}, default_bg, 1},
				{{{U"ｓ", U"Ｓ"}, {U"＞"}}, default_bg, 1},
				{{{U"ｄ", U"Ｄ"}, {U"（"}}, default_bg, 1},
				{{{U"ｆ", U"Ｆ"}, {U"）"}}, default_bg, 1},
				{{{U"ｇ", U"Ｇ"}, {U"［"}}, default_bg, 1},
				{{{U"ｈ", U"Ｈ"}, {U"］"}}, default_bg, 1},
				{{{U"ｊ", U"Ｊ"}, {U"｛"}}, default_bg, 1},
				{{{U"ｋ", U"Ｋ"}, {U"｝"}}, default_bg, 1},
				{{{U"ｌ", U"Ｌ"}, {U"｢"}}, default_bg, 1},
				{{{U"＇", U"＂"}, {U"｣"}}, default_bg, 1},

				// Row 4
				{{{U"ｚ", U"Ｚ"}, {U"＝"}}, default_bg, 1},
				{{{U"ｘ", U"Ｘ"}, {U"｜"}}, default_bg, 1},
				{{{U"ｃ", U"Ｃ"}, {U"｡"}}, default_bg, 1},
				{{{U"ｖ", U"Ｖ"}, {U"／"}}, default_bg, 1},
				{{{U"ｂ", U"Ｂ"}, {U"＼"}}, default_bg, 1},
				{{{U"ｎ", U"Ｎ"}, {U"￢"}}, default_bg, 1},
				{{{U"ｍ", U"Ｍ"}, {U"＄"}}, default_bg, 1},
				{{{U"，", U"－"}, {U"￥"}}, default_bg, 1},
				{{{U"．", U"＿"}, {U"､"}}, default_bg, 1},
				{{{U"？", U"／"}, {U""}}, default_bg, 1},

				// Control
				{{{U"Ａ/ａ"}, {U""}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"@#:"}, {U"ABC"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_numeral_half_width::osk_panel_numeral_half_width(callback_t /*shift_cb*/, callback_t /*layer_cb*/, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_NUMERAL)
		{
			num_rows = 2;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1"}}, default_bg, 1},
				{{{U"2"}}, default_bg, 1},
				{{{U"3"}}, default_bg, 1},
				{{{U"4"}}, default_bg, 1},
				{{{U"5"}}, default_bg, 1},
				{{{U"6"}}, default_bg, 1},
				{{{U"7"}}, default_bg, 1},
				{{{U"8"}}, default_bg, 1},
				{{{U"9"}}, default_bg, 1},
				{{{U"0"}}, default_bg, 1},

				// Control
				{{{U""}}, special2_bg, 2, button_flags::_shift, nullptr },
				{{{U""}}, special2_bg, 2, button_flags::_layer, nullptr },
				{{{space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_numeral_full_width::osk_panel_numeral_full_width(callback_t /*shift_cb*/, callback_t /*layer_cb*/, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_NUMERAL_FULL_WIDTH)
		{
			num_rows = 2;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"１"}}, default_bg, 1},
				{{{U"２"}}, default_bg, 1},
				{{{U"３"}}, default_bg, 1},
				{{{U"４"}}, default_bg, 1},
				{{{U"５"}}, default_bg, 1},
				{{{U"６"}}, default_bg, 1},
				{{{U"７"}}, default_bg, 1},
				{{{U"８"}}, default_bg, 1},
				{{{U"９"}}, default_bg, 1},
				{{{U"０"}}, default_bg, 1},

				// Control
				{{{U""}}, special2_bg, 2, button_flags::_shift, nullptr },
				{{{U""}}, special2_bg, 2, button_flags::_layer, nullptr },
				{{{space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_url::osk_panel_url(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel(CELL_OSKDIALOG_PANELMODE_URL)
		{
			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			layout =
			{
				// Row 1
				{{{U"1", U"!"}, {U"!"}}, default_bg, 1},
				{{{U"2", U"@"}, {U"?"}}, default_bg, 1},
				{{{U"3", U"#"}, {U"#"}}, default_bg, 1},
				{{{U"4", U"$"}, {U"$"}}, default_bg, 1},
				{{{U"5", U"%"}, {U"%"}}, default_bg, 1},
				{{{U"6", U"~"}, {U"&"}}, default_bg, 1},
				{{{U"7", U"&"}, {U"'"}}, default_bg, 1},
				{{{U"8", U"*"}, {U"("}}, default_bg, 1},
				{{{U"9", U";"}, {U")"}}, default_bg, 1},
				{{{U"0", U"+"}, {U"*"}}, default_bg, 1},

				// Row 2
				{{{U"q", U"Q"}, {U"/"}}, default_bg, 1},
				{{{U"w", U"W"}, {U"\\"}}, default_bg, 1},
				{{{U"e", U"E"}, {U"["}}, default_bg, 1},
				{{{U"r", U"R"}, {U"]"}}, default_bg, 1},
				{{{U"t", U"T"}, {U"^"}}, default_bg, 1},
				{{{U"y", U"Y"}, {U"_"}}, default_bg, 1},
				{{{U"u", U"U"}, {U"`"}}, default_bg, 1},
				{{{U"i", U"I"}, {U"{"}}, default_bg, 1},
				{{{U"o", U"O"}, {U"}"}}, default_bg, 1},
				{{{U"p", U"P"}, {U"|"}}, default_bg, 1},

				// Row 3
				{{{U"a", U"A"}, {U"@"}}, default_bg, 1},
				{{{U"s", U"S"}, {U"°"}}, default_bg, 1},
				{{{U"d", U"D"}, {U"‹"}}, default_bg, 1},
				{{{U"f", U"F"}, {U"›"}}, default_bg, 1},
				{{{U"g", U"G"}, {U"«"}}, default_bg, 1},
				{{{U"h", U"H"}, {U"»"}}, default_bg, 1},
				{{{U"j", U"J"}, {U"ª"}}, default_bg, 1},
				{{{U"k", U"K"}, {U"º"}}, default_bg, 1},
				{{{U"l", U"L"}, {U"×"}}, default_bg, 1},
				{{{U"-", U"="}, {U"÷"}}, default_bg, 1},

				// Row 4
				{{{U"z", U"Z"}, {U"+"}}, default_bg, 1},
				{{{U"x", U"X"}, {U","}}, default_bg, 1},
				{{{U"c", U"C"}, {U"-"}}, default_bg, 1},
				{{{U"v", U"V"}, {U"."}}, default_bg, 1},
				{{{U"b", U"B"}, {U"\""}}, default_bg, 1},
				{{{U"n", U"N"}, {U":"}}, default_bg, 1},
				{{{U"m", U"M"}, {U";"}}, default_bg, 1},
				{{{U"/", U":"}, {U"<"}}, default_bg, 1},
				{{{U".", U","}, {U"="}}, default_bg, 1},
				{{{U"_", U"?"}, {U">"}}, default_bg, 1},

				// Control
				{{{U"A/a"}, {U""}}, special2_bg, 2, button_flags::_shift, shift_cb },
				{{{U"@#:"}, {U"ABC"}}, special2_bg, 2, button_flags::_layer, layer_cb },
				{{{space}, {space}}, special_bg, 2, button_flags::_space, space_cb },
				{{{backspace}, {backspace}}, special_bg, 2, button_flags::_default, delete_cb },
				{{{enter}, {enter}}, special2_bg, 2, button_flags::_return, enter_cb },
			};
		}

		osk_panel_password::osk_panel_password(callback_t shift_cb, callback_t layer_cb, callback_t space_cb, callback_t delete_cb, callback_t enter_cb)
			: osk_panel_alphabet_half_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb, CELL_OSKDIALOG_PANELMODE_PASSWORD)
		{
			// Same as the half-width alphanumeric character panel.
		}
	}
}
