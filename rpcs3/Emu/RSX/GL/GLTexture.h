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

	struct image_memory_requirements
	{
		u64 image_size_in_texels;
		u64 image_size_in_bytes;
		u64 memory_required;
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

	void* copy_image_to_buffer(const pixel_buffer_layout& pack_info, const gl::texture* src, gl::buffer* dst,
		const int src_level, const coord3u& src_region, image_memory_requirements* mem_info);

	void copy_buffer_to_image(const pixel_buffer_layout& unpack_info, gl::buffer* src, gl::texture* dst,
		const void* src_offset, const int dst_level, const coord3u& dst_region, image_memory_requirements* mem_info);

	void upload_texture(texture* dst, u32 gcm_format, bool is_swizzled, const std::vector<rsx::subresource_layout>& subresources_layout);

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
