#include "stdafx.h"
#include "GLDMA.h"

#include "Emu/Memory/vm.h"
#include "Emu/RSX/GL/glutils/common.h"

namespace gl
{
	static constexpr u32 s_dma_block_size = 0x10000;
	static constexpr u32 s_dma_block_mask = ~(s_dma_block_size - 1);

	std::unordered_map<u32, std::unique_ptr<dma_block>> g_dma_pool;

	void dma_block::allocate(u32 base_address, u32 block_size)
	{
		// Since this is a userptr block, we don't need to move data around on resize. Just "claim" a different chunk and move on.
		if (m_data)
		{
			m_data->remove();
		}

		void* userptr = vm::get_super_ptr(base_address);

		m_data = std::make_unique<gl::buffer>();
		m_data->create(buffer::target::array, block_size, userptr, buffer::memory_type::userptr, 0);
		m_base_address = base_address;

		// Some drivers may reject userptr input for whatever reason. Check that the state is still valid.
		gl::check_state();
	}

	void* dma_block::map(const utils::address_range32& range) const
	{
		ensure(range.inside(this->range()));
		return vm::get_super_ptr(range.start);
	}

	void dma_block::resize(u32 new_length)
	{
		if (new_length <= length())
		{
			return;
		}

		allocate(m_base_address, new_length);
	}

	void dma_block::set_parent(const dma_block* other)
	{
		ensure(this->range().inside(other->range()));
		ensure(other != this);

		m_parent = other;
		if (m_data)
		{
			m_data->remove();
			m_data.reset();
		}
	}

	bool dma_block::can_map(const utils::address_range32& range) const
	{
		if (m_parent)
		{
			return m_parent->can_map(range);
		}

		return range.inside(this->range());
	}

	void clear_dma_resources()
	{
		g_dma_pool.clear();
	}

	utils::address_range32 to_dma_block_range(u32 start, u32 length)
	{
		const auto start_block_address = start & s_dma_block_mask;
		const auto end_block_address = (start + length + s_dma_block_size - 1) & s_dma_block_mask;
		return utils::address_range32::start_end(start_block_address, end_block_address);
	}

	const dma_block& get_block(u32 start, u32 length)
	{
		const auto block_range = to_dma_block_range(start, length);
		auto& block = g_dma_pool[block_range.start];
		if (!block)
		{
			block = std::make_unique<dma_block>();
			block->allocate(block_range.start, block_range.length());
			return *block;
		}

		const auto range = utils::address_range32::start_length(start, length);
		if (block->can_map(range)) [[ likely ]]
		{
			return *block;
		}

		const auto owner = block->head();
		const auto new_length = (block_range.end + 1) - owner->base_addr();
		const auto search_end = (block_range.end + 1);

		// 1. Resize to new length
		ensure((new_length & ~s_dma_block_mask) == 0);
		auto new_owner = std::make_unique<dma_block>();
		new_owner->allocate(owner->base_addr(), new_length);

		// 2. Acquire all the extras
		for (u32 id = owner->base_addr() + s_dma_block_size;
			 id < search_end;
			 id += s_dma_block_size)
		{
			ensure((id % s_dma_block_size) == 0);
			g_dma_pool[id]->set_parent(new_owner.get());
		}

		block = std::move(new_owner);
		return *block;
	}

	dma_mapping_handle map_dma(u32 guest_address, u32 length)
	{
		auto& block = get_block(guest_address, length);
		return { guest_address - block.base_addr(), block.get() };
	}
}
