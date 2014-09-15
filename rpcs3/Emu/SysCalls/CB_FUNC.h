#pragma once
#include "Emu/Cell/PPUThread.h"

namespace cb_detail
{
	enum _func_arg_type
	{
		ARG_GENERAL,
		ARG_FLOAT,
		ARG_VECTOR,
		ARG_STACK,
	};

	// Current implementation can handle only fixed amount of stack arguments.
	// This constant can be increased if necessary.
	// It's possible to calculate suitable stack frame size in template, but too complicated.
	static const auto FIXED_STACK_FRAME_SIZE = 0x100;

	template<typename T, _func_arg_type type, int g_count, int f_count, int v_count>
	struct _func_arg;

	template<typename T, int g_count, int f_count, int v_count>
	struct _func_arg<T, ARG_GENERAL, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid callback argument type for ARG_GENERAL");
		
		__forceinline static void set_value(PPUThread& CPU, const T arg)
		{
			(T&)CPU.GPR[g_count + 2] = arg;
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct _func_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid callback argument type for ARG_FLOAT");

		__forceinline static void set_value(PPUThread& CPU, const T arg)
		{
			CPU.FPR[f_count] = arg;
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct _func_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, u128>::value, "Invalid callback argument type for ARG_VECTOR");

		__forceinline static void set_value(PPUThread& CPU, const T arg)
		{
			(T&)CPU.VPR[v_count + 1] = arg;
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct _func_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 12, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 12, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 8, "Invalid callback argument type for ARG_STACK");

		__forceinline static void set_value(PPUThread& CPU, const T arg)
		{
			const int stack_pos = 0x70 + (g_count - 9) * 8 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < 0, "TODO: Increase fixed stack frame size (arg count limit broken)");
			u64 value = 0;
			(T&)value = arg;
			vm::write64(CPU.GPR[1] + stack_pos, value);
		}
	};

	template<int g_count, int f_count, int v_count>
	__forceinline static bool _bind_func_args(PPUThread& CPU)
	{
		// terminator
		return false;
	}

	template<int g_count, int f_count, int v_count, typename T1, typename... T>
	__forceinline static bool _bind_func_args(PPUThread& CPU, T1 arg1, T... args)
	{
		static_assert(!std::is_pointer<T1>::value, "Invalid callback argument type (pointer)");
		static_assert(!std::is_reference<T1>::value, "Invalid callback argument type (reference)");
		const bool is_float = std::is_floating_point<T1>::value;
		const bool is_vector = std::is_same<T1, u128>::value;
		const _func_arg_type t = is_float
			? ((f_count >= 12) ? ARG_STACK : ARG_FLOAT)
			: (is_vector ? ((v_count >= 12) ? ARG_STACK : ARG_VECTOR) : ((g_count >= 8) ? ARG_STACK : ARG_GENERAL));
		const int g = g_count + (is_float || is_vector ? 0 : 1);
		const int f = f_count + (is_float ? 1 : 0);
		const int v = v_count + (is_vector ? 1 : 0);
		
		_func_arg<T1, t, g, f, v>::set_value(CPU, arg1);
		// return true if stack was used
		return _bind_func_args<g, f, v>(CPU, args...) || (t == ARG_STACK);
	}

	template<typename RT>
	struct _func_res
	{
		static_assert(sizeof(RT) <= 8, "Invalid callback result type");
		static_assert(!std::is_pointer<RT>::value, "Invalid callback result type (pointer)");
		static_assert(!std::is_reference<RT>::value, "Invalid callback result type (reference)");

		__forceinline static RT get_value(const PPUThread& CPU)
		{
			if (std::is_floating_point<RT>::value)
			{
				return (RT)CPU.FPR[1];
			}
			else
			{
				return (RT&)CPU.GPR[3];
			}
		}
	};

	template<>
	struct _func_res<u128>
	{
		__forceinline static const u128 get_value(const PPUThread& CPU)
		{
			return CPU.VPR[2];
		}
	};

	template<>
	struct _func_res<const u128>
	{
		__forceinline static const u128 get_value(const PPUThread& CPU)
		{
			return CPU.VPR[2];
		}
	};

	template<>
	struct _func_res<void>
	{
		__forceinline static void get_value(const PPUThread& CPU)
		{
		}
	};

	template<typename RT, typename... T>
	struct _func_caller
	{
		__forceinline static RT call(PPUThread& CPU, u32 pc, u32 rtoc, T... args)
		{
			const bool stack = _bind_func_args<0, 0, 0>(CPU, args...);
			if (stack) CPU.GPR[1] -= FIXED_STACK_FRAME_SIZE;
			CPU.FastCall2(pc, rtoc);
			if (stack) CPU.GPR[1] += FIXED_STACK_FRAME_SIZE;
			return _func_res<RT>::get_value(CPU);
		}
	};
}

namespace vm
{
	template<typename AT, typename RT, typename... T>
	__forceinline RT _ptr_base<RT(*)(T...), 1, AT>::call(PPUThread& CPU, T... args) const
	{
		const u32 pc = vm::get_ref<be_t<u32>>(m_addr);
		const u32 rtoc = vm::get_ref<be_t<u32>>(m_addr + 4);

		return cb_detail::_func_caller<RT, T...>::call(CPU, pc, rtoc, args...);
	}

	template<typename AT, typename RT, typename... T>
	__forceinline RT _ptr_base<RT(*)(T...), 1, AT>::operator ()(T... args) const
	{
		return call(GetCurrentPPUThread(), args...);
	}
}

template<typename RT, typename... T>
struct cb_caller : public cb_detail::_func_caller<RT, T...>
{
};
