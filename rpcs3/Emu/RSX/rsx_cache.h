#pragma once
#include "Utilities/VirtualMemory.h"
#include "Utilities/hash.h"
#include "Utilities/File.h"
#include "Utilities/lockless.h"
#include "Emu/Memory/vm.h"
#include "gcm_enums.h"
#include "Common/ProgramStateCache.h"
#include "Emu/System.h"
#include "Common/texture_cache_checker.h"
#include "Overlays/Shaders/shader_loading_dialog.h"

#include "rsx_utils.h"
#include <thread>
#include <chrono>

namespace rsx
{
	enum protection_policy
	{
		protect_policy_one_page,     //Only guard one page, preferrably one where this section 'wholly' fits
		protect_policy_conservative, //Guards as much memory as possible that is guaranteed to only be covered by the defined range without sharing
		protect_policy_full_range    //Guard the full memory range. Shared pages may be invalidated by access outside the object we're guarding
	};

	enum section_bounds
	{
		full_range,
		locked_range,
		confirmed_range
	};

	static inline void memory_protect(const address_range& range, utils::protection prot)
	{
		verify(HERE), range.is_page_range();

		//rsx_log.error("memory_protect(0x%x, 0x%x, %x)", static_cast<u32>(range.start), static_cast<u32>(range.length()), static_cast<u32>(prot));
		utils::memory_protect(vm::base(range.start), range.length(), prot);

#ifdef TEXTURE_CACHE_DEBUG
		tex_cache_checker.set_protection(range, prot);
#endif
	}

	class buffered_section
	{
	public:
		static const protection_policy guard_policy = protect_policy_full_range;

	private:
		address_range locked_range;
		address_range cpu_range = {};
		address_range confirmed_range;

		utils::protection protection = utils::protection::rw;

		bool locked = false;

		inline void init_lockable_range(const address_range &range)
		{
			locked_range = range.to_page_range();

			if ((guard_policy != protect_policy_full_range) && (range.length() >= 4096))
			{
				const u32 block_start = (locked_range.start < range.start) ? (locked_range.start + 4096u) : locked_range.start;
				const u32 block_end = locked_range.end;

				if (block_start < block_end)
				{
					// protect unique page range
					locked_range.start = block_start;
					locked_range.end = block_end;
				}

				if (guard_policy == protect_policy_one_page)
				{
					// protect exactly one page
					locked_range.set_length(4096u);
				}
			}

			AUDIT( (locked_range.start == page_start(range.start)) || (locked_range.start == next_page(range.start)) );
			AUDIT( locked_range.end <= page_end(range.end) );
			verify(HERE), locked_range.is_page_range();
		}

	public:

		buffered_section() = default;
		~buffered_section() = default;

		void reset(const address_range &memory_range)
		{
			verify(HERE), memory_range.valid() && locked == false;

			cpu_range = address_range(memory_range);
			confirmed_range.invalidate();
			locked_range.invalidate();

			protection = utils::protection::rw;
			locked = false;

			init_lockable_range(cpu_range);
		}

	protected:
		void invalidate_range()
		{
			ASSERT(!locked);

			cpu_range.invalidate();
			confirmed_range.invalidate();
			locked_range.invalidate();
		}

	public:
		void protect(utils::protection new_prot, bool force = false)
		{
			if (new_prot == protection && !force) return;

			verify(HERE), locked_range.is_page_range();
			AUDIT( !confirmed_range.valid() || confirmed_range.inside(cpu_range) );

#ifdef TEXTURE_CACHE_DEBUG
			if (new_prot != protection || force)
			{
				if (locked && !force) // When force=true, it is the responsibility of the caller to remove this section from the checker refcounting
					tex_cache_checker.remove(locked_range, protection);
				if (new_prot != utils::protection::rw)
					tex_cache_checker.add(locked_range, new_prot);
			}
#endif // TEXTURE_CACHE_DEBUG

			rsx::memory_protect(locked_range, new_prot);
			protection = new_prot;
			locked = (protection != utils::protection::rw);

			if (protection == utils::protection::no)
			{
				tag_memory();
			}
			else
			{
				if (!locked)
				{
					//Unprotect range also invalidates secured range
					confirmed_range.invalidate();
				}
			}

		}

