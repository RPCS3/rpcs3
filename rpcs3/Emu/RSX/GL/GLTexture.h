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
	GLenum get_target(rsx::texture_dimension_extended type);
	GLenum get_sized_internal_format(u32 gcm_format);
	std::tuple<GLenum, GLenum> get_format_type(u32 texture_format);
	std::tuple<GLenum, GLenum, bool> get_format_type(texture::internal_format format);
	GLenum wrap_mode(rsx::texture_wrap_mode wrap);
	float max_aniso(rsx::texture_max_anisotropy aniso);
	std::array<GLenum, 4> get_swizzle_remap(u32 texture_format);

	texture* create_texture(u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, rsx::texture_dimension_extended type, rsx::texture_colorspace colorspace);

	void copy_typeless(texture* dst, const texture* src);
	/**
	 * is_swizzled - determines whether input bytes are in morton order
	 * subresources_layout - descriptor of the mipmap levels in memory
	 * decoded_remap - two vectors, first one contains index to read, e.g if v[0] = 1 then component 0[A] in the texture should read as component 1[R]
	 * - layout of vector is in A-R-G-B
	 * - second vector contains overrides to force the value to either 0 or 1 instead of reading from texture
	 * static_state - set up the texture without consideration for sampler state (useful for vertex textures which have no real sampler state on RSX)
	 */
	void upload_texture(GLuint id, u32 texaddr, u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, bool is_swizzled, rsx::texture_dimension_extended type,
		const std::vector<rsx_subresource_layout>& subresources_layout, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& decoded_remap, bool static_state,
		rsx::texture_colorspace colorspace);

	std::array<GLenum, 4> apply_swizzle_remap(const std::array<GLenum, 4>& swizzle_remap, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& decoded_remap);

	class sampler_state
	{
		GLuint samplerHandle = 0;

	public:

		void create()
		{
			glGenSamplers(1, &samplerHandle);
		}

		void remove()
		{
			glDeleteSamplers(1, &samplerHandle);
		}

		void bind(int index)
		{
			glBindSampler(index, samplerHandle);
		}

		void apply(rsx::fragment_texture& tex, const rsx::sampled_image_descriptor_base* sampled_image);
	};
}
