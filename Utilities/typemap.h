#pragma once

#include "types.h"
#include "mutex.h"
#include "cond.h"
#include "util/atomic.hpp"
#include "util/typeindices.hpp"
#include "VirtualMemory.h"
#include <memory>

namespace utils
{
	class typemap;

	template <typename T>
	class typeptr;

	class typeptr_base;

	// Special tag for typemap access: request free id
	constexpr struct id_new_t{} id_new{};

	// Special tag for typemap access: unconditionally access the only object (max_count = 1 only)
	constexpr struct id_any_t{} id_any{};

	// Special tag for typemap access: like id_any but also default-construct the object if not exists
	constexpr struct id_always_t{} id_always{};

	// Aggregate with information for more accurate object retrieval, isn't accepted internally
	struct weak_typeptr
	{
		uint id;
		uint type;

		// Stamp isn't automatically stored and checked anywhere
		ullong stamp;
	};

	// Detect id transformation trait (multiplier)
	template <typename T, typename = void>
	struct typeinfo_step
	{
		static constexpr uint step = 1;
	};

	template <typename T>
	struct typeinfo_step<T, std::void_t<decltype(std::decay_t<T>::id_step)>>
	{
		static constexpr uint step = uint{std::decay_t<T>::id_step};
	};

	// Detect id transformation trait (addend)
	template <typename T, typename = void>
	struct typeinfo_bias
	{
		static constexpr uint bias = 0;
	};

	template <typename T>
	struct typeinfo_bias<T, std::void_t<decltype(std::decay_t<T>::id_base)>>
	{
		static constexpr uint bias = uint{std::decay_t<T>::id_base};
	};

	// Detect max number of objects, default = 1
	template <typename T, typename = void>
	struct typeinfo_count
	{
		static constexpr uint max_count = 1;
	};

	template <typename T>
	struct typeinfo_count<T, std::void_t<decltype(std::decay_t<T>::id_count)>>
	{
		static constexpr uint max_count = uint{std::decay_t<T>::id_count};

		static_assert(ullong{max_count} * typeinfo_step<T>::step <= 0x1'0000'0000ull);
	};

	// Detect operator ->
	template <typename T, typename = void>
	struct typeinfo_pointer
	{
		static constexpr bool is_ptr = false;
	};

	template <typename T>
	struct typeinfo_pointer<T, std::void_t<decltype(&std::decay_t<T>::operator->)>>
	{
		static constexpr bool is_ptr = true;
	};

	// Type information
	struct typeinfo_base
	{
		uint size = 0;
		uint align = 0;
		uint count = 0;
		void(*clean)(class typemap_block*) = 0;

		constexpr typeinfo_base() noexcept = default;

		template <typename T>
		static void call_destructor(typemap_block* ptr) noexcept;

		template <typename T>
		static constexpr typeinfo_base make_typeinfo() noexcept
		{
			static_assert(alignof(T) < 4096);

			typeinfo_base r;
			r.size  = uint{sizeof(T)};
			r.align = uint{alignof(T)};
			r.count = typeinfo_count<T>::max_count;
			r.clean = &call_destructor<T>;
			return r;
		}
	};

	// Internal, control block for a particular object
	class typemap_block
	{
		friend typemap;

		template <typename T>
		friend class typeptr;

		friend class typeptr_base;

		shared_mutex m_mutex;
		atomic_t<uint> m_type;
	public:
		typemap_block() = default;

		// Get pointer to the object of type T, with respect to alignment
		template <typename T, uint SelfSize = 8>
		T* get_ptr()
		{
			constexpr uint offset = alignof(T) < SelfSize ? ::align(SelfSize, alignof(T)) : alignof(T);
			return reinterpret_cast<T*>(reinterpret_cast<uchar*>(this) + offset);
		}
	};

	static_assert(std::is_standard_layout_v<typemap_block>);
	static_assert(sizeof(typemap_block) == 8);

