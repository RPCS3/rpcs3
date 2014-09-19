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
		volatile T data;
		typedef typename _to_atomic<T, sizeof(T)>::type atomic_type;

	public:
		__forceinline const T compare_and_swap(const T cmp, const T exch) volatile
		{
			const atomic_type res = InterlockedCompareExchange((volatile atomic_type*)&data, (atomic_type&)exch, (atomic_type&)cmp);
			return (T&)res;
		}

		__forceinline const T exchange(const T value) volatile
		{
			const atomic_type res = InterlockedExchange((volatile atomic_type*)&data, (atomic_type&)value);
			return (T&)res;
		}

		__forceinline const T read_relaxed() const volatile
		{
			return (T&)data;
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