#pragma once

#include "Utilities/types.h"
#include "Utilities/mutex.h"

#include <memory>
#include <vector>

// Helper namespace
namespace id_manager
{
	// Common global mutex
	extern shared_mutex g_mutex;

	// ID traits
	template <typename T, typename = void>
	struct id_traits
	{
		static_assert(sizeof(T) == 0, "ID object must specify: id_base, id_step, id_count");

		static const u32 base    = 1;     // First ID (N = 0)
		static const u32 step    = 1;     // Any ID: N * id_step + id_base
		static const u32 count   = 65535; // Limit: N < id_count
		static const u32 invalid = 0;
	};

	template <typename T>
	struct id_traits<T, void_t<decltype(&T::id_base), decltype(&T::id_step), decltype(&T::id_count)>>
	{
		static const u32 base    = T::id_base;
		static const u32 step    = T::id_step;
		static const u32 count   = T::id_count;
		static const u32 invalid = base > 0 ? 0 : -1;

		static_assert(u64{step} * count + base < UINT32_MAX, "ID traits: invalid object range");
	};

	// Optional object initialization function (called after ID registration)
	template <typename T, typename = void>
	struct on_init
	{
		static inline void func(T*, const std::shared_ptr<void>&)
		{
			// Forbid forward declarations
			static constexpr auto size = sizeof(std::conditional_t<std::is_void<T>::value, void*, T>);
		}
	};

	template <typename T>
	struct on_init<T, decltype(std::declval<T>().on_init(std::declval<const std::shared_ptr<void>&>()))>
	{
		static inline void func(T* ptr, const std::shared_ptr<void>& _ptr)
		{
			if (ptr) ptr->on_init(_ptr);
		}
	};

	// Optional object finalization function (called after ID removal)
	template <typename T, typename = void>
	struct on_stop
	{
		static inline void func(T*)
		{
			// Forbid forward declarations
			static constexpr auto size = sizeof(std::conditional_t<std::is_void<T>::value, void*, T>);
		}
	};

	template <typename T>
	struct on_stop<T, decltype(std::declval<T>().on_stop())>
	{
		static inline void func(T* ptr)
		{
			if (ptr) ptr->on_stop();
		}
	};

	// Correct usage testing
	template <typename T, typename T2, typename = void>
	struct id_verify : std::integral_constant<bool, std::is_base_of<T, T2>::value>
	{
		// If common case, T2 shall be derived from or equal to T
	};

	template <typename T, typename T2>
	struct id_verify<T, T2, void_t<typename T2::id_type>> : std::integral_constant<bool, std::is_same<T, typename T2::id_type>::value>
	{
		// If T2 contains id_type type, T must be equal to it
	};

	class typeinfo
	{
		// Global variable for each registered type
		template <typename T>
		struct registered
		{
			static const u32 index;
		};

		// Increment type counter
		static u32 add_type(u32 i)
		{
			static atomic_t<u32> g_next{0};

			return g_next.fetch_add(i);
		}

	public:
		// Get type index
		template <typename T>
		static inline u32 get_index()
		{
			return registered<T>::index;
		}

		// Get type count
		static inline u32 get_count()
		{
			return add_type(0);
		}

		// Get type finalizer
		template <typename T>
		static inline auto get_stop()
		{
			return [](void* ptr) -> void
			{
				return id_manager::on_stop<T>::func(static_cast<T*>(ptr));
			};
		}
	};

	template <typename T>
	const u32 typeinfo::registered<T>::index = typeinfo::add_type(1);

	// ID value with additional type stored
	class id_key
	{
		u32 m_value;           // ID value
		u32 m_type;            // True object type
		void (*m_stop)(void*); // Finalizer

	public:
		id_key() = default;

		id_key(u32 value, u32 type, void (*stop)(void*))
			: m_value(value)
			, m_type(type)
			, m_stop(stop)
		{
		}

		u32 value() const
		{
			return m_value;
		}

		u32 type() const
		{
			return m_type;
		}

		auto on_stop() const
		{
			return m_stop;
		}

		operator u32() const
		{
			return m_value;
		}
	};

	using id_map = std::vector<std::pair<id_key, std::shared_ptr<void>>>;
}

// Object manager for emulated process. Multiple objects of specified arbitrary type are given unique IDs.
class idm
{
	// Last allocated ID for constructors
	static thread_local u32 g_id;

