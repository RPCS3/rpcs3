#pragma once
#include "Utilities/VirtualMemory.h"
#include "Utilities/hash.h"
#include "Emu/Memory/vm.h"
#include "gcm_enums.h"
#include "Common/ProgramStateCache.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/System.h"

namespace rsx
{
	struct blit_src_info
	{
		blit_engine::transfer_source_format format;
		u16 offset_x;
		u16 offset_y;
		u16 width;
		u16 height;
		u16 slice_h;
		u16 pitch;
		void *pixels;

		u32 rsx_address;
	};

	struct blit_dst_info
	{
		blit_engine::transfer_destination_format format;
		u16 offset_x;
		u16 offset_y;
		u16 width;
		u16 height;
		u16 pitch;
		u16 clip_x;
		u16 clip_y;
		u16 clip_width;
		u16 clip_height;

		bool swizzled;
		void *pixels;

		u32  rsx_address;
	};

	enum protection_policy
	{
		protect_policy_one_page,	//Only guard one page, preferrably one where this section 'wholly' fits
		protect_policy_full_range	//Guard the full memory range. Shared pages may be invalidated by access outside the object we're guarding
	};

	class buffered_section
	{
	private:
		u32 locked_address_base = 0;
		u32 locked_address_range = 0;

	protected:
		u32 cpu_address_base = 0;
		u32 cpu_address_range = 0;

		utils::protection protection = utils::protection::rw;

		bool locked = false;
		bool dirty = false;

		inline bool region_overlaps(u32 base1, u32 limit1, u32 base2, u32 limit2)
		{
			return (base1 < limit2 && base2 < limit1);
		}

	public:

		buffered_section() {}
		~buffered_section() {}

		void reset(u32 base, u32 length, protection_policy protect_policy= protect_policy_full_range)
		{
			verify(HERE), locked == false;

			cpu_address_base = base;
			cpu_address_range = length;

			locked_address_base = (base & ~4095);

			if (protect_policy == protect_policy_one_page)
			{
				locked_address_range = 4096;
				if (locked_address_base < base)
				{
					//Try the next page if we can
					//TODO: If an object spans a boundary without filling either side, guard the larger page occupancy
					const u32 next_page = locked_address_base + 4096;
					if ((base + length) >= (next_page + 4096))
					{
						//The object spans the entire page. Guard this instead
						locked_address_base = next_page;
					}
				}
			}
			else
				locked_address_range = align(base + length, 4096) - locked_address_base;

			protection = utils::protection::rw;
			locked = false;
		}

		void protect(utils::protection prot)
		{
			if (prot == protection) return;

			utils::memory_protect(vm::base(locked_address_base), locked_address_range, prot);
			protection = prot;
			locked = prot != utils::protection::rw;
		}

		void unprotect()
		{
			protect(utils::protection::rw);
			locked = false;
		}

		void discard()
		{
			protection = utils::protection::rw;
			dirty = true;
			locked = false;
		}

		bool overlaps(std::pair<u32, u32> range)
		{
			return region_overlaps(locked_address_base, locked_address_base + locked_address_range, range.first, range.first + range.second);
		}

		bool overlaps(u32 address)
		{
			return (locked_address_base <= address && (address - locked_address_base) < locked_address_range);
		}

		/**
		 * Check if range overlaps with this section.
		 * ignore_protection_range - if true, the test should not check against the aligned protection range, instead
		 * tests against actual range of contents in memory
		 */
		bool overlaps(std::pair<u32, u32> range, bool ignore_protection_range)
		{
			if (!ignore_protection_range)
				return region_overlaps(locked_address_base, locked_address_base + locked_address_range, range.first, range.first + range.second);
			else
				return region_overlaps(cpu_address_base, cpu_address_base + cpu_address_range, range.first, range.first + range.second);
		}

