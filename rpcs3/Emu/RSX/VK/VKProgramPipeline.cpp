#include "stdafx.h"
#include "VKHelpers.h"

namespace vk
{
	namespace glsl
	{
		using namespace ::glsl;

		program::program(VkDevice dev, VkPipeline p, const std::vector<program_input> &vertex_input, const std::vector<program_input>& fragment_inputs)
			: m_device(dev), pipeline(p)
		{
			load_uniforms(program_domain::glsl_vertex_program, vertex_input);
			load_uniforms(program_domain::glsl_vertex_program, fragment_inputs);
			attribute_location_mask = 0;
			vertex_attributes_mask = 0;
		}

		program::~program()
		{
			vkDestroyPipeline(m_device, pipeline, nullptr);
		}

		program& program::load_uniforms(program_domain domain, const std::vector<program_input>& inputs)
		{
			for (auto &item : inputs)
			{
				uniforms[item.type].push_back(item);
			}

			return *this;
		}

		bool program::has_uniform(program_input_type type, const std::string &uniform_name)
		{
			for (const auto &uniform : uniforms[type])
			{
				if (uniform.name == uniform_name)
					return true;
			}

			return false;
		}

		void program::bind_uniform(const VkDescriptorImageInfo &image_descriptor, const std::string& uniform_name, VkDescriptorSet &descriptor_set)
		{
			for (const auto &uniform : uniforms[program_input_type::input_type_texture])
			{
				if (uniform.name == uniform_name)
				{
					const VkWriteDescriptorSet descriptor_writer =
					{
						VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
						nullptr,                                   // pNext
						descriptor_set,                            // dstSet
						uniform.location,                          // dstBinding
						0,                                         // dstArrayElement
						1,                                         // descriptorCount
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
						&image_descriptor,                         // pImageInfo
						nullptr,                                   // pBufferInfo
						nullptr                                    // pTexelBufferView
					};

					vkUpdateDescriptorSets(m_device, 1, &descriptor_writer, 0, nullptr);
					attribute_location_mask |= (1ull << uniform.location);
					return;
				}
			}

			LOG_NOTICE(RSX, "texture not found in program: %s", uniform_name.c_str());
		}

		void program::bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, uint32_t binding_point, VkDescriptorSet &descriptor_set)
		{
			bind_buffer(buffer_descriptor, binding_point, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_set);
		}

		void program::bind_uniform(const VkBufferView &buffer_view, program_input_type type, const std::string &binding_name, VkDescriptorSet &descriptor_set)
		{
			for (const auto &uniform : uniforms[type])
			{
				if (uniform.name == binding_name)
				{
					const VkWriteDescriptorSet descriptor_writer =
					{
						VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // sType
						nullptr,                                // pNext
						descriptor_set,                         // dstSet
						uniform.location,                       // dstBinding
						0,                                      // dstArrayElement
						1,                                      // descriptorCount
						VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,// descriptorType
						nullptr,                                // pImageInfo
						nullptr,                                // pBufferInfo
						&buffer_view                            // pTexelBufferView
					};

					vkUpdateDescriptorSets(m_device, 1, &descriptor_writer, 0, nullptr);
					attribute_location_mask |= (1ull << uniform.location);
					return;
				}
			}
			
			LOG_NOTICE(RSX, "vertex buffer not found in program: %s", binding_name.c_str());
		}

		void program::bind_buffer(const VkDescriptorBufferInfo &buffer_descriptor, uint32_t binding_point, VkDescriptorType type, VkDescriptorSet &descriptor_set)
		{
			const VkWriteDescriptorSet descriptor_writer =
			{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // sType
				nullptr,                                // pNext
				descriptor_set,                         // dstSet
				binding_point,                          // dstBinding
				0,                                      // dstArrayElement
				1,                                      // descriptorCount
				type,                                   // descriptorType
				nullptr,                                // pImageInfo
				&buffer_descriptor,                     // pBufferInfo
				nullptr                                 // pTexelBufferView
			};

			vkUpdateDescriptorSets(m_device, 1, &descriptor_writer, 0, nullptr);
			attribute_location_mask |= (1ull << binding_point);
		}

		u64 program::get_vertex_input_attributes_mask()
		{
			if (vertex_attributes_mask)
				return vertex_attributes_mask;

			for (const auto &uniform : uniforms[program_input_type::input_type_texel_buffer])
			{
				if (uniform.domain == program_domain::glsl_vertex_program)
				{
					vertex_attributes_mask |= (1ull << (uniform.location - VERTEX_BUFFERS_FIRST_BIND_SLOT));
				}
			}

			return vertex_attributes_mask;
		}
	}
}
