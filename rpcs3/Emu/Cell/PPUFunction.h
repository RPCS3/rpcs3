#pragma once

#include "PPUThread.h"
#include "PPUInterpreter.h"

#include "util/v128.hpp"

// BIND_FUNC macro "converts" any appropriate HLE function to ppu_intrp_func_t, binding it to PPU thread context.
#define BIND_FUNC(func, ...) (static_cast<ppu_intrp_func_t>([](ppu_thread& ppu, ppu_opcode_t, be_t<u32>* this_op, ppu_intrp_func*) {\
	const auto old_f = ppu.current_function;\
	if (!old_f) ppu.last_function = #func;\
	ppu.current_function = #func;\
	ppu.cia = vm::get_addr(this_op); \
	std::memcpy(ppu.syscall_args, ppu.gpr + 3, sizeof(ppu.syscall_args)); \
	ppu_func_detail::do_call(ppu, func);\
	static_cast<void>(ppu.test_stopped());\
	auto& history = ppu.syscall_history.data[ppu.syscall_history.index++ % ppu.syscall_history.data.size()];\
	history.cia = ppu.cia;\
	history.func_name = ppu.current_function;\
	history.error = ppu.gpr[3];\
	if (ppu.syscall_history.count_debug_arguments) std::copy_n(ppu.syscall_args, std::size(history.args), history.args.data());\
	ppu.current_function = old_f;\
	ppu.cia += 4;\
	__VA_ARGS__;\
}))

struct ppu_va_args_t
{
	u32 count; // Number of 64-bit args passed
};

namespace ppu_func_detail
{
	// argument type classification
	enum arg_class : u32
	{
		ARG_GENERAL, // argument stored in gpr (from r3 to r10)
		ARG_FLOAT, // argument stored in fpr (from f1 to f13)
		ARG_VECTOR, // argument stored in vr (from v2 to v13)
		ARG_STACK, // argument stored on the stack
		ARG_CONTEXT, // ppu_thread& passed, doesn't affect g/f/v_count
		ARG_VARIADIC, // argument count at specific position, doesn't affect g/f/v_count
		ARG_UNKNOWN,
	};

