#pragma once

#include "Utilities/mutex.h"
#include "Utilities/sema.h"

#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"
#include "Emu/system_config.h"

#include <deque>
#include <thread>
#include <string_view>

// attr_protocol (waiting scheduling policy)
enum lv2_protocol : u32
{
	SYS_SYNC_FIFO                = 0x1, // First In, First Out Order
	SYS_SYNC_PRIORITY            = 0x2, // Priority Order
	SYS_SYNC_PRIORITY_INHERIT    = 0x3, // Basic Priority Inheritance Protocol
	SYS_SYNC_RETRY               = 0x4, // Not selected while unlocking
};

enum : u32
{
	SYS_SYNC_ATTR_PROTOCOL_MASK  = 0xf,
};

// attr_recursive (recursive locks policy)
enum
{
	SYS_SYNC_RECURSIVE           = 0x10,
	SYS_SYNC_NOT_RECURSIVE       = 0x20,
	SYS_SYNC_ATTR_RECURSIVE_MASK = 0xf0,
};

// attr_pshared (sharing among processes policy)
enum
{
	SYS_SYNC_PROCESS_SHARED      = 0x100,
	SYS_SYNC_NOT_PROCESS_SHARED  = 0x200,
	SYS_SYNC_ATTR_PSHARED_MASK   = 0xf00,
};

// attr_flags (creation policy)
enum
{
	SYS_SYNC_NEWLY_CREATED       = 0x1, // Create new object, fails if specified IPC key exists
	SYS_SYNC_NOT_CREATE          = 0x2, // Reference existing object, fails if IPC key not found
	SYS_SYNC_NOT_CARE            = 0x3, // Reference existing object, create new one if IPC key not found
	SYS_SYNC_ATTR_FLAGS_MASK     = 0xf,
};

// attr_adaptive
enum
{
	SYS_SYNC_ADAPTIVE            = 0x1000,
	SYS_SYNC_NOT_ADAPTIVE        = 0x2000,
	SYS_SYNC_ATTR_ADAPTIVE_MASK  = 0xf000,
};

// Base class for some kernel objects (shared set of 8192 objects).
struct lv2_obj
{
	using id_type = lv2_obj;

	static const u32 id_step = 0x100;
	static const u32 id_count = 8192;
	static constexpr std::pair<u32, u32> id_invl_range = {0, 8};

private:
	enum thread_cmd : s32
	{
		yield_cmd = INT32_MIN,
		enqueue_cmd,
	};

	// Function executed under IDM mutex, error will make the object creation fail and the error will be returned
	CellError on_id_create()
	{
		return {};
	}

public:

	static std::string name64(u64 name_u64)
	{
		const auto ptr = reinterpret_cast<const char*>(&name_u64);

		// NTS string, ignore invalid/newline characters
		// Example: "lv2\n\0tx" will be printed as "lv2"
		std::string str{ptr, std::find(ptr, ptr + 7, '\0')};
		str.erase(std::remove_if(str.begin(), str.end(), [](uchar c){ return !std::isprint(c); }), str.end());

		return str;
	};

	// Find and remove the object from the container (deque or vector)
	template <typename T, typename E>
	static T* unqueue(std::deque<T*>& queue, E* object)
	{
		for (auto found = queue.cbegin(), end = queue.cend(); found != end; found++)
		{
			if (*found == object)
			{
				queue.erase(found);
				return static_cast<T*>(object);
			}
		}

		return nullptr;
	}

	template <typename E, typename T>
	static T* schedule(std::deque<T*>& queue, u32 protocol)
	{
		if (queue.empty())
		{
			return nullptr;
		}

		if (protocol == SYS_SYNC_FIFO)
		{
			const auto res = queue.front();
			queue.pop_front();
			return res;
		}

		s32 prio = 3071;
		auto it = queue.cbegin();

		for (auto found = it, end = queue.cend(); found != end; found++)
		{
			const s32 _prio = static_cast<E*>(*found)->prio;

			if (_prio < prio)
			{
				it = found;
				prio = _prio;
			}
		}

		const auto res = *it;
		queue.erase(it);
		return res;
	}

private:
	// Remove the current thread from the scheduling queue, register timeout
	static void sleep_unlocked(cpu_thread&, u64 timeout);

	// Schedule the thread
	static bool awake_unlocked(cpu_thread*, s32 prio = enqueue_cmd);

public:
	static constexpr u64 max_timeout = UINT64_MAX / 1000;

	static void sleep(cpu_thread& cpu, const u64 timeout = 0);

	static bool awake(cpu_thread* const thread, s32 prio = enqueue_cmd);

	// Returns true on successful context switch, false otherwise
	static bool yield(cpu_thread& thread);

	static void set_priority(cpu_thread& thread, s32 prio)
	{
		ensure(prio + 512u < 3712);
		awake(&thread, prio);
	}

	static inline void awake_all()
	{
		awake({});
		g_to_awake.clear();
	}

	static inline void append(cpu_thread* const thread)
	{
		g_to_awake.emplace_back(thread);
	}

	static void cleanup();

