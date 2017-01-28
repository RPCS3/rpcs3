#pragma once

#include "Utilities/types.h"
#include "Utilities/mutex.h"

#include <memory>
#include <vector>
#include <unordered_map>

// idm/fxm: helper namespace
namespace id_manager
{
	// Common global mutex
	extern shared_mutex g_mutex;

	// Optional ID traits
	template<typename T, typename = void>
	struct id_traits
	{
		static_assert(sizeof(T) == 0, "ID object must specify: id_base, id_step, id_count");

		static const u32 base = 1; // First ID (N = 0)
		static const u32 step = 1; // Any ID: N * id_step + id_base
		static const u32 count = 65535; // Limit: N < id_count
		static const u32 invalid = 0;
	};

	template<typename T>
	struct id_traits<T, void_t<decltype(&T::id_base), decltype(&T::id_step), decltype(&T::id_count)>>
	{
		static const u32 base = T::id_base;
		static const u32 step = T::id_step;
		static const u32 count = T::id_count;
		static const u32 invalid = base > 0 ? 0 : -1;

		static_assert(u64{step} * count + base < UINT32_MAX, "ID traits: invalid object range");
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

		operator u32() const
		{
			return m_value;
		}
	};

	struct id_hash
	{
		std::size_t operator()(const id_key& id) const
		{
			return id ^ (id >> 8);
		}
	};

	using id_map = std::unordered_map<id_key, std::shared_ptr<void>, id_hash>;
}

// Object manager for emulated process. Multiple objects of specified arbitrary type are given unique IDs.
class idm
{
	// Last allocated ID for constructors
	static thread_local u32 g_id;

	// Type Index -> ID -> Object. Use global since only one process is supported atm.
	static std::vector<id_manager::id_map> g_map;

	template<typename T>
	static inline u32 get_type()
	{
		return id_manager::typeinfo::get_index<T>();
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
	static id_manager::id_map::pointer allocate_id(std::pair<u32, u32> types, u32 base, u32 step, u32 count);

	// Remove ID and return the object (additionally check true_type if not equal)
	static std::shared_ptr<void> delete_id(u32 type, u32 true_type, u32 id);

	// Get ID (additionally check true_type if not equal)
	static id_manager::id_map::const_pointer find_id(u32 type, u32 true_type, u32 id);

	// Allocate new ID and assign the object from the provider()
	template<typename T, typename Type, typename F>
	static id_manager::id_map::pointer create_id(F&& provider)
	{
		writer_lock lock(id_manager::g_mutex);

		// Register destructors
		id_manager::typeinfo::update<T>();

		// Type IDs
		std::pair<u32, u32> types(get_type<T>(), get_type<Type>());

		// Allocate new id
		if (auto* place = allocate_id(types, id_manager::id_traits<T>::base, id_manager::id_traits<T>::step, id_manager::id_traits<T>::count))
		{
			try
			{
				// Get object, store it
				place->second = provider();
				return place;
			}
			catch (...)
			{
				delete_id(types.first, types.first, place->first.id());
				throw;
			}
		}

		return nullptr;
	}

public:
	// Initialize object manager
	static void init();

	// Remove all objects
	static void clear();

	// Get last ID
	static inline u32 last_id()
	{
		return g_id;
	}

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

		return id_manager::id_traits<T>::invalid;
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

		return id_manager::id_traits<T>::invalid;
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
	template<typename T, typename Get = T>
	static inline explicit_bool_t check(u32 id)
	{
		reader_lock lock(id_manager::g_mutex);

		return find_id(get_type<T>(), get_type<Get>(), id) != nullptr;
	}

	// Check the ID, access object under shared lock
	template<typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline explicit_bool_t check(u32 id, F&& func, int = 0)
	{
		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return false;
		}

		func(*static_cast<Get*>(found->second.get()));
		return true;
	}

	// Check the ID, access object under reader lock, propagate return value
	template<typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline return_pair<bool, FRT> check(u32 id, F&& func)
	{
		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return {false};
		}