		void protect(utils::protection prot, const std::pair<u32, u32>& new_confirm)
		{
			// new_confirm.first is an offset after cpu_range.start
			// new_confirm.second is the length (after cpu_range.start + new_confirm.first)

#ifdef TEXTURE_CACHE_DEBUG
			// We need to remove the lockable range from page_info as we will be re-protecting with force==true
			if (locked)
				tex_cache_checker.remove(locked_range, protection);
#endif

			if (prot != utils::protection::rw)
			{
				if (confirmed_range.valid())
				{
					confirmed_range.start = std::min(confirmed_range.start, cpu_range.start + new_confirm.first);
					confirmed_range.end = std::max(confirmed_range.end, cpu_range.start + new_confirm.first + new_confirm.second - 1);
				}
				else
				{
					confirmed_range = address_range::start_length(cpu_range.start + new_confirm.first, new_confirm.second);
					ASSERT(!locked || locked_range.inside(confirmed_range.to_page_range()));
				}

				verify(HERE), confirmed_range.inside(cpu_range);
				init_lockable_range(confirmed_range);
			}

			protect(prot, true);
		}

		inline void unprotect()
		{
			AUDIT(protection != utils::protection::rw);
			protect(utils::protection::rw);
		}

		inline void discard()
		{
#ifdef TEXTURE_CACHE_DEBUG
			if (locked)
				tex_cache_checker.remove(locked_range, protection);
#endif

			protection = utils::protection::rw;
			confirmed_range.invalidate();
			locked = false;
		}

		inline const address_range& get_bounds(section_bounds bounds) const
		{
			switch (bounds)
			{
			case section_bounds::full_range:
				return cpu_range;
			case section_bounds::locked_range:
				return locked_range;
			case section_bounds::confirmed_range:
				return confirmed_range.valid() ? confirmed_range : cpu_range;
			default:
				ASSUME(0);
			}
		}


		/**
		 * Overlapping checks
		 */
		inline bool overlaps(const u32 address, section_bounds bounds) const
		{
			return get_bounds(bounds).overlaps(address);
		}

		inline bool overlaps(const address_range &other, section_bounds bounds) const
		{
			return get_bounds(bounds).overlaps(other);
		}

		inline bool overlaps(const address_range_vector &other, section_bounds bounds) const
		{
			return get_bounds(bounds).overlaps(other);
		}

		inline bool overlaps(const buffered_section &other, section_bounds bounds) const
		{
			return get_bounds(bounds).overlaps(other.get_bounds(bounds));
		}

		inline bool inside(const address_range &other, section_bounds bounds) const
		{
			return get_bounds(bounds).inside(other);
		}

		inline bool inside(const address_range_vector &other, section_bounds bounds) const
		{
			return get_bounds(bounds).inside(other);
		}

		inline bool inside(const buffered_section &other, section_bounds bounds) const
		{
			return get_bounds(bounds).inside(other.get_bounds(bounds));
		}

		inline s32 signed_distance(const address_range &other, section_bounds bounds) const
		{
			return get_bounds(bounds).signed_distance(other);
		}

		inline u32 distance(const address_range &other, section_bounds bounds) const
		{
			return get_bounds(bounds).distance(other);
		}

		/**
		* Utilities
		*/
		inline bool valid_range() const
		{
			return cpu_range.valid();
		}

		inline bool is_locked() const
		{
			return locked;
		}

		inline u32 get_section_base() const
		{
			return cpu_range.start;
		}

		inline u32 get_section_size() const
		{
			return cpu_range.valid() ? cpu_range.length() : 0;
		}

