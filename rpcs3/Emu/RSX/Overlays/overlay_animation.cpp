#include "stdafx.h"
#include "overlay_animation.h"
#include "overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		void animation_base::begin_animation(u64 timestamp_us)
		{
			timestamp_start_us = timestamp_us;
			timestamp_end_us = timestamp_us + get_total_duration_us();
		}

		u64 animation_base::get_total_duration_us() const
		{
			return u64(duration_sec * 1'000'000.f);
		}

		u64 animation_base::get_remaining_duration_us(u64 timestamp_us) const
		{
			return timestamp_us >= timestamp_end_us ? 0 : (timestamp_end_us - timestamp_us);
		}

		f32 animation_base::get_progress_ratio(u64 timestamp_us) const
		{
			if (!timestamp_start_us)
			{
				return 0.f;
			}

			f32 t = f32(timestamp_us - timestamp_start_us) / (timestamp_end_us - timestamp_start_us);

			switch (type) {
			case animation_type::linear:
				break;
			case animation_type::ease_in_quad:
				t = t * t;
				break;
			case animation_type::ease_out_quad:
				t = t * (2.0f - t);
				break;
			case animation_type::ease_in_out_cubic:
				t = t > 0.5f ? 4.0f * std::pow((t - 1.0f), 3.0f) + 1.0f : 4.0f * std::pow(t, 3.0f);
				break;
			}

			return t;
		}

		void animation_translate::reset(u64 start_timestamp_us)
		{
			active = false;
			current = start;
			timestamp_start_us = start_timestamp_us;

			if (timestamp_start_us > 0)
			{
				timestamp_end_us = timestamp_start_us + get_total_duration_us();
			}
		}

		void animation_translate::apply(compiled_resource& resource)
		{
			if (!active)
			{
				return;
			}

			const vertex delta = { current.x, current.y, current.z, 0.f };
			for (auto& cmd : resource.draw_commands)
			{
				for (auto& v : cmd.verts)
				{
					v += delta;
				}
			}
		}

		void animation_translate::update(u64 timestamp_us)
		{
			if (!active)
			{
				return;
			}

			if (timestamp_start_us == 0)
			{
				start = current;
				begin_animation(timestamp_us);
				return;
			}

			if (timestamp_us >= timestamp_end_us)
			{
				// Exit condition
				finish();
				return;
			}

			f32 t = get_progress_ratio(timestamp_us);
			current = lerp(start, end, t);
		}

		void animation_translate::finish()
		{
			active = false;
			timestamp_start_us = 0;
			timestamp_end_us = 0;
			current = end; // Snap current to limit in case we went over

			if (on_finish)
			{
				on_finish();
			}
		}

		void animation_color_interpolate::reset(u64 start_timestamp_us)
		{
			active = false;
			current = start;
			timestamp_start_us = start_timestamp_us;

			if (timestamp_start_us > 0)
			{
				timestamp_end_us = timestamp_start_us + get_total_duration_us();
			}
		}

		void animation_color_interpolate::apply(compiled_resource& data)
		{
			if (!active)
			{
				return;
			}

			for (auto& cmd : data.draw_commands)
			{
				cmd.config.color *= current;
			}
		}

		void animation_color_interpolate::update(u64 timestamp_us)
		{
			if (!active)
			{
				return;
			}

			if (timestamp_start_us == 0)
			{
				start = current;
				begin_animation(timestamp_us);
				return;
			}

			if (timestamp_us >= timestamp_end_us)
			{
				finish();
				return;
			}

			f32 t = get_progress_ratio(timestamp_us);
			current = lerp(start, end, t);
		}

		void animation_color_interpolate::finish()
		{
			active = false;
			timestamp_start_us = 0;
			timestamp_end_us = 0;
			current = end;

			if (on_finish)
			{
				on_finish();
			}
		}
	};
}
