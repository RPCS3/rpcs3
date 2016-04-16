#pragma once

#include "ARMv7Function.h"

namespace vm
{
	template<typename AT, typename RT, typename... T>
	force_inline RT _ptr_base<RT(T...), AT>::operator()(ARMv7Thread& cpu, T... args) const
	{
		return arm_func_detail::func_caller<RT, T...>::call(cpu, vm::cast(this->addr(), HERE), args...);
	}
}

template<typename RT, typename... T> inline RT cb_call(ARMv7Thread& cpu, u32 addr, T... args)
{
	return arm_func_detail::func_caller<RT, T...>::call(cpu, addr, args...);
}
