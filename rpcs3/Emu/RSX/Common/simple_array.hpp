#pragma once

#include <util/types.hpp>
#include <functional>

namespace rsx
{
	template <typename Ty> requires std::is_trivially_destructible_v<Ty>
	struct simple_array
	{
	public:
		using iterator = Ty*;
		using const_iterator = const Ty*;
		using value_type = Ty;

	private:
		u32 _capacity = 0;
		u32 _size = 0;
		Ty* _data = nullptr;

		inline u64 offset(const_iterator pos)
		{
			return (_data) ? u64(pos - _data) : 0ull;
		}

	public:
		simple_array() = default;

		simple_array(u32 initial_size)
		{
			reserve(initial_size);
			_size = initial_size;
		}

		simple_array(u32 initial_size, const Ty val)
		{
			reserve(initial_size);
			_size = initial_size;

			for (u32 n = 0; n < initial_size; ++n)
			{
				_data[n] = val;
			}
		}

		simple_array(const std::initializer_list<Ty>& args)
		{
			reserve(::size32(args));

			for (const auto& arg : args)
			{
				push_back(arg);
			}
		}

		simple_array(const simple_array& other)
		{
			_capacity = other._capacity;
			_size = other._size;

			const auto size_bytes = sizeof(Ty) * _capacity;
			_data = static_cast<Ty*>(malloc(size_bytes));
			std::memcpy(_data, other._data, size_bytes);
		}

		simple_array(simple_array&& other) noexcept
		{
			swap(other);
		}

		simple_array& operator=(const simple_array& other)
		{
			if (&other != this)
			{
				simple_array{ other }.swap(*this);
			}

			return *this;
		}

		simple_array& operator=(simple_array&& other) noexcept
		{
			swap(other);
			return *this;
		}

		~simple_array()
		{
			if (_data)
			{
				free(_data);
				_data = nullptr;
				_size = _capacity = 0;
			}
		}

		void swap(simple_array<Ty>& other) noexcept
		{
			std::swap(_capacity, other._capacity);
			std::swap(_size, other._size);
			std::swap(_data, other._data);
		}

		void reserve(u32 size)
		{
			if (_capacity >= size)
				return;

			ensure(_data = static_cast<Ty*>(std::realloc(_data, sizeof(Ty) * size))); // "realloc() failed!"
			_capacity = size;
		}

		template <typename T> requires UnsignedInt<T>
		void resize(T size)
		{
			const auto new_size = static_cast<u32>(size);
			reserve(new_size);
			_size = new_size;
		}

		void push_back(const Ty& val)
		{
			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
			}

			_data[_size++] = val;
		}

		void push_back(Ty&& val)
		{
			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
			}

			_data[_size++] = val;
		}

		template <typename... Args>
		void emplace_back(Args&&... args)
		{
			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
			}

			std::construct_at(&_data[_size++], std::forward<Args&&>(args)...);
		}

		Ty pop_back()
		{
			return _data[--_size];
		}

		iterator insert(iterator pos, const Ty& val)
		{
			ensure(pos >= _data);
			const auto _loc = offset(pos);

			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
				pos = _data + _loc;
			}

			if (_loc >= _size)
			{
				_data[_size++] = val;
				return pos;
			}

			ensure(_loc < _size);

			const auto remaining = (_size - _loc);
			memmove(pos + 1, pos, remaining * sizeof(Ty));

			*pos = val;
			_size++;

			return pos;
		}

		iterator insert(iterator pos, Ty&& val)
		{
			ensure(pos >= _data);
			const auto _loc = offset(pos);

			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
				pos = _data + _loc;
			}

			if (_loc >= _size)
			{
				_data[_size++] = val;
				return pos;
			}

			ensure(_loc < _size);

			const u32 remaining = (_size - _loc);
			memmove(pos + 1, pos, remaining * sizeof(Ty));

			*pos = val;
			_size++;

			return pos;
		}

		void clear()
		{
			_size = 0;
		}

		bool empty() const
		{
			return _size == 0;
		}

		u32 size() const
		{
			return _size;
		}

		u64 size_bytes() const
		{
			return _size * sizeof(Ty);
		}

		u32 capacity() const
		{
			return _capacity;
		}

		Ty& operator[] (u32 index)
		{
			return _data[index];
		}

		const Ty& operator[] (u32 index) const
		{
			return _data[index];
		}

		Ty* data()
		{
			return _data;
		}

		const Ty* data() const
		{
			return _data;
		}

		Ty& back()
		{
			return _data[_size - 1];
		}

		const Ty& back() const
		{
			return _data[_size - 1];
		}

		Ty& front()
		{
			return _data[0];
		}

		const Ty& front() const
		{
			return _data[0];
		}

		iterator begin()
		{
			return _data;
		}

		iterator end()
		{
			return _data ? _data + _size : nullptr;
		}

		const_iterator begin() const
		{
			return _data;
		}

		const_iterator end() const
		{
			return _data ? _data + _size : nullptr;
		}

		bool any(std::predicate<const Ty&> auto predicate) const
		{
			for (auto it = begin(); it != end(); ++it)
			{
				if (std::invoke(predicate, *it))
				{
					return true;
				}
			}
			return false;
		}

		void filter(std::predicate<const Ty&> auto predicate)
		{
			if (!_size)
			{
				return;
			}

			for (auto ptr = _data, last = _data + _size - 1; ptr < last; ptr++)
			{
				if (!predicate(*ptr))
				{
					// Move item to the end of the list and shrink by 1
					std::memcpy(ptr, last, sizeof(Ty));
					last = _data + (--_size);
				}
			}
		}

		void sort(std::predicate<const Ty&, const Ty&> auto predicate)
		{
			if (_size < 2)
			{
				return;
			}

			std::sort(begin(), end(), predicate);
		}
	};
}