	template <typename T>
	void typeinfo_base::call_destructor(typemap_block* ptr) noexcept
	{
		ptr->get_ptr<T>()->~T();
	}

	// An object of type T paired with atomic refcounter
	template <typename T>
	class refctr final
	{
		atomic_t<std::size_t> m_ref{1};

	public:
		T object;

		template <typename... Args>
		refctr(Args&&... args)
			: object(std::forward<Args>(args)...)
		{
		}

		void add_ref() noexcept
		{
			m_ref++;
		}

		std::size_t remove_ref() noexcept
		{
			return --m_ref;
		}
	};

	// Simplified "shared" ptr making use of refctr<T> class
	template <typename T>
	class refptr final
	{
		refctr<T>* m_ptr = nullptr;

		void destroy()
		{
			if (m_ptr && !m_ptr->remove_ref())
				delete m_ptr;
		}

	public:
		constexpr refptr() = default;

		// Construct directly from refctr<T> pointer
		explicit refptr(refctr<T>* ptr) noexcept
			: m_ptr(ptr)
		{
		}

		refptr(const refptr& rhs) noexcept
			: m_ptr(rhs.m_ptr)
		{
			if (m_ptr)
				m_ptr->add_ref();
		}

		refptr(refptr&& rhs) noexcept
			: m_ptr(rhs.m_ptr)
		{
			rhs.m_ptr = nullptr;
		}

		~refptr()
		{
			destroy();
		}

		refptr& operator =(const refptr& rhs) noexcept
		{
			destroy();
			m_ptr = rhs.m_ptr;
			if (m_ptr)
				m_ptr->add_ref();
		}

		refptr& operator =(refptr&& rhs) noexcept
		{
			std::swap(m_ptr, rhs.m_ptr);
		}

		void reset() noexcept
		{
			destroy();
			m_ptr = nullptr;
		}

		refctr<T>* release() noexcept
		{
			return std::exchange(m_ptr, nullptr);
		}

		void swap(refptr&& rhs) noexcept
		{
			std::swap(m_ptr, rhs.m_ptr);
		}

		refctr<T>* get() const noexcept
		{
			return m_ptr;
		}

		T& operator *() const noexcept
		{
			return m_ptr->object;
		}

		T* operator ->() const noexcept
		{
			return &m_ptr->object;
		}

		explicit operator bool() const noexcept
		{
			return !!m_ptr;
		}
	};

	// Internal, typemap control block for a particular type
	struct alignas(64) typemap_head
	{
		// Pointer to the uninitialized storage
		uchar* m_ptr = nullptr;

		// Free ID counter
		atomic_t<uint> m_sema{0};

		// Max ID ever used + 1
		atomic_t<uint> m_limit{0};

		// Increased on each constructor call
		atomic_t<ullong> m_create_count{0};

		// Increased on each destructor call
		atomic_t<ullong> m_destroy_count{0};

		// Aligned size of the storage for each object
		uint m_ssize = 0;

		// Total object count in the storage
		uint m_count = 0;

		// Destructor caller; related to particular type, not the current storage
		void(*clean)(typemap_block*) = 0;
	};

	class typeptr_base
	{
		typemap_head* m_head;
		typemap_block* m_block;

		template <typename T>
		friend class typeptr;

		friend typemap;
	};

	// Pointer + lock object, possible states:
	// 1) Invalid - bad id, no space, or after release()
	// 2) Null - locked, but the object does not exist
	// 3) OK - locked and the object exists
	template <typename T>
	class typeptr : typeptr_base
	{
		using typeptr_base::m_head;
		using typeptr_base::m_block;

		friend typemap;

		void release()
		{
			if constexpr (type_const() && type_volatile())
			{
			}
			else if constexpr (type_const() || type_volatile())
			{
				m_block->m_mutex.unlock_shared();
			}
			else
			{
				m_block->m_mutex.unlock();
			}

			if (m_block->m_type == 0)
			{
				if constexpr (typeinfo_count<T>::max_count > 1)
				{
					// Return semaphore
					m_head->m_sema--;
				}
			}
		}

