#pragma once
#include "VKHelpers.h"
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "VKRenderTargets.h"

#include "../Overlays/overlays.h"

namespace vk
{
	//TODO: Refactor text print class to inherit from this base class
	struct overlay_pass
	{
		VKVertexProgram m_vertex_shader;
		VKFragmentProgram m_fragment_shader;

		vk::descriptor_pool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set = nullptr;
		VkDescriptorSetLayout m_descriptor_layout = nullptr;
		VkPipelineLayout m_pipeline_layout = nullptr;
		u32 m_used_descriptors = 0;

		std::unordered_map<VkRenderPass, std::unique_ptr<vk::glsl::program>> m_program_cache;
		std::unique_ptr<vk::sampler> m_sampler;
		std::unique_ptr<vk::framebuffer> m_draw_fbo;
		std::unique_ptr<vk::buffer> m_vao;
		std::unique_ptr<vk::buffer> m_ubo;
		vk::render_device* m_device = nullptr;

		std::string vs_src;
		std::string fs_src;

		graphics_pipeline_state renderpass_config;

		bool initialized = false;
		bool compiled = false;

		u32 num_drawable_elements = 4;
		u32 first_vertex = 0;

		overlay_pass()
		{
			//Override-able defaults
			renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		}

		~overlay_pass()
		{}

		void init_descriptors()
		{
			VkDescriptorPoolSize descriptor_pool_sizes[2] =
			{
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 120 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 120 },
			};

			//Reserve descriptor pools
			m_descriptor_pool.create(*m_device, descriptor_pool_sizes, 2);

			VkDescriptorSetLayoutBinding bindings[2] = {};

			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[0].binding = 0;

			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[1].binding = 1;

			VkDescriptorSetLayoutCreateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			infos.pBindings = bindings;
			infos.bindingCount = 2;

			CHECK_RESULT(vkCreateDescriptorSetLayout(*m_device, &infos, nullptr, &m_descriptor_layout));

			VkPipelineLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1;
			layout_info.pSetLayouts = &m_descriptor_layout;

			CHECK_RESULT(vkCreatePipelineLayout(*m_device, &layout_info, nullptr, &m_pipeline_layout));
		}

		virtual void update_uniforms(vk::glsl::program* /*program*/)
		{
		}

