#pragma once

#include "Utilities/mutex.h"
#include "Utilities/sema.h"

#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"
#include "Emu/system_config.h"

#include <thread>

// attr_protocol (waiting scheduling policy)
enum lv2_protocol : u8
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

enum ppu_thread_status : u32;

// Base class for some kernel objects (shared set of 8192 objects).
struct lv2_obj
{
	static const u32 id_step = 0x100;
	static const u32 id_count = 8192;
	static constexpr std::pair<u32, u32> id_invl_range = {0, 8};

private:
	enum thread_cmd : s32
	{
		yield_cmd = smin,
		enqueue_cmd,
	};

	// Function executed under IDM mutex, error will make the object creation fail and the error will be returned
	CellError on_id_create()
	{
		exists++;
		return {};
	}

public:
	SAVESTATE_INIT_POS(4); // Dependency on PPUs

	// Existence validation (workaround for shared-ptr ref-counting)
	atomic_t<u32> exists = 0;

	template <typename Ptr>
	static bool check(Ptr&& ptr)
	{
		return ptr && ptr->exists;
	}

	static std::string name64(u64 name_u64)
	{
		const auto ptr = reinterpret_cast<const char*>(&name_u64);

		// NTS string, ignore invalid/newline characters
		// Example: "lv2\n\0tx" will be printed as "lv2"
		std::string str{ptr, std::find(ptr, ptr + 7, '\0')};
		str.erase(std::remove_if(str.begin(), str.end(), [](uchar c){ return !std::isprint(c); }), str.end());

		return str;
	}

	// Find and remove the object from the linked list
	template <typename T>
	static T* unqueue(T*& first, T* object, T* T::* mem_ptr = &T::next_cpu)
	{
		auto it = +first;

		if (it == object)
		{
			atomic_storage<T*>::release(first, it->*mem_ptr);
			atomic_storage<T*>::release(it->*mem_ptr, nullptr);
			return it;
		}

		for (; it;)
		{
			const auto next = it->*mem_ptr + 0;

			if (next == object)
			{
				atomic_storage<T*>::release(it->*mem_ptr, next->*mem_ptr);
				atomic_storage<T*>::release(next->*mem_ptr, nullptr);
				return next;
			}

			it = next;
		}

		return {};
	}

	// Remove an object from the linked set according to the protocol
	template <typename E, typename T>
	static E* schedule(T& first, u32 protocol)
	{
		auto it = static_cast<E*>(first);

		if (!it)
		{
			return it;
		}

		auto parent_found = &first;

		if (protocol == SYS_SYNC_FIFO)
		{
			while (true)
			{
				const auto next = +it->next_cpu;

				if (next)
				{
					parent_found = &it->next_cpu;
					it = next;
					continue;
				}

				if (it && cpu_flag::again - it->state)
				{
					atomic_storage<T>::release(*parent_found, nullptr);
				}

				return it;
			}
		}

		s32 prio = it->prio;
		auto found = it;

		while (true)
		{
			auto& node = it->next_cpu;
			const auto next = static_cast<E*>(node);

			if (!next)
			{
				break;
			}

			const s32 _prio = static_cast<E*>(next)->prio;

			// This condition tests for equality as well so the eraliest element to be pushed is popped
			if (_prio <= prio)
			{
				found = next;
				parent_found = &node;
				prio = _prio;
			}

			it = next;
		}

		if (cpu_flag::again - found->state)
		{
			atomic_storage<T>::release(*parent_found, found->next_cpu);
			atomic_storage<T>::release(found->next_cpu, nullptr);
		}

		return found;
	}

	template <typename T>
	static void emplace(T& first, T object)
	{
		atomic_storage<T>::release(object->next_cpu, first);
		atomic_storage<T>::release(first, object);
	}

private:
	// Remove the current thread from the scheduling queue, register timeout
	static bool sleep_unlocked(cpu_thread&, u64 timeout, u64 current_time);

	// Schedule the thread
	static bool awake_unlocked(cpu_thread*, s32 prio = enqueue_cmd);

public:
	static constexpr u64 max_timeout = u64{umax} / 1000;

	static bool sleep(cpu_thread& cpu, const u64 timeout = 0);

	static bool awake(cpu_thread* thread, s32 prio = enqueue_cmd);

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

	static void make_scheduler_ready();

	static ppu_thread_status ppu_state(ppu_thread* ppu, bool lock_idm = true, bool lock_lv2 = true);

	static inline void append(cpu_thread* const thread)
	{
		g_to_awake.emplace_back(thread);
	}

	// Serialization related
	static void set_future_sleep(cpu_thread* cpu);
	static bool is_scheduler_ready();

	// Must be called under IDM lock
	static bool has_ppus_in_running_state();

	static void set_yield_frequency(u64 freq, u64 max_allowed_tsx);

	static void cleanup();