	public:
		constexpr typeptr(typeptr_base base) noexcept
			: typeptr_base(base)
		{
		}

		typeptr(const typeptr&) = delete;

		typeptr& operator=(const typeptr&) = delete;

		~typeptr()
		{
			if (m_block)
			{
				release();
			}
		}

		// Verify the object exists
		bool exists() const noexcept
		{
			return m_block->m_type != 0;
		}

		// Verify the state is valid
		explicit operator bool() const noexcept
		{
			return m_block != nullptr;
		}

		// Get the pointer to the existing object
		template <typename D = std::remove_reference_t<T>>
		auto get() const noexcept
		{
			ASSUME(m_block->m_type != 0);
			return m_block->get_ptr<T>();
		}

		auto operator->() const noexcept
		{
			// Invoke object's operator -> if available
			if constexpr (typeinfo_pointer<T>::is_ptr)
			{
				return get()->operator->();
			}
			else
			{
				return get();
			}
		}

		// Release the lock and set invalid state
		void unlock()
		{
			if (m_block)
			{
				release();
				m_block = nullptr;
			}
		}

		// Call the constructor, return the stamp
		template <typename New = std::decay_t<T>, typename... Args>
		ullong create(Args&&... args)
		{
			static_assert(!type_const());
			static_assert(!type_volatile());

			const ullong result = ++m_head->m_create_count;

			if constexpr (typeinfo_count<T>::max_count > 1)
			{
				// Update hints only if the object is not being recreated
				if (!m_block->m_type)
				{
					const uint this_id = this->get_id();

					// Update max count
					m_head->m_limit.fetch_op([this_id](uint& limit)
					{
						if (limit <= this_id)
						{
							limit = this_id + 1;
							return true;
						}

						return false;
					});
				}
			}

			if constexpr (true)
			{
				static_assert(std::is_same_v<New, T>);

				// Set type; zero value shall not be observed in the case of recreation
				if (m_block->m_type.exchange(1) != 0)
				{
					// Destroy object if it exists
					m_block->get_ptr<T>()->~T();
					m_head->m_destroy_count++;
				}

				new (m_block->get_ptr<New>()) New(std::forward<Args>(args)...);
			}

			return result;
		}

		// Call the destructor if object exists
		void destroy() noexcept
		{
			static_assert(!type_const());

			if (!m_block->m_type.exchange(0))
			{
				return;
			}

			m_block->get_ptr<T>()->~T();
			m_head->m_destroy_count++;
		}

		// Get the ID
		uint get_id() const
		{
			// It's not often needed so figure it out instead of storing it
			const std::size_t diff = reinterpret_cast<uchar*>(m_block) - m_head->m_ptr;
			const std::size_t quot = diff / m_head->m_ssize;

			if (diff % m_head->m_ssize || quot > typeinfo_count<T>::max_count)
			{
				return -1;
			}

			constexpr uint bias = typeinfo_bias<T>::bias;
			constexpr uint step = typeinfo_step<T>::step;
			return static_cast<uint>(quot) * step + bias;
		}

		static constexpr bool type_const()
		{
			return std::is_const_v<std::remove_reference_t<T>>;
		}

		static constexpr bool type_volatile()
		{
			return std::is_volatile_v<std::remove_reference_t<T>>;
		}
	};

	// Dynamic object collection, one or more per any type; shall not be initialized before main()
	class typemap
	{
		// Pointer to the dynamic array
		typemap_head* m_map = nullptr;

		// Pointer to the virtual memory
		void* m_memory = nullptr;

		// Virtual memory size
		std::size_t m_total = 0;

