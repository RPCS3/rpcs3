#pragma once
#include "Utilities/types.h"
#include "Utilities/geometry.h"
#include "overlay_utils.h"

namespace rsx
{
    namespace overlays
    {
        struct compiled_resource;

        struct animation_base
        {
            bool active = false;

            virtual void apply(compiled_resource&) = 0;
            virtual void update(u64 t) = 0;
        };

        struct animation_translate : animation_base
        {
            vector3i direction{};
            vector3i progress{};
            vector3i progress_limit{};

            std::function<void()> on_finish;

            int speed = 0;
            int distance = -1;
            u64 time = 0;

            void apply(compiled_resource& data) override;
            void update(u64 t) override;
            void finish();
        };
    }
}
