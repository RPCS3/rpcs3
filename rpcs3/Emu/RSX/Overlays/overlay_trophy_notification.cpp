#include "stdafx.h"
#include "overlay_trophy_notification.h"
#include "Emu/RSX/RSXThread.h"

namespace rsx
{
	namespace overlays
	{
		// TODO: Move somewhere in rsx_utils or general utils if needed anywhere else
		class ticket_semaphore_t
		{
			atomic_t<u64> acquired{0};
			atomic_t<u64> released{0};

		public:
			u64 enqueue()
			{
				return acquired.fetch_add(1);
			}

			bool try_acquire(u64 queue_id) const
			{
				return (queue_id == released.load());
			}

			void release()
			{
				released++;
			}
		};

		static ticket_semaphore_t s_trophy_semaphore;

		trophy_notification::trophy_notification()
		{
			frame.set_pos(0, 0);
			frame.set_size(300, 80);
			frame.back_color.a = 0.85f;

			image.set_pos(8, 8);
			image.set_size(64, 64);
			image.back_color.a = 0.f;

			text_view.set_pos(85, 0);
			text_view.set_padding(0, 0, 24, 0);
			text_view.set_font("Arial", 9);
			text_view.align_text(overlay_element::text_align::center);
			text_view.back_color.a = 0.f;

			sliding_animation.duration = 1.5;
			sliding_animation.type = animation_type::ease_in_out_cubic;
			sliding_animation.current = { -f32(frame.w), 0, 0 };
			sliding_animation.end = { 0, 0, 0};
			sliding_animation.active = true;
		}

		void trophy_notification::update()
		{
			if (!s_trophy_semaphore.try_acquire(display_sched_id))
			{
				// Not scheduled to run just yet
				return;
			}

			const u64 t = get_system_time();
			if (!creation_time)
			{
				// First tick
				creation_time = t;
				return;
			}

			if (((t - creation_time) / 1000) > 7500)
			{
				if (!sliding_animation.active)
				{
					sliding_animation.end = { -f32(frame.w), 0, 0 };
					sliding_animation.on_finish = [this]
					{
						s_trophy_semaphore.release();
						close(false, false);
					};

					sliding_animation.active = true;
				}
			}

			if (sliding_animation.active)
			{
				sliding_animation.update(rsx::get_current_renderer()->vblank_count);
			}
		}

		compiled_resource trophy_notification::get_compiled()
		{
			if (!creation_time || !visible)
			{
				return {};
			}

			auto result = frame.get_compiled();
			result.add(image.get_compiled());
			result.add(text_view.get_compiled());

			sliding_animation.apply(result);
			return result;
		}

		s32 trophy_notification::show(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer)
		{
			// Schedule to display this trophy
			display_sched_id = s_trophy_semaphore.enqueue();
			visible = false;

			if (!trophy_icon_buffer.empty())
			{
				icon_info = std::make_unique<image_info>(trophy_icon_buffer);
				image.set_raw_image(icon_info.get());
			}

			std::string trophy_message;
			switch (trophy.trophyGrade)
			{
			case SCE_NP_TROPHY_GRADE_BRONZE: trophy_message = "bronze"; break;
			case SCE_NP_TROPHY_GRADE_SILVER: trophy_message = "silver"; break;
			case SCE_NP_TROPHY_GRADE_GOLD: trophy_message = "gold"; break;
			case SCE_NP_TROPHY_GRADE_PLATINUM: trophy_message = "platinum"; break;
			default: break;
			}

			trophy_message = "You have earned the " + trophy_message + " trophy\n" + trophy.name;
			text_view.set_text(trophy_message);
			text_view.auto_resize();

			// Resize background to cover the text
			u16 margin_sz = text_view.x - image.w - image.x;
			frame.w       = text_view.x + text_view.w + margin_sz;

			visible = true;
			return CELL_OK;
		}
	} // namespace overlays
} // namespace rsx
