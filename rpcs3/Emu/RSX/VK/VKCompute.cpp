#include "VKCompute.h"
#include "VKHelpers.h"
#include "VKRenderPass.h"
#include "vkutils/buffer_object.h"
#include "VKPipelineCompiler.h"

#define VK_MAX_COMPUTE_TASKS 8192   // Max number of jobs per frame

namespace vk
{
	std::vector<glsl::program_input> compute_task::get_inputs()
	{
		std::vector<glsl::program_input> result;
		for (unsigned i = 0; i < ssbo_count; ++i)
		{
			const auto input = glsl::program_input::make
			(
				::glsl::glsl_compute_program,
				"ssbo" + std::to_string(i),
				glsl::program_input_type::input_type_storage_buffer,
				0,
				i
			);
			result.push_back(input);
		}

		if (use_push_constants && push_constants_size > 0)
		{
			const auto input = glsl::program_input::make
			(
				::glsl::glsl_compute_program,
				"push_constants",
				glsl::program_input_type::input_type_push_constant,
				0,
				0,
				glsl::push_constant_ref{ .offset = 0, .size = push_constants_size }
			);
			result.push_back(input);
		}

		return result;
	}

	void compute_task::create()
	{
		if (!initialized)
		{
			switch (vk::get_driver_vendor())
			{
			case vk::driver_vendor::unknown:
			case vk::driver_vendor::INTEL:
			case vk::driver_vendor::ANV:
				// Intel hw has 8 threads, but LDS allocation behavior makes optimal group size between 64 and 256
				// Based on intel's own OpenCL recommended settings
				unroll_loops = true;
				optimal_kernel_size = 1;
				optimal_group_size = 128;
				break;
			case vk::driver_vendor::LAVAPIPE:
			case vk::driver_vendor::V3DV:
			case vk::driver_vendor::PANVK:
			case vk::driver_vendor::ARM_MALI:
				// TODO: Actually bench this. Using 32 for now to match other common configurations.
			case vk::driver_vendor::DOZEN:
				// Actual optimal size depends on the D3D device. Use 32 since it should work well on both AMD and NVIDIA
			case vk::driver_vendor::NVIDIA:
			case vk::driver_vendor::NVK:
				// Warps are multiples of 32. Increasing kernel depth seems to hurt performance (Nier, Big Duck sample)
				unroll_loops = true;
				optimal_kernel_size = 1;
				optimal_group_size = 32;
				break;
			case vk::driver_vendor::AMD:
			case vk::driver_vendor::RADV:
				// Wavefronts are multiples of 64. (RDNA also supports wave32)
				unroll_loops = false;
				optimal_kernel_size = 1;
				optimal_group_size = 64;
				break;
			case vk::driver_vendor::MVK:
			case vk::driver_vendor::HONEYKRISP:
				unroll_loops = true;
				optimal_kernel_size = 1;
				optimal_group_size = 256;
				break;
			}

			const auto& gpu = vk::g_render_device->gpu();
			max_invocations_x = gpu.get_limits().maxComputeWorkGroupCount[0];

			initialized = true;
		}
	}

	void compute_task::destroy()
	{
		if (initialized)
		{
			m_shader.destroy();
			m_program.reset();
			m_param_buffer.reset();

			initialized = false;
		}
	}

	void compute_task::load_program(const vk::command_buffer& cmd)
	{
		if (!m_program)
		{
			m_shader.create(::glsl::program_domain::glsl_compute_program, m_src);
			auto handle = m_shader.compile();

			VkComputePipelineCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.module = handle,
					.pName = "main"
				},
			};

