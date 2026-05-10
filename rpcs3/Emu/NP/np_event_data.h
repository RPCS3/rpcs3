#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "util/asm.hpp"

namespace np
{
	class event_data
	{
	public:
		event_data(u32 vm_addr, u32 initial_size, u32 max_size)
			: m_max_size(max_size), m_cur_size(utils::align(initial_size, 4))
		{
			m_data_ptr.set(vm_addr);
		}

		event_data()
		{
		}

		u8* data()
		{
			return m_data_ptr.get_ptr();
		}

		const u8* data() const
		{
			return m_data_ptr.get_ptr();
		}

		u32 size() const
		{
			return m_cur_size;
		}

		u32 addr() const
		{
			return m_data_ptr.addr();
		}

		template <typename T>
		void add_relocation(vm::bptr<T>& dest)
		{
			u8* dest_ptr = reinterpret_cast<u8*>(&dest);
			ensure(dest_ptr >= m_data_ptr.get_ptr() && dest_ptr < (m_data_ptr.get_ptr() + m_cur_size), "event_data::allocate: dest is out of bounds!");

			m_relocs.push_back(static_cast<u32>(dest_ptr - m_data_ptr.get_ptr()));
		}

		template <typename T>
		T* allocate(u32 size, vm::bptr<T>& dest)
		{
			const u32 to_alloc = utils::align(size, 4);
			ensure((m_cur_size + to_alloc) <= m_max_size, "event_data::allocate: size would overflow the allocated buffer!");

			u8* dest_ptr = reinterpret_cast<u8*>(&dest);
			ensure(dest_ptr >= m_data_ptr.get_ptr() && dest_ptr < (m_data_ptr.get_ptr() + m_cur_size), "event_data::allocate: dest is out of bounds!");

			const u32 offset_alloc = m_cur_size;

			// Set the vm::bptr to new allocated portion of memory
			dest.set(m_data_ptr.addr() + offset_alloc);
			// Save the relocation offset
			m_relocs.push_back(static_cast<u32>(dest_ptr - m_data_ptr.get_ptr()));
			// Update currently allocated space
			m_cur_size += to_alloc;
			// Return actual pointer to allocated
			return reinterpret_cast<T*>(m_data_ptr.get_ptr() + offset_alloc);
		}

		template <typename T>
		void allocate_ptr_array(u32 num_pointers, vm::bpptr<T>& dest)
		{
			const u32 to_alloc = num_pointers * sizeof(u32);
			ensure((m_cur_size + to_alloc) <= m_max_size, "event_data::allocate: size would overflow the allocated buffer!");

			u8* dest_ptr = reinterpret_cast<u8*>(&dest);
			ensure(dest_ptr >= m_data_ptr.get_ptr() && dest_ptr < (m_data_ptr.get_ptr() + m_cur_size), "event_data::allocate_ptr_array: dest is out of bounds!");

			const u32 offset_alloc = m_cur_size;

			// Set the vm::bpptr to relative offset
			dest.set(m_data_ptr.addr() + offset_alloc);
			// Save the relocation offset
			m_relocs.push_back(static_cast<u32>(dest_ptr - m_data_ptr.get_ptr()));
			// Add all the pointers to the relocation
			for (u32 i = 0; i < num_pointers; i++)
			{
				m_relocs.push_back(offset_alloc + (i * 4));
			}
			// Update currently allocated space
			m_cur_size += to_alloc;
		}

		void apply_relocations(u32 new_addr)
		{
			u32 diff_offset = new_addr - m_data_ptr.addr();
			for (const auto offset : m_relocs)
			{
				auto* ptr = reinterpret_cast<be_t<u32>*>(m_data_ptr.get_ptr() + offset);
				*ptr += diff_offset;
			}
		}

	private:
		vm::bptr<u8> m_data_ptr{};
		u32 m_max_size = 0, m_cur_size = 0;
		std::vector<u32> m_relocs;
	};
} // namespace np
