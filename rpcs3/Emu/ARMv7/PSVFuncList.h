#pragma once
#include "ARMv7Thread.h"
#include "Emu/SysCalls/LogBase.h"

class psv_log_base : public LogBase
{
	std::string m_name;

public:
	psv_log_base(const std::string& name)
		: m_name(name)
	{
	}

	virtual const std::string& GetName() const override
	{
		return m_name;
	}

};

class psv_func_caller
{
public:
	virtual void operator()(ARMv7Thread& CPU) = 0;
	virtual ~psv_func_caller(){};
};

namespace psv_func_detail
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
		static_assert(sizeof(T) <= 4, "Invalid function argument type for ARG_GENERAL");

		static __forceinline T func(ARMv7Thread& CPU)
		{
			return (T&)CPU.GPR[g_count - 1];
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(f_count <= 0, "TODO: Unsupported argument type (float)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		static __forceinline T func(ARMv7Thread& CPU)
		{
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(v_count <= 0, "TODO: Unsupported argument type (vector)");
		static_assert(sizeof(T) == 16, "Invalid function argument type for ARG_VECTOR");

		static __forceinline T func(ARMv7Thread& CPU)
		{
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 4, "TODO: Unsupported stack argument type (general)");
		static_assert(f_count <= 0, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 0, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 4, "Invalid function argument type for ARG_STACK");

		static __forceinline T func(ARMv7Thread& CPU)
		{
		}
	};

	template<typename T, bind_arg_type type>
	struct bind_result
	{
		static_assert(type != ARG_FLOAT, "TODO: Unsupported funcion result type (float)");
		static_assert(type != ARG_VECTOR, "TODO: Unsupported funcion result type (vector)");
		static_assert(type == ARG_GENERAL, "Wrong use of bind_result template");
		static_assert(sizeof(T) <= 4, "Invalid function result type for ARG_GENERAL");

		static __forceinline void func(ARMv7Thread& CPU, T result)
		{
			(T&)CPU.GPR[0] = result;
		}
	};

	//template<typename T>
	//struct bind_result<T, ARG_FLOAT>
	//{
	//	static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_FLOAT");

	//	static __forceinline void func(ARMv7Thread& CPU, T result)
	//	{
	//	}
	//};

	//template<typename T>
	//struct bind_result<T, ARG_VECTOR>
	//{
	//	static_assert(sizeof(T) == 16, "Invalid function result type for ARG_VECTOR");

	//	static __forceinline void func(ARMv7Thread& CPU, const T result)
	//	{
	//	}
	//};

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
		return psv_func_detail::call_impl<RT, F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
	}

	template<int g_count, int f_count, int v_count>
	static __forceinline std::tuple<> iterate(ARMv7Thread& CPU)
	{
		// terminator
		return std::tuple<>();
	}

	template<int g_count, int f_count, int v_count, typename T, typename... A>
	static __forceinline std::tuple<T, A...> iterate(ARMv7Thread& CPU)
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

	template<typename RT, typename... T>
	class func_binder;

	template<typename... T>
	class func_binder<void, T...> : public psv_func_caller
	{
		typedef void(*func_t)(T...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: psv_func_caller()
			, m_call(call)
		{
		}

		virtual void operator()(ARMv7Thread& CPU)
		{
			call<void>(m_call, iterate<0, 0, 0, T...>(CPU));
		}
	};

	template<typename... T>
	class func_binder<void, ARMv7Thread&, T...> : public psv_func_caller
	{
		typedef void(*func_t)(ARMv7Thread&, T...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: psv_func_caller()
			, m_call(call)
		{
		}

		virtual void operator()(ARMv7Thread& CPU)
		{
			call<void>(m_call, std::tuple_cat(std::tuple<ARMv7Thread&>(CPU), iterate<0, 0, 0, T...>(CPU)));
		}
	};

	template<typename RT, typename... T>
	class func_binder : public psv_func_caller
	{
		typedef RT(*func_t)(T...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: psv_func_caller()
			, m_call(call)
		{
		}

		virtual void operator()(ARMv7Thread& CPU)
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
	class func_binder<RT, ARMv7Thread&, T...> : public psv_func_caller
	{
		typedef RT(*func_t)(ARMv7Thread&, T...);
		const func_t m_call;

	public:
		func_binder(func_t call)
			: psv_func_caller()
			, m_call(call)
		{
		}

		virtual void operator()(ARMv7Thread& CPU)
		{
			static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
			static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
			const bool is_float = std::is_floating_point<RT>::value;
			const bool is_vector = std::is_same<RT, u128>::value;
			const bind_arg_type t = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);

			bind_result<RT, t>::func(CPU, call<RT>(m_call, std::tuple_cat(std::tuple<ARMv7Thread&>(CPU), iterate<0, 0, 0, T...>(CPU))));
		}
	};
}

struct psv_func
{
	const u32 nid;
	psv_func_caller* const func;
	psv_log_base* const module;
};

void add_psv_func(psv_func& data);

template<typename RT, typename... T>
void reg_psv_func(u32 nid, psv_log_base* module, RT(*func)(T...))
{
	psv_func f =
	{
		nid,
		new psv_func_detail::func_binder<RT, T...>(func),
		module
	};

	add_psv_func(f);
}

psv_func* get_psv_func_by_nid(u32 nid);
u32 get_psv_func_index(psv_func* func);

void execute_psv_func_by_index(ARMv7Thread& CPU, u32 index);
void list_known_psv_modules();
