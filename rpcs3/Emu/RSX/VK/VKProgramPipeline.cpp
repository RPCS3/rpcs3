#include "stdafx.h"
#include "VKProgramPipeline.h"
#include "vkutils/descriptors.h"
#include "vkutils/device.h"

#include "../Program/SPIRVCommon.h"

namespace vk
{
	namespace glsl
	{
		using namespace ::glsl;

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
				const std::string shader_type = type == ::glsl::program_domain::glsl_vertex_program ? "vertex" :
					type == ::glsl::program_domain::glsl_fragment_program ? "fragment" : "compute";

				rsx_log.notice("%s", m_source);
				fmt::throw_exception("Failed to compile %s shader", shader_type);
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

		void program::create_impl()
		{
			linked = false;
			attribute_location_mask = 0;
			vertex_attributes_mask = 0;

			fs_texture_bindings.fill(~0u);
			fs_texture_mirror_bindings.fill(~0u);
			vs_texture_bindings.fill(~0u);
		}

		program::program(VkDevice dev, VkPipeline p, VkPipelineLayout layout, const std::vector<program_input> &vertex_input, const std::vector<program_input>& fragment_inputs)
			: m_device(dev), pipeline(p), pipeline_layout(layout)
		{
			create_impl();
			load_uniforms(vertex_input);
			load_uniforms(fragment_inputs);
		}

		program::program(VkDevice dev, VkPipeline p, VkPipelineLayout layout)
			: m_device(dev), pipeline(p), pipeline_layout(layout)
		{
			create_impl();
		}

		program::~program()
		{
			vkDestroyPipeline(m_device, pipeline, nullptr);
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

		void program::bind_uniform(const VkDescriptorImageInfo &image_descriptor, const std::string& uniform_name, VkDescriptorType type, vk::descriptor_set &set)
		{
			for (const auto &uniform : uniforms[program_input_type::input_type_texture])
			{
				if (uniform.name == uniform_name)
				{
					set.push(image_descriptor, type, uniform.location);
					attribute_location_mask |= (1ull << uniform.location);
					return;
				}
			}

			rsx_log.notice("texture not found in program: %s", uniform_name.c_str());
		}

		void program::bind_uniform(const VkDescriptorImageInfo & image_descriptor, int texture_unit, ::glsl::program_domain domain, vk::descriptor_set &set, bool is_stencil_mirror)
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

			if (binding != ~0u)
			{
				set.push(image_descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, binding);
				attribute_location_mask |= (1ull << binding);
				return;
			}

			rsx_log.notice("texture not found in program: %stex%u", (domain == ::glsl::program_domain::glsl_vertex_program)? "v" : "", texture_unit);
		}

		void program::bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, u32 binding_point, vk::descriptor_set &set)
		{
			bind_buffer(buffer_descriptor, binding_point, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, set);
		}

		void program::bind_uniform(const VkBufferView &buffer_view, u32 binding_point, vk::descriptor_set &set)
		{
			set.push(buffer_view, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, binding_point);
			attribute_location_mask |= (1ull << binding_point);
		}

		void program::bind_uniform(const VkBufferView &buffer_view, program_input_type type, const std::string &binding_name, vk::descriptor_set &set)
		{
			for (const auto &uniform : uniforms[type])
			{
				if (uniform.name == binding_name)
				{
					bind_uniform(buffer_view, uniform.location, set);
					return;
				}
			}

			rsx_log.notice("vertex buffer not found in program: %s", binding_name.c_str());
		}

		void program::bind_buffer(const VkDescriptorBufferInfo &buffer_descriptor, u32 binding_point, VkDescriptorType type, vk::descriptor_set &set)
		{
			set.push(buffer_descriptor, type, binding_point);
			attribute_location_mask |= (1ull << binding_point);
		}
	}
}
