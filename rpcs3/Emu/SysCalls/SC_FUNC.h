#pragma once
#include "Emu/Cell/PPUThread.h"

using ppu_func_caller = void(*)(PPUThread&);

namespace ppu_func_detail
{
	enum arg_class : u8
	{
		ARG_GENERAL,
		ARG_FLOAT,
		ARG_VECTOR,
		ARG_STACK,
	};

	template<typename T, arg_class type, u8 g_count, u8 f_count, u8 v_count>
	struct bind_arg;

	template<typename T, u8 g_count, u8 f_count, u8 v_count>
	struct bind_arg<T, ARG_GENERAL, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_GENERAL");

		static force_inline T get_arg(PPUThread& CPU)
		{
			return cast_from_ppu_gpr<T>(CPU.GPR[g_count + 2]);
		}
	};

	template<typename T, u8 g_count, u8 f_count, u8 v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		static force_inline T get_arg(PPUThread& CPU)
		{
			return static_cast<T>(CPU.FPR[f_count]);
		}
	};

	template<typename T, u8 g_count, u8 f_count, u8 v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, u128>::value, "Invalid function argument type for ARG_VECTOR");

		static force_inline T get_arg(PPUThread& CPU)
		{
			return CPU.VPR[v_count + 1];
		}
	};

	template<typename T, u8 g_count, u8 f_count, u8 v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 13, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 12, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_STACK");

		static force_inline T get_arg(PPUThread& CPU)
		{
			// TODO: check stack argument displacement
			const u64 res = CPU.GetStackArg(8 + std::max(g_count - 8, 0) + std::max(f_count - 13, 0) + std::max(v_count - 12, 0));
			return cast_from_ppu_gpr<T>(res);
		}
	};

	template<typename T, arg_class type>
	struct bind_result
	{
		static_assert(type == ARG_GENERAL, "Wrong use of bind_result template");
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_GENERAL");

		static force_inline void put_result(PPUThread& CPU, const T& result)
		{
			CPU.GPR[3] = cast_to_ppu_gpr<T>(result);
		}
	};

	template<typename T>
	struct bind_result<T, ARG_FLOAT>
	{
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_FLOAT");

		static force_inline void put_result(PPUThread& CPU, const T& result)
		{
			CPU.FPR[1] = static_cast<T>(result);
		}
	};

	template<typename T>
	struct bind_result<T, ARG_VECTOR>
	{
		static_assert(std::is_same<T, u128>::value, "Invalid function result type for ARG_VECTOR");

		static force_inline void put_result(PPUThread& CPU, const T& result)
		{
			CPU.VPR[2] = result;
		}
	};

	struct arg_type_pack
	{
		arg_class type;
		u8 g_count;
		u8 f_count;
		u8 v_count;
	};

	template<typename T, u32 type_pack>
	struct bind_arg_packed
	{
		static force_inline T get_arg(PPUThread& CPU)
		{
			return bind_arg<T, type_pack, (type_pack >> 8), (type_pack >> 16), (type_pack >> 24)>::get_arg(CPU);
		}
	};

	template <typename RT, typename F, typename Tuple, bool Done, int Total, int... N>
	struct call_impl
	{
		static force_inline RT call(F f, Tuple && t)
		{
			return call_impl<RT, F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t));
		}
	};

	template <typename RT, typename F, typename Tuple, int Total, int... N>
	struct call_impl<RT, F, Tuple, true, Total, N...>
	{
		static force_inline RT call(F f, Tuple && t)
		{
			return f(std::get<N>(std::forward<Tuple>(t))...);
		}
	};

	template <typename RT, typename F, typename Tuple>
	force_inline RT call(F f, Tuple && t)
	{
		using ttype = std::decay_t<Tuple>;
		return ppu_func_detail::call_impl<RT, F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
	}

	template<u32 g_count, u32 f_count, u32 v_count>
	force_inline std::tuple<> iterate(PPUThread& CPU)
	{
		// terminator
		return std::tuple<>();
	}

	template<u32 g_count, u32 f_count, u32 v_count, typename T, typename... A>
	force_inline std::tuple<T, A...> iterate(PPUThread& CPU)
	{
		static_assert(!std::is_pointer<T>::value, "Invalid function argument type (pointer)");
		static_assert(!std::is_reference<T>::value, "Invalid function argument type (reference)");
		// TODO: check calculations
		const bool is_float = std::is_floating_point<T>::value;
		const bool is_vector = std::is_same<T, u128>::value;
		const arg_class t = is_float
			? ((f_count >= 13) ? ARG_STACK : ARG_FLOAT)
			: (is_vector ? ((v_count >= 12) ? ARG_STACK : ARG_VECTOR) : ((g_count >= 8) ? ARG_STACK : ARG_GENERAL));
		const u32 g = g_count + (is_float || is_vector ? 0 : 1);
		const u32 f = f_count + (is_float ? 1 : 0);
		const u32 v = v_count + (is_vector ? 1 : 0);

		return std::tuple_cat(std::tuple<T>(bind_arg<T, t, g, f, v>::get_arg(CPU)), iterate<g, f, v, A...>(CPU));
	}

	template<typename RT>
	struct result_type
	{
		static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
		static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
		static const bool is_float = std::is_floating_point<RT>::value;
		static const bool is_vector = std::is_same<RT, u128>::value;
		static const arg_class value = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);
	};

	template<typename RT, typename... T>
	struct func_binder;

	template<typename... T>
	struct func_binder<void, PPUThread&, T...>
	{
		using func_t = void(*)(PPUThread&, T...);

		static void do_call(PPUThread& CPU, func_t func)
		{
			call<void>(func, std::tuple_cat(std::tuple<PPUThread&>(CPU), iterate<0, 0, 0, T...>(CPU)));
		}
	};

	template<typename... T>
	struct func_binder<void, T...>
	{
		using func_t = void(*)(T...);

		static void do_call(PPUThread& CPU, func_t func)
		{
			call<void>(func, iterate<0, 0, 0, T...>(CPU));
		}
	};

	template<typename RT, typename... T>
	struct func_binder<RT, PPUThread&, T...>
	{
		using func_t = RT(*)(PPUThread&, T...);

		static void do_call(PPUThread& CPU, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(CPU, call<RT>(func, std::tuple_cat(std::tuple<PPUThread&>(CPU), iterate<0, 0, 0, T...>(CPU))));
		}
	};

	template<typename RT, typename... T>
	struct func_binder
	{
		using func_t = RT(*)(T...);

		static void do_call(PPUThread& CPU, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(CPU, call<RT>(func, iterate<0, 0, 0, T...>(CPU)));
		}
	};
}

template<typename RT, typename... T> force_inline void call_ppu_func(PPUThread& CPU, RT(*func)(T...))
{
	ppu_func_detail::func_binder<RT, T...>::do_call(CPU, func);
}

#define bind_func(func) [](PPUThread& CPU){ call_ppu_func(CPU, func); }
