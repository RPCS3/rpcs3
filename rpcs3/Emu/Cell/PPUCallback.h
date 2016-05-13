#pragma once

#include "Emu/Cell/PPUThread.h"

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
		static_assert(!std::is_pointer<T>::value, "Invalid callback argument type (pointer)");
		static_assert(!std::is_reference<T>::value, "Invalid callback argument type (reference)");
		static_assert(sizeof(T) <= 8, "Invalid callback argument type for ARG_GENERAL");
		
		static inline void set_value(PPUThread& CPU, const T& arg)
		{
			CPU.GPR[g_count + 2] = ppu_gpr_cast(arg);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid callback argument type for ARG_FLOAT");

		static inline void set_value(PPUThread& CPU, const T& arg)
		{
			CPU.FPR[f_count] = static_cast<T>(arg);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(std::is_same<CV T, CV v128>::value, "Invalid callback argument type for ARG_VECTOR");

		static inline void set_value(PPUThread& CPU, const T& arg)
		{
			CPU.VR[v_count + 1] = arg;
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(alignof(T) <= 16, "Unsupported callback argument type alignment for ARG_STACK");

		static inline void set_value(PPUThread& CPU, const T& arg)
		{
			const s64 stack_pos = (g_count - 1) * 0x8 + 0x30 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < 0, "TODO: Increase FIXED_STACK_FRAME_SIZE (arg count limit broken)");
			vm::ps3::write64(CPU.GPR[1] + stack_pos, ppu_gpr_cast(arg)); // TODO
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct _func_arg<T, ARG_CONTEXT, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, PPUThread&>::value, "Invalid callback argument type for ARG_CONTEXT");

		force_inline static void set_value(PPUThread& CPU, const T& arg)
		{
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	force_inline static bool _bind_func_args(PPUThread& CPU)
	{
		// terminator
		return false;
	}

	template<u32 g_count, u32 f_count, u32 v_count, typename T1, typename... T>
	force_inline static bool _bind_func_args(PPUThread& CPU, T1 arg1, T... args)
	{
		const bool is_float = std::is_floating_point<T1>::value;
		const bool is_vector = std::is_same<CV T1, CV v128>::value;
		const bool is_context = std::is_same<T1, PPUThread&>::value;
		const bool is_general = !is_float && !is_vector && !is_context;

		const _func_arg_type t =
			is_general ? (g_count >= 8 ? ARG_STACK : ARG_GENERAL) :
			is_float ? (f_count >= 13 ? ARG_STACK : ARG_FLOAT) :
			is_vector ? (v_count >= 12 ? ARG_STACK : ARG_VECTOR) :
			is_context ? ARG_CONTEXT :
			ARG_UNKNOWN;

		const u32 g = g_count + (is_general || is_float ? 1 : is_vector ? ::align(g_count, 2) + 2 : 0);
		const u32 f = f_count + is_float;
		const u32 v = v_count + is_vector;
		
		_func_arg<T1, t, g, f, v>::set_value(CPU, arg1);

		// return true if stack was used
		return _bind_func_args<g, f, v>(CPU, args...) || (t == ARG_STACK);
	}

	template<typename T, _func_arg_type type>
	struct _func_res
	{
		static_assert(type == ARG_GENERAL, "Unknown callback result type");
		static_assert(sizeof(T) <= 8, "Invalid callback result type for ARG_GENERAL");

		force_inline static T get_value(const PPUThread& CPU)
		{
			return ppu_gpr_cast<T>(CPU.GPR[3]);
		}
	};

	template<typename T>
	struct _func_res<T, ARG_FLOAT>
	{
		static_assert(sizeof(T) <= 8, "Invalid callback result type for ARG_FLOAT");

		force_inline static T get_value(const PPUThread& CPU)
		{
			return static_cast<T>(CPU.FPR[1]);
		}
	};

	template<typename T>
	struct _func_res<T, ARG_VECTOR>
	{
		static_assert(std::is_same<CV T, CV v128>::value, "Invalid callback result type for ARG_VECTOR");

		force_inline static T get_value(const PPUThread& CPU)
		{
			return CPU.VR[2];
		}
	};

	template<typename RT, typename... T>
	struct _func_caller
	{
		force_inline static RT call(PPUThread& CPU, u32 pc, u32 rtoc, T... args)
		{
			_func_caller<void, T...>::call(CPU, pc, rtoc, args...);

			static_assert(!std::is_pointer<RT>::value, "Invalid callback result type (pointer)");
			static_assert(!std::is_reference<RT>::value, "Invalid callback result type (reference)");
			const bool is_float = std::is_floating_point<RT>::value;
			const bool is_vector = std::is_same<std::remove_cv_t<RT>, v128>::value;
			const _func_arg_type t = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);

			return _func_res<RT, t>::get_value(CPU);
		}
	};

	template<typename... T>
	struct _func_caller<void, T...>
	{
		force_inline static void call(PPUThread& CPU, u32 pc, u32 rtoc, T... args)
		{
			const bool stack = _bind_func_args<0, 0, 0, T...>(CPU, args...);
			CPU.GPR[1] -= stack ? FIXED_STACK_FRAME_SIZE : 0x30; // create reserved area
			CPU.fast_call(pc, rtoc);
			CPU.GPR[1] += stack ? FIXED_STACK_FRAME_SIZE : 0x30;
		}
	};
}

namespace vm
{
	template<typename AT, typename RT, typename... T>
	force_inline RT _ptr_base<RT(T...), AT>::operator()(PPUThread& CPU, T... args) const
	{
		const auto data = vm::ps3::_ptr<u32>(vm::cast(m_addr, HERE));
		const u32 pc = data[0];
		const u32 rtoc = data[1];

		return ppu_cb_detail::_func_caller<RT, T...>::call(CPU, pc, rtoc, args...);
	}
}

template<typename RT, typename... T> inline RT cb_call(PPUThread& CPU, u32 pc, u32 rtoc, T... args)
{
	return ppu_cb_detail::_func_caller<RT, T...>::call(CPU, pc, rtoc, args...);
}

#include <queue>
#include <mutex>

class CallbackManager
{
	using check_cb_t = std::function<s32(PPUThread&)>;
	using async_cb_t = std::function<void(PPUThread&)>;

	std::mutex m_mutex;

	std::queue<check_cb_t> m_check_cb;
	std::queue<async_cb_t> m_async_cb;

	std::shared_ptr<PPUThread> m_cb_thread;

public:
	// Register checked callback
	void Register(check_cb_t func);

	// Register async callback, called in callback thread
	void Async(async_cb_t func);

	// Get one registered callback
	check_cb_t Check();

	void Init();

	void Clear();
};