	// Type Index -> ID -> Object. Use global since only one process is supported atm.
	static std::vector<id_manager::id_map> g_map;

	template <typename T>
	static inline u32 get_type()
	{
		return id_manager::typeinfo::get_index<T>();
	}

	template <typename T>
	static constexpr u32 get_index(u32 id)
	{
		return (id - id_manager::id_traits<T>::base) / id_manager::id_traits<T>::step;
	}

	// Helper
	template <typename F>
	struct function_traits;

	template <typename F, typename R, typename A1, typename A2>
	struct function_traits<R (F::*)(A1, A2&) const>
	{
		using object_type = A2;
		using result_type = R;
	};

	template <typename F, typename R, typename A1, typename A2>
	struct function_traits<R (F::*)(A1, A2&)>
	{
		using object_type = A2;
		using result_type = R;
	};

	template <typename F, typename A1, typename A2>
	struct function_traits<void (F::*)(A1, A2&) const>
	{
		using object_type = A2;
		using void_type   = void;
	};

	template <typename F, typename A1, typename A2>
	struct function_traits<void (F::*)(A1, A2&)>
	{
		using object_type = A2;
		using void_type   = void;
	};

	// Helper type: pointer + return value propagated
	template <typename T, typename RT>
	struct return_pair
	{
		std::shared_ptr<T> ptr;
		RT ret;

		explicit operator bool() const
		{
			return ptr.operator bool();
		}

		T* operator->() const
		{
			return ptr.get();
		}
	};

	// Unsafe specialization (not refcounted)
	template <typename T, typename RT>
	struct return_pair<T*, RT>
	{
		T* ptr;
		RT ret;

		explicit operator bool() const
		{
			return ptr != nullptr;
		}

		T* operator->() const
		{
			return ptr;
		}
	};

	// Prepare new ID (returns nullptr if out of resources)
	static id_manager::id_map::pointer allocate_id(const id_manager::id_key& info, u32 base, u32 step, u32 count);

	// Find ID (additionally check type if types are not equal)
	template <typename T, typename Type>
	static id_manager::id_map::pointer find_id(u32 id)
	{
		static_assert(id_manager::id_verify<T, Type>::value, "Invalid ID type combination");

		const u32 index = get_index<Type>(id);

		auto& vec = g_map[get_type<T>()];

		if (index >= vec.size() || index >= id_manager::id_traits<Type>::count)
		{
			return nullptr;
		}

		auto& data = vec[index];

		if (data.second)
		{
			if (std::is_same<T, Type>::value || data.first.type() == get_type<Type>())
			{
				return &data;
			}
		}

		return nullptr;
	}

	// Allocate new ID and assign the object from the provider()
	template <typename T, typename Type, typename F>
	static id_manager::id_map::pointer create_id(F&& provider)
	{
		static_assert(id_manager::id_verify<T, Type>::value, "Invalid ID type combination");

		// ID info
		const id_manager::id_key info{get_type<T>(), get_type<Type>(), id_manager::typeinfo::get_stop<Type>()};

		// ID traits
		using traits = id_manager::id_traits<Type>;

		// Allocate new id
		writer_lock lock(id_manager::g_mutex);

		if (auto* place = allocate_id(info, traits::base, traits::step, traits::count))
		{
			// Get object, store it
			place->second = provider();

			if (place->second)
			{
				return place;
			}
		}

		return nullptr;
	}

public:
	// Initialize object manager
	static void init();

	// Remove all objects
	static void clear();

	// Get last ID (updated in create_id/allocate_id)
	static inline u32 last_id()
	{
		return g_id;
	}

	// Add a new ID of specified type with specified constructor arguments (returns object or nullptr)
	template <typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<Make>> make_ptr(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return std::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			id_manager::on_init<Make>::func(static_cast<Make*>(pair->second.get()), pair->second);
			return {pair->second, static_cast<Make*>(pair->second.get())};
		}

