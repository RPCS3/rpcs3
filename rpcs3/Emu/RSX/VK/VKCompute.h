#pragma once
#include "VKHelpers.h"
#include "Utilities/StrUtil.h"

#define VK_MAX_COMPUTE_TASKS 1024   // Max number of jobs per frame

namespace vk
{
	struct compute_task
	{
		std::string m_src;
		vk::glsl::shader m_shader;
		std::unique_ptr<vk::glsl::program> m_program;
		std::unique_ptr<vk::buffer> m_param_buffer;

		vk::descriptor_pool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set = nullptr;
		VkDescriptorSetLayout m_descriptor_layout = nullptr;
		VkPipelineLayout m_pipeline_layout = nullptr;
		u32 m_used_descriptors = 0;

		bool initialized = false;
		bool unroll_loops = true;
		bool uniform_inputs = false;
		u32 optimal_group_size = 1;
		u32 optimal_kernel_size = 1;

		virtual std::vector<std::pair<VkDescriptorType, u8>> get_descriptor_layout()
		{
			std::vector<std::pair<VkDescriptorType, u8>> result;
			result.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);

			if (uniform_inputs)
			{
				result.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
			}

			return result;
		}

		void init_descriptors()
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
						uint32_t(bindings.size()),
						e.first,
						1,
						VK_SHADER_STAGE_COMPUTE_BIT,
						nullptr
					});
				}
			}

			// Reserve descriptor pools
			m_descriptor_pool.create(*get_current_renderer(), descriptor_pool_sizes.data(), (u32)descriptor_pool_sizes.size(), VK_MAX_COMPUTE_TASKS, 2);

			VkDescriptorSetLayoutCreateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			infos.pBindings = bindings.data();
			infos.bindingCount = (u32)bindings.size();

			CHECK_RESULT(vkCreateDescriptorSetLayout(*get_current_renderer(), &infos, nullptr, &m_descriptor_layout));

			VkPipelineLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1;
			layout_info.pSetLayouts = &m_descriptor_layout;

			CHECK_RESULT(vkCreatePipelineLayout(*get_current_renderer(), &layout_info, nullptr, &m_pipeline_layout));
		}

		void create()
		{
			if (!initialized)
			{
				init_descriptors();

				switch (vk::get_driver_vendor())
				{
				case vk::driver_vendor::unknown:
				case vk::driver_vendor::INTEL:
				case vk::driver_vendor::NVIDIA:
					unroll_loops = true;
					optimal_group_size = 32;
					optimal_kernel_size = 16;
					break;
				case vk::driver_vendor::AMD:
				case vk::driver_vendor::RADV:
					unroll_loops = false;
					optimal_kernel_size = 1;
					optimal_group_size = 64;
					break;
				}

				initialized = true;
			}
		}

		void destroy()
		{
			if (initialized)
			{
				m_shader.destroy();
				m_program.reset();
				m_param_buffer.reset();

				vkDestroyDescriptorSetLayout(*get_current_renderer(), m_descriptor_layout, nullptr);
				vkDestroyPipelineLayout(*get_current_renderer(), m_pipeline_layout, nullptr);
				m_descriptor_pool.destroy();

				initialized = false;
			}
		}

		void free_resources()
		{
			if (m_used_descriptors == 0)
				return;

			m_descriptor_pool.reset(0);
			m_used_descriptors = 0;
		}

		virtual void bind_resources()
		{}

		virtual void declare_inputs()
		{}

		void load_program(VkCommandBuffer cmd)
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

				VkPipeline pipeline;
				vkCreateComputePipelines(*get_current_renderer(), nullptr, 1, &info, nullptr, &pipeline);

				m_program = std::make_unique<vk::glsl::program>(*get_current_renderer(), pipeline);
				declare_inputs();
			}

			verify(HERE), m_used_descriptors < VK_MAX_COMPUTE_TASKS;

			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.descriptorPool = m_descriptor_pool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &m_descriptor_layout;
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

			CHECK_RESULT(vkAllocateDescriptorSets(*get_current_renderer(), &alloc_info, &m_descriptor_set));
			m_used_descriptors++;

			bind_resources();

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_program->pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);
		}

		virtual void run(VkCommandBuffer cmd, u32 invocations_x, u32 invocations_y)
		{
			load_program(cmd);
			vkCmdDispatch(cmd, invocations_x, invocations_y, 1);
		}

		virtual void run(VkCommandBuffer cmd, u32 num_invocations)
		{
			run(cmd, num_invocations, 1);
		}
	};

	struct cs_shuffle_base : compute_task
	{
		const vk::buffer* m_data;
		u32 m_data_offset = 0;
		u32 m_data_length = 0;
		u32 kernel_size = 1;

		std::string variables, work_kernel, loop_advance, suffix;

		cs_shuffle_base()
		{
			work_kernel =
			{
				"		value = data[index];\n"
				"		data[index] = %f(value);\n"
			};

			loop_advance =
			{
				"		index++;\n"
			};

			suffix =
			{
				"}\n"
			};
		}

		void build(const char* function_name, u32 _kernel_size = 0)
		{
			// Initialize to allow detecting optimal settings
			create();

			kernel_size = _kernel_size? _kernel_size : optimal_kernel_size;

			m_src =
			{
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
				"#define d24x8_to_f32(bits)           d24_to_f32(bits >> 8)\n"
				"#define d24x8_to_d24x8_swapped(bits) (bits & 0xFF00) | (bits & 0xFF0000) >> 16 | (bits & 0xFF) << 16\n"
				"#define f32_to_d24x8_swapped(bits)   d24x8_to_d24x8_swapped(f32_to_d24(bits))\n"
				"\n"
				"void main()\n"
				"{\n"
				"	uint index = gl_GlobalInvocationID.x * KERNEL_SIZE;\n"
				"	uint value;\n"
				"	%vars"
				"\n"
			};

			const std::pair<std::string, std::string> syntax_replace[] =
			{
				{ "%ws", std::to_string(optimal_group_size) },
				{ "%ks", std::to_string(kernel_size) },
				{ "%vars", variables },
				{ "%f", function_name },
				{ "%ub", uniform_inputs? "layout(std140, set=0, binding=1) uniform ubo{ uvec4 params[16]; };\n" : "" },
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

		void bind_resources() override
		{
			m_program->bind_buffer({ m_data->value, m_data_offset, m_data_length }, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);

			if (uniform_inputs)
			{
				verify(HERE), m_param_buffer, m_param_buffer->value != VK_NULL_HANDLE;
				m_program->bind_buffer({ m_param_buffer->value, 0, 256 }, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_descriptor_set);
			}
		}

		void set_parameters(VkCommandBuffer cmd, const u32* params, u8 count)
		{
			verify(HERE), uniform_inputs;

			if (!m_param_buffer)
			{
				auto pdev = vk::get_current_renderer();
				m_param_buffer = std::make_unique<vk::buffer>(*pdev, 256, pdev->get_memory_mapping().host_visible_coherent,
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);
			}

			vkCmdUpdateBuffer(cmd, m_param_buffer->value, 0, count * sizeof(u32), params);
		}

		void run(VkCommandBuffer cmd, const vk::buffer* data, u32 data_length, u32 data_offset = 0)
		{
			m_data = data;
			m_data_offset = data_offset;
			m_data_length = data_length;

			const auto num_bytes_per_invocation = optimal_group_size * kernel_size * 4;
			const auto num_bytes_to_process = align(data_length, num_bytes_per_invocation);
			const auto num_invocations = num_bytes_to_process / num_bytes_per_invocation;

			if ((num_bytes_to_process + data_offset) > data->size())
			{
				// Technically robust buffer access should keep the driver from crashing in OOB situations
				LOG_ERROR(RSX, "Inadequate buffer length submitted for a compute operation."
					"Required=%d bytes, Available=%d bytes", num_bytes_to_process, data->size());
			}

			compute_task::run(cmd, num_invocations);
		}
	};

	struct cs_shuffle_16 : cs_shuffle_base
	{
		// byteswap ushort
		cs_shuffle_16()
		{
			cs_shuffle_base::build("bswap_u16");
		}
	};

	struct cs_shuffle_32 : cs_shuffle_base
	{
		// byteswap_ulong
		cs_shuffle_32()
		{
			cs_shuffle_base::build("bswap_u32");
		}
	};

	struct cs_shuffle_32_16 : cs_shuffle_base
	{
		// byteswap_ulong + byteswap_ushort
		cs_shuffle_32_16()
		{
			cs_shuffle_base::build("bswap_u16_u32");
		}
	};

	struct cs_shuffle_d24x8_f32 : cs_shuffle_base
	{
		// convert d24x8 to f32
		cs_shuffle_d24x8_f32()
		{
			cs_shuffle_base::build("d24x8_to_f32");
		}
	};

	struct cs_shuffle_se_f32_d24x8 : cs_shuffle_base
	{
		// convert f32 to d24x8 and swap endianness
		cs_shuffle_se_f32_d24x8()
		{
			cs_shuffle_base::build("f32_to_d24x8_swapped");
		}
	};

	struct cs_shuffle_se_d24x8 : cs_shuffle_base
	{
		// swap endianness of d24x8
		cs_shuffle_se_d24x8()
		{
			cs_shuffle_base::build("d24x8_to_d24x8_swapped");
		}
	};

	// NOTE: D24S8 layout has the stencil in the MSB! Its actually S8|D24|S8|D24 starting at offset 0
	struct cs_interleave_task : cs_shuffle_base
	{
		u32 m_ssbo_length = 0;

		cs_interleave_task()
		{
			uniform_inputs = true;

			variables =
			{
				"	uint block_length = params[0].x >> 2;\n"
				"	uint z_offset = params[0].y >> 2;\n"
				"	uint s_offset = params[0].z >> 2;\n"
				"	uint depth;\n"
				"	uint stencil;\n"
				"	uint stencil_shift;\n"
				"	uint stencil_offset;\n"
			};
		}

		void bind_resources() override
		{
			m_program->bind_buffer({ m_data->value, m_data_offset, m_ssbo_length }, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);

			if (uniform_inputs)
			{
				verify(HERE), m_param_buffer;
				m_program->bind_buffer({ m_param_buffer->value, 0, 256 }, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_descriptor_set);
			}
		}

		void run(VkCommandBuffer cmd, const vk::buffer* data, u32 data_offset, u32 data_length, u32 zeta_offset, u32 stencil_offset)
		{
			u32 parameters[3] = { data_length, zeta_offset - data_offset, stencil_offset - data_offset };
			set_parameters(cmd, parameters, 3);

			m_ssbo_length = stencil_offset + (data_length / 4) - data_offset;
			cs_shuffle_base::run(cmd, data, data_length, data_offset);
		}
	};

	struct cs_gather_d24x8 : cs_interleave_task
	{
		cs_gather_d24x8()
		{
			work_kernel =
			{
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		depth = data[index + z_offset] & 0x00FFFFFF;\n"
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = data[stencil_offset + s_offset];\n"
				"		stencil = (stencil >> stencil_shift) & 0xFF;\n"
				"		value = (depth << 8) | stencil;\n"
				"		data[index] = value;\n"
			};

			cs_shuffle_base::build("");
		}
	};

	struct cs_gather_d32x8 : cs_interleave_task
	{
		cs_gather_d32x8()
		{
			work_kernel =
			{
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		depth = f32_to_d24(data[index + z_offset]);\n"
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = data[stencil_offset + s_offset];\n"
				"		stencil = (stencil >> stencil_shift) & 0xFF;\n"
				"		value = (depth << 8) | stencil;\n"
				"		data[index] = value;\n"
			};

			cs_shuffle_base::build("");
		}
	};

	struct cs_scatter_d24x8 : cs_interleave_task
	{
		cs_scatter_d24x8()
		{
			work_kernel =
			{
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		value = data[index];\n"
				"		data[index + z_offset] = (value >> 8);\n"
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = (value & 0xFF) << stencil_shift;\n"
				"		data[stencil_offset + s_offset] |= stencil;\n"
			};

			cs_shuffle_base::build("");
		}
	};

	struct cs_scatter_d32x8 : cs_interleave_task
	{
		cs_scatter_d32x8()
		{
			work_kernel =
			{
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		value = data[index];\n"
				"		data[index + z_offset] = d24_to_f32(value >> 8);\n"
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = (value & 0xFF) << stencil_shift;\n"
				"		data[stencil_offset + s_offset] |= stencil;\n"
			};

			cs_shuffle_base::build("");
		}
	};

	// TODO: Replace with a proper manager
	extern std::unordered_map<u32, std::unique_ptr<vk::compute_task>> g_compute_tasks;

	template<class T>
	T* get_compute_task()
	{
		u32 index = id_manager::typeinfo::get_index<T>();
		auto &e = g_compute_tasks[index];

		if (!e)
		{
			e = std::make_unique<T>();
			e->create();
		}

		return static_cast<T*>(e.get());
	}

	void reset_compute_tasks();
}
