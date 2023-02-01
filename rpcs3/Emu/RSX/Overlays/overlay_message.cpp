#include "stdafx.h"
#include "overlay_message.h"
#include "Emu/RSX/RSXThread.h"

namespace rsx
{
	namespace overlays
	{
		static u64 get_expiration_time(u64 duration)
		{
			if (duration == umax)
			{
				return duration;
			}

			return rsx::uclock() + duration;
		}

		template <typename T>
		message_item::message_item(T msg_id, u64 expiration, std::shared_ptr<atomic_t<u32>> refs, std::unique_ptr<overlay_element> icon)
		{
			m_visible_duration = expiration;
			m_refs = std::move(refs);

			m_text.set_font("Arial", 14);
			m_text.set_text(msg_id);
			m_text.auto_resize();
			m_text.back_color.a = 0.f;

			m_fade_animation.current = color4f(1.f);
			m_fade_animation.end = color4f(1.0f, 1.0f, 1.0f, 0.f);
			m_fade_animation.duration = 2.f;
			m_fade_animation.active = true;

			if (icon)
			{
				m_icon = std::move(icon);
				m_icon->set_pos(m_text.x + m_text.w + 8, m_text.y);
			}
		}
		template message_item::message_item(std::string msg_id, u64, std::shared_ptr<atomic_t<u32>>, std::unique_ptr<overlay_element>);
		template message_item::message_item(localized_string_id msg_id, u64, std::shared_ptr<atomic_t<u32>>, std::unique_ptr<overlay_element>);

		u64 message_item::get_expiration() const
		{
			// If reference counting is enabled and reached 0 consider it expired 
			return m_refs && *m_refs == 0 ? 0 : m_expiration_time;
		}

		compiled_resource& message_item::get_compiled()
		{
			if (!m_processed)
			{
				compiled_resources = {};
				return compiled_resources;
			}

			compiled_resources = m_text.get_compiled();
			if (m_icon)
			{
				compiled_resources.add(m_icon->get_compiled());
			}
			m_fade_animation.apply(compiled_resources);

			return compiled_resources;
		}

		void message_item::update(usz index, u64 time, u16 origin, int grow_direction)
		{
			if (m_cur_pos != index)
			{
				m_cur_pos = index;
				m_text.set_pos(10, static_cast<u16>(origin + (index * 18 * grow_direction)));
				if (m_icon)
				{
					m_icon->set_pos(m_icon->x, m_text.y);
				}
			}

			if (!m_processed)
			{
				m_expiration_time = get_expiration_time(m_visible_duration);
			}
			else if ((m_expiration_time - time) < 2'000'000)
			{
				m_fade_animation.update(rsx::get_current_renderer()->vblank_count);
			}

			m_processed = true;
		}

		void message::update_queue(std::deque<message_item>& vis_set, std::deque<message_item>& ready_set, u16 origin, int grow_direction)
		{
			const u64 cur_time = rsx::uclock();

			while (!vis_set.empty() && vis_set.front().get_expiration() < cur_time)
			{
				vis_set.pop_front();
			}

			while (vis_set.size() < max_visible_items && !ready_set.empty())
			{
				vis_set.emplace_back(std::move(ready_set.front()));
				ready_set.pop_front();
			}

			if (vis_set.empty() && ready_set.empty())
			{
				return;
			}

			usz index = 0;
			for (auto& item : vis_set)
			{
				item.update(index, cur_time, origin, grow_direction);
				index++;
			}
		}

		void message::update()
		{
			if (!visible)
			{
				return;
			}

			std::lock_guard lock(m_mutex_queue);

			update_queue(m_vis_items_top, m_ready_queue_top, 0, 1);
			update_queue(m_vis_items_bottom, m_ready_queue_bottom, virtual_height - 18, -1);

			visible = !m_vis_items_top.empty() || !m_vis_items_bottom.empty();
		}

		compiled_resource message::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			std::lock_guard lock(m_mutex_queue);

			compiled_resource cr{};

			for (auto& item : m_vis_items_top)
			{
				cr.add(item.get_compiled());
			}

			for (auto& item : m_vis_items_bottom)
			{
				cr.add(item.get_compiled());
			}

			return cr;
		}

	} // namespace overlays
} // namespace rsx
