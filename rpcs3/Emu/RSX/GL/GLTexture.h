#pragma once

#include "OpenGL.h"
#include "../GCM.h"
#include "../Common/TextureUtils.h"
#include "GLHelpers.h"

namespace rsx
{
	class vertex_texture;
	class fragment_texture;
}

namespace gl
{
	struct pixel_buffer_layout
	{
		GLenum format;
		GLenum type;
		u8     size;
		bool   swap_bytes;
	};

	GLenum get_target(rsx::texture_dimension_extended type);
	GLenum get_sized_internal_format(u32 texture_format);
	std::tuple<GLenum, GLenum> get_format_type(u32 texture_format);
	pixel_buffer_layout get_format_type(texture::internal_format format);
	GLenum wrap_mode(rsx::texture_wrap_mode wrap);
	float max_aniso(rsx::texture_max_anisotropy aniso);
	std::array<GLenum, 4> get_swizzle_remap(u32 texture_format);

	viewable_image* create_texture(u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, rsx::texture_dimension_extended type);

	bool formats_are_bitcast_compatible(GLenum format1, GLenum format2);
	void copy_typeless(texture* dst, const texture* src, const coord3u& dst_region, const coord3u& src_region);
	void copy_typeless(texture* dst, const texture* src);

	/**
	 * is_swizzled - determines whether input bytes are in morton order
	 * subresources_layout - descriptor of the mipmap levels in memory
	 * decoded_remap - two vectors, first one contains index to read, e.g if v[0] = 1 then component 0[A] in the texture should read as component 1[R]
	 * - layout of vector is in A-R-G-B
	 * - second vector contains overrides to force the value to either 0 or 1 instead of reading from texture
	 * static_state - set up the texture without consideration for sampler state (useful for vertex textures which have no real sampler state on RSX)
	 */
	void upload_texture(GLuint id, u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, bool is_swizzled, rsx::texture_dimension_extended type,
		const std::vector<rsx::subresource_layout>& subresources_layout);

	class sampler_state
	{
		GLuint samplerHandle = 0;
		std::unordered_map<GLenum, GLuint> m_propertiesi;
		std::unordered_map<GLenum, GLfloat> m_propertiesf;

	public:

		void create()
		{
			glGenSamplers(1, &samplerHandle);
		}

		void remove()
		{
			glDeleteSamplers(1, &samplerHandle);
		}

		void bind(int index) const
		{
			glBindSampler(index, samplerHandle);
		}

		void set_parameteri(GLenum pname, GLuint value)
		{
			auto prop = m_propertiesi.find(pname);
			if (prop != m_propertiesi.end() &&
				prop->second == value)
			{
				return;
			}

			m_propertiesi[pname] = value;
			glSamplerParameteri(samplerHandle, pname, value);
		}

		void set_parameterf(GLenum pname, GLfloat value)
		{
			auto prop = m_propertiesf.find(pname);
			if (prop != m_propertiesf.end() &&
				prop->second == value)
			{
				return;
			}

			m_propertiesf[pname] = value;
			glSamplerParameterf(samplerHandle, pname, value);
		}

		GLuint get_parameteri(GLenum pname)
		{
			auto prop = m_propertiesi.find(pname);
			return (prop == m_propertiesi.end()) ? 0 : prop->second;
		}

		GLfloat get_parameterf(GLenum pname)
		{
			auto prop = m_propertiesf.find(pname);
			return (prop == m_propertiesf.end()) ? 0 : prop->second;
		}

		void apply(const rsx::fragment_texture& tex, const rsx::sampled_image_descriptor_base* sampled_image);
		void apply(const rsx::vertex_texture& tex, const rsx::sampled_image_descriptor_base* sampled_image);

		void apply_defaults(GLenum default_filter = GL_NEAREST);
	};

	extern buffer g_typeless_transfer_buffer;
}