		template <typename T>
		typemap_head* get_head() const
		{
			return &m_map[stx::typeindex<typeinfo_base, std::decay_t<T>>()];
		}

	public:
		typemap(const typemap&) = delete;

		typemap& operator=(const typemap&) = delete;

		// Construct without initialization (suitable for global typemap)
		explicit constexpr typemap(std::nullptr_t) noexcept
		{
		}

		// Construct with initialization
		typemap()
		{
			init();
		}

		~typemap()
		{
			delete[] m_map;

			if (m_memory)
			{
				utils::memory_release(m_memory, m_total);
			}
		}

		// Recreate, also required if constructed without initialization.
		void init()
		{
			if (!stx::typelist_v<typeinfo_base>.count())
			{
				return;
			}

			// Recreate and copy some type information
			if (m_map == nullptr)
			{
				m_map = new typemap_head[stx::typelist_v<typeinfo_base>.count()]();
			}
			else
			{
				auto type = stx::typelist_v<typeinfo_base>.begin();
				auto _end = stx::typelist_v<typeinfo_base>.end();

				for (uint i = 0; type != _end; i++, ++type)
				{
					// Delete objects (there shall be no threads accessing them)
					const uint lim = m_map[i].m_count != 1 ? +m_map[i].m_limit : 1;

					for (std::size_t j = 0; j < lim; j++)
					{
						const auto block = reinterpret_cast<typemap_block*>(m_map[i].m_ptr + j * m_map[i].m_ssize);

						if (block->m_type)
						{
							m_map[i].clean(block);
						}
					}

					// Reset mutable fields
					m_map[i].m_sema.raw()  = 0;
					m_map[i].m_limit.raw() = 0;

					m_map[i].m_create_count.raw()  = 0;
					m_map[i].m_destroy_count.raw() = 0;
				}
			}

			// Initialize virtual memory if necessary
			if (m_memory == nullptr)
			{
				// Determine total size, copy typeinfo
				auto type = stx::typelist_v<typeinfo_base>.begin();
				auto _end = stx::typelist_v<typeinfo_base>.end();

				for (uint i = 0; type != _end; i++, ++type)
				{
					const uint align = type->align;
					const uint ssize = ::align<uint>(sizeof(typemap_block), align) + ::align(type->size, align);
					const auto total = std::size_t{ssize} * type->count;
					const auto start = std::uintptr_t{::align(m_total, align)};

					if (total)
					{
						// Move forward hoping there are no usable gaps wasted
						m_total = start + total;

						// Store storage size and object count
						m_map[i].m_ssize = ssize;
						m_map[i].m_count = type->count;
						m_map[i].m_ptr   = reinterpret_cast<uchar*>(start);
					}

					// Copy destructor for indexed access
					m_map[i].clean = type->clean;
				}

				// Allocate virtual memory
				m_memory = utils::memory_reserve(m_total);
				utils::memory_commit(m_memory, m_total);

				// Update pointers
				for (uint i = 0, n = stx::typelist_v<typeinfo_base>.count(); i < n; i++)
				{
					if (m_map[i].m_count)
					{
						m_map[i].m_ptr = static_cast<uchar*>(m_memory) + reinterpret_cast<std::uintptr_t>(m_map[i].m_ptr);
					}
				}
			}
			else
			{
				// Reinitialize virtual memory at the same location
				utils::memory_reset(m_memory, m_total);
			}
		}

		// Return allocated virtual memory block size (not aligned)
		std::size_t get_memory_size() const
		{
			return m_total;
		}

	private:

		// Prepare pointers
		template <typename Type, typename Arg>
		typeptr_base init_ptr(Arg&& id) const
		{
			if constexpr (typeinfo_count<Type>::max_count == 0)
			{
				return {};
			}

			using id_tag = std::decay_t<Arg>;

			typemap_head* head = get_head<Type>();
			typemap_block* block;

			if constexpr (std::is_same_v<id_tag, id_new_t> || std::is_same_v<id_tag, id_any_t> || std::is_same_v<id_tag, id_always_t>)
			{
				if constexpr (constexpr uint last = typeinfo_count<Type>::max_count - 1)
				{
					// If max_count > 1 only id_new is supported
					static_assert(std::is_same_v<id_tag, id_new_t>);
					static_assert(!std::is_const_v<std::remove_reference_t<Type>>);
					static_assert(!std::is_volatile_v<std::remove_reference_t<Type>>);

					// Try to acquire the semaphore
					if (!head->m_sema.try_inc(last + 1)) [[unlikely]]
					{
						block = nullptr;
					}
					else
					{
						// Find empty location and lock it, starting from hint index
						for (uint lim = head->m_limit, i = (lim > last ? 0 : lim);; i = (i == last ? 0 : i + 1))
						{
							block = reinterpret_cast<typemap_block*>(head->m_ptr + std::size_t{i} * head->m_ssize);

							if (block->m_type == 0 && block->m_mutex.try_lock())
							{
								if (block->m_type == 0) [[likely]]
								{
									break;
								}

								block->m_mutex.unlock();
							}
						}
					}
				}
				else
				{
					// Always access first element
					block = reinterpret_cast<typemap_block*>(head->m_ptr);

					if constexpr (std::is_same_v<id_tag, id_new_t>)
					{
						static_assert(!std::is_const_v<std::remove_reference_t<Type>>);
						static_assert(!std::is_volatile_v<std::remove_reference_t<Type>>);

						if (block->m_type != 0 || !block->m_mutex.try_lock())
						{
							block = nullptr;
						}
						else if (block->m_type != 0) [[unlikely]]
						{
							block->m_mutex.unlock();
							block = nullptr;
						}
					}
				}
			}
			else if constexpr (std::is_invocable_r_v<bool, const Arg&, const Type&>)
			{
				// Access with a lookup function
				for (std::size_t j = 0; j < (typeinfo_count<Type>::max_count != 1 ? +head->m_limit : 1); j++)
				{
					block = reinterpret_cast<typemap_block*>(head->m_ptr + j * head->m_ssize);

					if (block->m_type)
					{
						std::lock_guard lock(block->m_mutex);

						if (block->m_type)
						{
							if (std::invoke(std::forward<Arg>(id), std::as_const(*block->get_ptr<Type>())))
							{
								break;
							}
						}
					}

					block = nullptr;
				}
			}
			else
			{
				// Access by transformed id
				constexpr uint bias = typeinfo_bias<Type>::bias;
				constexpr uint step = typeinfo_step<Type>::step;
				const uint unbiased = static_cast<uint>(std::forward<Arg>(id)) - bias;
				const uint unscaled = unbiased / step;

				block = reinterpret_cast<typemap_block*>(head->m_ptr + std::size_t{head->m_ssize} * unscaled);

				// Check id range and type
				if (unscaled >= typeinfo_count<Type>::max_count || unbiased % step) [[unlikely]]
				{
					block = nullptr;
				}
				else
				{
					if (block->m_type == 0) [[unlikely]]
					{
						block = nullptr;
					}
				}
			}

			typeptr_base result;
			result.m_head  = head;
			result.m_block = block;
			return result;
		}

