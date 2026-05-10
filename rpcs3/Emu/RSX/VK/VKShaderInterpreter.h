#pragma once
#include "Emu/RSX/VK/VKProgramPipeline.h"
#include "Emu/RSX/Program/ProgramStateCache.h"
#include "Emu/RSX/VK/VKPipelineCompiler.h"
#include "vkutils/descriptors.h"
#include <unordered_map>

class VKVertexProgram;
class VKFragmentProgram;

namespace rsx
{
	struct shader_loading_dialog;
}

namespace vk
{
	using ::program_hash_util::fragment_program_utils;
	using ::program_hash_util::vertex_program_utils;

	class shader_interpreter
	{
		using async_build_fn_callback = std::function<void(std::shared_ptr<glsl::program>&)>;

		VkDevice m_device = VK_NULL_HANDLE;
		VkPipelineCache m_driver_pipeline_cache = VK_NULL_HANDLE;

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
			std::shared_ptr<VKFragmentProgram> m_fs;
			std::shared_ptr<VKVertexProgram> m_vs;
		};

		struct pipeline_info_ex_t
		{
			u32 vertex_instruction_location = 0;
			u32 fragment_instruction_location = 0;
			u32 fragment_textures_location = 0;
		};

		struct pipeline_cache_entry_t
		{
			u32 flags = 0;
			std::shared_ptr<glsl::program> program;
		};

		std::unordered_map<pipeline_key, pipeline_cache_entry_t, key_hasher> m_program_cache;
		std::unordered_map<u64, std::shared_ptr<VKVertexProgram>> m_vs_shader_cache;
		std::unordered_map<u64, std::shared_ptr<VKFragmentProgram>> m_fs_shader_cache;
		std::unordered_map<u64, pipeline_info_ex_t> m_pipeline_info_cache;

		mutable shared_mutex m_program_cache_lock;
		mutable shared_mutex m_vs_shader_cache_lock;
		mutable shared_mutex m_fs_shader_cache_lock;

		pipeline_key m_current_key{};
		pipeline_info_ex_t m_current_pipeline_info_ex{};
		std::shared_ptr<glsl::program> m_current_interpreter;

		std::shared_ptr<VKVertexProgram> build_vs(u64 compiler_opt);
		std::shared_ptr<VKFragmentProgram> build_fs(u64 compiler_opt);
		std::shared_ptr<glsl::program> link(const vk::pipeline_props& properties, u64 compiler_opt, bool async = false, async_build_fn_callback async_callback = {});

		u32 init(std::shared_ptr<VKVertexProgram>& vk_prog, u64 compiler_opt) const;
		u32 init(std::shared_ptr<VKFragmentProgram>& vk_prog, u64 compiler_opt) const;

		const pipeline_info_ex_t* get_pipeline_info_ex(u64 compiler_opt);

	public:
		void init(const vk::render_device& dev);
		void destroy();

		void preload(rsx::shader_loading_dialog* dlg);

		glsl::program* get(
			const vk::pipeline_props& properties,
			const program_hash_util::fragment_program_utils::fragment_program_metadata& fp_metadata,
			const program_hash_util::vertex_program_utils::vertex_program_metadata& vp_metadata,
			u32 vp_ctrl,
			u32 fp_ctrl);

		// Retrieve the shader components that make up the current interpreter
		std::pair<std::shared_ptr<VKVertexProgram>, std::shared_ptr<VKFragmentProgram>> get_shaders() const;

		bool is_interpreter(const glsl::program* prog) const;

		u32 get_vertex_instruction_location() const;
		u32 get_fragment_instruction_location() const;

		void update_fragment_textures(const std::array<VkDescriptorImageInfoEx, 68>& sampled_images);
	};
}
