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

	void texture::settings::apply(const texture &texture) const
	{
		save_binding_state save(texture);

		texture.pixel_unpack_settings().apply();

		if (compressed_format(m_internal_format))
		{
			int compressed_image_size = m_compressed_image_size;
			if (!compressed_image_size)
			{
				switch (m_internal_format)
				{
				case texture::internal_format::compressed_rgb_s3tc_dxt1:
					compressed_image_size = ((m_width + 2) / 3) * ((m_height + 2) / 3) * 6;
					break;

				case texture::internal_format::compressed_rgba_s3tc_dxt1:
					compressed_image_size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 8;
					break;

				case texture::internal_format::compressed_rgba_s3tc_dxt3:
				case texture::internal_format::compressed_rgba_s3tc_dxt5:
					compressed_image_size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 16;
					break;
				default:
					fmt::throw_exception("Tried to load unimplemented internal_format type." HERE);
					break;
				}
			}

			if (m_parent->get_target() != gl::texture::target::texture2D)
				fmt::throw_exception("Mutable compressed texture of non-2D type is unimplemented" HERE);

			glCompressedTexImage2D((GLenum)m_parent->get_target(), m_level, (GLint)m_internal_format, m_width, m_height, 0, compressed_image_size, m_pixels);
		}
		else
		{
			switch ((GLenum)m_parent->get_target())
			{
			case GL_TEXTURE_1D:
			{
				glTexImage1D(GL_TEXTURE_1D, m_level, (GLint)m_internal_format, m_width, 0, (GLint)m_format, (GLint)m_type, m_pixels);
				break;
			}
			case GL_TEXTURE_2D:
			{
				glTexImage2D(GL_TEXTURE_2D, m_level, (GLint)m_internal_format, m_width, m_height, 0, (GLint)m_format, (GLint)m_type, m_pixels);
				break;
			}
			case GL_TEXTURE_3D:
			{
				glTexImage3D(GL_TEXTURE_3D, m_level, (GLint)m_internal_format, m_width, m_height, m_depth, 0, (GLint)m_format, (GLint)m_type, m_pixels);
				break;
			}
			case GL_TEXTURE_CUBE_MAP:
			{
				for (int face = 0; face < 6; ++face)
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_level, (GLint)m_internal_format, m_width, m_height, 0, (GLint)m_format, (GLint)m_type, m_pixels);
				break;
			}
			}
		}

		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_MAX_LEVEL, m_max_level);

		if (m_pixels && m_generate_mipmap)
		{
			glGenerateMipmap((GLenum)m_parent->get_target());
		}

		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_WRAP_S, (GLint)m_wrap_s);
		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_WRAP_T, (GLint)m_wrap_t);
		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_WRAP_R, (GLint)m_wrap_r);

		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_COMPARE_MODE, (GLint)m_compare_mode);
		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_COMPARE_FUNC, (GLint)m_compare_func);

		glTexParameterf((GLenum)m_parent->get_target(), GL_TEXTURE_MIN_LOD, m_max_lod);
		glTexParameterf((GLenum)m_parent->get_target(), GL_TEXTURE_MAX_LOD, m_min_lod);
		glTexParameterf((GLenum)m_parent->get_target(), GL_TEXTURE_LOD_BIAS, m_lod);

		glTexParameterfv((GLenum)m_parent->get_target(), GL_TEXTURE_BORDER_COLOR, m_border_color.rgba);

		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_MIN_FILTER, (GLint)m_min_filter);
		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_MAG_FILTER, (GLint)m_mag_filter);

		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_SWIZZLE_R, (GLint)m_swizzle_r);
		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_SWIZZLE_G, (GLint)m_swizzle_g);
		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_SWIZZLE_B, (GLint)m_swizzle_b);
		glTexParameteri((GLenum)m_parent->get_target(), GL_TEXTURE_SWIZZLE_A, (GLint)m_swizzle_a);

		glTexParameterf((GLenum)m_parent->get_target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, m_aniso);
	}

	void texture::settings::apply()
	{
		if (m_parent)
		{
			apply(*m_parent);
			m_parent = nullptr;
		}
	}

	texture::settings& texture::settings::swizzle(texture::channel r, texture::channel g, texture::channel b, texture::channel a)
	{
		m_swizzle_r = r;
		m_swizzle_g = g;
		m_swizzle_b = b;
		m_swizzle_a = a;

		return *this;
	}

	texture::settings& texture::settings::format(texture::format format)
	{
		m_format = format;
		return *this;
	}

	texture::settings& texture::settings::type(texture::type type)
	{
		m_type = type;
		return *this;
	}

	texture::settings& texture::settings::internal_format(texture::internal_format format)
	{
		m_internal_format = format;
		return *this;
	}

	texture::settings& texture::settings::filter(min_filter min_filter, gl::filter mag_filter)
	{
		m_min_filter = min_filter;
		m_mag_filter = mag_filter;

		return *this;
	}

	texture::settings& texture::settings::width(uint width)
	{
		m_width = width;
		return *this;
	}

	texture::settings& texture::settings::height(uint height)
	{
		m_height = height;
		return *this;
	}

	texture::settings& texture::settings::depth(uint depth)
	{
		m_depth = depth;
		return *this;
	}

	texture::settings& texture::settings::size(sizei size)
	{
		return width(size.width).height(size.height);
	}

	texture::settings& texture::settings::level(int value)
	{
		m_level = value;
		return *this;
	}

	texture::settings& texture::settings::compressed_image_size(int size)
	{
		m_compressed_image_size = size;
		return *this;
	}

	texture::settings& texture::settings::pixels(const void* pixels)
	{
		m_pixels = pixels;
		return *this;
	}

	texture::settings& texture::settings::aniso(float value)
	{
		m_aniso = value;
		return *this;
	}

	texture::settings& texture::settings::compare_mode(texture::compare_mode value)
	{
		m_compare_mode = value;
		return *this;
	}
	texture::settings& texture::settings::compare_func(texture::compare_func value)
	{
		m_compare_func = value;
		return *this;
	}
	texture::settings& texture::settings::compare(texture::compare_func func, texture::compare_mode mode)
	{
		return compare_func(func).compare_mode(mode);
	}

	texture::settings& texture::settings::wrap_s(texture::wrap value)
	{
		m_wrap_s = value;
		return *this;
	}
	texture::settings& texture::settings::wrap_t(texture::wrap value)
	{
		m_wrap_t = value;
		return *this;
	}
	texture::settings& texture::settings::wrap_r(texture::wrap value)
	{
		m_wrap_r = value;
		return *this;
	}
	texture::settings& texture::settings::wrap(texture::wrap s, texture::wrap t, texture::wrap r)
	{
		return wrap_s(s).wrap_t(t).wrap_r(r);
	}

	texture::settings& texture::settings::max_lod(float value)
	{
		m_max_lod = value;
		return *this;
	}
	texture::settings& texture::settings::min_lod(float value)
	{
		m_min_lod = value;
		return *this;
	}
	texture::settings& texture::settings::lod(float value)
	{
		m_lod = value;
		return *this;
	}
	texture::settings& texture::settings::max_level(int value)
	{
		m_max_level = value;
		return *this;
	}
	texture::settings& texture::settings::generate_mipmap(bool value)
	{
		m_generate_mipmap = value;
		return *this;
	}
	texture::settings& texture::settings::mipmap(int level, int max_level, float lod, float min_lod, float max_lod, bool generate)
	{
		return this->level(level).max_level(max_level).lod(lod).min_lod(min_lod).max_lod(max_lod).generate_mipmap(generate);
	}

	texture::settings& texture::settings::border_color(color4f value)
	{
		m_border_color = value;
		return *this;
	}

	texture_view texture::with_level(int level)
	{
		return{ get_target(), id() };
	}

	texture::settings texture::config()
	{
		return{ this };
	}

	void texture::config(const settings& settings_)
	{
		settings_.apply(*this);
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
