#include "stdafx.h"
#include "fbo.h"
#include "buffer_object.h"
#include "vao.hpp"

#include "Emu/RSX/Common/simple_array.hpp"

namespace gl
{
	const fbo screen{};

	void fbo::create()
	{
		glGenFramebuffers(1, &m_id);
	}

	void fbo::bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	}

	void fbo::blit(const fbo& dst, areai src_area, areai dst_area, buffers buffers_, filter filter_) const
	{
		bind_as(target::read_frame_buffer);
		dst.bind_as(target::draw_frame_buffer);
		glBlitFramebuffer(
			src_area.x1, src_area.y1, src_area.x2, src_area.y2,
			dst_area.x1, dst_area.y1, dst_area.x2, dst_area.y2,
			static_cast<GLbitfield>(buffers_), static_cast<GLenum>(filter_));
	}

	void fbo::bind_as(target target_) const
	{
		glBindFramebuffer(static_cast<int>(target_), id());
	}

	void fbo::remove()
	{
		if (m_id != GL_NONE)
		{
			glDeleteFramebuffers(1, &m_id);
			m_id = GL_NONE;
		}
	}

	bool fbo::created() const
	{
		return m_id != GL_NONE;
	}

	bool fbo::check() const
	{
		GLenum status = DSA_CALL2_RET(CheckNamedFramebufferStatus, m_id, GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			rsx_log.error("FBO check failed: 0x%04x", status);
			return false;
		}

		return true;
	}

	void fbo::recreate()
	{
		if (created())
			remove();

		create();
	}

	void fbo::draw_buffer(const attachment& buffer) const
	{
		GLenum buf = buffer.id();
		DSA_CALL3(NamedFramebufferDrawBuffers, FramebufferDrawBuffers, m_id, 1, &buf);
	}

	void fbo::draw_buffer(swapchain_buffer buffer) const
	{
		GLenum buf = static_cast<GLenum>(buffer);
		DSA_CALL3(NamedFramebufferDrawBuffers, FramebufferDrawBuffers, m_id, 1, &buf);
	}

	void fbo::draw_buffers(const std::initializer_list<attachment>& indexes) const
	{
		rsx::simple_array<GLenum> ids;
		ids.reserve(::size32(indexes));

		for (auto& index : indexes)
			ids.push_back(index.id());

		DSA_CALL3(NamedFramebufferDrawBuffers, FramebufferDrawBuffers, m_id, static_cast<GLsizei>(ids.size()), ids.data());
	}

	void fbo::read_buffer(const attachment& buffer) const
	{
		DSA_CALL3(NamedFramebufferReadBuffer, FramebufferReadBuffer, m_id, buffer.id());
	}

	void fbo::read_buffer(swapchain_buffer buffer) const
	{
		GLenum buf = static_cast<GLenum>(buffer);
		DSA_CALL3(NamedFramebufferReadBuffer, FramebufferReadBuffer, m_id, buf);
	}

	void fbo::draw_arrays(GLenum mode, GLsizei count, GLint first) const
	{
		save_binding_state save(*this);
		glDrawArrays(mode, first, count);
	}

	void fbo::draw_arrays(const buffer& buffer, GLenum mode, GLsizei count, GLint first) const
	{
		buffer.bind(buffer::target::array);
		draw_arrays(mode, count, first);
	}

	void fbo::draw_arrays(const vao& buffer, GLenum mode, GLsizei count, GLint first) const
	{
		buffer.bind();
		draw_arrays(mode, count, first);
	}

	void fbo::draw_elements(GLenum mode, GLsizei count, indices_type type, const GLvoid* indices) const
	{
		save_binding_state save(*this);
		glDrawElements(mode, count, static_cast<GLenum>(type), indices);
	}

	void fbo::draw_elements(const buffer& buffer, GLenum mode, GLsizei count, indices_type type, const GLvoid* indices) const
	{
		buffer.bind(buffer::target::array);
		glDrawElements(mode, count, static_cast<GLenum>(type), indices);
	}

	void fbo::draw_elements(GLenum mode, GLsizei count, indices_type type, const buffer& indices, usz indices_buffer_offset) const
	{
		indices.bind(buffer::target::element_array);
		glDrawElements(mode, count, static_cast<GLenum>(type), reinterpret_cast<GLvoid*>(indices_buffer_offset));
	}

	void fbo::draw_elements(const buffer& buffer_, GLenum mode, GLsizei count, indices_type type, const buffer& indices, usz indices_buffer_offset) const
	{
		buffer_.bind(buffer::target::array);
		draw_elements(mode, count, type, indices, indices_buffer_offset);
	}

	void fbo::draw_elements(GLenum mode, GLsizei count, const GLubyte* indices) const
	{
		draw_elements(mode, count, indices_type::ubyte, indices);
	}

	void fbo::draw_elements(const buffer& buffer, GLenum mode, GLsizei count, const GLubyte* indices) const
	{
		draw_elements(buffer, mode, count, indices_type::ubyte, indices);
	}

	void fbo::draw_elements(GLenum mode, GLsizei count, const GLushort* indices) const
	{
		draw_elements(mode, count, indices_type::ushort, indices);
	}

	void fbo::draw_elements(const buffer& buffer, GLenum mode, GLsizei count, const GLushort* indices) const
	{
		draw_elements(buffer, mode, count, indices_type::ushort, indices);
	}

	void fbo::draw_elements(GLenum mode, GLsizei count, const GLuint* indices) const
	{
		draw_elements(mode, count, indices_type::uint, indices);
	}

	void fbo::draw_elements(const buffer& buffer, GLenum mode, GLsizei count, const GLuint* indices) const
	{
		draw_elements(buffer, mode, count, indices_type::uint, indices);
	}

	void fbo::clear(buffers buffers_) const
	{
		save_binding_state save(*this);
		glClear(static_cast<GLbitfield>(buffers_));
	}

	void fbo::copy_from(const void* pixels, const sizei& size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings) const
	{
		save_binding_state save(*this);
		pixel_settings.apply();
		glDrawPixels(size.width, size.height, static_cast<GLenum>(format_), static_cast<GLenum>(type_), pixels);
	}

	void fbo::copy_from(const buffer& buf, const sizei& size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings) const
	{
		save_binding_state save(*this);
		buffer::save_binding_state save_buffer(buffer::target::pixel_unpack, buf);
		pixel_settings.apply();
		glDrawPixels(size.width, size.height, static_cast<GLenum>(format_), static_cast<GLenum>(type_), nullptr);
	}

	void fbo::copy_to(void* pixels, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings) const
	{
		save_binding_state save(*this);
		pixel_settings.apply();
		glReadPixels(coord.x, coord.y, coord.width, coord.height, static_cast<GLenum>(format_), static_cast<GLenum>(type_), pixels);
	}

	void fbo::copy_to(const buffer& buf, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings) const
	{
		save_binding_state save(*this);
		buffer::save_binding_state save_buffer(buffer::target::pixel_pack, buf);
		pixel_settings.apply();
		glReadPixels(coord.x, coord.y, coord.width, coord.height, static_cast<GLenum>(format_), static_cast<GLenum>(type_), nullptr);
	}

	fbo fbo::get_bound_draw_buffer()
	{
		GLint value;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);

		return{ static_cast<GLuint>(value) };
	}

	fbo fbo::get_bound_read_buffer()
	{
		GLint value;
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value);

		return{ static_cast<GLuint>(value) };
	}

	fbo fbo::get_bound_buffer()
	{
		GLint value;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &value);

		return{ static_cast<GLuint>(value) };
	}

	GLuint fbo::id() const
	{
		return m_id;
	}

	void fbo::set_id(GLuint id)
	{
		m_id = id;
	}

	void fbo::set_extents(const size2i& extents)
	{
		m_size = extents;
	}

	size2i fbo::get_extents() const
	{
		return m_size;
	}

	bool fbo::matches(const std::array<GLuint, 4>& color_targets, GLuint depth_stencil_target) const
	{
		for (u32 index = 0; index < 4; ++index)
		{
			if (color[index].resource_id() != color_targets[index])
			{
				return false;
			}
		}

		const auto depth_resource = depth.resource_id() | depth_stencil.resource_id();
		return (depth_resource == depth_stencil_target);
	}

	bool fbo::references_any(const std::vector<GLuint>& resources) const
	{
		return std::any_of(m_resource_bindings.cbegin(), m_resource_bindings.cend(), [&resources](const auto& e)
			{
				return std::find(resources.cbegin(), resources.cend(), e.second) != resources.cend();
			});
	}
}