		template <typename Type, typename Arg>
		void check_ptr(typemap_block*& block, Arg&& id) const
		{
			using id_tag = std::decay_t<Arg>;

			if constexpr (std::is_same_v<id_tag, id_new_t>)
			{
				// No action for id_new
				return;
			}
			else if constexpr (std::is_same_v<id_tag, id_any_t>)
			{
				// No action for id_any
				return;
			}
			else if constexpr (std::is_same_v<id_tag, id_always_t>)
			{
				if (block->m_type == 0 && block->m_type.compare_and_swap_test(0, 1))
				{
					// Initialize object if necessary
					static_assert(!std::is_const_v<std::remove_reference_t<Type>>);
					static_assert(!std::is_volatile_v<std::remove_reference_t<Type>>);
					new (block->get_ptr<Type>) Type();
				}

				return;
			}
			else if constexpr (std::is_invocable_r_v<bool, const Arg&, const Type&>)
			{
				if (!block) [[unlikely]]
				{
					return;
				}

				if (block->m_type) [[likely]]
				{
					if (std::invoke(std::forward<Arg>(id), std::as_const(*block->get_ptr<Type>())))
					{
						return;
					}
				}
			}
			else if (block)
			{
				if (block->m_type) [[likely]]
				{
					return;
				}
			}
			else
			{
				return;
			}

			// Fallback: unlock and invalidate
			block->m_mutex.unlock();
			block = nullptr;
		}

		template <bool Try, typename Type, bool Lock>
		bool lock_ptr(typemap_block* block) const
		{
			// Use reader lock for const access
			constexpr bool is_const = std::is_const_v<std::remove_reference_t<Type>>;
			constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<Type>>;

			// Already locked or lock is unnecessary
			if constexpr (!Lock)
			{
				return true;
			}
			else
			{
				// Skip failed ids
				if (!block)
				{
					return true;
				}

				if constexpr (Try)
				{
					if constexpr (is_const || is_volatile)
					{
						return block->m_mutex.try_lock_shared();
					}
					else
					{
						return block->m_mutex.try_lock();
					}
				}
				else if constexpr (is_const || is_volatile)
				{
					if (block->m_mutex.is_lockable()) [[likely]]
					{
						return true;
					}

					block->m_mutex.lock_shared();
					return false;
				}
				else
				{
					if (block->m_mutex.is_free()) [[likely]]
					{
						return true;
					}

					block->m_mutex.lock();
					return false;
				}
			}
		}

		template <std::size_t I, typename Type, typename... Types, bool Lock, bool... Locks, std::size_t N>
		bool try_lock(const std::array<typeptr_base, N>& array, uint locked, std::integer_sequence<bool, Lock, Locks...>) const
		{
			// Try to lock mutex if not locked from the previous step
			if (I == locked || lock_ptr<true, Type, Lock>(array[I].m_block))
			{
				if constexpr (I + 1 < N)
				{
					// Proceed recursively
					if (try_lock<I + 1, Types...>(array, locked, std::integer_sequence<bool, Locks...>{})) [[likely]]
					{
						return true;
					}

					// Retire: unlock everything, including (I == locked) case
					if constexpr (Lock)
					{
						if (array[I].m_block)
						{
							if constexpr (std::is_const_v<std::remove_reference_t<Type>> || std::is_volatile_v<std::remove_reference_t<Type>>)
							{
								array[I].m_block->m_mutex.unlock_shared();
							}
							else
							{
								array[I].m_block->m_mutex.unlock();
							}
						}
					}
				}
				else
				{
					return true;
				}
			}

			return false;
		}

		template <typename... Types, std::size_t N, std::size_t... I, bool... Locks>
		uint lock_array(const std::array<typeptr_base, N>& array, std::integer_sequence<std::size_t, I...>, std::integer_sequence<bool, Locks...>) const
		{
			// Verify all mutexes are free or wait for one of them and return its index
			uint locked = 0;
			((lock_ptr<false, Types, Locks>(array[I].m_block) && ++locked) && ...);
			return locked;
		}

		template <typename... Types, std::size_t N, std::size_t... I, typename... Args>
		void check_array(std::array<typeptr_base, N>& array, std::integer_sequence<std::size_t, I...>, Args&&... ids) const
		{
			// Check types and unlock on mismatch
			(check_ptr<Types, Args>(array[I].m_block, std::forward<Args>(ids)), ...);
		}

