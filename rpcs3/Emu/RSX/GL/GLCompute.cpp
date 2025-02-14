#include "GLCompute.h"
#include "GLTexture.h"
#include "Utilities/StrUtil.h"

namespace gl
{
	struct bind_image_view_safe
	{
		GLuint m_layer;
		GLenum m_target;
		GLuint m_value;
		gl::command_context& m_commands;

		bind_image_view_safe(gl::command_context& cmd, GLuint layer, gl::texture_view* value)
			: m_layer(layer), m_target(value->target()), m_commands(cmd)
		{
			m_value = cmd->get_bound_texture(layer, m_target);
			value->bind(cmd, layer);
		}

		~bind_image_view_safe()
		{
			m_commands->bind_texture(m_layer, m_target, m_value);
		}
	};

	void compute_task::initialize()
	{
		// Set up optimal kernel size
		const auto& caps = gl::get_driver_caps();
		if (caps.vendor_AMD || caps.vendor_MESA)
		{
			optimal_group_size = 64;
			unroll_loops = false;
		}
		else if (caps.vendor_NVIDIA)
		{
			optimal_group_size = 32;
		}
		else
		{
			optimal_group_size = 128;
		}

		optimal_kernel_size = 256 / optimal_group_size;

		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, reinterpret_cast<GLint*>(&max_invocations_x));

