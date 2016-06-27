#pragma once

#include "Utilities/types.h"
#include "Utilities/Macro.h"
#include "Utilities/Platform.h"
#include "Utilities/SharedMutex.h"

#include <memory>
#include <vector>
#include <unordered_map>

// Mostly helper namespace
namespace id_manager
{
	// Optional ID traits
	template<typename T, typename = void>
	struct id_traits
	{
		using tag = void;

		static constexpr u32 min = 1;
		static constexpr u32 max = 0x7fffffff;
	};

	template<typename T>
	struct id_traits<T, void_t<typename T::id_base, decltype(&T::id_min), decltype(&T::id_max)>>
	{
		using tag = typename T::id_base;

		static constexpr u32 min = T::id_min;
		static constexpr u32 max = T::id_max;
	};

	// Optional object initialization function (called after ID registration)
	template<typename T, typename = void>
	struct on_init
	{
		static inline void func(T*)
		{
			// Forbid forward declarations
			static constexpr auto size = sizeof(std::conditional_t<std::is_void<T>::value, void*, T>);
		}
	};

	template<typename T>
	struct on_init<T, decltype(std::declval<T>().on_init())>
	{
		static inline void func(T* ptr)
		{
			if (ptr) ptr->on_init();
		}
	};

	// Optional object finalization function (called after ID removal)
	template<typename T, typename = void>
	struct on_stop
	{
		static inline void func(T*)
		{
			// Forbid forward declarations
			static constexpr auto size = sizeof(std::conditional_t<std::is_void<T>::value, void*, T>);
		}
	};

	template<typename T>
	struct on_stop<T, decltype(std::declval<T>().on_stop())>
	{
		static inline void func(T* ptr)
		{
			if (ptr) ptr->on_stop();
		}
	};

	class typeinfo
	{
		// Global variable for each registered type
		template<typename T>
		struct registered
		{
			static const u32 index;
		};

		// Access global type list
		static std::vector<typeinfo>& access();

		// Add to the global list
		static u32 add_type();

	public:
		void(*on_init)(void*) = nullptr;
		void(*on_stop)(void*) = nullptr;

		// Get type index
		template<typename T>
		static inline u32 get_index()
		{
			return registered<T>::index;
		}

		// Register functions
		template<typename T>
		static inline void update()
		{
			auto& info = access()[get_index<T>()];

			info.on_init = [](void* ptr) { return_ id_manager::on_init<T>::func(static_cast<T*>(ptr)); };
			info.on_stop = [](void* ptr) { return_ id_manager::on_stop<T>::func(static_cast<T*>(ptr)); };
		}

		// Read all registered types
		static inline const auto& get()
		{
			return access();
		}
	};

	template<typename T>
	const u32 typeinfo::registered<T>::index = typeinfo::add_type();
}

// Object manager for emulated process. Multiple objects of specified arbitrary type are given unique IDs.
class idm
{
	// Rules for ID allocation:
	// 0) Individual ID counter may be specified for each type by defining 'using id_base = ...;'
	// 1) If no id_base specified, void is assumed.
	// 2) g_id[id_base] indicates next ID allocated in g_map.
	// 3) g_map[id_base] contains the additional copy of object pointer.

	// Custom hasher for ID values
	struct id_hash_t final
	{
		std::size_t operator ()(u32 value) const
		{
			return value;
		}
	};

	using map_type = std::unordered_map<u32, std::shared_ptr<void>, id_hash_t>;

	static shared_mutex g_mutex;

	// Type Index -> ID -> Object. Use global since only one process is supported atm.
	static std::vector<map_type> g_map;

	// Next ID for each category
	static std::vector<u32> g_id;

	template<typename T>
	static inline u32 get_type()
	{
		return id_manager::typeinfo::get_index<T>();
	}

	template<typename T>
	static inline u32 get_tag()
	{
		return get_type<typename id_manager::id_traits<T>::tag>();
	}

	// Update optional ID storage
	template<typename T>
	static inline auto set_id_value(T* ptr, u32 id) -> decltype(static_cast<void>(std::declval<T&>().id))
	{
		ptr->id = id;
	}

