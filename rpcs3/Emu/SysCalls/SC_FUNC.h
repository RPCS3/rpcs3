#pragma once
#include "Emu/Cell/PPUThread.h"

class func_caller
{
public:
	virtual void operator()(PPUThread& CPU) = 0;
	virtual ~func_caller(){};
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
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_GENERAL");

		static __forceinline T func(PPUThread& CPU)
		{
			return (T&)CPU.GPR[g_count + 2];
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		static __forceinline T func(PPUThread& CPU)
		{
			return (T)CPU.FPR[f_count];
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(sizeof(T) == 16, "Invalid function argument type for ARG_VECTOR");

		static __forceinline T func(PPUThread& CPU)
		{
			return (T&)CPU.VPR[v_count + 1];
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 12, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 12, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_STACK");

		static __forceinline T func(PPUThread& CPU)
		{
			// TODO: check stack argument displacement
			const u64 res = CPU.GetStackArg(8 + std::max(g_count - 8, 0) + std::max(f_count - 12, 0) + std::max(v_count - 12, 0));
			return (T&)res;
		}
	};

	template<typename T, bind_arg_type type>
	struct bind_result
	{
		static_assert(type == ARG_GENERAL, "Wrong use of bind_result template");
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_GENERAL");

		static __forceinline void func(PPUThread& CPU, T result)
		{
			(T&)CPU.GPR[3] = result;
		}
	};

	template<typename T>
	struct bind_result<T, ARG_FLOAT>
	{
		static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_FLOAT");

		static __forceinline void func(PPUThread& CPU, T result)
		{
			CPU.FPR[1] = (double)result;
		}
	};

	template<typename T>
	struct bind_result<T, ARG_VECTOR>
	{
		static_assert(sizeof(T) == 16, "Invalid function result type for ARG_VECTOR");

		static __forceinline void func(PPUThread& CPU, const T result)
		{
			(T&)CPU.VPR[2] = result;
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
		// terminator
		return std::tuple<>();
	}

	template<int g_count, int f_count, int v_count, typename T, typename... A>
	static __forceinline std::tuple<T, A...> iterate(PPUThread& CPU)
	{
		static_assert(!std::is_pointer<T>::value, "Invalid function argument type (pointer)");
		static_assert(!std::is_reference<T>::value, "Invalid function argument type (reference)");
		// TODO: check calculations
		const bool is_float = std::is_floating_point<T>::value;
		const bool is_vector = std::is_same<T, u128>::value;
		const bind_arg_type t = is_float
			? ((f_count >= 12) ? ARG_STACK : ARG_FLOAT)
			: (is_vector ? ((v_count >= 12) ? ARG_STACK : ARG_VECTOR) : ((g_count >= 8) ? ARG_STACK : ARG_GENERAL));
		const int g = g_count + (is_float || is_vector ? 0 : 1);
		const int f = f_count + (is_float ? 1 : 0);
		const int v = v_count + (is_vector ? 1 : 0);

		return std::tuple_cat(std::tuple<T>(bind_arg<T, t, g, f, v>::func(CPU)), iterate<g, f, v, A...>(CPU));
	}

	template<typename RT, typename... T>
	class func_binder;

	template<typename... T>
	class func_binder<void, T...> : public func_caller
	{
		typedef void(*func_t)(T...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: func_caller()
			, m_call(call)
		{
		}

		virtual void operator()(PPUThread& CPU)
		{
			call<void>(m_call, iterate<0, 0, 0, T...>(CPU));
		}
	};

	template<typename... T>
	class func_binder<void, PPUThread&, T...> : public func_caller
	{
		typedef void(*func_t)(PPUThread&, T...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: func_caller()
			, m_call(call)
		{
		}

		virtual void operator()(PPUThread& CPU)
		{
			call<void>(m_call, std::tuple_cat(std::tuple<PPUThread&>(CPU), iterate<0, 0, 0, T...>(CPU)));
		}
	};

	template<typename RT, typename... T>
	class func_binder : public func_caller
	{
		typedef RT(*func_t)(T...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: func_caller()
			, m_call(call)
		{
		}

		virtual void operator()(PPUThread& CPU)
		{
			static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
			static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
			const bool is_float = std::is_floating_point<RT>::value;
			const bool is_vector = std::is_same<RT, u128>::value;
			const bind_arg_type t = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);

			bind_result<RT, t>::func(CPU, call<RT>(m_call, iterate<0, 0, 0, T...>(CPU)));
		}
	};

	template<typename RT, typename... T>
	class func_binder<RT, PPUThread&, T...> : public func_caller
	{
		typedef RT(*func_t)(PPUThread&, T...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: func_caller()
			, m_call(call)
		{
		}

		virtual void operator()(PPUThread& CPU)
		{
			static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
			static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
			const bool is_float = std::is_floating_point<RT>::value;
			const bool is_vector = std::is_same<RT, u128>::value;
			const bind_arg_type t = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);

			bind_result<RT, t>::func(CPU, call<RT>(m_call, std::tuple_cat(std::tuple<PPUThread&>(CPU), iterate<0, 0, 0, T...>(CPU))));
		}
	};
}

template<typename RT, typename... T>
func_caller* bind_func(RT(*call)(T...))
{
	return new detail::func_binder<RT, T...>(call);
}