#pragma once

#include "Emu/Cell/PPUThread.h"

#include "util/v128.hpp"

struct ppu_func_opd_t;

namespace ppu_cb_detail
{
	enum _func_arg_type
	{
		ARG_GENERAL,
		ARG_FLOAT,
		ARG_VECTOR,
		ARG_STACK,
		ARG_CONTEXT,
		ARG_UNKNOWN,
	};

	// Current implementation can handle only fixed amount of stack arguments.
	// This constant can be increased if necessary.
	// It's possible to calculate suitable stack frame size in template, but too complicated.
	static const auto FIXED_STACK_FRAME_SIZE = 0x90;

	template<typename T, _func_arg_type type, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg
	{
		static_assert(type == ARG_GENERAL, "Unknown callback argument type");
		static_assert(!std::is_pointer_v<T>, "Invalid callback argument type (pointer)");
		static_assert(!std::is_reference_v<T>, "Invalid callback argument type (reference)");
		static_assert(sizeof(T) <= 8, "Invalid callback argument type for ARG_GENERAL");

		static inline void set_value(ppu_thread& CPU, const T& arg)
		{
			CPU.gpr[g_count + 2] = ppu_gpr_cast(arg);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid callback argument type for ARG_FLOAT");

		static inline void set_value(ppu_thread& CPU, const T& arg)
		{
			CPU.fpr[f_count] = static_cast<T>(arg);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(std::is_same_v<std::decay_t<T>, v128>, "Invalid callback argument type for ARG_VECTOR");

		static inline void set_value(ppu_thread& CPU, const T& arg)
		{
			CPU.vr[v_count + 1] = arg;
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(alignof(T) <= 16, "Unsupported callback argument type alignment for ARG_STACK");

		static inline void set_value(ppu_thread& CPU, const T& arg)
		{
			const s64 stack_pos = (static_cast<s64>(g_count) - 1) * 0x8 + 0x30 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < 0, "TODO: Increase FIXED_STACK_FRAME_SIZE (arg count limit broken)");
			vm::write64(static_cast<u32>(CPU.gpr[1] + stack_pos), ppu_gpr_cast(arg)); // TODO
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg<T, ARG_CONTEXT, g_count, f_count, v_count>
	{
		static_assert(std::is_same_v<std::decay_t<T>, ppu_thread>, "Invalid callback argument type for ARG_CONTEXT");

		FORCE_INLINE static void set_value(ppu_thread&, const T&)
		{
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	FORCE_INLINE static bool _bind_func_args(ppu_thread&)
	{
		// terminator
		return false;
	}

	template<u32 g_count, u32 f_count, u32 v_count, typename T1, typename... T>
	FORCE_INLINE static bool _bind_func_args(ppu_thread& CPU, T1 arg1, T... args)
	{
		constexpr bool is_float = std::is_floating_point_v<T1>;
		constexpr bool is_vector = std::is_same_v<std::decay_t<T1>, v128>;
		constexpr bool is_context = std::is_same_v<std::decay_t<T1>, ppu_thread>;
		constexpr bool is_general = !is_float && !is_vector && !is_context;

		constexpr _func_arg_type t =
			is_general ? (g_count >= 8 ? ARG_STACK : ARG_GENERAL) :
			is_float ? (f_count >= 13 ? ARG_STACK : ARG_FLOAT) :
			is_vector ? (v_count >= 12 ? ARG_STACK : ARG_VECTOR) :
			is_context ? ARG_CONTEXT :
			ARG_UNKNOWN;

		constexpr u32 g = g_count + (is_general || is_float ? 1 : is_vector ? (g_count & 1) + 2 : 0);
		constexpr u32 f = f_count + is_float;
		constexpr u32 v = v_count + is_vector;

		_func_arg<T1, t, g, f, v>::set_value(CPU, arg1);

		// return true if stack was used
		return _bind_func_args<g, f, v>(CPU, args...) || (t == ARG_STACK);
	}

	template<typename T, _func_arg_type type>
	struct _func_res
	{
		static_assert(type == ARG_GENERAL, "Unknown callback result type");
		static_assert(sizeof(T) <= 8, "Invalid callback result type for ARG_GENERAL");

		FORCE_INLINE static T get_value(const ppu_thread& CPU)
		{
			return ppu_gpr_cast<T>(CPU.gpr[3]);
		}
	};

	template<typename T>
	struct _func_res<T, ARG_FLOAT>
	{
		static_assert(sizeof(T) <= 8, "Invalid callback result type for ARG_FLOAT");

		FORCE_INLINE static T get_value(const ppu_thread& CPU)
		{
			return static_cast<T>(CPU.fpr[1]);
		}
	};

	template<typename T>
	struct _func_res<T, ARG_VECTOR>
	{
		static_assert(std::is_same_v<std::decay_t<T>, v128>, "Invalid callback result type for ARG_VECTOR");

		FORCE_INLINE static T get_value(const ppu_thread& CPU)
		{
			return CPU.vr[2];
		}
	};

	template<typename RT, typename... T>
	struct _func_caller
	{
		FORCE_INLINE static RT call(ppu_thread& CPU, u32 pc, u32 rtoc, T... args)
		{
			_func_caller<void, T...>::call(CPU, pc, rtoc, args...);

			static_assert(!std::is_pointer_v<RT>, "Invalid callback result type (pointer)");
			static_assert(!std::is_reference_v<RT>, "Invalid callback result type (reference)");
			constexpr bool is_float = std::is_floating_point_v<RT>;
			constexpr bool is_vector = std::is_same_v<std::decay_t<RT>, v128>;
			constexpr _func_arg_type t = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);

			return _func_res<RT, t>::get_value(CPU);
		}
	};

	template<typename... T>
	struct _func_caller<void, T...>
	{
		FORCE_INLINE static void call(ppu_thread& CPU, u32 pc, u32 rtoc, T... args)
		{
			const u64 old_r1 = CPU.gpr[1];
			CPU.gpr[1] &= -16; // Ensure 16-byte alignment
			const bool stack = _bind_func_args<0, 0, 0, T...>(CPU, args...);
			CPU.gpr[1] -= stack ? FIXED_STACK_FRAME_SIZE : 0x70; // create reserved area
			CPU.fast_call(pc, rtoc);
			CPU.gpr[1] = old_r1;
		}
	};
}

namespace vm
{
	template<typename AT, typename RT, typename... T>
	FORCE_INLINE RT _ptr_base<RT(T...), AT>::operator()(ppu_thread& CPU, T... args) const
	{
		const auto data = vm::_ptr<ppu_func_opd_t>(vm::cast(m_addr));
		const u32 pc = data->addr;
		const u32 rtoc = data->rtoc;

		return ppu_cb_detail::_func_caller<RT, T...>::call(CPU, pc, rtoc, args...);
	}

	template<typename AT, typename RT, typename... T>
	FORCE_INLINE const ppu_func_opd_t& _ptr_base<RT(T...), AT>::opd() const
	{
		return vm::_ref<ppu_func_opd_t>(vm::cast(m_addr));
	}
}

template<typename RT, typename... T> inline RT cb_call(ppu_thread& CPU, u32 pc, u32 rtoc, T... args)
{
	return ppu_cb_detail::_func_caller<RT, T...>::call(CPU, pc, rtoc, args...);
}
