#include "stdafx.h"
#include "VKProgramPipeline.h"
#include "VKResourceManager.h"
#include "vkutils/descriptors.h"
#include "vkutils/device.h"

#include "../Program/SPIRVCommon.h"

namespace vk
{
	extern const vk::render_device* get_current_renderer();

	namespace glsl
	{
		using namespace ::glsl;

		VkDescriptorType to_descriptor_type(program_input_type type)
		{
			switch (type)
			{
			case input_type_uniform_buffer:
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case input_type_texel_buffer:
				return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			case input_type_texture:
				return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case input_type_storage_buffer:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case input_type_storage_texture:
				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			default:
				fmt::throw_exception("Unexpected program input type %d", static_cast<int>(type));
			}
		}

		VkShaderStageFlags to_shader_stage_flags(::glsl::program_domain domain)
		{
			switch (domain)
			{
			case glsl_vertex_program:
				return VK_SHADER_STAGE_VERTEX_BIT;
			case glsl_fragment_program:
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			case glsl_compute_program:
				return VK_SHADER_STAGE_COMPUTE_BIT;
			default:
				fmt::throw_exception("Unexpected domain %d", static_cast<int>(domain));
			}
		}

		const char* to_string(::glsl::program_domain domain)
		{
			switch (domain)
			{
			case glsl_vertex_program:
				return "vertex";
			case glsl_fragment_program:
				return "fragment";
			case glsl_compute_program:
				return "compute";
			default:
				fmt::throw_exception("Unexpected domain %d", static_cast<int>(domain));
			}
		}

		void shader::create(::glsl::program_domain domain, const std::string& source)
		{
			type     = domain;
			m_source = source;
		}

		VkShaderModule shader::compile()
		{
			ensure(m_handle == VK_NULL_HANDLE);

			if (!spirv::compile_glsl_to_spv(m_compiled, m_source, type, ::glsl::glsl_rules_vulkan))
			{
				rsx_log.notice("%s", m_source);
				fmt::throw_exception("Failed to compile %s shader", to_string(type));
			}

			VkShaderModuleCreateInfo vs_info;
			vs_info.codeSize = m_compiled.size() * sizeof(u32);
			vs_info.pNext    = nullptr;
			vs_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vs_info.pCode    = m_compiled.data();
			vs_info.flags    = 0;

			vkCreateShaderModule(*g_render_device, &vs_info, nullptr, &m_handle);

			return m_handle;
		}

		void shader::destroy()
		{
			m_source.clear();
			m_compiled.clear();

			if (m_handle)
			{
				vkDestroyShaderModule(*g_render_device, m_handle, nullptr);
				m_handle = nullptr;
			}
		}

		const std::string& shader::get_source() const
		{
			return m_source;
		}

		const std::vector<u32> shader::get_compiled() const
		{
			return m_compiled;
		}

		VkShaderModule shader::get_handle() const
		{
			return m_handle;
		}

		void program::init()
		{
			linked = false;

			fs_texture_bindings.fill(~0u);
			fs_texture_mirror_bindings.fill(~0u);
			vs_texture_bindings.fill(~0u);
		}

		program::program(VkDevice dev, const VkGraphicsPipelineCreateInfo& create_info, const std::vector<program_input> &vertex_inputs, const std::vector<program_input>& fragment_inputs)
			: m_device(dev)
		{
			init();

			load_uniforms(vertex_inputs);
			load_uniforms(fragment_inputs);

			create_pipeline_layout();
			ensure(m_pipeline_layout);

			auto _create_info = create_info;
			_create_info.layout = m_pipeline_layout;
			CHECK_RESULT(vkCreateGraphicsPipelines(dev, nullptr, 1, &create_info, nullptr, &m_pipeline));
		}

		program::program(VkDevice dev, const VkComputePipelineCreateInfo& create_info, const std::vector<program_input>& compute_inputs)
			: m_device(dev)
		{
			init();

			load_uniforms(compute_inputs);

			create_pipeline_layout();
			ensure(m_pipeline_layout);

			auto _create_info = create_info;
			_create_info.layout = m_pipeline_layout;
			CHECK_RESULT(vkCreateComputePipelines(dev, nullptr, 1, &create_info, nullptr, &m_pipeline));
		}

		program::~program()
		{
			vkDestroyPipeline(m_device, m_pipeline, nullptr);

			if (m_pipeline_layout)
			{
				vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
				vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
				vk::get_resource_manager()->dispose(m_descriptor_pool);
			}
		}

		program& program::load_uniforms(const std::vector<program_input>& inputs)
		{
			ensure(!linked); // "Cannot change uniforms in already linked program!"

			for (auto &item : inputs)
			{
				uniforms[item.type].push_back(item);
			}

			return *this;
		}

