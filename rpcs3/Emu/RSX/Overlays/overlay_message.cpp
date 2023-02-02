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
			m_text.set_padding(4, 8, 4, 8);
			m_text.auto_resize();
			m_text.back_color.a = 0.f;

			m_fade_in_animation.current = color4f(0.f);
			m_fade_in_animation.end = color4f(1.0f);
			m_fade_in_animation.duration = 1.f;
			m_fade_in_animation.active = true;

			m_fade_out_animation.current = color4f(1.f);
			m_fade_out_animation.end = color4f(0.f);
			m_fade_out_animation.duration = 2.f;
			m_fade_out_animation.active = true;

			back_color = color4f(0.25f, 0.25f, 0.25f, 0.85f);

			if (icon)
			{
				m_icon = std::move(icon);
				m_icon->set_pos(m_text.x + m_text.w + 8, m_text.y);

				set_size(m_margin + m_text.w + m_icon->w + m_margin, m_margin + std::max(m_text.h, m_icon->h) + m_margin);
			}
			else
			{
				set_size(m_text.w + m_margin + m_margin, m_text.h + m_margin + m_margin);
			}
		}
		template message_item::message_item(std::string msg_id, u64, std::shared_ptr<atomic_t<u32>>, std::unique_ptr<overlay_element>);
		template message_item::message_item(localized_string_id msg_id, u64, std::shared_ptr<atomic_t<u32>>, std::unique_ptr<overlay_element>);

		u64 message_item::get_expiration() const
		{
			// If reference counting is enabled and reached 0 consider it expired 
			return m_refs && *m_refs == 0 ? 0 : m_expiration_time;
		}

		bool message_item::text_matches(const std::u32string& text) const
		{
			return m_text.text == text;
		}

		void message_item::set_pos(u16 _x, u16 _y)
		{
			rounded_rect::set_pos(_x, _y);
			m_text.set_pos(_x + m_margin, y + m_margin);

			if (m_icon)
			{
				m_icon->set_pos(m_icon->x, m_text.y);
			}
		}

		compiled_resource& message_item::get_compiled()
		{
			auto& current_animation = m_fade_in_animation.active
				? m_fade_in_animation
				: m_fade_out_animation;

			if (!m_processed || !current_animation.active)
			{
				compiled_resources = {};
				return compiled_resources;
			}

			// Disable caching
			is_compiled = false;

			compiled_resources = rounded_rect::get_compiled();
			compiled_resources.add(m_text.get_compiled());
			if (m_icon)
			{
				compiled_resources.add(m_icon->get_compiled());
			}

			current_animation.apply(compiled_resources);
			return compiled_resources;
		}

		void message_item::update(usz index, u64 time, u16 y_offset)
		{
			if (m_cur_pos != index)
			{
				m_cur_pos = index;
				set_pos(10, y_offset);
			}

			if (!m_processed)
			{
				m_expiration_time = get_expiration_time(m_visible_duration);
			}

			if (m_fade_in_animation.active)
			{
				m_fade_in_animation.update(rsx::get_current_renderer()->vblank_count);
			}
			else if (time + 2'000'000 > m_expiration_time)
			{
				m_fade_out_animation.update(rsx::get_current_renderer()->vblank_count);
			}

			m_processed = true;
		}

		void message::update_queue(std::deque<message_item>& vis_set, std::deque<message_item>& ready_set, message_pin_location origin)
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

			// Render reversed list. Oldest entries are furthest from the border
			u16 y_offset = 8, margin = 4, index = 0;
			for (auto it = vis_set.rbegin(); it != vis_set.rend(); ++it, ++index)
			{
				if (origin == message_pin_location::top) [[ likely ]]
				{
					it->update(index, cur_time, y_offset);
					y_offset += (margin + it->h);
				}
				else
				{
					y_offset += (margin + it->h);
					it->update(index, cur_time, virtual_height - y_offset);
				}
			}
		}

		void message::update()
		{
			if (!visible)
			{
				return;
			}

			std::lock_guard lock(m_mutex_queue);

			update_queue(m_vis_items_top, m_ready_queue_top, message_pin_location::top);
			update_queue(m_vis_items_bottom, m_ready_queue_bottom, message_pin_location::bottom);

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

		bool message::message_exists(message_pin_location location, localized_string_id id)
		{
			return message_exists(location, get_localized_u32string(id));
		}

		bool message::message_exists(message_pin_location location, const std::string& msg)
		{
			return message_exists(location, utf8_to_u32string(msg));
		}

		bool message::message_exists(message_pin_location location, const std::u32string& msg)
		{
			auto check_list = [&](const std::deque<message_item>& list)
			{
				return std::any_of(list.cbegin(), list.cend(), [&](const message_item& item)
				{
					return item.text_matches(msg);
				});
			};

			switch (location)
			{
			case message_pin_location::top:
				return check_list(m_ready_queue_top) || check_list(m_vis_items_top);
			case message_pin_location::bottom:
				return check_list(m_ready_queue_bottom) || check_list(m_vis_items_bottom);
			default:
				fmt::throw_exception("Unreachable");
			}
		}

	} // namespace overlays
} // namespace rsx
