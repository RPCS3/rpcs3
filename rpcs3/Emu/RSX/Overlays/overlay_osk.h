#pragma once

#include "overlays.h"
#include "overlay_osk_panel.h"
#include "Emu/Cell/Modules/cellOskDialog.h"

namespace rsx
{
	namespace overlays
	{
		struct osk_dialog : public user_interface, public OskDialogBase
		{
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

			struct cell
			{
				position2u pos;
				color4f backcolor{};
				border_flags flags = default_cell;
				button_flags button_flag = button_flags::_default;
				bool selected = false;
				bool enabled = false;

				std::vector<std::vector<std::u32string>> outputs;
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
			std::vector<u32> num_shift_layers_by_charset;
			u32 selected_x = 0;
			u32 selected_y = 0;
			u32 selected_z = 0;
			u32 m_selected_charset = 0;

			std::vector<cell> m_grid;

			// Password mode (****)
			bool m_password_mode = false;

			// Fade in/out
			animation_color_interpolate fade_animation;

			bool m_update = true;
			compiled_resource m_cached_resource;

			u32 flags = 0;
			u32 char_limit = UINT32_MAX;

			std::vector<osk_panel> m_panels;
			size_t m_panel_index = 0;

			osk_dialog() = default;
			~osk_dialog() override = default;

			void Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 prohibit_flags, u32 panel_flag, u32 first_view_panel) override;
			void Close(bool ok) override;

			void initialize_layout(const std::u32string& title, const std::u32string& initial_text);
			void add_panel(const osk_panel& panel);
			void step_panel(bool next_panel);
			void update_panel();
			void update_layout();
			void update() override;

			void update_controls();
			void update_selection_by_index(u32 index);

			void on_button_pressed(pad_button button_press) override;
			void on_text_changed();

			void on_default_callback(const std::u32string& str);
			void on_shift(const std::u32string&);
			void on_layer(const std::u32string&);
			void on_space(const std::u32string&);
			void on_backspace(const std::u32string&);
			void on_enter(const std::u32string&);

			std::u32string get_placeholder();

			std::pair<u32, u32> get_cell_geometry(u32 index);

			compiled_resource get_compiled() override;
		};
	}
}