		inline const address_range& get_locked_range() const
		{
			AUDIT( locked );
			return locked_range;
		}

		inline const address_range& get_section_range() const
		{
			return cpu_range;
		}

		const address_range& get_confirmed_range() const
		{
			return confirmed_range.valid() ? confirmed_range : cpu_range;
		}

		const std::pair<u32, u32> get_confirmed_range_delta() const
		{
			if (!confirmed_range.valid())
				return { 0, cpu_range.length() };

			return { confirmed_range.start - cpu_range.start, confirmed_range.length() };
		}

		inline bool matches(const address_range &range) const
		{
			return cpu_range.valid() && cpu_range == range;
		}

		inline utils::protection get_protection() const
		{
			return protection;
		}

		inline address_range get_min_max(const address_range& current_min_max, section_bounds bounds) const
		{
			return get_bounds(bounds).get_min_max(current_min_max);
		}

		/**
		 * Super Pointer
		 */
		template <typename T = void>
		inline T* get_ptr(u32 address) const
		{
			return reinterpret_cast<T*>(vm::g_sudo_addr + address);
		}

		/**
		 * Memory tagging
		 */
	private:
		inline void tag_memory()
		{
			// We only need to tag memory if we are in full-range mode
			if (guard_policy == protect_policy_full_range)
				return;

			AUDIT(locked);

			const address_range& range = get_confirmed_range();

			volatile u32* first = get_ptr<volatile u32>(range.start);
			volatile u32* last = get_ptr<volatile u32>(range.end - 3);

			*first = range.start;
			*last = range.end;
		}

	public:
		bool test_memory_head()
		{
			if (guard_policy == protect_policy_full_range)
				return true;

			AUDIT(locked);

			const auto& range = get_confirmed_range();
			volatile const u32* first = get_ptr<volatile const u32>(range.start);
			return (*first == range.start);
		}

		bool test_memory_tail()
		{
			if (guard_policy == protect_policy_full_range)
				return true;

			AUDIT(locked);

			const auto& range = get_confirmed_range();
			volatile const u32* last = get_ptr<volatile const u32>(range.end-3);
			return (*last == range.end);
		}
	};



	template <typename pipeline_storage_type, typename backend_storage>
	class shaders_cache
	{
		using unpacked_type = lf_fifo<std::tuple<pipeline_storage_type, RSXVertexProgram, RSXFragmentProgram>, 1000>; // TODO: Determine best size

		struct pipeline_data
		{
			u64 vertex_program_hash;
			u64 fragment_program_hash;
			u64 pipeline_storage_hash;

			u32 vp_ctrl;
			u32 vp_texture_dimensions;
			u64 vp_instruction_mask[8];

			u32 vp_base_address;
			u32 vp_entry;
			u16 vp_jump_table[32];

			u32 fp_ctrl;
			u32 fp_texture_dimensions;
			u32 fp_texcoord_control;
			u16 fp_unnormalized_coords;
			u16 fp_height;
			u16 fp_pixel_layout;
			u16 fp_lighting_flags;
			u16 fp_shadow_textures;
			u16 fp_redirected_textures;
			u16 fp_alphakill_mask;
			u64 fp_zfunc_mask;

			pipeline_storage_type pipeline_properties;
		};

		std::string version_prefix;
		std::string root_path;
		std::string pipeline_class_name;
		std::mutex fpd_mutex;
		std::unordered_map<u64, std::vector<u8>> fragment_program_data;

		backend_storage& m_storage;

		std::string get_message(u32 index, u32 processed, u32 entry_count)
		{
			return fmt::format("%s pipeline object %u of %u", index == 0 ? "Loading" : "Compiling", processed, entry_count);
		};

