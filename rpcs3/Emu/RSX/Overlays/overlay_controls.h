#pragma once

#include "overlay_fonts.h"

#include "Emu/localized_string.h"

#include <memory>

// Definitions for common UI controls and their routines
namespace rsx
{
	namespace overlays
	{
		enum image_resource_id : u8
		{
			// NOTE: 1 - 252 are user defined
			none = 0,         // No image
			raw_image = 252,  // Raw image data passed via image_info struct
			font_file = 253,  // Font file
			game_icon = 254,  // Use game icon
			backbuffer = 255  // Use current backbuffer contents
		};

		enum class primitive_type : u8
		{
			quad_list = 0,
			triangle_strip = 1,
			line_list = 2,
			line_strip = 3,
			triangle_fan = 4
		};

		struct image_info_base
		{
			int w = 0, h = 0, channels = 0;
			int bpp = 0;
			bool dirty = false;

			image_info_base() {}
			virtual ~image_info_base() {}
			virtual const u8* get_data() const = 0;
		};

		struct image_info : public image_info_base
		{
		private:
			u8* data = nullptr;
			std::vector<u8> data_grey;

		public:
			using image_info_base::image_info_base;
			image_info(image_info&) = delete;
			image_info(const std::string& filename, bool grayscaled = false);
			image_info(const std::vector<u8>& bytes, bool grayscaled = false);
			virtual ~image_info();

			void load_data(const std::vector<u8>& bytes, bool grayscaled = false);
			const u8* get_data() const override { return channels == 4 ? data : data_grey.empty() ? nullptr : data_grey.data(); }
		};

		struct resource_config
		{
			enum standard_image_resource : u8
			{
				fade_top = 1,
				fade_bottom,
				select,
				start,
				cross,
				circle,
				triangle,
				square,
				L1,
				R1,
				L2,
				R2,
				save,
				new_entry
			};

			// Define resources
			std::vector<std::unique_ptr<image_info>> texture_raw_data;

			resource_config();

			void load_files();
			void free_resources();
		};

		struct compiled_resource
		{
			struct command_config
			{
				primitive_type primitives = primitive_type::quad_list;

				color4f color = { 1.f, 1.f, 1.f, 1.f };
				bool pulse_glow = false;
				bool disable_vertex_snap = false;
				f32 pulse_sinus_offset = 0.0f; // The current pulse offset
				f32 pulse_speed_modifier = 0.005f;

				areaf clip_rect = {};
				bool clip_region = false;

				u8 texture_ref = image_resource_id::none;
				font* font_ref = nullptr;
				void* external_data_ref = nullptr;

				u8 blur_strength = 0;

				command_config() = default;

				void set_image_resource(u8 ref);
				void set_font(font *ref);

				// Analog to overlay_element::set_sinus_offset
				f32 get_sinus_value() const;
			};

			struct command
			{
				command_config config;
				std::vector<vertex> verts;
			};

			std::vector<command> draw_commands;

			void add(const compiled_resource& other);
			void add(const compiled_resource& other, f32 x_offset, f32 y_offset);
			void add(const compiled_resource& other, f32 x_offset, f32 y_offset, const areaf& clip_rect);

			//! Clear commands list
			void clear();

			//! Append the command to the back of the commands list and return it
			command& append(const command& new_command);

			//! Prepend the command to the front of the commands list and return it
			command& prepend(const command& new_command);
		};

		struct overlay_element
		{
			enum text_align
			{
				left = 0,
				center,
				right
			};

			s16 x = 0;
			s16 y = 0;
			u16 w = 0;
			u16 h = 0;

			std::u32string text;
			font* font_ref = nullptr;
			text_align alignment = left;
			bool wrap_text = false;
			bool clip_text = true;

			color4f back_color = { 0.f, 0.f, 0.f, 1.f };
			color4f fore_color = { 1.f, 1.f, 1.f, 1.f };
			bool pulse_effect_enabled = false;
			f32 pulse_sinus_offset = 0.0f; // The current pulse offset
			f32 pulse_speed_modifier = 0.005f;

			// Analog to command_config::get_sinus_value
			// Apply modifier for sinus pulse. Resets the pulse. For example:
			//     0 -> reset to 0.5 rising
			//   0.5 -> reset to 0
			//     1 -> reset to 0.5 falling
			//   1.5 -> reset to 1
			void set_sinus_offset(f32 sinus_modifier);

			compiled_resource compiled_resources;

			bool visible = true;

			u16 padding_left = 0;
			u16 padding_right = 0;
			u16 padding_top = 0;
			u16 padding_bottom = 0;

			u16 margin_left = 0;
			u16 margin_top = 0;

			f32 horizontal_scroll_offset = 0.0f;
			f32 vertical_scroll_offset = 0.0f;

			overlay_element() = default;
			overlay_element(u16 _w, u16 _h) : w(_w), h(_h) {}
			virtual ~overlay_element() = default;