		program& program::link()
		{
			// Preprocess texture bindings
			// Link step is only useful for rasterizer programs, compute programs do not need this
			for (const auto &uniform : uniforms[program_input_type::input_type_texture])
			{
				if (const auto name_start = uniform.name.find("tex"); name_start != umax)
				{
					const auto name_end = uniform.name.find("_stencil");
					const auto index_start = name_start + 3;  // Skip 'tex' part
					const auto index_length = (name_end != umax) ? name_end - index_start : name_end;
					const auto index_part = uniform.name.substr(index_start, index_length);
					const auto index = std::stoi(index_part);

					if (name_start == 0)
					{
						// Fragment texture (tex...)
						if (name_end == umax)
						{
							// Normal texture
							fs_texture_bindings[index] = uniform.location;
						}
						else
						{
							// Stencil mirror
							fs_texture_mirror_bindings[index] = uniform.location;
						}
					}
					else
					{
						// Vertex texture (vtex...)
						vs_texture_bindings[index] = uniform.location;
					}
				}
			}

			linked = true;
			return *this;
		}

		bool program::has_uniform(program_input_type type, const std::string& uniform_name)
		{
			const auto& uniform = uniforms[type];
			return std::any_of(uniform.cbegin(), uniform.cend(), [&uniform_name](const auto& u)
			{
				return u.name == uniform_name;
			});
		}

		u32 program::get_uniform_location(program_input_type type, const std::string& uniform_name)
		{
			const auto& uniform = uniforms[type];
			const auto result = std::find_if(uniform.cbegin(), uniform.cend(), [&uniform_name](const auto& u)
			{
				return u.name == uniform_name;
			});

			if (result == uniform.end())
			{
				return { umax };
			}

			return result->location;
		}

		void program::bind_uniform(const VkDescriptorImageInfo &image_descriptor, const std::string& uniform_name, VkDescriptorType type)
		{
			for (const auto &uniform : uniforms[program_input_type::input_type_texture])
			{
				if (uniform.name == uniform_name)
				{
					if (m_descriptor_slots[uniform.location].matches(image_descriptor))
					{
						return;
					}

					next_descriptor_set();
					m_descriptor_set.push(image_descriptor, type, uniform.location);
					m_descriptors_dirty[uniform.location] = false;
					return;
				}
			}

			rsx_log.notice("texture not found in program: %s", uniform_name.c_str());
		}

		void program::bind_uniform(const VkDescriptorImageInfo & image_descriptor, int texture_unit, ::glsl::program_domain domain, bool is_stencil_mirror)
		{
			ensure(domain != ::glsl::program_domain::glsl_compute_program);

			u32 binding;
			if (domain == ::glsl::program_domain::glsl_fragment_program)
			{
				binding = (is_stencil_mirror) ? fs_texture_mirror_bindings[texture_unit] : fs_texture_bindings[texture_unit];
			}
			else
			{
				binding = vs_texture_bindings[texture_unit];
			}

			if (binding == ~0u) [[ unlikely ]]
			{
				rsx_log.notice("texture not found in program: %stex%u", (domain == ::glsl::program_domain::glsl_vertex_program) ? "v" : "", texture_unit);
				return;
			}

			if (m_descriptor_slots[binding].matches(image_descriptor))
			{
				return;
			}

			next_descriptor_set();
			m_descriptor_set.push(image_descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, binding);
			m_descriptors_dirty[binding] = false;
		}

