#include "stdafx.h"
#include "overlay_animation.h"
#include "overlay_controls.h"

namespace rsx
{
    namespace overlays
    {
        void animation_translate::apply(compiled_resource& resource)
        {
            if (progress == vector3i(0, 0, 0))
            {
                return;
            }

            const vertex delta = { static_cast<float>(progress.x), static_cast<float>(progress.y), static_cast<float>(progress.z), 0.f };
            for (auto& cmd : resource.draw_commands)
            {
                for (auto& v : cmd.verts)
                {
                    v += delta;
                }
            }
        }

        void animation_translate::update(u64 t)
        {
            if (!active)
            {
                return;
            }

            if (time == 0)
            {
                time = t;
                return;
            }

            const u64 delta = (t - time);
            const u64 frames = delta / 16667;

            if (frames)
            {
                const int mag = frames * speed;
                const vector3i new_progress = progress + direction * mag;
                const auto d = new_progress.distance(progress_limit);

                if (distance >= 0)
                {
                    if (d > distance)
                    {
                        // Exit condition
                        finish();
                        return;
                    }
                }

                progress = new_progress;
                distance = d;
                time = t;
            }
        }

        void animation_translate::finish()
        {
            active = false;
            distance = -1;
            time = 0;

            if (on_finish)
            {
                on_finish();
            }
        }
    };
}