		return nullptr;
	}

	// Add a new ID of specified type with specified constructor arguments (returns id)
	template <typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, u32> make(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return std::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			id_manager::on_init<Make>::func(static_cast<Make*>(pair->second.get()), pair->second);
			return pair->first;
		}

		return id_manager::id_traits<Make>::invalid;
	}

	// Add a new ID for an existing object provided (returns new id)
	template <typename T, typename Made = T>
	static inline u32 import_existing(const std::shared_ptr<T>& ptr)
	{
		if (auto pair = create_id<T, Made>([&] { return ptr; }))
		{
			id_manager::on_init<Made>::func(static_cast<Made*>(pair->second.get()), pair->second);
			return pair->first;
		}

		return id_manager::id_traits<Made>::invalid;
	}

	// Add a new ID for an object returned by provider()
	template <typename T, typename Made = T, typename F, typename = std::result_of_t<F()>>
	static inline u32 import(F&& provider)
	{
		if (auto pair = create_id<T, Made>(std::forward<F>(provider)))
		{
			id_manager::on_init<Made>::func(static_cast<Made*>(pair->second.get()), pair->second);
			return pair->first;
		}

		return id_manager::id_traits<Made>::invalid;
	}

	// Access the ID record without locking (unsafe)
	template <typename T, typename Get = T>
	static inline id_manager::id_map::pointer find_unlocked(u32 id)
	{
		return find_id<T, Get>(id);
	}

	// Check the ID without locking (can be called from other method)
	template <typename T, typename Get = T>
	static inline Get* check_unlocked(u32 id)
	{
		if (const auto found = find_id<T, Get>(id))
		{
			return static_cast<Get*>(found->second.get());
		}

		return nullptr;
	}

	// Check the ID
	template <typename T, typename Get = T>
	static inline Get* check(u32 id)
	{
		reader_lock lock(id_manager::g_mutex);

		return check_unlocked<T, Get>(id);
	}

	// Check the ID, access object under shared lock
	template <typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(Get&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline Get* check(u32 id, F&& func, int = 0)
	{
		reader_lock lock(id_manager::g_mutex);

		if (const auto ptr = check_unlocked<T, Get>(id))
		{
			func(*ptr);
			return ptr;
		}
		
		return nullptr;
	}

	// Check the ID, access object under reader lock, propagate return value
	template <typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(Get&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline return_pair<Get*, FRT> check(u32 id, F&& func)
	{
		reader_lock lock(id_manager::g_mutex);

		if (const auto ptr = check_unlocked<T, Get>(id))
		{
			return {ptr, func(*ptr)};
		}

		return {nullptr};
	}

	// Get the object without locking (can be called from other method)
	template <typename T, typename Get = T>
	static inline std::shared_ptr<Get> get_unlocked(u32 id)
	{
		const auto found = find_id<T, Get>(id);

		if (UNLIKELY(found == nullptr))
		{
			return nullptr;
		}

		return {found->second, static_cast<Get*>(found->second.get())};
	}

	// Get the object
	template <typename T, typename Get = T>
	static inline std::shared_ptr<Get> get(u32 id)
	{
		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id<T, Get>(id);

		if (UNLIKELY(found == nullptr))
		{
			return nullptr;
		}

		return {found->second, static_cast<Get*>(found->second.get())};
	}

	// Get the object, access object under reader lock
	template <typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(Get&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline auto get(u32 id, F&& func, int = 0)
	{
		using result_type = std::shared_ptr<Get>;

		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id<T, Get>(id);

		if (UNLIKELY(found == nullptr))
		{
			return result_type{nullptr};
		}

		const auto ptr = static_cast<Get*>(found->second.get());

		func(*ptr);

		return result_type{found->second, ptr};
	}

	// Get the object, access object under reader lock, propagate return value
	template <typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(Get&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline auto get(u32 id, F&& func)
	{
		using result_type = return_pair<Get, FRT>;

		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id<T, Get>(id);

		if (UNLIKELY(found == nullptr))
		{
			return result_type{nullptr};
		}

		const auto ptr = static_cast<Get*>(found->second.get());

		return result_type{{found->second, ptr}, func(*ptr)};
	}

	// Access all objects of specified type. Returns the number of objects processed.
	template <typename T, typename Get = T, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::void_type>
	static inline u32 select(F&& func, int = 0)
	{
		static_assert(id_manager::id_verify<T, Get>::value, "Invalid ID type combination");

		reader_lock lock(id_manager::g_mutex);

		u32 result = 0;

		for (auto& id : g_map[get_type<T>()])
		{
			if (id.second)
			{
				if (std::is_same<T, Get>::value || id.first.type() == get_type<Get>())
				{
					func(id.first, *static_cast<typename function_traits<FT>::object_type*>(id.second.get()));
					result++;
				}
			}
		}	

		return result;
	}

	// Access all objects of specified type. If function result evaluates to true, stop and return the object and the value.
	template <typename T, typename Get = T, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::result_type>
	static inline auto select(F&& func)
	{
		static_assert(id_manager::id_verify<T, Get>::value, "Invalid ID type combination");

		using object_type = typename function_traits<FT>::object_type;
		using result_type = return_pair<object_type, FRT>;

		reader_lock lock(id_manager::g_mutex);

		for (auto& id : g_map[get_type<T>()])
		{
			if (auto ptr = static_cast<object_type*>(id.second.get()))
			{
				if (std::is_same<T, Get>::value || id.first.type() == get_type<Get>())
				{
					if (FRT result = func(id.first, *ptr))
					{
						return result_type{{id.second, ptr}, std::move(result)};
					}
				}
			}
		}

		return result_type{nullptr};
	}

	// Remove the ID
	template <typename T, typename Get = T>
	static inline explicit_bool_t remove(u32 id)
	{
		std::shared_ptr<void> ptr;
		{
			writer_lock lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id))
			{
				ptr = std::move(found->second);
			}
			else
			{
				return false;
			}
		}

		id_manager::on_stop<Get>::func(static_cast<Get*>(ptr.get()));
		return true;
	}

	// Remove the ID and return the object
	template <typename T, typename Get = T>
	static inline std::shared_ptr<Get> withdraw(u32 id)
	{
		std::shared_ptr<void> ptr;
		{
			writer_lock lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id))
			{
				ptr = std::move(found->second);
			}
			else
			{
				return nullptr;
			}
		}

		id_manager::on_stop<Get>::func(static_cast<Get*>(ptr.get()));
		return {ptr, static_cast<Get*>(ptr.get())};
	}

	// Remove the ID after accessing the object under writer lock, return the object and propagate return value
	template <typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(Get&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline auto withdraw(u32 id, F&& func, int = 0)
	{
		using result_type = std::shared_ptr<Get>;

		std::shared_ptr<void> ptr;
		{
			writer_lock lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id))
			{
				func(*static_cast<Get*>(found->second.get()));

				ptr = std::move(found->second);
			}
			else
			{
				return result_type{nullptr};
			}
		}

		id_manager::on_stop<Get>::func(static_cast<Get*>(ptr.get()));
		return result_type{ptr, static_cast<Get*>(ptr.get())};
	}

	// Conditionally remove the ID (if return value evaluates to false) after accessing the object under writer lock, return the object and propagate return value
	template <typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(Get&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline auto withdraw(u32 id, F&& func)
	{
		using result_type = return_pair<Get, FRT>;

		std::shared_ptr<void> ptr;
		FRT ret;
		{
			writer_lock lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id))
			{
				const auto _ptr = static_cast<Get*>(found->second.get());

				ret = func(*_ptr);

				if (ret)
				{
					return result_type{{found->second, _ptr}, std::move(ret)};
				}

				ptr = std::move(found->second);
			}
			else
			{
				return result_type{nullptr};
			}
		}

		id_manager::on_stop<Get>::func(static_cast<Get*>(ptr.get()));
		return result_type{{ptr, static_cast<Get*>(ptr.get())}, std::move(ret)};
	}
};

