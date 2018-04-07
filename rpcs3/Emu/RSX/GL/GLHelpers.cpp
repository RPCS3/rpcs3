#include "stdafx.h"
#include "GLHelpers.h"
#include "Utilities/Log.h"

namespace gl
{
	blitter *g_hw_blitter = nullptr;
	capabilities g_driver_caps;
	const fbo screen{};

	GLenum draw_mode(rsx::primitive_type in)
	{
		switch (in)
		{
		case rsx::primitive_type::points: return GL_POINTS;
		case rsx::primitive_type::lines: return GL_LINES;
		case rsx::primitive_type::line_loop: return GL_LINE_LOOP;
		case rsx::primitive_type::line_strip: return GL_LINE_STRIP;
		case rsx::primitive_type::triangles: return GL_TRIANGLES;
		case rsx::primitive_type::triangle_strip: return GL_TRIANGLE_STRIP;
		case rsx::primitive_type::triangle_fan: return GL_TRIANGLE_FAN;
		case rsx::primitive_type::quads: return GL_TRIANGLES;
		case rsx::primitive_type::quad_strip: return GL_TRIANGLE_STRIP;
		case rsx::primitive_type::polygon: return GL_TRIANGLES;
		default:
			fmt::throw_exception("unknown primitive type" HERE);
		}
	}

#ifdef WIN32
	void APIENTRY dbgFunc(GLenum source, GLenum type, GLuint id,
		GLenum severity, GLsizei lenght, const GLchar* message,
		const void* userParam)
	{
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
			LOG_ERROR(RSX, "%s", message);
			return;
		default:
			LOG_WARNING(RSX, "%s", message);
			return;
		}
	}
