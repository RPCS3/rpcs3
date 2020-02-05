#include "stdafx.h"
#include "GLHelpers.h"
#include "GLTexture.h"
#include "GLCompute.h"
#include "Utilities/Log.h"

namespace gl
{
	std::unordered_map<u32, std::unique_ptr<gl::compute_task>> g_compute_tasks;
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

	void destroy_compute_tasks()
	{
		for (auto& [key, prog] : g_compute_tasks)
		{
			prog->destroy();
		}

		g_compute_tasks.clear();
	}

#ifdef WIN32
	void APIENTRY dbgFunc(GLenum source, GLenum type, GLuint id,
		GLenum severity, GLsizei lenght, const GLchar* message,
		const void* userParam)
	{
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
		{
			rsx_log.error("%s", message);
			return;
		}
		default:
		{
			rsx_log.warning("%s", message);
			return;
		}
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
			static_cast<GLbitfield>(buffers_), static_cast<GLenum>(filter_));
	}

	void fbo::bind_as(target target_) const
	{
		glBindFramebuffer(static_cast<int>(target_), id());
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

		glDrawBuffers(::narrow<GLsizei>(ids.size()), ids.data());
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
		glDrawElements(draw_mode(mode), count, static_cast<GLenum>(type), indices);
	}

	void fbo::draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const
	{
		buffer.bind(buffer::target::array);
		glDrawElements(draw_mode(mode), count, static_cast<GLenum>(type), indices);
	}

	void fbo::draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, size_t indices_buffer_offset) const
	{
		indices.bind(buffer::target::element_array);
		glDrawElements(draw_mode(mode), count, static_cast<GLenum>(type), reinterpret_cast<GLvoid*>(indices_buffer_offset));
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
		glClear(static_cast<GLbitfield>(buffers_));
	}

	void fbo::clear(buffers buffers_, color4f color_value, double depth_value, u8 stencil_value) const
	{
		save_binding_state save(*this);
		glClearColor(color_value.r, color_value.g, color_value.b, color_value.a);
		glClearDepth(depth_value);
		glClearStencil(stencil_value);
		clear(buffers_);
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

	fbo fbo::get_binded_draw_buffer()
	{
		GLint value;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);

		return{ static_cast<GLuint>(value) };
	}

	fbo fbo::get_binded_read_buffer()
	{
		GLint value;
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value);

		return{ static_cast<GLuint>(value) };
	}

	fbo fbo::get_binded_buffer()
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
		for (const auto &e : m_resource_bindings)
		{
			if (std::find(resources.begin(), resources.end(), e.second) != resources.end())
				return true;
		}

		return false;
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

	void blitter::scale_image(gl::command_context& cmd, const texture* src, texture* dst, areai src_rect, areai dst_rect,
		bool linear_interpolation, const rsx::typeless_xfer& xfer_info)
	{
		std::unique_ptr<texture> typeless_src;
		std::unique_ptr<texture> typeless_dst;
		const gl::texture* real_src = src;
		const gl::texture* real_dst = dst;

		// Optimization pass; check for pass-through data transfer
		if (!xfer_info.flip_horizontal && !xfer_info.flip_vertical && src_rect.height() == dst_rect.height())
		{
			auto src_w = src_rect.width();
			auto dst_w = dst_rect.width();

			if (xfer_info.src_is_typeless) src_w *= xfer_info.src_scaling_hint;
			if (xfer_info.dst_is_typeless) dst_w *= xfer_info.dst_scaling_hint;

			if (src_w == dst_w)
			{
				// Final dimensions are a match
				if (xfer_info.src_is_typeless || xfer_info.dst_is_typeless)
				{
					const coord3i src_region = { { src_rect.x1, src_rect.y1, 0 }, { src_rect.width(), src_rect.height(), 1 } };
					const coord3i dst_region = { { dst_rect.x1, dst_rect.y1, 0 }, { dst_rect.width(), dst_rect.height(), 1 } };
					gl::copy_typeless(dst, src, dst_region, src_region);
				}
				else
				{
					glCopyImageSubData(src->id(), GL_TEXTURE_2D, 0, src_rect.x1, src_rect.y1, 0,
						dst->id(), GL_TEXTURE_2D, 0, dst_rect.x1, dst_rect.y1, 0,
						src_rect.width(), src_rect.height(), 1);
				}

				return;
			}
		}

		if (xfer_info.src_is_typeless)
		{
			const auto internal_fmt = xfer_info.src_native_format_override ?
				GLenum(xfer_info.src_native_format_override) :
				get_sized_internal_format(xfer_info.src_gcm_format);

			if (static_cast<gl::texture::internal_format>(internal_fmt) != src->get_internal_format())
			{
				const u16 internal_width = static_cast<u16>(src->width() * xfer_info.src_scaling_hint);
				typeless_src = std::make_unique<texture>(GL_TEXTURE_2D, internal_width, src->height(), 1, 1, internal_fmt);
				copy_typeless(typeless_src.get(), src);

				real_src = typeless_src.get();
				src_rect.x1 = static_cast<u16>(src_rect.x1 * xfer_info.src_scaling_hint);
				src_rect.x2 = static_cast<u16>(src_rect.x2 * xfer_info.src_scaling_hint);
			}
		}

		if (xfer_info.dst_is_typeless)
		{
			const auto internal_fmt = xfer_info.dst_native_format_override ?
				GLenum(xfer_info.dst_native_format_override) :
				get_sized_internal_format(xfer_info.dst_gcm_format);

			if (static_cast<gl::texture::internal_format>(internal_fmt) != dst->get_internal_format())
			{
				const auto internal_width = static_cast<u16>(dst->width() * xfer_info.dst_scaling_hint);
				typeless_dst = std::make_unique<texture>(GL_TEXTURE_2D, internal_width, dst->height(), 1, 1, internal_fmt);
				copy_typeless(typeless_dst.get(), dst);

				real_dst = typeless_dst.get();
				dst_rect.x1 = static_cast<u16>(dst_rect.x1 * xfer_info.dst_scaling_hint);
				dst_rect.x2 = static_cast<u16>(dst_rect.x2 * xfer_info.dst_scaling_hint);
			}
		}

		verify("Incompatible source and destination format!" HERE), real_src->aspect() == real_dst->aspect();

		const bool is_depth_copy = (real_src->aspect() != image_aspect::color);
		const filter interp = (linear_interpolation && !is_depth_copy) ? filter::linear : filter::nearest;
		GLenum attachment;
		gl::buffers target;

		if (is_depth_copy)
		{
			if (real_dst->aspect() & gl::image_aspect::stencil)
			{
				attachment = GL_DEPTH_STENCIL_ATTACHMENT;
				target = gl::buffers::depth_stencil;
			}
			else
			{
				attachment = GL_DEPTH_ATTACHMENT;
				target = gl::buffers::depth;
			}
		}
		else
		{
			attachment = GL_COLOR_ATTACHMENT0;
			target = gl::buffers::color;
		}

		cmd.drv->enable(GL_FALSE, GL_SCISSOR_TEST);

		save_binding_state saved;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, blit_src.id());
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, attachment, GL_TEXTURE_2D, real_src->id(), 0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blit_dst.id());
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, real_dst->id(), 0);

		if (xfer_info.flip_horizontal)
		{
			src_rect.flip_horizontal();
		}

		if (xfer_info.flip_vertical)
		{
			src_rect.flip_vertical();
		}

		glBlitFramebuffer(src_rect.x1, src_rect.y1, src_rect.x2, src_rect.y2,
			dst_rect.x1, dst_rect.y1, dst_rect.x2, dst_rect.y2,
			static_cast<GLbitfield>(target), static_cast<GLenum>(interp));

		if (xfer_info.dst_is_typeless)
		{
			// Transfer contents from typeless dst back to original dst
			copy_typeless(dst, typeless_dst.get());
		}

		// Release the attachments explicitly (not doing so causes glitches, e.g Journey Menu)
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, attachment, GL_TEXTURE_2D, GL_NONE, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, GL_NONE, 0);
	}

	void blitter::fast_clear_image(gl::command_context& cmd, const texture* dst, const color4f& color)
	{
		save_binding_state saved;

		blit_dst.bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst->id(), 0);
		blit_dst.check();

		cmd.drv->clear_color(color);
		cmd.drv->color_maski(0, true, true, true, true);

		glClear(GL_COLOR_BUFFER_BIT);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_NONE, 0);
	}

	void blitter::fast_clear_image(gl::command_context& cmd, const texture* dst, float depth, u8 stencil)
	{
		GLenum attachment;
		GLbitfield clear_mask;

		switch (const auto fmt = dst->get_internal_format())
		{
		case texture::internal_format::depth:
		case texture::internal_format::depth16:
			clear_mask = GL_DEPTH_BUFFER_BIT;
			attachment = GL_DEPTH_ATTACHMENT;
			break;
		case texture::internal_format::depth_stencil:
		case texture::internal_format::depth24_stencil8:
		case texture::internal_format::depth32f_stencil8:
			clear_mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
			attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			break;
		default:
			fmt::throw_exception("Invalid texture passed to clear depth function, format=0x%x", static_cast<u32>(fmt));
		}

		save_binding_state saved;

		blit_dst.bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, dst->id(), 0);
		blit_dst.check();

		cmd.drv->depth_mask(GL_TRUE);
		cmd.drv->stencil_mask(0xFF);

		glClear(clear_mask);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, GL_NONE, 0);
	}
}