		void load_shaders(uint nb_workers, unpacked_type& unpacked, std::string& directory_path, std::vector<fs::dir_entry>& entries, u32 entry_count,
		    shader_loading_dialog* dlg)
		{
			atomic_t<u32> processed(0);

			std::function<void(u32)> shader_load_worker = [&](u32 stop_at)
			{
				u32 pos;
				while (((pos = processed++) < stop_at) && !Emu.IsStopped())
				{
					fs::dir_entry tmp = entries[pos];

					const auto filename = directory_path + "/" + tmp.name;
					std::vector<u8> bytes;
					fs::file f(filename);
					if (f.size() != sizeof(pipeline_data))
					{
						rsx_log.error("Removing cached pipeline object %s since it's not binary compatible with the current shader cache", tmp.name.c_str());
						fs::remove_file(filename);
						continue;
					}
					f.read<u8>(bytes, f.size());

					auto entry = unpack(*reinterpret_cast<pipeline_data*>(bytes.data()));
					m_storage.preload_programs(std::get<1>(entry), std::get<2>(entry));

					unpacked[unpacked.push_begin()] = entry;
				}
			};

			await_workers(nb_workers, 0, shader_load_worker, processed, entry_count, dlg);
		}

		template <typename... Args>
		void compile_shaders(uint nb_workers, unpacked_type& unpacked, u32 entry_count, shader_loading_dialog* dlg, Args&&... args)
		{
			atomic_t<u32> processed(0);

			std::function<void(u32)> shader_comp_worker = [&](u32 stop_at)
			{
				u32 pos;
				while (((pos = processed++) < stop_at) && !Emu.IsStopped())
				{
					auto& entry = unpacked[pos];
					m_storage.add_pipeline_entry(std::get<1>(entry), std::get<2>(entry), std::get<0>(entry), std::forward<Args>(args)...);
				}
			};

			await_workers(nb_workers, 1, shader_comp_worker, processed, entry_count, dlg);
		}

		void await_workers(uint nb_workers, u8 step, std::function<void(u32)>& worker, atomic_t<u32>& processed, u32 entry_count, shader_loading_dialog* dlg)
		{
			u32 processed_since_last_update = 0;

			if (nb_workers == 1)
			{
				std::chrono::time_point<steady_clock> last_update;

				// Call the worker function directly, stoping it prematurely to be able update the screen
				u8 inc = 10;
				u32 stop_at = 0;
				do
				{
					stop_at = std::min(stop_at + inc, entry_count);

					worker(stop_at);

					// Only update the screen at about 10fps since updating it everytime slows down the process
					std::chrono::time_point<steady_clock> now = std::chrono::steady_clock::now();
					processed_since_last_update += inc;
					if ((std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update) > 100ms) || (stop_at == entry_count))
					{
						dlg->update_msg(step, get_message(step, stop_at, entry_count));
						dlg->inc_value(step, processed_since_last_update);
						last_update = now;
						processed_since_last_update = 0;
					}
				} while (stop_at < entry_count && !Emu.IsStopped());
			}
			else
			{
				std::vector<std::thread> worker_threads(nb_workers);

				// Start workers
				for (u32 i = 0; i < nb_workers; i++)
				{
					worker_threads[i] = std::thread(worker, entry_count);
				}

				u32 current_progress = 0;
				u32 last_update_progress = 0;
				while ((current_progress < entry_count) && !Emu.IsStopped())
				{
					std::this_thread::sleep_for(100ms); // Around 10fps should be good enough

					current_progress = std::min(processed.load(), entry_count);
					processed_since_last_update = current_progress - last_update_progress;
					last_update_progress = current_progress;

					if (processed_since_last_update > 0)
					{
						dlg->update_msg(step, get_message(step, current_progress, entry_count));
						dlg->inc_value(step, processed_since_last_update);
					}
				}

				for (std::thread& worker_thread : worker_threads)
				{
					worker_thread.join();
				}
			}
		}

	public:

		shaders_cache(backend_storage& storage, std::string pipeline_class, std::string version_prefix_str = "v1")
			: version_prefix(std::move(version_prefix_str))
			, pipeline_class_name(std::move(pipeline_class))
			, m_storage(storage)
		{
			if (!g_cfg.video.disable_on_disk_shader_cache)
			{
				root_path = Emu.PPUCache() + "shaders_cache";
			}
		}

		template <typename... Args>
		void load(shader_loading_dialog* dlg, Args&& ...args)
		{
			if (g_cfg.video.disable_on_disk_shader_cache)
			{
				return;
			}

			std::string directory_path = root_path + "/pipelines/" + pipeline_class_name + "/" + version_prefix;

			if (!fs::is_dir(directory_path))
			{
				fs::create_path(directory_path);
				fs::create_path(root_path + "/raw");

				return;
			}

			fs::dir root = fs::dir(directory_path);

			u32 entry_count = 0;
			std::vector<fs::dir_entry> entries;
			for (auto It = root.begin(); It != root.end(); ++It, entry_count++)
			{
				fs::dir_entry tmp = *It;

				if (tmp.name == "." || tmp.name == "..")
					continue;

				entries.push_back(tmp);
			}

			if ((entry_count = ::size32(entries)) <= 2)
				return;

			root.rewind();

			// Progress dialog
			std::unique_ptr<shader_loading_dialog> fallback_dlg;
			if (!dlg)
			{
				fallback_dlg = std::make_unique<shader_loading_dialog>();
				dlg = fallback_dlg.get();
			}

			dlg->create("Preloading cached shaders from disk.\nPlease wait...", "Shader Compilation");
			dlg->set_limit(0, entry_count);
			dlg->set_limit(1, entry_count);
			dlg->update_msg(0, get_message(0, 0, entry_count));
			dlg->update_msg(1, get_message(1, 0, entry_count));

			// Preload everything needed to compile the shaders
			unpacked_type unpacked;
			uint nb_workers = g_cfg.video.renderer == video_renderer::vulkan ? std::thread::hardware_concurrency() : 1;

			load_shaders(nb_workers, unpacked, directory_path, entries, entry_count, dlg);

			// Account for any invalid entries
			entry_count = unpacked.size();

			compile_shaders(nb_workers, unpacked, entry_count, dlg, std::forward<Args>(args)...);

			dlg->refresh();
			dlg->close();
		}

		void store(pipeline_storage_type &pipeline, RSXVertexProgram &vp, RSXFragmentProgram &fp)
		{
			if (g_cfg.video.disable_on_disk_shader_cache)
			{
				return;
			}

			if (vp.jump_table.size() > 32)
			{
				rsx_log.error("shaders_cache: vertex program has more than 32 jump addresses. Entry not saved to cache");
				return;
			}

			pipeline_data data = pack(pipeline, vp, fp);
			std::string fp_name = root_path + "/raw/" + fmt::format("%llX.fp", data.fragment_program_hash);
			std::string vp_name = root_path + "/raw/" + fmt::format("%llX.vp", data.vertex_program_hash);

			if (!fs::is_file(fp_name))
			{
				fs::file(fp_name, fs::rewrite).write(fp.addr, fp.ucode_length);
			}

			if (!fs::is_file(vp_name))
			{
				fs::file(vp_name, fs::rewrite).write<u32>(vp.data);
			}

			u64 state_hash = 0;
			state_hash ^= rpcs3::hash_base<u32>(data.vp_ctrl);
			state_hash ^= rpcs3::hash_base<u32>(data.fp_ctrl);
			state_hash ^= rpcs3::hash_base<u32>(data.vp_texture_dimensions);
			state_hash ^= rpcs3::hash_base<u32>(data.fp_texture_dimensions);
			state_hash ^= rpcs3::hash_base<u32>(data.fp_texcoord_control);
			state_hash ^= rpcs3::hash_base<u16>(data.fp_unnormalized_coords);
			state_hash ^= rpcs3::hash_base<u16>(data.fp_height);
			state_hash ^= rpcs3::hash_base<u16>(data.fp_pixel_layout);
			state_hash ^= rpcs3::hash_base<u16>(data.fp_lighting_flags);
			state_hash ^= rpcs3::hash_base<u16>(data.fp_shadow_textures);
			state_hash ^= rpcs3::hash_base<u16>(data.fp_redirected_textures);
			state_hash ^= rpcs3::hash_base<u16>(data.fp_alphakill_mask);
			state_hash ^= rpcs3::hash_base<u64>(data.fp_zfunc_mask);

			std::string pipeline_file_name = fmt::format("%llX+%llX+%llX+%llX.bin", data.vertex_program_hash, data.fragment_program_hash, data.pipeline_storage_hash, state_hash);
			std::string pipeline_path = root_path + "/pipelines/" + pipeline_class_name + "/" + version_prefix + "/" + pipeline_file_name;
			fs::file(pipeline_path, fs::rewrite).write(&data, sizeof(pipeline_data));
		}

