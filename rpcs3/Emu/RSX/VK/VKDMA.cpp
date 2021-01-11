#include "stdafx.h"
#include "VKResourceManager.h"
#include "VKDMA.h"
#include "vkutils/device.h"

#include "Emu/Memory/vm.h"

#include "util/asm.hpp"
#include <unordered_map>

namespace vk
{
	static constexpr usz s_dma_block_length = 0x00010000;
	static constexpr u32 s_dma_block_mask   = 0xFFFF0000;
	//static constexpr u32 s_dma_offset_mask  = 0x0000FFFF;

	static constexpr u32 s_page_size = 65536;
	static constexpr u32 s_page_align = s_page_size - 1;
	static constexpr u32 s_pages_per_entry = 32;
	static constexpr u32 s_bits_per_page = 2;
	static constexpr u32 s_bytes_per_entry = (s_page_size * s_pages_per_entry);

	std::unordered_map<u32, std::unique_ptr<dma_block>> g_dma_pool;

	void* dma_block::map_range(const utils::address_range& range)
	{
		if (inheritance_info.parent)
		{
			return inheritance_info.parent->map_range(range);
		}

		ensure(range.start >= base_address);
		u32 start = range.start;
		start -= base_address;
		return allocated_memory->map(start, range.length());
	}

	void dma_block::unmap()
	{
		if (inheritance_info.parent)
		{
			inheritance_info.parent->unmap();
		}
		else
		{
			allocated_memory->unmap();
		}
	}

	void dma_block::allocate(const render_device& dev, usz size)
	{
		if (allocated_memory)
		{
			// Acquired blocks are always to be assumed dirty. It is not possible to synchronize host access and inline
			// buffer copies without causing weird issues. Overlapped incomplete data ends up overwriting host-uploaded data.
			auto gc = vk::get_resource_manager();
			gc->dispose(allocated_memory);
		}

		allocated_memory = std::make_unique<vk::buffer>(dev, size,
			dev.get_memory_mapping().host_visible_coherent, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);
	}

	void dma_block::init(const render_device& dev, u32 addr, usz size)
	{
		ensure(size);
		ensure(!(size % s_dma_block_length));
		base_address = addr;

		allocate(dev, size);
		page_info.resize(size / s_bytes_per_entry, ~0ull);
	}

	void dma_block::init(dma_block* parent, u32 addr, usz size)
	{
		base_address = addr;
		inheritance_info.parent = parent;
		inheritance_info.block_offset = (addr - parent->base_address);
	}

	void dma_block::set_page_bit(u32 offset, u64 bits)
	{
		const auto entry = (offset / s_bytes_per_entry);
		const auto word = entry / s_pages_per_entry;
		const auto shift = (entry % s_pages_per_entry) * s_bits_per_page;

		page_info[word] &= ~(3 << shift);
		page_info[word] |= (bits << shift);
	}

	bool dma_block::test_page_bit(u32 offset, u64 bits)
	{
		const auto entry = (offset / s_bytes_per_entry);
		const auto word = entry / s_pages_per_entry;
		const auto shift = (entry % s_pages_per_entry) * s_bits_per_page;

		return !!(page_info[word] & (bits << shift));
	}

	void dma_block::mark_dirty(const utils::address_range& range)
	{
		if (!inheritance_info.parent)
		{
			const u32 start = utils::align(range.start, s_page_size);
			const u32 end = ((range.end + 1) & s_page_align);

			for (u32 page = start; page < end; page += s_page_size)
			{
				set_page_bit(page - base_address, page_bits::dirty);
			}

			if (start > range.start) [[unlikely]]
			{
				set_page_bit(start - s_page_size, page_bits::nocache);
			}

			if (end < range.end) [[unlikely]]
			{
				set_page_bit(end + s_page_size, page_bits::nocache);
			}
		}
		else
		{
			inheritance_info.parent->mark_dirty(range);
		}
	}