		return {true, func(*static_cast<Get*>(found->second.get()))};
	}

	// Get the object
	template<typename T, typename Get = T, typename Made = std::conditional_t<std::is_void<Get>::value, T, Get>>
	static inline std::shared_ptr<Made> get(u32 id)
	{
		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return nullptr;
		}

		return {found->second, static_cast<Made*>(found->second.get())};
	}

	// Get the object, access object under reader lock
	template<typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline auto get(u32 id, F&& func, int = 0)
	{
		using result_type = std::shared_ptr<Get>;

		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return result_type{nullptr};
		}

		const auto ptr = static_cast<Get*>(found->second.get());

		func(*ptr);

		return result_type{found->second, ptr};
	}

	// Get the object, access object under reader lock, propagate return value
	template<typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline auto get(u32 id, F&& func)
	{
		using result_type = return_pair<Get, FRT>;

		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id(get_type<T>(), get_type<Get>(), id);

		if (UNLIKELY(found == nullptr))
		{
			return result_type{nullptr};
		}

		const auto ptr = static_cast<Get*>(found->second.get());

		return result_type{{found->second, ptr}, func(*ptr)};
	}

	// Access all objects of specified types under reader lock (use lambda or callable object), return the number of objects processed
	template<typename... Types, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::void_type>
	static inline u32 select(F&& func, int = 0)
	{
		reader_lock lock(id_manager::g_mutex);

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

		reader_lock lock(id_manager::g_mutex);

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
		reader_lock lock(id_manager::g_mutex);

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
	template<typename T, typename Get = T>
	static inline explicit_bool_t remove(u32 id)
	{
		if (auto ptr = (writer_lock{id_manager::g_mutex}, delete_id(get_type<T>(), get_type<Get>(), id)))
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
			return true;
		}

		return false;
	}

	// Remove the ID and return the object
	template<typename T, typename Get = T>
	static inline std::shared_ptr<Get> withdraw(u32 id)
	{
		if (auto ptr = (writer_lock{id_manager::g_mutex}, delete_id(get_type<T>(), get_type<Get>(), id)))
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
			return {ptr, static_cast<Get*>(ptr.get())};
		}

		return nullptr;
	}

	// Remove the ID after accessing the object under writer lock, return the object and propagate return value
	template<typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<std::is_void<FRT>::value>>
	static inline auto withdraw(u32 id, F&& func, int = 0)
	{
		using result_type = std::shared_ptr<Get>;

		std::shared_ptr<void> ptr;
		{
			writer_lock lock(id_manager::g_mutex);

			const auto found = find_id(get_type<T>(), get_type<Get>(), id);

			if (UNLIKELY(found == nullptr))
			{
				return result_type{nullptr};
			}

			func(*static_cast<Get*>(found->second.get()));

			ptr = delete_id(get_type<T>(), get_type<Get>(), id);
		}

		id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));

		return result_type{ptr, static_cast<Get*>(ptr.get())};
	}

	// Conditionally remove the ID (if return value evaluates to false) after accessing the object under writer lock, return the object and propagate return value
	template<typename T, typename Get = T, typename F, typename FRT = std::result_of_t<F(T&)>, typename = std::enable_if_t<!std::is_void<FRT>::value>>
	static inline auto withdraw(u32 id, F&& func)
	{
		using result_type = return_pair<Get, FRT>;

		std::shared_ptr<void> ptr;
		FRT ret;
		{
			writer_lock lock(id_manager::g_mutex);

			const auto found = find_id(get_type<T>(), get_type<Get>(), id);

			if (UNLIKELY(found == nullptr))
			{
				return result_type{nullptr};
			}

			const auto _ptr = static_cast<Get*>(found->second.get());

			ret = func(*_ptr);

			if (ret)
			{
				return result_type{{found->second, _ptr}, std::move(ret)};
			}

			ptr = delete_id(get_type<T>(), get_type<Get>(), id);
		}
		
		id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));

		return result_type{{ptr, static_cast<Get*>(ptr.get())}, std::move(ret)};
	}
};

// Object manager for emulated process. One unique object per type, or zero.
class fxm
{
	// Type Index -> Object. Use global since only one process is supported atm.
	static std::vector<std::shared_ptr<void>> g_vec;

	template<typename T>
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
	template<typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> make(Args&&... args)
	{
		id_manager::typeinfo::update<T>();

		std::shared_ptr<T> ptr;
		{
			writer_lock lock(id_manager::g_mutex);

			if (!g_vec[get_type<T>()])
			{
				ptr = std::make_shared<Make>(std::forward<Args>(args)...);

				g_vec[get_type<T>()] = ptr;
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
			writer_lock lock(id_manager::g_mutex);

			old = std::move(g_vec[get_type<T>()]);
			ptr = std::make_shared<Make>(std::forward<Args>(args)...);

			g_vec[get_type<T>()] = ptr;
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
			writer_lock lock(id_manager::g_mutex);

			if (!g_vec[get_type<T>()])
			{
				ptr = provider();

				g_vec[get_type<T>()] = ptr;
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
			writer_lock lock(id_manager::g_mutex);

			old = std::move(g_vec[get_type<T>()]);
			ptr = provider();

			g_vec[get_type<T>()] = ptr;
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
			writer_lock lock(id_manager::g_mutex);

			if (auto& value = g_vec[get_type<T>()])
			{
				return{ value, static_cast<T*>(value.get()) };
			}
			else
			{
				ptr = std::make_shared<Make>(std::forward<Args>(args)...);

				g_vec[get_type<T>()] = ptr;
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
		reader_lock lock(id_manager::g_mutex);

		return g_vec[get_type<T>()].operator bool();
	}

	// Get the object (returns nullptr if it doesn't exist)
	template<typename T>
	static inline std::shared_ptr<T> get()
	{
		reader_lock lock(id_manager::g_mutex);

		auto& ptr = g_vec[get_type<T>()];

		return{ ptr, static_cast<T*>(ptr.get()) };
	}

	// Delete the object
	template<typename T>
	static inline explicit_bool_t remove()
	{
		std::shared_ptr<void> ptr;
		{
			writer_lock lock(id_manager::g_mutex);
			ptr = std::move(g_vec[get_type<T>()]);
		}
		
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
		std::shared_ptr<void> ptr;
		{
			writer_lock lock(id_manager::g_mutex);
			ptr = std::move(g_vec[get_type<T>()]);
		}

		if (ptr)
		{
			id_manager::on_stop<T>::func(static_cast<T*>(ptr.get()));
		}
		
		return{ ptr, static_cast<T*>(ptr.get()) };
	}
};
