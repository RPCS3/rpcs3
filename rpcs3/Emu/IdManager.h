#pragma once

#include "util/types.hpp"
#include "Utilities/mutex.h"

#include <memory>
#include <vector>
#include <span>

#include "util/serialization.hpp"
#include "util/shared_ptr.hpp"
#include "util/fixed_typemap.hpp"

extern stx::manual_typemap<void, 0x20'00000, 128> g_fixed_typemap;

constexpr auto* g_fxo = &g_fixed_typemap;

enum class thread_state : u32;

extern u16 serial_breathe_and_tag(utils::serial& ar, std::string_view name, bool tag_bit);

template <typename T>
concept IdmCompatible = requires () { u32{T::id_base}, u32{T::id_step}, u32{T::id_count}; };

template <typename T>
concept IdmBaseCompatible = (std::is_final_v<T> ? IdmCompatible<T> : !!(requires () { u32{T::id_step}, u32{T::id_count}; }));

template <typename T>
concept IdmSavable = IdmBaseCompatible<T> && T::savestate_init_pos != 0 && (requires(T& t, utils::serial& ar) { t.save(stx::exact_t<utils::serial&>(ar)); });

// If id_base is declared in base type, than storage type must declare id_type
template <typename Base, typename Type>
concept IdmTypesCompatible = PtrSame<Base, Type> && IdmCompatible<Type> && IdmBaseCompatible<Base> && (std::is_same_v<Base, Type> || !IdmCompatible<Base> || !!(requires () { u32{Type::id_type}; }));

// Helper namespace
namespace id_manager
{
	using pointer_keeper = std::function<void(void*)>;

	// Common global mutex
	extern shared_mutex g_mutex;

	template <typename T>
	constexpr std::pair<u32, u32> get_invl_range()
	{
		return {0, 0};
	}

	template <typename T> requires requires () { T::id_invl_range.first + T::id_invl_range.second; }
	constexpr std::pair<u32, u32> get_invl_range()
	{
		return T::id_invl_range;
	}

	template <typename T>
	consteval bool get_force_lowest_id()
	{
		return false;
	}

	template <typename T> requires requires () { bool{T::id_lowest}; }
	consteval bool get_force_lowest_id()
	{
		return T::id_lowest;
	}

	// Last allocated ID for constructors
	extern thread_local u32 g_id;

	// ID traits
	template <typename T>
	struct id_traits
	{
		static_assert(IdmCompatible<T>, "ID object must specify: id_base, id_step, id_count");

		static constexpr u32 base = T::id_base; // First ID (N = 0)
		static constexpr u32 step = T::id_step; // Any ID: N * id_setp + id_base
		static constexpr u32 count = T::id_count; // Limit: N < id_count
		static constexpr u32 invalid = -+!base; // Invalid ID sample

		static constexpr std::pair<u32, u32> invl_range = get_invl_range<T>();
		static constexpr bool uses_lowest_id = get_force_lowest_id<T>();

		static_assert(u32{count} && u32{step} && u64{step} * (count - 1) + base < u32{umax} + u64{base != 0 ? 1 : 0}, "ID traits: invalid object range");

		// TODO: Add more conditions
		static_assert(!invl_range.second || (u64{invl_range.second} + invl_range.first <= 32 /*....*/ ));
	};

	static constexpr u32 get_index(u32 id, u32 base, u32 step, u32 count, std::pair<u32, u32> invl_range)
	{
		u32 mask_out = ((1u << invl_range.second) - 1) << invl_range.first;

		// Note: if id is lower than base, diff / step will be higher than count
		u32 diff = (id & ~mask_out) - base;

		if (diff % step)
		{
			// id is invalid, return invalid index
			return count;
		}

		// Get actual index
		return diff / step;
	}

	// ID traits
	template <typename T>
	struct id_traits_load_func
	{
		static constexpr pointer_keeper(*load)(utils::serial&) = [](utils::serial& ar) -> pointer_keeper
		{
			stx::shared_ptr<T> ptr;

			if constexpr (std::is_constructible_v<T, stx::exact_t<const stx::launch_retainer&>, stx::exact_t<utils::serial&>>)
			{
				ptr = stx::make_shared<T>(stx::launch_retainer{}, stx::exact_t<utils::serial&>(ar));
			}
			else
			{
				ptr = stx::make_shared<T>(stx::exact_t<utils::serial&>(ar));
			}

			return [ptr](void* storage) { *static_cast<stx::atomic_ptr<T>*>(storage) = ptr; };
		};
	};

