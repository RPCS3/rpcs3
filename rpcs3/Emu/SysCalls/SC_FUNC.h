#pragma once
#include "Emu/Cell/PPUThread.h"

#if defined(_MSC_VER)
typedef void(*ps3_func_caller)(PPUThread&);
#else
typedef ps3_func_caller std::function<void(PPUThread&)>;
#endif

namespace ppu_func_detail
{
	enum bind_arg_type
	{
		ARG_GENERAL,
		ARG_FLOAT,
		ARG_VECTOR,
		ARG_STACK,
	};

	template<typename T, bind_arg_type type, int g_count, int f_count, int v_count>
	struct bind_arg;

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_GENERAL, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_GENERAL");

		static __forceinline T func(PPUThread& CPU)
		{
			return cast_from_ppu_gpr<T>(CPU.GPR[g_count + 2]);
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		static __forceinline T func(PPUThread& CPU)
		{
			return static_cast<T>(CPU.FPR[f_count]);
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, u128>::value, "Invalid function argument type for ARG_VECTOR");

		static __forceinline T func(PPUThread& CPU)
		{
			return CPU.VPR[v_count + 1];
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 13, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 12, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_STACK");

		static __forceinline T func(PPUThread& CPU)
		{
			// TODO: check stack argument displacement
			const u64 res = CPU.GetStackArg(8 + std::max(g_count - 8, 0) + std::max(f_count - 13, 0) + std::max(v_count - 12, 0));
			return cast_from_ppu_gpr<T>(res);
		}
	};

	template<typename T, bind_arg_type type>
	struct bind_result
	{
		static_assert(type == ARG_GENERAL, "Wrong use of bind_result template");
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_GENERAL");

		static __forceinline void func(PPUThread& CPU, const T& result)
		{
			CPU.GPR[3] = cast_to_ppu_gpr<T>(result);
		}
	};

	template<typename T>
	struct bind_result<T, ARG_FLOAT>
	{
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_FLOAT");

		static __forceinline void func(PPUThread& CPU, const T& result)
		{
			CPU.FPR[1] = static_cast<T>(result);
		}
	};

	template<typename T>
	struct bind_result<T, ARG_VECTOR>
	{
		static_assert(std::is_same<T, u128>::value, "Invalid function result type for ARG_VECTOR");

		static __forceinline void func(PPUThread& CPU, const T& result)
		{
			CPU.VPR[2] = result;
		}
	};

	template <typename RT, typename F, typename Tuple, bool Done, int Total, int... N>
	struct call_impl
	{
		static __forceinline RT call(F f, Tuple && t)
		{
			return call_impl<RT, F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t));
		}
	};

	template <typename RT, typename F, typename Tuple, int Total, int... N>
	struct call_impl<RT, F, Tuple, true, Total, N...>
	{
		static __forceinline RT call(F f, Tuple && t)
		{
			return f(std::get<N>(std::forward<Tuple>(t))...);
		}
	};

	template <typename RT, typename F, typename Tuple>
	__forceinline RT call(F f, Tuple && t)
	{
		typedef typename std::decay<Tuple>::type ttype;
		return ppu_func_detail::call_impl<RT, F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
	}

	template<int g_count, int f_count, int v_count>
	__forceinline std::tuple<> iterate(PPUThread& CPU)
	{
		// terminator
		return std::tuple<>();
	}

	template<int g_count, int f_count, int v_count, typename T, typename... A>
	__forceinline std::tuple<T, A...> iterate(PPUThread& CPU)
	{
		static_assert(!std::is_pointer<T>::value, "Invalid function argument type (pointer)");
		static_assert(!std::is_reference<T>::value, "Invalid function argument type (reference)");
		// TODO: check calculations
		const bool is_float = std::is_floating_point<T>::value;
		const bool is_vector = std::is_same<T, u128>::value;
		const bind_arg_type t = is_float
			? ((f_count >= 13) ? ARG_STACK : ARG_FLOAT)
			: (is_vector ? ((v_count >= 12) ? ARG_STACK : ARG_VECTOR) : ((g_count >= 8) ? ARG_STACK : ARG_GENERAL));
		const int g = g_count + (is_float || is_vector ? 0 : 1);
		const int f = f_count + (is_float ? 1 : 0);
		const int v = v_count + (is_vector ? 1 : 0);

		return std::tuple_cat(std::tuple<T>(bind_arg<T, t, g, f, v>::func(CPU)), iterate<g, f, v, A...>(CPU));
	}

	template<typename RT>
	struct result_type
	{
		static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
		static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
		static const bool is_float = std::is_floating_point<RT>::value;
		static const bool is_vector = std::is_same<RT, u128>::value;
		static const bind_arg_type value = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);
	};

	template<void* func, typename RT, typename... T>
	struct func_binder;

	template<void* func, typename... T>
	struct func_binder<func, void, T...>
	{
		typedef void(*func_t)(T...);

		static void do_call(PPUThread& CPU, func_t _func)
		{
			call<void>(_func, iterate<0, 0, 0, T...>(CPU));
		}

		static void do_call(PPUThread& CPU)
		{
			do_call(CPU, (func_t)func);
		}
	};

	template<void* func, typename... T>
	struct func_binder<func, void, PPUThread&, T...>
	{
		typedef void(*func_t)(PPUThread&, T...);

		static void do_call(PPUThread& CPU, func_t _func)
		{
			call<void>(_func, std::tuple_cat(std::tuple<PPUThread&>(CPU), iterate<0, 0, 0, T...>(CPU)));
		}

		static void do_call(PPUThread& CPU)
		{
			do_call(CPU, (func_t)func);
		}
	};

	template<void* func, typename RT, typename... T>
	struct func_binder
	{
		typedef RT(*func_t)(T...);

		static void do_call(PPUThread& CPU, func_t _func)
		{
			bind_result<RT, result_type<RT>::value>::func(CPU, call<RT>(_func, iterate<0, 0, 0, T...>(CPU)));
		}

		static void do_call(PPUThread& CPU)
		{
			do_call(CPU, (func_t)func);
		}
	};

	template<void* func, typename RT, typename... T>
	struct func_binder<func, RT, PPUThread&, T...>
	{
		typedef RT(*func_t)(PPUThread&, T...);

		static void do_call(PPUThread& CPU, func_t _func)
		{
			bind_result<RT, result_type<RT>::value>::func(CPU, call<RT>(_func, std::tuple_cat(std::tuple<PPUThread&>(CPU), iterate<0, 0, 0, T...>(CPU))));
		}

		static void do_call(PPUThread& CPU)
		{
			do_call(CPU, (func_t)func);
		}
	};

	template<void* func, typename RT, typename... T>
	ps3_func_caller _bind_func(RT(*_func)(T...))
	{
#if defined(_MSC_VER)
		return ppu_func_detail::func_binder<func, RT, T...>::do_call;
#else
		return [_func](PPUThread& CPU){ ppu_func_detail::func_binder<func, RT, T...>::do_call(CPU, _func); };
#endif
	}
}

#if defined(_MSC_VER)
#define _targ(name) name
#else
#define _targ(name) nullptr
#endif

#define bind_func(func) ppu_func_detail::_bind_func<_targ(func)>(func)
