#pragma once

#include "Utilities/mutex.h"

#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "util/shared_ptr.hpp"

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

struct ppu_non_sleeping_count_t
{
	bool has_running; // no actual count for optimization sake
	u32 onproc_count;
};

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

	lv2_obj() noexcept = default;
	lv2_obj(u32 i) noexcept : exists{ i } {}
	lv2_obj(lv2_obj&& rhs) noexcept : exists{ +rhs.exists } {}
	lv2_obj(utils::serial&) noexcept {}
	lv2_obj& operator=(lv2_obj&& rhs) noexcept { exists = +rhs.exists; return *this; }
	void save(utils::serial&) {}

	// Existence validation (workaround for shared-ptr ref-counting)
	atomic_t<u32> exists = 0;

	template <typename Ptr>
	static bool check(Ptr&& ptr)
	{
		return ptr && ptr->exists;
	}

	// wrapper for name64 string formatting
	struct name_64
	{
		u64 data;
	};

	static std::string name64(u64 name_u64);

	// Find and remove the object from the linked list
	template <bool ModifyNode = true, typename T>
	static T* unqueue(T*& first, T* object, T* T::* mem_ptr = &T::next_cpu)
	{
		auto it = +first;

		if (it == object)
		{
			atomic_storage<T*>::release(first, it->*mem_ptr);

			if constexpr (ModifyNode)
			{
				atomic_storage<T*>::release(it->*mem_ptr, nullptr);
			}

			return it;
		}

		for (; it;)
		{
			const auto next = it->*mem_ptr + 0;

			if (next == object)
			{
				atomic_storage<T*>::release(it->*mem_ptr, next->*mem_ptr);

				if constexpr (ModifyNode)
				{
					atomic_storage<T*>::release(next->*mem_ptr, nullptr);
				}

				return next;
			}

			it = next;
		}

		return {};
	}

	// Remove an object from the linked set according to the protocol
	template <typename E, typename T>
	static E* schedule(T& first, u32 protocol, bool modify_node = true)
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

				if (cpu_flag::again - it->state)
				{
					atomic_storage<T>::release(*parent_found, nullptr);
				}

				return it;
			}
		}

		auto prio = it->prio.load();
		auto found = it;

		while (true)
		{
			auto& node = it->next_cpu;
			const auto next = static_cast<E*>(node);

			if (!next)
			{
				break;
			}

			const auto _prio = static_cast<E*>(next)->prio.load();

			// This condition tests for equality as well so the earliest element to be pushed is popped
			if (_prio.prio < prio.prio || (_prio.prio == prio.prio && _prio.order < prio.order))
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

			if (modify_node)
			{
				atomic_storage<T>::release(found->next_cpu, nullptr);
			}
		}

		return found;
	}

	template <typename T>
	static void emplace(T& first, T object)
	{
		atomic_storage<T>::release(object->next_cpu, first);
		atomic_storage<T>::release(first, object);

		object->prio.atomic_op([order = ++g_priority_order_tag](std::common_type_t<decltype(std::declval<T>()->prio.load())>& prio)
		{
			if constexpr (requires { +std::declval<decltype(prio)>().preserve_bit; } )
			{
				if (prio.preserve_bit)
				{
					// Restoring state on load
					prio.preserve_bit = 0;
					return;
				}
			}

			prio.order = order;
		});
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

	static std::pair<ppu_thread_status, u32> ppu_state(ppu_thread* ppu, bool lock_idm = true, bool lock_lv2 = true);

	static inline void append(cpu_thread* const thread)
	{
		g_to_awake.emplace_back(thread);
	}

	// Serialization related
	static void set_future_sleep(cpu_thread* cpu);
	static bool is_scheduler_ready();

	// Must be called under IDM lock
	static ppu_non_sleeping_count_t count_non_sleeping_threads();

	static inline bool has_ppus_in_running_state() noexcept
	{
		return count_non_sleeping_threads().has_running != 0;
	}

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

		if (!idm::import<lv2_obj, T>([&]() -> shared_ptr<T>
		{
			shared_ptr<T> result = make();

			auto finalize_construct = [&]() -> shared_ptr<T>
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
	static void on_id_destroy(T& obj, u64 ipc_key, u64 pshared = umax)
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
	static shared_ptr<T> load(u64 ipc_key, shared_ptr<T> make, u64 pshared = umax)
	{
		if (pshared == umax ? ipc_key != 0 : pshared != 0)
		{
			g_fxo->need<ipc_manager<T, u64>>();

			g_fxo->get<ipc_manager<T, u64>>().add(ipc_key, [&]()
			{
				return make;
			});
		}

		// Ensure no error
		ensure(!make->on_id_create());
		return make;
	}

	template <typename T, typename Storage = lv2_obj>
	static std::function<void(void*)> load_func(shared_ptr<T> make, u64 pshared = umax)
	{
		const u64 key = make->key;
		return [ptr = load<T>(key, make, pshared)](void* storage) { *static_cast<atomic_ptr<Storage>*>(storage) = ptr; };
	}

	static bool wait_timeout(u64 usec, ppu_thread* cpu = {}, bool scale = true, bool is_usleep = false);

	static void notify_all() noexcept;

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
				// Disabled to allow reservation notifications from here
				if (false && cpu != &g_to_notify && static_cast<const decltype(cpu_thread::state)*>(cpu)->none_of(cpu_flag::signal + cpu_flag::pending))
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

	// Proirity tags
	static atomic_t<u64> g_priority_order_tag;

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