	template <typename T>
		requires requires() { &T::load; }
	struct id_traits_load_func<T>
	{
		static constexpr pointer_keeper(*load)(utils::serial&) = [](utils::serial& ar) -> pointer_keeper
		{
			return T::load(stx::exact_t<utils::serial&>(ar));
		};
	};

	template <typename T>
	struct id_traits_savable_func
	{
		static constexpr bool(*savable)(void*) = [](void*) -> bool { return true; };
	};

	template <typename T>
		requires requires { &T::savable; }
	struct id_traits_savable_func<T>
	{
		static constexpr bool(*savable)(void* ptr) = [](void* ptr) -> bool { return static_cast<const T*>(ptr)->savable(); };
	};

	struct dummy_construct
	{
		dummy_construct() = default;
		dummy_construct(utils::serial&) noexcept {}
		void save(utils::serial&) {}

		static constexpr u32 id_base = 1, id_step = 1, id_count = 1;
		static constexpr double savestate_init_pos = 0;
	};

	struct typeinfo;

	// Use a vector instead of map to reduce header dependencies in this commonly used header
	std::vector<std::pair<u128, typeinfo>>& get_typeinfo_map();

	struct typeinfo
	{
	public:
		std::function<void(void*)>(*load)(utils::serial&);
		void(*save)(utils::serial&, void*);
		bool(*savable)(void* ptr);

		u32 base;
		u32 step;
		u32 count;
		bool uses_lowest_id;
		std::pair<u32, u32> invl_range;

		// Unique type ID within the same container: we use id_base if nothing else was specified
		template <typename T>
		static consteval u32 get_type()
		{
			return T::id_base;
		}

		// Specified type ID for containers which their types may be sharing an overlapping IDs range
		template <typename T> requires requires () { u32{T::id_type}; }
		static consteval u32 get_type()
		{
			return T::id_type;
		}

		template <typename T>
		static typeinfo make_typeinfo()
		{
			typeinfo info{};

			using C = std::conditional_t<IdmCompatible<T> && IdmSavable<T>, T, dummy_construct>;
			using Type = std::conditional_t<IdmCompatible<T>, T, dummy_construct>;

			if constexpr (std::is_same_v<C, T>)
			{
				info =
				{
					+id_traits_load_func<C>::load,
					+[](utils::serial& ar, void* obj) { static_cast<C*>(obj)->save(ar); },
					+id_traits_savable_func<C>::savable,
					id_traits<C>::base, id_traits<C>::step, id_traits<C>::count, id_traits<C>::uses_lowest_id, id_traits<C>::invl_range,
				};

				const u128 key = u128{get_type<C>()} << 64 | std::bit_cast<u64>(C::savestate_init_pos);

				for (const auto& [tkey, tinfo] : get_typeinfo_map())
				{
					if (!(tkey ^ key))
					{
						ensure(tinfo == info);
						return info;
					}
				}

				// id_base must be unique within all the objects with the same initialization posistion by definition of id_map with multiple types
				get_typeinfo_map().emplace_back(key, info);
			}
			else
			{
				info =
				{
					nullptr,
					nullptr,
					nullptr,
					id_traits<Type>::base, id_traits<Type>::step, id_traits<Type>::count, id_traits<Type>::uses_lowest_id, id_traits<Type>::invl_range,
				};
			}

			return info;
		}

		bool operator==(const typeinfo& rhs) const noexcept
		{
			return base == rhs.base && invl_range == rhs.invl_range && save == rhs.save;
		}
	};

	// ID value with additional type stored
	class id_key
	{
		u32 m_value = 0; // ID value
		u32 m_base = umax;  // ID base (must be unique for each type in the same container)

	public:
		id_key() noexcept = default;

		id_key(u32 value, u32 type) noexcept
			: m_value(value)
			, m_base(type)
		{
		}

		u32 value() const
		{
			return m_value;
		}

		u32 type() const
		{
			return m_base;
		}

		void clear()
		{
			m_base = umax;
		}

		operator u32() const noexcept
		{
			return m_value;
		}
	};

	template <typename T>
	struct id_map
	{
		static_assert(IdmBaseCompatible<T>, "Please specify IDM compatible type.");

