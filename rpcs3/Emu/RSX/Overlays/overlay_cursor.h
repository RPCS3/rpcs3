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

		void set_cursor(u32 id, s16 x, s16 y, const color4f& color, u64 duration_us, bool force_update);

	} // namespace overlays
} // namespace rsx
