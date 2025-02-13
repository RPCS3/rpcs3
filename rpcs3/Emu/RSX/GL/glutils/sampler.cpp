#include "stdafx.h"
#include "sampler.h"

#include "Emu/RSX/gcm_enums.h"
#include "Emu/RSX/Common/TextureUtils.h"

//GLenum wrap_mode(rsx::texture_wrap_mode wrap);
//float max_aniso(rsx::texture_max_anisotropy aniso);

namespace gl
{
	GLenum wrap_mode(rsx::texture_wrap_mode wrap)
	{
		switch (wrap)
		{
		case rsx::texture_wrap_mode::wrap: return GL_REPEAT;
		case rsx::texture_wrap_mode::mirror: return GL_MIRRORED_REPEAT;
		case rsx::texture_wrap_mode::clamp_to_edge: return GL_CLAMP_TO_EDGE;
		case rsx::texture_wrap_mode::border: return GL_CLAMP_TO_BORDER;
		case rsx::texture_wrap_mode::clamp: return GL_CLAMP_TO_EDGE;
		case rsx::texture_wrap_mode::mirror_once_clamp_to_edge: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
		case rsx::texture_wrap_mode::mirror_once_border: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
		case rsx::texture_wrap_mode::mirror_once_clamp: return GL_MIRROR_CLAMP_EXT;
		}

		rsx_log.error("Texture wrap error: bad wrap (%d)", static_cast<u32>(wrap));
		return GL_REPEAT;
	}

	float max_aniso(rsx::texture_max_anisotropy aniso)
	{
		switch (aniso)
		{
		case rsx::texture_max_anisotropy::x1: return 1.0f;
		case rsx::texture_max_anisotropy::x2: return 2.0f;
		case rsx::texture_max_anisotropy::x4: return 4.0f;
		case rsx::texture_max_anisotropy::x6: return 6.0f;
		case rsx::texture_max_anisotropy::x8: return 8.0f;
		case rsx::texture_max_anisotropy::x10: return 10.0f;
		case rsx::texture_max_anisotropy::x12: return 12.0f;
		case rsx::texture_max_anisotropy::x16: return 16.0f;
		}

		rsx_log.error("Texture anisotropy error: bad max aniso (%d)", static_cast<u32>(aniso));
		return 1.0f;
	}

	int tex_min_filter(rsx::texture_minify_filter min_filter)
	{
		switch (min_filter)
		{
		case rsx::texture_minify_filter::nearest: return GL_NEAREST;
		case rsx::texture_minify_filter::linear: return GL_LINEAR;
		case rsx::texture_minify_filter::nearest_nearest: return GL_NEAREST_MIPMAP_NEAREST;
		case rsx::texture_minify_filter::linear_nearest: return GL_LINEAR_MIPMAP_NEAREST;
		case rsx::texture_minify_filter::nearest_linear: return GL_NEAREST_MIPMAP_LINEAR;
		case rsx::texture_minify_filter::linear_linear: return GL_LINEAR_MIPMAP_LINEAR;
		case rsx::texture_minify_filter::convolution_min: return GL_LINEAR_MIPMAP_LINEAR;
		}
		fmt::throw_exception("Unknown min filter");
	}

	int tex_mag_filter(rsx::texture_magnify_filter mag_filter)
	{
		switch (mag_filter)
		{
		case rsx::texture_magnify_filter::nearest: return GL_NEAREST;
		case rsx::texture_magnify_filter::linear: return GL_LINEAR;
		case rsx::texture_magnify_filter::convolution_mag: return GL_LINEAR;
		}
		fmt::throw_exception("Unknown mag filter");
	}