		std::array<stx::atomic_ptr<T>, T::id_count> vec_data{};
		std::array<stx::shared_ptr<T>, T::id_count> private_copy{};
		std::array<id_key, T::id_count> vec_keys{};
		u32 highest_index = 0;

		shared_mutex mutex{}; // TODO: Use this instead of global mutex

		id_map() noexcept = default;

		// Order it directly before the source type's position
		static constexpr double savestate_init_pos_original = T::savestate_init_pos;
		static constexpr double savestate_init_pos = std::bit_cast<double>(std::bit_cast<u64>(savestate_init_pos_original) - 1);

		id_map(utils::serial& ar) noexcept requires IdmSavable<T>
		{
			while (true)
			{
				const u16 tag = serial_breathe_and_tag(ar, g_fxo->get_name<id_map<T>>(), false);

				if (tag >> 15)
				{
					// End
					break;
				}

				// ID, type hash
				const u32 id = ar.pop<u32>();

				const u128 type_init_pos = u128{ar.pop<u32>()} << 64 | std::bit_cast<u64>(T::savestate_init_pos);
				const typeinfo* info = nullptr;

				// Search load functions for the one of this type (see make_typeinfo() for explenation about key composition reasoning)
				for (const auto& typeinfo : get_typeinfo_map())
				{
					if (!(typeinfo.first ^ type_init_pos))
					{
						info = std::addressof(typeinfo.second);
					}
				}

				ensure(info);

				// Construct each object from information collected

				// Simulate construction semantics (idm::last_id() value)
				g_id = id;

				const u32 object_index = get_index(id, info->base, info->step, info->count, info->invl_range);
				auto& obj = ::at32(vec_data, object_index);
				ensure(!obj);

				highest_index = std::max(highest_index, object_index + 1);

				vec_keys[object_index] = id_key(id, static_cast<u32>(static_cast<u64>(type_init_pos >> 64)));
				info->load(ar)(&obj);
			}
		}

		void save(utils::serial& ar) requires IdmSavable<T>
		{
			for (const auto& p : vec_data)
			{
				if (!p) continue;

				auto& key = vec_keys[&p - vec_data.data()];

				const u128 type_init_pos = u128{key.type()} << 64 | std::bit_cast<u64>(T::savestate_init_pos);
				const typeinfo* info = nullptr;

				// Search load functions for the one of this type (see make_typeinfo() for explenation about key composition reasoning)
				for (const auto& typeinfo : get_typeinfo_map())
				{
					if (!(typeinfo.first ^ type_init_pos))
					{
						ensure(!std::exchange(info, std::addressof(typeinfo.second)));
					}
				}

				// Save each object with needed information
				if (info && info->savable(p.observe()))
				{
					// Create a tag for each object
					serial_breathe_and_tag(ar, g_fxo->get_name<id_map<T>>(), false);

					ar(key.value(), key.type());
					info->save(ar, p.observe());
				}
			}

			// End sequence with tag bit set
			serial_breathe_and_tag(ar, g_fxo->get_name<id_map<T>>(), true);
		}

		id_map& operator=(thread_state state) noexcept requires (std::is_assignable_v<T&, thread_state>)
		{
			if (highest_index)
			{
				reader_lock lock(g_mutex);

				// Save all entries
				for (u32 i = 0; i < highest_index; i++)
				{
					private_copy[i] = vec_data[i].load();
				}
			}

			// Signal or join threads
			for (const auto& ptr : private_copy)
			{
				if (ptr)
				{
					*ptr = state;
				}
			}

			return *this;
		}
	};
}

