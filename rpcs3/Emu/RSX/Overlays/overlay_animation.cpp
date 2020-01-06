#include "stdafx.h"
#pragma optimize("", off)
#include "overlay_animation.h"
#include "overlay_controls.h"

#include <numeric>

namespace rsx
{
    namespace overlays
    {
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
                frame_start = frame;
                frame_end = u64(frame + duration * g_cfg.video.vblank_rate);

                return;
            }

            if (frame >= frame_end)
            {
                // Exit condition
                finish();
                return;
            }

            f32 t = f32(frame - frame_start) / (frame_end - frame_start);

            switch (type) {
            case animation_type::linear:
                break;
            case animation_type::ease_in_quad:
                t = t * t;
                break;
            case animation_type::ease_out_quad:
                t = t * (2.0 - t);
                break;
            case animation_type::ease_in_out_cubic:
                t = t > 0.5 ? 4.0 * std::pow((t - 1.0), 3.0) + 1.0 : 4.0 * std::pow(t, 3.0);
                break;
            }

            current = (1.f - t) * start + t * end;
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
    };
}