	static inline void set_id_value(...)
	{
	}

	// Helper
	template<typename F>
	struct function_traits;

	template<typename F, typename R, typename A1, typename A2>
	struct function_traits<R(F::*)(A1, A2&) const>
	{
		using second_type = A2;
		using return_type = R;
	};

	// Helper
	template<bool Value>
	struct bool_if_void
	{
		friend bool operator ,(bool lhs, const bool_if_void&)
		{
			return lhs;
		}

		operator bool() const
		{
			return Value;
		}
	};

	// Prepares new ID, returns nullptr if out of resources
	static map_type::pointer allocate_id(u32 tag, u32 min, u32 max);

	// Deallocate ID, returns object
	static std::shared_ptr<void> deallocate_id(u32 tag, u32 id);

	// Allocate new ID and construct it from the provider()
	template<typename T, typename F>
	static map_type::pointer create_id(F&& provider)
	{
		id_manager::typeinfo::update<T>();
		id_manager::typeinfo::update<typename id_manager::id_traits<T>::tag>();

		writer_lock lock(g_mutex);

		if (auto place = allocate_id(get_tag<T>(), id_manager::id_traits<T>::min, id_manager::id_traits<T>::max))
		{
			try
			{
				// Get object, store it
				place->second = provider();
				
				// Update ID value if required
				set_id_value(static_cast<T*>(place->second.get()), place->first);

				return &*g_map[get_type<T>()].emplace(*place).first;
			}
			catch (...)
			{
				deallocate_id(get_tag<T>(), place->first);
				throw;
			}
		}

		return nullptr;
	}

	// Get ID (internal)
	static map_type::pointer find_id(u32 type, u32 id);

	// Remove ID and return object
	static std::shared_ptr<void> delete_id(u32 type, u32 tag, u32 id);

public:
	// Initialize object manager
	static void init();

	// Remove all objects
	static void clear();

	// Add a new ID of specified type with specified constructor arguments (returns object or nullptr)
	template<typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> make_ptr(Args&&... args)
	{
		if (auto pair = create_id<T>(WRAP_EXPR(std::make_shared<Make>(std::forward<Args>(args)...))))
		{
			id_manager::on_init<T>::func(static_cast<T*>(pair->second.get()));
			id_manager::on_stop<T>::func(nullptr);
			return{ pair->second, static_cast<T*>(pair->second.get()) };
		}

		return nullptr;
	}

	// Add a new ID of specified type with specified constructor arguments (returns id)
	template<typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, u32> make(Args&&... args)
	{
		if (auto pair = create_id<T>(WRAP_EXPR(std::make_shared<Make>(std::forward<Args>(args)...))))
		{
			id_manager::on_init<T>::func(static_cast<T*>(pair->second.get()));
			id_manager::on_stop<T>::func(nullptr);
			return pair->first;
		}

		throw EXCEPTION("Out of IDs ('%s')", typeid(T).name());
	}

	// Add a new ID for an existing object provided (returns new id)
	template<typename T>
	static inline u32 import_existing(const std::shared_ptr<T>& ptr)
	{
		if (auto pair = create_id<T>(WRAP_EXPR(ptr)))
		{
			id_manager::on_init<T>::func(static_cast<T*>(pair->second.get()));
			id_manager::on_stop<T>::func(nullptr);
			return pair->first;
		}

		throw EXCEPTION("Out of IDs ('%s')", typeid(T).name());
	}

	// Add a new ID for an object returned by provider()
	template<typename T, typename F, typename = std::result_of_t<F()>>
	static inline std::shared_ptr<T> import(F&& provider)
	{
		if (auto pair = create_id<T>(std::forward<F>(provider)))
		{
			id_manager::on_init<T>::func(static_cast<T*>(pair->second.get()));
			id_manager::on_stop<T>::func(nullptr);
			return { pair->second, static_cast<T*>(pair->second.get()) };
		}

		return nullptr;
	}

