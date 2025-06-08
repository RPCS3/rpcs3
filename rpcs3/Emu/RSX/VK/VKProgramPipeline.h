#pragma once

#include "VulkanAPI.h"
#include "Emu/RSX/Program/GLSLTypes.h"

#include "vkutils/descriptors.h"

#include <string>
#include <vector>
#include <variant>

namespace vk
{
	namespace glsl
	{
		enum program_input_type : u32
		{
			input_type_uniform_buffer = 0,
			input_type_texel_buffer,
			input_type_texture,
			input_type_storage_buffer,
			input_type_storage_texture,
			input_type_push_constant,

			input_type_max_enum
		};

		struct bound_sampler
		{
			VkFormat format = VK_FORMAT_UNDEFINED;
			VkImage image = VK_NULL_HANDLE;
			VkComponentMapping mapping{};
		};

		struct bound_buffer
		{
			VkFormat format = VK_FORMAT_UNDEFINED;
			VkBuffer buffer = nullptr;
			u64 offset = 0;
			u64 size = 0;
		};

		struct push_constant_ref
		{
			u32 offset = 0;
			u32 size = 0;
		};

		struct program_input
		{
			::glsl::program_domain domain;
			program_input_type type;

			using bound_data_t = std::variant<bound_buffer, bound_sampler, push_constant_ref>;
			bound_data_t bound_data;

			u32 location;
			std::string name;

			inline bound_buffer& as_buffer() { return *std::get_if<bound_buffer>(&bound_data); }
			inline bound_sampler& as_sampler() { return *std::get_if<bound_sampler>(&bound_data); }
			inline push_constant_ref& as_push_constant() { return *std::get_if<push_constant_ref>(&bound_data); }

			inline const bound_buffer& as_buffer() const { return *std::get_if<bound_buffer>(&bound_data); }
			inline const bound_sampler& as_sampler() const { return *std::get_if<bound_sampler>(&bound_data); }
			inline const push_constant_ref& as_push_constant() const { return *std::get_if<push_constant_ref>(&bound_data); }

			static program_input make(
				::glsl::program_domain domain,
				const std::string& name,
				program_input_type type,
				u32 location,
				const bound_data_t& data = bound_buffer{})
			{
				return program_input
				{
					.domain = domain,
					.type = type,
					.bound_data = data,
					.location = location,
					.name = name
				};
			}
		};

		union descriptor_slot_t
		{
			VkDescriptorImageInfo image_info;
			VkDescriptorBufferInfo buffer_info;
			VkBufferView buffer_view;

			bool matches(const VkDescriptorImageInfo& test) const
			{
				return test.imageView == image_info.imageView &&
					test.sampler == image_info.sampler &&
					test.imageLayout == image_info.imageLayout;
			}

			bool matches(const VkDescriptorBufferInfo& test) const
			{
				return test.buffer == buffer_info.buffer &&
					test.offset == buffer_info.offset &&
					test.range == buffer_info.range;
			}

			bool matches(VkBufferView test) const
			{
				return test == buffer_view;
			}
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
			VkDevice m_device = VK_NULL_HANDLE;

			VkPipeline m_pipeline = VK_NULL_HANDLE;
			VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;

			std::array<u32, 16> fs_texture_bindings;
			std::array<u32, 16> fs_texture_mirror_bindings;
			std::array<u32, 4>  vs_texture_bindings;
			bool linked = false;

			std::unique_ptr<vk::descriptor_pool> m_descriptor_pool;
			VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;
			vk::descriptor_set m_descriptor_set{};
			rsx::simple_array<VkDescriptorPoolSize> m_descriptor_pool_sizes;

			std::vector<descriptor_slot_t> m_descriptor_slots;
			std::vector<bool> m_descriptors_dirty;
			rsx::simple_array<VkCopyDescriptorSet> m_copy_cmds;

			void init();

			void create_descriptor_set_layout();
			void create_pipeline_layout();
			void create_descriptor_pool();

			VkDescriptorSet allocate_descriptor_set();
			void next_descriptor_set();

			program& load_uniforms(const std::vector<program_input>& inputs);

		public:

			program(VkDevice dev, const VkGraphicsPipelineCreateInfo& create_info, const std::vector<program_input> &vertex_inputs, const std::vector<program_input>& fragment_inputs);
			program(VkDevice dev, const VkComputePipelineCreateInfo& create_info, const std::vector<program_input>& compute_inputs);
			program(const program&) = delete;
			program(program&& other) = delete;
			~program();

			program& link();
			program& bind(const vk::command_buffer& cmd, VkPipelineBindPoint bind_point);

			bool has_uniform(program_input_type type, const std::string &uniform_name);
			u32 get_uniform_location(program_input_type type, const std::string& uniform_name);

			void bind_uniform(const VkDescriptorImageInfo &image_descriptor, const std::string &uniform_name, VkDescriptorType type);
			void bind_uniform(const VkDescriptorImageInfo &image_descriptor, int texture_unit, ::glsl::program_domain domain, bool is_stencil_mirror = false);
			void bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, u32 binding_point);
			void bind_uniform(const VkBufferView &buffer_view, u32 binding_point);
			void bind_uniform(const VkBufferView &buffer_view, program_input_type type, const std::string &binding_name);
			void bind_buffer(const VkDescriptorBufferInfo &buffer_descriptor, u32 binding_point, VkDescriptorType type);

			void bind_uniform_array(const VkDescriptorImageInfo* image_descriptors, VkDescriptorType type, int count, u32 binding_point);

			inline VkPipelineLayout layout() const { return m_pipeline_layout; }
			inline VkPipeline value() const { return m_pipeline; }
		};
	}
}