	template<typename T, arg_class type, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg
	{
		static_assert(type == ARG_GENERAL, "Unknown function argument type");
		static_assert(!std::is_pointer_v<T>, "Invalid function argument type (pointer)");
		static_assert(!std::is_reference_v<T>, "Invalid function argument type (reference)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_GENERAL");

		static inline T get_arg(ppu_thread& ppu)
		{
			return ppu_gpr_cast<T>(ppu.gpr[g_count + 2]);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		static inline T get_arg(ppu_thread& ppu)
		{
			return static_cast<T>(ppu.fpr[f_count]);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(std::is_same_v<std::decay_t<T>, v128>, "Invalid function argument type for ARG_VECTOR");

		static FORCE_INLINE T get_arg(ppu_thread& ppu)
		{
			return ppu.vr[v_count + 1];
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(alignof(T) <= 16, "Unsupported type alignment for ARG_STACK");

		static FORCE_INLINE T get_arg(ppu_thread& ppu)
		{
			return ppu_gpr_cast<T, u64>(*ppu.get_stack_arg(g_count, alignof(T))); // TODO
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_CONTEXT, g_count, f_count, v_count>
	{
		static_assert(std::is_base_of_v<std::decay_t<T>, ppu_thread>, "Invalid function argument type for ARG_CONTEXT");

		static FORCE_INLINE T& get_arg(ppu_thread& ppu)
		{
			return ppu;
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VARIADIC, g_count, f_count, v_count>
	{
		static_assert(std::is_same_v<std::decay_t<T>, ppu_va_args_t>, "Invalid function argument type for ARG_VARIADIC");

		static FORCE_INLINE ppu_va_args_t get_arg(ppu_thread&)
		{
			return {g_count};
		}
	};

	template<typename T, arg_class type>
	struct bind_result
	{
		static_assert(type == ARG_GENERAL, "Unknown function result type");
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_GENERAL");

		static FORCE_INLINE void put_result(ppu_thread& ppu, const T& result)
		{
			if (ppu.state & cpu_flag::again) return;
			ppu.gpr[3] = ppu_gpr_cast(result);
		}
	};

	template<typename T>
	struct bind_result<T, ARG_FLOAT>
	{
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_FLOAT");

		static FORCE_INLINE void put_result(ppu_thread& ppu, const T& result)
		{
			if (ppu.state & cpu_flag::again) return;
			ppu.fpr[1] = static_cast<T>(result);
		}
	};

	template<typename T>
	struct bind_result<T, ARG_VECTOR>
	{
		static_assert(std::is_same_v<std::decay_t<T>, v128>, "Invalid function result type for ARG_VECTOR");

		static FORCE_INLINE void put_result(ppu_thread& ppu, const T& result)
		{
			if (ppu.state & cpu_flag::again) return;
			ppu.vr[2] = result;
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
		static FORCE_INLINE T get_arg(ppu_thread& ppu)
		{
			return bind_arg<T, static_cast<arg_class>(type_pack & 0xff), (type_pack >> 8) & 0xff, (type_pack >> 16) & 0xff, (type_pack >> 24)>::get_arg(ppu);
		}
	};

	template<u32... Info, typename RT, typename... Args>
	FORCE_INLINE RT call(ppu_thread& ppu, RT(*func)(Args...), arg_info_pack_t<Info...>)
	{
		// do the actual function call when all arguments are prepared (simultaneous unpacking of Args... and Info...)
		return func(bind_arg_packed<Args, Info>::get_arg(ppu)...);
	}

	template<typename T, typename... Types, u32... Info, typename RT, typename... Args>
	FORCE_INLINE RT call(ppu_thread& ppu, RT(*func)(Args...), arg_info_pack_t<Info...> info)
	{
		// unpack previous type counts (0/0/0 for the first time)
		const u32 g_count = (info.last_value >> 8) & 0xff;
		const u32 f_count = (info.last_value >> 16) & 0xff;
		const u32 v_count = (info.last_value >> 24);

		// TODO: check calculations
		constexpr bool is_float = std::is_floating_point_v<T>;
		constexpr bool is_vector = std::is_same_v<std::decay_t<T>, v128>;
		constexpr bool is_context = std::is_base_of_v<std::decay_t<T>, ppu_thread>;
		constexpr bool is_variadic = std::is_same_v<std::decay_t<T>, ppu_va_args_t>;
		constexpr bool is_general = !is_float && !is_vector && !is_context && !is_variadic;

		constexpr arg_class t =
			is_general ? (g_count >= 8 ? ARG_STACK : ARG_GENERAL) :
			is_float ? (f_count >= 13 ? ARG_STACK : ARG_FLOAT) :
			is_vector ? (v_count >= 12 ? ARG_STACK : ARG_VECTOR) :
			is_context ? ARG_CONTEXT :
			is_variadic ? ARG_VARIADIC :
			ARG_UNKNOWN;

		constexpr u32 g = g_count + (is_general || is_float ? 1 : is_vector ? (g_count & 1) + 2 : 0);
		constexpr u32 f = f_count + is_float;
		constexpr u32 v = v_count + is_vector;

		return call<Types...>(ppu, func, arg_info_pack_t<Info..., t | (g << 8) | (f << 16) | (v << 24)>{});
	}

	template<typename RT>
	struct result_type
	{
		static_assert(!std::is_pointer_v<RT>, "Invalid function result type (pointer)");
		static_assert(!std::is_reference_v<RT>, "Invalid function result type (reference)");
		static constexpr bool is_float = std::is_floating_point_v<RT>;
		static constexpr bool is_vector = std::is_same_v<std::decay_t<RT>, v128>;
		static constexpr arg_class value = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);
	};

	template<typename RT, typename... T> struct func_binder;

	template<typename... T>
	struct func_binder<void, T...>
	{
		using func_t = void(*)(T...);

		static FORCE_INLINE void do_call(ppu_thread& ppu, func_t func)
		{
			call<T...>(ppu, func, arg_info_pack_t<>{});
		}
	};

	template<typename RT, typename... T>
	struct func_binder
	{
		using func_t = RT(*)(T...);

		static FORCE_INLINE void do_call(ppu_thread& ppu, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(ppu, call<T...>(ppu, func, arg_info_pack_t<>{}));
		}
	};

	template<typename RT, typename... T>
	FORCE_INLINE void do_call(ppu_thread& ppu, RT(*func)(T...))
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
	static std::vector<ppu_intrp_func_t>& access(bool ghc = false);

	static u32 add_function(ppu_intrp_func_t function);

public:
	ppu_function_manager() = default;

	ppu_function_manager(const ppu_function_manager&) = delete;

	ppu_function_manager& operator=(const ppu_function_manager&) = delete;

	// Register function (shall only be called during global initialization)
	template<typename T, T Func>
	static inline u32 register_function(ppu_intrp_func_t func)
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
	static inline const auto& get(bool llvm = false)
	{
		return access(llvm);
	}

	u32 func_addr(u32 index, bool is_code_addr = false) const
	{
		if (index >= access().size() || !addr)
		{
			return 0;
		}

		return addr + index * 8 + (is_code_addr ? 4 : 0);
	}

	bool is_func(u32 cia) const
	{
		if (cia % 4 || !addr || cia < addr)
		{
			return false;
		}

		return (cia - addr) / 8 < access().size();
	}

	// Allocation address
	u32 addr = 0;

	void save(utils::serial& ar);
	ppu_function_manager(utils::serial& ar);
};

template<typename T, T Func>
u32 ppu_function_manager::registered<T, Func>::index = 0;

#define FIND_FUNC(func) ppu_function_manager::get_index<decltype(&func), &func>()