	// Apply sampler state settings
	void sampler_state::apply(const rsx::fragment_texture& tex, const rsx::sampled_image_descriptor_base* sampled_image)
	{
		set_parameteri(GL_TEXTURE_WRAP_S, wrap_mode(tex.wrap_s()));
		set_parameteri(GL_TEXTURE_WRAP_T, wrap_mode(tex.wrap_t()));
		set_parameteri(GL_TEXTURE_WRAP_R, wrap_mode(tex.wrap_r()));

		if (rsx::is_border_clamped_texture(tex))
		{
			// NOTE: In OpenGL, the border texels are processed by the pipeline and will be swizzled by the texture view.
			// Therefore, we pass the raw value here, and the texture view will handle the rest for us.
			const auto encoded_color = tex.border_color();
			if (get_parameteri(GL_TEXTURE_BORDER_COLOR) != encoded_color)
			{
				m_propertiesi[GL_TEXTURE_BORDER_COLOR] = encoded_color;
				const auto border_color = rsx::decode_border_color(encoded_color);
				glSamplerParameterfv(sampler_handle, GL_TEXTURE_BORDER_COLOR, border_color.rgba);
			}
		}

		if (sampled_image->upload_context != rsx::texture_upload_context::shader_read ||
			tex.get_exact_mipmap_count() == 1)
		{
			GLint min_filter = tex_min_filter(tex.min_filter());

			if (min_filter != GL_LINEAR && min_filter != GL_NEAREST)
			{
				switch (min_filter)
				{
				case GL_NEAREST_MIPMAP_NEAREST:
				case GL_NEAREST_MIPMAP_LINEAR:
					min_filter = GL_NEAREST; break;
				case GL_LINEAR_MIPMAP_NEAREST:
				case GL_LINEAR_MIPMAP_LINEAR:
					min_filter = GL_LINEAR; break;
				default:
					rsx_log.error("No mipmap fallback defined for rsx_min_filter = 0x%X", static_cast<u32>(tex.min_filter()));
					min_filter = GL_NEAREST;
				}
			}

			set_parameteri(GL_TEXTURE_MIN_FILTER, min_filter);
			set_parameterf(GL_TEXTURE_LOD_BIAS, 0.f);
			set_parameterf(GL_TEXTURE_MIN_LOD, -1000.f);
			set_parameterf(GL_TEXTURE_MAX_LOD, 1000.f);
		}
		else
		{
			set_parameteri(GL_TEXTURE_MIN_FILTER, tex_min_filter(tex.min_filter()));
			set_parameterf(GL_TEXTURE_LOD_BIAS, tex.bias());
			set_parameterf(GL_TEXTURE_MIN_LOD, tex.min_lod());
			set_parameterf(GL_TEXTURE_MAX_LOD, tex.max_lod());
		}

		const f32 af_level = max_aniso(tex.max_aniso());
		set_parameterf(GL_TEXTURE_MAX_ANISOTROPY_EXT, af_level);
		set_parameteri(GL_TEXTURE_MAG_FILTER, tex_mag_filter(tex.mag_filter()));

		const u32 texture_format = tex.format() & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
		if (texture_format == CELL_GCM_TEXTURE_DEPTH16 || texture_format == CELL_GCM_TEXTURE_DEPTH24_D8 ||
			texture_format == CELL_GCM_TEXTURE_DEPTH16_FLOAT || texture_format == CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT)
		{
			//NOTE: The stored texture function is reversed wrt the textureProj compare function
			GLenum compare_mode = static_cast<GLenum>(tex.zfunc()) | GL_NEVER;

			switch (compare_mode)
			{
			case GL_GREATER: compare_mode = GL_LESS; break;
			case GL_GEQUAL: compare_mode = GL_LEQUAL; break;
			case GL_LESS: compare_mode = GL_GREATER; break;
			case GL_LEQUAL: compare_mode = GL_GEQUAL; break;
			}

			set_parameteri(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			set_parameteri(GL_TEXTURE_COMPARE_FUNC, compare_mode);
		}
		else
			set_parameteri(GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}

	void sampler_state::apply(const rsx::vertex_texture& tex, const rsx::sampled_image_descriptor_base* /*sampled_image*/)
	{
		if (rsx::is_border_clamped_texture(tex))
		{
			// NOTE: In OpenGL, the border texels are processed by the pipeline and will be swizzled by the texture view.
			// Therefore, we pass the raw value here, and the texture view will handle the rest for us.
			const auto encoded_color = tex.border_color();
			if (get_parameteri(GL_TEXTURE_BORDER_COLOR) != encoded_color)
			{
				m_propertiesi[GL_TEXTURE_BORDER_COLOR] = encoded_color;
				const auto border_color = rsx::decode_border_color(encoded_color);
				glSamplerParameterfv(sampler_handle, GL_TEXTURE_BORDER_COLOR, border_color.rgba);
			}
		}

		set_parameteri(GL_TEXTURE_WRAP_S, wrap_mode(tex.wrap_s()));
		set_parameteri(GL_TEXTURE_WRAP_T, wrap_mode(tex.wrap_t()));
		set_parameteri(GL_TEXTURE_WRAP_R, wrap_mode(tex.wrap_r()));
		set_parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		set_parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		set_parameterf(GL_TEXTURE_LOD_BIAS, tex.bias());
		set_parameterf(GL_TEXTURE_MIN_LOD, tex.min_lod());
		set_parameterf(GL_TEXTURE_MAX_LOD, tex.max_lod());
		set_parameteri(GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}

	void sampler_state::apply_defaults(GLenum default_filter)
	{
		set_parameteri(GL_TEXTURE_WRAP_S, GL_REPEAT);
		set_parameteri(GL_TEXTURE_WRAP_T, GL_REPEAT);
		set_parameteri(GL_TEXTURE_WRAP_R, GL_REPEAT);
		set_parameteri(GL_TEXTURE_MIN_FILTER, default_filter);
		set_parameteri(GL_TEXTURE_MAG_FILTER, default_filter);
		set_parameterf(GL_TEXTURE_LOD_BIAS, 0.f);
		set_parameteri(GL_TEXTURE_MIN_LOD, 0);
		set_parameteri(GL_TEXTURE_MAX_LOD, 0);
		set_parameteri(GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}
}
