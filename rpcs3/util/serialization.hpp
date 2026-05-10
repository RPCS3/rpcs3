#pragma once

#include "util/types.hpp"
#include <vector>

namespace utils
{
	template <typename T>
	concept FastRandomAccess = requires (const T& obj)
	{
		std::data(obj)[std::size(obj)];
	};

	template <typename T>
	concept Reservable = requires (T& obj)
	{
		obj.reserve(std::size(obj));
	};

	template <typename T>
	concept Bitcopy = (std::is_arithmetic_v<T>) || (std::is_enum_v<T>) || Integral<T> || typename T::enable_bitcopy()();

	template <typename T>
	concept TupleAlike = (!FastRandomAccess<T>) && requires ()
	{
		std::tuple_size<std::remove_cvref_t<T>>::value;
	};

	template <typename T>

	concept ListAlike = requires(std::remove_cvref_t<T>& obj, T::value_type item) { obj.insert(obj.end(), std::move(item)); };
	struct serial;

	struct serialization_file_handler
	{
		serialization_file_handler() = default;
		virtual ~serialization_file_handler() = default;

		// Handle read/write operations
		virtual bool handle_file_op(serial& ar, usz pos, usz size, const void* data = nullptr) = 0;

		// Obtain data size (targets to be only higher than 'recommended' and thus may not be accurate)
		virtual usz get_size(const utils::serial& /*ar*/, usz /*recommended*/) const
		{
			return 0;
		}

		// Skip reading some (compressed) data
		virtual void skip_until(utils::serial& /*ar*/)
		{
		}

		// Detect empty stream (TODO: Clean this, instead perhaps use a magic static representing empty stream)
		virtual bool is_null() const
		{
			return false;
		}

		virtual void finalize_block(utils::serial& /*ar*/)
		{
		}

		virtual bool is_valid() const
		{
			return true;
		}

		virtual void finalize(utils::serial&) = 0;
	};

	struct serial
	{
private:
		bool m_is_writing = true;
		bool m_expect_little_data = false;
public:
		std::vector<u8> data;
		usz data_offset = 0;
		usz pos = 0;
		usz m_max_data = umax;
		std::unique_ptr<serialization_file_handler> m_file_handler;

		serial() noexcept = default;
		serial(const serial&) = delete;
		serial& operator=(const serial&) = delete;
		explicit serial(serial&&) noexcept = default;
		serial& operator=(serial&&) noexcept = default;
		~serial() noexcept = default;

		// Checks if this instance is currently used for serialization
		bool is_writing() const
		{
			return m_is_writing;
		}

		void add_padding(usz padding)
		{
			if (m_is_writing) return;
			pos += padding;
		}

		// Add padding needed between two members
		template <typename T, typename T2, typename T3>
		void add_padding(T T2::*const first, T3 T2::*const second)
		{
			if (m_is_writing) return;

			const u32 offset1 = ::offset32(first) + sizeof(T);
			const u32 offset2 = ::offset32(second);

			AUDIT(::offset32(first) <= ::offset32(second));

			if (offset2 > offset1)
			{
				pos += offset2 - offset1;
			}
		}

		void set_expect_little_data(bool value)
		{
			m_expect_little_data = value;
		}

		// Return true if small amounts of both input and output memory are expected (performance hint)
		bool expect_little_data() const
		{
			return m_expect_little_data;
		}

		// Reserve memory for serialization
		void reserve(usz size)
		{
			data.reserve(data.size() + size);
		}

