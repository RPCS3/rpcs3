#pragma once

#include "overlays.h"
#include "Emu/Cell/Modules/cellOskDialog.h"

namespace rsx
{
	namespace overlays
	{
		struct osk_dialog : public user_interface, public OskDialogBase
		{
			using callback_t = std::function<void(const std::u32string&)>;

			enum border_flags
			{
				top = 1,
				bottom = 2,
				left = 4,
				right = 8,

				start_cell = top | bottom | left,
				end_cell = top | bottom | right,
				middle_cell = top | bottom,
				default_cell = top | bottom | left | right
			};

			enum button_flags
			{
				_default = 0,
				_return = 1,
				_space = 2,
				_shift = 3,
				_mode = 4
			};

			enum layer_mode : u32
			{
				alphanumeric = 0,
				extended = 1,
				special = 2,

				mode_count
			};

			struct cell
			{
				position2u pos;
				color4f backcolor{};
				border_flags flags = default_cell;
				bool selected = false;
				bool enabled = false;

				// TODO: change to array with layer_mode::layer_count
				std::vector<std::vector<std::u32string>> outputs;
				callback_t callback;
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

			// Base UI
			overlay_element m_frame;
			overlay_element m_background;
			label m_title;
			edit_text m_preview;
			image_button m_btn_accept;
			image_button m_btn_cancel;
			image_button m_btn_shift;
			image_button m_btn_space;
			image_button m_btn_delete;

			// Grid
			u32 cell_size_x = 0;
			u32 cell_size_y = 0;
			u32 num_columns = 0;
			u32 num_rows = 0;
			std::vector<u32> num_layers;
			u32 selected_x = 0;
			u32 selected_y = 0;
			u32 selected_z = 0;
			layer_mode m_selected_mode = layer_mode::alphanumeric;

			std::vector<cell> m_grid;

			// Fade in/out
			animation_color_interpolate fade_animation;

			bool m_update = true;
			compiled_resource m_cached_resource;

			u32 flags = 0;
			u32 char_limit = UINT32_MAX;

			osk_dialog() = default;
			~osk_dialog() override = default;

			void Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 options) override = 0;
			void Close(bool ok) override;

			void initialize_layout(const std::vector<grid_entry_ctor>& layout, const std::u32string& title, const std::u32string& initial_text);
			void update() override;

			void on_button_pressed(pad_button button_press) override;
			void on_text_changed();

			void on_default_callback(const std::u32string&);
			void on_shift(const std::u32string&);
			void on_mode(const std::u32string&);
			void on_space(const std::u32string&);
			void on_backspace(const std::u32string&);
			void on_enter(const std::u32string&);

			compiled_resource get_compiled() override;
		};

		struct osk_latin : osk_dialog
		{
			using osk_dialog::osk_dialog;

			void Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 options) override;
		};
	}
}