			auto compiler = vk::get_pipe_compiler();
			m_program = compiler->compile(create_info, vk::pipe_compiler::COMPILE_INLINE, {}, get_inputs());
		}

		bind_resources(cmd);
		m_program->bind(cmd, VK_PIPELINE_BIND_POINT_COMPUTE);
	}

	void compute_task::run(const vk::command_buffer& cmd, u32 invocations_x, u32 invocations_y, u32 invocations_z)
	{
		// CmdDispatch is outside renderpass scope only
		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		load_program(cmd);
		vkCmdDispatch(cmd, invocations_x, invocations_y, invocations_z);
	}

	void compute_task::run(const vk::command_buffer& cmd, u32 num_invocations)
	{
		u32 invocations_x, invocations_y;
		if (num_invocations > max_invocations_x)
		{
			// AMD hw reports an annoyingly small maximum number of invocations in the X dimension
			// Split the 1D job into 2 dimensions to accomodate this
			invocations_x = static_cast<u32>(floor(std::sqrt(num_invocations)));
			invocations_y = invocations_x;

			if (num_invocations % invocations_x) invocations_y++;
		}
		else
		{
			invocations_x = num_invocations;
			invocations_y = 1;
		}

		run(cmd, invocations_x, invocations_y, 1);
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
		create();

		kernel_size = _kernel_size? _kernel_size : optimal_kernel_size;

		m_src =
		#include "../Program/GLSLSnippets/ShuffleBytes.glsl"
		;

		const auto parameters_size = utils::align(push_constants_size, 16) / 16;
		const std::pair<std::string_view, std::string> syntax_replace[] =
		{
			{ "%loc", "0" },
			{ "%set", "set = 0"},
			{ "%ws", std::to_string(optimal_group_size) },
			{ "%ks", std::to_string(kernel_size) },
			{ "%vars", variables },
			{ "%f", function_name },
			{ "%md", method_declarations },
			{ "%ub", use_push_constants? "layout(push_constant) uniform ubo{ uvec4 params[" + std::to_string(parameters_size) + "]; };\n" : "" },
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

	void cs_shuffle_base::bind_resources(const vk::command_buffer& cmd)
	{
		set_parameters(cmd);
		m_program->bind_uniform({ *m_data, m_data_offset, m_data_length }, 0, 0);
	}

	void cs_shuffle_base::set_parameters(const vk::command_buffer& cmd)
	{
		if (!m_params.empty())
		{
			ensure(use_push_constants);
			vkCmdPushConstants(cmd, m_program->layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, m_params.size_bytes32(), m_params.data());
		}
	}

	void cs_shuffle_base::run(const vk::command_buffer& cmd, const vk::buffer* data, u32 data_length, u32 data_offset)
	{
		m_data = data;
		m_data_offset = data_offset;
		m_data_length = data_length;

		const auto num_bytes_per_invocation = optimal_group_size * kernel_size * 4;
		const auto num_bytes_to_process = rsx::align2(data_length, num_bytes_per_invocation);
		const auto num_invocations = num_bytes_to_process / num_bytes_per_invocation;

		if ((num_bytes_to_process + data_offset) > data->size())
		{
			// Technically robust buffer access should keep the driver from crashing in OOB situations
			rsx_log.error("Inadequate buffer length submitted for a compute operation."
				"Required=%d bytes, Available=%d bytes", num_bytes_to_process, data->size());
		}

		compute_task::run(cmd, num_invocations);
	}

	cs_interleave_task::cs_interleave_task()
	{
		use_push_constants = true;
		push_constants_size = 16;

		variables =
			"	uint block_length = params[0].x >> 2;\n"
			"	uint z_offset = params[0].y >> 2;\n"
			"	uint s_offset = params[0].z >> 2;\n"
			"	uint depth;\n"
			"	uint stencil;\n"
			"	uint stencil_shift;\n"
			"	uint stencil_offset;\n";
	}

	void cs_interleave_task::bind_resources(const vk::command_buffer& cmd)
	{
		set_parameters(cmd);
		m_program->bind_uniform({ *m_data, m_data_offset, m_ssbo_length }, 0, 0);
	}

	void cs_interleave_task::run(const vk::command_buffer& cmd, const vk::buffer* data, u32 data_offset, u32 data_length, u32 zeta_offset, u32 stencil_offset)
	{
		m_params = { data_length, zeta_offset - data_offset, stencil_offset - data_offset, 0 };

		ensure(stencil_offset > data_offset);
		m_ssbo_length = stencil_offset + (data_length / 4) - data_offset;
		cs_shuffle_base::run(cmd, data, data_length, data_offset);
	}

	cs_scatter_d24x8::cs_scatter_d24x8()
	{
		work_kernel =
			"		if (index >= block_length)\n"
			"			return;\n"
			"\n"
			"		value = data[index];\n"
			"		data[index + z_offset] = (value >> 8);\n"
			"		stencil_offset = (index / 4);\n"
			"		stencil_shift = (index % 4) * 8;\n"
			"		stencil = (value & 0xFF) << stencil_shift;\n"
			"		atomicOr(data[stencil_offset + s_offset], stencil);\n";

		cs_shuffle_base::build("");
	}

	cs_aggregator::cs_aggregator()
	{
		ssbo_count = 2;

		create();

		m_src =
			"#version 450\n"
			"layout(local_size_x = %ws, local_size_y = 1, local_size_z = 1) in;\n\n"

			"layout(set=0, binding=0, std430) readonly buffer ssbo0{ uint src[]; };\n"
			"layout(set=0, binding=1, std430) writeonly buffer ssbo1{ uint result; };\n\n"

			"void main()\n"
			"{\n"
			"	if (gl_GlobalInvocationID.x < src.length())\n"
			"	{\n"
			"		atomicAdd(result, src[gl_GlobalInvocationID.x]);\n"
			"	}\n"
			"}\n";

		const std::pair<std::string_view, std::string> syntax_replace[] =
		{
			{ "%ws", std::to_string(optimal_group_size) },
		};

		m_src = fmt::replace_all(m_src, syntax_replace);
	}

	void cs_aggregator::bind_resources(const vk::command_buffer& /*cmd*/)
	{
		m_program->bind_uniform({ *src, 0, block_length }, 0, 0);
		m_program->bind_uniform({ *dst, 0, 4 }, 0, 1);
	}

	void cs_aggregator::run(const vk::command_buffer& cmd, const vk::buffer* dst, const vk::buffer* src, u32 num_words)
	{
		this->dst = dst;
		this->src = src;
		word_count = num_words;
		block_length = num_words * 4;

		const u32 linear_invocations = utils::aligned_div(word_count, optimal_group_size);
		compute_task::run(cmd, linear_invocations);
	}
}
