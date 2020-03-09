#pragma once

#include "Emu/RSX/RSXFragmentProgram.h"
#include "Emu/RSX/RSXVertexProgram.h"

#include "Utilities/hash.h"
#include "Utilities/mutex.h"
#include "util/logs.hpp"
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

	struct async_decompiler_job
	{
		RSXVertexProgram vertex_program;
		RSXFragmentProgram fragment_program;
		pipeline_properties properties;

		std::vector<u8> local_storage;

		async_decompiler_job(RSXVertexProgram v, const RSXFragmentProgram f, pipeline_properties p) :
			vertex_program(std::move(v)), fragment_program(f), properties(std::move(p))
		{
			local_storage.resize(fragment_program.ucode_length);
			std::memcpy(local_storage.data(), fragment_program.addr, fragment_program.ucode_length);
			fragment_program.addr = local_storage.data();
		}
	};

protected:
	using decompiler_callback_t = std::function<void(const pipeline_properties&, const RSXVertexProgram&, const RSXFragmentProgram&)>;

	shared_mutex m_vertex_mutex;
	shared_mutex m_fragment_mutex;
	shared_mutex m_pipeline_mutex;
	shared_mutex m_decompiler_mutex;

	atomic_t<size_t> m_next_id = 0;
	bool m_cache_miss_flag; // Set if last lookup did not find any usable cached programs

	binary_to_vertex_program m_vertex_shader_cache;
	binary_to_fragment_program m_fragment_shader_cache;
	std::unordered_map<pipeline_key, pipeline_storage_type, pipeline_key_hash, pipeline_key_compare> m_storage;

	std::deque<async_decompiler_job> m_decompile_queue;
	std::unordered_map<pipeline_key, bool, pipeline_key_hash, pipeline_key_compare> m_decompiler_map;
	decompiler_callback_t notify_pipeline_compiled;

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
	{}

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
		bool sync = false;
		u32 count = 0;

		while (true)
		{
			{
				reader_lock lock(m_decompiler_mutex);
				if (m_decompile_queue.empty())
				{
					break;
				}
			}

			// Decompile
			const auto& vp_search = search_vertex_program(m_decompile_queue.front().vertex_program, true);
			const auto& fp_search = search_fragment_program(m_decompile_queue.front().fragment_program, true);

			const bool already_existing_fragment_program = std::get<1>(fp_search);
			const bool already_existing_vertex_program = std::get<1>(vp_search);
			const vertex_program_type& vertex_program = std::get<0>(vp_search);
			const fragment_program_type& fragment_program = std::get<0>(fp_search);
			const pipeline_key key = { vertex_program.id, fragment_program.id, m_decompile_queue.front().properties };

			// Retest
			bool found = false;
			if (already_existing_vertex_program && already_existing_fragment_program)
			{
				if (auto I = m_storage.find(key); I != m_storage.end())
				{
					found = true;
				}
			}

			if (!found)
			{
				pipeline_storage_type pipeline = backend_traits::build_pipeline(vertex_program, fragment_program, m_decompile_queue.front().properties, std::forward<Args>(args)...);
				rsx_log.success("New program compiled successfully");
				sync = true;

				if (notify_pipeline_compiled)
				{
					notify_pipeline_compiled(m_decompile_queue.front().properties, m_decompile_queue.front().vertex_program, m_decompile_queue.front().fragment_program);
				}

				std::scoped_lock lock(m_pipeline_mutex);
				m_storage[key] = std::move(pipeline);
			}

			{
				std::scoped_lock lock(m_decompiler_mutex);
				m_decompile_queue.pop_front();
				m_decompiler_map.erase(key);
			}

			if (++count >= max_decompile_count)
			{
				// Allows configurable decompiler 'load'
				// Smaller unit count will release locks faster
				busy = true;
				break;
			}
		}

		return { busy, sync };
	}

	template<typename... Args>
	pipeline_storage_type& get_graphics_pipeline(
		const RSXVertexProgram& vertexShader,
		const RSXFragmentProgram& fragmentShader,
		pipeline_properties& pipelineProperties,
		bool allow_async,
		bool allow_notification,
		Args&& ...args
		)
	{
		const auto &vp_search = search_vertex_program(vertexShader, !allow_async);
		const auto &fp_search = search_fragment_program(fragmentShader, !allow_async);

		const bool already_existing_fragment_program = std::get<1>(fp_search);
		const bool already_existing_vertex_program = std::get<1>(vp_search);
		const vertex_program_type& vertex_program = std::get<0>(vp_search);
		const fragment_program_type& fragment_program = std::get<0>(fp_search);
		const pipeline_key key = { vertex_program.id, fragment_program.id, pipelineProperties };

		m_cache_miss_flag = true;

		if (!allow_async || (already_existing_vertex_program && already_existing_fragment_program))
		{
			backend_traits::validate_pipeline_properties(vertex_program, fragment_program, pipelineProperties);

			{
				reader_lock lock(m_pipeline_mutex);
				if (const auto I = m_storage.find(key); I != m_storage.end())
				{
					m_cache_miss_flag = false;
					return I->second;
				}
			}

			if (!allow_async)
			{
				rsx_log.notice("Add program (vp id = %d, fp id = %d)", vertex_program.id, fragment_program.id);
				pipeline_storage_type pipeline = backend_traits::build_pipeline(vertex_program, fragment_program, pipelineProperties, std::forward<Args>(args)...);

				if (allow_notification && notify_pipeline_compiled)
				{
					notify_pipeline_compiled(pipelineProperties, vertexShader, fragmentShader);
					rsx_log.success("New program compiled successfully");
				}

				std::lock_guard lock(m_pipeline_mutex);
				auto &rtn = m_storage[key] = std::move(pipeline);
				return rtn;
			}
		}

		verify(HERE), allow_async;

		std::scoped_lock lock(m_decompiler_mutex, m_pipeline_mutex);

		// Rechecks
		if (already_existing_vertex_program && already_existing_fragment_program)
		{
			if (const auto I = m_storage.find(key); I != m_storage.end())
			{
				m_cache_miss_flag = false;
				return I->second;
			}

			if (const auto I = m_decompiler_map.find(key); I != m_decompiler_map.end())
			{
				// Already in queue
				return __null_pipeline_handle;
			}

			m_decompiler_map[key] = true;
		}

		// Enqueue if not already queued
		m_decompile_queue.emplace_back(vertexShader, fragmentShader, pipelineProperties);

		return __null_pipeline_handle;
	}

	void fill_fragment_constants_buffer(gsl::span<f32> dst_buffer, const RSXFragmentProgram &fragment_program, bool sanitize = false) const
	{
		const auto I = m_fragment_shader_cache.find(fragment_program);
		if (I == m_fragment_shader_cache.end())
			return;

		verify(HERE), (dst_buffer.size_bytes() >= ::narrow<int>(I->second.FragmentConstantOffsetCache.size()) * 16u);

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
		std::scoped_lock lock(m_vertex_mutex, m_fragment_mutex, m_decompiler_mutex, m_pipeline_mutex);

		for (auto& pair : m_fragment_shader_cache)
		{
			free(pair.first.addr);
		}

		notify_pipeline_compiled = {};
		m_fragment_shader_cache.clear();
		m_vertex_shader_cache.clear();
		m_storage.clear();
	}
};
