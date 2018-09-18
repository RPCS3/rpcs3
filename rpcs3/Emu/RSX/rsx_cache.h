#pragma once
#include "Utilities/VirtualMemory.h"
#include "Utilities/hash.h"
#include "Emu/Memory/vm.h"
#include "gcm_enums.h"
#include "Common/ProgramStateCache.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/System.h"

#include "rsx_utils.h"
#include <thread>

namespace rsx
{
	enum protection_policy
	{
		protect_policy_one_page,     //Only guard one page, preferrably one where this section 'wholly' fits
		protect_policy_conservative, //Guards as much memory as possible that is guaranteed to only be covered by the defined range without sharing
		protect_policy_full_range    //Guard the full memory range. Shared pages may be invalidated by access outside the object we're guarding
	};

	enum overlap_test_bounds
	{
		full_range,
		protected_range,
		confirmed_range
	};

	class buffered_section
	{
	private:
		u32 locked_address_base = 0;
		u32 locked_address_range = 0;
		weak_ptr locked_memory_ptr;
		std::pair<u32, u32> confirmed_range;

		inline void tag_memory()
		{
			if (locked_memory_ptr)
			{
				const u32 valid_limit = (confirmed_range.second) ? confirmed_range.first + confirmed_range.second : cpu_address_range;
				u32* first = locked_memory_ptr.get<u32>(confirmed_range.first, true);
				u32* last = locked_memory_ptr.get<u32>(valid_limit - 4, true);

				*first = cpu_address_base + confirmed_range.first;
				*last = cpu_address_base + valid_limit - 4;

				locked_memory_ptr.flush(confirmed_range.first, 4);
				locked_memory_ptr.flush(valid_limit - 4, 4);
			}
		}

	protected:
		u32 cpu_address_base = 0;
		u32 cpu_address_range = 0;

		utils::protection protection = utils::protection::rw;
		protection_policy guard_policy;

		bool locked = false;
		bool dirty = false;

		inline void init_lockable_range(u32 base, u32 length)
		{
			locked_address_base = (base & ~4095);

			if ((guard_policy != protect_policy_full_range) && (length >= 4096))
			{
				const u32 limit = base + length;
				const u32 block_end = (limit & ~4095);
				const u32 block_start = (locked_address_base < base) ? (locked_address_base + 4096) : locked_address_base;

				locked_address_range = 4096;

				if (block_start < block_end)
				{
					//Page boundaries cover at least one unique page
					locked_address_base = block_start;

					if (guard_policy == protect_policy_conservative)
					{
						//Protect full unique range
						locked_address_range = (block_end - block_start);
					}
				}
			}
			else
				locked_address_range = align(base + length, 4096) - locked_address_base;

			verify(HERE), locked_address_range > 0;
		}

	public:

		buffered_section() {}
		~buffered_section() {}

		void reset(u32 base, u32 length, protection_policy protect_policy = protect_policy_full_range)
		{
			verify(HERE), locked == false;

			cpu_address_base = base;
			cpu_address_range = length;

			confirmed_range = { 0, 0 };
			protection = utils::protection::rw;
			guard_policy = protect_policy;
			locked = false;

			init_lockable_range(cpu_address_base, cpu_address_range);
		}

		void protect(utils::protection prot, bool force = false)
		{
			if (prot == protection && !force) return;

			verify(HERE), locked_address_range > 0;
			utils::memory_protect(vm::base(locked_address_base), locked_address_range, prot);
			protection = prot;
			locked = prot != utils::protection::rw;

			if (prot == utils::protection::no)
			{
				locked_memory_ptr = rsx::get_super_ptr(cpu_address_base, cpu_address_range);
				tag_memory();
			}
			else
			{
				if (!locked)
				{
					//Unprotect range also invalidates secured range
					confirmed_range = { 0, 0 };
				}

				locked_memory_ptr = {};
			}
		}

