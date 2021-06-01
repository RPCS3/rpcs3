#pragma once

#include "util/types.hpp"
#include <vector>

namespace stx
{
	template <typename T>
	struct exact_t
	{
		T obj;

		exact_t(T&& _obj) : obj(std::forward<T>(_obj)) {}

		// TODO: More conversions
		template <typename U> requires (std::is_same_v<U&, T>)
		operator U&() const { return obj; };
	};
}

namespace utils
{
	template <typename T>
	concept FastRandomAccess = requires (T& obj)
	{
		std::data(obj)[0];
	};

	template <typename T>
	concept Reservable = requires (T& obj)
	{
		obj.reserve(0);
	};

	template <typename T>
	concept Bitcopy = (std::is_arithmetic_v<T>) || (std::is_enum_v<T>) || requires (T& obj)
	{
		std::enable_if_t<static_cast<bool>(std::remove_cv_t<T>::enable_bitcopy)>();
	};

	template <typename T>
	concept TupleAlike = requires ()
	{
		std::tuple_size<std::remove_cv_t<T>>::value;
	};

	template <typename T>
	concept ListAlike = requires (T& obj) { obj.insert(obj.end(), std::declval<typename T::value_type>()); };

	struct serial
	{
		std::vector<u8> data;
		usz pos = umax;

		// Checks if this strcuture is currently used for serialization
		bool is_writing() const
		{
			return pos == umax;
		}

		// Reserve memory for serialization
		void reserve(usz size)
		{
			// Is a NO-OP for deserialization in order to allow usage from serialization specializations regardless
			if (is_writing())
			{
				data.reserve(data.size() + size);
			}
		}

		bool raw_serialize(const void* ptr, usz size)
		{
			if (is_writing())
			{
				data.insert(data.end(), static_cast<const u8*>(ptr), static_cast<const u8*>(ptr) + size);
				return true;
			}

			ensure(data.size() - pos >= size);
			std::memcpy(const_cast<void*>(ptr), data.data() + pos, size);
			pos += size;
			return true;
		}

		// (De)serialization function
		template <typename T>
		bool serialize(T& obj)
		{
			// Fallback to global overload
			return ::serialize(*this, obj);
		}

		// Enabled for fundamental types, enumerations and if specfied explicitly that type can be saved in pure bitwise manner
		template <typename T> requires Bitcopy<T>
		bool serialize(T& obj)
		{
			return raw_serialize(std::addressof(obj), sizeof(obj));
		}

		// std::vector, std::basic_string
		template <typename T> requires FastRandomAccess<T> && ListAlike<T>
		bool serialize(T& obj)
		{
			if (is_writing())
			{
				for (usz i = obj.size();;)
				{
					const usz i_old = std::exchange(i, i >> 7);
					operator()<u8>(static_cast<u8>(i_old % 0x80) | (u8{!!i} << 7));
					if (!i)
						break;
				}

				if constexpr (std::is_trivially_copyable_v<typename std::remove_reference_t<T>::value_type>)
				{
					raw_serialize(obj.data(), sizeof(obj[0]) * obj.size());
				}
				else
				{
					for (auto&& value : obj)
					{
						if (!serialize(value))
						{
							return false;
						}
					}
				}

				return true;
			}

			obj.clear();

			usz size = 0;

			for (u32 i = 0;; i += 7)
			{
				u8 byte_data = 0;

				if (!raw_serialize(&byte_data, 1))
				{
					return false;
				}

				size |= static_cast<usz>(byte_data % 0x80) << i;

				if (!(byte_data >> 7))
				{
					break;
				}
			}

			obj.resize(size);

			if constexpr (std::is_trivially_copyable_v<typename T::value_type>)
			{
				if (!raw_serialize(obj.data(), sizeof(obj[0]) * size))
				{
					obj.clear();
					return false;
				}
			}
			else
			{
				for (auto&& value : obj)
				{
					if (!serialize(value))
					{
						obj.clear();
						return false;
					}
				}
			}

			return true;
		}

