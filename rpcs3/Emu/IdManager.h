#pragma once

#include "Utilities/types.h"
#include "Utilities/mutex.h"

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
		static inline void func(T*, const std::shared_ptr<void>&)
		{
			// Forbid forward declarations
			static constexpr auto size = sizeof(std::conditional_t<std::is_void<T>::value, void*, T>);
		}
	};

	template<typename T>
	struct on_init<T, decltype(std::declval<T>().on_init(std::declval<const std::shared_ptr<void>&>()))>
	{
		static inline void func(T* ptr, const std::shared_ptr<void>&_ptr)
		{
			if (ptr) ptr->on_init(_ptr);
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
			access()[get_index<T>()].on_stop = [](void* ptr) { return id_manager::on_stop<T>::func(static_cast<T*>(ptr)); };
		}

		// Read all registered types
		static inline const auto& get()
		{
			return access();
		}
	};

	template<typename T>
	const u32 typeinfo::registered<T>::index = typeinfo::add_type();

	// ID value with additional type stored
	class id_key
	{
		u32 m_value; // ID value
		u32 m_type; // True object type

	public:
		id_key() = default;

		id_key(u32 value, u32 type = 0)
			: m_value(value)
			, m_type(type)
		{
		}

		u32 id() const
		{
			return m_value;
		}

		u32 type() const
		{
			return m_type;
		}

		bool operator ==(const id_key& rhs) const
		{
			return m_value == rhs.m_value;
		}

		bool operator !=(const id_key& rhs) const
		{
			return m_value != rhs.m_value;
		}
	};

	// Custom hasher for ID values
	struct id_hash final
	{
		std::size_t operator ()(const id_key& key) const
		{
			return key.id();
		}
	};

	using id_map = std::unordered_map<id_key, std::shared_ptr<void>, id_hash>;
}

// Object manager for emulated process. Multiple objects of specified arbitrary type are given unique IDs.
class idm
{
	// Rules for ID allocation:
	// 0) Individual ID counter may be specified for each type by defining 'using id_base = ...;'
	// 1) If no id_base specified, void is assumed.
	// 2) g_id[id_base] indicates next ID allocated in g_map.
	// 3) g_map[id_base] contains the additional copy of object pointer.

	static shared_mutex g_mutex;

	// Type Index -> ID -> Object. Use global since only one process is supported atm.
	static std::vector<id_manager::id_map> g_map;

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
		using object_type = A2;
		using result_type = R;
	};

	template<typename F, typename R, typename A1, typename A2>
	struct function_traits<R(F::*)(A1, A2&)>
	{
		using object_type = A2;
		using result_type = R;
	};

	template<typename F, typename A1, typename A2>
	struct function_traits<void(F::*)(A1, A2&) const>
	{
		using object_type = A2;
		using void_type = void;
	};

	template<typename F, typename A1, typename A2>
	struct function_traits<void(F::*)(A1, A2&)>
	{
		using object_type = A2;
		using void_type = void;
	};

	// Helper
	template<typename T, typename RT>
	struct return_pair
	{
		std::shared_ptr<T> ptr;
		RT value;

		explicit operator bool() const
		{
			return ptr.operator bool();
		}

		auto operator->() const
		{
			return ptr.get();
		}
	};

	template<typename RT>
	struct return_pair<bool, RT>
	{
		bool result;
		RT value;

		explicit operator bool() const
		{
			return result;
		}
	};

	// Prepare new ID (returns nullptr if out of resources)
	static id_manager::id_map::pointer allocate_id(u32 tag, u32 type, u32 min, u32 max);

	// Deallocate ID, returns object
	static std::shared_ptr<void> deallocate_id(u32 tag, u32 id);

	// Allocate new ID and assign the object from the provider()
	template<typename T, typename Set, typename F>
	static id_manager::id_map::pointer create_id(F&& provider)
	{
		id_manager::typeinfo::update<T>();
		id_manager::typeinfo::update<typename id_manager::id_traits<T>::tag>();

		writer_lock lock(g_mutex);

		if (auto place = allocate_id(get_tag<T>(), get_type<Set>(), id_manager::id_traits<T>::min, id_manager::id_traits<T>::max))
		{
			try
			{
				// Get object, store it
				place->second = provider();
				
				// Update ID value if required
				set_id_value(static_cast<T*>(place->second.get()), place->first.id());

				return &*g_map[get_type<T>()].emplace(*place).first;
			}
			catch (...)
			{
				deallocate_id(get_tag<T>(), place->first.id());
				throw;
			}
		}

		return nullptr;
	}

	// Get ID (internal)
	static id_manager::id_map::pointer find_id(u32 type, u32 true_type, u32 id);

	// Remove ID and return the object
	static std::shared_ptr<void> delete_id(u32 type, u32 true_type, u32 tag, u32 id);

