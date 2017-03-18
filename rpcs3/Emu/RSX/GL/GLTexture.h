#include "OpenGL.h"
#include "../GCM.h"

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

namespace rsx
{
	namespace gl
	{
		class texture
		{
			u32 m_id = 0;
			u32 m_target = GL_TEXTURE_2D;

		public:
			void create();

			void init(int index, rsx::fragment_texture& tex);
			void init(int index, rsx::vertex_texture& tex);
			
			/**
			* If a format is marked as mandating expansion, any request to have the data uploaded to the GPU shall require that the pixel data
			* be decoded/expanded fully, regardless of whether the input is swizzled. This is because some formats behave differently when swizzled pixel data
			* is decoded and when data is fed directly, usually byte order is not the same. Forcing decoding/expanding fixes this but slows performance.
			*/
			static bool mandates_expansion(u32 format);
			
			/**
			* The pitch modifier changes the pitch value supplied by the rsx::texture by supplying a suitable divisor or 0 if no change is needed.
			* The modified value, if any, is then used to supply to GL the UNPACK_ROW_LENGTH for the texture data to be supplied.
			*/
			static u16  get_pitch_modifier(u32 format);
			
			void bind();
			void unbind();
			void remove();

			void set_target(u32 target) { m_target = target; }
			void set_id(u32 id) { m_id = id;  }
			u32 id() const;
		};
	}
}
