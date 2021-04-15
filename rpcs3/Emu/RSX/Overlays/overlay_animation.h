#pragma once

#include "util/types.hpp"
#include "Utilities/geometry.h"
#include "overlay_utils.h"

#include <functional>

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
        protected:
            u64 frame_start = 0;
            u64 frame_end = 0;

            void begin_animation(u64 frame);
            f32 get_progress_ratio(u64 frame) const;

            template<typename T>
            static T lerp(const T& a, const T& b, f32 t)
            {
                return (a * (1.f - t)) + (b * t);
            }

        public:
            bool active = false;
            animation_type type { animation_type::linear };
            f32 duration = 1.f; // in seconds

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
        public:
            vector3f current{};
            vector3f end{};

            void apply(compiled_resource& data) override;
            void update(u64 frame) override;
            void finish();
        };

        struct animation_color_interpolate : animation_translate
        {
        private:
            color4f start{};

        public:
            color4f current{};
            color4f end{};

            void apply(compiled_resource& data) override;
            void update(u64 frame) override;
            void finish();
        };
    }
}