		/**
		 * Check if the page containing the address tramples this section. Also compares a former trampled page range to compare
		 * If true, returns the range <min, max> with updated invalid range 
		 */
		std::tuple<bool, std::pair<u32, u32>> overlaps_page(std::pair<u32, u32> old_range, u32 address)
		{
			const u32 page_base = address & ~4095;
			const u32 page_limit = address + 4096;

			const u32 compare_min = std::min(old_range.first, page_base);
			const u32 compare_max = std::max(old_range.second, page_limit);

			if (!region_overlaps(locked_address_base, locked_address_base + locked_address_range, compare_min, compare_max))
				return std::make_tuple(false, old_range);

			return std::make_tuple(true, get_min_max(std::make_pair(compare_min, compare_max)));
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

		std::pair<u32, u32> get_min_max(std::pair<u32, u32> current_min_max)
		{
			u32 min = std::min(current_min_max.first, locked_address_base);
			u32 max = std::max(current_min_max.second, locked_address_base + locked_address_range);

			return std::make_pair(min, max);
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

		shaders_cache(backend_storage& storage, std::string pipeline_class, std::string version_prefix_str = "v1")
			: version_prefix(version_prefix_str)
			, pipeline_class_name(pipeline_class)
			, m_storage(storage)
		{
			root_path = Emu.GetCachePath() + "/shaders_cache";
		}

		template <typename... Args>
		void load(Args&& ...args)
		{
			std::string directory_path = root_path + "/pipelines/" + pipeline_class_name;

			if (!fs::is_dir(directory_path))
			{
				fs::create_path(directory_path);
				fs::create_path(root_path + "/raw");

				return;
			}

			fs::dir root = fs::dir(directory_path);
			fs::dir_entry tmp;

			u32 entry_count = 0;
			for (auto It = root.begin(); It != root.end(); ++It, entry_count++);

			if (entry_count <= 2)
				return;

			entry_count -= 2;
			f32 delta = 100.f / entry_count;
			f32 tally = 0.f;

			root.rewind();

			// Progress dialog
			auto dlg = Emu.GetCallbacks().get_msg_dialog();
			dlg->type.se_normal = true;
			dlg->type.bg_invisible = true;
			dlg->type.progress_bar_count = 1;
			dlg->on_close = [](s32 status)
			{
				Emu.CallAfter([]()
				{
					Emu.Stop();
				});
			};

			Emu.CallAfter([=]()
			{
				dlg->Create("Preloading cached shaders from disk.\nPlease wait...");
			});

			u32 processed = 0;
			while (root.read(tmp))
			{
				if (tmp.name == "." || tmp.name == "..")
					continue;

				std::vector<u8> bytes;
				fs::file f(directory_path + "/" + tmp.name);

				processed++;
				Emu.CallAfter([=]()
				{
					dlg->ProgressBarSetMsg(0, fmt::format("Loading pipeline object %u of %u", processed, entry_count));
				});

				if (f.size() != sizeof(pipeline_data))
				{
					LOG_ERROR(RSX, "Cached pipeline object %s is not binary compatible with the current shader cache", tmp.name.c_str());
					continue;
				}

				f.read<u8>(bytes, f.size());
				auto unpacked = unpack(*(pipeline_data*)bytes.data());
				m_storage.add_pipeline_entry(std::get<1>(unpacked), std::get<2>(unpacked), std::get<0>(unpacked), std::forward<Args>(args)...);

				tally += delta;
				if (tally > 1.f)
				{
					u32 value = (u32)tally;
					Emu.CallAfter([=]()
					{
						dlg->ProgressBarInc(0, value);
					});

					tally -= (f32)value;
				}
			}
		}

		void store(pipeline_storage_type &pipeline, RSXVertexProgram &vp, RSXFragmentProgram &fp)
		{
			pipeline_data data = pack(pipeline, vp, fp);
			std::string fp_name = root_path + "/raw/" + fmt::format("%llX.fp", data.fragment_program_hash);
			std::string vp_name = root_path + "/raw/" + fmt::format("%llX.vp", data.vertex_program_hash);

			if (!fs::is_file(fp_name))
			{
				const auto size = program_hash_util::fragment_program_utils::get_fragment_program_ucode_size(fp.addr);
				fs::file(fp_name, fs::rewrite).write(fp.addr, size);
			}

			if (!fs::is_file(vp_name))
			{
				fs::file(vp_name, fs::rewrite).write<u32>(vp.data);
			}

			u64 state_hash = 0;
			state_hash ^= rpcs3::hash_base<u32>(data.vp_ctrl);
			state_hash ^= rpcs3::hash_base<u32>(data.fp_ctrl);
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
			std::string pipeline_path = root_path + "/pipelines/" + pipeline_class_name + "/" + version_prefix + "-" + pipeline_file_name;
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

			return fp;
		}

		std::tuple<pipeline_storage_type, RSXVertexProgram, RSXFragmentProgram> unpack(pipeline_data &data)
		{
			RSXVertexProgram vp = load_vp_raw(data.vertex_program_hash);
			RSXFragmentProgram fp = load_fp_raw(data.fragment_program_hash);
			pipeline_storage_type pipeline = data.pipeline_properties;

			vp.output_mask = data.vp_ctrl;

			fp.ctrl = data.fp_ctrl;
			fp.texture_dimensions = data.fp_texture_dimensions;
			fp.unnormalized_coords = data.fp_unnormalized_coords;
			fp.height = data.fp_height;
			fp.pixel_center_mode = (rsx::window_pixel_center)(data.fp_pixel_layout & 0x3);
			fp.origin_mode = (rsx::window_origin)((data.fp_pixel_layout >> 2) & 0x1);
			fp.alpha_func = (rsx::comparison_function)((data.fp_pixel_layout >> 3) & 0xF);
			fp.fog_equation = (rsx::fog_mode)((data.fp_pixel_layout >> 7) & 0xF);
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

			data_block.fp_ctrl = fp.ctrl;
			data_block.fp_texture_dimensions = fp.texture_dimensions;
			data_block.fp_unnormalized_coords = fp.unnormalized_coords;
			data_block.fp_height = fp.height;
			data_block.fp_pixel_layout = (u16)fp.pixel_center_mode | (u16)fp.origin_mode << 2 | (u16)fp.alpha_func << 3;
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
