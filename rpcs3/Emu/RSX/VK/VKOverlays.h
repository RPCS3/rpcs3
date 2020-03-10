#pragma once
#include "VKHelpers.h"
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "VKRenderTargets.h"
#include "VKFramebuffer.h"
#include "VKResourceManager.h"
#include "VKRenderPass.h"

#include "../Overlays/overlays.h"

#define VK_OVERLAY_MAX_DRAW_CALLS 1024

namespace vk
{
	//TODO: Refactor text print class to inherit from this base class
	struct overlay_pass
	{
		vk::glsl::shader m_vertex_shader;
		vk::glsl::shader m_fragment_shader;

		vk::descriptor_pool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set = nullptr;
		VkDescriptorSetLayout m_descriptor_layout = nullptr;
		VkPipelineLayout m_pipeline_layout = nullptr;
		u32 m_used_descriptors = 0;

		VkFilter m_sampler_filter = VK_FILTER_LINEAR;
		u32 m_num_usable_samplers = 1;

		std::unordered_map<u64, std::unique_ptr<vk::glsl::program>> m_program_cache;
		std::unique_ptr<vk::sampler> m_sampler;
		std::unique_ptr<vk::framebuffer> m_draw_fbo;
		vk::data_heap m_vao;
		vk::data_heap m_ubo;
		vk::render_device* m_device = nullptr;

		std::string vs_src;
		std::string fs_src;

		graphics_pipeline_state renderpass_config;
		bool multi_primitive = false;

		bool initialized = false;
		bool compiled = false;

		u32 num_drawable_elements = 4;
		u32 first_vertex = 0;

		u32 m_ubo_length = 128;
		u32 m_ubo_offset = 0;
		u32 m_vao_offset = 0;

		overlay_pass()
		{
			//Override-able defaults
			renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		}

		~overlay_pass()
		{
			m_vao.destroy();
			m_ubo.destroy();
		}

		u64 get_pipeline_key(VkRenderPass pass)
		{
			if (!multi_primitive)
			{
				// Default fast path
				return reinterpret_cast<u64>(pass);
			}
			else
			{
				struct
				{
					u64 pass_value;
					u64 config;
				}
				key{ reinterpret_cast<uintptr_t>(pass), static_cast<u64>(renderpass_config.ia.topology) };
				return rpcs3::hash_struct(key);
			}
		}