	void dma_block::set_page_info(u32 page_offset, const std::vector<u64>& bits)
	{
		if (!inheritance_info.parent)
		{
			auto bit_offset = page_offset / s_bytes_per_entry;
			ensure(bit_offset + bits.size() <= page_info.size());
			std::memcpy(page_info.data() + bit_offset, bits.data(), bits.size());
		}
		else
		{
			inheritance_info.parent->set_page_info(page_offset + inheritance_info.block_offset, bits);
		}
	}

	void dma_block::flush(const utils::address_range& range)
	{
		auto src = map_range(range);
		auto dst = vm::get_super_ptr(range.start);
		std::memcpy(dst, src, range.length());

		// TODO: Clear page bits
		unmap();
	}

	void dma_block::load(const utils::address_range& range)
	{
		auto src = vm::get_super_ptr(range.start);
		auto dst = map_range(range);
		std::memcpy(dst, src, range.length());

		// TODO: Clear page bits to sychronized
		unmap();
	}

	std::pair<u32, buffer*> dma_block::get(const utils::address_range& range)
	{
		if (inheritance_info.parent)
		{
			return inheritance_info.parent->get(range);
		}

		ensure(range.start >= base_address);
		ensure(range.end <= end());

		// mark_dirty(range);
		return { (range.start - base_address), allocated_memory.get() };
	}

	dma_block* dma_block::head()
	{
		if (!inheritance_info.parent)
			return this;

		return inheritance_info.parent->head();
	}

	const dma_block* dma_block::head() const
	{
		if (!inheritance_info.parent)
			return this;

		return inheritance_info.parent->head();
	}

	void dma_block::set_parent(const command_buffer& cmd, dma_block* parent)
	{
		ensure(parent);
		if (inheritance_info.parent == parent)
		{
			// Nothing to do
			return;
		}

		inheritance_info.parent = parent;
		inheritance_info.block_offset = (base_address - parent->base_address);

		if (allocated_memory)
		{
			// Acquired blocks are always to be assumed dirty. It is not possible to synchronize host access and inline
			// buffer copies without causing weird issues. Overlapped incomplete data ends up overwriting host-uploaded data.
			auto gc = vk::get_resource_manager();
			gc->dispose(allocated_memory);

			parent->set_page_info(inheritance_info.block_offset, page_info);
			page_info.clear();
		}
	}

	void dma_block::extend(const command_buffer& cmd, const render_device& dev, usz new_size)
	{
		ensure(allocated_memory);
		if (new_size <= allocated_memory->size())
			return;

		allocate(dev, new_size);

		const auto required_entries = new_size / s_bytes_per_entry;
		page_info.resize(required_entries, ~0ull);
	}

	u32 dma_block::start() const
	{
		return base_address;
	}

	u32 dma_block::end() const
	{
		auto source = head();
		return (source->base_address + source->allocated_memory->size() - 1);
	}

	u32 dma_block::size() const
	{
		return (allocated_memory) ? allocated_memory->size() : 0;
	}

	void dma_block_EXT::allocate(const render_device& dev, usz size)
	{
		if (allocated_memory)
		{
			// Acquired blocks are always to be assumed dirty. It is not possible to synchronize host access and inline
			// buffer copies without causing weird issues. Overlapped incomplete data ends up overwriting host-uploaded data.
			auto gc = vk::get_resource_manager();
			gc->dispose(allocated_memory);
		}

		allocated_memory = std::make_unique<vk::buffer>(dev,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			vm::get_super_ptr<void>(base_address),
			size);
	}

	void* dma_block_EXT::map_range(const utils::address_range& range)
	{
		return vm::get_super_ptr<void>(range.start);
	}

	void dma_block_EXT::unmap()
	{
		// NOP
	}

	void dma_block_EXT::flush(const utils::address_range& range)
	{
		// NOP
	}

	void dma_block_EXT::load(const utils::address_range& range)
	{
		// NOP
	}