	// Check whether the ID exists
	template<typename T>
	static inline bool check(u32 id)
	{
		reader_lock lock(g_mutex);

		return find_id(get_type<T>(), id) != nullptr;
	}

	// Get the ID
	template<typename T>
	static inline std::shared_ptr<T> get(u32 id)
	{
		reader_lock lock(g_mutex);

		const auto found = find_id(get_type<T>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return nullptr;
		}

		return{ found->second, static_cast<T*>(found->second.get()) };
	}

	// Conditionally get the ID, almost similar to select() but for the single object only.
	template<typename T, typename F, typename FT = decltype(&F::operator()), typename A2 = typename function_traits<FT>::second_type>
	static inline auto get(u32 id, F&& pred)
	{
		using result_type = std::conditional_t<std::is_void<typename function_traits<FT>::return_type>::value, void, std::shared_ptr<A2>>;

		reader_lock lock(g_mutex);

		const auto found = find_id(get_type<T>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return static_cast<result_type>(nullptr);
		}

		if (pred(id, *static_cast<A2*>(found->second.get())), bool_if_void<false>())
		{
			return static_cast<result_type>(std::static_pointer_cast<A2>(found->second));
		}

		return static_cast<result_type>(nullptr);
	}

	// Execute for all IDs (unsorted), may return void. If the result evaluates to true, the loop stops and returns the object.
	template<typename... Types, typename F, typename FT = decltype(&F::operator()), typename A2 = typename function_traits<FT>::second_type>
	static inline auto select(F&& pred)
	{
		using result_type = std::conditional_t<std::is_void<typename function_traits<FT>::return_type>::value, void, std::shared_ptr<A2>>;

		reader_lock lock(g_mutex);

		for (u32 type : { get_type<Types>()... })
		{
			for (auto& id : g_map[type])
			{
				if (pred(id.first, *static_cast<A2*>(id.second.get())), bool_if_void<false>())
				{
					return static_cast<result_type>(std::static_pointer_cast<A2>(id.second));
				}
			}
		}

		return static_cast<result_type>(nullptr);
	}

	// Get count of objects
	template<typename T>
	static inline u32 get_count()
	{
		reader_lock lock(g_mutex);

		return ::size32(g_map[get_type<T>()]);
	}

	// Remove the ID
	template<typename T>
	static inline bool remove(u32 id)
	{
		auto&& ptr = delete_id(get_type<T>(), get_tag<T>(), id);

		if (LIKELY(ptr))
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}

		return ptr.operator bool();
	}

	// Remove the ID and return it
	template<typename T>
	static inline std::shared_ptr<T> withdraw(u32 id)
	{
		auto&& ptr = delete_id(get_type<T>(), get_tag<T>(), id);

		if (LIKELY(ptr))
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}

		return{ ptr, static_cast<T*>(ptr.get()) };
	}

	// Conditionally remove the ID and return it.
	template<typename T, typename F>
	static inline std::shared_ptr<T> withdraw(u32 id, F&& pred)
	{
		std::shared_ptr<void> ptr;
		{
			writer_lock lock(g_mutex);

			const auto found = find_id(get_type<T>(), id);

			if (UNLIKELY(found == nullptr || !pred(id, *static_cast<T*>(found->second.get()))))
			{
				return nullptr;
			}

			ptr = deallocate_id(get_tag<T>(), id);

			g_map[get_type<T>()].erase(id);
		}
		
		id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));

		return{ ptr, static_cast<T*>(ptr.get()) };
	}
};

// Object manager for emulated process. One unique object per type, or zero.
class fxm
{
	// Type Index -> Object. Use global since only one process is supported atm.
	static std::vector<std::shared_ptr<void>> g_map;

	static shared_mutex g_mutex;

	template<typename T>
	static inline u32 get_type()
	{
		return id_manager::typeinfo::get_index<T>();
	}

	static std::shared_ptr<void> remove(u32 type);

public:
	// Initialize object manager
	static void init();
	
	// Remove all objects
	static void clear();

