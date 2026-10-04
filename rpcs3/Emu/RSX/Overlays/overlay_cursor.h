#pragma once

#include "overlays.h"
#include "Utilities/mutex.h"
#include <map>

namespace rsx
{
	namespace overlays
	{
		enum cursor_offset : u32
		{
			cell_gem = 0, // CELL_GEM_MAX_NUM = 4 Move controllers
			last = 4
		};

		class cursor_item
		{
		public:
			cursor_item();

			void set_expiration(u64 expiration_time);
			bool set_position(s16 x, s16 y);
			bool set_color(color4f color);

			bool update_visibility(u64 time);
			bool visible() const;

			compiled_resource get_compiled();

		private:
			bool m_visible = false;
			overlay_element m_cross_h{};
			overlay_element m_cross_v{};
			u64 m_expiration_time = 0;
			s16 m_x = 0;
			s16 m_y = 0;
		};

		class cursor_manager final : public overlay
		{
		public:
			void update(u64 timestamp_us) override;
			compiled_resource get_compiled() override;

			void update_cursor(u32 id, s16 x, s16 y, const color4f& color, u64 duration_us, bool force_update);

		private:
			shared_mutex m_mutex;
			std::map<u32, cursor_item> m_cursors;
		};

		class bitmap_cursor final : public overlay
		{
		public:
			void enable();
			void disable();
			void set_pos(s32 x, s32 y);
			void set_bitmap(u32 address);
			void set_screen_size(u16 w, u16 h);

			compiled_resource get_compiled() override;
			u16 get_virtual_width() const override { return m_virtual_width; }
			u16 get_virtual_height() const override { return m_virtual_height; }

		private:
			bool m_visible = false;
			position2_base<s16> m_position = {};
			std::unique_ptr<overlays::image_view> m_bitmap;
			overlays::memory_image_info m_image_storage;

			u16 m_virtual_width = 1280;
			u16 m_virtual_height = 720;

			shared_mutex m_mutex;
		};

		void set_cursor(u32 id, s16 x, s16 y, const color4f& color, u64 duration_us, bool force_update);

	} // namespace overlays
} // namespace rsx
