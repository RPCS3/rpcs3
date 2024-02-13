#pragma once

#include "overlays.h"
#include "overlay_edit_text.hpp"
#include "overlay_cursor.h"
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

			// Mutex for interaction between overlay and cellOskDialog
			shared_mutex m_preview_mutex;

			// Base UI configuration
			bool m_use_separate_windows = false;
			bool m_show_panel = true;
			osk_window_layout m_layout = {};
			osk_window_layout m_input_layout = {}; // Only used with separate windows
			osk_window_layout m_panel_layout = {}; // Only used with separate windows
			u32 m_input_field_window_width = 0; // Only used with separate windows
			f32 m_scaling = 1.0f;

			// Base UI
			overlay_element m_input_frame;
			overlay_element m_panel_frame;
			overlay_element m_background;
			label m_title;
			edit_text m_preview;
			image_button m_btn_accept;
			image_button m_btn_cancel;
			image_button m_btn_shift;
			image_button m_btn_space;
			image_button m_btn_delete;

			// Pointer
			cursor_item m_pointer{};

			// Analog movement
			s16 m_x_input_pos = 0;
			s16 m_y_input_pos = 0;
			s16 m_x_panel_pos = 0;
			s16 m_y_panel_pos = 0;

			// Grid
			u16 cell_size_x = 0;
			u16 cell_size_y = 0;
			u16 num_columns = 0;
			u16 num_rows = 0;
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

			bool m_reset_pulse = false;
			overlay_element m_key_pulse_cache; // Let's use this to store the pulse offset of the key, since we don't seem to cache the keys themselves.

			bool m_update = true;
			compiled_resource m_cached_resource;

			u32 flags = 0;
			u32 char_limit = umax;

			std::vector<osk_panel> m_panels;
			usz m_panel_index = 0;

			osk_dialog();
			~osk_dialog() override = default;

			void Create(const osk_params& params) override;
			void Close(s32 status) override;
			void Clear(bool clear_all_data) override;
			void SetText(const std::u16string& text) override;
			void Insert(const std::u16string& text) override;

			void initialize_layout(const std::u32string& title, const std::u32string& initial_text);
			void add_panel(const osk_panel& panel);
			void step_panel(bool next_panel);
			void update_panel();
			void update_layout();
			void update(u64 timestamp_us) override;

			void update_controls();
			void update_selection_by_index(u32 index);

			void set_visible(bool visible);

			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;
			void on_key_pressed(u32 led, u32 mkey, u32 key_code, u32 out_key_code, bool pressed, std::u32string key) override;
			void on_text_changed();

			void on_default_callback(const std::u32string& str);
			void on_shift(const std::u32string&);
			void on_layer(const std::u32string&);
			void on_space(const std::u32string&);
			void on_backspace(const std::u32string&);
			void on_delete(const std::u32string&);
			void on_enter(const std::u32string&);
			void on_move_cursor(const std::u32string&, edit_text::direction dir);

			std::u32string get_placeholder() const;

			std::pair<u32, u32> get_cell_geometry(u32 index);

			template <typename T>
			s16 get_scaled(T val)
			{
				return static_cast<s16>(static_cast<f32>(val) * m_scaling);
			}

			compiled_resource get_compiled() override;
		};
	}
}
