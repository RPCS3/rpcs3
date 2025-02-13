#pragma once

#include "util/types.hpp"

#ifndef _MSC_VER
#define ATTR_PURE __attribute__((pure))
#else
#define ATTR_PURE /* nothing available */
#endif

namespace stx
{
	template <typename Info>
	class type_counter;

	// Type information for given Info type, also serving as tag
	template <typename Info>
	class type_info final : public Info
	{
		// Current type id (starts from 0)
		u32 type = umax;

		u32 size = 1;
		u32 align = 1;
		u32 begin = 0;
		double order{};

		// Next typeinfo in linked list
		type_info* next = nullptr;

		// Auxiliary pointer to base type
		const type_info* base = nullptr;

		type_info(Info info, u32 size, u32 align, double order, const type_info* base = nullptr) noexcept;

		friend type_counter<Info>;

	public:
		constexpr type_info() noexcept
			: Info()
		{
		}

		ATTR_PURE u32 index() const
		{
			return type;
		}

		ATTR_PURE u32 pos() const
		{
			return begin;
		}

		ATTR_PURE u32 end() const
		{
			return begin + size;
		}

		ATTR_PURE double init_pos() const
		{
			return order;
		}
	};

	// Class for automatic type registration for given Info type
	template <typename Info>
	class type_counter final
	{
		// Dummy first element to simplify list filling logic
		type_info<Info> first{};

		// Linked list built at global initialization time
		type_info<Info>* next = &first;

		friend type_info<Info>;

	public:
		constexpr type_counter() noexcept = default;

		u32 count() const
		{
			return next->index() + 1;
		}

		u32 align() const
		{
			return first.align;
		}

		u32 size() const
		{
			// Get on first use
			static const u32 sz = [&]()
			{
				u32 result = 0;

				for (auto* ptr = first.next; ptr; ptr = ptr->next)
				{
					result = ((result + ptr->align - 1) & (u32{0} - ptr->align));
					ptr->begin = result;

					result = result + ptr->size;
				}

				return result;
			}();

			return sz;
		}

		class const_iterator
		{
			const type_info<Info>* ptr;

		public:
			const_iterator(const type_info<Info>* ptr)
				: ptr(ptr)
			{
			}

			const type_info<Info>& operator*() const
			{
				return *ptr;
			}

			const type_info<Info>* operator->() const
			{
				return ptr;
			}

			const_iterator& operator++()
			{
				ptr = ptr->next;
				return *this;
			}

			const_iterator operator++(int)
			{
				const_iterator r = ptr;
				ptr = ptr->next;
				return r;
			}

			bool operator==(const const_iterator& r) const
			{
				return ptr == r.ptr;
			}
		};

		const_iterator begin() const
		{
			return first.next;
		}

		const_iterator end() const
		{
			return nullptr;
		}

		// Global type info instance
		template <typename T>
		static const type_info<Info> type;

		// Helper for dynamic types
		template <typename T, typename As>
		static const type_info<Info> dyn_type;
	};

	// Global typecounter instance
	template <typename Info>
	auto& typelist()
	{
		static type_counter<Info> typelist_v;
		return typelist_v;
	}

	// Helper for dynamic types
	template <typename Info>
	auto& dyn_typelist()
	{
		static type_counter<Info> typelist_v;
		return typelist_v;
	}

	template <typename T> requires requires () { T::savestate_init_pos + 0.; }
	constexpr double get_savestate_init_pos()
	{
		return T::savestate_init_pos;
	}
	template <typename T> requires (!(requires () { T::savestate_init_pos + 0.; }))
	constexpr double get_savestate_init_pos()
	{
		return {};
	}

	template <typename Info> template <typename T>
	const type_info<Info> type_counter<Info>::type{Info::template make_typeinfo<T>(), sizeof(T), alignof(T), get_savestate_init_pos<T>()};

	template <typename Info> template <typename T, typename As>
	const type_info<Info> type_counter<Info>::dyn_type{Info::template make_typeinfo<As>(), sizeof(As), alignof(As), get_savestate_init_pos<T>(), &type_counter<Info>::template type<T>};

	template <typename Info>
	type_info<Info>::type_info(Info info, u32 _size, u32 _align, double order, const type_info<Info>* cbase) noexcept
		: Info(info)
	{
		auto& tl = typelist<Info>();

		// Update type info
		this->size = _size > this->size ? _size : this->size;
		this->align = _align > this->align ? _align : this->align;
		this->base = cbase;
		this->order = order;

		// Update global max alignment
		tl.first.align = _align > tl.first.align ? _align : tl.first.align;

		auto& dl = dyn_typelist<Info>();

		if (cbase)
		{
			dl.next->next = this;
			dl.next       = this;

			// Update base max size/align for dynamic types
			for (auto ptr = tl.first.next; ptr; ptr = ptr->next)
			{
				if (cbase == ptr)
				{
					ptr->size = _size > ptr->size ? _size : ptr->size;
					ptr->align = _align > ptr->align ? _align : ptr->align;
					this->type = ptr->type;
				}
			}

			return;
		}

		// Update type index
		this->type = tl.next->type + 1;

		// Update base max size/align for dynamic types
		for (auto ptr = dl.first.next; ptr; ptr = ptr->next)
		{
			if (ptr->base == this)
			{
				this->size = ptr->size > this->size ? ptr->size : this->size;
				this->align = ptr->align > this->align ? ptr->align : this->align;
				ptr->type = this->type;
			}
		}

		// Update linked list
		tl.next->next = this;
		tl.next       = this;
	}

	// Type index accessor
	template <typename Info, typename T, typename As = T>
	ATTR_PURE inline u32 typeindex() noexcept
	{
		static_assert(sizeof(T) > 0);

		if constexpr (std::is_same_v<T, As>)
		{
			return type_counter<Info>::template type<T>.index();
		}
		else
		{
			static_assert(sizeof(As) > 0);
			static_assert(PtrSame<T, As>);
			return type_counter<Info>::template dyn_type<T, As>.index();
		}
	}

	// Type global offset
	template <typename Info, typename T>
	ATTR_PURE inline u32 typeoffset() noexcept
	{
		static_assert(sizeof(T) > 0);

		return type_counter<Info>::template type<T>.pos();
	}

	// Type info accessor
	template <typename Info, typename T, typename As = T>
	ATTR_PURE inline const Info& typedata() noexcept
	{
		static_assert(sizeof(T) > 0 && sizeof(As) > 0);
		static_assert(PtrSame<T, As>); // TODO

		return type_counter<Info>::template dyn_type<T, As>;
	}
}

#undef ATTR_PURE
