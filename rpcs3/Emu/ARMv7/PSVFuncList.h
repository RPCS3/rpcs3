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

enum psv_error_codes
{
	SCE_OK = 0,

	SCE_ERROR_ERRNO_EPERM				 = 0x80010001,
	SCE_ERROR_ERRNO_ENOENT				 = 0x80010002,
	SCE_ERROR_ERRNO_ESRCH				 = 0x80010003,
	SCE_ERROR_ERRNO_EINTR				 = 0x80010004,
	SCE_ERROR_ERRNO_EIO					 = 0x80010005,
	SCE_ERROR_ERRNO_ENXIO				 = 0x80010006,
	SCE_ERROR_ERRNO_E2BIG				 = 0x80010007,
	SCE_ERROR_ERRNO_ENOEXEC				 = 0x80010008,
	SCE_ERROR_ERRNO_EBADF				 = 0x80010009,
	SCE_ERROR_ERRNO_ECHILD				 = 0x8001000A,
	SCE_ERROR_ERRNO_EAGAIN				 = 0x8001000B,
	SCE_ERROR_ERRNO_ENOMEM				 = 0x8001000C,
	SCE_ERROR_ERRNO_EACCES				 = 0x8001000D,
	SCE_ERROR_ERRNO_EFAULT				 = 0x8001000E,
	SCE_ERROR_ERRNO_ENOTBLK				 = 0x8001000F,
	SCE_ERROR_ERRNO_EBUSY				 = 0x80010010,
	SCE_ERROR_ERRNO_EEXIST				 = 0x80010011,
	SCE_ERROR_ERRNO_EXDEV				 = 0x80010012,
	SCE_ERROR_ERRNO_ENODEV				 = 0x80010013,
	SCE_ERROR_ERRNO_ENOTDIR				 = 0x80010014,
	SCE_ERROR_ERRNO_EISDIR				 = 0x80010015,
	SCE_ERROR_ERRNO_EINVAL				 = 0x80010016,
	SCE_ERROR_ERRNO_ENFILE				 = 0x80010017,
	SCE_ERROR_ERRNO_EMFILE				 = 0x80010018,
	SCE_ERROR_ERRNO_ENOTTY				 = 0x80010019,
	SCE_ERROR_ERRNO_ETXTBSY				 = 0x8001001A,
	SCE_ERROR_ERRNO_EFBIG				 = 0x8001001B,
	SCE_ERROR_ERRNO_ENOSPC				 = 0x8001001C,
	SCE_ERROR_ERRNO_ESPIPE				 = 0x8001001D,
	SCE_ERROR_ERRNO_EROFS				 = 0x8001001E,
	SCE_ERROR_ERRNO_EMLINK				 = 0x8001001F,
	SCE_ERROR_ERRNO_EPIPE				 = 0x80010020,
	SCE_ERROR_ERRNO_EDOM				 = 0x80010021,
	SCE_ERROR_ERRNO_ERANGE				 = 0x80010022,
	SCE_ERROR_ERRNO_ENOMSG				 = 0x80010023,
	SCE_ERROR_ERRNO_EIDRM				 = 0x80010024,
	SCE_ERROR_ERRNO_ECHRNG				 = 0x80010025,
	SCE_ERROR_ERRNO_EL2NSYNC			 = 0x80010026,
	SCE_ERROR_ERRNO_EL3HLT				 = 0x80010027,
	SCE_ERROR_ERRNO_EL3RST				 = 0x80010028,
	SCE_ERROR_ERRNO_ELNRNG				 = 0x80010029,
	SCE_ERROR_ERRNO_EUNATCH				 = 0x8001002A,
	SCE_ERROR_ERRNO_ENOCSI				 = 0x8001002B,
	SCE_ERROR_ERRNO_EL2HLT				 = 0x8001002C,
	SCE_ERROR_ERRNO_EDEADLK				 = 0x8001002D,
	SCE_ERROR_ERRNO_ENOLCK				 = 0x8001002E,
	SCE_ERROR_ERRNO_EFORMAT				 = 0x8001002F,
	SCE_ERROR_ERRNO_EUNSUP				 = 0x80010030,
	SCE_ERROR_ERRNO_EBADE				 = 0x80010032,
	SCE_ERROR_ERRNO_EBADR				 = 0x80010033,
	SCE_ERROR_ERRNO_EXFULL				 = 0x80010034,
	SCE_ERROR_ERRNO_ENOANO				 = 0x80010035,
	SCE_ERROR_ERRNO_EBADRQC				 = 0x80010036,
	SCE_ERROR_ERRNO_EBADSLT				 = 0x80010037,
	SCE_ERROR_ERRNO_EDEADLOCK			 = 0x80010038,
	SCE_ERROR_ERRNO_EBFONT				 = 0x80010039,
	SCE_ERROR_ERRNO_ENOSTR				 = 0x8001003C,
	SCE_ERROR_ERRNO_ENODATA				 = 0x8001003D,
	SCE_ERROR_ERRNO_ETIME				 = 0x8001003E,
	SCE_ERROR_ERRNO_ENOSR				 = 0x8001003F,
	SCE_ERROR_ERRNO_ENONET				 = 0x80010040,
	SCE_ERROR_ERRNO_ENOPKG				 = 0x80010041,
	SCE_ERROR_ERRNO_EREMOTE				 = 0x80010042,
	SCE_ERROR_ERRNO_ENOLINK				 = 0x80010043,
	SCE_ERROR_ERRNO_EADV				 = 0x80010044,
	SCE_ERROR_ERRNO_ESRMNT				 = 0x80010045,
	SCE_ERROR_ERRNO_ECOMM				 = 0x80010046,
	SCE_ERROR_ERRNO_EPROTO				 = 0x80010047,
	SCE_ERROR_ERRNO_EMULTIHOP			 = 0x8001004A,
	SCE_ERROR_ERRNO_ELBIN				 = 0x8001004B,
	SCE_ERROR_ERRNO_EDOTDOT				 = 0x8001004C,
	SCE_ERROR_ERRNO_EBADMSG				 = 0x8001004D,
	SCE_ERROR_ERRNO_EFTYPE				 = 0x8001004F,
	SCE_ERROR_ERRNO_ENOTUNIQ			 = 0x80010050,
	SCE_ERROR_ERRNO_EBADFD				 = 0x80010051,
	SCE_ERROR_ERRNO_EREMCHG				 = 0x80010052,
	SCE_ERROR_ERRNO_ELIBACC				 = 0x80010053,
	SCE_ERROR_ERRNO_ELIBBAD				 = 0x80010054,
	SCE_ERROR_ERRNO_ELIBSCN				 = 0x80010055,
	SCE_ERROR_ERRNO_ELIBMAX				 = 0x80010056,
	SCE_ERROR_ERRNO_ELIBEXEC			 = 0x80010057,
	SCE_ERROR_ERRNO_ENOSYS				 = 0x80010058,
	SCE_ERROR_ERRNO_ENMFILE				 = 0x80010059,
	SCE_ERROR_ERRNO_ENOTEMPTY			 = 0x8001005A,
	SCE_ERROR_ERRNO_ENAMETOOLONG		 = 0x8001005B,
	SCE_ERROR_ERRNO_ELOOP				 = 0x8001005C,
	SCE_ERROR_ERRNO_EOPNOTSUPP			 = 0x8001005F,
	SCE_ERROR_ERRNO_EPFNOSUPPORT		 = 0x80010060,
	SCE_ERROR_ERRNO_ECONNRESET			 = 0x80010068,
	SCE_ERROR_ERRNO_ENOBUFS				 = 0x80010069,
	SCE_ERROR_ERRNO_EAFNOSUPPORT		 = 0x8001006A,
	SCE_ERROR_ERRNO_EPROTOTYPE			 = 0x8001006B,
	SCE_ERROR_ERRNO_ENOTSOCK			 = 0x8001006C,
	SCE_ERROR_ERRNO_ENOPROTOOPT			 = 0x8001006D,
	SCE_ERROR_ERRNO_ESHUTDOWN			 = 0x8001006E,
	SCE_ERROR_ERRNO_ECONNREFUSED		 = 0x8001006F,
	SCE_ERROR_ERRNO_EADDRINUSE			 = 0x80010070,
	SCE_ERROR_ERRNO_ECONNABORTED		 = 0x80010071,
	SCE_ERROR_ERRNO_ENETUNREACH			 = 0x80010072,
	SCE_ERROR_ERRNO_ENETDOWN			 = 0x80010073,
	SCE_ERROR_ERRNO_ETIMEDOUT			 = 0x80010074,
	SCE_ERROR_ERRNO_EHOSTDOWN			 = 0x80010075,
	SCE_ERROR_ERRNO_EHOSTUNREACH		 = 0x80010076,
	SCE_ERROR_ERRNO_EINPROGRESS			 = 0x80010077,
	SCE_ERROR_ERRNO_EALREADY			 = 0x80010078,
	SCE_ERROR_ERRNO_EDESTADDRREQ		 = 0x80010079,
	SCE_ERROR_ERRNO_EMSGSIZE			 = 0x8001007A,
	SCE_ERROR_ERRNO_EPROTONOSUPPORT		 = 0x8001007B,
	SCE_ERROR_ERRNO_ESOCKTNOSUPPORT		 = 0x8001007C,
	SCE_ERROR_ERRNO_EADDRNOTAVAIL		 = 0x8001007D,
	SCE_ERROR_ERRNO_ENETRESET			 = 0x8001007E,
	SCE_ERROR_ERRNO_EISCONN				 = 0x8001007F,
	SCE_ERROR_ERRNO_ENOTCONN			 = 0x80010080,
	SCE_ERROR_ERRNO_ETOOMANYREFS		 = 0x80010081,
	SCE_ERROR_ERRNO_EPROCLIM			 = 0x80010082,
	SCE_ERROR_ERRNO_EUSERS				 = 0x80010083,
	SCE_ERROR_ERRNO_EDQUOT				 = 0x80010084,
	SCE_ERROR_ERRNO_ESTALE				 = 0x80010085,
	SCE_ERROR_ERRNO_ENOTSUP				 = 0x80010086,
	SCE_ERROR_ERRNO_ENOMEDIUM			 = 0x80010087,
	SCE_ERROR_ERRNO_ENOSHARE			 = 0x80010088,
	SCE_ERROR_ERRNO_ECASECLASH			 = 0x80010089,
	SCE_ERROR_ERRNO_EILSEQ				 = 0x8001008A,
	SCE_ERROR_ERRNO_EOVERFLOW			 = 0x8001008B,
	SCE_ERROR_ERRNO_ECANCELED			 = 0x8001008C,
	SCE_ERROR_ERRNO_ENOTRECOVERABLE		 = 0x8001008D,
	SCE_ERROR_ERRNO_EOWNERDEAD			 = 0x8001008E,

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
			return 0.0f;
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(v_count <= 0, "TODO: Unsupported argument type (vector)");
		static_assert(sizeof(T) == 16, "Invalid function argument type for ARG_VECTOR");