		void protect(utils::protection prot, const std::pair<u32, u32>& range_confirm)
		{
			if (prot != utils::protection::rw)
			{
				const auto old_prot = protection;
				const auto old_locked_base = locked_address_base;
				const auto old_locked_length = locked_address_range;

				if (confirmed_range.second)
				{
					const u32 range_limit = std::max(range_confirm.first + range_confirm.second, confirmed_range.first + confirmed_range.second);
					confirmed_range.first = std::min(confirmed_range.first, range_confirm.first);
					confirmed_range.second = range_limit - confirmed_range.first;
				}
				else
				{
					confirmed_range = range_confirm;
				}

				init_lockable_range(confirmed_range.first + cpu_address_base, confirmed_range.second);
			}

			protect(prot, true);
		}

		void unprotect()
		{
			protect(utils::protection::rw);
		}

		void discard()
		{
			protection = utils::protection::rw;
			dirty = true;
			locked = false;

			confirmed_range = { 0, 0 };
			locked_memory_ptr = {};
		}

		/**
		* Check if range overlaps with this section.
		* ignore_protection_range - if true, the test should not check against the aligned protection range, instead
		* tests against actual range of contents in memory
		*/
		bool overlaps(std::pair<u32, u32> range) const
		{
			return region_overlaps(locked_address_base, locked_address_base + locked_address_range, range.first, range.first + range.second);
		}

		bool overlaps(u32 address, overlap_test_bounds bounds) const
		{
			switch (bounds)
			{
			case overlap_test_bounds::full_range:
			{
				return (cpu_address_base <= address && (address - cpu_address_base) < cpu_address_range);
			}
			case overlap_test_bounds::protected_range:
			{
				return (locked_address_base <= address && (address - locked_address_base) < locked_address_range);
			}
			case overlap_test_bounds::confirmed_range:
			{
				const auto range = get_confirmed_range();
				return ((range.first + cpu_address_base) <= address && (address - range.first) < range.second);
			}
			default:
				fmt::throw_exception("Unreachable" HERE);
			}
		}

		bool overlaps(const std::pair<u32, u32>& range, overlap_test_bounds bounds) const
		{
			switch (bounds)
			{
			case overlap_test_bounds::full_range:
			{
				return region_overlaps(cpu_address_base, cpu_address_base + cpu_address_range, range.first, range.first + range.second);
			}
			case overlap_test_bounds::protected_range:
			{
				return region_overlaps(locked_address_base, locked_address_base + locked_address_range, range.first, range.first + range.second);
			}
			case overlap_test_bounds::confirmed_range:
			{
				const auto test_range = get_confirmed_range();
				return region_overlaps(test_range.first + cpu_address_base, test_range.first + cpu_address_base + test_range.second, range.first, range.first + range.second);
			}
			default:
				fmt::throw_exception("Unreachable" HERE);
			}
		}

		/**
		 * Check if the page containing the address tramples this section. Also compares a former trampled page range to compare
		 * If true, returns the range <min, max> with updated invalid range
		 */
		std::tuple<bool, std::pair<u32, u32>> overlaps_page(const std::pair<u32, u32>& old_range, u32 address, overlap_test_bounds bounds) const
		{
			const u32 page_base = address & ~4095;
			const u32 page_limit = page_base + 4096;

			const u32 compare_min = std::min(old_range.first, page_base);
			const u32 compare_max = std::max(old_range.second, page_limit);

			u32 memory_base, memory_range;
			switch (bounds)
			{
			case overlap_test_bounds::full_range:
			{
				memory_base = (cpu_address_base & ~4095);
				memory_range = align(cpu_address_base + cpu_address_range, 4096u) - memory_base;
				break;
			}
			case overlap_test_bounds::protected_range:
			{
				memory_base = locked_address_base;
				memory_range = locked_address_range;
				break;
			}
			case overlap_test_bounds::confirmed_range:
			{
				const auto range = get_confirmed_range();
				memory_base = (cpu_address_base + range.first) & ~4095;
				memory_range = align(cpu_address_base + range.first + range.second, 4096u) - memory_base;
				break;
			}
			default:
				fmt::throw_exception("Unreachable" HERE);
			}

			if (!region_overlaps(memory_base, memory_base + memory_range, compare_min, compare_max))
				return std::make_tuple(false, old_range);

			const u32 _min = std::min(memory_base, compare_min);
			const u32 _max = std::max(memory_base + memory_range, compare_max);
			return std::make_tuple(true, std::make_pair(_min, _max));
		}

