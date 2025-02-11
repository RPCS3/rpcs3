#include "stdafx.h"
#include "overlay_cursor.h"
#include "overlay_manager.h"
#include "Emu/Cell/timers.hpp"

namespace rsx
{
	namespace overlays
	{
		cursor_item::cursor_item()
		{
			m_cross_h.set_size(15, 1);
			m_cross_v.set_size(1, 15);
		}

		bool cursor_item::set_position(s16 x, s16 y)
		{
			if (m_x == x && m_y == y)
			{
				return false;
			}

			m_x = x;
			m_y = y;

			m_cross_h.set_pos(m_x - m_cross_h.w / 2, m_y - m_cross_h.h / 2);
			m_cross_v.set_pos(m_x - m_cross_v.w / 2, m_y - m_cross_v.h / 2);

			return true;
		}

		bool cursor_item::set_color(color4f color)
		{
			if (m_cross_h.back_color == color && m_cross_v.back_color == color)
			{
				return false;
			}

			m_cross_h.back_color = color;
			m_cross_h.refresh();

			m_cross_v.back_color = color;
			m_cross_v.refresh();

			return true;
		}

		void cursor_item::set_expiration(u64 expiration_time)
		{
			m_expiration_time = expiration_time;
		}

		bool cursor_item::update_visibility(u64 time)
		{
			m_visible = time <= m_expiration_time;

			return m_visible;
		}

		bool cursor_item::visible() const
		{
			return m_visible;
		}

		compiled_resource cursor_item::get_compiled()
		{
			if (!m_visible)
			{
				return {};
			}

			compiled_resource cr = m_cross_h.get_compiled();
			cr.add(m_cross_v.get_compiled());

			return cr;
		}

		void cursor_manager::update(u64 timestamp_us)
		{
			if (!visible)
			{
				return;
			}

			std::lock_guard lock(m_mutex);

			bool any_cursor_visible = false;

			for (auto& entry : m_cursors)
			{
				any_cursor_visible |= entry.second.update_visibility(timestamp_us);
			}

			if (!any_cursor_visible)
			{
				visible = false;
			}
		}

		compiled_resource cursor_manager::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			std::lock_guard lock(m_mutex);

			compiled_resource cr{};

			for (auto& entry : m_cursors)
			{
				cr.add(entry.second.get_compiled());
			}

			return cr;
		}

		void cursor_manager::update_cursor(u32 id, s16 x, s16 y, const color4f& color, u64 duration_us, bool force_update)
		{
			std::lock_guard lock(m_mutex);

			cursor_item& cursor = m_cursors[id];

			bool is_dirty = cursor.set_position(x, y);
			is_dirty |= cursor.set_color(color);

			if (is_dirty || force_update)
			{
				const u64 expiration_time = get_system_time() + duration_us;
				cursor.set_expiration(expiration_time);

				if (cursor.update_visibility(expiration_time))
				{
					visible = true;
				}
			}
		}

		void set_cursor(u32 id, s16 x, s16 y, const color4f& color, u64 duration_us, bool force_update)
		{
			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				auto cursor_overlay = manager->get<rsx::overlays::cursor_manager>();
				if (!cursor_overlay)
				{
					cursor_overlay = std::make_shared<rsx::overlays::cursor_manager>();
					cursor_overlay = manager->add(cursor_overlay);
				}
				cursor_overlay->update_cursor(id, x, y, color, duration_us, force_update);
			}
		}

	} // namespace overlays
} // namespace rsx
