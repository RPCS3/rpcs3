#include "VKCompute.h"
#include "VKHelpers.h"
#include "VKRenderPass.h"
#include "vkutils/buffer_object.h"

#define VK_MAX_COMPUTE_TASKS 4096   // Max number of jobs per frame

namespace vk
{
	std::vector<std::pair<VkDescriptorType, u8>> compute_task::get_descriptor_layout()
	{
		std::vector<std::pair<VkDescriptorType, u8>> result;
		result.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, ssbo_count);
		return result;
	}

	void compute_task::init_descriptors()
	{
		std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		const auto layout = get_descriptor_layout();
		for (const auto &e : layout)
		{
			descriptor_pool_sizes.push_back({e.first, u32(VK_MAX_COMPUTE_TASKS * e.second)});

			for (unsigned n = 0; n < e.second; ++n)
			{
				bindings.push_back
				({
					u32(bindings.size()),
					e.first,
					1,
					VK_SHADER_STAGE_COMPUTE_BIT,
					nullptr
				});
			}
		}

		// Reserve descriptor pools
		m_descriptor_pool.create(*g_render_device, descriptor_pool_sizes.data(), ::size32(descriptor_pool_sizes), VK_MAX_COMPUTE_TASKS, 3);

		VkDescriptorSetLayoutCreateInfo infos = {};
		infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		infos.pBindings = bindings.data();
		infos.bindingCount = ::size32(bindings);

		CHECK_RESULT(vkCreateDescriptorSetLayout(*g_render_device, &infos, nullptr, &m_descriptor_layout));

		VkPipelineLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount = 1;
		layout_info.pSetLayouts = &m_descriptor_layout;

		VkPushConstantRange push_constants{};
		if (use_push_constants)
		{
			push_constants.size = push_constants_size;
			push_constants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			layout_info.pushConstantRangeCount = 1;
			layout_info.pPushConstantRanges = &push_constants;
		}

		CHECK_RESULT(vkCreatePipelineLayout(*g_render_device, &layout_info, nullptr, &m_pipeline_layout));
	}

	void compute_task::create()
	{
		if (!initialized)
		{
			init_descriptors();

			switch (vk::get_driver_vendor())
			{
			case vk::driver_vendor::unknown:
			case vk::driver_vendor::INTEL:
				// Intel hw has 8 threads, but LDS allocation behavior makes optimal group size between 64 and 256
				// Based on intel's own OpenCL recommended settings
				unroll_loops = true;
				optimal_kernel_size = 1;
				optimal_group_size = 128;
				break;
			case vk::driver_vendor::NVIDIA:
				// Warps are multiples of 32. Increasing kernel depth seems to hurt performance (Nier, Big Duck sample)
				unroll_loops = true;
				optimal_group_size = 32;
				optimal_kernel_size = 1;
				break;
			case vk::driver_vendor::AMD:
			case vk::driver_vendor::RADV:
				// Wavefronts are multiples of 64
				unroll_loops = false;
				optimal_kernel_size = 1;
				optimal_group_size = 64;
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

			vkDestroyDescriptorSetLayout(*g_render_device, m_descriptor_layout, nullptr);
			vkDestroyPipelineLayout(*g_render_device, m_pipeline_layout, nullptr);
			m_descriptor_pool.destroy();

			initialized = false;
		}
	}

	void compute_task::free_resources()
	{
		if (m_used_descriptors == 0)
			return;

		m_descriptor_pool.reset(0);
		m_used_descriptors = 0;
	}

	void compute_task::load_program(VkCommandBuffer cmd)
	{
		if (!m_program)
		{
			m_shader.create(::glsl::program_domain::glsl_compute_program, m_src);
			auto handle = m_shader.compile();

			VkPipelineShaderStageCreateInfo shader_stage{};
			shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shader_stage.module = handle;
			shader_stage.pName = "main";

			VkComputePipelineCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			info.stage = shader_stage;
			info.layout = m_pipeline_layout;
			info.basePipelineIndex = -1;
			info.basePipelineHandle = VK_NULL_HANDLE;

			auto compiler = vk::get_pipe_compiler();
			m_program = compiler->compile(info, m_pipeline_layout, vk::pipe_compiler::COMPILE_INLINE);
			declare_inputs();
		}

		ensure(m_used_descriptors < VK_MAX_COMPUTE_TASKS);

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.descriptorPool = m_descriptor_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &m_descriptor_layout;
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		CHECK_RESULT(vkAllocateDescriptorSets(*g_render_device, &alloc_info, &m_descriptor_set));
		m_used_descriptors++;

		bind_resources();

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_program->pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);
	}

	void compute_task::run(VkCommandBuffer cmd, u32 invocations_x, u32 invocations_y, u32 invocations_z)
	{
		// CmdDispatch is outside renderpass scope only
		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		load_program(cmd);
		vkCmdDispatch(cmd, invocations_x, invocations_y, invocations_z);
	}

	void compute_task::run(VkCommandBuffer cmd, u32 num_invocations)
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
			"#version 430\n"
			"layout(local_size_x=%ws, local_size_y=1, local_size_z=1) in;\n"
			"layout(std430, set=0, binding=0) buffer ssbo{ uint data[]; };\n"
			"%ub"
			"\n"
			"#define KERNEL_SIZE %ks\n"
			"\n"
			"// Generic swap routines\n"
			"#define bswap_u16(bits)     (bits & 0xFF) << 8 | (bits & 0xFF00) >> 8 | (bits & 0xFF0000) << 8 | (bits & 0xFF000000) >> 8\n"
			"#define bswap_u32(bits)     (bits & 0xFF) << 24 | (bits & 0xFF00) << 8 | (bits & 0xFF0000) >> 8 | (bits & 0xFF000000) >> 24\n"
			"#define bswap_u16_u32(bits) (bits & 0xFFFF) << 16 | (bits & 0xFFFF0000) >> 16\n"
			"\n"
			"// Depth format conversions\n"
			"#define d24_to_f32(bits)             floatBitsToUint(float(bits) / 16777215.f)\n"
			"#define f32_to_d24(bits)             uint(uintBitsToFloat(bits) * 16777215.f)\n"
			"#define d24f_to_f32(bits)            (bits << 7)\n"
			"#define f32_to_d24f(bits)            (bits >> 7)\n"
			"#define d24x8_to_f32(bits)           d24_to_f32(bits >> 8)\n"
			"#define d24x8_to_d24x8_swapped(bits) (bits & 0xFF00) | (bits & 0xFF0000) >> 16 | (bits & 0xFF) << 16\n"
			"#define f32_to_d24x8_swapped(bits)   d24x8_to_d24x8_swapped(f32_to_d24(bits))\n"
			"\n"
			"%md"
			"void main()\n"
			"{\n"
			"	uint invocations_x = (gl_NumWorkGroups.x * gl_WorkGroupSize.x);"
			"	uint invocation_id = (gl_GlobalInvocationID.y * invocations_x) + gl_GlobalInvocationID.x;\n"
			"	uint index = invocation_id * KERNEL_SIZE;\n"
			"	uint value;\n"
			"%vars"
			"\n";

		const auto parameters_size = utils::align(push_constants_size, 16) / 16;
		const std::pair<std::string, std::string> syntax_replace[] =
		{
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

	void cs_shuffle_base::bind_resources()
	{
		m_program->bind_buffer({ m_data->value, m_data_offset, m_data_length }, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);
	}

	void cs_shuffle_base::set_parameters(VkCommandBuffer cmd, const u32* params, u8 count)
	{
		ensure(use_push_constants);
		vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, count * 4, params);
	}

	void cs_shuffle_base::run(VkCommandBuffer cmd, const vk::buffer* data, u32 data_length, u32 data_offset)
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

	void cs_interleave_task::bind_resources()
	{
		m_program->bind_buffer({ m_data->value, m_data_offset, m_ssbo_length }, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);
	}

	void cs_interleave_task::run(VkCommandBuffer cmd, const vk::buffer* data, u32 data_offset, u32 data_length, u32 zeta_offset, u32 stencil_offset)
	{
		u32 parameters[4] = { data_length, zeta_offset - data_offset, stencil_offset - data_offset, 0 };
		set_parameters(cmd, parameters, 4);

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

		const std::pair<std::string, std::string> syntax_replace[] =
		{
			{ "%ws", std::to_string(optimal_group_size) },
		};

		m_src = fmt::replace_all(m_src, syntax_replace);
	}

	void cs_aggregator::bind_resources()
	{
		m_program->bind_buffer({ src->value, 0, block_length }, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);
		m_program->bind_buffer({ dst->value, 0, 4 }, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);
	}

	void cs_aggregator::run(VkCommandBuffer cmd, const vk::buffer* dst, const vk::buffer* src, u32 num_words)
	{
		this->dst = dst;
		this->src = src;
		word_count = num_words;
		block_length = num_words * 4;

		const u32 linear_invocations = utils::aligned_div(word_count, optimal_group_size);
		compute_task::run(cmd, linear_invocations);
	}
}
