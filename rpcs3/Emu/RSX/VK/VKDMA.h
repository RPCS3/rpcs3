#pragma once
#include "vkutils/buffer_object.h"
#include "vkutils/commands.h"

namespace vk
{
	std::pair<u32, vk::buffer*> map_dma(command_buffer& cmd, u32 local_address, u32 length);
	void load_dma(u32 local_address, u32 length);
	void flush_dma(u32 local_address, u32 length);

	void clear_dma_resources();

	class dma_block
	{
		enum page_bits
		{
			synchronized = 0,
			dirty = 1,
			nocache = 3
		};

		struct
		{
			dma_block* parent = nullptr;
			u32 block_offset = 0;
		}
		inheritance_info;

		u32 base_address = 0;
		std::unique_ptr<buffer> allocated_memory;
		std::vector<u64> page_info;

		void* map_range(const utils::address_range& range);
		void unmap();

		void set_page_bit(u32 page, u64 bits);
		bool test_page_bit(u32 page, u64 bits);
		void mark_dirty(const utils::address_range& range);
		void set_page_info(u32 page_offset, const std::vector<u64>& bits);

	public:

		void init(const render_device& dev, u32 addr, usz size);
		void init(dma_block* parent, u32 addr, usz size);
		void flush(const utils::address_range& range);
		void load(const utils::address_range& range);
		std::pair<u32, buffer*> get(const utils::address_range& range);

		u32 start() const;
		u32 end() const;
		u32 size() const;

		dma_block* head();
		const dma_block* head() const;
		void set_parent(command_buffer& cmd, dma_block* parent);
		void extend(command_buffer& cmd, const render_device& dev, usz new_size);
	};
}
