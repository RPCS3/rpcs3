#include "OpenGL.h"
#include "../GCM.h"
#include "../Common/TextureUtils.h"

namespace rsx
{
	class vertex_texture;
	class fragment_texture;
}

namespace gl
{
	GLenum get_sized_internal_format(u32 gcm_format);
	std::tuple<GLenum, GLenum> get_format_type(u32 texture_format);
	GLenum wrap_mode(rsx::texture_wrap_mode wrap);
	float max_aniso(rsx::texture_max_anisotropy aniso);

	GLuint create_texture(u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, rsx::texture_dimension_extended type);

	void upload_texture(const GLuint id, const u32 texaddr, const u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, bool is_swizzled, rsx::texture_dimension_extended type,
		std::vector<rsx_subresource_layout>& subresources_layout, std::pair<std::array<u8, 4>, std::array<u8, 4>>& decoded_remap, bool static_state);

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

		void apply(rsx::fragment_texture& tex);
	};
}
