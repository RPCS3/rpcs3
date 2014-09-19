#pragma once

namespace vm
{
	template<typename T, size_t size = sizeof(T)>
	struct _to_atomic
	{
		static_assert(size == 4 || size == 8, "Invalid atomic type");

		typedef T type;
	};

	template<typename T>
	struct _to_atomic<T, 4>
	{
		typedef uint32_t type;
	};

	template<typename T>
	struct _to_atomic<T, 8>
	{
		typedef uint64_t type;
	};

	template<typename T>
	class _atomic_base
	{
		T data;
		typedef typename _to_atomic<T, sizeof(T)>::type atomic_type;

	public:
		T compare_and_swap(T cmp, T exch)
		{
			const atomic_type res = InterlockedCompareExchange((volatile atomic_type*)&data, (atomic_type&)exch, (atomic_type&)cmp);
			return (T&)res;
		}

	};

	template<typename T> struct atomic_le : public _atomic_base<T>
	{
	};

	template<typename T> struct atomic_be : public _atomic_base<typename to_be_t<T>::type>
	{
	};

	namespace ps3
	{
		template<typename T> struct atomic : public atomic_be<T>
		{
		};
	}

	namespace psv
	{
		template<typename T> struct atomic : public atomic_le<T>
		{
		};
	}

	using namespace ps3;
}