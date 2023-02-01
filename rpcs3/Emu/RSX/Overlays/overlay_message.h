#pragma once

#include "overlays.h"
#include <deque>

namespace rsx
{
	namespace overlays
	{
		enum message_pin_location
		{
			top,
			bottom
		};

		class message_item : public overlay_element
		{
		public:
			template <typename T>
			message_item(T msg_id, u64 expiration, std::shared_ptr<atomic_t<u32>> refs, std::unique_ptr<overlay_element> icon = {});
			void update(usz index, u64 time, u16 origin, int grow_direction);

			u64 get_expiration() const;
			compiled_resource& get_compiled();

		private:
			label m_text{};
			std::unique_ptr<overlay_element> m_icon{};
			animation_color_interpolate m_fade_animation;

			u64 m_expiration_time = 0;
			u64 m_visible_duration = 0;
			std::shared_ptr<atomic_t<u32>> m_refs;
			bool m_processed      = false;
			usz m_cur_pos         = umax;
		};

		class message final : public overlay
		{
		public:
			void update() override;
			compiled_resource get_compiled() override;

			template <typename T>
			void queue_message(
				T msg_id,
				u64 expiration,
				std::shared_ptr<atomic_t<u32>> refs,
				message_pin_location location = message_pin_location::top,
				std::unique_ptr<overlay_element> icon = {})
			{
				std::lock_guard lock(m_mutex_queue);

				auto& queue = location == message_pin_location::top
					? m_ready_queue_top
					: m_ready_queue_bottom;

				if constexpr (std::is_same_v<T, std::initializer_list<localized_string_id>>)
				{
					for (auto id : msg_id)
					{
						queue.emplace_back(id, expiration, refs, icon);
					}
				}
				else
				{
					queue.emplace_back(msg_id, expiration, std::move(refs), std::move(icon));
				}

				visible = true;
			}

		private:
			const u32 max_visible_items = 3;
			shared_mutex m_mutex_queue;

			// Top and bottom enqueued sets
			std::deque<message_item> m_ready_queue_top;
			std::deque<message_item> m_ready_queue_bottom;

			// Top and bottom visible sets
			std::deque<message_item> m_vis_items_top;
			std::deque<message_item> m_vis_items_bottom;

			void update_queue(std::deque<message_item>& vis_set, std::deque<message_item>& ready_set, u16 origin, int grow_direction);
		};

		template <typename T>
		void queue_message(
			T msg_id,
			u64 expiration = 5'000'000,
			std::shared_ptr<atomic_t<u32>> refs = {},
			message_pin_location location = message_pin_location::top,
			std::unique_ptr<overlay_element> icon = {})
		{
			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				auto msg_overlay = manager->get<rsx::overlays::message>();
				if (!msg_overlay)
				{
					msg_overlay = std::make_shared<rsx::overlays::message>();
					msg_overlay = manager->add(msg_overlay);
				}
				msg_overlay->queue_message(msg_id, expiration, std::move(refs), location, std::move(icon));
			}
		}

	} // namespace overlays
} // namespace rsx
