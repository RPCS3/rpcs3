#pragma once
#include <util/types.hpp>
#include <concepts>

namespace rsx
{
    template <typename E>
    concept ErrorType = requires (E& e)
    {
        { e.empty() } -> bool;
    };

    template <typename T, ErrorType E = std::string>
    class expected
    {
        T value;
        E error{};

    public:
        expected(const T& value_)
            : value(value_)
        {}

        expected(const E& error_)
            : error(error_)
        {}

        operator T() const
        {
            ensure(!error);
            return value;
        }

        std::enable_if<!std::is_same_v<T, bool>>
        operator bool() const
        {
            return !error;
        }
    };
}
