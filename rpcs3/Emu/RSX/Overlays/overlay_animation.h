#pragma once
#include "Utilities/types.h"
#include "Utilities/geometry.h"

namespace rsx
{
    template<typename T>
    struct vector3_base : public position3_base<T>
    {
        using position3_base<T>::position3_base;

        vector3_base<T>(T x, T y, T z)
        {
            this->x = x;
            this->y = y;
            this->z = z;
        }

        vector3_base<T>(const position3_base<T>& other)
        {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
        }

        vector3_base<T> operator * (const vector3_base<T>& rhs) const
        {
            return { this->x * rhs.x, this->y * rhs.y, this->z * rhs.z };
        }

        vector3_base<T> operator * (T rhs) const
        {
            return { this->x * rhs, this->y * rhs, this->z * rhs };
        }

         void operator *= (const vector3_base<T>& rhs)
        {
            this->x *= rhs.x;
            this->y *= rhs.y;
            this->z *= rhs.z;
        }

        void operator *= (T rhs)
        {
            this->x *= rhs;
            this->y *= rhs;
            this->z *= rhs;
        }

        T dot(const vector3_base<T>& rhs) const
        {
            return (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z);
        }

        T distance(const vector3_base<T>& rhs) const
        {
            const vector3_base<T> d = *this - rhs;
            return d.dot(d);
        }
    };

    using vector3i = vector3_base<int>;
    using vector3f = vector3_base<float>;

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
