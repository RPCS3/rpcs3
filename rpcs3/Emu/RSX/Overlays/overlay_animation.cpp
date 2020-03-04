#include "stdafx.h"
#include "overlay_animation.h"
#include "overlay_controls.h"
#include "Emu/system_config.h"

#include <numeric>

namespace rsx
{
    namespace overlays
    {
        void animation_base::begin_animation(u64 frame)
        {
            frame_start = frame;
            frame_end = u64(frame + duration * g_cfg.video.vblank_rate);
        }

        f32 animation_base::get_progress_ratio(u64 frame) const
        {
            if (!frame_start)
            {
                return 0.f;
            }

            f32 t = f32(frame - frame_start) / (frame_end - frame_start);

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

        void animation_translate::update(u64 frame)
        {
            if (!active)
            {
                return;
            }

            if (frame_start == 0)
            {
                start = current;
                begin_animation(frame);
                return;
            }

            if (frame >= frame_end)
            {
                // Exit condition
                finish();
                return;
            }

            f32 t = get_progress_ratio(frame);
            current = lerp(start, end, t);
        }

        void animation_translate::finish()
        {
            active = false;
            frame_start = 0;
            frame_end = 0;
            current = end; // Snap current to limit in case we went over

            if (on_finish)
            {
                on_finish();
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

        void animation_color_interpolate::update(u64 frame)
        {
            if (!active)
            {
                return;
            }

            if (frame_start == 0)
            {
                start = current;
                begin_animation(frame);
                return;
            }

            if (frame >= frame_end)
            {
                finish();
                return;
            }

            f32 t = get_progress_ratio(frame);
            current = lerp(start, end, t);
        }

        void animation_color_interpolate::finish()
        {
            active = false;
            frame_start = 0;
            frame_end = 0;
            current = end;

            if (on_finish)
            {
                on_finish();
            }
        }
    };
}
