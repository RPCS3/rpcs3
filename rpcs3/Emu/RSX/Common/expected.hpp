#pragma once
#include <util/types.hpp>
#include <concepts>
#include <utility>

namespace rsx
{
	namespace exception_utils
	{
		enum soft_exception_error_code
		{
			none = 0,
			range_exception = 1,
			invalid_enum = 2
		};

		struct soft_exception_t
		{
			soft_exception_error_code error = soft_exception_error_code::none;

			soft_exception_t() = default;
			soft_exception_t(soft_exception_error_code code)
				: error(code) {}

			bool empty() const
			{
				return error == soft_exception_error_code::none;
			}
		};
	}

	template <typename E>
	concept ErrorType = requires (E & e)
	{
		{ e.empty() } -> std::same_as<bool>;
	};

    template <typename T, ErrorType E = exception_utils::soft_exception_t>
    class expected
    {
        T value;
        E error{};

    public:
        [[ nodiscard ]] expected(const T& value_)
            : value(value_)
        {}

        [[ nodiscard ]] expected(const E& error_)
            : error(error_)
        {
            ensure(!error.empty());
        }

        operator T() const
        {
            ensure(error.empty());
            return value;
        }

        T operator *() const
        {
            ensure(error.empty());
            return value;
        }

        template<typename = std::enable_if<!std::is_same_v<T, bool>>>
        operator bool() const
        {
            return error.empty();
        }

        operator std::pair<T&, E&>() const
        {
            return { value, error };
        }

        bool operator == (const T& other) const
        {
            return error.empty() && value == other;
        }
    };
}
