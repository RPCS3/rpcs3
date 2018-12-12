#pragma once
#include "VKHelpers.h"

#define VK_MAX_COMPUTE_TASKS 1024   // Max number of jobs per frame

namespace vk
{
	struct compute_task
	{
		std::string m_src;
		vk::glsl::shader m_shader;
		std::unique_ptr<vk::glsl::program> m_program;

		vk::descriptor_pool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set = nullptr;
		VkDescriptorSetLayout m_descriptor_layout = nullptr;
		VkPipelineLayout m_pipeline_layout = nullptr;
		u32 m_used_descriptors = 0;

		bool initialized = false;
		bool unroll_loops = true;
		u32 optimal_group_size = 1;
		u32 optimal_kernel_size = 1;

		void init_descriptors()
		{
			VkDescriptorPoolSize descriptor_pool_sizes[1] =
			{
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_MAX_COMPUTE_TASKS },
			};

			//Reserve descriptor pools
			m_descriptor_pool.create(*get_current_renderer(), descriptor_pool_sizes, 1);

			std::vector<VkDescriptorSetLayoutBinding> bindings(1);

			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings[0].binding = 0;
			bindings[0].pImmutableSamplers = nullptr;

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
					// Probably intel
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

			vkResetDescriptorPool(*get_current_renderer(), m_descriptor_pool, 0);
			m_used_descriptors = 0;
		}

		virtual void bind_resources()
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

				std::vector<vk::glsl::program_input> inputs;
				m_program = std::make_unique<vk::glsl::program>(*get_current_renderer(), pipeline, inputs, inputs);
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

		virtual void run(VkCommandBuffer cmd, u32 num_invocations)
		{
			load_program(cmd);
			vkCmdDispatch(cmd, num_invocations, 1, 1);
		}
	};

	struct cs_shuffle_base : compute_task
	{
		vk::buffer* m_data;
		u32 m_data_offset = 0;
		u32 m_data_length = 0;
		u32 kernel_size = 1;

		void build(const char* function_name, u32 _kernel_size = 0)
		{
			// Initialize to allow detecting optimal settings
			create();

			kernel_size = _kernel_size? _kernel_size : optimal_kernel_size;

			m_src =
			{
				"#version 430\n"
				"layout(local_size_x=%ws, local_size_y=1, local_size_z=1) in;\n"
				"layout(std430, set=0, binding=0) buffer ssbo{ uint data[]; };\n\n"
				"\n"
				"#define KERNEL_SIZE %ks\n"
				"\n"
				"// Generic swap routines\n"
				"#define bswap_u16(bits)     (bits & 0xFF) << 8 | (bits & 0xFF00) >> 8 | (bits & 0xFF0000) << 8 | (bits & 0xFF000000) >> 8\n"
				"#define bswap_u32(bits)     (bits & 0xFF) << 24 | (bits & 0xFF00) << 8 | (bits & 0xFF0000) >> 8 | (bits & 0xFF000000) >> 24\n"
				"#define bswap_u16_u32(bits) (bits & 0xFFFF) << 16 | (bits & 0xFFFF0000) >> 16\n"
				"\n"
				"// Depth format conversions\n"
				"#define d24x8_to_f32(bits)           floatBitsToUint(float(bits >> 8) / 16777214.f)\n"
				"#define d24x8_to_d24x8_swapped(bits) (bits & 0xFF00) | (bits & 0xFF0000) >> 16 | (bits & 0xFF) << 16\n"
				"#define f32_to_d24x8_swapped(bits)   d24x8_to_d24x8_swapped(uint(uintBitsToFloat(bits) * 16777214.f))\n"
				"\n"
				"void main()\n"
				"{\n"
				"	uint index = gl_GlobalInvocationID.x * KERNEL_SIZE;\n"
				"	uint value;\n"
				"\n"
			};

			std::string work_kernel =
			{
				"		value = data[index];\n"
				"		data[index] = %f(value);\n"
			};

			std::string loop_advance =
			{
				"		index++;\n"
			};

			const std::string suffix =
			{
				"}\n"
			};

			const std::pair<std::string, std::string> syntax_replace[] =
			{
				{ "%ws", std::to_string(optimal_group_size) },
				{ "%ks", std::to_string(kernel_size) },
				{ "%f", function_name }
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
		}

		void run(VkCommandBuffer cmd, vk::buffer* data, u32 data_length, u32 data_offset = 0)
		{
			m_data = data;
			m_data_offset = data_offset;
			m_data_length = data_length;

			const auto num_bytes_per_invocation = optimal_group_size * kernel_size * 4;
			const auto num_bytes_to_process = align(data_length, num_bytes_per_invocation);
			const auto num_invocations = num_bytes_to_process / num_bytes_per_invocation;

			if (num_bytes_to_process > data->size())
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