	template <typename T>
	static inline u64 get_key(const T& attr)
	{
		return (attr.pshared == SYS_SYNC_PROCESS_SHARED ? +attr.ipc_key : 0);
	}

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
			case SYS_SYNC_NOT_CREATE:
			{
				break;
			}
			default: return CELL_EINVAL;
			}

			break;
		}
		case SYS_SYNC_NOT_PROCESS_SHARED:
		{
			break;
		}
		default: return CELL_EINVAL;
		}

		// EAGAIN for IDM IDs shortage
		CellError error = CELL_EAGAIN;

		if (!idm::import<lv2_obj, T>([&]() -> std::shared_ptr<T>
		{
			std::shared_ptr<T> result = make();

			auto finalize_construct = [&]() -> std::shared_ptr<T>
			{
				if ((error = result->on_id_create()))
				{
					result.reset();
				}

				return std::move(result);
			};

			if (pshared != SYS_SYNC_PROCESS_SHARED)
			{
				// Creation of unique (non-shared) object handle
				return finalize_construct();
			}

			auto& ipc_container = g_fxo->get<ipc_manager<T, u64>>();

			if (flags == SYS_SYNC_NOT_CREATE)
			{
				result = ipc_container.get(ipc_key);

				if (!result)
				{
					error = CELL_ESRCH;
					return result;
				}

				// Run on_id_create() on existing object
				return finalize_construct();
			}

			bool added = false;
			std::tie(added, result) = ipc_container.add(ipc_key, finalize_construct, flags != SYS_SYNC_NEWLY_CREATED);

			if (!added)
			{
				if (flags == SYS_SYNC_NEWLY_CREATED)
				{
					// Object already exists but flags does not allow it
					error = CELL_EEXIST;

					// We specified we do not want to peek pointer's value, result must be empty
					AUDIT(!result);
					return result;
				}

				// Run on_id_create() on existing object
				return finalize_construct();
			}

			return result;
		}))
		{
			return error;
		}

		return CELL_OK;
	}

	template <typename T>
	static void on_id_destroy(T& obj, u64 ipc_key, u64 pshared = -1)
	{
		if (pshared == umax)
		{
			// Default is to check key
			pshared = ipc_key != 0;
		}

		if (obj.exists-- == 1u && pshared)
		{
			g_fxo->get<ipc_manager<T, u64>>().remove(ipc_key);
		}
	}

	template <typename T>
	static std::shared_ptr<T> load(u64 ipc_key, std::shared_ptr<T> make, u64 pshared = -1)
	{
		if (pshared == umax ? ipc_key != 0 : pshared != 0)
		{
			g_fxo->need<ipc_manager<T, u64>>();

			make = g_fxo->get<ipc_manager<T, u64>>().add(ipc_key, [&]()
			{
				return make;
			}, true).second;
		}

		// Ensure no error
		ensure(!make->on_id_create());
		return make;
	}

	static bool wait_timeout(u64 usec, ppu_thread* cpu = {}, bool scale = true, bool is_usleep = false);

	static inline void notify_all()
	{
		for (auto cpu : g_to_notify)
		{
			if (!cpu)
			{
				break;
			}

			if (cpu != &g_to_notify)
			{
				// Note: by the time of notification the thread could have been deallocated which is why the direct function is used
				// TODO: Pass a narrower mask
				atomic_wait_engine::notify_one(cpu, 4, atomic_wait::default_mask<atomic_bs_t<cpu_flag>>);
			}
		}

		g_to_notify[0] = nullptr;
		g_postpone_notify_barrier = false;
	}

	// Can be called before the actual sleep call in order to move it out of mutex scope
	static void prepare_for_sleep(cpu_thread& cpu);

	struct notify_all_t
	{
		notify_all_t() noexcept
		{
			g_postpone_notify_barrier = true;
		}

		notify_all_t(const notify_all_t&) = delete;

		static void cleanup()
		{
			for (auto& cpu : g_to_notify)
			{
				if (!cpu)
				{
					return;
				}

				// While IDM mutex is still locked (this function assumes so) check if the notification is still needed
				// Pending flag is meant for forced notification (if the CPU really has pending work it can restore the flag in theory)
				if (cpu != &g_to_notify && static_cast<const decltype(cpu_thread::state)*>(cpu)->none_of(cpu_flag::signal + cpu_flag::pending))
				{
					// Omit it (this is a void pointer, it can hold anything)
					cpu = &g_to_notify;
				}
			}
		}

		~notify_all_t() noexcept
		{
			lv2_obj::notify_all();
		}
	};

	// Scheduler mutex
	static shared_mutex g_mutex;

private:
	// Pending list of threads to run
	static thread_local std::vector<class cpu_thread*> g_to_awake;

	// Scheduler queue for active PPU threads
	static class ppu_thread* g_ppu;

	// Waiting for the response from
	static u32 g_pending;

	// Pending list of threads to notify (cpu_thread::state ptr)
	static thread_local std::add_pointer_t<const void> g_to_notify[4];

	// If a notify_all_t object exists locally, postpone notifications to the destructor of it (not recursive, notifies on the first destructor for safety)
	static thread_local bool g_postpone_notify_barrier;

	static void schedule_all(u64 current_time = 0);
};
