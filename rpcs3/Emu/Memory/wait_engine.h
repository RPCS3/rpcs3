#pragma once

#include "Utilities/types.h"
#include "Utilities/Macro.h"

class thread_ctrl;

namespace vm
{
	struct waiter_base
	{
		u32 addr;
		u32 mask;
		thread_ctrl* thread{};

		void initialize(u32 addr, u32 size);
		bool try_notify();

	protected:
		virtual bool test() = 0;
	};

	// Wait until pred() returns true, addr must be aligned to size which must be a power of 2.
	// It's possible for pred() to be called from any thread once the waiter is registered.
	template<typename F>
	auto wait_op(u32 addr, u32 size, F&& pred) -> decltype(static_cast<void>(pred()))
	{
		if (LIKELY(pred())) return;

		struct waiter : waiter_base
		{
			std::conditional_t<sizeof(F) <= sizeof(void*), std::remove_reference_t<F>, F&&> func;

			waiter(F&& func)
				: func(std::forward<F>(func))
			{
			}

			bool test() override
			{
				return func();
			}
		};

		waiter(std::forward<F>(pred)).initialize(addr, size);
	}

	// Notify waiters on specific addr, addr must be aligned to size which must be a power of 2
	void notify_at(u32 addr, u32 size);
}
