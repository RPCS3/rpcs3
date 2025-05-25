#pragma once

#include <util/types.hpp>
#include "Utilities/address_range.h"

#include "glutils/buffer_object.h"

// TODO: Unify the DMA implementation across backends as part of RSX restructuring.
namespace gl
{
	using dma_mapping_handle = std::pair<u32, gl::buffer*>;

	dma_mapping_handle map_dma(u32 guest_addr, u32 length);
	void clear_dma_resources();

	// GL does not currently support mixed block types...
	class dma_block
	{
	public:
		dma_block() = default;

		void allocate(u32 base_address, u32 block_size);
		void resize(u32 new_length);
		void* map(const utils::address_range32& range) const;

		void set_parent(const dma_block* other);
		const dma_block* head() const { return m_parent ? m_parent : this; }
		bool can_map(const utils::address_range32& range) const;

		u32 base_addr() const { return m_base_address; }
		u32 length() const { return m_data ? static_cast<u32>(m_data->size()) : 0; }
		bool empty() const { return length() == 0; }
		buffer* get() const { return m_data.get(); }
		utils::address_range32 range() const { return utils::address_range32::start_length(m_base_address, length()); }

	protected:
		u32 m_base_address = 0;
		const dma_block* m_parent = nullptr;
		std::unique_ptr<gl::buffer> m_data;
	};
}