// Object manager for emulated process. Multiple objects of specified arbitrary type are given unique IDs.
class idm
{
	template <typename T>
	static constexpr u32 get_index(u32 id)
	{
		using traits = id_manager::id_traits<T>;

		return id_manager::get_index(id, traits::base, traits::step, traits::count, traits::invl_range);
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

	// Helper type: pointer + return value propagated
	template <typename T, typename RT>
	struct return_pair;

	template <typename T, typename RT>
	struct return_pair<stx::shared_ptr<T>, RT>
	{
		stx::shared_ptr<T> ptr;
		RT ret;

		explicit operator bool() const noexcept
		{
			return ptr.operator bool();
		}

		T& operator*() const noexcept
		{
			return *ptr;
		}

		T* operator->() const noexcept
		{
			return ptr.operator->();
		}
	};

	// Unsafe specialization (not refcounted)
	template <typename T, typename RT>
	struct return_pair<T*, RT>
	{
		T* ptr;
		RT ret;

		explicit operator bool() const noexcept
		{
			return ptr != nullptr;
		}

		T& operator*() const noexcept
		{
			return *ptr;
		}

		T* operator->() const noexcept
		{
			return ptr;
		}
	};

	// Get type ID that is meant to be unique within the same container
	template <typename T>
	static consteval u32 get_type()
	{
		return id_manager::typeinfo::get_type<T>();
	}

	// Prepare new ID (returns nullptr if out of resources)
	static id_manager::id_key* allocate_id(std::span<id_manager::id_key> keys, u32& highest_index, u32 type_id, u32 dst_id, u32 base, u32 step, u32 count, bool uses_lowest_id, std::pair<u32, u32> invl_range);

	// Get object by internal index if exists (additionally check type if types are not equal)
	template <typename T, typename Type>
	static std::pair<atomic_ptr<T>*, id_manager::id_key*> find_index(u32 index, u32 id)
	{
		static_assert(IdmTypesCompatible<T, Type>, "Invalid ID type combination");

		auto& map = g_fxo->get<id_manager::id_map<T>>();

		if (index >= map.highest_index)
		{
			return {};
		}

		auto& data = map.vec_data[index];
		auto& key = map.vec_keys[index];

		if (data)
		{
			if (std::is_same_v<T, Type> || key.type() == get_type<Type>())
			{
				if (!id_manager::id_traits<Type>::invl_range.second || key.value() == id)
				{
					return { &data, &key };
				}
			}
		}

		return {};
	}

	// Find ID
	template <typename T, typename Type>
	static std::pair<atomic_ptr<T>*, id_manager::id_key*> find_id(u32 id)
	{
		static_assert(IdmTypesCompatible<T, Type>, "Invalid ID type combination");

		const u32 index = get_index<Type>(id);

		return find_index<T, Type>(index, id);
	}

	// Allocate new ID (or use fixed ID) and assign the object from the provider()
	template <typename T, typename Type, typename F>
	static stx::shared_ptr<Type> create_id(F&& provider, u32 id = id_manager::id_traits<Type>::invalid)
	{
		static_assert(IdmTypesCompatible<T, Type>, "Invalid ID type combination");

		// ID traits
		using traits = id_manager::id_traits<Type>;

		// Ensure make_typeinfo() is used for this type
		[[maybe_unused]] auto& td = stx::typedata<id_manager::typeinfo, Type>();

		// Allocate new id
		std::lock_guard lock(id_manager::g_mutex);

		auto& map = g_fxo->get<id_manager::id_map<T>>();

		if (auto* key_ptr = allocate_id({map.vec_keys.data(), map.vec_keys.size()}, map.highest_index, get_type<Type>(), id, traits::base, traits::step, traits::count, traits::uses_lowest_id, traits::invl_range))
		{
			auto& place = map.vec_data[key_ptr - map.vec_keys.data()];

			// Get object, store it
			if (auto object = provider())
			{
				place = object;
				return object;
			}

			*key_ptr = {};
		}

		return {};
	}

public:

	// Remove all objects of a type
	template <typename T>
	static inline void clear()
	{
		std::lock_guard lock(id_manager::g_mutex);

		for (auto& ptr : g_fxo->get<id_manager::id_map<T>>().vec_data)
		{
			ptr.reset();
		}

		for (auto& key : g_fxo->get<id_manager::id_map<T>>().vec_keys)
		{
			key.clear();
		}
	}

	// Get last ID (updated in create_id/allocate_id)
	static inline u32 last_id()
	{
		return id_manager::g_id;
	}

	// Add a new ID of specified type with specified constructor arguments (returns object or null_ptr)
	template <typename T, typename Make = T, typename... Args> requires (std::is_constructible_v<Make, Args&&...>)
	static inline stx::shared_ptr<Make> make_ptr(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return stx::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			return pair;
		}

		return null_ptr;
	}