	void create_dma_block(std::unique_ptr<dma_block>& block)
	{
#ifdef _WIN32
		if (g_render_device->get_external_memory_host_support())
		{
			block.reset(new dma_block_EXT());
		}
		else
#endif
		{
			block.reset(new dma_block());
		}
	}

	std::pair<u32, vk::buffer*> map_dma(const command_buffer& cmd, u32 local_address, u32 length)
	{
		const auto map_range = utils::address_range::start_length(local_address, length);
		const auto first_block = (local_address & s_dma_block_mask);
		const auto limit = local_address + length - 1;
		auto last_block = (limit & s_dma_block_mask);

		if (auto found = g_dma_pool.find(first_block); found != g_dma_pool.end())
		{
			if (found->second->end() >= limit)
			{
				return found->second->get(map_range);
			}
		}

		if (first_block == last_block) [[likely]]
		{
			auto &block_info = g_dma_pool[first_block];
			if (!block_info) create_dma_block(block_info);

			block_info->init(*g_render_device, first_block, s_dma_block_length);
			return block_info->get(map_range);
		}

		dma_block* block_head = nullptr;
		auto block_end = utils::align(limit, s_dma_block_length);

		// Reverse scan to try and find the minimum required length in case of other chaining
		for (auto block = last_block; block != first_block; block -= s_dma_block_length)
		{
			if (auto found = g_dma_pool.find(block); found != g_dma_pool.end())
			{
				const auto end = found->second->end();
				last_block = std::max(last_block, end & s_dma_block_mask);
				block_end = std::max(block_end, end + 1);

				break;
			}
		}

		for (auto block = first_block; block <= last_block; block += s_dma_block_length)
		{
			auto found = g_dma_pool.find(block);
			auto &entry = g_dma_pool[block];

			const bool exists = !!entry;
			if (!exists) create_dma_block(entry);

			if (block == first_block)
			{
				block_head = entry->head();

				if (exists)
				{
					if (entry->end() < limit)
					{
						auto new_length = block_end - block_head->start();
						block_head->extend(cmd, *g_render_device, new_length);
					}
				}
				else
				{
					auto required_size = (block_end - block);
					block_head->init(*g_render_device, block, required_size);
				}
			}
			else
			{
				if (exists)
				{
					entry->set_parent(cmd, block_head);
				}
				else
				{
					entry->init(block_head, block, s_dma_block_length);
				}
			}
		}

		ensure(block_head);
		return block_head->get(map_range);
	}

	template<bool load>
	void sync_dma_impl(u32 local_address, u32 length)
	{
		const auto limit = local_address + length - 1;
		while (length)
		{
			u32 block = (local_address & s_dma_block_mask);
			if (auto found = g_dma_pool.find(block); found != g_dma_pool.end())
			{
				const auto sync_end = std::min(limit, found->second->end());
				const auto range = utils::address_range::start_end(local_address, sync_end);

				if constexpr (load)
				{
					found->second->load(range);
				}
				else
				{
					found->second->flush(range);
				}

				if (sync_end < limit) [[unlikely]]
				{
					// Technically legal but assuming a map->flush usage, this shouldnot happen
					// Optimizations could in theory batch together multiple transfers though
					rsx_log.error("Sink request spans multiple allocated blocks!");
					const auto write_end = (sync_end + 1u);
					const auto written = (write_end - local_address);
					length -= written;
					local_address = write_end;
					continue;
				}

				break;
			}
			else
			{
				rsx_log.error("Sync command on range not mapped!");
				return;
			}
		}
	}

	void load_dma(u32 local_address, u32 length)
	{
		sync_dma_impl<true>(local_address, length);
	}

	void flush_dma(u32 local_address, u32 length)
	{
		sync_dma_impl<false>(local_address, length);
	}

	void clear_dma_resources()
	{
		g_dma_pool.clear();
	}
}
