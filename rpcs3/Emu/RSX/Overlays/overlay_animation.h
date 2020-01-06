#pragma once
#include "Utilities/types.h"
#include "Utilities/geometry.h"
#include "overlay_utils.h"

namespace rsx
{
    namespace overlays
    {
        struct compiled_resource;

        enum class animation_type
        {
            linear,
            ease_in_quad,
            ease_out_quad,
            ease_in_out_cubic,
        };

        struct animation_base
        {
            bool active = false;
            animation_type type { animation_type::linear };

            std::function<void()> on_finish;

            virtual void apply(compiled_resource&) = 0;
            virtual void update(u64 frame) = 0;
        };

        struct animation_translate : animation_base
        {
        private:
            vector3f start{}; // Set `current` instead of this
                              // NOTE: Necessary because update() is called after rendering,
                              //       resulting in one frame of incorrect translation
            u64 frame_start = 0;
            u64 frame_end = 0;
        public:
            vector3f current{};
            vector3f end{};
            f32 duration{}; // in seconds

            void apply(compiled_resource& data) override;
            void update(u64 frame) override;
            void finish();
        };
    }
}