	// Add a new ID of specified type with specified constructor arguments (returns id)
	template <typename T, typename Make = T, typename... Args> requires (std::is_constructible_v<Make, Args&&...>)
	static inline u32 make(Args&&... args)
	{
		if (create_id<T, Make>([&] { return stx::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			return last_id();
		}

		return id_manager::id_traits<Make>::invalid;
	}

	// Add a new ID for an object returned by provider()
	template <typename T, typename Made = T, typename F>
		requires IdmTypesCompatible<T, Made> && std::is_convertible_v<std::invoke_result_t<F&&>, stx::shared_ptr<Made>>
	static inline u32 import(F&& provider, u32 id = id_manager::id_traits<Made>::invalid)
	{
		if (create_id<T, Made>(std::forward<F>(provider), id))
		{
			return last_id();
		}

		return id_manager::id_traits<Made>::invalid;
	}

	// Add a new ID for an existing object provided (returns new id)
	template <typename T, typename Made = T>
		requires IdmTypesCompatible<T, Made>
	static inline u32 import_existing(stx::shared_ptr<Made> ptr, u32 id = id_manager::id_traits<Made>::invalid)
	{
		return import<T, Made>([&]() -> stx::shared_ptr<Made> { return std::move(ptr); }, id);
	}

	// Check the ID without locking (can be called from other method)
	template <typename T, typename Get = T>
		requires IdmTypesCompatible<T, Get>
	static inline Get* check_unlocked(u32 id)
	{
		if (const auto found = find_id<T, Get>(id); found.first)
		{
			return static_cast<Get*>(found.first->observe());
		}

		return nullptr;
	}

	// Check the ID, access object under shared lock
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
		requires IdmTypesCompatible<T, Get>
	static inline std::conditional_t<std::is_void_v<FRT>, Get*, return_pair<Get*, FRT>> check(u32 id, F&& func)
	{
		const u32 index = get_index<Get>(id);

		if (index >= id_manager::id_traits<Get>::count)
		{
			return {};
		}

		reader_lock lock(id_manager::g_mutex);

		if (const auto found = find_index<T, Get>(index, id); found.first)
		{
			const auto ptr = static_cast<Get*>(found.first->observe());

			if constexpr (!std::is_void_v<FRT>)
			{
				return return_pair<Get*, FRT>{ptr, func(*ptr)};
			}
			else
			{
				func(*ptr);
				return ptr;
			}
		}

		return {};
	}

	// Get the object without locking (can be called from other method)
	template <typename T, typename Get = T>
		requires IdmTypesCompatible<T, Get>
	static inline stx::shared_ptr<Get> get_unlocked(u32 id)
	{
		const auto found = find_id<T, Get>(id);

		if (!found.first) [[unlikely]]
		{
			return null_ptr;
		}

		return static_cast<stx::shared_ptr<Get>>(found.first->load());
	}

	// Get the object, access object under reader lock
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
		requires IdmTypesCompatible<T, Get>
	static inline std::conditional_t<std::is_void_v<FRT>, stx::shared_ptr<Get>, return_pair<stx::shared_ptr<Get>, FRT>> get(u32 id, F&& func)
	{
		const u32 index = get_index<Get>(id);

		if (index >= id_manager::id_traits<Get>::count)
		{
			return {};
		}

		reader_lock lock(id_manager::g_mutex);

		const auto found = find_index<T, Get>(index, id);

		if (!found.first) [[unlikely]]
		{
			return {};
		}

		auto ptr = static_cast<stx::shared_ptr<Get>>(found.first->load());
		Get* obj_ptr = ptr.get();

		if constexpr (std::is_void_v<FRT>)
		{
			func(*obj_ptr);
			return ptr;
		}
		else
		{
			return {std::move(ptr), func(*obj_ptr)};
		}
	}

	static constexpr std::false_type unlocked{};

	// Access all objects of specified type. Returns the number of objects processed.
	// If function result evaluates to true, stop and return the object and the value.
	template <typename T, typename... Get, typename F, typename Lock = std::true_type>
		requires IdmBaseCompatible<T> && (IdmCompatible<Get> && ...) && (std::is_invocable_v<F, u32, Get&> && ...)
	static inline auto select(F&& func, Lock = std::true_type{})
	{
		static_assert((IdmTypesCompatible<T, Get> && ...), "Invalid ID type combination");

		[[maybe_unused]] std::conditional_t<!!Lock(), reader_lock, const shared_mutex&> lock(id_manager::g_mutex);

		using func_traits = function_traits<decltype(&decltype(std::function(std::declval<F>()))::operator())>;
		using object_type = typename func_traits::object_type;
		using result_type = typename func_traits::result_type;

		static_assert(PtrSame<object_type, T>, "Invalid function argument type combination");

		std::conditional_t<std::is_void_v<result_type>, u32, return_pair<stx::shared_ptr<object_type>, result_type>> result{};

		auto& map = g_fxo->get<id_manager::id_map<T>>();

		for (auto& id : map.vec_data)
		{
			if (auto ptr = static_cast<object_type*>(id.observe()))
			{
				auto& key = map.vec_keys[&id - map.vec_data.data()];

				if (sizeof...(Get) == 0 || ((key.type() == get_type<Get>()) || ...))
				{
					if constexpr (std::is_void_v<result_type>)
					{
						func(key, *ptr);
						result++;
					}
					else
					{
						if ((result.ret = func(key, *ptr)))
						{
							result.ptr = static_cast<stx::shared_ptr<object_type>>(id.load());
							break;
						}
					}
				}
			}
		}

		return result;
	}

	// Remove the ID
	template <typename T, typename Get = T>
		requires IdmTypesCompatible<T, Get>
	static inline bool remove(u32 id)
	{
		stx::shared_ptr<T> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id); found.first)
			{
				ptr = found.first->exchange(null_ptr);
				found.second->clear();
			}
			else
			{
				return false;
			}
		}