			virtual void refresh();
			virtual bool is_compiled() { return m_is_compiled; }
			virtual void translate(s16 _x, s16 _y);
			virtual void scale(f32 _x, f32 _y, bool origin_scaling);
			virtual void set_pos(s16 _x, s16 _y);
			virtual void set_size(u16 _w, u16 _h);
			virtual void set_padding(u16 left, u16 right, u16 top, u16 bottom);
			virtual void set_padding(u16 padding);
			// NOTE: Functions as a simple position offset. Top left corner is the anchor.
			virtual void set_margin(u16 left, u16 top);
			virtual void set_margin(u16 margin);
			virtual void set_text(const std::string& text);
			virtual void set_unicode_text(const std::u32string& text);
			void set_text(localized_string_id id);
			virtual void set_font(const char* font_name, u16 font_size);
			virtual void align_text(text_align align);
			virtual void set_wrap_text(bool state);
			virtual font* get_font() const;
			virtual std::vector<vertex> render_text(const char32_t* string, f32 x, f32 y);
			virtual compiled_resource& get_compiled();
			void measure_text(u16& width, u16& height, bool ignore_word_wrap = false) const;
			virtual void set_selected(bool selected) { static_cast<void>(selected); }

		protected:
			bool m_is_compiled = false; // Only use m_is_compiled as a getter in is_compiled() if possible
		};

		struct layout_container : public overlay_element
		{
			std::vector<std::unique_ptr<overlay_element>> m_items;
			u16 advance_pos = 0;
			u16 pack_padding = 0;
			u16 scroll_offset_value = 0;
			bool auto_resize = true;

			virtual overlay_element* add_element(std::unique_ptr<overlay_element>&, int = -1) = 0;

			layout_container();

			void translate(s16 _x, s16 _y) override;
			void set_pos(s16 _x, s16 _y) override;

			bool is_compiled() override;

			compiled_resource& get_compiled() override;

			virtual u16 get_scroll_offset_px() = 0;
			void add_spacer();
		};

		struct vertical_layout : public layout_container
		{
			overlay_element* add_element(std::unique_ptr<overlay_element>& item, int offset = -1) override;
			compiled_resource& get_compiled() override;
			u16 get_scroll_offset_px() override;
		};

		struct horizontal_layout : public layout_container
		{
			overlay_element* add_element(std::unique_ptr<overlay_element>& item, int offset = -1) override;
			compiled_resource& get_compiled() override;
			u16 get_scroll_offset_px() override;
		};

		// Controls
		struct spacer : public overlay_element
		{
			using overlay_element::overlay_element;
			compiled_resource& get_compiled() override
			{
				// No draw
				m_is_compiled = true;
				return compiled_resources;
			}
		};

		struct rounded_rect : public overlay_element
		{
			u8 radius = 5;
			u8 num_control_points = 8; // Smoothness control

			using overlay_element::overlay_element;
			compiled_resource& get_compiled() override;
		};

		struct image_view : public overlay_element
		{
		protected:
			u8 image_resource_ref = image_resource_id::none;
			void* external_ref = nullptr;

			// Strength of blur effect
			u8 blur_strength = 0;

		public:
			using overlay_element::overlay_element;

			compiled_resource& get_compiled() override;

			void set_image_resource(u8 resource_id);
			void set_raw_image(image_info_base* raw_image);
			void clear_image();
			void set_blur_strength(u8 strength);
		};

		struct image_button : public image_view
		{
			u16 text_horizontal_offset = 25;
			u16 m_text_offset_x = 0;
			s16 m_text_offset_y = 0;

			image_button();
			image_button(u16 _w, u16 _h);

			void set_text_vertical_adjust(s16 offset);
			void set_size(u16 /*w*/, u16 h) override;

			compiled_resource& get_compiled() override;
		};

		struct label : public overlay_element
		{
			label() = default;
			label(const std::string& text);

			bool auto_resize(bool grow_only = false, u16 limit_w = -1, u16 limit_h = -1);
		};

		struct graph : public overlay_element
		{
		private:
			std::string m_title;
			std::vector<f32> m_datapoints;
			u32 m_datapoint_count{};
			color4f m_color;
			f32 m_min{};
			f32 m_max{};
			f32 m_avg{};
			f32 m_1p{};
			f32 m_guide_interval{};
			label m_label{};

			bool m_show_min_max{false};
			bool m_show_1p_avg{false};
			bool m_1p_sort_high{false};

		public:
			graph();
			void set_pos(s16 _x, s16 _y) override;
			void set_size(u16 _w, u16 _h) override;
			void set_title(const char* title);
			void set_font(const char* font_name, u16 font_size) override;
			void set_font_size(u16 font_size);
			void set_count(u32 datapoint_count);
			void set_color(color4f color);
			void set_guide_interval(f32 guide_interval);
			void set_labels_visible(bool show_min_max, bool show_1p_avg);
			void set_one_percent_sort_high(bool sort_1p_high);
			u16 get_height() const;
			u32 get_datapoint_count() const;
			void record_datapoint(f32 datapoint, bool update_metrics);
			void update();
			compiled_resource& get_compiled() override;
		};
	}
}
