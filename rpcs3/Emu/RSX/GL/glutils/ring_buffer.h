#pragma once

#include "buffer_object.h"

namespace gl
{
	class ring_buffer : public buffer
	{
	protected:

		u32 m_data_loc = 0;
		void* m_memory_mapping = nullptr;

		fence m_fence;

	public:

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

	class legacy_ring_buffer : public ring_buffer
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
	class transient_ring_buffer : public ring_buffer
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
}