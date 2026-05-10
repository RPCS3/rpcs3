#pragma once

#include "common.h"
#include "image.h"
#include "pixel_settings.hpp"

#include "Utilities/geometry.h"

namespace gl
{
	enum class buffers
	{
		none = 0,
		color = GL_COLOR_BUFFER_BIT,
		depth = GL_DEPTH_BUFFER_BIT,
		stencil = GL_STENCIL_BUFFER_BIT,

		color_depth = color | depth,
		color_depth_stencil = color | depth | stencil,
		color_stencil = color | stencil,

		depth_stencil = depth | stencil
	};

	enum class indices_type
	{
		ubyte = GL_UNSIGNED_BYTE,
		ushort = GL_UNSIGNED_SHORT,
		uint = GL_UNSIGNED_INT
	};

	class vao;

	class fbo
	{
		GLuint m_id = GL_NONE;
		size2i m_size;

	protected:
		std::unordered_map<GLenum, GLuint> m_resource_bindings;

	public:
		fbo() = default;

		fbo(GLuint id)
		{
			set_id(id);
		}

		~fbo()
		{
			if (created())
				remove();
		}

		class save_binding_state
		{
			GLint m_last_binding;
			bool reset = true;

		public:
			save_binding_state(const fbo& new_binding)
			{
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_last_binding);

				if (m_last_binding + 0u != new_binding.id())
					new_binding.bind();
				else
					reset = false;
			}

			~save_binding_state()
			{
				if (reset)
					glBindFramebuffer(GL_FRAMEBUFFER, m_last_binding);
			}
		};

		class attachment
		{
		public:
			enum class type
			{
				color = GL_COLOR_ATTACHMENT0,
				depth = GL_DEPTH_ATTACHMENT,
				stencil = GL_STENCIL_ATTACHMENT,
				depth_stencil = GL_DEPTH_STENCIL_ATTACHMENT
			};

		protected:
			GLuint m_id = GL_NONE;
			fbo& m_parent;

			attachment(fbo& parent)
				: m_parent(parent)
			{}

		public:
			attachment(fbo& parent, type type)
				: m_id(static_cast<int>(type))
				, m_parent(parent)
			{
			}

			void set_id(uint id)
			{
				m_id = id;
			}

			uint id() const
			{
				return m_id;
			}

			GLuint resource_id() const
			{
				const auto found = m_parent.m_resource_bindings.find(m_id);
				if (found != m_parent.m_resource_bindings.end())
				{
					return found->second;
				}

				return 0;
			}

			void operator = (const texture& rhs)
			{
				ensure(rhs.get_target() == texture::target::texture2D ||
					rhs.get_target() == texture::target::texture2DMS);

				m_parent.m_resource_bindings[m_id] = rhs.id();
				DSA_CALL2(NamedFramebufferTexture, m_parent.id(), m_id, rhs.id(), 0);
			}

			void operator = (const GLuint rhs)
			{
				m_parent.m_resource_bindings[m_id] = rhs;
				DSA_CALL2(NamedFramebufferTexture, m_parent.id(), m_id, rhs, 0);
			}
		};

		class indexed_attachment : public attachment
		{
		public:
			indexed_attachment(fbo& parent, type type) : attachment(parent, type)
			{
			}

			attachment operator[](int index) const
			{
				return{ m_parent, type(id() + index) };
			}

			std::vector<attachment> range(int from, int count) const
			{
				std::vector<attachment> result;

				for (int i = from; i < from + count; ++i)
					result.push_back((*this)[i]);

				return result;
			}

			using attachment::operator =;
		};

		struct null_attachment : public attachment
		{
			null_attachment(fbo& parent)
				: attachment(parent)
			{}
		};

		indexed_attachment color{ *this, attachment::type::color };
		attachment depth{ *this, attachment::type::depth };
		attachment stencil{ *this, attachment::type::stencil };
		attachment depth_stencil{ *this, attachment::type::depth_stencil };
		null_attachment no_color{ *this };

		enum class target
		{
			read_frame_buffer = GL_READ_FRAMEBUFFER,
			draw_frame_buffer = GL_DRAW_FRAMEBUFFER
		};

		enum class swapchain_buffer
		{
			back = GL_BACK,
			front = GL_FRONT
		};

		void create();
		void bind() const;
		void blit(const fbo& dst, areai src_area, areai dst_area, buffers buffers_ = buffers::color, filter filter_ = filter::nearest) const;
		void bind_as(target target_) const;
		void remove();
		bool created() const;
		bool check() const;

		void recreate();
		void draw_buffer(const attachment& buffer) const;
		void draw_buffer(swapchain_buffer buffer) const;
		void draw_buffers(const std::initializer_list<attachment>& indexes) const;

		void read_buffer(const attachment& buffer) const;
		void read_buffer(swapchain_buffer buffer) const;

		void draw_arrays(GLenum mode, GLsizei count, GLint first = 0) const;
		void draw_arrays(const buffer& buffer, GLenum mode, GLsizei count, GLint first = 0) const;
		void draw_arrays(const vao& buffer, GLenum mode, GLsizei count, GLint first = 0) const;

		void draw_elements(GLenum mode, GLsizei count, indices_type type, const GLvoid* indices) const;
		void draw_elements(const buffer& buffer, GLenum mode, GLsizei count, indices_type type, const GLvoid* indices) const;
		void draw_elements(GLenum mode, GLsizei count, indices_type type, const buffer& indices, usz indices_buffer_offset = 0) const;
		void draw_elements(const buffer& buffer_, GLenum mode, GLsizei count, indices_type type, const buffer& indices, usz indices_buffer_offset = 0) const;
		void draw_elements(GLenum mode, GLsizei count, const GLubyte* indices) const;
		void draw_elements(const buffer& buffer, GLenum mode, GLsizei count, const GLubyte* indices) const;
		void draw_elements(GLenum mode, GLsizei count, const GLushort* indices) const;
		void draw_elements(const buffer& buffer, GLenum mode, GLsizei count, const GLushort* indices) const;
		void draw_elements(GLenum mode, GLsizei count, const GLuint* indices) const;
		void draw_elements(const buffer& buffer, GLenum mode, GLsizei count, const GLuint* indices) const;

		void clear(buffers buffers_) const;

		void copy_from(const void* pixels, const sizei& size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings = pixel_unpack_settings()) const;
		void copy_from(const buffer& buf, const sizei& size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings = pixel_unpack_settings()) const;

		void copy_to(void* pixels, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings = pixel_pack_settings()) const;
		void copy_to(const buffer& buf, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings = pixel_pack_settings()) const;

		static fbo get_bound_draw_buffer();
		static fbo get_bound_read_buffer();
		static fbo get_bound_buffer();

		GLuint id() const;
		void set_id(GLuint id);

		void set_extents(const size2i& extents);
		size2i get_extents() const;

		bool matches(const std::array<GLuint, 4>& color_targets, GLuint depth_stencil_target) const;
		bool references_any(const std::vector<GLuint>& resources) const;

		explicit operator bool() const
		{
			return created();
		}
	};

	extern const fbo screen;
}