		RSXVertexProgram load_vp_raw(u64 program_hash)
		{
			std::vector<u32> data;
			std::string filename = fmt::format("%llX.vp", program_hash);

			fs::file f(root_path + "/raw/" + filename);
			f.read<u32>(data, f.size() / sizeof(u32));

			RSXVertexProgram vp = {};
			vp.data = data;
			vp.skip_vertex_input_check = true;

			return vp;
		}

		RSXFragmentProgram load_fp_raw(u64 program_hash)
		{
			std::vector<u8> data;
			std::string filename = fmt::format("%llX.fp", program_hash);

			fs::file f(root_path + "/raw/" + filename);
			f.read<u8>(data, f.size());

			RSXFragmentProgram fp = {};
			{
				std::lock_guard<std::mutex> lock(fpd_mutex);
				fragment_program_data[program_hash] = data;
				fp.addr                             = fragment_program_data[program_hash].data();
			}
			fp.ucode_length = ::size32(data);

			return fp;
		}

		std::tuple<pipeline_storage_type, RSXVertexProgram, RSXFragmentProgram> unpack(pipeline_data &data)
		{
			RSXVertexProgram vp = load_vp_raw(data.vertex_program_hash);
			RSXFragmentProgram fp = load_fp_raw(data.fragment_program_hash);
			pipeline_storage_type pipeline = data.pipeline_properties;

			vp.output_mask = data.vp_ctrl;
			vp.texture_dimensions = data.vp_texture_dimensions;
			vp.base_address = data.vp_base_address;
			vp.entry = data.vp_entry;

			pack_bitset<512>(vp.instruction_mask, data.vp_instruction_mask);

			for (u8 index = 0; index < 32; ++index)
			{
				const auto address = data.vp_jump_table[index];
				if (address == UINT16_MAX)
				{
					// End of list marker
					break;
				}

				vp.jump_table.emplace(address);
			}

			fp.ctrl = data.fp_ctrl;
			fp.texture_dimensions = data.fp_texture_dimensions;
			fp.texcoord_control_mask = data.fp_texcoord_control;
			fp.unnormalized_coords = data.fp_unnormalized_coords;
			fp.two_sided_lighting = !!(data.fp_lighting_flags & 0x1);
			fp.shadow_textures = data.fp_shadow_textures;
			fp.redirected_textures = data.fp_redirected_textures;

			for (u8 index = 0; index < 16; ++index)
			{
				fp.textures_alpha_kill[index] = (data.fp_alphakill_mask & (1 << index))? 1: 0;
				fp.textures_zfunc[index] = (data.fp_zfunc_mask >> (index << 2)) & 0xF;
			}

			return std::make_tuple(pipeline, vp, fp);
		}

