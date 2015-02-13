#pragma once
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"

namespace vm
{
	template<typename AT, typename RT, typename... T>
	__forceinline RT _ptr_base<RT(T...), 1, AT>::operator()(ARMv7Context& context, T... args) const
	{
		return psv_func_detail::func_caller<RT, T...>::call(context, vm::cast(this->addr()), args...);
	}
}

template<typename RT, typename... T>
__forceinline RT cb_call(ARMv7Context& context, u32 addr, T... args)
{
	return psv_func_detail::func_caller<RT, T...>::call(context, addr, args...);
}
