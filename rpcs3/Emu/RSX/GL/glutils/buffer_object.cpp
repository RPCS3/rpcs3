#include "stdafx.h"
#include "buffer_object.h"

namespace gl
{
	void buffer::allocate(GLsizeiptr size, const void* data_, memory_type type, GLenum usage)
	{
		if (const auto& caps = get_driver_caps();
			caps.ARB_buffer_storage_supported)
		{
			GLenum flags = 0;
			if (type == memory_type::host_visible)
			{
				switch (usage)
				{
				case GL_STREAM_DRAW:
				case GL_STATIC_DRAW:
				case GL_DYNAMIC_DRAW:
					flags |= GL_MAP_WRITE_BIT;
					break;
				case GL_STREAM_READ:
				case GL_STATIC_READ:
				case GL_DYNAMIC_READ:
					flags |= GL_MAP_READ_BIT;
					break;
				default:
					fmt::throw_exception("Unsupported buffer usage 0x%x", usage);
				}
			}
			else
			{
				// Local memory hints
				if (usage == GL_DYNAMIC_COPY)
				{
					flags |= GL_DYNAMIC_STORAGE_BIT;
				}
			}

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
			data(size, data_, usage);
		}

		m_memory_type = type;
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

	void buffer::create(GLsizeiptr size, const void* data_, memory_type type, GLenum usage)
	{
		create();
		allocate(size, data_, type, usage);
	}

	void buffer::create(target target_, GLsizeiptr size, const void* data_, memory_type type, GLenum usage)
	{
		m_target = target_;

		create();
		allocate(size, data_, type, usage);
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

		DSA_CALL2(NamedBufferData, m_id, size, data_, usage);
		m_size = size;
	}

	void buffer::sub_data(GLsizeiptr offset, GLsizeiptr length, GLvoid* data)
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
		if (get_driver_caps().ARB_dsa_supported)
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
