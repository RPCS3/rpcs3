#pragma once
#include <cstddef>

namespace rpcs3
{
	inline namespace core
	{
		struct ignore
		{
			template<typename T>
			constexpr ignore(T)
			{
			}
		};

		template<std::size_t Size = sizeof(std::size_t)>
		struct fnv_1a;

		template<>
		struct fnv_1a<8>
		{
			static constexpr std::size_t offset_basis = 14695981039346656037ULL;
			static constexpr std::size_t prime = 1099511628211ULL;
		};

		template<>
		struct fnv_1a<4>
		{
			static constexpr std::size_t offset_basis = 2166136261;
			static constexpr std::size_t prime = 16777619;
		};

		struct fnv_1a_hasher
		{
			static std::size_t hash(const u8* raw, std::size_t size)
			{
				std::size_t result = fnv_1a<>::offset_basis;

				for (std::size_t byte = 0; byte < size; ++byte)
				{
					result ^= (std::size_t)raw[byte];
					result *= fnv_1a<>::prime;
				}

				return result;
			}

			template<typename Type>
			static std::size_t hash(const Type& value)
			{
				return hash((const u8*)&value, sizeof(Type));
			}

			template<typename Type>
			std::size_t operator()(const Type& value) const
			{
				return hash(value);
			}
		};

		struct binary_equals
		{
			template<typename T>
			bool operator()(const T& a, const T& b)
			{
				return memcmp(&a, &b, sizeof(T)) == 0;
			}
		};

		struct nullref_t
		{
			constexpr nullref_t() {};
		} static constexpr nullref{};

		struct null_t
		{
			constexpr null_t() {};
			constexpr null_t(nullptr_t) {};
			constexpr null_t(nullref_t) {};

			constexpr operator nullref_t() const { return nullref; };
			constexpr operator nullptr_t() const { return nullptr; };
		} static constexpr null{};

		template<typename Type>
		struct ref
		{
			using type = Type;

		private:
			type *m_data_ptr;

		public:
			ref() : m_data_ptr(nullptr)
			{
			}

			ref(type& raw_ref) : m_data_ptr(&raw_ref)
			{
			}

			type *operator&()
			{
				return m_data_ptr;
			}

			const type *operator&() const
			{
				return m_data_ptr;
			}

			operator type&()
			{
				return *m_data_ptr;
			}

			operator const type&() const
			{
				return *m_data_ptr;
			}

			bool empty() const
			{
				return m_data_ptr == nullptr;
			}

			void value(const type &value_)
			{
				*m_data_ptr = value_;
			}

			type value() const
			{
				return *m_data_ptr;
			}

			bool operator == (nullref_t) const
			{
				return empty();
			}

			bool operator != (nullref_t) const
			{
				return !empty();
			}

			ref& operator = (nullref_t)
			{
				m_data_ptr = nullptr;
				return *this;
			}

			ref& operator = (type &raw_ref)
			{
				m_data_ptr = &raw_ref;
				return *this;
			}
		};

		template<typename Type>
		struct cref
		{
			using type = Type;

		private:
			const type *m_data_ptr;

		public:
			cref() : m_data_ptr(nullptr)
			{
			}

			cref(const ref<type> &data) : m_data_ptr(data.m_data_ptr)
			{
			}

			cref(const type& raw_ref) : m_data_ptr(&raw_ref)
			{
			}

			const type *operator&() const
			{
				return m_data_ptr;
			}

			operator const type&() const
			{
				return *m_data_ptr;
			}

			bool empty() const
			{
				return m_data_ptr == nullptr;
			}

			type value() const
			{
				return *m_data_ptr;
			}

			bool operator == (nullref_t) const
			{
				return empty();
			}

			bool operator != (nullref_t) const
			{
				return !empty();
			}

			cref& operator = (nullref_t)
			{
				m_data_ptr = nullptr;
				return *this;
			}

			cref& operator = (const type &raw_ref)
			{
				m_data_ptr = &raw_ref;
				return *this;
			}

			cref& operator = (const ref<type> &data)
			{
				m_data_ptr = &data.m_data_ptr;
				return *this;
			}
		};
	}
}