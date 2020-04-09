#pragma once
#include "VKProgramBuffer.h"

namespace vk
{
	using ::program_hash_util::fragment_program_utils;
	using ::program_hash_util::vertex_program_utils;

	class shader_interpreter
	{
		glsl::shader m_vs;
		glsl::shader m_fs;

		std::vector<glsl::program_input> m_vs_inputs;
		std::vector<glsl::program_input> m_fs_inputs;

		VkDevice m_device = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_shared_descriptor_layout = VK_NULL_HANDLE;
		VkPipelineLayout m_shared_pipeline_layout = VK_NULL_HANDLE;
		glsl::program* m_current_interpreter = nullptr;

		struct key_hasher
		{
			size_t operator()(const vk::pipeline_props& props) const
			{
				return rpcs3::hash_struct(props);
			}
		};

		std::unordered_map<vk::pipeline_props, std::unique_ptr<glsl::program>, key_hasher> m_program_cache;
		vk::descriptor_pool m_descriptor_pool;
		size_t m_used_descriptors = 0;

		uint32_t m_vertex_instruction_start = 0;
		uint32_t m_fragment_instruction_start = 0;
		uint32_t m_fragment_textures_start = 0;

		std::pair<VkDescriptorSetLayout, VkPipelineLayout> create_layout(VkDevice dev);
		void create_descriptor_pools(const vk::render_device& dev);

		void build_vs();
		void build_fs();
		glsl::program* link(const vk::pipeline_props& properties);

	public:
		void init(const vk::render_device& dev);
		void destroy();

		glsl::program* get(const vk::pipeline_props& properties);
		bool is_interpreter(const glsl::program* prog) const;

		uint32_t get_vertex_instruction_location() const;
		uint32_t get_fragment_instruction_location() const;

		void update_fragment_textures(const std::array<VkDescriptorImageInfo, 68>& sampled_images, VkDescriptorSet descriptor_set);
		VkDescriptorSet allocate_descriptor_set();
	};
}
