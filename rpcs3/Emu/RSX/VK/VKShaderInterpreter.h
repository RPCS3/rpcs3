#pragma once
#include "VKProgramBuffer.h"

namespace vk
{
	using ::program_hash_util::fragment_program_utils;
	using ::program_hash_util::vertex_program_utils;

	class shader_interpreter
	{
		glsl::shader m_vs;

		std::vector<glsl::program_input> m_vs_inputs;
		std::vector<glsl::program_input> m_fs_inputs;

		VkDevice m_device = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_shared_descriptor_layout = VK_NULL_HANDLE;
		VkPipelineLayout m_shared_pipeline_layout = VK_NULL_HANDLE;
		glsl::program* m_current_interpreter = nullptr;

		struct pipeline_key
		{
			u64 compiler_opt;
			vk::pipeline_props properties;

			bool operator == (const pipeline_key& other) const
			{
				return other.compiler_opt == compiler_opt && other.properties == properties;
			}
		};

		struct key_hasher
		{
			usz operator()(const pipeline_key& key) const
			{
				return rpcs3::hash_struct(key.properties) ^ key.compiler_opt;
			}
		};

		std::unordered_map<pipeline_key, std::unique_ptr<glsl::program>, key_hasher> m_program_cache;
		std::unordered_map<u64, std::unique_ptr<glsl::shader>> m_fs_cache;
		vk::descriptor_pool m_descriptor_pool;
		usz m_used_descriptors = 0;

		u32 m_vertex_instruction_start = 0;
		u32 m_fragment_instruction_start = 0;
		u32 m_fragment_textures_start = 0;

		pipeline_key m_current_key{};

		std::pair<VkDescriptorSetLayout, VkPipelineLayout> create_layout(VkDevice dev);
		void create_descriptor_pools(const vk::render_device& dev);

		void build_vs();
		glsl::shader* build_fs(u64 compiler_opt);
		glsl::program* link(const vk::pipeline_props& properties, u64 compiler_opt);

	public:
		void init(const vk::render_device& dev);
		void destroy();

		glsl::program* get(const vk::pipeline_props& properties, const program_hash_util::fragment_program_utils::fragment_program_metadata& metadata);
		bool is_interpreter(const glsl::program* prog) const;

		u32 get_vertex_instruction_location() const;
		u32 get_fragment_instruction_location() const;

		void update_fragment_textures(const std::array<VkDescriptorImageInfo, 68>& sampled_images, VkDescriptorSet descriptor_set);
		VkDescriptorSet allocate_descriptor_set();
	};
}
