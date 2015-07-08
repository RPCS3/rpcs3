#pragma once

#include "Emu/Cell/PPUThread.h"

using ppu_func_caller = void(*)(PPUThread&);

namespace ppu_func_detail
{
	// argument type classification
	enum arg_class : u32
	{
		ARG_GENERAL, // argument is stored in GPR registers (from r3 to r10)
		ARG_FLOAT, // argument is stored in FPR registers (from f1 to f12)
		ARG_VECTOR, // argument is stored in VPR registers (from v2 to v13)
		ARG_STACK,
	};

	template<typename T, arg_class type, u32 g_count, u32 f_count, u32 v_count> struct bind_arg;

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_GENERAL, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_GENERAL");

		static force_inline T get_arg(PPUThread& CPU)
		{
			return cast_from_ppu_gpr<T>(CPU.GPR[g_count + 2]);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		static force_inline T get_arg(PPUThread& CPU)
		{
			return static_cast<T>(CPU.FPR[f_count]);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, u128>::value, "Invalid function argument type for ARG_VECTOR");

		static force_inline T get_arg(PPUThread& CPU)
		{
			return CPU.VPR[v_count + 1];
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 13, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 12, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_STACK");

		static force_inline T get_arg(PPUThread& CPU)
		{
			// TODO: check stack argument displacement
			const u64 res = CPU.GetStackArg(8 + std::max<u32>(g_count - 8, 0) + std::max<u32>(f_count - 13, 0) + std::max<u32>(v_count - 12, 0));
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

	// wrapper for variadic argument info list, each value contains packed argument type and counts of GENERAL, FLOAT and VECTOR arguments
	template<u32... Values> struct arg_info_pack_t;

	template<u32 First, u32... Values> struct arg_info_pack_t<First, Values...>
	{
		static const u32 last_value = arg_info_pack_t<Values...>::last_value;
	};

	template<u32 First> struct arg_info_pack_t<First>
	{
		static const u32 last_value = First;
	};

	template<> struct arg_info_pack_t<>
	{
		static const u32 last_value = 0;
	};

	// argument unpacker
	template<typename T, u32 type_pack> struct bind_arg_packed
	{
		static force_inline T get_arg(PPUThread& CPU)
		{
			return bind_arg<T, static_cast<arg_class>(type_pack & 0xff), (type_pack >> 8) & 0xff, (type_pack >> 16) & 0xff, (type_pack >> 24)>::get_arg(CPU);
		}
	};

	template<u32... Info, typename RT, typename... Args>
	force_inline RT call(PPUThread& CPU, RT(*func)(Args...), arg_info_pack_t<Info...>)
	{
		return func(bind_arg_packed<Args, Info>::get_arg(CPU)...);
	}

	template<u32... Info, typename RT, typename... Args>
	force_inline RT call(PPUThread& CPU, RT(*func)(PPUThread&, Args...), arg_info_pack_t<Info...>)
	{
		return func(CPU, bind_arg_packed<Args, Info>::get_arg(CPU)...);
	}

	template<typename T, typename... Types, u32... Info, typename RT, typename... Args>
	force_inline RT call(PPUThread& CPU, RT(*func)(Args...), arg_info_pack_t<Info...> info)
	{
		static_assert(!std::is_pointer<T>::value, "Invalid function argument type (pointer)");
		static_assert(!std::is_reference<T>::value, "Invalid function argument type (reference)");

		// unpack previous type counts (0/0/0 for the first time)
		const u32 g_count = (info.last_value >> 8) & 0xff;
		const u32 f_count = (info.last_value >> 16) & 0xff;
		const u32 v_count = (info.last_value >> 24);

		// TODO: check calculations
		const bool is_float = std::is_floating_point<T>::value;
		const bool is_vector = std::is_same<T, u128>::value;
		const arg_class t = is_float
			? ((f_count >= 13) ? ARG_STACK : ARG_FLOAT)
			: (is_vector ? ((v_count >= 12) ? ARG_STACK : ARG_VECTOR) : ((g_count >= 8) ? ARG_STACK : ARG_GENERAL));
		const u32 g = g_count + (is_float || is_vector ? 0 : 1);
		const u32 f = f_count + (is_float ? 1 : 0);
		const u32 v = v_count + (is_vector ? 1 : 0);

		return call<Types...>(CPU, func, arg_info_pack_t<Info..., t | (g << 8) | (f << 16) | (v << 24)>{});
	}

	template<typename RT> struct result_type
	{
		static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
		static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
		static const bool is_float = std::is_floating_point<RT>::value;
		static const bool is_vector = std::is_same<RT, u128>::value;
		static const arg_class value = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);
	};

	template<typename RT, typename... T> struct func_binder;

	template<typename... T>
	struct func_binder<void, PPUThread&, T...>
	{
		using func_t = void(*)(PPUThread&, T...);

		static force_inline void do_call(PPUThread& CPU, func_t func)
		{
			call<T...>(CPU, func, arg_info_pack_t<>{});
		}
	};

	template<typename... T>
	struct func_binder<void, T...>
	{
		using func_t = void(*)(T...);

		static force_inline void do_call(PPUThread& CPU, func_t func)
		{
			call<T...>(CPU, func, arg_info_pack_t<>{});
		}
	};

	template<>
	struct func_binder<void> // redundant specialization to bypass internal compiler error in MSVC
	{
		using func_t = void(*)();

		static force_inline void do_call(PPUThread& CPU, func_t func)
		{
			func();
		}
	};

	template<typename RT>
	struct func_binder<RT> // redundant specialization to bypass internal compiler error in MSVC
	{
		using func_t = RT(*)();

		static force_inline void do_call(PPUThread& CPU, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(CPU, func());
		}
	};

	template<typename RT, typename... T>
	struct func_binder<RT, PPUThread&, T...>
	{
		using func_t = RT(*)(PPUThread&, T...);

		static force_inline void do_call(PPUThread& CPU, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(CPU, call<T...>(CPU, func, arg_info_pack_t<>{}));
		}
	};

	template<typename RT, typename... T>
	struct func_binder
	{
		using func_t = RT(*)(T...);

		static force_inline void do_call(PPUThread& CPU, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(CPU, call<T...>(CPU, func, arg_info_pack_t<>{}));
		}
	};
}

template<typename RT, typename... T> force_inline void call_ppu_func(PPUThread& CPU, RT(*func)(T...))
{
	ppu_func_detail::func_binder<RT, T...>::do_call(CPU, func);
}

#define bind_func(func) [](PPUThread& CPU){ call_ppu_func(CPU, func); }
