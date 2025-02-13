#include "stdafx.h"

#include "../../glutils/fbo.h"
#include "../fsr_pass.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#define A_CPU 1
#include "3rdparty/GPUOpen/include/ffx_a.h"
#include "3rdparty/GPUOpen/include/ffx_fsr1.h"
#undef A_CPU

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace gl
{
	namespace FidelityFX
	{
		fsr_pass::fsr_pass(const std::string& config_definitions, u32 push_constants_size)
		{
			// Just use AMD-provided source with minimal modification
			const char* shader_core =
				#include "Emu/RSX/Program/Upscalers/FSR1/fsr_ubershader.glsl"
				;

			// Replacements
			const char* ffx_a_contents =
				#include "Emu/RSX/Program/Upscalers/FSR1/fsr_ffx_a_flattened.inc"
				;

			const char* ffx_fsr_contents =
				#include "Emu/RSX/Program/Upscalers/FSR1/fsr_ffx_fsr1_flattened.inc"
				;

			const std::pair<std::string_view, std::string> replacement_table[] =
			{
				{ "%FFX_DEFINITIONS%", config_definitions },
				{ "%FFX_A_IMPORT%", ffx_a_contents },
				{ "%FFX_FSR_IMPORT%", ffx_fsr_contents },
				{ "layout(set=0,", "layout(" },
				{ "%push_block%", fmt::format("binding=%d, std140", GL_COMPUTE_BUFFER_SLOT(0)) }
			};

			m_src = shader_core;
			m_src = fmt::replace_all(m_src, replacement_table);

			// Fill with 0 to avoid sending incomplete/unused variables to the GPU
			m_constants_buf.resize(utils::rounded_div(push_constants_size, 4), 0);

			create();

			m_sampler.create();
			m_sampler.apply_defaults(GL_LINEAR);
		}

		fsr_pass::~fsr_pass()
		{
			m_ubo.remove();
			m_sampler.remove();
		}

		void fsr_pass::bind_resources()
		{
			// Bind relevant stuff
			const u32 push_buffer_size = ::size32(m_constants_buf) * sizeof(m_constants_buf[0]);

			if (!m_ubo)
			{
				ensure(compiled);
				m_ubo.create(gl::buffer::target::uniform, push_buffer_size, nullptr, gl::buffer::memory_type::local, gl::buffer::usage::dynamic_update);

				// Statically bind the image sources
				m_program.uniforms["InputTexture"] = GL_TEMP_IMAGE_SLOT(0);
				m_program.uniforms["OutputTexture"] = GL_COMPUTE_IMAGE_SLOT(0);
			}

			m_ubo.sub_data(0, push_buffer_size, m_constants_buf.data());
			m_ubo.bind_range(GL_COMPUTE_BUFFER_SLOT(0), 0, push_buffer_size);
		}

		void fsr_pass::run(gl::command_context& cmd, gl::texture* src, gl::texture* dst, const size2u& input_size, const size2u& output_size)
		{
			m_input_image = src;
			m_output_image = dst;
			m_input_size = input_size;
			m_output_size = output_size;

			configure();

			saved_sampler_state saved(GL_TEMP_IMAGE_SLOT(0), m_sampler);
			cmd->bind_texture(GL_TEMP_IMAGE_SLOT(0), GL_TEXTURE_2D, src->id());

			glBindImageTexture(GL_COMPUTE_IMAGE_SLOT(0), dst->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

			constexpr auto wg_size = 16;
			const auto invocations_x = utils::aligned_div(output_size.width, wg_size);
			const auto invocations_y = utils::aligned_div(output_size.height, wg_size);

			ensure(invocations_x == (output_size.width + (wg_size - 1)) / wg_size);
			ensure(invocations_y == (output_size.height + (wg_size - 1)) / wg_size);
			compute_task::run(cmd, invocations_x, invocations_y);
		}

		easu_pass::easu_pass()
			: fsr_pass(
				"#define SAMPLE_EASU 1\n"
				"#define SAMPLE_RCAS 0\n"
				"#define SAMPLE_BILINEAR 0\n"
				"#define SAMPLE_SLOW_FALLBACK 1",
				80 // 5*VEC4
			)
		{}

		void easu_pass::configure()
		{
			// NOTE: Configuration vector 4 is unused as we do not support HDR natively
			auto con0 = &m_constants_buf[0];
			auto con1 = &m_constants_buf[4];
			auto con2 = &m_constants_buf[8];
			auto con3 = &m_constants_buf[12];

			FsrEasuCon(con0, con1, con2, con3,
				static_cast<f32>(m_input_size.width), static_cast<f32>(m_input_size.height),     // Incoming viewport size to upscale (actual size)
				static_cast<f32>(m_input_image->width()), static_cast<f32>(m_input_image->height()),     // Size of the raw image to upscale (in case viewport does not cover it all)
				static_cast<f32>(m_output_size.width), static_cast<f32>(m_output_size.height));  // Size of output viewport (target size)
		}

		rcas_pass::rcas_pass()
			: fsr_pass(
				"#define SAMPLE_RCAS 1\n"
				"#define SAMPLE_EASU 0\n"
				"#define SAMPLE_BILINEAR 0\n"
				"#define SAMPLE_SLOW_FALLBACK 1",
				32 // 2*VEC4
			)
		{}

		void rcas_pass::configure()
		{
			// 0 is actually the sharpest with 2 being the chosen limit. Each progressive unit 'halves' the sharpening intensity.
			auto cas_attenuation = 2.f - (g_cfg.video.vk.rcas_sharpening_intensity / 50.f);
			FsrRcasCon(&m_constants_buf[0], cas_attenuation);
		}
	}

	fsr_upscale_pass::~fsr_upscale_pass()
	{
		dispose_images();
	}

	void fsr_upscale_pass::dispose_images()
	{
		m_output_left.reset();
		m_output_right.reset();
		m_intermediate_data.reset();
		m_flip_fbo.remove();
	}

	void fsr_upscale_pass::initialize_image(u32 output_w, u32 output_h, rsx::flags32_t mode)
	{
		dispose_images();

		const auto initialize_image_impl = [&]()
		{
			return std::make_unique<gl::viewable_image>(
				GL_TEXTURE_2D,
				output_w, output_h, 1, 1, 1,
				GL_RGBA8, RSX_FORMAT_CLASS_COLOR);
		};

		if (mode & UPSCALE_LEFT_VIEW)
		{
			m_output_left = initialize_image_impl();
		}

		if (mode & UPSCALE_RIGHT_VIEW)
		{
			m_output_right = initialize_image_impl();
		}

		m_intermediate_data = initialize_image_impl();
	}

	gl::texture* fsr_upscale_pass::scale_output(
		gl::command_context& cmd,
		gl::texture* src,
		const areai& src_region,
		const areai& dst_region,
		gl::flags32_t mode)
	{
		size2u input_size, output_size;
		input_size.width = src_region.width();
		input_size.height = src_region.height();
		output_size.width = dst_region.width();
		output_size.height = dst_region.height();

		auto src_image = src;
		auto input_region = src_region;

		if (input_size.width < output_size.width && input_size.height < output_size.height)
		{
			// Cannot upscale both LEFT and RIGHT images at the same time.
			// Default maps to LEFT for simplicity
			ensure((mode & (UPSCALE_LEFT_VIEW | UPSCALE_RIGHT_VIEW)) != (UPSCALE_LEFT_VIEW | UPSCALE_RIGHT_VIEW));

			auto& m_output_data = (mode & UPSCALE_LEFT_VIEW) ? m_output_left : m_output_right;
			if (!m_output_data || m_output_data->width() != output_size.width || m_output_data->height() != output_size.height)
			{
				initialize_image(output_size.width, output_size.height, mode);
			}

			if (m_output_data)
			{
				// Execute the pass here
				auto cs_easu_task = gl::get_compute_task<gl::FidelityFX::easu_pass>();
				auto cs_rcas_task = gl::get_compute_task<gl::FidelityFX::rcas_pass>();

				// EASU
				cs_easu_task->run(cmd, src, m_intermediate_data.get(), input_size, output_size);

				// RCAS
				cs_rcas_task->run(cmd, m_intermediate_data.get(), m_output_data.get(), input_size, output_size);

				// Swap input for FSR target
				src_image = m_output_data.get();

				input_region.x1 = 0;
				input_region.x2 = src_image->width();
				input_region.y1 = 0;
				input_region.y2 = src_image->height();
			}
		}

		if (mode & UPSCALE_AND_COMMIT)
		{
			m_flip_fbo.recreate();
			m_flip_fbo.bind();
			m_flip_fbo.color = src_image->id();
			m_flip_fbo.read_buffer(m_flip_fbo.color);
			m_flip_fbo.draw_buffer(m_flip_fbo.color);
			m_flip_fbo.blit(gl::screen, input_region, dst_region, gl::buffers::color, gl::filter::linear);
			return 0;
		}

		return src_image;
	}
}
