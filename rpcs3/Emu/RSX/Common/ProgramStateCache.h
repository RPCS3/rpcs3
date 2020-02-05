#pragma once

#include "Emu/RSX/RSXFragmentProgram.h"
#include "Emu/RSX/RSXVertexProgram.h"

#include "Utilities/hash.h"
#include "Utilities/mutex.h"
#include "Utilities/Log.h"
#include "Utilities/span.h"

#include <deque>

enum class SHADER_TYPE
{
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_FRAGMENT
};

namespace program_hash_util
{
	// Based on
	// https://github.com/AlexAltea/nucleus/blob/master/nucleus/gpu/rsx_pgraph.cpp
	// TODO: eliminate it and implement independent hash utility
	union qword
	{
		u64 dword[2];
		u32 word[4];
	};

	struct vertex_program_utils
	{
		struct vertex_program_metadata
		{
			std::bitset<512> instruction_mask;
			u32 ucode_length;
			u32 referenced_textures_mask;
		};

		static size_t get_vertex_program_ucode_hash(const RSXVertexProgram &program);

		static vertex_program_metadata analyse_vertex_program(const u32* data, u32 entry, RSXVertexProgram& dst_prog);
	};

	struct vertex_program_storage_hash
	{
		size_t operator()(const RSXVertexProgram &program) const;
	};

	struct vertex_program_compare
	{
		bool operator()(const RSXVertexProgram &binary1, const RSXVertexProgram &binary2) const;
	};

	struct fragment_program_utils
	{
		struct fragment_program_metadata
		{
			u32 program_start_offset;
			u32 program_ucode_length;
			u32 program_constants_buffer_length;
			u16 referenced_textures_mask;
		};

		/**
		* returns true if the given source Operand is a constant
		*/
		static bool is_constant(u32 sourceOperand);

		static size_t get_fragment_program_ucode_size(void *ptr);

		static fragment_program_metadata analyse_fragment_program(void *ptr);

		static size_t get_fragment_program_ucode_hash(const RSXFragmentProgram &program);
	};

	struct fragment_program_storage_hash
	{
		size_t operator()(const RSXFragmentProgram &program) const;
	};

	struct fragment_program_compare
	{
		bool operator()(const RSXFragmentProgram &binary1, const RSXFragmentProgram &binary2) const;
	};
}


/**
* Cache for program help structure (blob, string...)
* The class is responsible for creating the object so the state only has to call getGraphicPipelineState
* Template argument is a struct which has the following type declaration :
* - a typedef VertexProgramData to a type that encapsulate vertex program info. It should provide an Id member.
* - a typedef FragmentProgramData to a types that encapsulate fragment program info. It should provide an Id member and a fragment constant offset vector.
* - a typedef PipelineData encapsulating monolithic program.
* - a typedef PipelineProperties to a type that encapsulate various state info relevant to program compilation (alpha test, primitive type,...)
* - a	typedef ExtraData type that will be passed to the buildProgram function.
* It should also contains the following function member :
* - static void recompile_fragment_program(RSXFragmentProgram *RSXFP, FragmentProgramData& fragmentProgramData, size_t ID);
* - static void recompile_vertex_program(RSXVertexProgram *RSXVP, VertexProgramData& vertexProgramData, size_t ID);
* - static PipelineData build_program(VertexProgramData &vertexProgramData, FragmentProgramData &fragmentProgramData, const PipelineProperties &pipelineProperties, const ExtraData& extraData);
* - static void validate_pipeline_properties(const VertexProgramData &vertexProgramData, const FragmentProgramData &fragmentProgramData, PipelineProperties& props);
*/
template<typename backend_traits>
class program_state_cache
{
	using pipeline_storage_type = typename backend_traits::pipeline_storage_type;
	using pipeline_properties = typename backend_traits::pipeline_properties;
	using vertex_program_type = typename backend_traits::vertex_program_type;
	using fragment_program_type = typename backend_traits::fragment_program_type;

	using binary_to_vertex_program = std::unordered_map<RSXVertexProgram, vertex_program_type, program_hash_util::vertex_program_storage_hash, program_hash_util::vertex_program_compare> ;
	using binary_to_fragment_program = std::unordered_map<RSXFragmentProgram, fragment_program_type, program_hash_util::fragment_program_storage_hash, program_hash_util::fragment_program_compare>;


	struct pipeline_key
	{
		u32 vertex_program_id;
		u32 fragment_program_id;
		pipeline_properties properties;
	};

	struct pipeline_key_hash
	{
		size_t operator()(const pipeline_key &key) const
		{
			size_t hashValue = 0;
			hashValue ^= rpcs3::hash_base<unsigned>(key.vertex_program_id);
			hashValue ^= rpcs3::hash_base<unsigned>(key.fragment_program_id);
			hashValue ^= rpcs3::hash_struct<pipeline_properties>(key.properties);
			return hashValue;
		}
	};

