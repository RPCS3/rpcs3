#include "stdafx.h"
#include "VKPipelineCompiler.h"
#include "VKRenderPass.h"
#include "Utilities/Thread.h"

#include <thread>

namespace vk
{
	// Global list of worker threads
	std::unique_ptr<named_thread_group<pipe_compiler>> g_pipe_compilers;
	int g_num_pipe_compilers = 0;
	atomic_t<int> g_compiler_index{};

	pipe_compiler::pipe_compiler()
	{
		// TODO: Initialize workqueue
	}

	pipe_compiler::~pipe_compiler()
	{
		// TODO: Destroy and do cleanup
	}

	void pipe_compiler::initialize(const vk::render_device* pdev)
	{
		m_device = pdev;
	}

	void pipe_compiler::operator()()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
			for (auto&& job : m_work_queue.pop_all())
			{
				if (job.is_graphics_job)
				{
					auto compiled = int_compile_graphics_pipe(job.graphics_data, job.graphics_modules, job.pipe_layout, job.inputs, {});
					job.callback_func(compiled);
				}
				else
				{
					auto compiled = int_compile_compute_pipe(job.compute_data, job.pipe_layout);
					job.callback_func(compiled);
				}
			}

			m_work_queue.wait();
		}
	}

	std::unique_ptr<glsl::program> pipe_compiler::int_compile_compute_pipe(const VkComputePipelineCreateInfo& create_info, VkPipelineLayout pipe_layout)
	{
		VkPipeline pipeline;
		vkCreateComputePipelines(*get_current_renderer(), nullptr, 1, &create_info, nullptr, &pipeline);
		return std::make_unique<vk::glsl::program>(*m_device, pipeline, pipe_layout);
	}

	std::unique_ptr<glsl::program> pipe_compiler::int_compile_graphics_pipe(const VkGraphicsPipelineCreateInfo& create_info, VkPipelineLayout pipe_layout,
			const std::vector<glsl::program_input>& vs_inputs, const std::vector<glsl::program_input>& fs_inputs)
	{
		VkPipeline pipeline;
		CHECK_RESULT(vkCreateGraphicsPipelines(*m_device, nullptr, 1, &create_info, NULL, &pipeline));
		auto result = std::make_unique<vk::glsl::program>(*m_device, pipeline, pipe_layout, vs_inputs, fs_inputs);
		result->link();
		return result;
	}

	std::unique_ptr<glsl::program> pipe_compiler::int_compile_graphics_pipe(const vk::pipeline_props &create_info, VkShaderModule modules[2], VkPipelineLayout pipe_layout,
			const std::vector<glsl::program_input>& vs_inputs, const std::vector<glsl::program_input>& fs_inputs)
	{
		VkPipelineShaderStageCreateInfo shader_stages[2] = {};
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = modules[0];
		shader_stages[0].pName = "main";

		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = modules[1];
		shader_stages[1].pName = "main";

		std::vector<VkDynamicState> dynamic_state_descriptors;
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_SCISSOR);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

		if (vk::get_current_renderer()->get_depth_bounds_support())
		{
			dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
		}

		VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.pDynamicStates = dynamic_state_descriptors.data();
		dynamic_state_info.dynamicStateCount = ::size32(dynamic_state_descriptors);

		VkPipelineVertexInputStateCreateInfo vi = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.viewportCount = 1;
		vp.scissorCount = 1;

		VkPipelineMultisampleStateCreateInfo ms = create_info.state.ms;
		verify("Multisample state mismatch!" HERE), ms.rasterizationSamples == VkSampleCountFlagBits((create_info.renderpass_key >> 16) & 0xF);
		if (ms.rasterizationSamples != VK_SAMPLE_COUNT_1_BIT)
		{
			// Update the sample mask pointer
			ms.pSampleMask = &create_info.state.temp_storage.msaa_sample_mask;
		}

		// Rebase pointers from pipeline structure in case it is moved/copied
		VkPipelineColorBlendStateCreateInfo cs = create_info.state.cs;
		cs.pAttachments = create_info.state.att_state;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.pVertexInputState = &vi;
		info.pInputAssemblyState = &create_info.state.ia;
		info.pRasterizationState = &create_info.state.rs;
		info.pColorBlendState = &cs;
		info.pMultisampleState = &ms;
		info.pViewportState = &vp;
		info.pDepthStencilState = &create_info.state.ds;
		info.stageCount = 2;
		info.pStages = shader_stages;
		info.pDynamicState = &dynamic_state_info;
		info.layout = pipe_layout;
		info.basePipelineIndex = -1;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.renderPass = vk::get_renderpass(*m_device, create_info.renderpass_key);

		return int_compile_graphics_pipe(info, pipe_layout, vs_inputs, fs_inputs);
	}

	std::unique_ptr<glsl::program> pipe_compiler::compile(
		const VkComputePipelineCreateInfo& create_info,
		VkPipelineLayout pipe_layout,
		op_flags flags, callback_t callback)
	{
		if (flags == COMPILE_INLINE)
		{
			return int_compile_compute_pipe(create_info, pipe_layout);
		}

		m_work_queue.push(create_info, pipe_layout, callback);
		return {};
	}

	std::unique_ptr<glsl::program> pipe_compiler::compile(
		const VkGraphicsPipelineCreateInfo& create_info,
		VkPipelineLayout pipe_layout,
		op_flags flags, callback_t /*callback*/,
		const std::vector<glsl::program_input>& vs_inputs, const std::vector<glsl::program_input>& fs_inputs)
	{
		// It is very inefficient to defer this as all pointers need to be saved
		verify(HERE), flags == COMPILE_INLINE;
		return int_compile_graphics_pipe(create_info, pipe_layout, vs_inputs, fs_inputs);
	}

	std::unique_ptr<glsl::program> pipe_compiler::compile(
		const vk::pipeline_props& create_info,
		VkShaderModule module_handles[2],
		VkPipelineLayout pipe_layout,
		op_flags flags, callback_t callback,
		const std::vector<glsl::program_input>& vs_inputs, const std::vector<glsl::program_input>& fs_inputs)
	{
		if (flags == COMPILE_INLINE)
		{
			return int_compile_graphics_pipe(create_info, module_handles, pipe_layout, vs_inputs, fs_inputs);
		}

		m_work_queue.push(create_info, pipe_layout, module_handles, vs_inputs, fs_inputs, callback);
		return {};
	}

	void initialize_pipe_compiler(int num_worker_threads)
	{
		if (num_worker_threads == 0)
		{
			// Select optimal number of compiler threads
			const auto hw_threads = std::thread::hardware_concurrency();
			if (hw_threads > 12)
			{
				num_worker_threads = 6;
			}
			else if (hw_threads > 8)
			{
				num_worker_threads = 4;
			}
			else if (hw_threads == 8)
			{
				num_worker_threads = 2;
			}
			else
			{
				num_worker_threads = 1;
			}
		}

		verify(HERE), num_worker_threads >= 1;

		const vk::render_device* dev = vk::get_current_renderer();
		verify("Cannot initialize pipe compiler before creating a logical device" HERE), dev;

		// Create the thread pool
		g_pipe_compilers = std::make_unique<named_thread_group<pipe_compiler>>("RSX.W", num_worker_threads);
		g_num_pipe_compilers = num_worker_threads;

		// Initialize the workers. At least one inline compiler shall exist (doesn't actually run)
		for (pipe_compiler& compiler : *g_pipe_compilers.get())
		{
			compiler.initialize(dev);
		}
	}

	void destroy_pipe_compiler()
	{
		g_pipe_compilers.reset();
	}

	pipe_compiler* get_pipe_compiler()
	{
		verify(HERE), g_pipe_compilers;
		int thread_index = g_compiler_index++;

		return g_pipe_compilers.get()->begin() + (thread_index % g_num_pipe_compilers);
	}
}
