#pragma once

#include "Emu/Memory/Memory.h"
#include "ARMv7Thread.h"

namespace vm { using namespace psv; }

// PSV module class
class psv_log_base : public _log::channel
{
	using init_func_t = void(*)();

	init_func_t m_init;

public:
	std::function<void()> on_load;
	std::function<void()> on_unload;
	std::function<void()> on_stop;
	std::function<void(s32 error, struct psv_func* func)> on_error;

public:
	psv_log_base(const std::string& name, init_func_t init);

	void Init()
	{
		on_load = nullptr;
		on_unload = nullptr;
		on_stop = nullptr;
		on_error = nullptr;

		m_init();
	}
};

using armv7_func_caller = void(*)(ARMv7Thread&);

struct armv7_va_args_t
{
	u32 g_count;
	u32 f_count;
	u32 v_count;
};

// Utilities for binding ARMv7Context to C++ function arguments received by HLE functions or sent to callbacks
namespace psv_func_detail
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

	static const auto FIXED_STACK_FRAME_SIZE = 0x80; // described in CB_FUNC.h

	template<typename T, arg_class type, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg
	{
		static_assert(type == ARG_GENERAL, "Unknown function argument type");
		static_assert(!std::is_pointer<T>::value, "Invalid function argument type (pointer)");
		static_assert(!std::is_reference<T>::value, "Invalid function argument type (reference)");
		static_assert(sizeof(T) <= 4, "Invalid function argument type for ARG_GENERAL");

		force_inline static T get_arg(ARMv7Thread& context)
		{
			return cast_from_armv7_gpr<T>(context.GPR[g_count - 1]);
		}

		force_inline static void put_arg(ARMv7Thread& context, const T& arg)
		{
			context.GPR[g_count - 1] = cast_to_armv7_gpr<T>(arg);
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<u64, ARG_GENERAL, g_count, f_count, v_count>
	{
		// first u64 argument is passed in r0-r1, second one is passed in r2-r3 (if g_count = 3)
		static_assert(g_count == 2 || g_count == 4, "Wrong u64 argument position");

		force_inline static u64 get_arg(ARMv7Thread& context)
		{
			return context.GPR_D[(g_count - 1) >> 1];
		}

		force_inline static void put_arg(ARMv7Thread& context, u64 arg)
		{
			context.GPR_D[(g_count - 1) >> 1] = arg;
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<s64, ARG_GENERAL, g_count, f_count, v_count>
	{
		static_assert(g_count == 2 || g_count == 4, "Wrong s64 argument position");

		force_inline static s64 get_arg(ARMv7Thread& context)
		{
			return context.GPR_D[(g_count - 1) >> 1];
		}

		force_inline static void put_arg(ARMv7Thread& context, s64 arg)
		{
			context.GPR_D[(g_count - 1) >> 1] = arg;
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_FLOAT, g_count, f_count, v_count>
	{
		static_assert(f_count <= 0, "TODO: Unsupported argument type (float)");
		static_assert(sizeof(T) <= 8, "Invalid function argument type for ARG_FLOAT");

		force_inline static T get_arg(ARMv7Thread& context)
		{
		}

		force_inline static void put_arg(ARMv7Thread& context, const T& arg)
		{
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VECTOR, g_count, f_count, v_count>
	{
		static_assert(v_count <= 0, "TODO: Unsupported argument type (vector)");
		static_assert(std::is_same<std::remove_cv_t<T>, v128>::value, "Invalid function argument type for ARG_VECTOR");

		force_inline static T get_arg(ARMv7Thread& context)
		{
		}

		force_inline static void put_arg(ARMv7Thread& context, const T& arg)
		{
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_STACK, g_count, f_count, v_count>
	{
		static_assert(f_count <= 0, "TODO: Unsupported stack argument type (float)");
		static_assert(v_count <= 0, "TODO: Unsupported stack argument type (vector)");
		static_assert(sizeof(T) <= 4, "Invalid function argument type for ARG_STACK");

		force_inline static T get_arg(ARMv7Thread& context)
		{
			// TODO: check
			return cast_from_armv7_gpr<T>(vm::read32(context.SP + sizeof(u32) * (g_count - 5)));
		}

		force_inline static void put_arg(ARMv7Thread& context, const T& arg)
		{
			// TODO: check
			const int stack_pos = (g_count - 5) * 4 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < 0, "TODO: Increase fixed stack frame size (arg count limit broken)");

			vm::write32(context.SP + stack_pos, cast_to_armv7_gpr<T>(arg));
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<u64, ARG_STACK, g_count, f_count, v_count>
	{
		force_inline static u64 get_arg(ARMv7Thread& context)
		{
			// TODO: check
			return vm::read64(context.SP + sizeof(u32) * (g_count - 6));
		}

		force_inline static void put_arg(ARMv7Thread& context, u64 arg)
		{
			// TODO: check
			const int stack_pos = (g_count - 6) * 4 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < -4, "TODO: Increase fixed stack frame size (arg count limit broken)");

			vm::write64(context.SP + stack_pos, arg);
		}
	};

	template<u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<s64, ARG_STACK, g_count, f_count, v_count>
	{
		force_inline static s64 get_arg(ARMv7Thread& context)
		{
			// TODO: check
			return vm::read64(context.SP + sizeof(u32) * (g_count - 6));
		}

		force_inline static void put_arg(ARMv7Thread& context, s64 arg)
		{
			// TODO: check
			const int stack_pos = (g_count - 6) * 4 - FIXED_STACK_FRAME_SIZE;
			static_assert(stack_pos < -4, "TODO: Increase fixed stack frame size (arg count limit broken)");

			vm::write64(context.SP + stack_pos, arg);
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_CONTEXT, g_count, f_count, v_count>
	{
		static_assert(std::is_same<T, ARMv7Thread&>::value, "Invalid function argument type for ARG_CONTEXT");

		force_inline static ARMv7Thread& get_arg(ARMv7Thread& context)
		{
			return context;
		}

		force_inline static void put_arg(ARMv7Thread& context, ARMv7Thread& arg)
		{
		}
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct bind_arg<T, ARG_VARIADIC, g_count, f_count, v_count>
	{
		static_assert(std::is_same<std::remove_cv_t<T>, armv7_va_args_t>::value, "Invalid function argument type for ARG_VARIADIC");

		force_inline static armv7_va_args_t get_arg(ARMv7Thread& context)
		{
			return{ g_count, f_count, v_count };
		}
	};

	template<typename T, arg_class type>
	struct bind_result
	{
		static_assert(type != ARG_FLOAT, "TODO: Unsupported funcion result type (float)");
		static_assert(type != ARG_VECTOR, "TODO: Unsupported funcion result type (vector)");
		static_assert(type == ARG_GENERAL, "Wrong use of bind_result template");
		static_assert(sizeof(T) <= 4, "Invalid function result type for ARG_GENERAL");

		force_inline static T get_result(ARMv7Thread& context)
		{
			return cast_from_armv7_gpr<T>(context.GPR[0]);
		}

		force_inline static void put_result(ARMv7Thread& context, const T& result)
		{
			context.GPR[0] = cast_to_armv7_gpr<T>(result);
		}
	};

	template<>
	struct bind_result<u64, ARG_GENERAL>
	{
		force_inline static u64 get_result(ARMv7Thread& context)
		{
			return context.GPR_D[0];
		}

		force_inline static void put_result(ARMv7Thread& context, u64 result)
		{
			context.GPR_D[0] = result;
		}
	};

	template<>
	struct bind_result<s64, ARG_GENERAL>
	{
		force_inline static s64 get_result(ARMv7Thread& context)
		{
			return context.GPR_D[0];
		}

		force_inline static void put_result(ARMv7Thread& context, s64 result)
		{
			context.GPR_D[0] = result;
		}
	};

	//template<typename T>
	//struct bind_result<T, ARG_FLOAT>
	//{
	//	static_assert(sizeof(T) <= 8, "Invalid function result type for ARG_FLOAT");

	//	static force_inline void put_result(ARMv7Thread& context, const T& result)
	//	{
	//	}
	//};

	//template<typename T>
	//struct bind_result<T, ARG_VECTOR>
	//{
	//	static_assert(std::is_same<std::remove_cv_t<T>, v128>::value, "Invalid function result type for ARG_VECTOR");

	//	static force_inline void put_result(ARMv7Thread& context, const T& result)
	//	{
	//	}
	//};

	template<typename RT>
	struct result_type
	{
		static_assert(!std::is_pointer<RT>::value, "Invalid function result type (pointer)");
		static_assert(!std::is_reference<RT>::value, "Invalid function result type (reference)");
		static const bool is_float = std::is_floating_point<RT>::value;
		static const bool is_vector = std::is_same<std::remove_cv_t<RT>, v128>::value;
		static const arg_class value = is_float ? ARG_FLOAT : (is_vector ? ARG_VECTOR : ARG_GENERAL);
	};

	template<typename T, u32 g_count, u32 f_count, u32 v_count>
	struct arg_type
	{
		// TODO: check calculations
		static const bool is_float = std::is_floating_point<T>::value;
		static const bool is_vector = std::is_same<std::remove_cv_t<T>, v128>::value;
		static const bool is_context = std::is_same<T, ARMv7Thread&>::value;
		static const bool is_variadic = std::is_same<std::remove_cv_t<T>, armv7_va_args_t>::value;
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
		force_inline static T get_arg(ARMv7Thread& context)
		{
			return bind_arg<T, static_cast<arg_class>(type_pack & 0xff), (type_pack >> 8) & 0xff, (type_pack >> 16) & 0xff, (type_pack >> 24)>::get_arg(context);
		}
	};

	template<u32... Info, typename RT, typename... Args>
	force_inline RT call(ARMv7Thread& context, RT(*func)(Args...), arg_info_pack_t<Info...> info)
	{
		// do the actual function call when all arguments are prepared (simultaneous unpacking of Args... and Info...)
		return func(bind_arg_packed<Args, Info>::get_arg(context)...);
	}

	template<typename T, typename... Types, u32... Info, typename RT, typename... Args>
	force_inline RT call(ARMv7Thread& context, RT(*func)(Args...), arg_info_pack_t<Info...> info)
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

		return call<Types...>(context, func, arg_info_pack_t<Info..., t | (g << 8) | (f << 16) | (v << 24)>{});
	}

	template<u32 g_count, u32 f_count, u32 v_count>
	force_inline static bool put_func_args(ARMv7Thread& context)
	{
		// terminator
		return false;
	}

	template<u32 g_count, u32 f_count, u32 v_count, typename T1, typename... T>
	force_inline static bool put_func_args(ARMv7Thread& context, T1 arg, T... args)
	{
		using type = arg_type<T1, g_count, f_count, v_count>;
		const arg_class t = type::value;
		const u32 g = type::g_value;
		const u32 f = type::f_value;
		const u32 v = type::v_value;

		bind_arg<T1, t, g, f, v>::put_arg(context, arg);

		// return true if stack was used
		return put_func_args<g, f, v>(context, args...) || (t == ARG_STACK);
	}

	template<typename RT, typename... T>
	struct func_binder;

	template<typename... T>
	struct func_binder<void, T...>
	{
		using func_t = void(*)(T...);

		static void do_call(ARMv7Thread& context, func_t func)
		{
			call<T...>(context, func, arg_info_pack_t<>{});
		}
	};

	template<typename RT, typename... T>
	struct func_binder
	{
		using func_t =  RT(*)(T...);

		static void do_call(ARMv7Thread& context, func_t func)
		{
			bind_result<RT, result_type<RT>::value>::put_result(context, call<T...>(context, func, arg_info_pack_t<>{}));
		}
	};

	template<typename RT, typename... T>
	struct func_caller
	{
		force_inline static RT call(ARMv7Thread& context, u32 addr, T... args)
		{
			func_caller<void, T...>::call(context, addr, args...);

			return bind_result<RT, result_type<RT>::value>::get_result(context);
		}
	};

	template<typename... T>
	struct func_caller<void, T...>
	{
		force_inline static void call(ARMv7Thread& context, u32 addr, T... args)
		{
			if (put_func_args<0, 0, 0, T...>(context, args...))
			{
				context.SP -= FIXED_STACK_FRAME_SIZE;
				context.fast_call(addr);
				context.SP += FIXED_STACK_FRAME_SIZE;
			}
			else
			{
				context.fast_call(addr);
			}
		}
	};
}

// Basic information about the HLE function
struct psv_func
{
	u32 nid; // Unique function ID (should be generated individually for each elf loaded)
	u32 flags;
	const char* name; // Function name for information
	armv7_func_caller func; // Function caller
	psv_log_base* module; // Module for information

	psv_func()
	{
	}

	psv_func(u32 nid, u32 flags, psv_log_base* module, const char* name, armv7_func_caller func)
		: nid(nid)
		, flags(flags)
		, name(name)
		, func(func)
		, module(module)
	{
	}
};

enum psv_special_function_index : u16
{
	SFI_HLE_RETURN,

	SFI_MAX
};

// Do not call directly
u32 add_psv_func(psv_func data);
// Do not call directly
template<typename RT, typename... T> force_inline void call_psv_func(ARMv7Thread& context, RT(*func)(T...))
{
	psv_func_detail::func_binder<RT, T...>::do_call(context, func);
}

#define reg_psv_func(nid, module, name, func) add_psv_func(psv_func(nid, 0, module, name, [](ARMv7Thread& context){ call_psv_func(context, func); }))

// Find registered HLE function by NID
psv_func* get_psv_func_by_nid(u32 nid, u32* out_index = nullptr);
// Find registered HLE function by its index
psv_func* get_psv_func_by_index(u32 index);
// Execute registered HLE function by its index
void execute_psv_func_by_index(ARMv7Thread& context, u32 index);
// Register all HLE functions
void initialize_psv_modules();
// Unregister all HLE functions
void finalize_psv_modules();

// General definitions

enum psv_error_codes
{
	SCE_OK = 0,

	SCE_ERROR_ERRNO_EPERM = 0x80010001,
	SCE_ERROR_ERRNO_ENOENT = 0x80010002,
	SCE_ERROR_ERRNO_ESRCH = 0x80010003,
	SCE_ERROR_ERRNO_EINTR = 0x80010004,
	SCE_ERROR_ERRNO_EIO = 0x80010005,
	SCE_ERROR_ERRNO_ENXIO = 0x80010006,
	SCE_ERROR_ERRNO_E2BIG = 0x80010007,
	SCE_ERROR_ERRNO_ENOEXEC = 0x80010008,
	SCE_ERROR_ERRNO_EBADF = 0x80010009,
	SCE_ERROR_ERRNO_ECHILD = 0x8001000A,
	SCE_ERROR_ERRNO_EAGAIN = 0x8001000B,
	SCE_ERROR_ERRNO_ENOMEM = 0x8001000C,
	SCE_ERROR_ERRNO_EACCES = 0x8001000D,
	SCE_ERROR_ERRNO_EFAULT = 0x8001000E,
	SCE_ERROR_ERRNO_ENOTBLK = 0x8001000F,
	SCE_ERROR_ERRNO_EBUSY = 0x80010010,
	SCE_ERROR_ERRNO_EEXIST = 0x80010011,
	SCE_ERROR_ERRNO_EXDEV = 0x80010012,
	SCE_ERROR_ERRNO_ENODEV = 0x80010013,
	SCE_ERROR_ERRNO_ENOTDIR = 0x80010014,
	SCE_ERROR_ERRNO_EISDIR = 0x80010015,
	SCE_ERROR_ERRNO_EINVAL = 0x80010016,
	SCE_ERROR_ERRNO_ENFILE = 0x80010017,
	SCE_ERROR_ERRNO_EMFILE = 0x80010018,
	SCE_ERROR_ERRNO_ENOTTY = 0x80010019,
	SCE_ERROR_ERRNO_ETXTBSY = 0x8001001A,
	SCE_ERROR_ERRNO_EFBIG = 0x8001001B,
	SCE_ERROR_ERRNO_ENOSPC = 0x8001001C,
	SCE_ERROR_ERRNO_ESPIPE = 0x8001001D,
	SCE_ERROR_ERRNO_EROFS = 0x8001001E,
	SCE_ERROR_ERRNO_EMLINK = 0x8001001F,
	SCE_ERROR_ERRNO_EPIPE = 0x80010020,
	SCE_ERROR_ERRNO_EDOM = 0x80010021,
	SCE_ERROR_ERRNO_ERANGE = 0x80010022,
	SCE_ERROR_ERRNO_ENOMSG = 0x80010023,
	SCE_ERROR_ERRNO_EIDRM = 0x80010024,
	SCE_ERROR_ERRNO_ECHRNG = 0x80010025,
	SCE_ERROR_ERRNO_EL2NSYNC = 0x80010026,
	SCE_ERROR_ERRNO_EL3HLT = 0x80010027,
	SCE_ERROR_ERRNO_EL3RST = 0x80010028,
	SCE_ERROR_ERRNO_ELNRNG = 0x80010029,
	SCE_ERROR_ERRNO_EUNATCH = 0x8001002A,
	SCE_ERROR_ERRNO_ENOCSI = 0x8001002B,
	SCE_ERROR_ERRNO_EL2HLT = 0x8001002C,
	SCE_ERROR_ERRNO_EDEADLK = 0x8001002D,
	SCE_ERROR_ERRNO_ENOLCK = 0x8001002E,
	SCE_ERROR_ERRNO_EFORMAT = 0x8001002F,
	SCE_ERROR_ERRNO_EUNSUP = 0x80010030,
	SCE_ERROR_ERRNO_EBADE = 0x80010032,
	SCE_ERROR_ERRNO_EBADR = 0x80010033,
	SCE_ERROR_ERRNO_EXFULL = 0x80010034,
	SCE_ERROR_ERRNO_ENOANO = 0x80010035,
	SCE_ERROR_ERRNO_EBADRQC = 0x80010036,
	SCE_ERROR_ERRNO_EBADSLT = 0x80010037,
	SCE_ERROR_ERRNO_EDEADLOCK = 0x80010038,
	SCE_ERROR_ERRNO_EBFONT = 0x80010039,
	SCE_ERROR_ERRNO_ENOSTR = 0x8001003C,
	SCE_ERROR_ERRNO_ENODATA = 0x8001003D,
	SCE_ERROR_ERRNO_ETIME = 0x8001003E,
	SCE_ERROR_ERRNO_ENOSR = 0x8001003F,
	SCE_ERROR_ERRNO_ENONET = 0x80010040,
	SCE_ERROR_ERRNO_ENOPKG = 0x80010041,
	SCE_ERROR_ERRNO_EREMOTE = 0x80010042,
	SCE_ERROR_ERRNO_ENOLINK = 0x80010043,
	SCE_ERROR_ERRNO_EADV = 0x80010044,
	SCE_ERROR_ERRNO_ESRMNT = 0x80010045,
	SCE_ERROR_ERRNO_ECOMM = 0x80010046,
	SCE_ERROR_ERRNO_EPROTO = 0x80010047,
	SCE_ERROR_ERRNO_EMULTIHOP = 0x8001004A,
	SCE_ERROR_ERRNO_ELBIN = 0x8001004B,
	SCE_ERROR_ERRNO_EDOTDOT = 0x8001004C,
	SCE_ERROR_ERRNO_EBADMSG = 0x8001004D,
	SCE_ERROR_ERRNO_EFTYPE = 0x8001004F,
	SCE_ERROR_ERRNO_ENOTUNIQ = 0x80010050,
	SCE_ERROR_ERRNO_EBADFD = 0x80010051,
	SCE_ERROR_ERRNO_EREMCHG = 0x80010052,
	SCE_ERROR_ERRNO_ELIBACC = 0x80010053,
	SCE_ERROR_ERRNO_ELIBBAD = 0x80010054,
	SCE_ERROR_ERRNO_ELIBSCN = 0x80010055,
	SCE_ERROR_ERRNO_ELIBMAX = 0x80010056,
	SCE_ERROR_ERRNO_ELIBEXEC = 0x80010057,
	SCE_ERROR_ERRNO_ENOSYS = 0x80010058,
	SCE_ERROR_ERRNO_ENMFILE = 0x80010059,
	SCE_ERROR_ERRNO_ENOTEMPTY = 0x8001005A,
	SCE_ERROR_ERRNO_ENAMETOOLONG = 0x8001005B,
	SCE_ERROR_ERRNO_ELOOP = 0x8001005C,
	SCE_ERROR_ERRNO_EOPNOTSUPP = 0x8001005F,
	SCE_ERROR_ERRNO_EPFNOSUPPORT = 0x80010060,
	SCE_ERROR_ERRNO_ECONNRESET = 0x80010068,
	SCE_ERROR_ERRNO_ENOBUFS = 0x80010069,
	SCE_ERROR_ERRNO_EAFNOSUPPORT = 0x8001006A,
	SCE_ERROR_ERRNO_EPROTOTYPE = 0x8001006B,
	SCE_ERROR_ERRNO_ENOTSOCK = 0x8001006C,
	SCE_ERROR_ERRNO_ENOPROTOOPT = 0x8001006D,
	SCE_ERROR_ERRNO_ESHUTDOWN = 0x8001006E,
	SCE_ERROR_ERRNO_ECONNREFUSED = 0x8001006F,
	SCE_ERROR_ERRNO_EADDRINUSE = 0x80010070,
	SCE_ERROR_ERRNO_ECONNABORTED = 0x80010071,
	SCE_ERROR_ERRNO_ENETUNREACH = 0x80010072,
	SCE_ERROR_ERRNO_ENETDOWN = 0x80010073,
	SCE_ERROR_ERRNO_ETIMEDOUT = 0x80010074,
	SCE_ERROR_ERRNO_EHOSTDOWN = 0x80010075,
	SCE_ERROR_ERRNO_EHOSTUNREACH = 0x80010076,
	SCE_ERROR_ERRNO_EINPROGRESS = 0x80010077,
	SCE_ERROR_ERRNO_EALREADY = 0x80010078,
	SCE_ERROR_ERRNO_EDESTADDRREQ = 0x80010079,
	SCE_ERROR_ERRNO_EMSGSIZE = 0x8001007A,
	SCE_ERROR_ERRNO_EPROTONOSUPPORT = 0x8001007B,
	SCE_ERROR_ERRNO_ESOCKTNOSUPPORT = 0x8001007C,
	SCE_ERROR_ERRNO_EADDRNOTAVAIL = 0x8001007D,
	SCE_ERROR_ERRNO_ENETRESET = 0x8001007E,
	SCE_ERROR_ERRNO_EISCONN = 0x8001007F,
	SCE_ERROR_ERRNO_ENOTCONN = 0x80010080,
	SCE_ERROR_ERRNO_ETOOMANYREFS = 0x80010081,
	SCE_ERROR_ERRNO_EPROCLIM = 0x80010082,
	SCE_ERROR_ERRNO_EUSERS = 0x80010083,
	SCE_ERROR_ERRNO_EDQUOT = 0x80010084,
	SCE_ERROR_ERRNO_ESTALE = 0x80010085,
	SCE_ERROR_ERRNO_ENOTSUP = 0x80010086,
	SCE_ERROR_ERRNO_ENOMEDIUM = 0x80010087,
	SCE_ERROR_ERRNO_ENOSHARE = 0x80010088,
	SCE_ERROR_ERRNO_ECASECLASH = 0x80010089,
	SCE_ERROR_ERRNO_EILSEQ = 0x8001008A,
	SCE_ERROR_ERRNO_EOVERFLOW = 0x8001008B,
	SCE_ERROR_ERRNO_ECANCELED = 0x8001008C,
	SCE_ERROR_ERRNO_ENOTRECOVERABLE = 0x8001008D,
	SCE_ERROR_ERRNO_EOWNERDEAD = 0x8001008E,
};

struct SceDateTime
{
	le_t<u16> year;
	le_t<u16> month;
	le_t<u16> day;
	le_t<u16> hour;
	le_t<u16> minute;
	le_t<u16> second;
	le_t<u32> microsecond;
};

struct SceFVector3
{
	le_t<float> x, y, z;
};

struct SceFQuaternion
{
	le_t<float> x, y, z, w;
};

union SceUMatrix4
{
	struct
	{
		le_t<float> f[4][4];
	};

	struct
	{
		le_t<s32>   i[4][4];
	};
};