		bool is_locked() const
		{
			return locked;
		}

		bool is_dirty() const
		{
			return dirty;
		}

		void set_dirty(bool state)
		{
			dirty = state;
		}

		u32 get_section_base() const
		{
			return cpu_address_base;
		}

		u32 get_section_size() const
		{
			return cpu_address_range;
		}

		bool matches(u32 cpu_address, u32 size) const
		{
			return (cpu_address_base == cpu_address && cpu_address_range == size);
		}

		std::pair<u32, u32> get_min_max(const std::pair<u32, u32>& current_min_max) const
		{
			u32 min = std::min(current_min_max.first, locked_address_base);
			u32 max = std::max(current_min_max.second, locked_address_base + locked_address_range);

			return std::make_pair(min, max);
		}

		utils::protection get_protection() const
		{
			return protection;
		}

		template <typename T = void>
		T* get_raw_ptr(u32 offset = 0, bool no_sync = false)
		{
			verify(HERE), locked_memory_ptr;
			return locked_memory_ptr.get<T>(offset, no_sync);
		}

		bool test_memory_head()
		{
			if (!locked_memory_ptr)
			{
				return false;
			}

			const u32* first = locked_memory_ptr.get<u32>(confirmed_range.first);
			return (*first == (cpu_address_base + confirmed_range.first));
		}

		bool test_memory_tail()
		{
			if (!locked_memory_ptr)
			{
				return false;
			}

			const u32 valid_limit = (confirmed_range.second) ? confirmed_range.first + confirmed_range.second : cpu_address_range;
			const u32* last = locked_memory_ptr.get<u32>(valid_limit - 4);
			return (*last == (cpu_address_base + valid_limit - 4));
		}

		void flush_io(u32 offset = 0, u32 len = 0) const
		{
			const auto write_length = len ? len : (cpu_address_range - offset);
			locked_memory_ptr.flush(offset, write_length);
		}

		std::pair<u32, u32> get_protected_range() const
		{
			if (locked)
			{
				return { locked_address_base, locked_address_range };
			}
			else
			{
				return { 0, 0 };
			}
		}

		std::pair<u32, u32> get_confirmed_range() const
		{
			if (confirmed_range.second == 0)
			{
				return { 0, cpu_address_range };
			}

			return confirmed_range;
		}
	};

	template <typename pipeline_storage_type, typename backend_storage>
	class shaders_cache
	{
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
		std::unordered_map<u64, std::vector<u8>> fragment_program_data;

		backend_storage& m_storage;

	public:

		struct progress_dialog_helper
		{
			std::shared_ptr<MsgDialogBase> dlg;
			atomic_t<int> ref_cnt;

			virtual void create()
			{
				dlg = Emu.GetCallbacks().get_msg_dialog();
				dlg->type.se_normal = true;
				dlg->type.bg_invisible = true;
				dlg->type.progress_bar_count = 2;
				dlg->ProgressBarSetTaskbarIndex(-1); // -1 to combine all progressbars in the taskbar progress
				dlg->on_close = [](s32 status)
				{
					Emu.CallAfter([]()
					{
						Emu.Stop();
					});
				};

				ref_cnt++;

				Emu.CallAfter([&]()
				{
					dlg->Create("Preloading cached shaders from disk.\nPlease wait...", "Shader Compilation");
					ref_cnt--;
				});

				while (ref_cnt.load() && !Emu.IsStopped())
				{
					_mm_pause();
				}
			}

