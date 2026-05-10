#pragma once

#include <util/types.hpp>

namespace rsx
{
	// Generic reverse pointer primitive type for fast reverse iterators
	template <typename Ty>
	class reverse_pointer
	{
		Ty* ptr = nullptr;

	public:
		reverse_pointer() = default;
		reverse_pointer(Ty* val)
			: ptr(val)
		{}

		reverse_pointer& operator++() { ptr--; return *this; }
		reverse_pointer operator++(int) { return reverse_pointer(ptr--); }
		reverse_pointer& operator--() { ptr++; return *this; }
		reverse_pointer operator--(int) { reverse_pointer(ptr++); }

		bool operator == (const reverse_pointer& other) const { return ptr == other.ptr; }

		Ty* operator -> () const { return ptr; }
		Ty& operator * () const { return *ptr; }
	};
}