		virtual std::vector<vk::glsl::program_input> get_vertex_inputs()
		{
			if (!m_vao)
			{
				auto memory_types = vk::get_memory_mapping(m_device->gpu());
				m_vao = std::make_unique<vk::buffer>(*m_device, 1 * 0x100000, memory_types.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
				m_ubo = std::make_unique<vk::buffer>(*m_device, 8 * 0x100000, memory_types.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0);
			}

			return{};
		}

		virtual std::vector<vk::glsl::program_input> get_fragment_inputs()
		{
			std::vector<vk::glsl::program_input> fs_inputs;
			fs_inputs.push_back({ ::glsl::program_domain::glsl_fragment_program, vk::glsl::program_input_type::input_type_texture,{},{}, 0, "fs0" });
			fs_inputs.push_back({ ::glsl::program_domain::glsl_fragment_program, vk::glsl::program_input_type::input_type_uniform_buffer,{},{}, 1, "static_data" });
			return fs_inputs;
		}

		void upload_vertex_data(f32 *data, u32 first, u32 count)
		{
			verify(HERE), (first + count) <= 65536;
			if (!m_vao)
			{
				auto memory_types = vk::get_memory_mapping(m_device->gpu());
				m_vao = std::make_unique<vk::buffer>(*m_device, 1 * 0x100000, memory_types.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
				m_ubo = std::make_unique<vk::buffer>(*m_device, 8 * 0x100000, memory_types.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0);
			}

			auto dst = m_vao->map((first * 4), VK_WHOLE_SIZE);
			std::memcpy(dst, data, count * sizeof(f32));
			m_vao->unmap();
		}

		vk::glsl::program* build_pipeline(VkRenderPass render_pass)
		{
			if (!compiled)
			{
				m_vertex_shader.shader = vs_src;
				m_vertex_shader.Compile();

				m_fragment_shader.shader = fs_src;
				m_fragment_shader.Compile();

				compiled = true;
			}

			VkPipelineShaderStageCreateInfo shader_stages[2] = {};
			shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shader_stages[0].module = m_vertex_shader.handle;
			shader_stages[0].pName = "main";

			shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shader_stages[1].module = m_fragment_shader.handle;
			shader_stages[1].pName = "main";

			VkDynamicState dynamic_state_descriptors[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
			VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
			dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
			dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
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

			VkPipelineMultisampleStateCreateInfo ms = {};
			ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			ms.pSampleMask = NULL;
			ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipeline pipeline;
			VkGraphicsPipelineCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			info.pVertexInputState = &vi;
			info.pInputAssemblyState = &renderpass_config.ia;
			info.pRasterizationState = &renderpass_config.rs;
			info.pColorBlendState = &renderpass_config.cs;
			info.pMultisampleState = &ms;
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
			m_program_cache[render_pass] = std::move(program);

			return result;
		}

		void load_program(vk::command_buffer cmd, VkRenderPass pass, VkImageView src)
		{
			vk::glsl::program *program = nullptr;
			auto found = m_program_cache.find(pass);
			if (found != m_program_cache.end())
				program = found->second.get();
			else
				program = build_pipeline(pass);

			verify(HERE), m_used_descriptors < 120;

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
					VK_FALSE, 0.f, 1.f, 0.f, 0.f, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
			}

			VkDescriptorImageInfo info = { m_sampler->value, src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			program->bind_uniform(info, "fs0", m_descriptor_set);
			program->bind_uniform({ m_ubo->value, first_vertex * 128, 128 }, 1, m_descriptor_set);

			update_uniforms(program);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program->pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);

			VkBuffer buffers = m_vao->value;
			VkDeviceSize offsets = 0;
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

			vkResetDescriptorPool(*m_device, m_descriptor_pool, 0);
			m_used_descriptors = 0;
		}

		vk::framebuffer* get_framebuffer(vk::image* target, VkRenderPass render_pass, std::list<std::unique_ptr<vk::framebuffer_holder>>& framebuffer_resources)
		{
			std::vector<vk::image*> test = {target};
			for (auto It = framebuffer_resources.begin(); It != framebuffer_resources.end(); It++)
			{
				auto fbo = It->get();
				if (fbo->matches(test, target->width(), target->height()))
				{
					fbo->deref_count = 0;
					return fbo;
				}
			}

			//No match, create new fbo and add to the list
			std::vector<std::unique_ptr<vk::image_view>> views;
			VkComponentMapping mapping = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
			VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			switch (target->info.format)
			{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; //We are only writing to depth
				break;
			}

			auto view = std::make_unique<vk::image_view>(*m_device, target->value, VK_IMAGE_VIEW_TYPE_2D, target->info.format, mapping, range);
			views.push_back(std::move(view));

			auto fbo = std::make_unique<vk::framebuffer_holder>(*m_device, render_pass, target->width(), target->height(), std::move(views));
			auto result = fbo.get();
			framebuffer_resources.push_back(std::move(fbo));

			return result;
		}

		virtual void emit_geometry(vk::command_buffer &cmd)
		{
			vkCmdDraw(cmd, num_drawable_elements, 1, first_vertex, 0);
		}

		virtual void set_up_viewport(vk::command_buffer &cmd, u16 max_w, u16 max_h)
		{
			VkViewport vp{};
			vp.width = (f32)max_w;
			vp.height = (f32)max_h;
			vp.minDepth = 0.f;
			vp.maxDepth = 1.f;
			vkCmdSetViewport(cmd, 0, 1, &vp);

			VkRect2D vs = { { 0, 0 }, { 0u + max_w, 0u + max_h } };
			vkCmdSetScissor(cmd, 0, 1, &vs);
		}

		void run(vk::command_buffer &cmd, u16 w, u16 h, vk::framebuffer* fbo, VkImageView src, VkRenderPass render_pass)
		{
			load_program(cmd, render_pass, src);
			set_up_viewport(cmd, w, h);

			VkRenderPassBeginInfo rp_begin = {};
			rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin.renderPass = render_pass;
			rp_begin.framebuffer = fbo->value;
			rp_begin.renderArea.offset.x = 0;
			rp_begin.renderArea.offset.y = 0;
			rp_begin.renderArea.extent.width = w;
			rp_begin.renderArea.extent.height = h;

			vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
			emit_geometry(cmd);
			vkCmdEndRenderPass(cmd);
		}

		void run(vk::command_buffer &cmd, u16 w, u16 h, vk::image* target, VkImageView src, VkRenderPass render_pass, std::list<std::unique_ptr<vk::framebuffer_holder>>& framebuffer_resources)
		{
			vk::framebuffer *fbo = get_framebuffer(target, render_pass, framebuffer_resources);
			run(cmd, w, h, fbo, src, render_pass);
		}

		void run(vk::command_buffer &cmd, u16 w, u16 h, vk::image* target, vk::image_view* src, VkRenderPass render_pass, std::list<std::unique_ptr<vk::framebuffer_holder>>& framebuffer_resources)
		{
			run(cmd, w, h, target, src->value, render_pass, framebuffer_resources);
		}
	};

	struct depth_convert_pass : public overlay_pass
	{
		depth_convert_pass()
		{
			vs_src =
			{
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(location=0) out vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexIndex % 4], 0., 1.);\n"
				"	tc0 = coords[gl_VertexIndex % 4];\n"
				"}\n"
			};

			fs_src =
			{
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(set=0, binding=0) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 rgba_in = texture(fs0, tc0);\n"
				"	gl_FragDepth = rgba_in.w * 0.99609 + rgba_in.x * 0.00389 + rgba_in.y * 0.00002;\n"
				"}\n"
			};

			renderpass_config.set_depth_mask(true);
			renderpass_config.enable_depth_test(VK_COMPARE_OP_ALWAYS);

			m_vertex_shader.id = 100002;
			m_fragment_shader.id = 100003;
		}
	};

	struct ui_overlay_renderer : public overlay_pass
	{
		f32 m_time = 0.f;
		color4f m_scale_offset;
		color4f m_color;
		bool m_pulse_glow = false;
		bool m_skip_texture_read = false;
		bool m_clip_enabled;
		areaf m_clip_region;

		std::vector<std::unique_ptr<vk::image>> resources;
		std::unordered_map<u64, std::unique_ptr<vk::image>> font_cache;
		std::unordered_map<u64, std::unique_ptr<vk::image_view>> view_cache;
		std::unordered_map<u64, std::pair<u32, std::unique_ptr<vk::image>>> temp_image_cache;
		std::unordered_map<u64, std::unique_ptr<vk::image_view>> temp_view_cache;

		ui_overlay_renderer()
		{
			vs_src =
			{
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(location=0) in vec4 in_pos;\n"
				"layout(std140, set=0, binding=1) uniform static_data{ vec4 regs[8]; };\n"
				"layout(location=0) out vec2 tc0;\n"
				"layout(location=1) out vec4 color;\n"
				"layout(location=2) out vec4 parameters;\n"
				"layout(location=3) out vec4 clip_rect;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	tc0.xy = in_pos.zw;\n"
				"	color = regs[1];\n"
				"	parameters = regs[2];\n"
				"	clip_rect = regs[3] * regs[0].zwzw;\n"
				"	vec4 pos = vec4((in_pos.xy * regs[0].zw) / regs[0].xy, 0.5, 1.);\n"
				"	gl_Position = (pos + pos) - 1.;\n"
				"}\n"
			};

			fs_src =
			{
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(set=0, binding=0) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=1) in vec4 color;\n"
				"layout(location=2) in vec4 parameters;\n"
				"layout(location=3) in vec4 clip_rect;\n"
				"layout(location=0) out vec4 ocol;\n"
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
				"	if (parameters.z != 0)\n"
				"		ocol = texture(fs0, tc0) * diff_color;\n"
				"	else\n"
				"		ocol = diff_color;\n"
				"}\n"
			};

			renderpass_config.set_attachment_count(1);
			renderpass_config.set_color_mask(true, true, true, true);
			renderpass_config.set_depth_mask(false);
			renderpass_config.enable_blend(0,
				VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				VK_BLEND_OP_ADD, VK_BLEND_OP_ADD);

			m_vertex_shader.id = 100004;
			m_fragment_shader.id = 100005;
		}

		vk::image_view* upload_simple_texture(vk::render_device &dev, vk::command_buffer &cmd,
			vk::vk_data_heap& upload_heap, u64 key, int w, int h, bool font, bool temp, void *pixel_src, u32 owner_uid)
		{
			const VkFormat format = (font) ? VK_FORMAT_R8_UNORM : VK_FORMAT_B8G8R8A8_UNORM;
			const u32 pitch = (font) ? w : w * 4;
			const u32 data_size = pitch * h;
			const auto offset = upload_heap.alloc<512>(data_size);
			const auto addr = upload_heap.map(offset, data_size);

			const VkComponentMapping bgra = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
			const VkComponentMapping rrrr = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
			const VkComponentMapping mapping = (font) ? rrrr : bgra;

			const VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			auto tex = std::make_unique<vk::image>(dev, dev.get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D, format, std::max(w, 1), std::max(h, 1), 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				0);

			if (pixel_src && data_size)
				std::memcpy(addr, pixel_src, data_size);
			else if (data_size)
				std::memset(addr, 0, data_size);

			upload_heap.unmap();

			VkBufferImageCopy region;
			region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			region.bufferOffset = offset;
			region.bufferRowLength = w;
			region.bufferImageHeight = h;
			region.imageOffset = {};
			region.imageExtent = { (u32)w, (u32)h, 1u};

			change_image_layout(cmd, tex.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
			vkCmdCopyBufferToImage(cmd, upload_heap.heap->value, tex->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			change_image_layout(cmd, tex.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);

			auto view = std::make_unique<vk::image_view>(dev, tex.get(), range, mapping);

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

		void create(vk::command_buffer &cmd, vk::vk_data_heap &upload_heap)
		{
			auto& dev = cmd.get_command_pool().get_owner();
			overlay_pass::create(dev);

			rsx::overlays::resource_config configuration;
			configuration.load_files();

			u64 storage_key = 1;
			for (const auto &res : configuration.texture_raw_data)
			{
				upload_simple_texture(dev, cmd, upload_heap, storage_key++, res->w, res->h, false, false, res->data, UINT32_MAX);
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
			for (auto It = temp_image_cache.begin(); It != temp_image_cache.end(); ++It)
			{
				if (It->second.first == key)
				{
					keys_to_remove.push_back(It->first);
				}
			}

			for (const auto& _key : keys_to_remove)
			{
				temp_image_cache.erase(_key);
				temp_view_cache.erase(_key);
			}
		}

		vk::image_view* find_font(rsx::overlays::font *font, vk::command_buffer &cmd, vk::vk_data_heap &upload_heap)
		{
			u64 key = (u64)font;
			auto found = view_cache.find(key);
			if (found != view_cache.end())
				return found->second.get();

			//Create font file
			return upload_simple_texture(cmd.get_command_pool().get_owner(), cmd, upload_heap, key, font->width, font->height,
					true, false, font->glyph_data.data(), UINT32_MAX);
		}

		vk::image_view* find_temp_image(rsx::overlays::image_info *desc, vk::command_buffer &cmd, vk::vk_data_heap &upload_heap, u32 owner_uid)
		{
			u64 key = (u64)desc;
			auto found = temp_view_cache.find(key);
			if (found != temp_view_cache.end())
				return found->second.get();

			return upload_simple_texture(cmd.get_command_pool().get_owner(), cmd, upload_heap, key, desc->w, desc->h,
					false, true, desc->data, owner_uid);
		}

		void update_uniforms(vk::glsl::program* /*program*/) override
		{
			auto dst = (f32*)m_ubo->map(first_vertex * 128, 128);
			dst[0] = m_scale_offset.r;
			dst[1] = m_scale_offset.g;
			dst[2] = m_scale_offset.b;
			dst[3] = m_scale_offset.a;
			dst[4] = m_color.r;
			dst[5] = m_color.g;
			dst[6] = m_color.b;
			dst[7] = m_color.a;
			dst[8] = m_time;
			dst[9] = m_pulse_glow? 1.f : 0.f;
			dst[10] = m_skip_texture_read? 0.f : 1.f;
			dst[11] = m_clip_enabled ? 1.f : 0.f;
			dst[12] = m_clip_region.x1;
			dst[13] = m_clip_region.y1;
			dst[14] = m_clip_region.x2;
			dst[15] = m_clip_region.y2;
			m_ubo->unmap();
		}

		void emit_geometry(vk::command_buffer &cmd)
		{
			//Split into groups of 4
			auto tmp_first = first_vertex;
			auto num_quads = num_drawable_elements / 4;

			for (u32 n = 0; n < num_quads; ++n)
			{
				vkCmdDraw(cmd, 4, 1, tmp_first, 0);
				tmp_first += 4;
			}
		}

		void run(vk::command_buffer &cmd, u16 w, u16 h, vk::framebuffer* target, VkRenderPass render_pass,
				vk::vk_data_heap &upload_heap, rsx::overlays::overlay &ui)
		{
			m_scale_offset = color4f((f32)ui.virtual_width, (f32)ui.virtual_height, 1.f, 1.f);
			m_time = (f32)(get_system_time() / 1000) * 0.005f;

			u32 vertex_data_offset = 0;
			first_vertex = 0;

			for (auto &command : ui.get_compiled().draw_commands)
			{
				num_drawable_elements = (u32)command.second.size();
				const u32 value_count = num_drawable_elements * 4;

				upload_vertex_data((f32*)command.second.data(), vertex_data_offset, value_count);

				m_skip_texture_read = false;
				m_color = command.first.color;
				m_pulse_glow = command.first.pulse_glow;
				m_clip_enabled = command.first.clip_region;
				m_clip_region = command.first.clip_rect;

				auto src = vk::null_image_view(cmd);
				switch (command.first.texture_ref)
				{
				case rsx::overlays::image_resource_id::game_icon:
				case rsx::overlays::image_resource_id::backbuffer:
					//TODO
				case rsx::overlays::image_resource_id::none:
					m_skip_texture_read = true;
					break;
				case rsx::overlays::image_resource_id::font_file:
					src = find_font(command.first.font_ref, cmd, upload_heap)->value;
					break;
				case rsx::overlays::image_resource_id::raw_image:
					src = find_temp_image((rsx::overlays::image_info*)command.first.external_data_ref, cmd, upload_heap, ui.uid)->value;
					break;
				default:
					src = view_cache[command.first.texture_ref]->value;
					break;
				}

				overlay_pass::run(cmd, w, h, target, src, render_pass);

				vertex_data_offset += value_count;
				first_vertex += num_drawable_elements;
			}

			ui.update();
		}
	};

	struct depth_scaling_pass : public overlay_pass
	{
		depth_scaling_pass()
		{
			vs_src =
			{
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(location=0) out vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexIndex % 4], 0., 1.);\n"
				"	tc0 = coords[gl_VertexIndex % 4];\n"
				"}\n"
			};

			fs_src =
			{
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(set=0, binding=0) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	gl_FragDepth = texture(fs0, tc0).x;\n"
				"}\n"
			};

			renderpass_config.set_depth_mask(true);
			renderpass_config.enable_depth_test(VK_COMPARE_OP_ALWAYS);

			m_vertex_shader.id = 100006;
			m_fragment_shader.id = 100007;
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
			{
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(std140, set=0, binding=1) uniform static_data{ vec4 regs[8]; };\n"
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
				"}\n"
			};

			fs_src =
			{
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(set=0, binding=0) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=1) in vec4 color;\n"
				"layout(location=2) in vec4 mask;\n"
				"layout(location=0) out vec4 out_color;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 original_color = texture(fs0, tc0);\n"
				"	out_color = mix(original_color, color, bvec4(mask));\n"
				"}\n"
			};

			renderpass_config.set_depth_mask(false);
			renderpass_config.set_color_mask(true, true, true, true);
			renderpass_config.set_attachment_count(1);

			m_vertex_shader.id = 100006;
			m_fragment_shader.id = 100007;
		}

		void update_uniforms(vk::glsl::program* /*program*/) override
		{
			auto dst = (f32*)m_ubo->map(0, 128);
			dst[0] = clear_color.r;
			dst[1] = clear_color.g;
			dst[2] = clear_color.b;
			dst[3] = clear_color.a;
			dst[4] = colormask.r;
			dst[5] = colormask.g;
			dst[6] = colormask.b;
			dst[7] = colormask.a;
			m_ubo->unmap();
		}

		void set_up_viewport(vk::command_buffer &cmd, u16 max_w, u16 max_h) override
		{
			VkViewport vp{};
			vp.width = (f32)max_w;
			vp.height = (f32)max_h;
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

		void run(vk::command_buffer &cmd, vk::render_target* target, VkRect2D rect, VkRenderPass render_pass, std::list<std::unique_ptr<vk::framebuffer_holder>>& framebuffer_resources)
		{
			region = rect;

			overlay_pass::run(cmd, target->width(), target->height(), target,
				target->get_view(0xAAE4, rsx::default_remap_vector)->value,
				render_pass, framebuffer_resources);
		}
	};
}
