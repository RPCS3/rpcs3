#pragma once

#include "types.hpp"
#include <type_traits>
#include <utility>

namespace utils
{
	template <typename... Ts>
		requires ((std::is_trivially_copyable_v<Ts> && std::is_trivially_destructible_v<Ts>) && ...)
	struct tuple;

	template <>
	struct tuple<>
	{
		constexpr tuple() = default;

		static constexpr usz size() noexcept { return 0; }
	};

	template <typename Head, typename... Tail>
	struct tuple<Head, Tail...> : tuple<Tail...>
	{
	private:
		Head head;

	public:
		constexpr tuple()
			: tuple<Tail...>()
			, head{}
		{}

		constexpr tuple(Head h, Tail... t)
			: tuple<Tail...>(std::forward<Tail>(t)...)
			, head(std::move(h))
		{}

		static constexpr usz size() noexcept
		{
			return 1 + sizeof...(Tail);
		}

		template <usz N>
			requires (N < size())
		constexpr auto& get()
		{
			if constexpr (N == 0)
				return head;
			else
				return tuple<Tail...>::template get<N - 1>();
		}

		template <usz N>
			requires (N < size())
		constexpr const auto& get() const
		{
			if constexpr (N == 0)
				return head;
			else
				return tuple<Tail...>::template get<N - 1>();
		}
	};
}