	struct pipeline_key_compare
	{
		bool operator()(const pipeline_key &key1, const pipeline_key &key2) const
		{
			return (key1.vertex_program_id == key2.vertex_program_id) && (key1.fragment_program_id == key2.fragment_program_id) && (key1.properties == key2.properties);
		}
	};

public:
	struct async_link_task_entry
	{
		const vertex_program_type& vp;
		const fragment_program_type& fp;
		pipeline_properties props;

		async_link_task_entry(const vertex_program_type& _V, const fragment_program_type& _F, pipeline_properties _P)
			: vp(_V), fp(_F), props(std::move(_P))
		{}
	};

	struct async_decompile_task_entry
	{
		RSXVertexProgram vp;
		RSXFragmentProgram fp;
		bool is_fp;

		std::vector<u8> tmp_cache;

		async_decompile_task_entry(RSXVertexProgram _V)
			: vp(std::move(_V)), is_fp(false)
		{
		}

		async_decompile_task_entry(const RSXFragmentProgram& _F)
			: fp(_F), is_fp(true)
		{
			tmp_cache.resize(fp.ucode_length);
			std::memcpy(tmp_cache.data(), fp.addr, fp.ucode_length);
			fp.addr = tmp_cache.data();
		}
	};

protected:
	shared_mutex m_vertex_mutex;
	shared_mutex m_fragment_mutex;
	shared_mutex m_pipeline_mutex;
	shared_mutex m_decompiler_mutex;

	atomic_t<size_t> m_next_id = 0;
	bool m_cache_miss_flag; // Set if last lookup did not find any usable cached programs
	bool m_program_compiled_flag; // Set if last lookup caused program to be linked

	binary_to_vertex_program m_vertex_shader_cache;
	binary_to_fragment_program m_fragment_shader_cache;
	std::unordered_map <pipeline_key, pipeline_storage_type, pipeline_key_hash, pipeline_key_compare> m_storage;

	std::unordered_map <pipeline_key, std::unique_ptr<async_link_task_entry>, pipeline_key_hash, pipeline_key_compare> m_link_queue;
	std::deque<async_decompile_task_entry> m_decompile_queue;

	vertex_program_type __null_vertex_program;
	fragment_program_type __null_fragment_program;
	pipeline_storage_type __null_pipeline_handle;

	/// bool here to inform that the program was preexisting.
	std::tuple<const vertex_program_type&, bool> search_vertex_program(const RSXVertexProgram& rsx_vp, bool force_load = true)
	{
		bool recompile = false;
		vertex_program_type* new_shader;
		{
			reader_lock lock(m_vertex_mutex);

			const auto& I = m_vertex_shader_cache.find(rsx_vp);
			if (I != m_vertex_shader_cache.end())
			{
				return std::forward_as_tuple(I->second, true);
			}

			if (!force_load)
			{
				return std::forward_as_tuple(__null_vertex_program, false);
			}

			rsx_log.notice("VP not found in buffer!");

			lock.upgrade();
			auto [it, inserted] = m_vertex_shader_cache.try_emplace(rsx_vp);
			new_shader = &(it->second);
			recompile = inserted;
		}

		if (recompile)
		{
			backend_traits::recompile_vertex_program(rsx_vp, *new_shader, m_next_id++);
		}

		return std::forward_as_tuple(*new_shader, false);
	}

	/// bool here to inform that the program was preexisting.
	std::tuple<const fragment_program_type&, bool> search_fragment_program(const RSXFragmentProgram& rsx_fp, bool force_load = true)
	{
		bool recompile = false;
		fragment_program_type* new_shader;
		void* fragment_program_ucode_copy;
		{
			reader_lock lock(m_fragment_mutex);

			const auto& I = m_fragment_shader_cache.find(rsx_fp);
			if (I != m_fragment_shader_cache.end())
			{
				return std::forward_as_tuple(I->second, true);
			}

			if (!force_load)
			{
				return std::forward_as_tuple(__null_fragment_program, false);
			}

			rsx_log.notice("FP not found in buffer!");
			fragment_program_ucode_copy = malloc(rsx_fp.ucode_length);

			verify("malloc() failed!" HERE), fragment_program_ucode_copy;
			std::memcpy(fragment_program_ucode_copy, rsx_fp.addr, rsx_fp.ucode_length);

			RSXFragmentProgram new_fp_key = rsx_fp;
			new_fp_key.addr = fragment_program_ucode_copy;

			lock.upgrade();
			auto [it, inserted] = m_fragment_shader_cache.try_emplace(new_fp_key);
			new_shader = &(it->second);
			recompile = inserted;
		}

		if (recompile)
		{
			backend_traits::recompile_fragment_program(rsx_fp, *new_shader, m_next_id++);
		}
		else
		{
			free(fragment_program_ucode_copy);
		}

		return std::forward_as_tuple(*new_shader, false);
	}

public:

	struct program_buffer_patch_entry
	{
		union
		{
			u32 hex_key;
			f32 fp_key;
		};

		union
		{
			u32 hex_value;
			f32 fp_value;
		};

		program_buffer_patch_entry() = default;

		program_buffer_patch_entry(f32& key, f32& value)
		{
			fp_key = key;
			fp_value = value;
		}

		program_buffer_patch_entry(u32& key, u32& value)
		{
			hex_key = key;
			hex_value = value;
		}

		bool test_and_set(f32 value, f32* dst) const
		{
			u32 hex = std::bit_cast<u32>(value);
			if ((hex & 0x7FFFFFFF) == (hex_key & 0x7FFFFFFF))
			{
				hex = (hex & ~0x7FFFFFF) | hex_value;
				*dst = std::bit_cast<f32>(hex);
				return true;
			}

			return false;
		}
	};

	struct
	{
		std::unordered_map<f32, program_buffer_patch_entry> db;

		void add(program_buffer_patch_entry& e)
		{
			db[e.fp_key] = e;
		}

		void add(f32& key, f32& value)
		{
			db[key] = { key, value };
		}

		void clear()
		{
			db.clear();
		}

		bool is_empty() const
		{
			return db.empty();
		}
	}
	patch_table;

public:
	program_state_cache() = default;
	~program_state_cache()
	{
		for (auto& pair : m_fragment_shader_cache)
		{
			free(pair.first.addr);
		}
	}

	// Returns 2 booleans.
	// First flag hints that there is more work to do (busy hint)
	// Second flag is true if at least one program has been linked successfully (sync hint)
	template<typename... Args>
	std::pair<bool, bool> async_update(u32 max_decompile_count, Args&& ...args)
	{
		// Decompile shaders and link one pipeline object per 'run'
		// NOTE: Linking is much slower than decompilation step, so always decompile at least 1 unit
		// TODO: Use try_lock instead
		bool busy = false;
		u32 count = 0;
		std::unique_ptr<async_decompile_task_entry> decompile_task;

		while (true)
		{
			{
				std::lock_guard lock(m_decompiler_mutex);
				if (m_decompile_queue.empty())
				{
					break;
				}
				else
				{
					decompile_task = std::make_unique<async_decompile_task_entry>(std::move(m_decompile_queue.front()));
					m_decompile_queue.pop_front();
				}
			}

			if (decompile_task->is_fp)
			{
				search_fragment_program(decompile_task->fp);
			}
			else
			{
				search_vertex_program(decompile_task->vp);
			}

			if (++count >= max_decompile_count)
			{
				// Allows configurable decompiler 'load'
				// Smaller unit count will release locks faster
				busy = true;
				break;
			}
		}

		async_link_task_entry* link_entry;
		pipeline_key key;
		{
			reader_lock lock(m_pipeline_mutex);
			if (!m_link_queue.empty())
			{
				auto It = m_link_queue.begin();
				link_entry = It->second.get();
				key = It->first;
			}
			else
			{
				return { busy, false };
			}
		}

		pipeline_storage_type pipeline = backend_traits::build_pipeline(link_entry->vp, link_entry->fp, link_entry->props, std::forward<Args>(args)...);
		rsx_log.success("New program compiled successfully");

		std::lock_guard lock(m_pipeline_mutex);
		m_storage[key] = std::move(pipeline);
		m_link_queue.erase(key);

		return { (busy || !m_link_queue.empty()), true };
	}

