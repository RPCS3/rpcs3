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

			// Meta
			input_type_max_enum,
			input_type_undefined = 0xffff'ffff
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
			::glsl::program_domain domain = ::glsl::glsl_invalid_program;
			program_input_type type = input_type_undefined;

			using bound_data_t = std::variant<bound_buffer, bound_sampler, push_constant_ref>;
			bound_data_t bound_data;

			u32 set = 0;
			u32 location = umax;
			std::string name = "undefined";

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
				u32 set,
				u32 location,
				const bound_data_t& data = bound_buffer{})
			{
				return program_input
				{
					.domain = domain,
					.type = type,
					.bound_data = data,
					.set = set,
					.location = location,
					.name = name
				};
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

		struct descriptor_array_ref_t
		{
			u32 first = 0;
			u32 count = 0;
		};

		using descriptor_slot_t = std::variant<VkDescriptorImageInfo, VkDescriptorBufferInfo, VkBufferView, descriptor_array_ref_t>;

		struct descriptor_table_t
		{
			VkDevice m_device = VK_NULL_HANDLE;
			std::array<std::vector<program_input>, input_type_max_enum> m_inputs;

			std::unique_ptr<vk::descriptor_pool> m_descriptor_pool;
			VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;
			vk::descriptor_set m_descriptor_set{};
			rsx::simple_array<VkDescriptorPoolSize> m_descriptor_pool_sizes;
			rsx::simple_array<VkDescriptorType> m_descriptor_types;

			std::vector<descriptor_slot_t> m_descriptor_slots;
			std::vector<bool> m_descriptors_dirty;
			bool m_any_descriptors_dirty = false;

			rsx::simple_array< VkDescriptorImageInfo> m_scratch_images_array;

			void init(VkDevice dev);
			void destroy();

			void validate() const;

			void create_descriptor_set_layout();
			void create_descriptor_pool();

			VkDescriptorSet allocate_descriptor_set();
			VkDescriptorSet commit();

			template <typename T>
			inline void notify_descriptor_slot_updated(u32 slot, const T& data)
			{
				m_descriptors_dirty[slot] = true;
				m_descriptor_slots[slot] = data;
				m_any_descriptors_dirty = true;
			}
		};

		enum binding_set_index : u32
		{
			// For separate shader objects
			binding_set_index_vertex = 0,
			binding_set_index_fragment = 1,

			// Aliases
			binding_set_index_compute = 0,
			binding_set_index_unified = 0,

			// Meta
			binding_set_index_max_enum = 2,
		};

		class program
		{
			VkDevice m_device = VK_NULL_HANDLE;
			VkPipeline m_pipeline = VK_NULL_HANDLE;
			VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;

			std::variant<VkGraphicsPipelineCreateInfo, VkComputePipelineCreateInfo> m_info;
			std::array<descriptor_table_t, binding_set_index_max_enum> m_sets;
			bool m_linked = false;

			void init();
			void create_pipeline_layout();

			program& load_uniforms(const std::vector<program_input>& inputs);

		public:

			program(VkDevice dev, const VkGraphicsPipelineCreateInfo& create_info, const std::vector<program_input> &vertex_inputs, const std::vector<program_input>& fragment_inputs);
			program(VkDevice dev, const VkComputePipelineCreateInfo& create_info, const std::vector<program_input>& compute_inputs);
			program(const program&) = delete;
			program(program&& other) = delete;
			~program();

			program& link(bool separate_stages);
			program& bind(const vk::command_buffer& cmd, VkPipelineBindPoint bind_point);

			bool has_uniform(program_input_type type, const std::string &uniform_name);
			std::pair<u32, u32> get_uniform_location(::glsl::program_domain domain, program_input_type type, const std::string& uniform_name);

			void bind_uniform(const VkDescriptorImageInfo &image_descriptor, u32 set_id, u32 binding_point);
			void bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, u32 set_id, u32 binding_point);
			void bind_uniform(const VkBufferView &buffer_view, u32 set_id, u32 binding_point);
			void bind_uniform(const VkBufferView &buffer_view, ::glsl::program_domain domain, program_input_type type, const std::string &binding_name);

			void bind_uniform_array(const VkDescriptorImageInfo* image_descriptors, int count, u32 set_id, u32 binding_point);

			inline VkPipelineLayout layout() const { return m_pipeline_layout; }
			inline VkPipeline value() const { return m_pipeline; }
		};
	}
}
