#pragma once
#include <util/types.hpp>

#include <concepts>
#include <string>
#include <utility>

namespace fmt
{
	template <typename CharT, usz N, typename... Args>
	static std::string format(const CharT(&)[N], const Args&...);
}

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

			std::string to_string() const
			{
				switch (error)
				{
				case soft_exception_error_code::none:
					return "No error";
				case soft_exception_error_code::range_exception:
					return "Bad Range";
				case soft_exception_error_code::invalid_enum:
					return "Invalid enum";
				default:
					return "Unknown Error";
				}
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 26495) // disable warning for uninitialized value member (performance reasons)
#endif
        [[ nodiscard ]] expected(const E& error_)
            : error(error_)
        {
            ensure(!error.empty());
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

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

		operator bool() const
			requires(!std::is_same_v<T, bool>)
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

		std::string to_string() const
		{
			if (error.empty())
			{
				return fmt::format("%s", value);
			}

			if constexpr (std::is_same_v<E, exception_utils::soft_exception_t>)
			{
				return error.to_string();
			}

			return fmt::format("%s", error);
		}
    };
}
