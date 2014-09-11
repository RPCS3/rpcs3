#pragma once
#include "Emu/Cell/PPUThread.h"

namespace vm
{
	template<typename TT>
	struct _func_arg
	{
		static_assert(!std::is_floating_point<TT>::value, "TODO: Unsupported callback argument type (floating point)");
		static_assert(!std::is_same<TT, u128>::value, "TODO: Unsupported callback argument type (vector)");

		static_assert(sizeof(TT) <= 8, "Invalid callback argument type");
		static_assert(!std::is_pointer<TT>::value, "Invalid callback argument type (pointer)");
		static_assert(!std::is_reference<TT>::value, "Invalid callback argument type (reference)");

		__forceinline static u64 get_value(const TT& arg)
		{
			u64 res = 0;
			(TT&)res = arg;
			return res;
		}
	};

	template<typename RT>
	struct _func_res
	{
		static_assert(!std::is_floating_point<RT>::value, "TODO: Unsupported callback result type (floating point)");
		static_assert(!std::is_same<RT, u128>::value, "TODO: Unsupported callback result type (vector)");

		static_assert(sizeof(RT) <= 8, "Invalid callback result type");
		static_assert(!std::is_pointer<RT>::value, "Invalid callback result type (pointer)");
		static_assert(!std::is_reference<RT>::value, "Invalid callback result type (reference)");

		__forceinline static RT get_value(const u64 res)
		{
			return (RT&)res;
		}
	};

	template<>
	struct _func_res<void>
	{
		__forceinline static void get_value(const u64 res)
		{
		}
	};

	template<typename AT, typename RT, typename... T>
	RT _ptr_base<RT(*)(T...), 1, AT>::operator ()(T... args) const
	{
		return _func_res<RT>::get_value(GetCurrentPPUThread().FastCall(vm::read32(m_addr), vm::read32(m_addr + 4), _func_arg<T>::get_value(args)...));
	}
}