		initialized = true;
	}

	void compute_task::create()
	{
		if (!initialized)
		{
			initialize();
		}

		if (!compiled)
		{
			ensure(!m_src.empty(), "Compute shader is not initialized!");

			m_shader.create(::glsl::program_domain::glsl_compute_program, m_src);
			m_shader.compile();

			m_program.create();
			m_program.attach(m_shader);
			m_program.link();

			compiled = true;
		}
	}

	void compute_task::destroy()
	{
		if (compiled)
		{
			m_program.remove();
			m_shader.remove();

			compiled = false;
		}
	}

	void compute_task::run(gl::command_context& cmd, u32 invocations_x, u32 invocations_y)
	{
		ensure(compiled && m_program.id() != GL_NONE);
		bind_resources();

		cmd->use_program(m_program.id());
		glDispatchCompute(invocations_x, invocations_y, 1);
	}

	void compute_task::run(gl::command_context& cmd, u32 num_invocations)
	{
		u32 invocations_x, invocations_y;
		if (num_invocations <= max_invocations_x) [[likely]]
		{
			invocations_x = num_invocations;
			invocations_y = 1;
		}
		else
		{
			// Since all the invocations will run, the optimal distribution is sqrt(count)
			const u32 optimal_length = static_cast<u32>(floor(std::sqrt(num_invocations)));
			invocations_x = optimal_length;
			invocations_y = invocations_x;

			if (num_invocations % invocations_x) invocations_y++;
		}

		run(cmd, invocations_x, invocations_y);
	}

	cs_shuffle_base::cs_shuffle_base()
	{
		work_kernel =
			"		value = data[index];\n"
			"		data[index] = %f(value);\n";

		loop_advance =
			"		index++;\n";

		suffix =
			"}\n";
	}

	void cs_shuffle_base::build(const char* function_name, u32 _kernel_size)
	{
		// Initialize to allow detecting optimal settings
		initialize();

		kernel_size = _kernel_size? _kernel_size : optimal_kernel_size;

		m_src =
		#include "../Program/GLSLSnippets/ShuffleBytes.glsl"
		;

		const std::pair<std::string_view, std::string> syntax_replace[] =
		{
			{ "%set, ", ""},
			{ "%loc", std::to_string(GL_COMPUTE_BUFFER_SLOT(0)) },
			{ "%ws", std::to_string(optimal_group_size) },
			{ "%ks", std::to_string(kernel_size) },
			{ "%vars", variables },
			{ "%f", function_name },
			{ "%ub", uniforms },
			{ "%md", method_declarations },
		};

		m_src = fmt::replace_all(m_src, syntax_replace);
		work_kernel = fmt::replace_all(work_kernel, syntax_replace);

		if (kernel_size <= 1)
		{
			m_src += "	{\n" + work_kernel + "	}\n";
		}
		else if (unroll_loops)
		{
			work_kernel += loop_advance + "\n";

			m_src += std::string
			(
				"	//Unrolled loop\n"
				"	{\n"
			);

			// Assemble body with manual loop unroll to try loweing GPR usage
			for (u32 n = 0; n < kernel_size; ++n)
			{
				m_src += work_kernel;
			}

			m_src += "	}\n";
		}
		else
		{
			m_src += "	for (int loop = 0; loop < KERNEL_SIZE; ++loop)\n";
			m_src += "	{\n";
			m_src += work_kernel;
			m_src += loop_advance;
			m_src += "	}\n";
		}

		m_src += suffix;
	}

	void cs_shuffle_base::bind_resources()
	{
		m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_data_length);
	}

	void cs_shuffle_base::run(gl::command_context& cmd, const gl::buffer* data, u32 data_length, u32 data_offset)
	{
		m_data = data;
		m_data_offset = data_offset;
		m_data_length = data_length;

		const auto num_bytes_per_invocation = optimal_group_size * kernel_size * 4;
		const auto num_bytes_to_process = utils::align(data_length, num_bytes_per_invocation);
		const auto num_invocations = num_bytes_to_process / num_bytes_per_invocation;

		if ((num_bytes_to_process + data_offset) > data->size())
		{
			// Technically robust buffer access should keep the driver from crashing in OOB situations
			rsx_log.error("Inadequate buffer length submitted for a compute operation."
				"Required=%d bytes, Available=%d bytes", num_bytes_to_process, data->size());
		}

		compute_task::run(cmd, num_invocations);
	}

	template <bool SwapBytes>
	cs_shuffle_d32fx8_to_x8d24f<SwapBytes>::cs_shuffle_d32fx8_to_x8d24f()
	{
		uniforms = "uniform uint in_ptr, out_ptr;\n";

		variables =
			"	uint in_offset = in_ptr >> 2;\n"
			"	uint out_offset = out_ptr >> 2;\n"
			"	uint depth, stencil;\n";

		work_kernel =
			"		depth = data[index * 2 + in_offset];\n"
			"		stencil = data[index * 2 + (in_offset + 1)] & 0xFFu;\n"
			"		value = f32_to_d24f(depth) << 8;\n"
			"		value |= stencil;\n"
			"		data[index + out_ptr] = bswap_u32(value);\n";

		if constexpr (!SwapBytes)
		{
			work_kernel = fmt::replace_all(work_kernel, "bswap_u32(value)", "value", 1);
		}

		cs_shuffle_base::build("");
	}

	template <bool SwapBytes>
	void cs_shuffle_d32fx8_to_x8d24f<SwapBytes>::bind_resources()
	{
		m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_ssbo_length);
	}

	template <bool SwapBytes>
	void cs_shuffle_d32fx8_to_x8d24f<SwapBytes>::run(gl::command_context& cmd, const gl::buffer* data, u32 src_offset, u32 dst_offset, u32 num_texels)
	{
		u32 data_offset;
		if (src_offset > dst_offset)
		{
			data_offset = dst_offset;
			m_ssbo_length = (src_offset + num_texels * 8) - data_offset;
		}
		else
		{
			data_offset = src_offset;
			m_ssbo_length = (dst_offset + num_texels * 4) - data_offset;
		}

		m_program.uniforms["in_ptr"] = src_offset - data_offset;
		m_program.uniforms["out_ptr"] = dst_offset - data_offset;
		cs_shuffle_base::run(cmd, data, num_texels * 4, data_offset);
	}

	template struct cs_shuffle_d32fx8_to_x8d24f<true>;
	template struct cs_shuffle_d32fx8_to_x8d24f<false>;

	template <bool SwapBytes>
	cs_shuffle_x8d24f_to_d32fx8<SwapBytes>::cs_shuffle_x8d24f_to_d32fx8()
	{
		uniforms = "uniform uint texel_count, in_ptr, out_ptr;\n";

		variables =
			"	uint in_offset = in_ptr >> 2;\n"
			"	uint out_offset = out_ptr >> 2;\n"
			"	uint depth, stencil;\n";

		work_kernel =
			"		value = data[index + in_offset];\n"
			"		value = bswap_u32(value);\n"
			"		stencil = (value & 0xFFu);\n"
			"		depth = (value >> 8);\n"
			"		data[index * 2 + out_offset] = d24f_to_f32(depth);\n"
			"		data[index * 2 + (out_offset + 1)] = stencil;\n";

		if constexpr (!SwapBytes)
		{
			work_kernel = fmt::replace_all(work_kernel, "value = bswap_u32(value)", "// value = bswap_u32(value)", 1);
		}

		cs_shuffle_base::build("");
	}

	template <bool SwapBytes>
	void cs_shuffle_x8d24f_to_d32fx8<SwapBytes>::bind_resources()
	{
		m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_ssbo_length);
	}

	template <bool SwapBytes>
	void cs_shuffle_x8d24f_to_d32fx8<SwapBytes>::run(gl::command_context& cmd, const gl::buffer* data, u32 src_offset, u32 dst_offset, u32 num_texels)
	{
		u32 data_offset;
		if (src_offset > dst_offset)
		{
			data_offset = dst_offset;
			m_ssbo_length = (src_offset + num_texels * 4) - data_offset;
		}
		else
		{
			data_offset = src_offset;
			m_ssbo_length = (dst_offset + num_texels * 8) - data_offset;
		}

		m_program.uniforms["in_ptr"] = src_offset - data_offset;
		m_program.uniforms["out_ptr"] = dst_offset - data_offset;
		cs_shuffle_base::run(cmd, data, num_texels * 4, data_offset);
	}

	template struct cs_shuffle_x8d24f_to_d32fx8<true>;
	template struct cs_shuffle_x8d24f_to_d32fx8<false>;

	cs_d24x8_to_ssbo::cs_d24x8_to_ssbo()
	{
		initialize();

		const auto raw_data =
		#include "../Program/GLSLSnippets/CopyD24x8ToBuffer.glsl"
		;

		const std::pair<std::string_view, std::string> repl_list[] =
		{
			{ "%set, ", "" },
			{ "%loc", std::to_string(GL_COMPUTE_BUFFER_SLOT(0)) },
			{ "%ws", std::to_string(optimal_group_size) },
			{ "%wks", std::to_string(optimal_kernel_size) }
		};

		m_src = fmt::replace_all(raw_data, repl_list);
	}

	void cs_d24x8_to_ssbo::run(gl::command_context& cmd, gl::viewable_image* src, const gl::buffer* dst, u32 out_offset, const coordu& region, const gl::pixel_buffer_layout& layout)
	{
		const auto row_pitch = region.width;

		m_program.uniforms["swap_bytes"] = layout.swap_bytes;
		m_program.uniforms["output_pitch"] = row_pitch;
		m_program.uniforms["region_offset"] = color2i(region.x, region.y);
		m_program.uniforms["region_size"] = color2i(region.width, region.height);

		auto depth_view = src->get_view(rsx::default_remap_vector.with_encoding(GL_REMAP_IDENTITY), gl::image_aspect::depth);
		auto stencil_view = src->get_view(rsx::default_remap_vector.with_encoding(GL_REMAP_IDENTITY), gl::image_aspect::stencil);

		if (!m_sampler)
		{
			m_sampler.create();
			m_sampler.apply_defaults();
		}

		// This method is callable in sensitive code and must restore the GL state on exit
		gl::saved_sampler_state save_sampler0(GL_COMPUTE_BUFFER_SLOT(0), m_sampler);
		gl::saved_sampler_state save_sampler1(GL_COMPUTE_BUFFER_SLOT(1), m_sampler);

		gl::bind_image_view_safe save_image1(cmd, GL_COMPUTE_BUFFER_SLOT(0), depth_view);
		gl::bind_image_view_safe save_image2(cmd, GL_COMPUTE_BUFFER_SLOT(1), stencil_view);

		dst->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(2), out_offset, row_pitch * 4 * region.height);

		const int num_invocations = utils::aligned_div(region.width * region.height, optimal_kernel_size * optimal_group_size);
		compute_task::run(cmd, num_invocations);
	}

	cs_rgba8_to_ssbo::cs_rgba8_to_ssbo()
	{
		initialize();

		const auto raw_data =
		#include "../Program/GLSLSnippets/CopyRGBA8ToBuffer.glsl"
		;

		const std::pair<std::string_view, std::string> repl_list[] =
		{
			{ "%set, ", "" },
			{ "%loc", std::to_string(GL_COMPUTE_BUFFER_SLOT(0)) },
			{ "%ws", std::to_string(optimal_group_size) },
			{ "%wks", std::to_string(optimal_kernel_size) }
		};

		m_src = fmt::replace_all(raw_data, repl_list);
	}

	void cs_rgba8_to_ssbo::run(gl::command_context& cmd, gl::viewable_image* src, const gl::buffer* dst, u32 out_offset, const coordu& region, const gl::pixel_buffer_layout& layout)
	{
		const auto row_pitch = region.width;

		m_program.uniforms["swap_bytes"] = layout.swap_bytes;
		m_program.uniforms["output_pitch"] = row_pitch;
		m_program.uniforms["region_offset"] = color2i(region.x, region.y);
		m_program.uniforms["region_size"] = color2i(region.width, region.height);
		m_program.uniforms["is_bgra"] = (layout.format == static_cast<GLenum>(gl::texture::format::bgra));
		m_program.uniforms["block_width"] = static_cast<u32>(layout.size);

		auto data_view = src->get_view(rsx::default_remap_vector.with_encoding(GL_REMAP_IDENTITY), gl::image_aspect::color);

		if (!m_sampler)
		{
			m_sampler.create();
			m_sampler.apply_defaults();
		}

		// This method is callable in sensitive code and must restore the GL state on exit
		gl::saved_sampler_state save_sampler(GL_COMPUTE_BUFFER_SLOT(0), m_sampler);
		gl::bind_image_view_safe save_image(cmd, GL_COMPUTE_BUFFER_SLOT(0), data_view);

		dst->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(1), out_offset, row_pitch * 4 * region.height);

		const int num_invocations = utils::aligned_div(region.width * region.height, optimal_kernel_size * optimal_group_size);
		compute_task::run(cmd, num_invocations);
	}

	cs_ssbo_to_color_image::cs_ssbo_to_color_image()
	{
		initialize();

		const auto raw_data =
		#include "../Program/GLSLSnippets/CopyBufferToColorImage.glsl"
		;

		const std::pair<std::string_view, std::string> repl_list[] =
		{
			{ "%set, ", "" },
			{ "%image_slot", std::to_string(GL_COMPUTE_IMAGE_SLOT(0)) },
			{ "%ssbo_slot", std::to_string(GL_COMPUTE_BUFFER_SLOT(0)) },
			{ "%ws", std::to_string(optimal_group_size) },
			{ "%wks", std::to_string(optimal_kernel_size) }
		};

		m_src = fmt::replace_all(raw_data, repl_list);
	}

	void cs_ssbo_to_color_image::run(gl::command_context& cmd, const buffer* src, const texture_view* dst, const u32 src_offset, const coordu& dst_region, const pixel_buffer_layout& layout)
	{
		const u32 bpp = dst->image()->pitch() / dst->image()->width();
		const u32 row_length = utils::align(dst_region.width * bpp, std::max<int>(layout.alignment, 1)) / bpp;

		m_program.uniforms["swap_bytes"] = layout.swap_bytes;
		m_program.uniforms["src_pitch"] = row_length;
		m_program.uniforms["format"] = static_cast<GLenum>(dst->image()->get_internal_format());
		m_program.uniforms["region_offset"] = color2i(dst_region.x, dst_region.y);
		m_program.uniforms["region_size"] = color2i(dst_region.width, dst_region.height);

		src->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), src_offset, row_length * bpp * dst_region.height);
		glBindImageTexture(GL_COMPUTE_IMAGE_SLOT(0), dst->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, dst->view_format());

		const int num_invocations = utils::aligned_div(dst_region.width * dst_region.height, optimal_kernel_size * optimal_group_size);
		compute_task::run(cmd, num_invocations);
	}

	void cs_ssbo_to_color_image::run(gl::command_context& cmd, const buffer* src, texture* dst, const u32 src_offset, const coordu& dst_region, const pixel_buffer_layout& layout)
	{
		gl::nil_texture_view view(dst);
		run(cmd, src, &view, src_offset, dst_region, layout);
	}
}