		// C-array, std::array, std::span
		template <typename T> requires FastRandomAccess<T> && (!ListAlike<T>) && (!Bitcopy<T>)
		bool serialize(T& obj)
		{
			if constexpr (std::is_trivially_copyable_v<std::remove_reference_t<decltype(std::declval<T>()[0])>>)
			{
				return raw_serialize(std::data(obj), sizeof(obj[0]) * std::size(obj));
			}
			else
			{
				for (auto&& value : obj)
				{
					if (!serialize(value))
					{
						return false;
					}
				}

				return true;
			}
		}

		// std::deque, std::list, std::(unordered_)set, std::(unordered_)map, std::(unordered_)multiset, std::(unordered_)multimap
		template <typename T> requires (!FastRandomAccess<T>) && ListAlike<T>
		bool serialize(T& obj)
		{
			if (is_writing())
			{
				for (usz i = obj.size();;)
				{
					const usz i_old = std::exchange(i, i >> 7);
					operator()<u8>(static_cast<u8>(i_old % 0x80) | (u8{!!i} << 7));
					if (!i)
						break;
				}

				for (auto&& value : obj)
				{
					if (!serialize(value))
					{
						return false;
					}
				}

				return true;
			}

			obj.clear();

			usz size = 0;

			for (u32 i = 0;; i += 7)
			{
				u8 byte_data = 0;

				if (!raw_serialize(&byte_data, 1))
				{
					return false;
				}

				size |= static_cast<usz>(byte_data % 0x80) << i;

				if (!(byte_data >> 7))
				{
					break;
				}
			}

			if constexpr (Reservable<T>)
			{
				obj.reserve(size);
			}

			for (usz i = 0; i < size; i++)
			{
				obj.insert(obj.end(), static_cast<typename T::value_type>(*this));

				if (!is_valid())
				{
					obj.clear();
					return false;
				}
			}

			return true;
		}

		template <usz i = 0, typename T>
		bool serialize_tuple(T& obj)
		{
			const bool res = serialize(std::get<i>(obj));
			constexpr usz next_i = std::min<usz>(i + 1, std::tuple_size_v<T> - 1);

			if constexpr (next_i == i)
			{
				return res;
			}
			else
			{
				return res && serialize_tuple<next_i>(obj);
			}
		}

		// std::pair, std::tuple
		template <typename T> requires TupleAlike<T> && (!FastRandomAccess<T>)
		bool serialize(T& obj)
		{
			return serialize_tuple(obj);
		}

		// Wrapper for serialize(T&), allows to pass multiple objects at once
		template <typename... Args>
		bool operator()(Args&&... args)
		{
			return ((AUDIT(!std::is_const_v<std::remove_reference_t<Args>> || is_writing())
				, serialize(const_cast<Args&>(static_cast<const Args&>(args)))), ...);
		}

		// Convert serialization manager to deserializion manager (can't go the other way)
		// If no arg provided reuse saved buffer
		void set_reading_state(std::vector<u8>&& _data = std::vector<u8>{})
		{
			if (!_data.empty())
			{
				data = std::move(_data);
			}

			pos = 0;
		}

		template <typename T> requires (std::is_constructible_v<std::remove_const_t<T>> || Bitcopy<std::remove_const_t<T>> ||
			std::is_constructible_v<std::remove_const_t<T>, stx::exact_t<serial&>> || TupleAlike<std::remove_const_t<T>>)
		operator T()
		{
			AUDIT(!is_writing());

			using type = std::remove_const_t<T>;

			if constexpr (Bitcopy<T>)
			{
				u8 buf[sizeof(type)]{};
				ensure(raw_serialize(buf, sizeof(buf)));
				return std::bit_cast<type>(buf);
			}
			else if constexpr (std::is_constructible_v<type, stx::exact_t<serial&>>)
			{
				return type(stx::exact_t<serial&>(*this));
			}
			else if constexpr (std::is_constructible_v<type>)
			{
				type value{};
				ensure(serialize(value));
				return value;
			}
			else if constexpr (TupleAlike<T>)
			{
				static_assert(std::tuple_size_v<type> == 2, "Unimplemented tuple serialization!");
				return type{ operator std::remove_cvref_t<decltype(std::get<0>(std::declval<type&>()))>
					, operator std::remove_cvref_t<decltype(std::get<1>(std::declval<type&>()))> };
			}
		}

		// Returns true if writable or readable and valid
		bool is_valid() const
		{
			return is_writing() || pos < data.size();
		}
	};
}
