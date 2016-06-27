#pragma once

#include "ARMv7Thread.h"

using arm_function_t = void(*)(ARMv7Thread&);

#define BIND_FUNC(func) static_cast<arm_function_t>([](ARMv7Thread& cpu){\
	const auto old_f = cpu.last_function;\
	cpu.last_function = #func;\
	arm_func_detail::do_call(cpu, func);\
	cpu.last_function = old_f;\
})

struct arm_va_args_t
{
	u32 count; // Number of 32-bit args passed
};

namespace arm_func_detail
{
	enum arg_class : u32
	{
		ARG_GENERAL,
		ARG_FLOAT,
		ARG_VECTOR,
		ARG_STACK,
		ARG_CONTEXT,
		ARG_VARIADIC,
		ARG_UNKNOWN,
	};

	static const auto FIXED_STACK_FRAME_SIZE = 0x80;

	template<typename T, arg_class type, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg
	{
		static_assert(type == ARG_GENERAL, "Unknown function argument type");
		static_assert(!std::is_pointer<T>::value, "Invalid function argument type (pointer)");
		static_assert(!std::is_reference<T>::value, "Invalid function argument type (reference)");
		static_assert(sizeof(T) <= 4, "Invalid function argument type for ARG_GENERAL");

		force_inline static T get_arg(ARMv7Thread& cpu)
		{
			return arm_gpr_cast<T>(cpu.GPR[g_count - 1]);
		}

		force_inline static void put_arg(ARMv7Thread& cpu, const T& arg)
		{
			cpu.GPR[g_count - 1] = arm_gpr_cast(arg);
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<u64, ARG_GENERAL, g_count, f_count, v_count>
	{
		// first u64 argument is passed in r0-r1, second one is passed in r2-r3 (if g_count = 3)
		static_assert(g_count == 2 || g_count == 4, "Wrong u64 argument position");

		force_inline static u64 get_arg(ARMv7Thread& cpu)
		{
			return cpu.GPR_D[(g_count - 1) >> 1];
		}

		force_inline static void put_arg(ARMv7Thread& cpu, u64 arg)
		{
			cpu.GPR_D[(g_count - 1) >> 1] = arg;
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<s64, ARG_GENERAL, g_count, f_count, v_count>
	{
		static_assert(g_count == 2 || g_count == 4, "Wrong s64 argument position");

		force_inline static s64 get_arg(ARMv7Thread& cpu)
		{
			return cpu.GPR_D[(g_count - 1) >> 1];
		}

		force_inline static void put_arg(ARMv7Thread& cpu, s64 arg)
		{
			cpu.GPR_D[(g_count - 1) >> 1] = arg;
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(f_count <= 0, "TODO: Unsupported argument type (float)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		force_inline static T get_arg(ARMv7Thread& cpu)
		{
		}

		force_inline static void put_arg(ARMv7Thread& cpu, const T& arg)
		{
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(v_count <= 0, "TODO: Unsupported argument type (vector)");
		static_assert(std::is_same<CV T, CV v128>::value, "Invalid function argument type for ARG_VECTOR");

		force_inline static T get_arg(ARMv7Thread& cpu)
		{
		}

		force_inline static void put_arg(ARMv7Thread& cpu, const T& arg)
		{
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 0, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 0, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 4, "Invalid function argument type for ARG_STACK");

		force_inline static T get_arg(ARMv7Thread& cpu)
		{
			// TODO: check
			return arm_gpr_cast<T, u32>(vm::psv::read32(cpu.SP + sizeof(u32) * (g_count - 5)));
		}

		force_inline static void put_arg(ARMv7Thread& cpu, const T& arg)
		{
			// TODO: check
			const int stack_pos = (g_count - 5) * 4 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < 0, "TODO: Increase fixed stack frame size (arg count limit broken)");

			vm::psv::write32(cpu.SP + stack_pos, arm_gpr_cast(arg));
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<u64, ARG_STACK, g_count, f_count, v_count>
	{
		force_inline static u64 get_arg(ARMv7Thread& cpu)
		{
			// TODO: check
			return vm::psv::read64(cpu.SP + sizeof(u32) * (g_count - 6));
		}

		force_inline static void put_arg(ARMv7Thread& cpu, u64 arg)
		{
			// TODO: check
			const int stack_pos = (g_count - 6) * 4 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < -4, "TODO: Increase fixed stack frame size (arg count limit broken)");

			vm::psv::write64(cpu.SP + stack_pos, arg);
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<s64, ARG_STACK, g_count, f_count, v_count>
	{
		force_inline static s64 get_arg(ARMv7Thread& cpu)
		{
			// TODO: check
			return vm::psv::read64(cpu.SP + sizeof(u32) * (g_count - 6));
		}

		force_inline static void put_arg(ARMv7Thread& cpu, s64 arg)
		{
			// TODO: check
			const int stack_pos = (g_count - 6) * 4 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < -4, "TODO: Increase fixed stack frame size (arg count limit broken)");

			vm::psv::write64(cpu.SP + stack_pos, arg);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_CONTEXT, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, ARMv7Thread&>::value, "Invalid function argument type for ARG_CONTEXT");

		force_inline static ARMv7Thread& get_arg(ARMv7Thread& cpu)
		{
			return cpu;
		}

		force_inline static void put_arg(ARMv7Thread& cpu, ARMv7Thread& arg)
		{
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VARIADIC, g_count, f_count, v_count>
	{
		static_assert(std::is_same<CV T, CV arm_va_args_t>::value, "Invalid function argument type for ARG_VARIADIC");

		force_inline static arm_va_args_t get_arg(ARMv7Thread& cpu)
		{
			return{ g_count };
		}
	};

	template<typename T, arg_class type>
	struct bind_result
	{
		static_assert(type != ARG_FLOAT, "TODO: Unsupported funcion result type (float)");
		static_assert(type != ARG_VECTOR, "TODO: Unsupported funcion result type (vector)");
		static_assert(type == ARG_GENERAL, "Wrong use of bind_result template");
		static_assert(sizeof(T) <= 4, "Invalid function result type for ARG_GENERAL");

		force_inline static T get_result(ARMv7Thread& cpu)
		{
			return arm_gpr_cast<T>(cpu.GPR[0]);
		}

		force_inline static void put_result(ARMv7Thread& cpu, const T& result)
		{
			cpu.GPR[0] = arm_gpr_cast(result);
		}
	};

	template<>
	struct bind_result<u64, ARG_GENERAL>
	{
		force_inline static u64 get_result(ARMv7Thread& cpu)
		{
			return cpu.GPR_D[0];
		}

		force_inline static void put_result(ARMv7Thread& cpu, u64 result)
		{
			cpu.GPR_D[0] = result;
		}
	};

	template<>
	struct bind_result<s64, ARG_GENERAL>
	{
		force_inline static s64 get_result(ARMv7Thread& cpu)
		{
			return cpu.GPR_D[0];
		}

		force_inline static void put_result(ARMv7Thread& cpu, s64 result)
		{
			cpu.GPR_D[0] = result;
		}
	};

	//template<typename T>
	//struct bind_result<T, ARG_FLOAT>
	//{
	//	static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_FLOAT");

	//	static force_inline void put_result(ARMv7Thread& cpu, const T& result)
	//	{
	//	}
	//};

	//template<typename T>
	//struct bind_result<T, ARG_VECTOR>
	//{
	//	static_assert(std::is_same<std::remove_cv_t<T>, v128>::value, "Invalid function result type for ARG_VECTOR");

	//	static force_inline void put_result(ARMv7Thread& cpu, const T& result)
	//	{
	//	}
	//};

	template<typename RT>
	struct result_type
	{
		static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
		static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
		static const bool is_float = std::is_floating_point<RT>::value;
		static const bool is_vector = std::is_same<CV RT, CV v128>::value;
		static const arg_class value = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct arg_type
	{
		// TODO: check calculations
		static const bool is_float = std::is_floating_point<T>::value;
		static const bool is_vector = std::is_same<CV T, CV v128>::value;
		static const bool is_context = std::is_same<T, ARMv7Thread&>::value;
		static const bool is_variadic = std::is_same<CV T, CV arm_va_args_t>::value;
		static const bool is_general = !is_float && !is_vector && !is_context && !is_variadic;

		static const u32 g_align = ALIGN_32(T) > 4 ? ALIGN_32(T) >> 2 : 1;
		static const u32 g_value = is_general ? ((g_count + (g_align - 1)) & ~(g_align - 1)) + (g_align) : g_count;
		static const u32 f_value = f_count + is_float;
		static const u32 v_value = v_count + is_vector;

		static const arg_class value =
			is_general ? (g_value > 4 ? ARG_STACK : ARG_GENERAL) :
			is_float ? (f_value > 9000 ? ARG_STACK : ARG_FLOAT) :
			is_vector ? (v_value > 9000 ? ARG_STACK : ARG_VECTOR) :
			is_context ? ARG_CONTEXT :
			is_variadic ? ARG_VARIADIC :
			ARG_UNKNOWN;
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
		force_inline static T get_arg(ARMv7Thread& cpu)
		{
			return bind_arg<T, static_cast<arg_class>(type_pack & 0xff), (type_pack >> 8) & 0xff, (type_pack >> 16) & 0xff, (type_pack >> 24)>::get_arg(cpu);
		}
	};

	template<u32... Info, typename RT, typename... Args>
	force_inline RT call(ARMv7Thread& cpu, RT(*func)(Args...), arg_info_pack_t<Info...> info)
	{
		// do the actual function call when all arguments are prepared (simultaneous unpacking of Args... and Info...)
		return func(bind_arg_packed<Args, Info>::get_arg(cpu)...);
	}

	template<typename T, typename... Types, u32... Info, typename RT, typename... Args>
	force_inline RT call(ARMv7Thread& cpu, RT(*func)(Args...), arg_info_pack_t<Info...> info)
	{
		// unpack previous type counts (0/0/0 for the first time)
		const u32 g_count = (info.last_value >> 8) & 0xff;
		const u32 f_count = (info.last_value >> 16) & 0xff;
		const u32 v_count = (info.last_value >> 24);

		using type = arg_type<T, g_count, f_count, v_count>;
		const arg_class t = type::value;
		const u32 g = type::g_value;
		const u32 f = type::f_value;
		const u32 v = type::v_value;

		return call<Types...>(cpu, func, arg_info_pack_t<Info..., t | (g << 8) | (f << 16) | (v << 24)>{});
	}

	template<u32 g_count, u32 f_count, u32 v_count>
	force_inline static bool put_func_args(ARMv7Thread& cpu)
	{
		// terminator
		return false;
	}

	template<u32 g_count, u32 f_count, u32 v_count, typename T1, typename... T>
	force_inline static bool put_func_args(ARMv7Thread& cpu, T1 arg, T... args)
	{
		using type = arg_type<T1, g_count, f_count, v_count>;
		const arg_class t = type::value;
		const u32 g = type::g_value;
		const u32 f = type::f_value;
		const u32 v = type::v_value;

		bind_arg<T1, t, g, f, v>::put_arg(cpu, arg);

		// return true if stack was used
		return put_func_args<g, f, v>(cpu, args...) || (t == ARG_STACK);
	}

	template<typename RT, typename... T>
	struct func_binder;

	template<typename... T>
	struct func_binder<void, T...>
	{
		using func_t = void(*)(T...);

		static void do_call(ARMv7Thread& cpu, func_t func)
		{
			call<T...>(cpu, func, arg_info_pack_t<>{});
		}
	};

	template<typename RT, typename... T>
	struct func_binder
	{
		using func_t =  RT(*)(T...);

		static void do_call(ARMv7Thread& cpu, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(cpu, call<T...>(cpu, func, arg_info_pack_t<>{}));
		}
	};

	template<typename RT, typename... T>
	struct func_caller
	{
		force_inline static RT call(ARMv7Thread& cpu, u32 addr, T... args)
		{
			func_caller<void, T...>::call(cpu, addr, args...);

			return bind_result<RT, result_type<RT>::value>::get_result(cpu);
		}
	};

	template<typename... T>
	struct func_caller<void, T...>
	{
		force_inline static void call(ARMv7Thread& cpu, u32 addr, T... args)
		{
			if (put_func_args<0, 0, 0, T...>(cpu, args...))
			{
				cpu.SP -= FIXED_STACK_FRAME_SIZE;
				cpu.fast_call(addr);
				cpu.SP += FIXED_STACK_FRAME_SIZE;
			}
			else
			{
				cpu.fast_call(addr);
			}
		}
	};

	template<typename RT, typename... T> force_inline void do_call(ARMv7Thread& cpu, RT(*func)(T...))
	{
		func_binder<RT, T...>::do_call(cpu, func);
	}
}

class arm_function_manager
{
	// Global variable for each registered function
	template<typename T, T Func>
	struct registered
	{
		static u32 index;
	};

	// Access global function list
	static std::vector<arm_function_t>& access();

	static u32 add_function(arm_function_t function);

public:
	// Register function (shall only be called during global initialization)
	template<typename T, T Func>
	static inline u32 register_function(arm_function_t func)
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
u32 arm_function_manager::registered<T, Func>::index = 0;

#define FIND_FUNC(func) arm_function_manager::get_index<decltype(&func), &func>()
