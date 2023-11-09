#pragma once

#include "util/types.hpp"
#include <vector>

namespace utils
{
	template <typename T>
	concept FastRandomAccess = requires (T& obj)
	{
		std::data(obj)[std::size(obj)];
	};

	template <typename T>
	concept Reservable = requires (T& obj)
	{
		obj.reserve(std::size(obj));
	};

	template <typename T>
	concept Bitcopy = (std::is_arithmetic_v<T>) || (std::is_enum_v<T>) || Integral<T> || requires ()
	{
		std::enable_if_t<std::conjunction_v<typename T::enable_bitcopy>>();
	};

	template <typename T>
	concept TupleAlike = requires ()
	{
		std::tuple_size<std::remove_cv_t<T>>::value;
	};

	template <typename T>
	concept ListAlike = requires (T& obj) { obj.insert(obj.end(), std::declval<typename T::value_type>()); };

	struct serial;

	struct serialization_file_handler
	{
		serialization_file_handler() = default;
		virtual ~serialization_file_handler() = default;

		virtual bool handle_file_op(serial& ar, usz pos, usz size, const void* data = nullptr) = 0;
		virtual usz get_size(const utils::serial& ar, usz recommended = umax) const = 0;
	};

	struct serial
	{
		std::vector<u8> data;
		usz data_offset = 0;
		usz pos = 0;
		bool m_is_writing = true;
		bool m_avoid_large_prefetch = false;
		std::unique_ptr<serialization_file_handler> m_file_handler;

		serial() noexcept = default;
		serial(const serial&) = delete;
		~serial() noexcept = default;

