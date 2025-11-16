#pragma once

#include "buffer_object.h"
#include "sync.hpp"
#include "Utilities/address_range.h"

namespace gl
{
	class ring_buffer : public buffer
	{
	protected:

		u32 m_data_loc = 0;
		void* m_memory_mapping = nullptr;

		fence m_fence;

	public:
		virtual ~ring_buffer() = default;

		virtual void bind() { buffer::bind(); }

		virtual void recreate(GLsizeiptr size, const void* data = nullptr);

		void create(target target_, GLsizeiptr size, const void* data_ = nullptr);

		virtual std::pair<void*, u32> alloc_from_heap(u32 alloc_size, u16 alignment);

		virtual void remove();

		virtual void reserve_storage_on_heap(u32 /*alloc_size*/) {}

		virtual void unmap() {}

		virtual void flush() {}

		virtual void notify();
	};

	class legacy_ring_buffer final : public ring_buffer
	{
		u32 m_mapped_bytes = 0;
		u32 m_mapping_offset = 0;
		u32 m_alignment_offset = 0;

	public:

		void recreate(GLsizeiptr size, const void* data = nullptr) override;

		void create(target target_, GLsizeiptr size, const void* data_ = nullptr);

		void reserve_storage_on_heap(u32 alloc_size) override;

		std::pair<void*, u32> alloc_from_heap(u32 alloc_size, u16 alignment) override;

		void remove() override;

		void unmap() override;

		void notify() override {}
	};

	// A non-persistent ring buffer
	// Internally maps and unmaps data. Uses persistent storage just like the regular persistent variant
	// Works around drivers that have issues using mapped data for specific sources (e.g AMD proprietary driver with index buffers)
	class transient_ring_buffer final : public ring_buffer
	{
		bool dirty = false;

		void* map_internal(u32 offset, u32 length);

	public:

		void bind() override;

		void recreate(GLsizeiptr size, const void* data = nullptr) override;

		std::pair<void*, u32> alloc_from_heap(u32 alloc_size, u16 alignment) override;

		void flush() override;

		void unmap() override;
	};

	// Simple GPU-side ring buffer with no map/unmap semantics
	class scratch_ring_buffer
	{
		struct barrier
		{
			fence signal;
			utils::address_range32 range;
		};

		buffer m_storage;
		std::vector<barrier> m_barriers;
		u64 m_alloc_pointer = 0;

		void pop_barrier(u32 start, u32 length);

	public:

		scratch_ring_buffer() = default;
		scratch_ring_buffer(const scratch_ring_buffer&) = delete;
		~scratch_ring_buffer();

		void create(buffer::target _target, u64 size, u32 usage_flags = 0);
		void remove();

		u32 alloc(u32 size, u32 alignment);
		void push_barrier(u32 start, u32 length);

		buffer& get() { return m_storage; }
		u64 size() const { return m_storage.size(); }
	};
}