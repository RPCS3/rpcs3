#pragma once

#include "VulkanAPI.h"
#include "VKCommonDecompiler.h"
#include "../Common/GLSLTypes.h"

#include <string>
#include <vector>

namespace vk
{
	namespace glsl
	{
		enum program_input_type : u32
		{
			input_type_uniform_buffer = 0,
			input_type_texel_buffer = 1,
			input_type_texture = 2,
			input_type_storage_buffer = 3,

			input_type_max_enum = 4
		};

		struct bound_sampler
		{
			VkFormat format;
			VkImage image;
			VkComponentMapping mapping;
		};

		struct bound_buffer
		{
			VkFormat format = VK_FORMAT_UNDEFINED;
			VkBuffer buffer = nullptr;
			u64 offset = 0;
			u64 size = 0;
		};

		struct program_input
		{
			::glsl::program_domain domain;
			program_input_type type;

			bound_buffer as_buffer;
			bound_sampler as_sampler;

			u32 location;
			std::string name;
		};

		class shader
		{
			::glsl::program_domain type = ::glsl::program_domain::glsl_vertex_program;
			VkShaderModule m_handle = VK_NULL_HANDLE;
			std::string m_source;
			std::vector<u32> m_compiled;

		public:
			shader() = default;
			~shader() = default;

			void create(::glsl::program_domain domain, const std::string& source);

			VkShaderModule compile();

			void destroy();

			const std::string& get_source() const;
			const std::vector<u32> get_compiled() const;

			VkShaderModule get_handle() const;
		};

		class program
		{
			std::array<std::vector<program_input>, input_type_max_enum> uniforms;
			VkDevice m_device;

			std::array<u32, 16> fs_texture_bindings;
			std::array<u32, 16> fs_texture_mirror_bindings;
			std::array<u32, 4>  vs_texture_bindings;
			bool linked;

			void create_impl();

		public:
			VkPipeline pipeline;
			VkPipelineLayout pipeline_layout;
			u64 attribute_location_mask;
			u64 vertex_attributes_mask;

			program(VkDevice dev, VkPipeline p, VkPipelineLayout layout, const std::vector<program_input> &vertex_input, const std::vector<program_input>& fragment_inputs);
			program(VkDevice dev, VkPipeline p, VkPipelineLayout layout);
			program(const program&) = delete;
			program(program&& other) = delete;
			~program();

			program& load_uniforms(const std::vector<program_input>& inputs);
			program& link();

			bool has_uniform(program_input_type type, const std::string &uniform_name);
			void bind_uniform(const VkDescriptorImageInfo &image_descriptor, const std::string &uniform_name, VkDescriptorType type, VkDescriptorSet &descriptor_set);
			void bind_uniform(const VkDescriptorImageInfo &image_descriptor, int texture_unit, ::glsl::program_domain domain, VkDescriptorSet &descriptor_set, bool is_stencil_mirror = false);
			void bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, u32 binding_point, VkDescriptorSet &descriptor_set);
			void bind_uniform(const VkBufferView &buffer_view, u32 binding_point, VkDescriptorSet &descriptor_set);
			void bind_uniform(const VkBufferView &buffer_view, program_input_type type, const std::string &binding_name, VkDescriptorSet &descriptor_set);

			void bind_buffer(const VkDescriptorBufferInfo &buffer_descriptor, u32 binding_point, VkDescriptorType type, VkDescriptorSet &descriptor_set);
			void bind_descriptor_set(const VkCommandBuffer cmd, VkDescriptorSet descriptor_set);
		};
	}
}
