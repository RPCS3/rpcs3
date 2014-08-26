#pragma once
#include "Emu/Cell/PPUThread.h"

class func_caller
{
public:
	virtual void operator()() = 0;
};

namespace detail
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
		static_assert(sizeof(T) <= 8, "Wrong argument type for ARG_GENERAL");

		static __forceinline T func(PPUThread& CPU)
		{
			return (T&)CPU.GPR[g_count + 2];
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Wrong argument type for ARG_FLOAT");

		static __forceinline T func(PPUThread& CPU)
		{
			return (T&)CPU.FPR[f_count];
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) == 16, "Wrong argument type for ARG_VECTOR");

		static __forceinline T func(PPUThread& CPU)
		{
			return (T&)CPU.VPR[v_count + 1]._u128;
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8 && v_count <= 12, "Wrong argument type for ARG_STACK");

		static __forceinline T func(PPUThread& CPU)
		{
			const u64 res = CPU.GetStackArg(8 + std::max(g_count - 8, 0) + std::max(f_count - 12, 0));
			return (T&)res;
		}
	};

	template<typename T>
	struct bind_result
	{
		static __forceinline void func(PPUThread& CPU, T value)
		{
			static_assert(!std::is_pointer<T>::value, "Invalid function result type: pointer");
			if (std::is_floating_point<T>::value)
			{
				(T&)CPU.FPR[1] = value;
			}
			else
			{
				(T&)CPU.GPR[3] = value;
			}
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
	static __forceinline RT call(F f, Tuple && t)
	{
		typedef typename std::decay<Tuple>::type ttype;
		return detail::call_impl<RT, F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
	}

	template<int g_count, int f_count, int v_count>
	static __forceinline std::tuple<> iterate(PPUThread& CPU)
	{
		return std::tuple<>();
	}

	template<int g_count, int f_count, int v_count, typename T, typename... A>
	static __forceinline std::tuple<T, A...> iterate(PPUThread& CPU)
	{
		static_assert(!std::is_pointer<T>::value, "Invalid function argument type: pointer");
		// TODO: check calculations
		const bool is_float = std::is_floating_point<T>::value;
		const bind_arg_type t = is_float
			? ((f_count >= 12) ? ARG_STACK : ARG_FLOAT)
			: ((g_count >= 8) ? ARG_STACK : ARG_GENERAL);
		const int g = g_count + (is_float ? 0 : 1);
		const int f = f_count + (is_float ? 1 : 0);
		const int v = v_count; // TODO: vector argument support (if possible)
		return std::tuple_cat(std::tuple<T>(bind_arg<T, t, g, f, v>::func(CPU)), iterate<g, f, v, A...>(CPU));
	}

	template<typename RT, typename... TA>
	class func_binder;

	template<typename... TA>
	class func_binder<void, TA...> : public func_caller
	{
		typedef void(*func_t)(TA...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: func_caller()
			, m_call(call)
		{
		}

		virtual void operator()()
		{
			declCPU();
			call<void>(m_call, iterate<0, 0, 0, TA...>(CPU));
		}
	};

	template<typename TR, typename... TA>
	class func_binder : public func_caller
	{
		typedef TR(*func_t)(TA...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: func_caller()
			, m_call(call)
		{
		}

		virtual void operator()()
		{
			declCPU();
			bind_result<TR>::func(CPU, call<TR>(m_call, iterate<0, 0, 0, TA...>(CPU)));
		}
	};
}

template<typename TR, typename... TA>
func_caller* bind_func(TR(*call)(TA...))
{
	return new detail::func_binder<TR, TA...>(call);
}