		pipeline_data pack(pipeline_storage_type &pipeline, RSXVertexProgram &vp, RSXFragmentProgram &fp)
		{
			pipeline_data data_block = {};
			data_block.pipeline_properties = pipeline;
			data_block.vertex_program_hash = m_storage.get_hash(vp);
			data_block.fragment_program_hash = m_storage.get_hash(fp);
			data_block.pipeline_storage_hash = m_storage.get_hash(pipeline);

			data_block.vp_ctrl = vp.output_mask;
			data_block.vp_texture_dimensions = vp.texture_dimensions;
			data_block.vp_base_address = vp.base_address;
			data_block.vp_entry = vp.entry;

			unpack_bitset<512>(vp.instruction_mask, data_block.vp_instruction_mask);

			u8 index = 0;
			while (index < 32)
			{
				if (!index && !vp.jump_table.empty())
				{
					for (auto &address : vp.jump_table)
					{
						data_block.vp_jump_table[index++] = static_cast<u16>(address);
					}
				}
				else
				{
					// End of list marker
					data_block.vp_jump_table[index] = UINT16_MAX;
					break;
				}
			}

			data_block.fp_ctrl = fp.ctrl;
			data_block.fp_texture_dimensions = fp.texture_dimensions;
			data_block.fp_texcoord_control = fp.texcoord_control_mask;
			data_block.fp_unnormalized_coords = fp.unnormalized_coords;
			data_block.fp_lighting_flags = u16(fp.two_sided_lighting);
			data_block.fp_shadow_textures = fp.shadow_textures;
			data_block.fp_redirected_textures = fp.redirected_textures;

			for (u8 index = 0; index < 16; ++index)
			{
				data_block.fp_alphakill_mask |= u32(fp.textures_alpha_kill[index] & 0x1) << index;
				data_block.fp_zfunc_mask |= u64(fp.textures_zfunc[index] & 0xF) << (index << 2);
			}

			return data_block;
		}
	};

	namespace vertex_cache
	{
		// A null vertex cache
		template <typename storage_type, typename upload_format>
		class default_vertex_cache
		{
		public:
			virtual ~default_vertex_cache() = default;
			virtual storage_type* find_vertex_range(uintptr_t /*local_addr*/, upload_format, u32 /*data_length*/) { return nullptr; }
			virtual void store_range(uintptr_t /*local_addr*/, upload_format, u32 /*data_length*/, u32 /*offset_in_heap*/) {}
			virtual void purge() {}
		};

		// A weak vertex cache with no data checks or memory range locks
		// Of limited use since contents are only guaranteed to be valid once per frame
		// TODO: Strict vertex cache with range locks
		template <typename upload_format>
		struct uploaded_range
		{
			uintptr_t local_address;
			upload_format buffer_format;
			u32 offset_in_heap;
			u32 data_length;
		};

		template <typename upload_format>
		class weak_vertex_cache : public default_vertex_cache<uploaded_range<upload_format>, upload_format>
		{
			using storage_type = uploaded_range<upload_format>;

		private:
			std::unordered_map<uintptr_t, std::vector<storage_type>> vertex_ranges;

		public:

			storage_type* find_vertex_range(uintptr_t local_addr, upload_format fmt, u32 data_length) override
			{
				const auto data_end = local_addr + data_length;

				for (auto &v : vertex_ranges[local_addr])
				{
					// NOTE: This has to match exactly. Using sized shortcuts such as >= comparison causes artifacting in some applications (UC1)
					if (v.buffer_format == fmt && v.data_length == data_length)
						return &v;
				}

				return nullptr;
			}

			void store_range(uintptr_t local_addr, upload_format fmt, u32 data_length, u32 offset_in_heap) override
			{
				storage_type v = {};
				v.buffer_format = fmt;
				v.data_length = data_length;
				v.local_address = local_addr;
				v.offset_in_heap = offset_in_heap;

				vertex_ranges[local_addr].push_back(v);
			}

			void purge() override
			{
				vertex_ranges.clear();
			}
		};
	}
}
