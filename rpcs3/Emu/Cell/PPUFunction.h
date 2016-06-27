#pragma once

#include "PPUThread.h"

using ppu_function_t = void(*)(PPUThread&);

#define BIND_FUNC(func) static_cast<ppu_function_t>([](PPUThread& ppu){\
	const auto old_f = ppu.last_function;\
	ppu.last_function = #func;\
	ppu_func_detail::do_call(ppu, func);\
	ppu.last_function = old_f;\
})

struct ppu_va_args_t
{
	u32 count; // Number of 64-bit args passed
};

namespace ppu_func_detail
{
	// argument type classification
	enum arg_class : u32
	{
		ARG_GENERAL, // argument stored in GPR (from r3 to r10)
		ARG_FLOAT, // argument stored in FPR (from f1 to f13)
		ARG_VECTOR, // argument stored in VR (from v2 to v13)
		ARG_STACK, // argument stored on the stack
		ARG_CONTEXT, // PPUThread& passed, doesn't affect g/f/v_count
		ARG_VARIADIC, // argument count at specific position, doesn't affect g/f/v_count
		ARG_UNKNOWN,
	};

	template<typename T, arg_class type, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg
	{
		static_assert(type == ARG_GENERAL, "Unknown function argument type");
		static_assert(!std::is_pointer<T>::value, "Invalid function argument type (pointer)");
		static_assert(!std::is_reference<T>::value, "Invalid function argument type (reference)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_GENERAL");

		static inline T get_arg(PPUThread& ppu)
		{
			return ppu_gpr_cast<T>(ppu.GPR[g_count + 2]);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		static inline T get_arg(PPUThread& ppu)
		{
			return static_cast<T>(ppu.FPR[f_count]);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(std::is_same<CV T, CV v128>::value, "Invalid function argument type for ARG_VECTOR");

		static force_inline T get_arg(PPUThread& ppu)
		{
			return ppu.VR[v_count + 1];
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(alignof(T) <= 16, "Unsupported type alignment for ARG_STACK");

		static force_inline T get_arg(PPUThread& ppu)
		{
			return ppu_gpr_cast<T, u64>(*ppu.get_stack_arg(g_count, alignof(T))); // TODO
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_CONTEXT, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, PPUThread&>::value, "Invalid function argument type for ARG_CONTEXT");

		static force_inline PPUThread& get_arg(PPUThread& ppu)
		{
			return ppu;
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VARIADIC, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, ppu_va_args_t>::value, "Invalid function argument type for ARG_VARIADIC");

		static force_inline ppu_va_args_t get_arg(PPUThread& ppu)
		{
			return{ g_count };
		}
	};

	template<typename T, arg_class type>
	struct bind_result
	{
		static_assert(type == ARG_GENERAL, "Unknown function result type");
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_GENERAL");

		static force_inline void put_result(PPUThread& ppu, const T& result)
		{
			ppu.GPR[3] = ppu_gpr_cast(result);
		}
	};

	template<typename T>
	struct bind_result<T, ARG_FLOAT>
	{
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_FLOAT");

		static force_inline void put_result(PPUThread& ppu, const T& result)
		{
			ppu.FPR[1] = static_cast<T>(result);
		}
	};

	template<typename T>
	struct bind_result<T, ARG_VECTOR>
	{
		static_assert(std::is_same<CV T, CV v128>::value, "Invalid function result type for ARG_VECTOR");

		static force_inline void put_result(PPUThread& ppu, const T& result)
		{
			ppu.VR[2] = result;
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

	// argument type + g/f/v_count unpacker
	template<typename T, u32 type_pack> struct bind_arg_packed
	{
		static force_inline T get_arg(PPUThread& ppu)
		{
			return bind_arg<T, static_cast<arg_class>(type_pack & 0xff), (type_pack >> 8) & 0xff, (type_pack >> 16) & 0xff, (type_pack >> 24)>::get_arg(ppu);
		}
	};

	template<u32... Info, typename RT, typename... Args>
	force_inline RT call(PPUThread& ppu, RT(*func)(Args...), arg_info_pack_t<Info...>)
	{
		// do the actual function call when all arguments are prepared (simultaneous unpacking of Args... and Info...)
		return func(bind_arg_packed<Args, Info>::get_arg(ppu)...);
	}

	template<typename T, typename... Types, u32... Info, typename RT, typename... Args>
	force_inline RT call(PPUThread& ppu, RT(*func)(Args...), arg_info_pack_t<Info...> info)
	{
		// unpack previous type counts (0/0/0 for the first time)
		const u32 g_count = (info.last_value >> 8) & 0xff;
		const u32 f_count = (info.last_value >> 16) & 0xff;
		const u32 v_count = (info.last_value >> 24);

		// TODO: check calculations
		const bool is_float = std::is_floating_point<T>::value;
		const bool is_vector = std::is_same<CV T, CV v128>::value;
		const bool is_context = std::is_same<T, PPUThread&>::value;
		const bool is_variadic = std::is_same<CV T, CV ppu_va_args_t>::value;
		const bool is_general = !is_float && !is_vector && !is_context && !is_variadic;

		const arg_class t =
			is_general ? (g_count >= 8 ? ARG_STACK : ARG_GENERAL) :
			is_float ? (f_count >= 13 ? ARG_STACK : ARG_FLOAT) :
			is_vector ? (v_count >= 12 ? ARG_STACK : ARG_VECTOR) :
			is_context ? ARG_CONTEXT :
			is_variadic ? ARG_VARIADIC :
			ARG_UNKNOWN;

		const u32 g = g_count + (is_general || is_float ? 1 : is_vector ? ::align(g_count, 2) + 2 : 0);
		const u32 f = f_count + is_float;
		const u32 v = v_count + is_vector;

		return call<Types...>(ppu, func, arg_info_pack_t<Info..., t | (g << 8) | (f << 16) | (v << 24)>{});
	}

	template<typename RT>
	struct result_type
	{
		static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
		static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
		static const bool is_float = std::is_floating_point<RT>::value;
		static const bool is_vector = std::is_same<CV RT, CV v128>::value;
		static const arg_class value = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);
	};

	template<typename RT, typename... T> struct func_binder;

	template<typename... T>
	struct func_binder<void, T...>
	{
		using func_t = void(*)(T...);

		static force_inline void do_call(PPUThread& ppu, func_t func)
		{
			call<T...>(ppu, func, arg_info_pack_t<>{});
		}
	};

	template<typename RT, typename... T>
	struct func_binder
	{
		using func_t = RT(*)(T...);

		static force_inline void do_call(PPUThread& ppu, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(ppu, call<T...>(ppu, func, arg_info_pack_t<>{}));
		}
	};

	template<typename RT, typename... T>
	force_inline void do_call(PPUThread& ppu, RT(*func)(T...))
	{
		func_binder<RT, T...>::do_call(ppu, func);
	}
}

class ppu_function_manager
{
	// Global variable for each registered function
	template<typename T, T Func>
	struct registered
	{
		static u32 index;
	};

	// Access global function list
	static std::vector<ppu_function_t>& access();

	static u32 add_function(ppu_function_t function);

public:
	// Register function (shall only be called during global initialization)
	template<typename T, T Func>
	static inline u32 register_function(ppu_function_t func)
	{
		return registered<T, Func>::index = add_function(func);
	}

	// Get function index
	template<typename T, T Func>
	static inline u32 get_index()
	{
		return registered<T, Func>::index;
	}

	// Read all registered functions
	static inline const auto& get()
	{
		return access();
	}
};

template<typename T, T Func>
u32 ppu_function_manager::registered<T, Func>::index = 0;

#define FIND_FUNC(func) ppu_function_manager::get_index<decltype(&func), &func>()
