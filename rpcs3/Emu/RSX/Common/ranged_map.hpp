#pragma once

#include <util/types.hpp>
#include "Utilities/address_range.h"

#include <unordered_map>

namespace rsx
{
	template <typename T, int BlockSize>
	class ranged_map
	{
	protected:
		struct block_metadata_t
		{
			u32 id = umax;             // ID of the matadata blob
			u32 head_block = umax;     // Earliest block that may have an object that intersects with the data at the block with ID 'id'
		};

	public:
		using inner_type = typename std::unordered_map<u32, T>;
		using outer_type = typename std::array<inner_type, 0x100000000ull / BlockSize>;
		using metadata_array = typename std::array<block_metadata_t, 0x100000000ull / BlockSize>;

	protected:
		outer_type m_data;
		metadata_array m_metadata;

		static inline u32 block_for(u32 address)
		{
			return address / BlockSize;
		}

		static inline u32 block_address(u32 block_id)
		{
			return block_id * BlockSize;
		}

		void broadcast_insert(const utils::address_range32& range)
		{
			const auto head_block = block_for(range.start);
			for (auto meta = &m_metadata[head_block]; meta <= &m_metadata[block_for(range.end)]; ++meta)
			{
				meta->head_block = std::min(head_block, meta->head_block);
			}
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

			inner_type* m_data_ptr = nullptr;
			block_metadata_t* m_metadata_ptr = nullptr;
			inner_iterator m_it{};

			void forward_scan()
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

			void begin_range(u32 address, inner_iterator& where)
			{
				m_current = &m_data_ptr[address / BlockSize];
				m_end = m_current;
				m_it = where;
			}

			void begin_range(const utils::address_range32& range)
			{
				const auto start_block_id = range.start / BlockSize;
				const auto& metadata = m_metadata_ptr[start_block_id];
				m_current = &m_data_ptr[std::min(start_block_id, metadata.head_block)];
				m_end = &m_data_ptr[range.end / BlockSize];

				--m_current;
				forward_scan();
			}

			void erase()
			{
				m_it = m_current->erase(m_it);
				if (m_it != m_current->end())
				{
					return;
				}

				forward_scan();
			}

			iterator(super* parent):
				m_data_ptr(parent->m_data.data()),
				m_metadata_ptr(parent->m_metadata.data())
			{}

		public:
			bool operator == (const iterator& other) const
			{
				return m_current == other.m_current && m_it == other.m_it;
			}

			auto* operator -> ()
			{
				ensure(m_current);
				return m_it.operator->();
			}

			auto& operator * ()
			{
				ensure(m_current);
				return m_it.operator*();
			}

			auto* operator -> () const
			{
				ensure(m_current);
				return m_it.operator->();
			}

			auto& operator * () const
			{
				ensure(m_current);
				return m_it.operator*();
			}

			iterator& operator ++ ()
			{
				ensure(m_current);
				next();
				return *this;
			}

			T& operator ++ (int)
			{
				ensure(m_current);
				auto old = *this;
				next();
				return old;
			}
		};

	public:
		ranged_map()
		{
			std::for_each(m_metadata.begin(), m_metadata.end(), [&](auto& meta) { meta.id = static_cast<u32>(&meta - m_metadata.data()); });
		}

		void emplace(const utils::address_range32& range, T&& value)
		{
			broadcast_insert(range);
			m_data[block_for(range.start)].insert_or_assign(range.start, std::forward<T>(value));
		}

		usz count(const u32 key) const
		{
			const auto& block = m_data[block_for(key)];
			if (const auto found = block.find(key);
				found != block.end())
			{
				return 1;
			}

			return 0;
		}

		iterator find(const u32 key)
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

		iterator erase(iterator& where)
		{
			where.erase();
			return where;
		}

		void erase(u32 address)
		{
			m_data[block_for(address)].erase(address);
		}

		iterator begin_range(const utils::address_range32& range)
		{
			iterator ret = { this };
			ret.begin_range(range);
			return ret;
		}

		iterator end()
		{
			iterator ret = { this };
			return ret;
		}

		void clear()
		{
			for (auto& e : m_data)
			{
				e.clear();
			}
		}
	};
}