		template <typename... Types, std::size_t N, std::size_t... I>
		std::tuple<typeptr<Types>...> array_to_tuple(const std::array<typeptr_base, N>& array, std::integer_sequence<std::size_t, I...>) const
		{
			return {array[I]...};
		}

		template <typename T, typename Arg>
		static constexpr bool does_need_lock()
		{
			if constexpr (std::is_same_v<std::decay_t<Arg>, id_new_t>)
			{
				return false;
			}

			if constexpr (std::is_const_v<std::remove_reference_t<T>> && std::is_volatile_v<std::remove_reference_t<T>>)
			{
				return false;
			}

			return true;
		}

		// Transform T&& into refptr<T>, moving const qualifier from T to refptr<T>
		template <typename T, typename U = std::remove_reference_t<T>>
		using decode_t = std::conditional_t<!std::is_rvalue_reference_v<T>, T,
			std::conditional_t<std::is_const_v<U>, const refptr<std::remove_const_t<U>>, refptr<U>>>;

	public:
		// Lock any objects by their identifiers, special tags id_new/id_any/id_always, or search predicates
		template <typename... Types, typename... Args, typename = std::enable_if_t<sizeof...(Types) == sizeof...(Args)>>
		auto lock(Args&&... ids) const
		{
			static_assert(((!std::is_lvalue_reference_v<Types>) && ...));
			static_assert(((!std::is_array_v<Types>) && ...));
			static_assert(((!std::is_void_v<Types>) && ...));

			// Initialize pointers
			std::array<typeptr_base, sizeof...(Types)> result{this->init_ptr<decode_t<Types>>(std::forward<Args>(ids))...};

			// Whether requires locking after init_ptr
			using locks_t = std::integer_sequence<bool, does_need_lock<decode_t<Types>, Args>()...>;

			// Array index helper
			using seq_t = std::index_sequence_for<decode_t<Types>...>;

			// Lock any number of objects in safe manner
			while (true)
			{
				const uint locked = lock_array<decode_t<Types>...>(result, seq_t{}, locks_t{});
				if (try_lock<0, decode_t<Types>...>(result, locked, locks_t{})) [[likely]]
					break;
			}

			// Verify object types
			check_array<decode_t<Types>...>(result, seq_t{}, std::forward<Args>(ids)...);

			// Return tuple of possibly locked pointers, or a single pointer
			if constexpr (sizeof...(Types) != 1)
			{
				return array_to_tuple<decode_t<Types>...>(result, seq_t{});
			}
			else
			{
				return typeptr<decode_t<Types>...>(result[0]);
			}
		}

		// Apply a function to all objects of one or more types
		template <typename Type, typename... Types, typename F>
		ullong apply(F&& func)
		{
			static_assert(!std::is_lvalue_reference_v<Type>);
			static_assert(!std::is_array_v<Type>);
			static_assert(!std::is_void_v<Type>);

			typemap_head* head = get_head<decode_t<Type>>();

			const ullong ix = head->m_create_count;

			for (std::size_t j = 0; j < (typeinfo_count<decode_t<Type>>::max_count != 1 ? +head->m_limit : 1); j++)
			{
				const auto block = reinterpret_cast<typemap_block*>(head->m_ptr + j * head->m_ssize);

				if (block->m_type)
				{
					std::lock_guard lock(block->m_mutex);

					if (block->m_type)
					{
						std::invoke(std::forward<F>(func), *block->get_ptr<decode_t<Type>>());
					}
				}
			}

			// Return "unsigned negative" value if the creation index has increased
			const ullong result = ix - head->m_create_count;

			if constexpr (sizeof...(Types) > 0)
			{
				return (result + ... + apply<Types>(func));
			}
			else
			{
				return result;
			}
		}

		template <typename Type>
		ullong get_create_count() const
		{
			return get_head<Type>()->m_create_count;
		}

		template <typename Type>
		ullong get_destroy_count() const
		{
			return get_head<Type>()->m_destroy_count;
		}
	};
} // namespace utils
