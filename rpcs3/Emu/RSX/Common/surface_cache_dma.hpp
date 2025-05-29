#pragma once

#include <util/types.hpp>
#include "Utilities/address_range.h"

namespace rsx
{
	template <typename Traits, int BlockSize>
	class surface_cache_dma
	{
	protected:
		static inline u32 block_for(u32 address)
		{
			return address / BlockSize;
		}

		static inline u32 block_address(u32 block_id)
		{
			return block_id * BlockSize;
		}

		using buffer_object_storage_type = typename Traits::buffer_object_storage_type;
		using buffer_object_type = typename Traits::buffer_object_type;
		using command_list_type = typename Traits::command_list_type;

		struct memory_buffer_entry_t
		{
			u32 id;
			buffer_object_storage_type bo;
			u64 memory_tag = 0;
			u32 base_address = 0;

			inline buffer_object_type get() { return Traits::get(bo); }
			inline operator bool () const { return base_address != 0; }

			inline void release() { bo.release(); }
			inline void acquire(buffer_object_type b) { bo = b; }
		};

		using buffer_block_array = typename std::array<memory_buffer_entry_t, 0x100000000ull / BlockSize>;
		buffer_block_array m_buffer_list;

	public:
		surface_cache_dma()
		{
			for (u32 i = 0; i < ::size32(m_buffer_list); ++i)
			{
				m_buffer_list[i].id = i;
			}
		}

		surface_cache_dma& with_range(command_list_type cmd, const utils::address_range32& range)
		{
			// Prepare underlying memory so that the range specified is provisioned and contiguous
			// 1. Check if we have a pre-existing bo layer
			const auto& this_entry = m_buffer_list[block_for(range.start)];
			if (this_entry)
			{
				const auto bo = this_entry.get();
				const auto buffer_range = utils::address_range32::start_length(bo.base_address, ::size32(*bo));

				if (range.inside(buffer_range))
				{
					// All is well
					return *this;
				}
			}

			// Data does not exist or is not contiguous. Merge the layer
			std::vector<buffer_object_type> bo_list;
			const auto start_address = this_entry ? this_entry.base_address : block_address(this_entry.id);

			for (u32 address = start_address; address <= range.end;)
			{
				auto& bo_storage = m_buffer_list[block_for(address)];
				bo_storage.base_address = start_address;

				if (auto bo = bo_storage.get())
				{
					bo_list.push_back(bo);
					bo_storage.release();
					address += ::size32(*bo);
					continue;
				}

				bo_list.push_back(nullptr);
				address += BlockSize;
			}

			auto unified = Traits::template merge_bo_list<BlockSize>(cmd, bo_list);
			ensure(unified);

			m_buffer_list[block_for(start_address)].acquire(unified);
			return *this;
		}

		utils::address_range32 to_block_range(const utils::address_range32& range)
		{
			u32 start = block_address(block_for(range.start));
			u32 end = block_address(block_for(range.end + BlockSize - 1));
			return utils::address_range32::start_end(start, end - 1);
		}

		std::tuple<buffer_object_type, u32, u64> get(u32 address)
		{
			const auto& block = m_buffer_list[block_for(address)];
			return { block.get(), block.base_address - address };
		}

		void touch(const utils::address_range32& range)
		{
			const u64 stamp = rsx::get_shared_tag();
			for (usz i = block_for(range.start); i <= block_for(range.end); i++)
			{
				m_buffer_list[i].memory_tag = stamp;
			}
		}
	};
}
