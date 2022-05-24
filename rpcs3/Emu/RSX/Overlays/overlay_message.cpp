#include "stdafx.h"
#include "overlay_message.h"
#include "Emu/RSX/RSXThread.h"

namespace rsx
{
	namespace overlays
	{
		template <typename T>
		message_item::message_item(T msg_id)
		{
			m_expiration_time = get_system_time() + 5'000'000;

			m_text.set_font("Arial", 16);
			m_text.set_text(msg_id);
			m_text.auto_resize();
			m_text.back_color.a = 0.f;

			m_fade_animation.current = color4f(1.f);
			m_fade_animation.end = color4f(1.0f, 1.0f, 1.0f, 0.f);
			m_fade_animation.duration = 2.f;
			m_fade_animation.active = true;
		}
		template message_item::message_item(std::string msg_id);
		template message_item::message_item(localized_string_id msg_id);

		u64 message_item::get_expiration() const
		{
			return m_expiration_time;
		}

		compiled_resource message_item::get_compiled()
		{
			if (!m_processed)
			{
				return {};
			}

			auto cr = m_text.get_compiled();
			m_fade_animation.apply(cr);

			return cr;
		}

		void message_item::update(usz index, u64 time)
		{
			if (m_cur_pos != index)
			{
				m_cur_pos = index;
				m_text.set_pos(10, index * 18);
			}

			if ((m_expiration_time - time) < 2'000'000)
			{
				m_fade_animation.update(rsx::get_current_renderer()->vblank_count);
			}

			m_processed = true;
		}

		void message::update()
		{
			if (!visible)
			{
				return;
			}

			std::lock_guard lock(m_mutex_queue);
			u64 cur_time = get_system_time();

			while (!m_queue.empty() && m_queue.front().get_expiration() < cur_time)
			{
				m_queue.pop_front();
			}

			if (m_queue.empty())
			{
				visible = false;
				return;
			}

			usz index = 0;

			for (auto& item : m_queue)
			{
				item.update(index, cur_time);
				index++;
			}
		}

		compiled_resource message::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			std::lock_guard lock(m_mutex_queue);

			compiled_resource cr{};

			for (auto& item : m_queue)
			{
				cr.add(item.get_compiled());
			}

			return cr;
		}

	} // namespace overlays
} // namespace rsx