	// Create the object (returns nullptr if it already exists)
	template<typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> make(Args&&... args)
	{
		id_manager::typeinfo::update<T>();

		std::shared_ptr<T> ptr;
		{
			writer_lock lock(g_mutex);

			if (!g_map[get_type<T>()])
			{
				ptr = std::make_shared<Make>(std::forward<Args>(args)...);

				g_map[get_type<T>()] = ptr;
			}
		}

		if (ptr)
		{
			id_manager::on_init<T>::func(ptr.get());
			id_manager::on_stop<T>::func(nullptr);
		}

		return ptr;
	}

	// Create the object unconditionally (old object will be removed if it exists)
	template<typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> make_always(Args&&... args)
	{
		id_manager::typeinfo::update<T>();

		std::shared_ptr<T> ptr;
		std::shared_ptr<void> old;
		{
			writer_lock lock(g_mutex);

			old = std::move(g_map[get_type<T>()]);
			ptr = std::make_shared<Make>(std::forward<Args>(args)...);

			g_map[get_type<T>()] = ptr;
		}

		if (old)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(old.get()));
		}

		id_manager::on_init<T>::func(ptr.get());
		return ptr;
	}

	// Emplace the object returned by provider() and return it if no object exists
	template<typename T, typename F>
	static auto import(F&& provider) -> decltype(static_cast<std::shared_ptr<T>>(provider()))
	{
		id_manager::typeinfo::update<T>();

		std::shared_ptr<T> ptr;
		{
			writer_lock lock(g_mutex);

			if (!g_map[get_type<T>()])
			{
				ptr = provider();

				g_map[get_type<T>()] = ptr;
			}
		}

		if (ptr)
		{
			id_manager::on_init<T>::func(ptr.get());
			id_manager::on_stop<T>::func(nullptr);
		}

		return ptr;
	}

	// Emplace the object return by provider() (old object will be removed if it exists)
	template<typename T, typename F>
	static auto import_always(F&& provider) -> decltype(static_cast<std::shared_ptr<T>>(provider()))
	{
		id_manager::typeinfo::update<T>();

		std::shared_ptr<T> ptr;
		std::shared_ptr<void> old;
		{
			writer_lock lock(g_mutex);

			old = std::move(g_map[get_type<T>()]);
			ptr = provider();

			g_map[get_type<T>()] = ptr;
		}

		if (old)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(old.get()));
		}

		id_manager::on_init<T>::func(ptr.get());
		return ptr;
	}

	// Get the object unconditionally (create an object if it doesn't exist)
	template<typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> get_always(Args&&... args)
	{
		id_manager::typeinfo::update<T>();

		std::shared_ptr<T> ptr;
		{
			writer_lock lock(g_mutex);

			if (auto& value = g_map[get_type<T>()])
			{
				return{ value, static_cast<T*>(value.get()) };
			}
			else
			{
				ptr = std::make_shared<Make>(std::forward<Args>(args)...);

				g_map[get_type<T>()] = ptr;
			}
		}

		id_manager::on_init<T>::func(ptr.get());
		id_manager::on_stop<T>::func(nullptr);
		return ptr;
	}

	// Check whether the object exists
	template<typename T>
	static inline bool check()
	{
		reader_lock lock(g_mutex);

		return g_map[get_type<T>()].operator bool();
	}

	// Get the object (returns nullptr if it doesn't exist)
	template<typename T>
	static inline std::shared_ptr<T> get()
	{
		reader_lock lock(g_mutex);

		auto& ptr = g_map[get_type<T>()];

		return{ ptr, static_cast<T*>(ptr.get()) };
	}

	// Delete the object
	template<typename T>
	static inline bool remove()
	{
		auto&& ptr = remove(get_type<T>());
		
		if (ptr)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}

		return ptr.operator bool();
	}

	// Delete the object and return it
	template<typename T>
	static inline std::shared_ptr<T> withdraw()
	{
		auto&& ptr = remove(get_type<T>());

		if (ptr)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}
		
		return{ ptr, static_cast<T*>(ptr.get()) };
	}
};
