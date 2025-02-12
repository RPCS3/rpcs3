#pragma once

#include "Emu/RSX/GL/OpenGL.h"

namespace gl
{
	class buffer
	{
	public:
		enum class target
		{
			pixel_pack = GL_PIXEL_PACK_BUFFER,
			pixel_unpack = GL_PIXEL_UNPACK_BUFFER,
			array = GL_ARRAY_BUFFER,
			element_array = GL_ELEMENT_ARRAY_BUFFER,
			uniform = GL_UNIFORM_BUFFER,
			texture = GL_TEXTURE_BUFFER,
			ssbo = GL_SHADER_STORAGE_BUFFER
		};

		enum class access
		{
			read = GL_MAP_READ_BIT,
			write = GL_MAP_WRITE_BIT,
			rw = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
			persistent_rw = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT
		};

		enum class memory_type
		{
			undefined = 0,
			local = 1,
			host_visible = 2,
			userptr = 4
		};

		enum usage
		{
			host_write     = (1 << 0),
			host_read      = (1 << 1),
			persistent_map = (1 << 2),
			dynamic_update = (1 << 3),
		};

		class save_binding_state
		{
			GLint m_last_binding = GL_ZERO;
			GLenum m_target = GL_NONE;

		public:
			save_binding_state(target target_, const buffer& new_state) : save_binding_state(target_)
			{
				new_state.bind(target_);
			}

			save_binding_state(target target_)
			{
				GLenum pname{};
				switch (target_)
				{
				case target::pixel_pack: pname = GL_PIXEL_PACK_BUFFER_BINDING; break;
				case target::pixel_unpack: pname = GL_PIXEL_UNPACK_BUFFER_BINDING; break;
				case target::array: pname = GL_ARRAY_BUFFER_BINDING; break;
				case target::element_array: pname = GL_ELEMENT_ARRAY_BUFFER_BINDING; break;
				case target::uniform: pname = GL_UNIFORM_BUFFER_BINDING; break;
				case target::texture: pname = GL_TEXTURE_BUFFER_BINDING; break;
				case target::ssbo: pname = GL_SHADER_STORAGE_BUFFER_BINDING; break;
				default: fmt::throw_exception("Invalid binding state target (0x%x)", static_cast<int>(target_));
				}

				glGetIntegerv(pname, &m_last_binding);
				m_target = static_cast<GLenum>(target_);
			}

			~save_binding_state()
			{
				if (!m_target)
				{
					return;
				}

				glBindBuffer(m_target, m_last_binding);
			}
		};

	protected:
		GLuint m_id = GL_NONE;
		GLsizeiptr m_size = 0;
		target m_target = target::array;
		memory_type m_memory_type = memory_type::undefined;

		// Metadata
		mutable std::pair<u32, u32> m_bound_range{};

		void allocate(GLsizeiptr size, const void* data_, memory_type type, GLuint usage_bits);

	public:
		buffer() = default;
		buffer(const buffer&) = delete;
		~buffer();

		void recreate();
		void recreate(GLsizeiptr size, const void* data = nullptr);

		void create();
		void create(GLsizeiptr size, const void* data_ = nullptr, memory_type type = memory_type::local, GLuint usage_bits = 0);
		void create(target target_, GLsizeiptr size, const void* data_ = nullptr, memory_type type = memory_type::local, GLuint usage_bits = 0);

		void remove();

		void bind(target target_) const { glBindBuffer(static_cast<GLenum>(target_), m_id); }
		void bind() const { bind(current_target()); }

		void data(GLsizeiptr size, const void* data_ = nullptr, GLenum usage = GL_STREAM_DRAW);
		void sub_data(GLsizeiptr offset, GLsizeiptr length, const GLvoid* data);

		GLubyte* map(GLsizeiptr offset, GLsizeiptr length, access access_);
		void unmap();

		void bind_range(u32 index, u32 offset, u32 size) const;
		void bind_range(target target_, u32 index, u32 offset, u32 size) const;

		void copy_to(buffer* other, u64 src_offset, u64 dst_offset, u64 size);

		target current_target() const { return m_target; }
		GLsizeiptr size() const { return m_size; }
		uint id() const { return m_id; }
		void set_id(uint id) { m_id = id; }
		bool created() const { return m_id != GL_NONE; }
		std::pair<u32, u32> bound_range() const { return m_bound_range; }

		explicit operator bool() const { return created(); }
	};

	class buffer_view
	{
		buffer* m_buffer = nullptr;
		u32 m_offset = 0;
		u32 m_range = 0;
		GLenum m_format = GL_R8UI;

	public:
		buffer_view(buffer* _buffer, u32 offset, u32 range, GLenum format = GL_R8UI)
			: m_buffer(_buffer), m_offset(offset), m_range(range), m_format(format)
		{}

		buffer_view() = default;

		void update(buffer* _buffer, u32 offset, u32 range, GLenum format = GL_R8UI);

		u32 offset() const { return m_offset; }

		u32 range() const { return m_range; }

		u32 format() const { return m_format; }

		buffer* value() const { return m_buffer; }

		bool in_range(u32 address, u32 size, u32& new_offset) const;
	};
}