		template <typename Func> requires (std::is_convertible_v<std::invoke_result_t<Func>, const void*>)
		bool raw_serialize(Func&& memory_provider, usz size)
		{
			if (!size)
			{
				return true;
			}

			if (m_file_handler && m_file_handler->is_null())
			{
				// Instead of doing nothing at all, increase pos so it would be possible to estimate memory requirements
				pos += size;
				return true;
			}

			// Overflow check
			ensure(~pos >= size - 1);

			if (is_writing())
			{
				ensure(pos <= data_offset + data.size());

				const auto ptr = reinterpret_cast<const u8*>(memory_provider());

				if (pos != data_offset + data.size())
				{
					data.insert(data.begin() + pos - data_offset, ptr, ptr + size);
					pos += size;
					return true;
				}

				// Seems to be much faster than data.begin() + pos on MSVC
				data.insert(data.end(), ptr, ptr + size);
				pos += size;
				return true;
			}

			if (data.empty() || pos < data_offset || pos + (size - 1) > (data.size() - 1) + data_offset)
			{
				// Load from file
				ensure(m_file_handler);
				ensure(m_file_handler->handle_file_op(*this, pos, size, nullptr));
				ensure(!data.empty() && pos >= data_offset && pos + (size - 1) <= (data.size() - 1) + data_offset);
			}

			std::memcpy(const_cast<void*>(static_cast<const void*>(memory_provider())), data.data() + (pos - data_offset), size);
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

		template <typename T>
		static constexpr usz c_tup_size = std::tuple_size_v<std::conditional_t<TupleAlike<T>, std::remove_cvref_t<T>, std::tuple<>>>;

		template <typename T>
		static std::remove_cvref_t<T>& as_nonconst(T&& arg) noexcept
		{
			return const_cast<std::remove_cvref_t<T>&>(static_cast<const T&>(arg));
		}

		// std::vector, std::basic_string
		// Discourage using std::pair/tuple with vectors because it eliminates the possibility of bitwise optimization
		template <typename T> requires FastRandomAccess<T> && ListAlike<T> && (!TupleAlike<typename T::value_type>)
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

			obj.clear();

			if (m_file_handler && m_file_handler->is_null())
			{
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
					if constexpr (c_tup_size<decltype(*std::data(obj))> == 2)
					{
						if (!serialize(as_nonconst(std::get<0>(value))))
						{
							return false;
						}

						if (!serialize(as_nonconst(std::get<1>(value))))
						{
							return false;
						}
					}
					else
					{
						if (!serialize(value))
						{
							return false;
						}
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
					if constexpr (c_tup_size<decltype(value)> == 2)
					{
						if (!serialize(as_nonconst(std::get<0>(value))))
						{
							return false;
						}

						if (!serialize(as_nonconst(std::get<1>(value))))
						{
							return false;
						}
					}
					else
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

			if (m_file_handler && m_file_handler->is_null())
			{
				return true;
			}

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

		template <typename T>
			requires requires(T& obj, utils::serial& ar) { (obj.*(&T::operator()))(stx::exact_t<utils::serial&>(ar)); }
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
		template <typename T> requires TupleAlike<T>
		bool serialize(T& obj)
		{
			return serialize_tuple(obj);
		}

		// Wrapper for serialize(T&), allows to pass multiple objects at once
		template <typename... Args> requires (sizeof...(Args) != 0)
		bool operator()(Args&&... args) noexcept
		{
			return ((AUDIT(!std::is_const_v<std::remove_reference_t<Args>> || is_writing())
				, serialize(as_nonconst(args))), ...);
		}

		// Code style utility, for when utils::serial is a pointer for example
		template <typename... Args> requires (sizeof...(Args) > 1 || !(std::is_convertible_v<Args&&, Args&> && ...))
		bool serialize(Args&&... args)
		{
			return this->operator()(std::forward<Args>(args)...);
		}

		// Convert serialization manager to deserializion manager
		// If no arg is provided reuse saved buffer
		void set_reading_state(std::vector<u8>&& _data = std::vector<u8>{}, bool expect_little_data = false)
		{
			if (!_data.empty())
			{
				data = std::move(_data);
			}

			m_is_writing = false;
			m_expect_little_data = expect_little_data;
			m_max_data = umax;
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

		usz seek_end()
		{
			pos = data.size() + data_offset;
			return pos;
		}

		usz trunc(usz count)
		{
			data.resize(data.size() - count);
			seek_end();
			return pos;
		}

		usz seek_pos(usz val, bool cleanup = false)
		{
			const usz old_pos = std::exchange(pos, val);

			if (cleanup || data.empty())
			{
				// Relocate future data
				if (m_file_handler)
				{
					m_file_handler->skip_until(*this);
				}

				breathe();
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
			if (!forced && (!m_file_handler || (data.size() < 0x100'0000 && pos >= data_offset)))
			{
				// Let's not do anything if less than 16MB
				return;
			}

			ensure(m_file_handler);
			ensure(m_file_handler->handle_file_op(*this, 0, umax, nullptr));
		}

		template <typename T> requires (std::is_copy_constructible_v<std::remove_const_t<T>>) && (std::is_constructible_v<std::remove_const_t<T>> || Bitcopy<std::remove_const_t<T>> ||
			std::is_constructible_v<std::remove_const_t<T>, stx::exact_t<serial&>> || TupleAlike<std::remove_const_t<T>>)
		operator T() noexcept
		{
			AUDIT(!is_writing());

			using type = std::remove_const_t<T>;
			using not_tuple_t = std::conditional_t<TupleAlike<T>, char, type>;

			if constexpr (Bitcopy<T>)
			{
				u8 buf[sizeof(not_tuple_t)]{};
				ensure(raw_serialize(buf, sizeof(buf)));
				return std::bit_cast<not_tuple_t>(buf);
			}
			else if constexpr (TupleAlike<T>)
			{
				constexpr int tup_size = c_tup_size<type>;

				static_assert(tup_size == 2 || tup_size == 4, "Unimplemented tuple serialization!");

				using first_t  = typename std::tuple_element<std::min(0, tup_size - 1), type>::type;
				using second_t = typename std::tuple_element<std::min(1, tup_size - 1), type>::type;
				using third_t  = typename std::tuple_element<std::min(2, tup_size - 1), type>::type;
				using fourth_t = typename std::tuple_element<std::min(3, tup_size - 1), type>::type;

				first_t first = this->operator first_t();

				if constexpr (tup_size == 4)
				{
					second_t second = this->operator second_t();
					third_t third = this->operator third_t();

					return type{ std::move(first), std::move(second), std::move(third), this->operator fourth_t() };
				}
				else
				{
					return type{ std::move(first), this->operator second_t() };
				}
			}
			else if constexpr (std::is_constructible_v<type, stx::exact_t<serial&>>)
			{
				return not_tuple_t(stx::exact_t<serial&>(*this));
			}
			else if constexpr (std::is_constructible_v<type>)
			{
				not_tuple_t value{};
				ensure(serialize(value));
				return value;
			}
		}

		// Code style utility wrapper for operator T()
		template <typename T>
		T pop()
		{
			return this->operator T();
		}

		void swap_handler(serial& ar)
		{
			std::swap(ar.m_file_handler, this->m_file_handler);
		}

		usz get_size(usz recommended = umax) const
		{
			recommended = std::min<usz>(recommended, m_max_data);
			return std::min<usz>(m_max_data, m_file_handler ? m_file_handler->get_size(*this, recommended) : (data.empty() ? 0 : data_offset + data.size()));
		}

		template <typename T> requires (Bitcopy<T>)
		usz predict_object_size(const T&)
		{
			return sizeof(T);
		}

		template <typename T> requires FastRandomAccess<T> && (!ListAlike<T>) && (!Bitcopy<T>)
		usz predict_object_size(const T& obj)
		{
			return std::size(obj) * sizeof(obj[0]);
		}

		template <typename T> requires (std::is_copy_constructible_v<std::remove_reference_t<T>> && std::is_constructible_v<std::remove_reference_t<T>>)
		usz try_read(T&& obj)
		{
			if (is_writing())
			{
				return 0;
			}

			const usz end_pos = pos + predict_object_size(std::forward<T>(obj));
			const usz size = get_size(end_pos);

			if (size >= end_pos)
			{
				serialize(std::forward<T>(obj));
				return 0;
			}

			return end_pos - size;
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
				return {true, this->operator type()};
			}

			return {};
		}

		void patch_raw_data(usz pos, const void* data, usz size)
		{
			if (m_file_handler && m_file_handler->is_null())
			{
				return;
			}

			if (!size)
			{
				return;
			}

			std::memcpy(&::at32(this->data, pos - data_offset + size - 1) - (size - 1), data, size);
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