	template<typename... Args>
	pipeline_storage_type& get_graphics_pipeline(
		const RSXVertexProgram& vertexShader,
		const RSXFragmentProgram& fragmentShader,
		pipeline_properties& pipelineProperties,
		bool allow_async,
		Args&& ...args
		)
	{
		const auto &vp_search = search_vertex_program(vertexShader, !allow_async);
		const auto &fp_search = search_fragment_program(fragmentShader, !allow_async);

		const bool already_existing_fragment_program = std::get<1>(fp_search);
		const bool already_existing_vertex_program = std::get<1>(vp_search);

		bool link_only = false;
		m_cache_miss_flag = true;
		m_program_compiled_flag = false;

		if (!allow_async || (already_existing_vertex_program && already_existing_fragment_program))
		{
			const vertex_program_type &vertex_program = std::get<0>(vp_search);
			const fragment_program_type &fragment_program = std::get<0>(fp_search);

			backend_traits::validate_pipeline_properties(vertex_program, fragment_program, pipelineProperties);
			pipeline_key key = { vertex_program.id, fragment_program.id, pipelineProperties };

			{
				reader_lock lock(m_pipeline_mutex);
				const auto I = m_storage.find(key);
				if (I != m_storage.end())
				{
					m_cache_miss_flag = false;
					return I->second;
				}
			}

			if (allow_async)
			{
				// Programs already exist, only linking required
				link_only = true;
			}
			else
			{
				rsx_log.notice("Add program (vp id = %d, fp id = %d)", vertex_program.id, fragment_program.id);
				m_program_compiled_flag = true;

				pipeline_storage_type pipeline = backend_traits::build_pipeline(vertex_program, fragment_program, pipelineProperties, std::forward<Args>(args)...);
				std::lock_guard lock(m_pipeline_mutex);
				auto &rtn = m_storage[key] = std::move(pipeline);
				rsx_log.success("New program compiled successfully");
				return rtn;
			}
		}

		verify(HERE), allow_async;

		if (link_only)
		{
			const vertex_program_type &vertex_program = std::get<0>(vp_search);
			const fragment_program_type &fragment_program = std::get<0>(fp_search);
			pipeline_key key = { vertex_program.id, fragment_program.id, pipelineProperties };

			reader_lock lock(m_pipeline_mutex);

			if (m_link_queue.find(key) != m_link_queue.end())
			{
				// Already in queue
				return __null_pipeline_handle;
			}

			rsx_log.notice("Add program (vp id = %d, fp id = %d)", vertex_program.id, fragment_program.id);
			m_program_compiled_flag = true;

			lock.upgrade();
			m_link_queue[key] = std::make_unique<async_link_task_entry>(vertex_program, fragment_program, pipelineProperties);
		}
		else
		{
			reader_lock lock(m_decompiler_mutex);

			auto vertex_program_found = std::find_if(m_decompile_queue.begin(), m_decompile_queue.end(), [&](const auto& V)
			{
				if (V.is_fp) return false;
				return program_hash_util::vertex_program_compare()(V.vp, vertexShader);
			});

			auto fragment_program_found = std::find_if(m_decompile_queue.begin(), m_decompile_queue.end(), [&](const auto& F)
			{
				if (!F.is_fp) return false;
				return program_hash_util::fragment_program_compare()(F.fp, fragmentShader);
			});

			const bool add_vertex_program = (vertex_program_found == m_decompile_queue.end());
			const bool add_fragment_program = (fragment_program_found == m_decompile_queue.end());

			if (add_vertex_program)
			{
				lock.upgrade();
				m_decompile_queue.emplace_back(vertexShader);
			}

			if (add_fragment_program)
			{
				lock.upgrade();
				m_decompile_queue.emplace_back(fragmentShader);
			}
		}

		return __null_pipeline_handle;
	}

	void fill_fragment_constants_buffer(gsl::span<f32> dst_buffer, const RSXFragmentProgram &fragment_program, bool sanitize = false) const
	{
		const auto I = m_fragment_shader_cache.find(fragment_program);
		if (I == m_fragment_shader_cache.end())
			return;

		verify(HERE), (dst_buffer.size_bytes() >= ::narrow<int>(I->second.FragmentConstantOffsetCache.size()) * 16);

		f32* dst = dst_buffer.data();
		alignas(16) f32 tmp[4];
		for (size_t offset_in_fragment_program : I->second.FragmentConstantOffsetCache)
		{
			char* data = static_cast<char*>(fragment_program.addr) + offset_in_fragment_program;
			const __m128i vector = _mm_loadu_si128(reinterpret_cast<__m128i*>(data));
			const __m128i shuffled_vector = _mm_or_si128(_mm_slli_epi16(vector, 8), _mm_srli_epi16(vector, 8));

			if (!patch_table.is_empty())
			{
				_mm_store_ps(tmp, _mm_castsi128_ps(shuffled_vector));
				bool patched;

				for (int i = 0; i < 4; ++i)
				{
					patched = false;
					for (auto& e : patch_table.db)
					{
						//TODO: Use fp comparison with fabsf without hurting performance
						patched = e.second.test_and_set(tmp[i], &dst[i]);
						if (patched)
						{
							break;
						}
					}

					if (!patched)
					{
						dst[i] = tmp[i];
					}
				}
			}
			else if (sanitize)
			{
				//Convert NaNs and Infs to 0
				const auto masked = _mm_and_si128(shuffled_vector, _mm_set1_epi32(0x7fffffff));
				const auto valid = _mm_cmplt_epi32(masked, _mm_set1_epi32(0x7f800000));
				const auto result = _mm_and_si128(shuffled_vector, valid);
				_mm_stream_si128(std::bit_cast<__m128i*>(dst), result);
			}
			else
			{
				_mm_stream_si128(std::bit_cast<__m128i*>(dst), shuffled_vector);
			}

			dst += 4;
		}
	}

	void clear()
	{
		m_storage.clear();
	}
};
