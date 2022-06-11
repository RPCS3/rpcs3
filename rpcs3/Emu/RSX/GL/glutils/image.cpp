#include "stdafx.h"
#include "image.h"
#include "buffer_object.h"
#include "state_tracker.hpp"
#include "pixel_settings.hpp"

namespace gl
{
	texture::texture(GLenum target, GLuint width, GLuint height, GLuint depth, GLuint mipmaps, GLenum sized_format, rsx::format_class format_class)
	{
		glGenTextures(1, &m_id);

		// Must bind to initialize the new texture
		gl::get_command_context()->bind_texture(GL_TEMP_IMAGE_SLOT, target, m_id);

		switch (target)
		{
		default:
			fmt::throw_exception("Invalid image target 0x%X", target);
		case GL_TEXTURE_1D:
			glTexStorage1D(target, mipmaps, sized_format, width);
			height = depth = 1;
			break;
		case GL_TEXTURE_2D:
		case GL_TEXTURE_CUBE_MAP:
			glTexStorage2D(target, mipmaps, sized_format, width, height);
			depth = 1;
			break;
		case GL_TEXTURE_3D:
		case GL_TEXTURE_2D_ARRAY:
			glTexStorage3D(target, mipmaps, sized_format, width, height, depth);
			break;
		case GL_TEXTURE_BUFFER:
			break;
		}

		if (target != GL_TEXTURE_BUFFER)
		{
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmaps - 1);

			m_width = width;
			m_height = height;
			m_depth = depth;
			m_mipmaps = mipmaps;
			m_aspect_flags = image_aspect::color;

			switch (sized_format)
			{
			case GL_DEPTH_COMPONENT16:
			{
				m_pitch = width * 2;
				m_aspect_flags = image_aspect::depth;
				break;
			}
			case GL_DEPTH_COMPONENT32F:
			{
				m_pitch = width * 4;
				m_aspect_flags = image_aspect::depth;
				break;
			}
			case GL_DEPTH24_STENCIL8:
			case GL_DEPTH32F_STENCIL8:
			{
				m_pitch = width * 4;
				m_aspect_flags = image_aspect::depth | image_aspect::stencil;
				break;
			}
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			{
				m_compressed = true;
				m_pitch = utils::align(width, 4) / 2;
				break;
			}
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			{
				m_compressed = true;
				m_pitch = utils::align(width, 4);
				break;
			}
			default:
			{
				GLenum query_target = (target == GL_TEXTURE_CUBE_MAP) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : target;
				GLint r, g, b, a;

				glGetTexLevelParameteriv(query_target, 0, GL_TEXTURE_RED_SIZE, &r);
				glGetTexLevelParameteriv(query_target, 0, GL_TEXTURE_GREEN_SIZE, &g);
				glGetTexLevelParameteriv(query_target, 0, GL_TEXTURE_BLUE_SIZE, &b);
				glGetTexLevelParameteriv(query_target, 0, GL_TEXTURE_ALPHA_SIZE, &a);

				m_pitch = width * (r + g + b + a) / 8;
				break;
			}
			}

			if (!m_pitch)
			{
				fmt::throw_exception("Unhandled GL format 0x%X", sized_format);
			}

			if (format_class == RSX_FORMAT_CLASS_UNDEFINED)
			{
				if (m_aspect_flags != image_aspect::color)
				{
					rsx_log.error("Undefined format class for depth texture is not allowed");
				}
				else
				{
					format_class = RSX_FORMAT_CLASS_COLOR;
				}
			}
		}

		m_target = static_cast<texture::target>(target);
		m_internal_format = static_cast<internal_format>(sized_format);
		m_component_layout = { GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
		m_format_class = format_class;
	}

	texture::~texture()
	{
		gl::get_command_context()->unbind_texture(static_cast<GLenum>(m_target), m_id);
		glDeleteTextures(1, &m_id);
		m_id = GL_NONE;
	}

	void texture::copy_from(const void* src, texture::format format, texture::type type, int level, const coord3u region, const pixel_unpack_settings& pixel_settings)
	{
		pixel_settings.apply();

		switch (const auto target_ = static_cast<GLenum>(m_target))
		{
		case GL_TEXTURE_1D:
		{
			DSA_CALL(TextureSubImage1D, m_id, GL_TEXTURE_1D, level, region.x, region.width, static_cast<GLenum>(format), static_cast<GLenum>(type), src);
			break;
		}
		case GL_TEXTURE_2D:
		{
			DSA_CALL(TextureSubImage2D, m_id, GL_TEXTURE_2D, level, region.x, region.y, region.width, region.height, static_cast<GLenum>(format), static_cast<GLenum>(type), src);
			break;
		}
		case GL_TEXTURE_3D:
		case GL_TEXTURE_2D_ARRAY:
		{
			DSA_CALL(TextureSubImage3D, m_id, target_, level, region.x, region.y, region.z, region.width, region.height, region.depth, static_cast<GLenum>(format), static_cast<GLenum>(type), src);
			break;
		}
		case GL_TEXTURE_CUBE_MAP:
		{
			if (get_driver_caps().ARB_dsa_supported)
			{
				glTextureSubImage3D(m_id, level, region.x, region.y, region.z, region.width, region.height, region.depth, static_cast<GLenum>(format), static_cast<GLenum>(type), src);
			}
			else
			{
				rsx_log.warning("Cubemap upload via texture::copy_from is halfplemented!");
				auto ptr = static_cast<const u8*>(src);
				const auto end = std::min(6u, region.z + region.depth);
				for (unsigned face = region.z; face < end; ++face)
				{
					glTextureSubImage2DEXT(m_id, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, region.x, region.y, region.width, region.height, static_cast<GLenum>(format), static_cast<GLenum>(type), ptr);
					ptr += (region.width * region.height * 4); //TODO
				}
			}
			break;
		}
		}
	}