		if constexpr (std::is_assignable_v<Get&, thread_state>)
		{
			if (ptr)
			{
				constexpr thread_state destroying_context{7};
				*static_cast<Get*>(ptr.get()) = destroying_context;
			}
		}

		return true;
	}

	// Remove the ID if matches the weak/shared ptr
	template <typename T, typename Get = T, typename Ptr, typename Lock = std::true_type>
		requires IdmTypesCompatible<T, Get> && std::is_convertible_v<Lock, bool>
	static inline bool remove_verify(u32 id, Ptr&& sptr, Lock = std::true_type{})
	{
		stx::shared_ptr<T> ptr;
		{
			[[maybe_unused]] std::conditional_t<!!Lock(), std::lock_guard<shared_mutex>, const shared_mutex&> lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id); found.first && found.first->is_equal(sptr))
			{
				ptr = found.first->exchange(null_ptr);
				found.second->clear();
			}
			else
			{
				return false;
			}
		}

		if constexpr (std::is_assignable_v<Get&, thread_state>)
		{
			if (ptr)
			{
				constexpr thread_state destroying_context{7};
				*static_cast<Get*>(ptr.get()) = destroying_context;
			}
		}

		return true;
	}

	// Remove the ID and return the object
	template <typename T, typename Get = T, typename Lock = std::true_type>
		requires IdmTypesCompatible<T, Get>
	static inline stx::shared_ptr<Get> withdraw(u32 id, int = 0, Lock = std::true_type{})
	{
		stx::shared_ptr<Get> ptr;
		{
			[[maybe_unused]] std::conditional_t<!!Lock(), std::lock_guard<shared_mutex>, const shared_mutex&> lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id); found.first)
			{
				ptr = static_cast<stx::shared_ptr<Get>>(found.first->exchange(null_ptr));
				found.second->clear();
			}
		}

		return ptr;
	}

	// Remove the ID after accessing the object under writer lock, return the object and propagate return value
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
		requires IdmTypesCompatible<T, Get> && std::is_invocable_v<F, Get&>
	static inline std::conditional_t<std::is_void_v<FRT>, stx::shared_ptr<Get>, return_pair<stx::shared_ptr<Get>, FRT>> withdraw(u32 id, F&& func)
	{
		const u32 index = get_index<Get>(id);

		if (index >= id_manager::id_traits<Get>::count)
		{
			return {};
		}

		std::unique_lock lock(id_manager::g_mutex);

		if (const auto found = find_index<T, Get>(index, id); found.first)
		{
			const auto _ptr = static_cast<Get*>(found.first->observe());

			if constexpr (std::is_void_v<FRT>)
			{
				func(*_ptr);
				found.second->clear();
				return static_cast<stx::shared_ptr<Get>>(found.first->exchange(null_ptr));
			}
			else
			{
				FRT ret = func(*_ptr);

				if (ret)
				{
					// If return value evaluates to true, don't delete the object (error code)
					return {static_cast<stx::shared_ptr<Get>>(found.first->load()), std::move(ret)};
				}

				found.second->clear();
				return {static_cast<stx::shared_ptr<Get>>(found.first->exchange(null_ptr)), std::move(ret)};
			}
		}

		return {};
	}
};
