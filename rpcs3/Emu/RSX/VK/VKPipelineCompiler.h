#pragma once
#include "../rsx_utils.h"
#include "Utilities/lockless.h"
#include "VKProgramPipeline.h"
#include "vkutils/graphics_pipeline_state.hpp"
#include "util/fnv_hash.hpp"

namespace vk
{
	class render_device;

	struct pipeline_props
	{
		graphics_pipeline_state state;
		u64 renderpass_key;

		bool operator==(const pipeline_props& other) const
		{
			if (renderpass_key != other.renderpass_key)
				return false;

			if (memcmp(&state.ia, &other.state.ia, sizeof(VkPipelineInputAssemblyStateCreateInfo)))
				return false;

			if (memcmp(&state.rs, &other.state.rs, sizeof(VkPipelineRasterizationStateCreateInfo)))
				return false;

			// Cannot memcmp cs due to pAttachments being a pointer to memory
			if (state.cs.attachmentCount != other.state.cs.attachmentCount ||
				state.cs.logicOp != other.state.cs.logicOp ||
				state.cs.logicOpEnable != other.state.cs.logicOpEnable ||
				memcmp(state.cs.blendConstants, other.state.cs.blendConstants, 4 * sizeof(f32)))
				return false;

			if (memcmp(state.att_state, &other.state.att_state, state.cs.attachmentCount * sizeof(VkPipelineColorBlendAttachmentState)))
				return false;

			if (memcmp(&state.ds, &other.state.ds, sizeof(VkPipelineDepthStencilStateCreateInfo)))
				return false;

			if (state.ms.rasterizationSamples != VK_SAMPLE_COUNT_1_BIT)
			{
				if (memcmp(&state.ms, &other.state.ms, sizeof(VkPipelineMultisampleStateCreateInfo)))
					return false;

				if (state.temp_storage.msaa_sample_mask != other.state.temp_storage.msaa_sample_mask)
					return false;
			}

			return true;
		}
	};

	class pipe_compiler
	{
	public:
		enum op_flags
		{
			COMPILE_DEFAULT = 0,
			COMPILE_INLINE = 1,
			COMPILE_DEFERRED = 2
		};

		using callback_t = std::function<void(std::unique_ptr<glsl::program>&)>;

		pipe_compiler();
		~pipe_compiler();

		void initialize(const vk::render_device* pdev);

		std::unique_ptr<glsl::program> compile(
			const VkComputePipelineCreateInfo& create_info,
			VkPipelineLayout pipe_layout,
			op_flags flags, callback_t callback = {});

		std::unique_ptr<glsl::program> compile(
			const VkGraphicsPipelineCreateInfo& create_info,
			VkPipelineLayout pipe_layout,
			op_flags flags, callback_t callback = {},
			const std::vector<glsl::program_input>& vs_inputs = {},
			const std::vector<glsl::program_input>& fs_inputs = {});

		std::unique_ptr<glsl::program> compile(
			const vk::pipeline_props &create_info,
			VkShaderModule module_handles[2],
			VkPipelineLayout pipe_layout,
			op_flags flags, callback_t callback = {},
			const std::vector<glsl::program_input>& vs_inputs = {},
			const std::vector<glsl::program_input>& fs_inputs = {});

		void operator()();

	private:
		class compute_pipeline_props : public VkComputePipelineCreateInfo
		{
			// Storage for the entry name
			std::string entry_name;

		public:
			compute_pipeline_props() = default;
			compute_pipeline_props(const VkComputePipelineCreateInfo& info)
			{
				(*static_cast<VkComputePipelineCreateInfo*>(this)) = info;
				entry_name = info.stage.pName;
				stage.pName = entry_name.c_str();
			}
		};

		struct pipe_compiler_job
		{
			bool is_graphics_job;
			callback_t callback_func;

			vk::pipeline_props graphics_data;
			compute_pipeline_props compute_data;
			VkPipelineLayout pipe_layout;
			VkShaderModule graphics_modules[2];
			std::vector<glsl::program_input> inputs;

			pipe_compiler_job(
				const vk::pipeline_props& props,
				VkPipelineLayout layout,
				VkShaderModule modules[2],
				const std::vector<glsl::program_input>& vs_in,
				const std::vector<glsl::program_input>& fs_in,
				callback_t func)
			{
				callback_func = func;
				graphics_data = props;
				pipe_layout = layout;
				graphics_modules[0] = modules[0];
				graphics_modules[1] = modules[1];
				is_graphics_job = true;

				inputs.reserve(vs_in.size() + fs_in.size());
				inputs.insert(inputs.end(), vs_in.begin(), vs_in.end());
				inputs.insert(inputs.end(), fs_in.begin(), fs_in.end());
			}

			pipe_compiler_job(
				const VkComputePipelineCreateInfo& props,
				VkPipelineLayout layout,
				callback_t func)
			{
				callback_func = func;
				compute_data = props;
				pipe_layout = layout;
				is_graphics_job = false;
			}
		};

		const vk::render_device* m_device = nullptr;
		lf_queue<pipe_compiler_job> m_work_queue;

		std::unique_ptr<glsl::program> int_compile_compute_pipe(const VkComputePipelineCreateInfo& create_info, VkPipelineLayout pipe_layout);
		std::unique_ptr<glsl::program> int_compile_graphics_pipe(const VkGraphicsPipelineCreateInfo& create_info, VkPipelineLayout pipe_layout,
			const std::vector<glsl::program_input>& vs_inputs, const std::vector<glsl::program_input>& fs_inputs);
		std::unique_ptr<glsl::program> int_compile_graphics_pipe(const vk::pipeline_props &create_info, VkShaderModule modules[2], VkPipelineLayout pipe_layout,
			const std::vector<glsl::program_input>& vs_inputs, const std::vector<glsl::program_input>& fs_inputs);
	};

	void initialize_pipe_compiler(int num_worker_threads = -1);
	void destroy_pipe_compiler();
	pipe_compiler* get_pipe_compiler();
}

namespace rpcs3
{
	template <>
	inline usz hash_struct<vk::pipeline_props>(const vk::pipeline_props& pipelineProperties)
	{
		usz seed = hash_base(pipelineProperties.renderpass_key);
		seed ^= hash_struct(pipelineProperties.state.ia);
		seed ^= hash_struct(pipelineProperties.state.ds);
		seed ^= hash_struct(pipelineProperties.state.rs);
		seed ^= hash_struct(pipelineProperties.state.ms);
		seed ^= hash_base(pipelineProperties.state.temp_storage.msaa_sample_mask);

		// Do not compare pointers to memory!
		VkPipelineColorBlendStateCreateInfo tmp;
		memcpy(&tmp, &pipelineProperties.state.cs, sizeof(VkPipelineColorBlendStateCreateInfo));
		tmp.pAttachments = nullptr;

		for (usz i = 0; i < pipelineProperties.state.cs.attachmentCount; ++i)
		{
			seed ^= hash_struct(pipelineProperties.state.att_state[i]);
		}
		return hash_base(seed);
	}
}
