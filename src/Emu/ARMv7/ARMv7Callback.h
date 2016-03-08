#pragma once
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"

namespace vm
{
	template<typename AT, typename RT, typename... T>
	force_inline RT _ptr_base<RT(T...), AT>::operator()(ARMv7Thread& context, T... args) const
	{
		return psv_func_detail::func_caller<RT, T...>::call(context, VM_CAST(this->addr()), args...);
	}
}

template<typename RT, typename... T> inline RT cb_call(ARMv7Thread& context, u32 addr, T... args)
{
	return psv_func_detail::func_caller<RT, T...>::call(context, addr, args...);
}
