#include "stdafx.h"
#include "VKResourceManager.h"
#include "VKDMA.h"
#include "vkutils/device.h"

#include "Emu/Memory/vm.h"
#include "Utilities/mutex.h"

#include "util/asm.hpp"
#include <unordered_map>

namespace vk
{
	static constexpr usz s_dma_block_length = 0x00010000;
	static constexpr u32 s_dma_block_mask   = 0xFFFF0000;

	std::unordered_map<u32, std::unique_ptr<dma_block>> g_dma_pool;
	shared_mutex g_dma_mutex;

	// Validation
	atomic_t<u64> s_allocated_dma_pool_size{ 0 };

	dma_block::~dma_block()
	{
		// Use safe free (uses gc to clean up)
		free();
	}

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
		// Acquired blocks are always to be assumed dirty. It is not possible to synchronize host access and inline
		// buffer copies without causing weird issues. Overlapped incomplete data ends up overwriting host-uploaded data.
		free();

		allocated_memory = std::make_unique<vk::buffer>(dev, size,
			dev.get_memory_mapping().host_visible_coherent, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);

		s_allocated_dma_pool_size += allocated_memory->size();
	}

	void dma_block::free()
	{
		if (allocated_memory)
		{
			s_allocated_dma_pool_size -= allocated_memory->size();

			auto gc = vk::get_resource_manager();
			gc->dispose(allocated_memory);
		}
	}

	void dma_block::init(const render_device& dev, u32 addr, usz size)
	{
		ensure((size > 0) && !((size | addr) & ~s_dma_block_mask));
		base_address = addr;

		allocate(dev, size);
		ensure(!inheritance_info.parent);
	}

	void dma_block::init(dma_block* parent, u32 addr, usz size)
	{
		ensure((size > 0) && !((size | addr) & ~s_dma_block_mask));

		base_address = addr;
		inheritance_info.parent = parent;
		inheritance_info.block_offset = (addr - parent->base_address);
	}

	void dma_block::flush(const utils::address_range& range)
	{
		if (inheritance_info.parent)
		{
			// Parent may be a different type of block
			inheritance_info.parent->flush(range);
			return;
		}

		auto src = map_range(range);
		auto dst = vm::get_super_ptr(range.start);
		std::memcpy(dst, src, range.length());

		// TODO: Clear page bits
		unmap();
	}

	void dma_block::load(const utils::address_range& range)
	{
		if (inheritance_info.parent)
		{
			// Parent may be a different type of block
			inheritance_info.parent->load(range);
			return;
		}

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

	void dma_block::set_parent(dma_block* parent)
	{
		ensure(parent);
		ensure(parent->base_address < base_address);
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
			free();
		}
	}

	void dma_block::extend(const render_device& dev, usz new_size)
	{
		ensure(allocated_memory);
		if (new_size <= allocated_memory->size())
			return;

		allocate(dev, new_size);
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
		// Acquired blocks are always to be assumed dirty. It is not possible to synchronize host access and inline
		// buffer copies without causing weird issues. Overlapped incomplete data ends up overwriting host-uploaded data.
		free();

		allocated_memory = std::make_unique<vk::buffer>(dev,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			vm::get_super_ptr<void>(base_address),
			size);

		s_allocated_dma_pool_size += allocated_memory->size();
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

	bool test_host_pointer(u32 base_address, usz length)
	{
#ifdef _WIN32
		MEMORY_BASIC_INFORMATION mem_info;
		if (!::VirtualQuery(vm::get_super_ptr<const void>(base_address), &mem_info, sizeof(mem_info)))
		{
			rsx_log.error("VirtualQuery failed! LastError=0x%x", GetLastError());
			return false;
		}

		return (mem_info.RegionSize >= length);
#else
		return true; // *nix behavior is unknown with NVIDIA drivers
#endif
	}

	void create_dma_block(std::unique_ptr<dma_block>& block, u32 base_address, usz expected_length)
	{
		const auto vendor = g_render_device->gpu().get_driver_vendor();

#ifdef _WIN32
		const bool allow_host_buffers = (vendor == driver_vendor::NVIDIA) ?
			//test_host_pointer(base_address, expected_length) :
			rsx::get_location(base_address) == CELL_GCM_LOCATION_LOCAL : // NVIDIA workaround
			true;
#else
		// Anything running on AMDGPU kernel driver will not work due to the check for fd-backed memory allocations
		const bool allow_host_buffers = (vendor != driver_vendor::AMD && vendor != driver_vendor::RADV);
#endif
		if (allow_host_buffers && g_render_device->get_external_memory_host_support())
		{
			block.reset(new dma_block_EXT());
		}
		else
		{
			block.reset(new dma_block());
		}

		block->init(*g_render_device, base_address, expected_length);
	}

	std::pair<u32, vk::buffer*> map_dma(u32 local_address, u32 length)
	{
		// Not much contention expected here, avoid searching twice
		std::lock_guard lock(g_dma_mutex);

		const auto map_range = utils::address_range::start_length(local_address, length);
		auto first_block = (local_address & s_dma_block_mask);

		if (auto found = g_dma_pool.find(first_block); found != g_dma_pool.end())
		{
			if (found->second->end() >= map_range.end)
			{
				return found->second->get(map_range);
			}
		}

		auto last_block = (map_range.end & s_dma_block_mask);
		if (first_block == last_block) [[likely]]
		{
			auto &block_info = g_dma_pool[first_block];
			ensure(!block_info);

			create_dma_block(block_info, first_block, s_dma_block_length);
			return block_info->get(map_range);
		}

		// Scan range for overlapping sections and update 'chains' accordingly
		for (auto block = first_block; block <= last_block; block += s_dma_block_length)
		{
			if (auto& entry = g_dma_pool[block])
			{
				first_block = std::min(first_block, entry->head()->start() & s_dma_block_mask);
				last_block = std::max(last_block, entry->end() & s_dma_block_mask);
			}
		}

		std::vector<std::unique_ptr<dma_block>> stale_references;
		dma_block* block_head = nullptr;

		for (auto block = first_block; block <= last_block; block += s_dma_block_length)
		{
			auto& entry = g_dma_pool[block];

			if (block == first_block)
			{
				if (entry)
				{
					// Then the references to this object do not go to the end of the list as will be done with this new allocation.
					// A dumb release is therefore safe...
					ensure(entry->end() < map_range.end);
					stale_references.push_back(std::move(entry));
				}

				auto required_size = (last_block - first_block + s_dma_block_length);
				create_dma_block(entry, block, required_size);
				block_head = entry->head();
			}
			else if (entry)
			{
				ensure((entry->end() & s_dma_block_mask) <= last_block);
				entry->set_parent(block_head);
			}
			else
			{
				entry.reset(new dma_block());
				entry->init(block_head, block, s_dma_block_length);
			}
		}

		// Check that all the math adds up...
		stale_references.clear();
		ensure(s_allocated_dma_pool_size == g_dma_pool.size() * s_dma_block_length);

		ensure(block_head);
		return block_head->get(map_range);
	}

	void unmap_dma(u32 local_address, u32 length)
	{
		std::lock_guard lock(g_dma_mutex);

		const u32 start = (local_address & s_dma_block_mask);
		const u32 end = utils::align(local_address + length, s_dma_block_length);

		for (u32 block = start; block < end;)
		{
			if (auto found = g_dma_pool.find(block); found != g_dma_pool.end())
			{
				auto head = found->second->head();
				if (dynamic_cast<dma_block_EXT*>(head))
				{
					// Passthrough block. Must unmap from GPU
					const u32 start_block = head->start();
					const u32 last_block = head->start() + head->size();

					for (u32 block_ = start_block; block_ < last_block; block_ += s_dma_block_length)
					{
						g_dma_pool.erase(block_);
					}

					block = last_block;
					continue;
				}
			}

			block += s_dma_block_length;
		}

		ensure(s_allocated_dma_pool_size == g_dma_pool.size() * s_dma_block_length);
	}

	template<bool load>
	void sync_dma_impl(u32 local_address, u32 length)
	{
		reader_lock lock(g_dma_mutex);

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
