#include "stdafx.h"
#include "overlay_trophy_notification.h"
#include "Emu/Cell/ErrorCodes.h"
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
			frame.set_pos(68.24, 55.34);
			frame.set_size(425, 72);
			frame.back_color.r = 0.247059f;
			frame.back_color.g = 0.250980f;
			frame.back_color.b = 0.247059f;
			frame.back_color.a = 0.88f;

			image.set_pos(78, 64);
			image.set_size(53.333, 53.333);
			image.back_color.a = 0.f;

			text_view.set_pos(139.14, 69.30);
			text_view.set_padding(0, 0, 0, 0);
			text_view.set_font("Arial", 14);
			text_view.align_text(overlay_element::text_align::center);
			text_view.back_color.a = 0.f;

			sliding_animation.duration = 1.5f;
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

			if (((t - creation_time) / 1000) > 5000)
			{
				if (!sliding_animation.active)
				{
					sliding_animation.end = { -f32(frame.w*1.25), 0, 0 };
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

			localized_string_id string_id = localized_string_id::INVALID;
			switch (trophy.trophyGrade)
			{
			case SCE_NP_TROPHY_GRADE_BRONZE: string_id = localized_string_id::RSX_OVERLAYS_TROPHY_BRONZE; break;
			case SCE_NP_TROPHY_GRADE_SILVER: string_id = localized_string_id::RSX_OVERLAYS_TROPHY_SILVER; break;
			case SCE_NP_TROPHY_GRADE_GOLD: string_id = localized_string_id::RSX_OVERLAYS_TROPHY_GOLD; break;
			case SCE_NP_TROPHY_GRADE_PLATINUM: string_id = localized_string_id::RSX_OVERLAYS_TROPHY_PLATINUM; break;
			default: break;
			}

			text_view.set_unicode_text(get_localized_u32string(string_id, trophy.name));
			text_view.auto_resize();

			// Resize background to cover the text
			u16 margin_sz = 9;
			frame.w       = 72 + text_view.w + margin_sz;

			visible = true;
			return CELL_OK;
		}
	} // namespace overlays
} // namespace rsx
