#pragma once

#include <util/types.hpp>
#include "Utilities/address_range.h"

#include <unordered_map>

namespace rsx
{
	template<typename T, int BlockSize>
	class ranged_map
	{
	public:
		using inner_type = typename std::unordered_map<u32, T>;
		using outer_type = typename std::array<inner_type, 0x100000000ull / BlockSize>;

	protected:
		outer_type m_data;

		static inline u32 block_for(u32 address)
		{
			return address / BlockSize;
		}

	public:
		class iterator
		{
			using super = typename rsx::ranged_map<T, BlockSize>;
			using inner_iterator = typename inner_type::iterator;
			friend super;

		protected:
			inner_type* m_current = nullptr;
			inner_type* m_end = nullptr;

			outer_type* m_data_ptr = nullptr;
			inner_iterator m_it{};

			inline void forward_scan()
			{
				while (m_current < m_end)
				{
					m_it = (++m_current)->begin();
					if (m_it != m_current->end()) [[ likely ]]
					{
						return;
					}
				}

				// end pointer
				m_current = nullptr;
				m_it = {};
			}

			void next()
			{
				if (!m_current)
				{
					return;
				}

				if (++m_it != m_current->end()) [[ likely ]]
				{
					return;
				}

				forward_scan();
			}

			inline void begin_range(const utils::address_range& range, inner_iterator& where)
			{
				m_it = where;
				m_current = &(*m_data_ptr)[range.start / BlockSize];
				m_end = &(*m_data_ptr)[(range.end + 1) / BlockSize];
			}

			inline void begin_range(u32 address, inner_iterator& where)
			{
				begin_range(utils::address_range::start_length(address, 1), where);
			}

			inline void begin_range(const utils::address_range& range)
			{
				m_current = &(*m_data_ptr)[range.start / BlockSize];
				m_end = &(*m_data_ptr)[(range.end + 1) / BlockSize];

				--m_current;
				forward_scan();
			}

			inline void erase()
			{
				m_it = m_current->erase(m_it);
				if (m_it != m_current->end())
				{
					return;
				}

				forward_scan();
			}

			iterator(super* parent)
				: m_data_ptr(&parent->m_data)
			{}

		public:
			inline bool operator == (const iterator& other)
			{
				return m_it == other.m_it;
			}

			inline bool operator != (const iterator& other)
			{
				return m_it != other.m_it;
			}

			inline auto* operator -> ()
			{
				ensure(m_current);
				return m_it.operator->();
			}

			inline auto& operator * ()
			{
				ensure(m_current);
				return m_it.operator*();
			}

			inline auto* operator -> () const
			{
				ensure(m_current);
				return m_it.operator->();
			}

			inline auto& operator * () const
			{
				ensure(m_current);
				return m_it.operator*();
			}

			inline iterator& operator ++ ()
			{
				ensure(m_current);
				next();
				return *this;
			}

			inline T& operator ++ (int)
			{
				ensure(m_current);
				auto old = *this;
				next();
				return old;
			}
		};

		inline T& operator[](const u32& key)
		{
			return m_data[block_for(key)][key];
		}

		inline auto find(const u32& key)
		{
			auto& block = m_data[block_for(key)];
			iterator ret = { this };

			if (auto found = block.find(key);
				found != block.end())
			{
				ret.begin_range(key, found);
			}

			return ret;
		}

		inline iterator erase(iterator& where)
		{
			where.erase();
			return where;
		}

		inline void erase(u32 address)
		{
			m_data[block_for(address)].erase(address);
		}

		inline iterator begin_range(const utils::address_range& range)
		{
			iterator ret = { this };
			ret.begin_range(range);
			return ret;
		}

		inline iterator end()
		{
			iterator ret = { this };
			return ret;
		}

		inline void clear()
		{
			for (auto& e : m_data)
			{
				e.clear();
			}
		}
	};
}
