#pragma once
#include "Emu/RSX/VK/VKProgramPipeline.h"
#include "Emu/RSX/Program/ProgramStateCache.h"
#include "Emu/RSX/VK/VKPipelineCompiler.h"
#include "vkutils/descriptors.h"
#include <unordered_map>

class VKVertexProgram;
class VKFragmentProgram;

namespace vk
{
	using ::program_hash_util::fragment_program_utils;
	using ::program_hash_util::vertex_program_utils;

	class shader_interpreter
	{
		VkDevice m_device = VK_NULL_HANDLE;
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

		struct shader_cache_entry_t
		{
			std::unique_ptr<VKFragmentProgram> m_fs;
			std::unique_ptr<VKVertexProgram> m_vs;
		};

		std::unordered_map<pipeline_key, std::unique_ptr<glsl::program>, key_hasher> m_program_cache;
		std::unordered_map<u64, shader_cache_entry_t> m_shader_cache;

		u32 m_vertex_instruction_start = 0;
		u32 m_fragment_instruction_start = 0;
		u32 m_fragment_textures_start = 0;

		pipeline_key m_current_key{};

		VKVertexProgram* build_vs(u64 compiler_opt);
		VKFragmentProgram* build_fs(u64 compiler_opt);
		glsl::program* link(const vk::pipeline_props& properties, u64 compiler_opt);

		u32 init(VKVertexProgram* vk_prog, u64 compiler_opt) const;
		u32 init(VKFragmentProgram* vk_prog, u64 compiler_opt) const;

	public:
		void init(const vk::render_device& dev);
		void destroy();

		glsl::program* get(
			const vk::pipeline_props& properties,
			const program_hash_util::fragment_program_utils::fragment_program_metadata& fp_metadata,
			const program_hash_util::vertex_program_utils::vertex_program_metadata& vp_metadata,
			u32 vp_ctrl,
			u32 fp_ctrl);

		// Retrieve the shader components that make up the current interpreter
		std::pair<VKVertexProgram*, VKFragmentProgram*> get_shaders() const;

		bool is_interpreter(const glsl::program* prog) const;

		u32 get_vertex_instruction_location() const;
		u32 get_fragment_instruction_location() const;

		void update_fragment_textures(const std::array<VkDescriptorImageInfo, 68>& sampled_images);
	};
}
