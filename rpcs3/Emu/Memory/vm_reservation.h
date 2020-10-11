#pragma once

#include "vm.h"
#include "vm_locking.h"
#include "Utilities/cond.h"
#include "util/atomic.hpp"
#include <functional>

extern bool g_use_rtm;

namespace vm
{
	enum : u64
	{
		rsrv_lock_mask = 127,
		rsrv_unique_lock = 64,
		rsrv_shared_mask = 63,
	};

	// Get reservation status for further atomic update: last update timestamp
	inline atomic_t<u64>& reservation_acquire(u32 addr, u32 size)
	{
		// Access reservation info: stamp and the lock bit
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	// Update reservation status
	void reservation_update(u32 addr);

	// Get reservation sync variable
	inline atomic_t<u64>& reservation_notifier(u32 addr, u32 size)
	{
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	u64 reservation_lock_internal(u32, atomic_t<u64>&);

	void reservation_shared_lock_internal(atomic_t<u64>&);

	inline bool reservation_try_lock(atomic_t<u64>& res, u64 rtime)
	{
		if (res.compare_and_swap_test(rtime, rtime | rsrv_unique_lock)) [[likely]]
		{
			return true;
		}

		return false;
	}

	inline std::pair<atomic_t<u64>&, u64> reservation_lock(u32 addr)
	{
		auto res = &vm::reservation_acquire(addr, 1);
		auto rtime = res->load();

		if (rtime & 127 || !reservation_try_lock(*res, rtime)) [[unlikely]]
		{
			static atomic_t<u64> no_lock{};

			rtime = reservation_lock_internal(addr, *res);

			if (rtime == umax)
			{
				res = &no_lock;
			}
		}

		return {*res, rtime};
	}

	void reservation_op_internal(u32 addr, std::function<bool()> func);

	template <typename T, typename AT = u32, typename F>
	SAFE_BUFFERS inline auto reservation_op(_ptr_base<T, AT> ptr, F op)
	{
		// Atomic operation will be performed on aligned 128 bytes of data, so the data size and alignment must comply
		static_assert(sizeof(T) <= 128 && alignof(T) == sizeof(T), "vm::reservation_op: unsupported type");
		static_assert(std::is_trivially_copyable_v<T>, "vm::reservation_op: not triv copyable (optimization)");

		// Use "super" pointer to prevent access violation handling during atomic op
		const auto sptr = vm::get_super_ptr<T>(static_cast<u32>(ptr.addr()));

		// Use 128-byte aligned addr
		const u32 addr = static_cast<u32>(ptr.addr()) & -128;

		if (g_use_rtm)
		{
			auto& res = vm::reservation_acquire(addr, 128);

			// Stage 1: single optimistic transaction attempt
			unsigned status = _XBEGIN_STARTED;
			u64 _old = 0;

#ifndef _MSC_VER
			__asm__ goto ("xbegin %l[stage2];" ::: "memory" : stage2);
#else
			status = _xbegin();
			if (status == _XBEGIN_STARTED)
#endif
			{
				if (res & rsrv_unique_lock)
				{
#ifndef _MSC_VER
					__asm__ volatile ("xabort $0;" ::: "memory");
#else
					_xabort(0);
#endif
				}

				if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
				{
					res += 128;
					std::invoke(op, *sptr);
#ifndef _MSC_VER
					__asm__ volatile ("xend;" ::: "memory");
#else
					_xend();
#endif
					res.notify_all();
					return;
				}
				else
				{
					if (auto result = std::invoke(op, *sptr))
					{
						res += 128;
#ifndef _MSC_VER
						__asm__ volatile ("xend;" ::: "memory");
#else
						_xend();
#endif
						res.notify_all();
						return result;
					}
					else
					{
#ifndef _MSC_VER
						__asm__ volatile ("xabort $1;" ::: "memory");
#else
						_xabort(1);
#endif
						// Unreachable code
						return std::invoke_result_t<F, T&>();
					}
				}
			}

			stage2:
#ifndef _MSC_VER
			__asm__ volatile ("movl %%eax, %0;" : "=r" (status) :: "memory");
#endif
			if constexpr (!std::is_void_v<std::invoke_result_t<F, T&>>)
			{
				if (_XABORT_CODE(status))
				{
					// Unfortunately, actual function result is not recoverable in this case
					return std::invoke_result_t<F, T&>();
				}
			}

			// Touch memory if transaction failed without RETRY flag on the first attempt (TODO)
			if (!(status & _XABORT_RETRY))
			{
				reinterpret_cast<atomic_t<u8>*>(sptr)->fetch_add(0);
			}

			// Stage 2: try to lock reservation first
			_old = res.fetch_add(1);

			// Start lightened transaction (TODO: tweaking)
			while (!(_old & rsrv_unique_lock))
			{
#ifndef _MSC_VER
				__asm__ goto ("xbegin %l[retry];" ::: "memory" : retry);
#else
				status = _xbegin();

				if (status != _XBEGIN_STARTED) [[unlikely]]
				{
					goto retry;
				}
#endif
				if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
				{
					std::invoke(op, *sptr);
#ifndef _MSC_VER
					__asm__ volatile ("xend;" ::: "memory");
#else
					_xend();
#endif
					res += 127;
					res.notify_all();
					return;
				}
				else
				{
					if (auto result = std::invoke(op, *sptr))
					{
#ifndef _MSC_VER
						__asm__ volatile ("xend;" ::: "memory");
#else
						_xend();
#endif
						res += 127;
						res.notify_all();
						return result;
					}
					else
					{
#ifndef _MSC_VER
						__asm__ volatile ("xabort $1;" ::: "memory");
#else
						_xabort(1);
#endif
						return std::invoke_result_t<F, T&>();
					}
				}

				retry:
#ifndef _MSC_VER
				__asm__ volatile ("movl %%eax, %0;" : "=r" (status) :: "memory");
#endif
				if (!(status & _XABORT_RETRY)) [[unlikely]]
				{
					if constexpr (!std::is_void_v<std::invoke_result_t<F, T&>>)
					{
						if (_XABORT_CODE(status))
						{
							res -= 1;
							return std::invoke_result_t<F, T&>();
						}
					}

					break;
				}
			}

			// Stage 3: all failed, heavyweight fallback (see comments at the bottom)
			if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
			{
				return vm::reservation_op_internal(addr, [&]
				{
					std::invoke(op, *sptr);
					return true;
				});
			}
			else
			{
				auto result = std::invoke_result_t<F, T&>();

				vm::reservation_op_internal(addr, [&]
				{
					T buf = *sptr;

					if ((result = std::invoke(op, buf)))
					{
						*sptr = buf;
						return true;
					}
					else
					{
						return false;
					}
				});

				return result;
			}
		}

		// Perform heavyweight lock
		auto [res, rtime] = vm::reservation_lock(addr);

		// Write directly if the op cannot fail
		if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
		{
			{
				vm::writer_lock lock(addr);
				std::invoke(op, *sptr);
				res += 127;
			}

			res.notify_all();
			return;
		}
		else
		{
			// Make an operational copy of data (TODO: volatile storage?)
			auto result = std::invoke_result_t<F, T&>();

			{
				vm::writer_lock lock(addr);
				T buf = *sptr;

				if ((result = std::invoke(op, buf)))
				{
					// If operation succeeds, write the data back
					*sptr = buf;
					res.release(rtime + 128);
				}
				else
				{
					// Operation failed, no memory has been modified
					res.release(rtime);
					return std::invoke_result_t<F, T&>();
				}
			}

			res.notify_all();
			return result;
		}
	}

	// For internal usage
	void reservation_escape_internal();

	// Read memory value in pseudo-atomic manner
	template <typename CPU, typename T, typename AT = u32, typename F>
	SAFE_BUFFERS inline auto reservation_peek(CPU&& cpu, _ptr_base<T, AT> ptr, F op)
	{
		// Atomic operation will be performed on aligned 128 bytes of data, so the data size and alignment must comply
		static_assert(sizeof(T) <= 128 && alignof(T) == sizeof(T), "vm::reservation_peek: unsupported type");

		// Use "super" pointer to prevent access violation handling during atomic op
		const auto sptr = vm::get_super_ptr<const T>(static_cast<u32>(ptr.addr()));

		// Use 128-byte aligned addr
		const u32 addr = static_cast<u32>(ptr.addr()) & -128;

		while (true)
		{
			if constexpr (std::is_class_v<std::remove_cvref_t<CPU>>)
			{
				if (cpu.test_stopped())
				{
					reservation_escape_internal();
				}
			}

			const u64 rtime = vm::reservation_acquire(addr, 128);

			if (rtime & 127)
			{
				continue;
			}

			// Observe data non-atomically and make sure no reservation updates were made
			if constexpr (std::is_void_v<std::invoke_result_t<F, const T&>>)
			{
				std::invoke(op, *sptr);

				if (rtime == vm::reservation_acquire(addr, 128))
				{
					return;
				}
			}
			else
			{
				auto res = std::invoke(op, *sptr);

				if (rtime == vm::reservation_acquire(addr, 128))
				{
					return res;
				}
			}
		}
	}

	template <bool Ack = false, typename T, typename F>
	SAFE_BUFFERS inline auto reservation_light_op(T& data, F op)
	{
		// Optimized real ptr -> vm ptr conversion, simply UB if out of range
		const u32 addr = static_cast<u32>(reinterpret_cast<const u8*>(&data) - g_base_addr);

		// Use "super" pointer to prevent access violation handling during atomic op
		const auto sptr = vm::get_super_ptr<T>(addr);

		// "Lock" reservation
		auto& res = vm::reservation_acquire(addr, 128);

		if (res.fetch_add(1) & vm::rsrv_unique_lock) [[unlikely]]
		{
			vm::reservation_shared_lock_internal(res);
		}

		if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
		{
			std::invoke(op, *sptr);
			res += 127;

			if constexpr (Ack)
			{
				res.notify_all();
			}
		}
		else
		{
			auto result = std::invoke(op, *sptr);
			res += 127;

			if constexpr (Ack)
			{
				res.notify_all();
			}

			return result;
		}
	}
} // namespace vm