		void check_heap()
		{
			if (!m_vao.heap)
			{
				m_vao.create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 1 * 0x100000, "overlays VAO", 128);
				m_ubo.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 8 * 0x100000, "overlays UBO", 128);
			}
		}

		void init_descriptors()
		{
			VkDescriptorPoolSize descriptor_pool_sizes[2] =
			{
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_OVERLAY_MAX_DRAW_CALLS * m_num_usable_samplers },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_OVERLAY_MAX_DRAW_CALLS },
			};

			//Reserve descriptor pools
			m_descriptor_pool.create(*m_device, descriptor_pool_sizes, 2, VK_OVERLAY_MAX_DRAW_CALLS, 2);

			std::vector<VkDescriptorSetLayoutBinding> bindings(1 + m_num_usable_samplers);

			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[0].binding = 0;
			bindings[0].pImmutableSamplers = nullptr;

			for (u32 n = 1; n <= m_num_usable_samplers; ++n)
			{
				bindings[n].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				bindings[n].descriptorCount = 1;
				bindings[n].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				bindings[n].binding = n;
				bindings[n].pImmutableSamplers = nullptr;
			}

			VkDescriptorSetLayoutCreateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			infos.pBindings = bindings.data();
			infos.bindingCount = 1 + m_num_usable_samplers;

			CHECK_RESULT(vkCreateDescriptorSetLayout(*m_device, &infos, nullptr, &m_descriptor_layout));

			VkPipelineLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1;
			layout_info.pSetLayouts = &m_descriptor_layout;

			std::vector<VkPushConstantRange> push_constants = get_push_constants();
			if (!push_constants.empty())
			{
				layout_info.pushConstantRangeCount = u32(push_constants.size());
				layout_info.pPushConstantRanges = push_constants.data();
			}

			CHECK_RESULT(vkCreatePipelineLayout(*m_device, &layout_info, nullptr, &m_pipeline_layout));
		}

		virtual void update_uniforms(vk::command_buffer& /*cmd*/, vk::glsl::program* /*program*/)
		{
		}

		virtual std::vector<vk::glsl::program_input> get_vertex_inputs()
		{
			check_heap();
			return{};
		}

		virtual std::vector<vk::glsl::program_input> get_fragment_inputs()
		{
			std::vector<vk::glsl::program_input> fs_inputs;
			fs_inputs.push_back({ ::glsl::program_domain::glsl_fragment_program, vk::glsl::program_input_type::input_type_uniform_buffer,{},{}, 0, "static_data" });

			for (u32 n = 1; n <= m_num_usable_samplers; ++n)
			{
				fs_inputs.push_back({ ::glsl::program_domain::glsl_fragment_program, vk::glsl::program_input_type::input_type_texture,{},{}, n, "fs" + std::to_string(n-1) });
			}

			return fs_inputs;
		}

		virtual void get_dynamic_state_entries(VkDynamicState* /*state_descriptors*/, VkPipelineDynamicStateCreateInfo& /*info*/)
		{}

		virtual std::vector<VkPushConstantRange> get_push_constants()
		{
			return {};
		}

		void upload_vertex_data(f32 *data, u32 count)
		{
			check_heap();

			const auto size = count * sizeof(f32);
			m_vao_offset = static_cast<u32>(m_vao.alloc<16>(size));
			auto dst = m_vao.map(m_vao_offset, size);
			std::memcpy(dst, data, size);
			m_vao.unmap();
		}

		vk::glsl::program* build_pipeline(u64 storage_key, VkRenderPass render_pass)
		{
			if (!compiled)
			{
				m_vertex_shader.create(::glsl::program_domain::glsl_vertex_program, vs_src);
				m_vertex_shader.compile();

				m_fragment_shader.create(::glsl::program_domain::glsl_fragment_program, fs_src);
				m_fragment_shader.compile();

				compiled = true;
			}

			VkPipelineShaderStageCreateInfo shader_stages[2] = {};
			shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shader_stages[0].module = m_vertex_shader.get_handle();
			shader_stages[0].pName = "main";

			shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shader_stages[1].module = m_fragment_shader.get_handle();
			shader_stages[1].pName = "main";

			VkDynamicState dynamic_state_descriptors[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
			VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
			dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
			dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

			get_dynamic_state_entries(dynamic_state_descriptors, dynamic_state_info);
			dynamic_state_info.pDynamicStates = dynamic_state_descriptors;

			VkVertexInputBindingDescription vb = { 0, 16, VK_VERTEX_INPUT_RATE_VERTEX };
			VkVertexInputAttributeDescription via = { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 };
			VkPipelineVertexInputStateCreateInfo vi = {};
			vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vi.vertexBindingDescriptionCount = 1;
			vi.pVertexBindingDescriptions = &vb;
			vi.vertexAttributeDescriptionCount = 1;
			vi.pVertexAttributeDescriptions = &via;

			VkPipelineViewportStateCreateInfo vp = {};
			vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			vp.scissorCount = 1;
			vp.viewportCount = 1;

			VkPipeline pipeline;
			VkGraphicsPipelineCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			info.pVertexInputState = &vi;
			info.pInputAssemblyState = &renderpass_config.ia;
			info.pRasterizationState = &renderpass_config.rs;
			info.pColorBlendState = &renderpass_config.cs;
			info.pMultisampleState = &renderpass_config.ms;
			info.pViewportState = &vp;
			info.pDepthStencilState = &renderpass_config.ds;
			info.stageCount = 2;
			info.pStages = shader_stages;
			info.pDynamicState = &dynamic_state_info;
			info.layout = m_pipeline_layout;
			info.basePipelineIndex = -1;
			info.basePipelineHandle = VK_NULL_HANDLE;
			info.renderPass = render_pass;

			CHECK_RESULT(vkCreateGraphicsPipelines(*m_device, nullptr, 1, &info, NULL, &pipeline));

			auto program = std::make_unique<vk::glsl::program>(*m_device, pipeline, get_vertex_inputs(), get_fragment_inputs());
			auto result = program.get();
			m_program_cache[storage_key] = std::move(program);

			return result;
		}

		void load_program(vk::command_buffer& cmd, VkRenderPass pass, const std::vector<vk::image_view*>& src)
		{
			vk::glsl::program *program = nullptr;
			const auto key = get_pipeline_key(pass);

			auto found = m_program_cache.find(key);
			if (found != m_program_cache.end())
				program = found->second.get();
			else
				program = build_pipeline(key, pass);

			verify(HERE), m_used_descriptors < VK_OVERLAY_MAX_DRAW_CALLS;

			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.descriptorPool = m_descriptor_pool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &m_descriptor_layout;
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

			CHECK_RESULT(vkAllocateDescriptorSets(*m_device, &alloc_info, &m_descriptor_set));
			m_used_descriptors++;

			if (!m_sampler)
			{
				m_sampler = std::make_unique<vk::sampler>(*m_device,
					VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					VK_FALSE, 0.f, 1.f, 0.f, 0.f, m_sampler_filter, m_sampler_filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
			}

			update_uniforms(cmd, program);

			program->bind_uniform({ m_ubo.heap->value, m_ubo_offset, std::max(m_ubo_length, 4u) }, 0, m_descriptor_set);

			for (uint n = 0; n < src.size(); ++n)
			{
				VkDescriptorImageInfo info = { m_sampler->value, src[n]->value, src[n]->image()->current_layout };
				program->bind_uniform(info, "fs" + std::to_string(n), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_descriptor_set);
			}

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program->pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);

			VkBuffer buffers = m_vao.heap->value;
			VkDeviceSize offsets = m_vao_offset;
			vkCmdBindVertexBuffers(cmd, 0, 1, &buffers, &offsets);
		}

		void create(vk::render_device &dev)
		{
			if (!initialized)
			{
				m_device = &dev;
				init_descriptors();

				initialized = true;
			}
		}

		void destroy()
		{
			if (initialized)
			{
				m_vertex_shader.destroy();
				m_fragment_shader.destroy();
				m_program_cache.clear();
				m_sampler.reset();

				vkDestroyDescriptorSetLayout(*m_device, m_descriptor_layout, nullptr);
				vkDestroyPipelineLayout(*m_device, m_pipeline_layout, nullptr);
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

			m_vao.reset_allocation_stats();
			m_ubo.reset_allocation_stats();
		}

		vk::framebuffer* get_framebuffer(vk::image* target, VkRenderPass render_pass)
		{
			VkDevice dev = (*vk::get_current_renderer());
			return vk::get_framebuffer(dev, target->width(), target->height(), render_pass, { target });
		}

		virtual void emit_geometry(vk::command_buffer &cmd)
		{
			vkCmdDraw(cmd, num_drawable_elements, 1, first_vertex, 0);
		}

		virtual void set_up_viewport(vk::command_buffer &cmd, u32 x, u32 y, u32 w, u32 h)
		{
			VkViewport vp{};
			vp.x = static_cast<f32>(x);
			vp.y = static_cast<f32>(y);
			vp.width = static_cast<f32>(w);
			vp.height = static_cast<f32>(h);
			vp.minDepth = 0.f;
			vp.maxDepth = 1.f;
			vkCmdSetViewport(cmd, 0, 1, &vp);

			VkRect2D vs = { { static_cast<s32>(x), static_cast<s32>(y) }, { w, h } };
			vkCmdSetScissor(cmd, 0, 1, &vs);
		}

		void run(vk::command_buffer &cmd, const areau& viewport, vk::framebuffer* fbo, const std::vector<vk::image_view*>& src, VkRenderPass render_pass)
		{
			load_program(cmd, render_pass, src);
			set_up_viewport(cmd, viewport.x1, viewport.y1, viewport.width(), viewport.height());

			vk::begin_renderpass(cmd, render_pass, fbo->value, viewport);
			emit_geometry(cmd);
		}

		void run(vk::command_buffer &cmd, const areau& viewport, vk::image* target, const std::vector<vk::image_view*>& src, VkRenderPass render_pass)
		{
			auto fbo = static_cast<vk::framebuffer_holder*>(get_framebuffer(target, render_pass));
			fbo->add_ref();

			run(cmd, viewport, fbo, src, render_pass);
			fbo->release();
		}

		void run(vk::command_buffer &cmd, const areau& viewport, vk::image* target, vk::image_view* src, VkRenderPass render_pass)
		{
			std::vector<vk::image_view*> views = { src };
			run(cmd, viewport, target, views, render_pass);
		}
	};

	// @Deprecated!!
	struct depth_convert_pass : public overlay_pass
	{
		f32 src_scale_x;
		f32 src_scale_y;

		depth_convert_pass()
		{
			vs_src =
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(std140, set=0, binding=0) uniform static_data{ vec4 regs[8]; };\n"
				"layout(location=0) out vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexIndex % 4], 0., 1.);\n"
				"	tc0 = coords[gl_VertexIndex % 4] * regs[0].xy;\n"
				"}\n";

			fs_src =
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"#extension GL_ARB_shader_stencil_export : enable\n\n"
				"layout(set=0, binding=1) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 rgba_in = texture(fs0, tc0);\n"
				"	gl_FragDepth = rgba_in.w * 0.99609 + rgba_in.x * 0.00389 + rgba_in.y * 0.00002;\n"
				"}\n";

			renderpass_config.set_depth_mask(true);
			renderpass_config.enable_depth_test(VK_COMPARE_OP_ALWAYS);
		}

		void update_uniforms(vk::command_buffer& /*cmd*/, vk::glsl::program* /*program*/) override
		{
			m_ubo_offset = static_cast<u32>(m_ubo.alloc<256>(128));
			auto dst = static_cast<f32*>(m_ubo.map(m_ubo_offset, 128));
			dst[0] = src_scale_x;
			dst[1] = src_scale_y;
			dst[2] = 0.f;
			dst[3] = 0.f;
			m_ubo.unmap();
		}

		void run(vk::command_buffer& cmd, const areai& src_area, const areai& dst_area, vk::image_view* src, vk::image* dst, VkRenderPass render_pass)
		{
			auto real_src = src->image();
			verify(HERE), real_src;

			src_scale_x = static_cast<f32>(src_area.x2) / real_src->width();
			src_scale_y = static_cast<f32>(src_area.y2) / real_src->height();

			overlay_pass::run(cmd, static_cast<areau>(dst_area), dst, src, render_pass);
		}
	};

	struct ui_overlay_renderer : public overlay_pass
	{
		f32 m_time = 0.f;
		f32 m_blur_strength = 0.f;
		color4f m_scale_offset;
		color4f m_color;
		bool m_pulse_glow = false;
		bool m_skip_texture_read = false;
		bool m_clip_enabled = false;
		int  m_texture_type;
		areaf m_clip_region;
		coordf m_viewport;

		std::vector<std::unique_ptr<vk::image>> resources;
		std::unordered_map<u64, std::unique_ptr<vk::image>> font_cache;
		std::unordered_map<u64, std::unique_ptr<vk::image_view>> view_cache;
		std::unordered_map<u64, std::pair<u32, std::unique_ptr<vk::image>>> temp_image_cache;
		std::unordered_map<u64, std::unique_ptr<vk::image_view>> temp_view_cache;
		rsx::overlays::primitive_type m_current_primitive_type = rsx::overlays::primitive_type::quad_list;

		ui_overlay_renderer()
		{
			vs_src =
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(location=0) in vec4 in_pos;\n"
				"layout(std140, set=0, binding=0) uniform static_data{ vec4 regs[8]; };\n"
				"layout(location=0) out vec2 tc0;\n"
				"layout(location=1) out vec4 color;\n"
				"layout(location=2) out vec4 parameters;\n"
				"layout(location=3) out vec4 clip_rect;\n"
				"layout(location=4) out vec4 parameters2;\n"
				"\n"
				"vec2 snap_to_grid(const in vec2 normalized)\n"
				"{\n"
				"	return (floor(normalized * regs[5].xy) + 0.5) / regs[5].xy;\n"
				"}\n"
				"\n"
				"vec4 clip_to_ndc(const in vec4 coord)\n"
				"{\n"
				"	return (coord * regs[0].zwzw) / regs[0].xyxy;\n"
				"}\n"
				"\n"
				"vec4 ndc_to_window(const in vec4 coord)\n"
				"{\n"
				"	return fma(coord, regs[5].xyxy, regs[5].zwzw);\n"
				"}\n"
				"\n"
				"void main()\n"
				"{\n"
				"	tc0.xy = in_pos.zw;\n"
				"	color = regs[1];\n"
				"	parameters = regs[2];\n"
				"	parameters2 = regs[4];\n"
				"	clip_rect = ndc_to_window(clip_to_ndc(regs[3]));\n"
				"	vec4 pos = vec4(clip_to_ndc(in_pos).xy, 0.5, 1.);\n"
				"	pos.xy = snap_to_grid(pos.xy);\n"
				"	gl_Position = (pos + pos) - 1.;\n"
				"}\n";

			fs_src =
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(set=0, binding=1) uniform sampler2D fs0;\n"
				"layout(set=0, binding=2) uniform sampler2DArray fs1;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=1) in vec4 color;\n"
				"layout(location=2) in vec4 parameters;\n"
				"layout(location=3) in vec4 clip_rect;\n"
				"layout(location=4) in vec4 parameters2;\n"
				"layout(location=0) out vec4 ocol;\n"
				"\n"
				"vec4 blur_sample(sampler2D tex, vec2 coord, vec2 tex_offset)\n"
				"{\n"
				"	vec2 coords[9];\n"
				"	coords[0] = coord - tex_offset\n;"
				"	coords[1] = coord + vec2(0., -tex_offset.y);\n"
				"	coords[2] = coord + vec2(tex_offset.x, -tex_offset.y);\n"
				"	coords[3] = coord + vec2(-tex_offset.x, 0.);\n"
				"	coords[4] = coord;\n"
				"	coords[5] = coord + vec2(tex_offset.x, 0.);\n"
				"	coords[6] = coord + vec2(-tex_offset.x, tex_offset.y);\n"
				"	coords[7] = coord + vec2(0., tex_offset.y);\n"
				"	coords[8] = coord + tex_offset;\n"
				"\n"
				"	float weights[9] =\n"
				"	{\n"
				"		1., 2., 1.,\n"
				"		2., 4., 2.,\n"
				"		1., 2., 1.\n"
				"	};\n"
				"\n"
				"	vec4 blurred = vec4(0.);\n"
				"	for (int n = 0; n < 9; ++n)\n"
				"	{\n"
				"		blurred += texture(tex, coords[n]) * weights[n];\n"
				"	}\n"
				"\n"
				"	return blurred / 16.f;\n"
				"}\n"
				"\n"
				"vec4 sample_image(sampler2D tex, vec2 coord, float blur_strength)\n"
				"{\n"
				"	vec4 original = texture(tex, coord);\n"
				"	if (blur_strength == 0) return original;\n"
				"	\n"
				"	vec2 constraints = 1.f / vec2(640, 360);\n"
				"	vec2 res_offset = 1.f / textureSize(fs0, 0);\n"
				"	vec2 tex_offset = max(res_offset, constraints);\n"
				"\n"
				"	// Sample triangle pattern and average\n"
				"	// TODO: Nicer looking gaussian blur with less sampling\n"
				"	vec4 blur0 = blur_sample(tex, coord + vec2(-res_offset.x, 0.), tex_offset);\n"
				"	vec4 blur1 = blur_sample(tex, coord + vec2(res_offset.x, 0.), tex_offset);\n"
				"	vec4 blur2 = blur_sample(tex, coord + vec2(0., res_offset.y), tex_offset);\n"
				"\n"
				"	vec4 blurred = blur0 + blur1 + blur2;\n"
				"	blurred /= 3.;\n"
				"	return mix(original, blurred, blur_strength);\n"
				"}\n"
				"\n"
				"void main()\n"
				"{\n"
				"	if (parameters.w != 0)\n"
				"	{"
				"		if (gl_FragCoord.x < clip_rect.x || gl_FragCoord.x > clip_rect.z ||\n"
				"			gl_FragCoord.y < clip_rect.y || gl_FragCoord.y > clip_rect.w)\n"
				"		{\n"
				"			discard;\n"
				"			return;\n"
				"		}\n"
				"	}\n"
				"\n"
				"	vec4 diff_color = color;\n"
				"	if (parameters.y != 0)\n"
				"		diff_color.a *= (sin(parameters.x) + 1.f) * 0.5f;\n"
				"\n"
				"	if (parameters.z < 1.)\n"
				"	{\n"
				"		ocol = diff_color;\n"
				"	}\n"
				"	else if (parameters.z > 2.)\n"
				"	{\n"
				"		ocol = texture(fs1, vec3(tc0.x, fract(tc0.y), trunc(tc0.y))).rrrr * diff_color;\n"
				"	}\n"
				"	else if (parameters.z > 1.)\n"
				"	{\n"
				"		ocol = texture(fs0, tc0).rrrr * diff_color;\n"
				"	}\n"
				"	else\n"
				"	{\n"
				"		ocol = sample_image(fs0, tc0, parameters2.x).bgra * diff_color;\n"
				"	}\n"
				"}\n";

			// Allow mixed primitive rendering
			multi_primitive = true;

			// 2 input textures
			m_num_usable_samplers = 2;

			renderpass_config.set_attachment_count(1);
			renderpass_config.set_color_mask(0, true, true, true, true);
			renderpass_config.set_depth_mask(false);
			renderpass_config.enable_blend(0,
				VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				VK_BLEND_OP_ADD, VK_BLEND_OP_ADD);
		}

		vk::image_view* upload_simple_texture(vk::render_device &dev, vk::command_buffer &cmd,
			vk::data_heap& upload_heap, u64 key, u32 w, u32 h, u32 layers, bool font, bool temp, void *pixel_src, u32 owner_uid)
		{
			const VkFormat format = (font) ? VK_FORMAT_R8_UNORM : VK_FORMAT_B8G8R8A8_UNORM;
			const u32 pitch = (font) ? w : w * 4;
			const u32 data_size = pitch * h * layers;
			const auto offset = upload_heap.alloc<512>(data_size);
			const auto addr = upload_heap.map(offset, data_size);

			const VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers };

			auto tex = std::make_unique<vk::image>(dev, dev.get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D, format, std::max(w, 1u), std::max(h, 1u), 1, 1, layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				0);

			if (pixel_src && data_size)
				std::memcpy(addr, pixel_src, data_size);
			else if (data_size)
				std::memset(addr, 0, data_size);

			upload_heap.unmap();

			VkBufferImageCopy region;
			region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layers };
			region.bufferOffset = offset;
			region.bufferRowLength = w;
			region.bufferImageHeight = h;
			region.imageOffset = {};
			region.imageExtent = { static_cast<u32>(w), static_cast<u32>(h), 1u };

			change_image_layout(cmd, tex.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
			vkCmdCopyBufferToImage(cmd, upload_heap.heap->value, tex->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			change_image_layout(cmd, tex.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);

			auto view = std::make_unique<vk::image_view>(dev, tex.get());

			auto result = view.get();

			if (!temp || font)
				view_cache[key] = std::move(view);
			else
				temp_view_cache[key] = std::move(view);

			if (font)
				font_cache[key] = std::move(tex);
			else if (!temp)
				resources.push_back(std::move(tex));
			else
				temp_image_cache[key] = std::move(std::make_pair(owner_uid, std::move(tex)));

			return result;
		}

		void create(vk::command_buffer &cmd, vk::data_heap &upload_heap)
		{
			auto& dev = cmd.get_command_pool().get_owner();
			overlay_pass::create(dev);

			rsx::overlays::resource_config configuration;
			configuration.load_files();

			u64 storage_key = 1;
			for (const auto &res : configuration.texture_raw_data)
			{
				upload_simple_texture(dev, cmd, upload_heap, storage_key++, res->w, res->h, 1, false, false, res->data, UINT32_MAX);
			}

			configuration.free_resources();
		}

		void destroy()
		{
			temp_image_cache.clear();
			temp_view_cache.clear();

			resources.clear();
			font_cache.clear();
			view_cache.clear();

			overlay_pass::destroy();
		}

		void remove_temp_resources(u32 key)
		{
			std::vector<u64> keys_to_remove;
			for (const auto& temp_image : temp_image_cache)
			{
				if (temp_image.second.first == key)
				{
					keys_to_remove.push_back(temp_image.first);
				}
			}

			for (const auto& _key : keys_to_remove)
			{
				temp_image_cache.erase(_key);
				temp_view_cache.erase(_key);
			}
		}

		vk::image_view* find_font(rsx::overlays::font *font, vk::command_buffer &cmd, vk::data_heap &upload_heap)
		{
			const auto image_size = font->get_glyph_data_dimensions();

			u64 key = reinterpret_cast<u64>(font);
			auto found = view_cache.find(key);
			if (found != view_cache.end())
			{
				if (const auto raw = found->second->image();
					image_size.width == raw->width() &&
					image_size.height == raw->height() &&
					image_size.depth == raw->layers())
				{
					return found->second.get();
				}
				else
				{
					auto gc = vk::get_resource_manager();
					gc->dispose(font_cache[key]);
					gc->dispose(view_cache[key]);
				}
			}

			// Create font resource
			std::vector<u8> bytes;
			font->get_glyph_data(bytes);

			return upload_simple_texture(cmd.get_command_pool().get_owner(), cmd, upload_heap, key, image_size.width, image_size.height, image_size.depth,
					true, false, bytes.data(), UINT32_MAX);
		}

		vk::image_view* find_temp_image(rsx::overlays::image_info *desc, vk::command_buffer &cmd, vk::data_heap &upload_heap, u32 owner_uid)
		{
			u64 key = reinterpret_cast<u64>(desc);
			auto found = temp_view_cache.find(key);
			if (found != temp_view_cache.end())
				return found->second.get();

			return upload_simple_texture(cmd.get_command_pool().get_owner(), cmd, upload_heap, key, desc->w, desc->h, 1,
					false, true, desc->data, owner_uid);
		}

		void update_uniforms(vk::command_buffer& /*cmd*/, vk::glsl::program* /*program*/) override
		{
			m_ubo_offset = static_cast<u32>(m_ubo.alloc<256>(128));
			auto dst = static_cast<f32*>(m_ubo.map(m_ubo_offset, 128));

			// regs[0] = scaling parameters
			dst[0] = m_scale_offset.r;
			dst[1] = m_scale_offset.g;
			dst[2] = m_scale_offset.b;
			dst[3] = m_scale_offset.a;

			// regs[1] = color
			dst[4] = m_color.r;
			dst[5] = m_color.g;
			dst[6] = m_color.b;
			dst[7] = m_color.a;

			// regs[2] = fs config parameters
			dst[8] = m_time;
			dst[9] = m_pulse_glow? 1.f : 0.f;
			dst[10] = m_skip_texture_read? 0.f : static_cast<f32>(m_texture_type);
			dst[11] = m_clip_enabled ? 1.f : 0.f;

			// regs[3] = clip rect
			dst[12] = m_clip_region.x1;
			dst[13] = m_clip_region.y1;
			dst[14] = m_clip_region.x2;
			dst[15] = m_clip_region.y2;

			// regs[4] = fs config parameters 2
			dst[16] = m_blur_strength;

			// regs[5] = viewport
			dst[20] = m_viewport.width;
			dst[21] = m_viewport.height;
			dst[22] = m_viewport.x;
			dst[23] = m_viewport.y;

			m_ubo.unmap();
		}

		void set_primitive_type(rsx::overlays::primitive_type type)
		{
			m_current_primitive_type = type;

			switch (type)
			{
				case rsx::overlays::primitive_type::quad_list:
				case rsx::overlays::primitive_type::triangle_strip:
					renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
					break;
				case rsx::overlays::primitive_type::line_list:
					renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
					break;
				case rsx::overlays::primitive_type::line_strip:
					renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
					break;
				default:
					fmt::throw_exception("Unexpected primitive type %d" HERE, static_cast<s32>(type));
			}
		}

		void emit_geometry(vk::command_buffer &cmd) override
		{
			if (m_current_primitive_type == rsx::overlays::primitive_type::quad_list)
			{
				// Emulate quads with disjointed triangle strips
				u32 first = 0;
				u32 num_quads = num_drawable_elements / 4;

				for (u32 n = 0; n < num_quads; ++n)
				{
					vkCmdDraw(cmd, 4, 1, first, 0);
					first += 4;
				}
			}
			else
			{
				overlay_pass::emit_geometry(cmd);
			}
		}

		void run(vk::command_buffer &cmd, const areau& viewport, vk::framebuffer* target, VkRenderPass render_pass,
				vk::data_heap &upload_heap, rsx::overlays::overlay &ui)
		{
			m_scale_offset = color4f(ui.virtual_width, ui.virtual_height, 1.f, 1.f);
			m_time = static_cast<f32>(get_system_time() / 1000) * 0.005f;
			m_viewport = { { static_cast<f32>(viewport.x1), static_cast<f32>(viewport.y1) }, { static_cast<f32>(viewport.width()), static_cast<f32>(viewport.height()) } };

			std::vector<vk::image_view*> image_views
			{
				vk::null_image_view(cmd, VK_IMAGE_VIEW_TYPE_2D),
				vk::null_image_view(cmd, VK_IMAGE_VIEW_TYPE_2D_ARRAY)
			};

			for (auto &command : ui.get_compiled().draw_commands)
			{
				num_drawable_elements = static_cast<u32>(command.verts.size());
				const u32 value_count = num_drawable_elements * 4;

				upload_vertex_data(reinterpret_cast<f32*>(command.verts.data()), value_count);
				set_primitive_type(command.config.primitives);

				m_skip_texture_read = false;
				m_color = command.config.color;
				m_pulse_glow = command.config.pulse_glow;
				m_blur_strength = static_cast<f32>(command.config.blur_strength) * 0.01f;
				m_clip_enabled = command.config.clip_region;
				m_clip_region = command.config.clip_rect;
				m_texture_type = 1;

				vk::image_view* src = nullptr;
				switch (command.config.texture_ref)
				{
				case rsx::overlays::image_resource_id::game_icon:
				case rsx::overlays::image_resource_id::backbuffer:
					//TODO
				case rsx::overlays::image_resource_id::none:
					m_skip_texture_read = true;
					break;
				case rsx::overlays::image_resource_id::font_file:
					src = find_font(command.config.font_ref, cmd, upload_heap);
					m_texture_type = src->image()->layers() == 1 ? 2 : 3;
					break;
				case rsx::overlays::image_resource_id::raw_image:
					src = find_temp_image(static_cast<rsx::overlays::image_info*>(command.config.external_data_ref), cmd, upload_heap, ui.uid);
					break;
				default:
					src = view_cache[command.config.texture_ref].get();
					break;
				}

				if (src)
				{
					const int res_id = src->image()->layers() > 1 ? 1 : 0;
					image_views[res_id] = src;
				}

				overlay_pass::run(cmd, viewport, target, image_views, render_pass);
			}

			ui.update();
		}
	};

	struct attachment_clear_pass : public overlay_pass
	{
		color4f clear_color = { 0.f, 0.f, 0.f, 0.f };
		color4f colormask = { 1.f, 1.f, 1.f, 1.f };
		VkRect2D region = {};

		attachment_clear_pass()
		{
			vs_src =
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(push_constant) uniform static_data{ vec4 regs[2]; };\n"
				"layout(location=0) out vec2 tc0;\n"
				"layout(location=1) out vec4 color;\n"
				"layout(location=2) out vec4 mask;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};\n"
				"	tc0 = coords[gl_VertexIndex % 4];\n"
				"	color = regs[0];\n"
				"	mask = regs[1];\n"
				"	gl_Position = vec4(positions[gl_VertexIndex % 4], 0., 1.);\n"
				"}\n";

			fs_src =
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(set=0, binding=1) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=1) in vec4 color;\n"
				"layout(location=2) in vec4 mask;\n"
				"layout(location=0) out vec4 out_color;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 original_color = texture(fs0, tc0);\n"
				"	out_color = mix(original_color, color, bvec4(mask));\n"
				"}\n";

			renderpass_config.set_depth_mask(false);
			renderpass_config.set_color_mask(0, true, true, true, true);
			renderpass_config.set_attachment_count(1);
		}

		std::vector<VkPushConstantRange> get_push_constants() override
		{
			VkPushConstantRange constant;
			constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			constant.offset = 0;
			constant.size = 32;

			return { constant };
		}

		void update_uniforms(vk::command_buffer& cmd, vk::glsl::program* /*program*/) override
		{
			f32 data[8];
			data[0] = clear_color.r;
			data[1] = clear_color.g;
			data[2] = clear_color.b;
			data[3] = clear_color.a;
			data[4] = colormask.r;
			data[5] = colormask.g;
			data[6] = colormask.b;
			data[7] = colormask.a;

			vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 32, data);
		}

		void set_up_viewport(vk::command_buffer &cmd, u32 x, u32 y, u32 w, u32 h) override
		{
			VkViewport vp{};
			vp.x = static_cast<f32>(x);
			vp.y = static_cast<f32>(y);
			vp.width = static_cast<f32>(w);
			vp.height = static_cast<f32>(h);
			vp.minDepth = 0.f;
			vp.maxDepth = 1.f;
			vkCmdSetViewport(cmd, 0, 1, &vp);

			vkCmdSetScissor(cmd, 0, 1, &region);
		}

		bool update_config(u32 clearmask, color4f color)
		{
			color4f mask = { 0.f, 0.f, 0.f, 0.f };
			if (clearmask & 0x10) mask.r = 1.f;
			if (clearmask & 0x20) mask.g = 1.f;
			if (clearmask & 0x40) mask.b = 1.f;
			if (clearmask & 0x80) mask.a = 1.f;

			if (mask != colormask || color != clear_color)
			{
				colormask = mask;
				clear_color = color;
				return true;
			}

			return false;
		}

		void run(vk::command_buffer &cmd, vk::render_target* target, VkRect2D rect, VkRenderPass render_pass)
		{
			region = rect;
			target->read_barrier(cmd);

			// Coverage sampling disabled, but actually report correct number of samples
			renderpass_config.set_multisample_state(target->samples(), 0xFFFF, false, false, false);

			overlay_pass::run(cmd, { 0, 0, target->width(), target->height() }, target,
				target->get_view(0xAAE4, rsx::default_remap_vector), render_pass);
		}
	};

	struct video_out_calibration_pass : public overlay_pass
	{
		union config_t
		{
			struct
			{
				float gamma;
				int   limit_range;
				int   stereo;
				int   stereo_image_count;
			};

			float data[4];
		}
		config;

		video_out_calibration_pass()
		{
			vs_src =
				"#version 450\n\n"
				"layout(location=0) out vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};\n"
				"	tc0 = coords[gl_VertexIndex % 4];\n"
				"	vec2 pos = positions[gl_VertexIndex % 4];\n"
				"	gl_Position = vec4(pos, 0., 1.);\n"
				"}\n";

			fs_src =
				"#version 420\n\n"
				"layout(set=0, binding=1) uniform sampler2D fs0;\n"
				"layout(set=0, binding=2) uniform sampler2D fs1;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=0) out vec4 ocol;\n"
				"\n"
				"layout(push_constant) uniform static_data\n"
				"{\n"
				"	float gamma;\n"
				"	int limit_range;\n"
				"	int stereo;\n"
				"	int stereo_image_count;\n"
				"};\n"
				"\n"
				"vec4 read_source()\n"
				"{\n"
				"	if (stereo == 0) return texture(fs0, tc0);\n"
				"\n"
				"	vec4 left, right;\n"
				"	if (stereo_image_count == 2)\n"
				"	{\n"
				"		left = texture(fs0, tc0);\n"
				"		right = texture(fs1, tc0);\n"
				"	}\n"
				"	else\n"
				"	{\n"
				"		vec2 coord_left = tc0 * vec2(1.f, 0.4898f);\n"
				"		vec2 coord_right = coord_left + vec2(0.f, 0.510204f);\n"
				"		left = texture(fs0, coord_left);\n"
				"		right = texture(fs0, coord_right);\n"
				"	}\n"
				"\n"
				"	return vec4(left.r, right.g, right.b, 1.);\n"
				"}\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 color = read_source();\n"
				"	color.rgb = pow(color.rgb, vec3(gamma));\n"
				"	if (limit_range > 0)\n"
				"		ocol = ((color * 220.) + 16.) / 255.;\n"
				"	else\n"
				"		ocol = color;\n"
				"}\n";

			renderpass_config.set_depth_mask(false);
			renderpass_config.set_color_mask(0, true, true, true, true);
			renderpass_config.set_attachment_count(1);

			m_num_usable_samplers = 2;
		}

		std::vector<VkPushConstantRange> get_push_constants() override
		{
			VkPushConstantRange constant;
			constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			constant.offset = 0;
			constant.size = 16;

			return { constant };
		}

		void update_uniforms(vk::command_buffer& cmd, vk::glsl::program* /*program*/) override
		{
			vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16, config.data);
		}

		void run(vk::command_buffer &cmd, const areau& viewport, vk::framebuffer* target,
			const rsx::simple_array<vk::viewable_image*>& src,
			f32 gamma, bool limited_rgb, bool _3d, VkRenderPass render_pass)
		{
			config.gamma = gamma;
			config.limit_range = limited_rgb? 1 : 0;
			config.stereo = _3d? 1 : 0;
			config.stereo_image_count = std::min(::size32(src), 2u);

			std::vector<vk::image_view*> views;
			views.reserve(2);

			for (auto& img : src)
			{
				// Only raw uploads can possibly have mismatched layout here
				img->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				views.push_back(img->get_view(VK_REMAP_IDENTITY, rsx::default_remap_vector));
			}

			if (views.size() < 2)
			{
				views.push_back(vk::null_image_view(cmd, VK_IMAGE_VIEW_TYPE_2D));
			}

			overlay_pass::run(cmd, viewport, target, views, render_pass);
		}
	};
}
