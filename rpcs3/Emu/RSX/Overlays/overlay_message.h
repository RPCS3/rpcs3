#pragma once

#include "overlays.h"
#include <deque>

namespace rsx
{
	namespace overlays
	{
		class message_item
		{
		public:
			template <typename T>
			message_item(T msg_id);
			void update(usz index, u64 time);

			u64 get_expiration() const;
			compiled_resource get_compiled();

		private:
			label m_text{};
			animation_color_interpolate m_fade_animation;

			u64 m_expiration_time = 0;
			bool m_processed      = false;
			usz m_cur_pos         = umax;
		};

		class message final : public user_interface
		{
		public:
			void update() override;
			compiled_resource get_compiled() override;

			template <typename T>
			void queue_message(T msg_id)
			{
				std::lock_guard lock(m_mutex_queue);

				if constexpr (std::is_same_v<T, std::initializer_list<localized_string_id>>)
				{
					for (auto id : msg_id)
					{
						m_queue.emplace_back(id);
					}
				}
				else
				{
					m_queue.emplace_back(msg_id);
				}

				visible = true;
			}

		private:
			shared_mutex m_mutex_queue;
			std::deque<message_item> m_queue;
		};

		template <typename T>
		void queue_message(T msg_id)
		{
			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				auto msg_overlay = manager->get<rsx::overlays::message>();
				if (!msg_overlay)
				{
					msg_overlay = std::make_shared<rsx::overlays::message>();
					msg_overlay = manager->add(msg_overlay);
				}
				msg_overlay->queue_message(msg_id);
			}
		}

	} // namespace overlays
} // namespace rsx