		// Checks if this instance is currently used for serialization
		bool is_writing() const
		{
			return m_is_writing;
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

		template <typename Func> requires (std::is_convertible_v<std::invoke_result_t<Func>, const void*>)
		bool raw_serialize(Func&& memory_provider, usz size)
		{
			if (!size)
			{
				return true;
			}

			// Overflow check
			ensure(~pos >= size);

			if (is_writing())
			{
				ensure(pos >= data_offset);
				const auto ptr = reinterpret_cast<const u8*>(memory_provider());
				data.insert(data.begin() + pos - data_offset, ptr, ptr + size);
				pos += size;
				return true;
			}

			if (data.empty() || pos < data_offset || pos + size > data.size() + data_offset)
			{
				// Load from file
				ensure(m_file_handler);
				ensure(m_file_handler->handle_file_op(*this, pos, size, nullptr));
				ensure(!data.empty() && pos >= data_offset && pos + size <= data.size() + data_offset);
			}

			std::memcpy(const_cast<void*>(static_cast<const void*>(memory_provider())), data.data() + pos - data_offset, size);
			pos += size;
			return true;
		}

		bool raw_serialize(const void* ptr, usz size)
		{
			return raw_serialize(FN(ptr), size);
		}

		template <typename T> requires Integral<T>
		bool serialize_vle(T&& value)
		{
			for (auto i = value;;)
			{
				const auto i_old = std::exchange(i, i >> 7);
				const u8 to_write = static_cast<u8>((static_cast<u8>(i_old) % 0x80) | (i ? 0x80 : 0));
				raw_serialize(&to_write, 1);

				if (!i)
				{
					break;
				}
			}

			return true;
		}

		template <typename T> requires Integral<T>
		bool deserialize_vle(T& value)
		{
			value = {};

			for (u32 i = 0;; i += 7)
			{
				u8 byte_data = 0;

				if (!raw_serialize(&byte_data, 1))
				{
					return false;
				}

				value |= static_cast<T>(byte_data % 0x80) << i;

				if (!(byte_data & 0x80))
				{
					break;
				}
			}

			return true;
		}

		// (De)serialization function
		template <typename T>
		bool serialize(T& obj)
		{
			// Fallback to global overload
			return ::serialize(*this, obj);
		}

		// Enabled for fundamental types, enumerations and if specified explicitly that type can be saved in pure bitwise manner
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
				serialize_vle(obj.size());

				if constexpr (Bitcopy<typename std::remove_reference_t<T>::value_type>)
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

			usz size = 0;
			if (!deserialize_vle(size))
			{
				return false;
			}

			if constexpr (Bitcopy<typename T::value_type>)
			{
				if (!raw_serialize([&](){ obj.resize(size); return obj.data(); }, sizeof(obj[0]) * size))
				{
					obj.clear();
					return false;
				}
			}
			else
			{
				// TODO: Postpone resizing to after file bounds checks
				obj.clear();
				obj.resize(size);

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

		// C-array, std::array, std::span (span must be supplied with size and address, this function does not modify it)
		template <typename T> requires FastRandomAccess<T> && (!ListAlike<T>) && (!Bitcopy<T>)
		bool serialize(T& obj)
		{
			if constexpr (Bitcopy<std::remove_reference_t<decltype(std::declval<T>()[0])>>)
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
				serialize_vle(obj.size());

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
			if (!deserialize_vle(size))
			{
				return false;
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

		template <typename T> requires requires (T& obj) { (obj.*(&T::operator()))(std::declval<stx::exact_t<utils::serial&>>()); }
		bool serialize(T& obj)
		{
			obj(*this);
			return is_valid();
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
				, serialize(const_cast<std::remove_cvref_t<Args>&>(static_cast<const Args&>(args)))), ...);
		}

		// Convert serialization manager to deserializion manager
		// If no arg is provided reuse saved buffer
		void set_reading_state(std::vector<u8>&& _data = std::vector<u8>{})
		{
			if (!_data.empty())
			{
				data = std::move(_data);
			}

			m_is_writing = false;
			m_avoid_large_prefetch = false;
			pos = 0;
			data_offset = 0;
		}

		// Reset to empty serialization manager
		void clear()
		{
			data.clear();
			m_is_writing = true;
			pos = 0;
			data_offset = 0;
			m_file_handler.reset();
		}

		usz seek_end(usz backwards = 0)
		{
			ensure(data.size() + data_offset >= backwards);
			pos = data.size() + data_offset - backwards;
			return pos;
		}

		usz seek_pos(usz val, bool empty_data = false)
		{
			const usz old_pos = std::exchange(pos, val);

			if (empty_data || data.empty())
			{
				// Relocate future data
				data.clear();
				data_offset = pos;
			}

			return old_pos;
		}

		usz pad_from_end(usz forwards)
		{
			ensure(is_writing());
			pos = data.size() + data_offset;
			data.resize(data.size() + forwards);
			return pos;
		}

		// Allow for memory saving operations: if writing, flush to file if too large. If reading, discard memory (not implemented).
		// Execute only if past memory is known to not going be reused
		void breathe(bool forced = false)
		{
			if (!forced && (!m_file_handler || (data.size() < 0x20'0000 && pos >= data_offset)))
			{
				// Let's not do anything if less than 32MB
				return;
			}

			ensure(m_file_handler);
			ensure(m_file_handler->handle_file_op(*this, 0, umax, nullptr));
		}

		template <typename T> requires (std::is_copy_constructible_v<std::remove_const_t<T>>) && (std::is_constructible_v<std::remove_const_t<T>> || Bitcopy<std::remove_const_t<T>> ||
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

		void swap_handler(serial& ar)
		{
			std::swap(ar.m_file_handler, this->m_file_handler);
		}

		usz get_size(usz recommended = umax) const
		{
			return m_file_handler ? m_file_handler->get_size(*this, recommended) : data_offset + data.size();
		}

		template <typename T> requires (std::is_copy_constructible_v<T> && std::is_constructible_v<T> && Bitcopy<T>)
		std::pair<bool, T> try_read()
		{
			if (is_writing())
			{
				return {};
			}

			const usz end_pos = pos + sizeof(T);
			const usz size = get_size(end_pos);
			using type = std::remove_const_t<T>;

			if (size >= end_pos)
			{
				u8 buf[sizeof(type)]{};
				ensure(raw_serialize(buf, sizeof(buf)));
				return {true, std::bit_cast<type>(buf)};
			}

			return {};
		}

		// Returns true if valid, can be invalidated by setting pos to umax
		// Used when an invalid state is encountered somewhere in a place we can't check success code such as constructor)
		bool is_valid() const
		{
			// TODO
			return true;
		}
	};
}