// Object manager for emulated process. One unique object per type, or zero.
class fxm
{
	// Type Index -> Object. Use global since only one process is supported atm.
	static std::vector<std::pair<void(*)(void*), std::shared_ptr<void>>> g_vec;

	template <typename T>
	static inline u32 get_type()
	{
		return id_manager::typeinfo::get_index<T>();
	}

public:
	// Initialize object manager
	static void init();
	
	// Remove all objects
	static void clear();

	// Create the object (returns nullptr if it already exists)
	template <typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> make(Args&&... args)
	{
		std::shared_ptr<T> ptr;
		{
			writer_lock lock(id_manager::g_mutex);

			auto& pair = g_vec[get_type<T>()];

			if (!pair.second)
			{
				ptr = std::make_shared<Make>(std::forward<Args>(args)...);

				pair.first = id_manager::typeinfo::get_stop<T>();
				pair.second = ptr;
			}
			else
			{
				return nullptr;
			}
		}

		id_manager::on_init<T>::func(ptr.get(), ptr);
		return ptr;
	}

	// Create the object unconditionally (old object will be removed if it exists)
	template <typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> make_always(Args&&... args)
	{
		std::shared_ptr<T> ptr;
		std::shared_ptr<void> old;
		{
			writer_lock lock(id_manager::g_mutex);

			auto& pair = g_vec[get_type<T>()];

			ptr = std::make_shared<Make>(std::forward<Args>(args)...);
			old = std::move(pair.second);

			pair.first = id_manager::typeinfo::get_stop<T>();
			pair.second = ptr;
		}

		if (old)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(old.get()));
		}

		id_manager::on_init<T>::func(ptr.get(), ptr);
		return ptr;
	}

	// Emplace the object returned by provider() and return it if no object exists
	template <typename T, typename F>
	static auto import(F&& provider) -> decltype(static_cast<std::shared_ptr<T>>(provider()))
	{
		std::shared_ptr<T> ptr;
		{
			writer_lock lock(id_manager::g_mutex);

			auto& pair = g_vec[get_type<T>()];

			if (!pair.second)
			{
				ptr = provider();

				if (ptr)
				{
					pair.first = id_manager::typeinfo::get_stop<T>();
					pair.second = ptr;
				}
			}

			if (!ptr)
			{
				return nullptr;
			}
		}

		id_manager::on_init<T>::func(ptr.get(), ptr);
		return ptr;
	}

	// Emplace the object return by provider() (old object will be removed if it exists)
	template <typename T, typename F>
	static auto import_always(F&& provider) -> decltype(static_cast<std::shared_ptr<T>>(provider()))
	{
		std::shared_ptr<T> ptr;
		std::shared_ptr<void> old;
		{
			writer_lock lock(id_manager::g_mutex);

			auto& pair = g_vec[get_type<T>()];

			ptr = provider();

			if (ptr)
			{
				old = std::move(pair.second);

				pair.first = id_manager::typeinfo::get_stop<T>();
				pair.second = ptr;
			}
			else
			{
				return nullptr;
			}
		}

		if (old)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(old.get()));
		}

		id_manager::on_init<T>::func(ptr.get(), ptr);
		return ptr;
	}

	// Get the object unconditionally (create an object if it doesn't exist)
	template <typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> get_always(Args&&... args)
	{
		std::shared_ptr<T> ptr;
		{
			writer_lock lock(id_manager::g_mutex);

			auto& pair = g_vec[get_type<T>()];

			if (auto& old = pair.second)
			{
				return {old, static_cast<T*>(old.get())};
			}
			else
			{
				ptr = std::make_shared<Make>(std::forward<Args>(args)...);

				pair.first = id_manager::typeinfo::get_stop<T>();
				pair.second = ptr;
			}
		}

		id_manager::on_init<T>::func(ptr.get(), ptr);
		return ptr;
	}

	// Unsafe version of check(), can be used in some cases
	template <typename T>
	static inline T* check_unlocked()
	{
		return static_cast<T*>(g_vec[get_type<T>()].second.get());
	}

	// Check whether the object exists
	template <typename T>
	static inline T* check()
	{
		reader_lock lock(id_manager::g_mutex);

		return check_unlocked<T>();
	}

	// Get the object (returns nullptr if it doesn't exist)
	template <typename T>
	static inline std::shared_ptr<T> get()
	{
		reader_lock lock(id_manager::g_mutex);

		auto& ptr = g_vec[get_type<T>()].second;

		return {ptr, static_cast<T*>(ptr.get())};
	}

	// Delete the object
	template <typename T>
	static inline explicit_bool_t remove()
	{
		std::shared_ptr<void> ptr;
		{
			writer_lock lock(id_manager::g_mutex);
			ptr = std::move(g_vec[get_type<T>()].second);
		}

		if (ptr)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}

		return ptr.operator bool();
	}

	// Delete the object and return it
	template <typename T>
	static inline std::shared_ptr<T> withdraw()
	{
		std::shared_ptr<void> ptr;
		{
			writer_lock lock(id_manager::g_mutex);
			ptr = std::move(g_vec[get_type<T>()].second);
		}

		if (ptr)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}

		return {ptr, static_cast<T*>(ptr.get())};
	}
};
