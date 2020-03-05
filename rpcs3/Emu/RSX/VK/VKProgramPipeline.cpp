#include "stdafx.h"
#include "VKHelpers.h"

#include <string>

namespace vk
{
	namespace glsl
	{
		using namespace ::glsl;

		void program::create_impl()
		{
			linked = false;
			attribute_location_mask = 0;
			vertex_attributes_mask = 0;

			fs_texture_bindings.fill(~0u);
			fs_texture_mirror_bindings.fill(~0u);
			vs_texture_bindings.fill(~0u);
		}

		program::program(VkDevice dev, VkPipeline p, const std::vector<program_input> &vertex_input, const std::vector<program_input>& fragment_inputs)
			: m_device(dev), pipeline(p)
		{
			create_impl();
			load_uniforms(vertex_input);
			load_uniforms(fragment_inputs);
		}

		program::program(VkDevice dev, VkPipeline p)
			: m_device(dev), pipeline(p)
		{
			create_impl();
		}

		program::~program()
		{
			vkDestroyPipeline(m_device, pipeline, nullptr);
		}

		program& program::load_uniforms(const std::vector<program_input>& inputs)
		{
			verify("Cannot change uniforms in already linked program!" HERE), !linked;

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

		bool program::has_uniform(program_input_type type, const std::string &uniform_name)
		{
			for (const auto &uniform : uniforms[type])
			{
				if (uniform.name == uniform_name)
					return true;
			}

			return false;
		}

		void program::bind_uniform(const VkDescriptorImageInfo &image_descriptor, const std::string& uniform_name, VkDescriptorType type, VkDescriptorSet &descriptor_set)
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
						type,                                      // descriptorType
						&image_descriptor,                         // pImageInfo
						nullptr,                                   // pBufferInfo
						nullptr                                    // pTexelBufferView
					};

					vkUpdateDescriptorSets(m_device, 1, &descriptor_writer, 0, nullptr);
					attribute_location_mask |= (1ull << uniform.location);
					return;
				}
			}

			rsx_log.notice("texture not found in program: %s", uniform_name.c_str());
		}

		void program::bind_uniform(const VkDescriptorImageInfo & image_descriptor, int texture_unit, ::glsl::program_domain domain, VkDescriptorSet &descriptor_set, bool is_stencil_mirror)
		{
			verify("Unsupported program domain" HERE, domain != ::glsl::program_domain::glsl_compute_program);

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
				const VkWriteDescriptorSet descriptor_writer =
				{
					VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
					nullptr,                                   // pNext
					descriptor_set,                            // dstSet
					binding,                                   // dstBinding
					0,                                         // dstArrayElement
					1,                                         // descriptorCount
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
					&image_descriptor,                         // pImageInfo
					nullptr,                                   // pBufferInfo
					nullptr                                    // pTexelBufferView
				};

				vkUpdateDescriptorSets(m_device, 1, &descriptor_writer, 0, nullptr);
				attribute_location_mask |= (1ull << binding);
				return;
			}

			rsx_log.notice("texture not found in program: %stex%u", (domain == ::glsl::program_domain::glsl_vertex_program)? "v" : "", texture_unit);
		}

		void program::bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, uint32_t binding_point, VkDescriptorSet &descriptor_set)
		{
			bind_buffer(buffer_descriptor, binding_point, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_set);
		}

		void program::bind_uniform(const VkBufferView &buffer_view, uint32_t binding_point, VkDescriptorSet &descriptor_set)
		{
			const VkWriteDescriptorSet descriptor_writer =
			{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // sType
				nullptr,                                // pNext
				descriptor_set,                         // dstSet
				binding_point,                          // dstBinding
				0,                                      // dstArrayElement
				1,                                      // descriptorCount
				VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,// descriptorType
				nullptr,                                // pImageInfo
				nullptr,                                // pBufferInfo
				&buffer_view                            // pTexelBufferView
			};

			vkUpdateDescriptorSets(m_device, 1, &descriptor_writer, 0, nullptr);
			attribute_location_mask |= (1ull << binding_point);
		}

		void program::bind_uniform(const VkBufferView &buffer_view, program_input_type type, const std::string &binding_name, VkDescriptorSet &descriptor_set)
		{
			for (const auto &uniform : uniforms[type])
			{
				if (uniform.name == binding_name)
				{
					bind_uniform(buffer_view, uniform.location, descriptor_set);
					return;
				}
			}

			rsx_log.notice("vertex buffer not found in program: %s", binding_name.c_str());
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
	}
}