#endif

	void enable_debugging()
	{
#ifdef WIN32
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(static_cast<GLDEBUGPROC>(dbgFunc), nullptr);
#endif
	}

	capabilities &get_driver_caps()
	{
		if (!g_driver_caps.initialized)
			g_driver_caps.initialize();

		return g_driver_caps;
	}

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
			(GLbitfield)buffers_, (GLenum)filter_);
	}

	void fbo::bind_as(target target_) const
	{
		glBindFramebuffer((int)target_, id());
	}

	void fbo::remove()
	{
		glDeleteFramebuffers(1, &m_id);
		m_id = 0;
	}

	bool fbo::created() const
	{
		return m_id != 0;
	}

	bool fbo::check() const
	{
		save_binding_state save(*this);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			LOG_ERROR(RSX, "FBO check failed: 0x%04x", status);
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
		save_binding_state save(*this);
		GLenum buf = buffer.id();
		glDrawBuffers(1, &buf);
	}

	void fbo::draw_buffers(const std::initializer_list<attachment>& indexes) const
	{
		save_binding_state save(*this);
		std::vector<GLenum> ids;
		ids.reserve(indexes.size());

		for (auto &index : indexes)
			ids.push_back(index.id());

		glDrawBuffers((GLsizei)ids.size(), ids.data());
	}

	void fbo::read_buffer(const attachment& buffer) const
	{
		save_binding_state save(*this);
		GLenum buf = buffer.id();

		glReadBuffer(buf);
	}

	void fbo::draw_arrays(rsx::primitive_type mode, GLsizei count, GLint first) const
	{
		save_binding_state save(*this);
		glDrawArrays(draw_mode(mode), first, count);
	}

	void fbo::draw_arrays(const buffer& buffer, rsx::primitive_type mode, GLsizei count, GLint first) const
	{
		buffer.bind(buffer::target::array);
		draw_arrays(mode, count, first);
	}

	void fbo::draw_arrays(const vao& buffer, rsx::primitive_type mode, GLsizei count, GLint first) const
	{
		buffer.bind();
		draw_arrays(mode, count, first);
	}

	void fbo::draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const
	{
		save_binding_state save(*this);
		glDrawElements(draw_mode(mode), count, (GLenum)type, indices);
	}

	void fbo::draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const
	{
		buffer.bind(buffer::target::array);
		glDrawElements(draw_mode(mode), count, (GLenum)type, indices);
	}

	void fbo::draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, size_t indices_buffer_offset) const
	{
		indices.bind(buffer::target::element_array);
		glDrawElements(draw_mode(mode), count, (GLenum)type, (GLvoid*)indices_buffer_offset);
	}

	void fbo::draw_elements(const buffer& buffer_, rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, size_t indices_buffer_offset) const
	{
		buffer_.bind(buffer::target::array);
		draw_elements(mode, count, type, indices, indices_buffer_offset);
	}

	void fbo::draw_elements(rsx::primitive_type mode, GLsizei count, const GLubyte *indices) const
	{
		draw_elements(mode, count, indices_type::ubyte, indices);
	}

	void fbo::draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLubyte *indices) const
	{
		draw_elements(buffer, mode, count, indices_type::ubyte, indices);
	}

	void fbo::draw_elements(rsx::primitive_type mode, GLsizei count, const GLushort *indices) const
	{
		draw_elements(mode, count, indices_type::ushort, indices);
	}

	void fbo::draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLushort *indices) const
	{
		draw_elements(buffer, mode, count, indices_type::ushort, indices);
	}

	void fbo::draw_elements(rsx::primitive_type mode, GLsizei count, const GLuint *indices) const
	{
		draw_elements(mode, count, indices_type::uint, indices);
	}

	void fbo::draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLuint *indices) const
	{
		draw_elements(buffer, mode, count, indices_type::uint, indices);
	}

	void fbo::clear(buffers buffers_) const
	{
		save_binding_state save(*this);
		glClear((GLbitfield)buffers_);
	}

	void fbo::clear(buffers buffers_, color4f color_value, double depth_value, u8 stencil_value) const
	{
		save_binding_state save(*this);
		glClearColor(color_value.r, color_value.g, color_value.b, color_value.a);
		glClearDepth(depth_value);
		glClearStencil(stencil_value);
		clear(buffers_);
	}

	void fbo::copy_from(const void* pixels, sizei size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings) const
	{
		save_binding_state save(*this);
		pixel_settings.apply();
		glDrawPixels(size.width, size.height, (GLenum)format_, (GLenum)type_, pixels);
	}

	void fbo::copy_from(const buffer& buf, sizei size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings) const
	{
		save_binding_state save(*this);
		buffer::save_binding_state save_buffer(buffer::target::pixel_unpack, buf);
		pixel_settings.apply();
		glDrawPixels(size.width, size.height, (GLenum)format_, (GLenum)type_, nullptr);
	}

	void fbo::copy_to(void* pixels, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings) const
	{
		save_binding_state save(*this);
		pixel_settings.apply();
		glReadPixels(coord.x, coord.y, coord.width, coord.height, (GLenum)format_, (GLenum)type_, pixels);
	}

	void fbo::copy_to(const buffer& buf, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings) const
	{
		save_binding_state save(*this);
		buffer::save_binding_state save_buffer(buffer::target::pixel_pack, buf);
		pixel_settings.apply();
		glReadPixels(coord.x, coord.y, coord.width, coord.height, (GLenum)format_, (GLenum)type_, nullptr);
	}

	fbo fbo::get_binded_draw_buffer()
	{
		GLint value;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);

		return{ (GLuint)value };
	}

	fbo fbo::get_binded_read_buffer()
	{
		GLint value;
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value);

		return{ (GLuint)value };
	}

	fbo fbo::get_binded_buffer()
	{
		GLint value;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &value);

		return{ (GLuint)value };
	}

	GLuint fbo::id() const
	{
		return m_id;
	}

	void fbo::set_id(GLuint id)
	{
		m_id = id;
	}

	void fbo::set_extents(size2i extents)
	{
		m_size = extents;
	}

	size2i fbo::get_extents() const
	{
		return m_size;
	}

	bool is_primitive_native(rsx::primitive_type in)
	{
		switch (in)
		{
		case rsx::primitive_type::points:
		case rsx::primitive_type::lines:
		case rsx::primitive_type::line_loop:
		case rsx::primitive_type::line_strip:
		case rsx::primitive_type::triangles:
		case rsx::primitive_type::triangle_strip:
		case rsx::primitive_type::triangle_fan:
		case rsx::primitive_type::quad_strip:
			return true;
		case rsx::primitive_type::quads:
		case rsx::primitive_type::polygon:
			return false;
		default:
			fmt::throw_exception("unknown primitive type" HERE);
		}
	}

	attrib_t vao::operator[](u32 index) const noexcept
	{
		return attrib_t(index);
	}
}