		static __forceinline T func(ARMv7Thread& CPU)
		{
			return {};
		}
	};

	template<typename T, int g_count, int f_count, int v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 0, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 0, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 4, "Invalid function argument type for ARG_STACK");

		static __forceinline T func(ARMv7Thread& CPU)
		{
			const u32 res = CPU.GetStackArg(g_count);
			return (T&)res;
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
			? ((f_count >= 4) ? ARG_STACK : ARG_FLOAT)
			: (is_vector ? ((v_count >= 4) ? ARG_STACK : ARG_VECTOR) : ((g_count >= 4) ? ARG_STACK : ARG_GENERAL));
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
	const char* const name;
	psv_func_caller* const func;
	psv_log_base* const module;
};

void add_psv_func(psv_func& data);

template<typename RT, typename... T>
void reg_psv_func(u32 nid, psv_log_base* module, const char* name, RT(*func)(T...))
{
	psv_func f =
	{
		nid,
		name,
		new psv_func_detail::func_binder<RT, T...>(func),
		module
	};

	add_psv_func(f);
}

psv_func* get_psv_func_by_nid(u32 nid);
u32 get_psv_func_index(psv_func* func);

void execute_psv_func_by_index(ARMv7Thread& CPU, u32 index);
void list_known_psv_modules();