	void texture::copy_from(buffer& buf, u32 gl_format_type, u32 offset, u32 length)
	{
		if (get_target() != target::textureBuffer)
			fmt::throw_exception("OpenGL error: texture cannot copy from buffer");

		DSA_CALL(TextureBufferRange, m_id, GL_TEXTURE_BUFFER, gl_format_type, buf.id(), offset, length);
	}

	void texture::copy_from(buffer_view& view)
	{
		copy_from(*view.value(), view.format(), view.offset(), view.range());
	}

	void texture::copy_to(void* dst, texture::format format, texture::type type, int level, const coord3u& region, const pixel_pack_settings& pixel_settings) const
	{
		pixel_settings.apply();
		const auto& caps = get_driver_caps();

		if (!region.x && !region.y && !region.z &&
			region.width == m_width && region.height == m_height && region.depth == m_depth)
		{
			if (caps.ARB_dsa_supported)
				glGetTextureImage(m_id, level, static_cast<GLenum>(format), static_cast<GLenum>(type), s32{ smax }, dst);
			else
				glGetTextureImageEXT(m_id, static_cast<GLenum>(m_target), level, static_cast<GLenum>(format), static_cast<GLenum>(type), dst);
		}
		else if (caps.ARB_dsa_supported)
		{
			glGetTextureSubImage(m_id, level, region.x, region.y, region.z, region.width, region.height, region.depth,
				static_cast<GLenum>(format), static_cast<GLenum>(type), s32{ smax }, dst);
		}
		else
		{
			// Worst case scenario. For some reason, EXT_dsa does not have glGetTextureSubImage
			const auto target_ = static_cast<GLenum>(m_target);
			texture tmp{ target_, region.width, region.height, region.depth, 1, static_cast<GLenum>(m_internal_format) };
			glCopyImageSubData(m_id, target_, level, region.x, region.y, region.z, tmp.id(), target_, 0, 0, 0, 0,
				region.width, region.height, region.depth);

			const coord3u region2 = { {0, 0, 0}, region.size };
			tmp.copy_to(dst, format, type, 0, region2, pixel_settings);
		}
	}

	void texture_view::create(texture* data, GLenum target, GLenum sized_format, GLenum aspect_flags, const GLenum* argb_swizzle)
	{
		m_target = target;
		m_format = sized_format;
		m_image_data = data;
		m_aspect_flags = aspect_flags;

		u32 num_layers;
		switch (target)
		{
		default:
			num_layers = 1; break;
		case GL_TEXTURE_CUBE_MAP:
			num_layers = 6; break;
		case GL_TEXTURE_2D_ARRAY:
			num_layers = data->depth(); break;
		}

		glGenTextures(1, &m_id);
		glTextureView(m_id, target, data->id(), sized_format, 0, data->levels(), 0, num_layers);

		if (argb_swizzle)
		{
			component_swizzle[0] = argb_swizzle[1];
			component_swizzle[1] = argb_swizzle[2];
			component_swizzle[2] = argb_swizzle[3];
			component_swizzle[3] = argb_swizzle[0];

			gl::get_command_context()->bind_texture(GL_TEMP_IMAGE_SLOT, m_target, m_id);
			glTexParameteriv(m_target, GL_TEXTURE_SWIZZLE_RGBA, reinterpret_cast<GLint*>(component_swizzle));
		}
		else
		{
			component_swizzle[0] = GL_RED;
			component_swizzle[1] = GL_GREEN;
			component_swizzle[2] = GL_BLUE;
			component_swizzle[3] = GL_ALPHA;
		}

		if (aspect_flags & image_aspect::stencil)
		{
			constexpr u32 depth_stencil_mask = (image_aspect::depth | image_aspect::stencil);
			ensure((aspect_flags & depth_stencil_mask) != depth_stencil_mask); // "Invalid aspect mask combination"

			gl::get_command_context()->bind_texture(GL_TEMP_IMAGE_SLOT, m_target, m_id);
			glTexParameteri(m_target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
		}
	}

	texture_view::~texture_view()
	{
		gl::get_command_context()->unbind_texture(static_cast<GLenum>(m_target), m_id);
		glDeleteTextures(1, &m_id);
		m_id = GL_NONE;
	}

	void texture_view::bind(gl::command_context& cmd, GLuint layer) const
	{
		cmd->bind_texture(layer, m_target, m_id);
	}

	texture_view* viewable_image::get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap, GLenum aspect_flags)
	{
		auto found = views.equal_range(remap_encoding);
		for (auto It = found.first; It != found.second; ++It)
		{
			if (It->second->aspect() & aspect_flags)
			{
				return It->second.get();
			}
		}

		ensure(aspect() & aspect_flags);
		auto mapping = apply_swizzle_remap(get_native_component_layout(), remap);
		auto view = std::make_unique<texture_view>(this, mapping.data(), aspect_flags);
		auto result = view.get();
		views.emplace(remap_encoding, std::move(view));
		return result;
	}

	void viewable_image::set_native_component_layout(const std::array<GLenum, 4>& layout)
	{
		if (m_component_layout[0] != layout[0] ||
			m_component_layout[1] != layout[1] ||
			m_component_layout[2] != layout[2] ||
			m_component_layout[3] != layout[3])
		{
			texture::set_native_component_layout(layout);
			views.clear();
		}
	}
}