	template <typename T, typename F>
	static error_code create(u32 pshared, u64 ipc_key, s32 flags, F&& make, bool key_not_zero = true)
	{
		switch (pshared)
		{
		case SYS_SYNC_PROCESS_SHARED:
		{
			if (key_not_zero && ipc_key == 0)
			{
				return CELL_EINVAL;
			}

			switch (flags)
			{
			case SYS_SYNC_NEWLY_CREATED:
			case SYS_SYNC_NOT_CARE:
			{
				std::shared_ptr<T> result = make();

				CellError error{};

				if (!ipc_manager<T, u64>::add(ipc_key, [&]()
				{
					if (!idm::import<lv2_obj, T>([&]()
					{
						if (result && (error = result->on_id_create()))
							result.reset();
						return result;
					}))
					{
						result.reset();
					}

					return result;
				}, &result))
				{
					if (error)
					{
						return error;
					}

					if (flags == SYS_SYNC_NEWLY_CREATED)
					{
						return CELL_EEXIST;
					}

					error = CELL_EAGAIN;

					if (!idm::import<lv2_obj, T>([&]() { if (result && (error = result->on_id_create())) result.reset(); return std::move(result); }))
					{
						return error;
					}

					return CELL_OK;
				}
				else if (!result)
				{
					return error ? CELL_EAGAIN : error;
				}
				else
				{
					return CELL_OK;
				}
			}
			case SYS_SYNC_NOT_CREATE:
			{
				auto result = ipc_manager<T, u64>::get(ipc_key);

				if (!result)
				{
					return CELL_ESRCH;
				}

				CellError error = CELL_EAGAIN;

				if (!idm::import<lv2_obj, T>([&]() { if (result && (error = result->on_id_create())) result.reset(); return std::move(result); }))
				{
					return error;
				}

				return CELL_OK;
			}
			default:
			{
				return CELL_EINVAL;
			}
			}
		}
		case SYS_SYNC_NOT_PROCESS_SHARED:
		{
			std::shared_ptr<T> result = make();

			CellError error = CELL_EAGAIN;

			if (!idm::import<lv2_obj, T>([&]() { if (result && (error = result->on_id_create())) result.reset(); return std::move(result); }))
			{
				return error;
			}

			return CELL_OK;
		}
		default:
		{
			return CELL_EINVAL;
		}
		}
	}

	template <bool IsUsleep = false, bool Scale = true>
	static bool wait_timeout(u64 usec, cpu_thread* const cpu = {})
	{
		static_assert(UINT64_MAX / max_timeout >= 100, "max timeout is not valid for scaling");

		if constexpr (Scale)
		{
			// Scale time
			usec = std::min<u64>(usec, UINT64_MAX / 100) * 100 / g_cfg.core.clocks_scale;
		}

		// Clamp
		usec = std::min<u64>(usec, max_timeout);

		extern u64 get_system_time();

		u64 passed = 0;
		u64 remaining;

		const u64 start_time = get_system_time();

		auto wait_for = [cpu](u64 timeout)
		{
			atomic_bs_t<cpu_flag> dummy{};
			auto& state = cpu ? cpu->state : dummy;
			const auto old = +state;

			if (old & cpu_flag::signal)
			{
				return true;
			}

			thread_ctrl::wait_on(state, old, timeout);
			return false;
		};

		while (usec >= passed)
		{
			remaining = usec - passed;
#ifdef __linux__
			// NOTE: Assumption that timer initialization has succeeded
			u64 host_min_quantum = IsUsleep && remaining <= 1000 ? 10 : 50;
#else
			// Host scheduler quantum for windows (worst case)
			// NOTE: On ps3 this function has very high accuracy
			constexpr u64 host_min_quantum = 500;
#endif
			// TODO: Tune for other non windows operating sytems
			bool escape = false;
			if (g_cfg.core.sleep_timers_accuracy < (IsUsleep ? sleep_timers_accuracy_level::_usleep : sleep_timers_accuracy_level::_all_timers))
			{
				escape = wait_for(remaining);
			}
			else
			{
				if (remaining > host_min_quantum)
				{
#ifdef __linux__
					// Do not wait for the last quantum to avoid loss of accuracy
					escape = wait_for(remaining - ((remaining % host_min_quantum) + host_min_quantum));
#else
					// Wait on multiple of min quantum for large durations to avoid overloading low thread cpus
					escape = wait_for(remaining - (remaining % host_min_quantum));
#endif
				}
				else
				{
					// Try yielding. May cause long wake latency but helps weaker CPUs a lot by alleviating resource pressure
					std::this_thread::yield();
				}
			}

			if (auto cpu = get_current_cpu_thread(); cpu && cpu->is_stopped())
			{
				break;
			}

			if (thread_ctrl::state() == thread_state::aborting)
			{
				return false;
			}

			if (escape)
			{
				return false;
			}

			passed = get_system_time() - start_time;
		}

		return true;
	}

private:
	// Scheduler mutex
	static shared_mutex g_mutex;

	// Pending list of threads to run
	static thread_local std::vector<class cpu_thread*> g_to_awake;

	// Scheduler queue for active PPU threads
	static std::deque<class ppu_thread*> g_ppu;

	// Waiting for the response from
	static std::deque<class cpu_thread*> g_pending;

	// Scheduler queue for timeouts (wait until -> thread)
	static std::deque<std::pair<u64, class cpu_thread*>> g_waiting;

	static void schedule_all();
};