			virtual void update_msg(u32 index, u32 processed, u32 entry_count)
			{
				ref_cnt++;

				Emu.CallAfter([&]()
				{
					const char *text = index == 0 ? "Loading pipeline object %u of %u" : "Compiling pipeline object %u of %u";
					dlg->ProgressBarSetMsg(index, fmt::format(text, processed, entry_count));
					ref_cnt--;
				});
			}

			virtual void inc_value(u32 index, u32 value)
			{
				ref_cnt++;

				Emu.CallAfter([&]()
				{
					dlg->ProgressBarInc(index, value);
					ref_cnt--;
				});
			}
			
			virtual void set_limit(u32 index, u32 limit)
			{
				ref_cnt++;

				Emu.CallAfter([&]()
				{
					dlg->ProgressBarSetLimit(index, limit);
					ref_cnt--;
				});
			}

			virtual void refresh()
			{};

			virtual void close()
			{
				while (ref_cnt.load() && !Emu.IsStopped())
				{
					_mm_pause();
				}
			}
		};

		shaders_cache(backend_storage& storage, std::string pipeline_class, std::string version_prefix_str = "v1")
			: version_prefix(version_prefix_str)
			, pipeline_class_name(pipeline_class)
			, m_storage(storage)
		{
			root_path = Emu.GetCachePath() + "/shaders_cache";
		}

		template <typename... Args>
		void load(progress_dialog_helper* dlg, Args&& ...args)
		{
			if (g_cfg.video.disable_on_disk_shader_cache || Emu.GetCachePath() == "")
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

			if ((entry_count = (u32)entries.size()) <= 2)
				return;

			root.rewind();

			// Invalid pipeline entries to be removed
			std::vector<std::string> invalid_entries;

			// Progress dialog
			std::unique_ptr<progress_dialog_helper> fallback_dlg;
			if (!dlg)
			{
				fallback_dlg = std::make_unique<progress_dialog_helper>();
				dlg = fallback_dlg.get();
			}

			dlg->create();
			dlg->set_limit(0, entry_count);
			dlg->set_limit(1, entry_count);
			dlg->update_msg(0, 0, entry_count);
			dlg->update_msg(1, 0, entry_count);

			// Setup worker threads
			unsigned nb_threads = std::thread::hardware_concurrency();
			std::vector<std::thread> worker_threads(nb_threads);

			// Preload everything needed to compile the shaders
			// Can probably be parallelized too, but since it's mostly reading files it's probably not worth it
			std::vector<std::tuple<pipeline_storage_type, RSXVertexProgram, RSXFragmentProgram>> unpackeds;
			std::chrono::time_point<steady_clock> last_update;
			u32 processed_since_last_update = 0;

			for (u32 i = 0; (i < entry_count) && !Emu.IsStopped(); i++)
			{
				fs::dir_entry tmp = entries[i];

				const auto filename = directory_path + "/" + tmp.name;
				std::vector<u8> bytes;
				fs::file f(filename);
				if (f.size() != sizeof(pipeline_data))
				{
					LOG_ERROR(RSX, "Cached pipeline object %s is not binary compatible with the current shader cache", tmp.name.c_str());
					invalid_entries.push_back(filename);
					continue;
				}
				f.read<u8>(bytes, f.size());

				auto unpacked = unpack(*(pipeline_data*)bytes.data());
				m_storage.preload_programs(std::get<1>(unpacked), std::get<2>(unpacked));
				unpackeds.push_back(unpacked);

				// Only update the screen at about 10fps since updating it everytime slows down the process
				std::chrono::time_point<steady_clock> now = std::chrono::steady_clock::now();
				processed_since_last_update++;
				if ((std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update) > 100ms) || (i == entry_count - 1))
				{
					dlg->update_msg(0, i + 1, entry_count);
					dlg->inc_value(0, processed_since_last_update);
					last_update = now;
					processed_since_last_update = 0;
				}
			}

			atomic_t<u32> processed(0);
			std::function<void(u32)> shader_comp_worker = [&](u32 index)
			{
				u32 pos;
				while (((pos = processed++) < entry_count) && !Emu.IsStopped())
				{
					auto unpacked = unpackeds[pos];
					m_storage.add_pipeline_entry(std::get<1>(unpacked), std::get<2>(unpacked), std::get<0>(unpacked), std::forward<Args>(args)...);
				}
			};