public:
	// Initialize object manager
	static void init();

	// Remove all objects
	static void clear();

	// Add a new ID of specified type with specified constructor arguments (returns object or nullptr)
	template<typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<Make>> make_ptr(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return std::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			id_manager::on_init<T>::func(static_cast<T*>(pair->second.get()), pair->second);
			id_manager::on_stop<T>::func(nullptr);
			return{ pair->second, static_cast<Make*>(pair->second.get()) };
		}

		return nullptr;
	}

	// Add a new ID of specified type with specified constructor arguments (returns id)
	template<typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, u32> make(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return std::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			id_manager::on_init<T>::func(static_cast<T*>(pair->second.get()), pair->second);
			id_manager::on_stop<T>::func(nullptr);
			return pair->first.id();
		}

		fmt::throw_exception("Out of IDs ('%s')" HERE, typeid(T).name());
	}

	// Add a new ID for an existing object provided (returns new id)
	template<typename T, typename Made = T>
	static inline u32 import_existing(const std::shared_ptr<T>& ptr)
	{
		if (auto pair = create_id<T, Made>([&] { return ptr; }))
		{
			id_manager::on_init<T>::func(static_cast<T*>(pair->second.get()), pair->second);
			id_manager::on_stop<T>::func(nullptr);
			return pair->first.id();
		}

		fmt::throw_exception("Out of IDs ('%s')" HERE, typeid(T).name());
	}

	// Add a new ID for an object returned by provider()
	template<typename T, typename Made = T, typename F, typename = std::result_of_t<F()>>
	static inline std::shared_ptr<Made> import(F&& provider)
	{
		if (auto pair = create_id<T, Made>(std::forward<F>(provider)))
		{
			id_manager::on_init<T>::func(static_cast<T*>(pair->second.get()), pair->second);
			id_manager::on_stop<T>::func(nullptr);
			return { pair->second, static_cast<Made*>(pair->second.get()) };
		}

		return nullptr;
	}

	// Check the ID
	template<typename T, typename Get = void>
	static inline explicit_bool_t check(u32 id)
	{
		reader_lock lock(g_mutex);

		return find_id(get_type<T>(), get_type<Get>(), id) != nullptr;
	}

	// Check the ID, access object under shared lock
	template<typename T, typename Get = void, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline explicit_bool_t check(u32 id, F&& func, int = 0)
	{
		using pointer_type = std::conditional_t<std::is_void<Get>::value, T, Get>;

		reader_lock lock(g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return false;
		}

		func(*static_cast<pointer_type*>(found->second.get()));
		return true;
	}

	// Check the ID, access object under reader lock, propagate return value
	template<typename T, typename Get = void, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline return_pair<bool, FRT> check(u32 id, F&& func)
	{
		using pointer_type = std::conditional_t<std::is_void<Get>::value, T, Get>;

		reader_lock lock(g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return {false};
		}

		return {true, func(*static_cast<pointer_type*>(found->second.get()))};
	}

	// Get the object
	template<typename T, typename Get = void, typename Made = std::conditional_t<std::is_void<Get>::value, T, Get>>
	static inline std::shared_ptr<Made> get(u32 id)
	{
		reader_lock lock(g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return nullptr;
		}

		return {found->second, static_cast<Made*>(found->second.get())};
	}

	// Get the object, access object under reader lock
	template<typename T, typename Get = void, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline auto get(u32 id, F&& func, int = 0)
	{
		using pointer_type = std::conditional_t<std::is_void<Get>::value, T, Get>;
		using result_type = std::shared_ptr<pointer_type>;

		reader_lock lock(g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return result_type{nullptr};
		}

		const auto ptr = static_cast<pointer_type*>(found->second.get());

		func(*ptr);

		return result_type{found->second, ptr};
	}

	// Get the object, access object under reader lock, propagate return value
	template<typename T, typename Get = void, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline auto get(u32 id, F&& func)
	{
		using pointer_type = std::conditional_t<std::is_void<Get>::value, T, Get>;
		using result_type = return_pair<pointer_type, FRT>;

		reader_lock lock(g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return result_type{nullptr};
		}

		const auto ptr = static_cast<pointer_type*>(found->second.get());

		return result_type{{found->second, ptr}, func(*ptr)};
	}

	// Access all objects of specified types under reader lock (use lambda or callable object), return the number of objects processed
	template<typename... Types, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::void_type>
	static inline u32 select(F&& func, int = 0)
	{
		reader_lock lock(g_mutex);

		u32 result = 0;

		for (u32 type : { get_type<Types>()... })
		{
			for (auto& id : g_map[type])
			{
				func(id.first.id(), *static_cast<typename function_traits<FT>::object_type*>(id.second.get()));
				result++;
			}
		}

		return result;
	}

	// Access all objects of specified types under reader lock (use lambda or callable object), if return value evaluates to true, stop and return the object and the value
	template<typename... Types, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::result_type>
	static inline auto select(F&& func)
	{
		using object_type = typename function_traits<FT>::object_type;
		using result_type = return_pair<object_type, FRT>;

		reader_lock lock(g_mutex);

		for (u32 type : { get_type<Types>()... })
		{
			for (auto& id : g_map[type])
			{
				if (FRT result = func(id.first.id(), *static_cast<object_type*>(id.second.get())))
				{
					return result_type{{id.second, static_cast<object_type*>(id.second.get())}, std::move(result)};
				}
			}
		}

		return result_type{nullptr};
	}

	// Get count of objects
	template<typename T, typename Get = void>
	static inline u32 get_count()
	{
		reader_lock lock(g_mutex);

		if (std::is_void<Get>::value)
		{
			return ::size32(g_map[get_type<T>()]);
		}

		u32 result = 0;
		
		for (auto& id : g_map[get_type<T>()])
		{
			if (id.first.type() == get_type<Get>())
			{
				result++;
			}
		}

		return result;
	}

	// Remove the ID
	template<typename T, typename Get = void>
	static inline explicit_bool_t remove(u32 id)
	{
		auto ptr = delete_id(get_type<T>(), get_type<Get>(), get_tag<T>(), id);

		if (LIKELY(ptr))
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}

		return ptr.operator bool();
	}

	// Remove the ID and return the object
	template<typename T, typename Get = void, typename Made = std::conditional_t<std::is_void<Get>::value, T, Get>>
	static inline std::shared_ptr<Made> withdraw(u32 id)
	{
		auto ptr = delete_id(get_type<T>(), get_type<Get>(), get_tag<T>(), id);

		if (LIKELY(ptr))
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}

		return {ptr, static_cast<Made*>(ptr.get())};
	}

	// Remove the ID after accessing the object under writer lock, return the object and propagate return value
	template<typename T, typename Get = void, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline auto withdraw(u32 id, F&& func, int = 0)
	{
		using pointer_type = std::conditional_t<std::is_void<Get>::value, T, Get>;
		using result_type = std::shared_ptr<pointer_type>;

		std::shared_ptr<void> ptr;
		{
			writer_lock lock(g_mutex);

			const auto found = find_id(get_type<T>(), get_type<Get>(), id);

			if (UNLIKELY(found == nullptr))
			{
				return result_type{nullptr};
			}

			func(*static_cast<pointer_type*>(found->second.get()));

			ptr = deallocate_id(get_tag<T>(), id);

			g_map[get_type<T>()].erase(id);
		}

		id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));

		return result_type{ptr, static_cast<pointer_type*>(ptr.get())};
	}

	// Conditionally remove the ID (if return value evaluates to false) after accessing the object under writer lock, return the object and propagate return value
	template<typename T, typename Get = void, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline auto withdraw(u32 id, F&& func)
	{
		using pointer_type = std::conditional_t<std::is_void<Get>::value, T, Get>;
		using result_type = return_pair<pointer_type, FRT>;

		std::shared_ptr<void> ptr;
		FRT ret;
		{
			writer_lock lock(g_mutex);

			const auto found = find_id(get_type<T>(), get_type<Get>(), id);

			if (UNLIKELY(found == nullptr))
			{
				return result_type{nullptr};
			}

			const auto _ptr = static_cast<pointer_type*>(found->second.get());

			ret = func(*_ptr);

			if (ret)
			{
				return result_type{{found->second, _ptr}, std::move(ret)};
			}

			ptr = deallocate_id(get_tag<T>(), id);

			g_map[get_type<T>()].erase(id);
		}
		
		id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));

		return result_type{{ptr, static_cast<pointer_type*>(ptr.get())}, std::move(ret)};
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
			id_manager::on_init<T>::func(ptr.get(), ptr);
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

		id_manager::on_init<T>::func(ptr.get(), ptr);
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
			id_manager::on_init<T>::func(ptr.get(), ptr);
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

		id_manager::on_init<T>::func(ptr.get(), ptr);
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

		id_manager::on_init<T>::func(ptr.get(), ptr);
		id_manager::on_stop<T>::func(nullptr);
		return ptr;
	}

	// Check whether the object exists
	template<typename T>
	static inline explicit_bool_t check()
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
	static inline explicit_bool_t remove()
	{
		auto ptr = remove(get_type<T>());
		
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
		auto ptr = remove(get_type<T>());

		if (ptr)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}
		
		return{ ptr, static_cast<T*>(ptr.get()) };
	}
};
