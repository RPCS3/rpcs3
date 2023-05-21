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
		message_item::message_item(T msg_id, u64 expiration, std::shared_ptr<atomic_t<u32>> refs, std::shared_ptr<overlay_element> icon)
		{
			m_visible_duration = expiration;
			m_refs = std::move(refs);

			m_text.set_font("Arial", 14);
			m_text.set_text(msg_id);
			m_text.set_padding(4, 8, 4, 8);
			m_text.auto_resize();
			m_text.back_color.a = 0.f;

			m_fade_in_animation.current = color4f(1.f, 1.f, 1.f, 0.f);
			m_fade_in_animation.end = color4f(1.0f);
			m_fade_in_animation.duration = 1.f;
			m_fade_in_animation.active = true;

			m_fade_out_animation.current = color4f(1.f);
			m_fade_out_animation.end = color4f(1.f, 1.f, 1.f, 0.f);
			m_fade_out_animation.duration = 1.f;
			m_fade_out_animation.active = false;

			back_color = color4f(0.25f, 0.25f, 0.25f, 0.85f);

			if (icon)
			{
				m_icon = icon;
				m_icon->set_pos(m_text.x + m_text.w + 8, m_text.y);

				set_size(m_margin + m_text.w + m_icon->w + m_margin, m_margin + std::max(m_text.h, m_icon->h) + m_margin);
			}
			else
			{
				set_size(m_text.w + m_margin + m_margin, m_text.h + m_margin + m_margin);
			}
		}
		template message_item::message_item(std::string msg_id, u64, std::shared_ptr<atomic_t<u32>>, std::shared_ptr<overlay_element>);
		template message_item::message_item(localized_string_id msg_id, u64, std::shared_ptr<atomic_t<u32>>, std::shared_ptr<overlay_element>);

		void message_item::reset_expiration()
		{
			m_expiration_time = get_expiration_time(m_visible_duration);
		}

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
			if (!m_processed)
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

			auto& current_animation = m_fade_in_animation.active
				? m_fade_in_animation
				: m_fade_out_animation;

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
				// We are fading in.
				m_fade_in_animation.update(rsx::get_current_renderer()->vblank_count);
			}
			else if (time + u64(m_fade_out_animation.duration * 1'000'000) > get_expiration())
			{
				// We are fading out.

				// Only activate the animation if the message hasn't expired yet (prevents glitches afterwards).
				if (time <= get_expiration())
				{
					m_fade_out_animation.active = true;
				}

				m_fade_out_animation.update(rsx::get_current_renderer()->vblank_count);
			}
			else if (m_fade_out_animation.active)
			{
				// We are fading out, but the expiration was extended.

				// Reset the fade in animation to the state of the fade out animation to prevent opacity pop.
				const usz current_frame = rsx::get_current_renderer()->vblank_count;
				const f32 fade_out_progress = static_cast<f32>(m_fade_out_animation.get_remaining_frames(current_frame)) / static_cast<f32>(m_fade_out_animation.get_duration_in_frames());
				const u64 fade_in_frames_done = u64(fade_out_progress * m_fade_in_animation.get_duration_in_frames());

				m_fade_in_animation.reset(current_frame - fade_in_frames_done);
				m_fade_in_animation.active = true;
				m_fade_in_animation.update(current_frame);

				// Reset the fade out animation.
				m_fade_out_animation.reset();
			}

			m_processed = true;
		}

		void message::update_queue(std::deque<message_item>& vis_set, std::deque<message_item>& ready_set, message_pin_location origin)
		{
			const u64 cur_time = rsx::uclock();

			for (auto it = vis_set.begin(); it != vis_set.end();)
			{
				if (it->get_expiration() < cur_time)
				{
					it = vis_set.erase(it);
				}
				else
				{
					it++;
				}
			}

			while (vis_set.size() < max_visible_items && !ready_set.empty())
			{
				vis_set.emplace_back(std::move(ready_set.front()));
				ready_set.pop_front();
			}

			if (vis_set.empty())
			{
				return;
			}

			// Render reversed list. Oldest entries are furthest from the border
			constexpr u16 spacing = 4;
			u16 y_offset = 8, index = 0;
			for (auto it = vis_set.rbegin(); it != vis_set.rend(); ++it, ++index)
			{
				if (origin == message_pin_location::top) [[ likely ]]
				{
					it->update(index, cur_time, y_offset);
					y_offset += (spacing + it->h);
				}
				else
				{
					y_offset += (spacing + it->h);
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

			update_queue(m_visible_items_top, m_ready_queue_top, message_pin_location::top);
			update_queue(m_visible_items_bottom, m_ready_queue_bottom, message_pin_location::bottom);

			visible = !m_visible_items_top.empty() || !m_visible_items_bottom.empty();
		}

		compiled_resource message::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			std::lock_guard lock(m_mutex_queue);

			compiled_resource cr{};

			for (auto& item : m_visible_items_top)
			{
				cr.add(item.get_compiled());
			}

			for (auto& item : m_visible_items_bottom)
			{
				cr.add(item.get_compiled());
			}

			return cr;
		}

		bool message::message_exists(message_pin_location location, localized_string_id id, bool allow_refresh)
		{
			return message_exists(location, get_localized_u32string(id), allow_refresh);
		}

		bool message::message_exists(message_pin_location location, const std::string& msg, bool allow_refresh)
		{
			return message_exists(location, utf8_to_u32string(msg), allow_refresh);
		}

		bool message::message_exists(message_pin_location location, const std::u32string& msg, bool allow_refresh)
		{
			auto check_list = [&](std::deque<message_item>& list)
			{
				return std::any_of(list.begin(), list.end(), [&](message_item& item)
				{
					if (item.text_matches(msg))
					{
						if (allow_refresh)
						{
							item.reset_expiration();
						}
						return true;
					}
					return false;
				});
			};

			switch (location)
			{
			case message_pin_location::top:
				return check_list(m_ready_queue_top) || check_list(m_visible_items_top);
			case message_pin_location::bottom:
				return check_list(m_ready_queue_bottom) || check_list(m_visible_items_bottom);
			}

			return false;
		}

		void refresh_message_queue()
		{
			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				if (auto msg_overlay = manager->get<rsx::overlays::message>())
				{
					msg_overlay->refresh();
				}
			}
		}

	} // namespace overlays
} // namespace rsx