			if (g_cfg.video.renderer == video_renderer::vulkan)
			{
				// Start workers
				for (u32 i = 0; i < nb_threads; i++)
				{
					worker_threads[i] = std::thread(shader_comp_worker, i);
				}

				// Wait for the workers to finish their task while updating UI
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
						dlg->update_msg(1, current_progress, entry_count);
						dlg->inc_value(1, processed_since_last_update);
					}
				}

				// Need to join the threads to be absolutely sure shader compilation is done.
				for (std::thread& worker_thread : worker_threads)
					worker_thread.join();
			}
			else
			{
				u32 pos;
				while (((pos = processed++) < entry_count) && !Emu.IsStopped())
				{
					auto unpacked = unpackeds[pos];
					m_storage.add_pipeline_entry(std::get<1>(unpacked), std::get<2>(unpacked), std::get<0>(unpacked), std::forward<Args>(args)...);

					// Update screen at about 10fps
					std::chrono::time_point<steady_clock> now = std::chrono::steady_clock::now();
					processed_since_last_update++;
					if ((std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update) > 100ms) || (pos == entry_count - 1))
					{
						dlg->update_msg(1, pos + 1, entry_count);
						dlg->inc_value(1, processed_since_last_update);
						last_update = now;
						processed_since_last_update = 0;
					}
				}
			}

			if (!invalid_entries.empty())
			{
				for (const auto &filename : invalid_entries)
				{
					fs::remove_file(filename);
				}

				LOG_NOTICE(RSX, "shader cache: %d entries were marked as invalid and removed", invalid_entries.size());
			}

			dlg->refresh();
			dlg->close();
		}

		void store(pipeline_storage_type &pipeline, RSXVertexProgram &vp, RSXFragmentProgram &fp)
		{
			if (g_cfg.video.disable_on_disk_shader_cache || Emu.GetCachePath() == "")
			{
				return;
			}

			if (vp.jump_table.size() > 32)
			{
				LOG_ERROR(RSX, "shaders_cache: vertex program has more than 32 jump addresses. Entry not saved to cache");
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
			fragment_program_data[program_hash] = data;
			fp.addr = fragment_program_data[program_hash].data();
			fp.ucode_length = (u32)data.size();

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
			fp.unnormalized_coords = data.fp_unnormalized_coords;
			fp.front_back_color_enabled = (data.fp_lighting_flags & 0x1) != 0;
			fp.back_color_diffuse_output = ((data.fp_lighting_flags >> 1) & 0x1) != 0;
			fp.back_color_specular_output = ((data.fp_lighting_flags >> 2) & 0x1) != 0;
			fp.front_color_diffuse_output = ((data.fp_lighting_flags >> 3) & 0x1) != 0;
			fp.front_color_specular_output = ((data.fp_lighting_flags >> 4) & 0x1) != 0;
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
						data_block.vp_jump_table[index++] = (u16)address;
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
			data_block.fp_unnormalized_coords = fp.unnormalized_coords;
			data_block.fp_lighting_flags = (u16)fp.front_back_color_enabled | (u16)fp.back_color_diffuse_output << 1 |
				(u16)fp.back_color_specular_output << 2 | (u16)fp.front_color_diffuse_output << 3 | (u16)fp.front_color_specular_output << 4;
			data_block.fp_shadow_textures = fp.shadow_textures;
			data_block.fp_redirected_textures = fp.redirected_textures;

			for (u8 index = 0; index < 16; ++index)
			{
				data_block.fp_alphakill_mask |= (u32)(fp.textures_alpha_kill[index] & 0x1) << index;
				data_block.fp_zfunc_mask |= (u32)(fp.textures_zfunc[index] & 0xF) << (index << 2);
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
			virtual ~default_vertex_cache() {};
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
				for (auto &v : vertex_ranges[local_addr])
				{
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
