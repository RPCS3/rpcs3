#pragma once

#include "overlays.h"
#include "overlay_manager.h"

#include <deque>

namespace rsx
{
	namespace overlays
	{
		enum class message_pin_location
		{
			bottom_right,
			bottom_left,
			top_right,
			top_left
		};

		class message_item : public rounded_rect
		{
		public:
			template <typename T>
			message_item(const T& msg_id, u64 expiration, std::shared_ptr<atomic_t<u32>> refs, std::shared_ptr<overlay_element> icon = {});
			void update(usz index, u64 timestamp_us, s16 x_offset, s16 y_offset);
			void set_pos(s16 _x, s16 _y) override;

			void reset_expiration();
			u64 get_expiration() const;
			void ensure_expired();
			compiled_resource& get_compiled() override;

			bool text_matches(const std::u32string& text) const;

		private:
			label m_text{};
			std::shared_ptr<overlay_element> m_icon{};
			animation_color_interpolate m_fade_in_animation;
			animation_color_interpolate m_fade_out_animation;

			u64 m_expiration_time = 0;
			u64 m_visible_duration = 0;
			std::shared_ptr<atomic_t<u32>> m_refs;
			bool m_processed = false;
			usz m_cur_pos = umax;
			static constexpr u16 m_margin = 6;
		};

		class message final : public overlay
		{
		public:
			void update(u64 timestamp_us) override;
			compiled_resource get_compiled() override;

			template <typename T>
			void queue_message(
				T msg_id,
				u64 expiration,
				std::shared_ptr<atomic_t<u32>> refs,
				message_pin_location location = message_pin_location::top_left,
				std::shared_ptr<overlay_element> icon = {},
				bool allow_refresh = false)
			{
				std::lock_guard lock(m_mutex_queue);

				auto* queue = &m_ready_queue_top_left;

				switch (location)
				{
				case message_pin_location::bottom_right:
					queue = &m_ready_queue_bottom_right;
					break;
				case message_pin_location::bottom_left:
					queue = &m_ready_queue_bottom_left;
					break;
				case message_pin_location::top_right:
					queue = &m_ready_queue_top_right;
					break;
				case message_pin_location::top_left:
					queue = &m_ready_queue_top_left;
					break;
				}

				if constexpr (std::is_same_v<T, std::initializer_list<localized_string_id>>)
				{
					for (auto id : msg_id)
					{
						if (!message_exists(location, id, allow_refresh))
						{
							queue->emplace_back(id, expiration, refs, icon);
						}
					}
				}
				else if (!message_exists(location, msg_id, allow_refresh))
				{
					queue->emplace_back(msg_id, expiration, std::move(refs), icon);
				}

				visible = true;
				refresh();
			}

		private:
			const u32 max_visible_items = 3;
			shared_mutex m_mutex_queue;

			// Top and bottom enqueued sets
			std::deque<message_item> m_ready_queue_bottom_right;
			std::deque<message_item> m_ready_queue_bottom_left;
			std::deque<message_item> m_ready_queue_top_right;
			std::deque<message_item> m_ready_queue_top_left;

			// Top and bottom visible sets
			std::deque<message_item> m_visible_items_bottom_right;
			std::deque<message_item> m_visible_items_bottom_left;
			std::deque<message_item> m_visible_items_top_right;
			std::deque<message_item> m_visible_items_top_left;

			void update_queue(std::deque<message_item>& vis_set, std::deque<message_item>& ready_set, message_pin_location origin);

			// Stacking. Extends the lifetime of a message instead of inserting a duplicate
			bool message_exists(message_pin_location location, localized_string_id id, bool allow_refresh);
			bool message_exists(message_pin_location location, const std::string& msg, bool allow_refresh);
			bool message_exists(message_pin_location location, const std::u32string& msg, bool allow_refresh);
		};

		template <typename T>
		void queue_message(
			T msg_id,
			u64 expiration = 5'000'000,
			std::shared_ptr<atomic_t<u32>> refs = {},
			message_pin_location location = message_pin_location::top_left,
			std::shared_ptr<overlay_element> icon = {},
			bool allow_refresh = false)
		{
			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				auto msg_overlay = manager->get<rsx::overlays::message>();
				if (!msg_overlay)
				{
					msg_overlay = std::make_shared<rsx::overlays::message>();
					msg_overlay = manager->add(msg_overlay);
				}
				msg_overlay->queue_message(msg_id, expiration, std::move(refs), location, std::move(icon), allow_refresh);
			}
		}

		void refresh_message_queue();

	} // namespace overlays
} // namespace rsx