		void program::bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, u32 binding_point)
		{
			bind_buffer(buffer_descriptor, binding_point, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		}

		void program::bind_uniform(const VkBufferView &buffer_view, u32 binding_point)
		{
			if (m_descriptor_slots[binding_point].matches(buffer_view))
			{
				return;
			}

			next_descriptor_set();
			m_descriptor_set.push(buffer_view, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, binding_point);
			m_descriptors_dirty[binding_point] = false;
		}

		void program::bind_uniform(const VkBufferView &buffer_view, program_input_type type, const std::string &binding_name)
		{
			for (const auto &uniform : uniforms[type])
			{
				if (uniform.name == binding_name)
				{
					bind_uniform(buffer_view, uniform.location);
					return;
				}
			}

			rsx_log.notice("vertex buffer not found in program: %s", binding_name.c_str());
		}

		void program::bind_buffer(const VkDescriptorBufferInfo &buffer_descriptor, u32 binding_point, VkDescriptorType type)
		{
			if (m_descriptor_slots[binding_point].matches(buffer_descriptor))
			{
				return;
			}

			next_descriptor_set();
			m_descriptor_set.push(buffer_descriptor, type, binding_point);
			m_descriptors_dirty[binding_point] = false;
		}

		void program::bind_uniform_array(const VkDescriptorImageInfo* image_descriptors, VkDescriptorType type, int count, u32 binding_point)
		{
			// FIXME: Unoptimized...
			bool match = true;
			for (int i = 0; i < count; ++i)
			{
				if (!m_descriptor_slots[binding_point + i].matches(image_descriptors[i]))
				{
					match = false;
					break;
				}
			}

			if (match)
			{
				return;
			}

			next_descriptor_set();
			m_descriptor_set.push(image_descriptors, static_cast<u32>(count), type, binding_point);

			for (int i = 0; i < count; ++i)
			{
				m_descriptors_dirty[binding_point] = false;
			}
		}

		VkDescriptorSet program::allocate_descriptor_set()
		{
			if (!m_descriptor_pool)
			{
				create_descriptor_pool();
			}

			return m_descriptor_pool->allocate(m_descriptor_set_layout);
		}

		void program::next_descriptor_set()
		{
			const auto new_set = allocate_descriptor_set();
			const auto old_set = m_descriptor_set.value();

			if (old_set)
			{
				m_copy_cmds.clear();
				for (unsigned i = 0; i < m_copy_cmds.size(); ++i)
				{
					if (!m_descriptors_dirty[i])
					{
						continue;
					}

					// Reuse already initialized memory. Each command is the same anyway.
					m_copy_cmds.resize(m_copy_cmds.size() + 1);
					auto& cmd = m_copy_cmds.back();
					cmd.srcBinding = cmd.dstBinding = i;
					cmd.srcSet = old_set;
					cmd.dstSet = new_set;
				}

				m_descriptor_set.push(m_copy_cmds);
			}

			m_descriptor_set = allocate_descriptor_set();
		}

		program& program::bind(const vk::command_buffer& cmd, VkPipelineBindPoint bind_point)
		{
			VkDescriptorSet set = m_descriptor_set.value();
			vkCmdBindPipeline(cmd, bind_point, m_pipeline);
			vkCmdBindDescriptorSets(cmd, bind_point, m_pipeline_layout, 0, 1, &set, 0, nullptr);
			return *this;
		}

		void program::create_descriptor_set_layout()
		{
			ensure(m_descriptor_set_layout == VK_NULL_HANDLE);

			rsx::simple_array<VkDescriptorSetLayoutBinding> bindings;
			bindings.reserve(16);

			m_descriptor_pool_sizes.clear();
			m_descriptor_pool_sizes.reserve(input_type_max_enum);

			for (const auto& type_arr : uniforms)
			{
				if (type_arr.empty() || type_arr.front().type == input_type_push_constant)
				{
					continue;
				}

				VkDescriptorType type = to_descriptor_type(type_arr.front().type);
				m_descriptor_pool_sizes.push_back({ .type = type });

				for (const auto& input : type_arr)
				{
					VkDescriptorSetLayoutBinding binding
					{
						.binding = input.location,
						.descriptorType = type,
						.descriptorCount = 1,
						.stageFlags = to_shader_stage_flags(input.domain)
					};
					bindings.push_back(binding);
					m_descriptor_pool_sizes.back().descriptorCount++;
				}
			}

			VkDescriptorSetLayoutCreateInfo set_layout_create_info
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.flags = 0,
				.bindingCount = ::size32(bindings),
				.pBindings = bindings.data()
			};
			CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &set_layout_create_info, nullptr, &m_descriptor_set_layout));
		}

		void program::create_pipeline_layout()
		{
			ensure(!linked);
			ensure(m_pipeline_layout == VK_NULL_HANDLE);

			create_descriptor_set_layout();

			rsx::simple_array<VkPushConstantRange> push_constants{};
			for (const auto& input : uniforms[input_type_push_constant])
			{
				const auto& range = input.as_push_constant();
				push_constants.push_back({ .offset = range.offset, .size = range.size });
			}

			VkPipelineLayoutCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.flags = 0,
				.setLayoutCount = 1,
				.pSetLayouts = &m_descriptor_set_layout,
				.pushConstantRangeCount = ::size32(push_constants),
				.pPushConstantRanges = push_constants.data()
			};
			CHECK_RESULT(vkCreatePipelineLayout(m_device, &create_info, nullptr, &m_pipeline_layout));
		}

		void program::create_descriptor_pool()
		{
			ensure(linked);

			m_descriptor_pool = std::make_unique<descriptor_pool>();
			m_descriptor_pool->create(*vk::get_current_renderer(), m_descriptor_pool_sizes);
		}
	}
}
