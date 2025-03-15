#include "stdafx.h"
#include "buffer_object.h"
#include "common.h"

namespace gl
{
	void buffer::allocate(GLsizeiptr size, const void* data_, memory_type type, GLuint usage_flags)
	{
		m_memory_type = type;

		if (const auto& caps = get_driver_caps();
			type != memory_type::userptr && caps.ARB_buffer_storage_supported)
		{
			GLenum flags = 0;
			if (usage_flags & usage::host_write)
			{
				flags |= GL_MAP_WRITE_BIT;
			}
			if (usage_flags & usage::host_read)
			{
				flags |= GL_MAP_READ_BIT;
			}
			if (usage_flags & usage::persistent_map)
			{
				flags |= GL_MAP_PERSISTENT_BIT;
			}
			if (usage_flags & usage::dynamic_update)
			{
				flags |= GL_DYNAMIC_STORAGE_BIT;
			}

			ensure((flags & (GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT)) != (GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT),
				"Mutually exclusive usage flags set!");

			ensure(type == memory_type::local || flags != 0, "Host-visible memory must have usage flags set!");

			if ((flags & GL_MAP_READ_BIT) && !caps.vendor_AMD)
			{
				// This flag stops NVIDIA from allocating read-only memory in VRAM.
				// NOTE: On AMD, allocating client-side memory via CLIENT_STORAGE_BIT or
				// making use of GL_AMD_pinned_memory brings everything down to a crawl.
				// Afaict there is no reason for this; disabling pixel pack/unpack operations does not alleviate the problem.
				// The driver seems to eventually figure out the optimal storage location by itself.
				flags |= GL_CLIENT_STORAGE_BIT;
			}

			DSA_CALL2(NamedBufferStorage, m_id, size, data_, flags);
			m_size = size;
		}
		else
		{
			data(size, data_, GL_STREAM_COPY);
		}
	}

	buffer::~buffer()
	{
		if (created())
			remove();
	}

	void buffer::recreate()
	{
		if (created())
		{
			remove();
		}

		create();
	}

	void buffer::recreate(GLsizeiptr size, const void* data)
	{
		if (created())
		{
			remove();
		}

		create(size, data);
	}

	void buffer::create()
	{
		glGenBuffers(1, &m_id);
		save_binding_state save(current_target(), *this);
	}

	void buffer::create(GLsizeiptr size, const void* data_, memory_type type, GLuint usage_bits)
	{
		create();
		allocate(size, data_, type, usage_bits);
	}

	void buffer::create(target target_, GLsizeiptr size, const void* data_, memory_type type, GLuint usage_bits)
	{
		m_target = target_;

		create();
		allocate(size, data_, type, usage_bits);
	}

	void buffer::remove()
	{
		if (m_id != GL_NONE)
		{
			glDeleteBuffers(1, &m_id);
			m_id = GL_NONE;
			m_size = 0;
		}
	}

	void buffer::data(GLsizeiptr size, const void* data_, GLenum usage)
	{
		ensure(m_memory_type != memory_type::local);

		m_size = size;

		if (m_memory_type == memory_type::userptr)
		{
			glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, m_id);
			glBufferData(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, size, data_, usage);
			return;
		}

		DSA_CALL2(NamedBufferData, m_id, size, data_, usage);
	}

	void buffer::sub_data(GLsizeiptr offset, GLsizeiptr length, const GLvoid* data)
	{
		ensure(m_memory_type == memory_type::local);
		DSA_CALL2(NamedBufferSubData, m_id, offset, length, data);
	}

	GLubyte* buffer::map(GLsizeiptr offset, GLsizeiptr length, access access_)
	{
		ensure(m_memory_type == memory_type::host_visible);

		GLenum access_bits = static_cast<GLenum>(access_);
		if (access_bits == GL_MAP_WRITE_BIT) access_bits |= GL_MAP_UNSYNCHRONIZED_BIT;

		auto raw_data = DSA_CALL2_RET(MapNamedBufferRange, id(), offset, length, access_bits);
		return reinterpret_cast<GLubyte*>(raw_data);
	}

	void buffer::unmap()
	{
		ensure(m_memory_type == memory_type::host_visible);
		DSA_CALL2(UnmapNamedBuffer, id());
	}

	void buffer::bind_range(u32 index, u32 offset, u32 size) const
	{
		m_bound_range = { offset, size };
		glBindBufferRange(static_cast<GLenum>(current_target()), index, id(), offset, size);
	}

	void buffer::bind_range(target target_, u32 index, u32 offset, u32 size) const
	{
		m_bound_range = { offset, size };
		glBindBufferRange(static_cast<GLenum>(target_), index, id(), offset, size);
	}

	void buffer::copy_to(buffer* other, u64 src_offset, u64 dst_offset, u64 size)
	{
		if (get_driver_caps().ARB_direct_state_access_supported)
		{
			glCopyNamedBufferSubData(this->id(), other->id(), src_offset, dst_offset, size);
		}
		else
		{
			glNamedCopyBufferSubDataEXT(this->id(), other->id(), src_offset, dst_offset, size);
		}
	}

	// Buffer view
	void buffer_view::update(buffer* _buffer, u32 offset, u32 range, GLenum format)
	{
		ensure(_buffer->size() >= (offset + range));
		m_buffer = _buffer;
		m_offset = offset;
		m_range = range;
		m_format = format;
	}

	bool buffer_view::in_range(u32 address, u32 size, u32& new_offset) const
	{
		if (address < m_offset)
			return false;

		const u32 _offset = address - m_offset;
		if (m_range < _offset)
			return false;

		const auto remaining = m_range - _offset;
		if (size <= remaining)
		{
			new_offset = _offset;
			return true;
		}

		return false;
	}
}
