#pragma once
#include "ranged_map.hpp"

namespace rsx
{
	template <typename Traits, int BlockSize>
	class surface_cache_data_map : public ranged_map<typename Traits::surface_storage_type, BlockSize>
	{
#ifdef _MSC_VER
		using super = ranged_map<typename Traits::surface_storage_type, BlockSize>;
#else
		using super = class ranged_map<typename Traits::surface_storage_type, BlockSize>;
#endif
		using metadata_t = typename super::block_metadata_t;

		const metadata_t& find_head_block(u32 address)
		{
			auto& meta = super::m_metadata[address];
			if (meta.head_block != umax)
			{
				return find_head_block(meta.head_block * BlockSize);
			}

			return meta;
		}

	public:
		using buffer_object_storage_type = typename Traits::buffer_object_storage_type;
		using buffer_object_type = typename Traits::buffer_object_type;

		struct buffer_object_t
		{
			buffer_object_storage_type bo;
			u64 memory_tag = 0;

			inline buffer_object_type get()
			{
				return Traits::get(bo);
			}

			inline void release()
			{
				bo.release();
			}

			inline void acquire(buffer_object_type obj)
			{
				ensure(!get());
				bo = obj;
			}
		};

	protected:
		using buffer_block_array = typename std::array<buffer_object_t, 0x100000000ull / BlockSize>;
		buffer_block_array m_buffer_list;

	public:
		surface_cache_data_map()
			: super::ranged_map()
		{}

		surface_cache_data_map& with_range(const utils::address_range& range)
		{
			// Prepare underlying memory so that the range specified is provisioned and contiguous
			const auto& head_block = find_head_block(range.start);
			const auto start_address = block_address(head_block.id);

			const auto& current = m_buffer_list[head_block.id];
			if (auto bo = current.get())
			{
				if (::size32(*bo) >= (range.end - start_address))
				{
					return *this;
				}
			}

			// Data does not exist or is not contiguous. Merge the layer
			std::vector<buffer_object_type> bo_list;
			for (u32 address = start_address; address <= range.end;)
			{
				auto& bo_storage = m_buffer_list[super::block_for(address)];
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

			auto unified = Traits::merge_bo_list<BlockSize>(bo_list);
			ensure(unified);

			current.acquire(unified);
			return *this;
		}

		void spill(const utils::address_range& range)
		{
			// Move VRAM to system RAM
			const auto& meta = with_range(range).find_head_block(range.start);
			auto& storage = m_buffer_list[meta.id];
			Traits::spill_buffer(storage.bo);
		}

		void unspill(const utils::address_range& range)
		{
			// Move system RAM to VRAM
			const auto& meta = with_range(range).find_head_block(range.start);
			auto& storage = m_buffer_list[meta.id];
			Traits::unspill_buffer(storage.bo);
		}
	};
}
