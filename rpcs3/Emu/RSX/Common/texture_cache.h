#pragma once

#include "../rsx_cache.h"
#include "../rsx_utils.h"
#include "TextureUtils.h"

#include <atomic>

namespace rsx
{
	enum texture_create_flags
	{
		default_component_order = 0,
		native_component_order = 1,
		swapped_native_component_order = 2,
	};

	enum texture_upload_context
	{
		shader_read = 0,
		blit_engine_src = 1,
		blit_engine_dst = 2,
		framebuffer_storage = 3
	};

	struct cached_texture_section : public rsx::buffered_section
	{
		u16 width;
		u16 height;
		u16 depth;
		u16 mipmaps;

		u16 real_pitch;
		u16 rsx_pitch;

		u64 cache_tag;

		rsx::texture_create_flags view_flags = rsx::texture_create_flags::default_component_order;
		rsx::texture_upload_context context = rsx::texture_upload_context::shader_read;

		bool matches(const u32 rsx_address, const u32 rsx_size)
		{
			return rsx::buffered_section::matches(rsx_address, rsx_size);
		}

		bool matches(const u32 rsx_address, const u32 width, const u32 height, const u32 depth, const u32 mipmaps)
		{
			if (rsx_address == cpu_address_base)
			{
				if (!width && !height && !mipmaps)
					return true;

				if (width && width != this->width)
					return false;

				if (height && height != this->height)
					return false;

				if (depth && depth != this->depth)
					return false;

				if (mipmaps && mipmaps != this->mipmaps)
					return false;

				return true;
			}

			return false;
		}

		void set_view_flags(const rsx::texture_create_flags flags)
		{
			view_flags = flags;
		}

		void set_context(const rsx::texture_upload_context upload_context)
		{
			context = upload_context;
		}

		u16 get_width() const
		{
			return width;
		}

		u16 get_height() const
		{
			return height;
		}

		rsx::texture_create_flags get_view_flags() const
		{
			return view_flags;
		}

		rsx::texture_upload_context get_context() const
		{
			return context;
		}
	};

	template <typename commandbuffer_type, typename section_storage_type, typename image_resource_type, typename image_view_type, typename image_storage_type, typename texture_format>
	class texture_cache
	{
	private:
		std::pair<std::array<u8, 4>, std::array<u8, 4>> default_remap_vector = 
		{
			{ CELL_GCM_TEXTURE_REMAP_FROM_A, CELL_GCM_TEXTURE_REMAP_FROM_R, CELL_GCM_TEXTURE_REMAP_FROM_G, CELL_GCM_TEXTURE_REMAP_FROM_B },
			{ CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP }
		};

	protected:

		struct ranged_storage
		{
			std::vector<section_storage_type> data;  //Stored data
			std::atomic_int valid_count = { 0 };  //Number of usable (non-dirty) blocks
			u32 max_range = 0;  //Largest stored block
			u32 max_addr = 0;
			u32 min_addr = UINT32_MAX;

			void notify(u32 addr, u32 data_size)
			{
				verify(HERE), valid_count >= 0;

				const u32 addr_base = addr & ~0xfff;
				const u32 block_sz = align(addr + data_size, 4096u) - addr_base;

				max_range = std::max(max_range, block_sz);
				max_addr = std::max(max_addr, addr);
				min_addr = std::min(min_addr, addr_base);
				valid_count++;
			}

			void add(section_storage_type& section, u32 addr, u32 data_size)
			{
				data.push_back(std::move(section));
				notify(addr, data_size);
			}

			void remove_one()
			{
				verify(HERE), valid_count > 0;
				valid_count--;
			}
		};

		// Keep track of cache misses to pre-emptively flush some addresses
		struct framebuffer_memory_characteristics
		{
			u32 misses;
			u32 block_size;
			texture_format format;
		};

		shared_mutex m_cache_mutex;
		std::unordered_map<u32, ranged_storage> m_cache;

		std::pair<u32, u32> read_only_range = std::make_pair(0xFFFFFFFF, 0);
		std::pair<u32, u32> no_access_range = std::make_pair(0xFFFFFFFF, 0);

		std::unordered_map<u32, framebuffer_memory_characteristics> m_cache_miss_statistics_table;

		//Set when a hw blit engine incompatibility is detected
		bool blit_engine_incompatibility_warning_raised = false;
		
		//Memory usage
		const s32 m_max_zombie_objects = 64; //Limit on how many texture objects to keep around for reuse after they are invalidated
		std::atomic<s32> m_unreleased_texture_objects = { 0 }; //Number of invalidated objects not yet freed from memory
		std::atomic<u32> m_texture_memory_in_use = { 0 };
		
		/* Helpers */
		virtual void free_texture_section(section_storage_type&) = 0;
		virtual image_view_type create_temporary_subresource_view(commandbuffer_type&, image_resource_type* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h) = 0;
		virtual image_view_type create_temporary_subresource_view(commandbuffer_type&, image_storage_type* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h) = 0;
		virtual section_storage_type* create_new_texture(commandbuffer_type&, u32 rsx_address, u32 rsx_size, u16 width, u16 height, u16 depth, u16 mipmaps, const u32 gcm_format,
				const rsx::texture_upload_context context, const rsx::texture_dimension_extended type, const texture_create_flags flags, std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap_vector) = 0;
		virtual section_storage_type* upload_image_from_cpu(commandbuffer_type&, u32 rsx_address, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, const u32 gcm_format, const texture_upload_context context,
				std::vector<rsx_subresource_layout>& subresource_layout, const rsx::texture_dimension_extended type, const bool swizzled, std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap_vector) = 0;
		virtual void enforce_surface_creation_type(section_storage_type& section, const texture_create_flags expected) = 0;
		virtual void insert_texture_barrier() = 0;

		constexpr u32 get_block_size() const { return 0x1000000; }
		inline u32 get_block_address(u32 address) const { return (address & ~0xFFFFFF); }

	public:
		//Struct to hold data on sections to be paged back onto cpu memory
		struct thrashed_set
		{
			bool violation_handled = false;
			std::vector<section_storage_type*> affected_sections; //Always laid out with flushable sections first then other affected sections last
			int num_flushable = 0;
		};

	private:
		//Internal implementation methods and helpers

		utils::protection get_memory_protection(u32 address)
		{
			auto found = m_cache.find(get_block_address(address));
			if (found != m_cache.end())
			{
				for (const auto &tex : found->second.data)
				{
					if (tex.is_locked() && tex.overlaps(address, false))
						return tex.get_protection();
				}
			}

			return utils::protection::rw;
		}

		//Get intersecting set - Returns all objects intersecting a given range and their owning blocks
		std::vector<std::pair<section_storage_type*, ranged_storage*>> get_intersecting_set(u32 address, u32 range, bool check_whole_size)
		{
			std::vector<std::pair<section_storage_type*, ranged_storage*>> result;
			u64 cache_tag = get_system_time();
			u32 last_dirty_block = UINT32_MAX;

			std::pair<u32, u32> trampled_range = std::make_pair(address, address + range);

			for (auto It = m_cache.begin(); It != m_cache.end(); It++)
			{
				auto &range_data = It->second;
				const u32 base = It->first;
				bool range_reset = false;

				if (base == last_dirty_block && range_data.valid_count == 0)
					continue;

				if (trampled_range.first <= trampled_range.second)
				{
					//Only if a valid range, ignore empty sets
					if (trampled_range.first >= (range_data.max_addr + range_data.max_range) || range_data.min_addr >= trampled_range.second)
						continue;
				}

				for (int i = 0; i < range_data.data.size(); i++)
				{
					auto &tex = range_data.data[i];
					if (tex.cache_tag == cache_tag) continue; //already processed
					if (!tex.is_locked()) continue;	//flushable sections can be 'clean' but unlocked. TODO: Handle this better

					auto overlapped = tex.overlaps_page(trampled_range, address, check_whole_size);
					if (std::get<0>(overlapped))
					{
						auto &new_range = std::get<1>(overlapped);

						if (new_range.first != trampled_range.first ||
							new_range.second != trampled_range.second)
						{
							i = 0;
							trampled_range = new_range;
							range_reset = true;
						}

						tex.cache_tag = cache_tag;
						result.push_back({&tex, &range_data});
					}
				}

				if (range_reset)
				{
					last_dirty_block = base;
					It = m_cache.begin();
				}
			}

			return result;
		}

		//Invalidate range base implementation
		template <typename ...Args>
		thrashed_set invalidate_range_impl_base(u32 address, u32 range, bool is_writing, bool discard_only, bool rebuild_cache, bool allow_flush, Args&&... extras)
		{
			auto trampled_set = get_intersecting_set(address, range, allow_flush);

			if (trampled_set.size() > 0)
			{
				// Rebuild the cache by only destroying ranges that need to be destroyed to unlock this page
				const auto to_reprotect = !rebuild_cache? trampled_set.end() :
				std::remove_if(trampled_set.begin(), trampled_set.end(),
				[&](const std::pair<section_storage_type*, ranged_storage*>& obj)
				{
					if (!is_writing && obj.first->get_protection() != utils::protection::no)
						return true;

					if (!obj.first->is_flushable())
						return false;

					const std::pair<u32, u32> null_check = std::make_pair(UINT32_MAX, 0);
					return !std::get<0>(obj.first->overlaps_page(null_check, address, true));
				});

				std::vector<section_storage_type*> sections_to_flush;
				for (auto It = trampled_set.begin(); It != to_reprotect; ++It)
				{
					auto obj = *It;

					if (obj.first->is_flushable())
					{
						sections_to_flush.push_back(obj.first);
					}
					else
					{
						obj.first->set_dirty(true);
						m_unreleased_texture_objects++;
					}

					if (discard_only)
						obj.first->discard();
					else
						obj.first->unprotect();

					obj.second->remove_one();
				}

				thrashed_set result = {};
				result.violation_handled = true;

				if (allow_flush)
				{
					// Flush here before 'reprotecting' since flushing will write the whole span
					for (const auto &tex : sections_to_flush)
					{
						if (!tex->flush(std::forward<Args>(extras)...))
						{
							//Missed address, note this
							//TODO: Lower severity when successful to keep the cache from overworking
							record_cache_miss(*tex);
						}
					}
				}
				else if (sections_to_flush.size() > 0)
				{
					result.num_flushable = static_cast<int>(sections_to_flush.size());
					result.affected_sections = std::move(sections_to_flush);
				}

				for (auto It = to_reprotect; It != trampled_set.end(); It++)
				{
					auto obj = *It;

					auto old_prot = obj.first->get_protection();
					obj.first->discard();
					obj.first->protect(old_prot);
					obj.first->set_dirty(false);

					if (result.affected_sections.size() > 0)
					{
						//Append to affected set. Not counted as part of num_flushable
						result.affected_sections.push_back(obj.first);
					}
				}

				return result;
			}

			return {};
		}

		template <typename ...Args>
		thrashed_set invalidate_range_impl(u32 address, u32 range, bool is_writing, bool discard, bool allow_flush, Args&&... extras)
		{
			return invalidate_range_impl_base(address, range, is_writing, discard, true, allow_flush, std::forward<Args>(extras)...);
		}

		bool is_hw_blit_engine_compatible(const u32 format) const
		{
			switch (format)
			{
			case CELL_GCM_TEXTURE_A8R8G8B8:
			case CELL_GCM_TEXTURE_R5G6B5:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH24_D8:
				return true;
			default:
				return false;
			}
		}

	public:

		texture_cache() {}
		~texture_cache() {}
		
		virtual void destroy() = 0;
		virtual bool is_depth_texture(const u32, const u32) = 0;
		virtual void on_frame_end() = 0;

		std::vector<section_storage_type*> find_texture_from_range(u32 rsx_address, u32 range)
		{
			std::vector<section_storage_type*> results;
			auto test = std::make_pair(rsx_address, range);
			for (auto &address_range : m_cache)
			{
				if (address_range.second.valid_count == 0) continue;
				auto &range_data = address_range.second;

				for (auto &tex : range_data.data)
				{
					if (tex.get_section_base() > rsx_address)
						continue;

					if (!tex.is_dirty() && tex.overlaps(test, true))
						results.push_back(&tex);
				}
			}

			return results;
		}

		section_storage_type *find_texture_from_dimensions(u32 rsx_address, u16 width = 0, u16 height = 0, u16 depth = 0, u16 mipmaps = 0)
		{
			auto found = m_cache.find(get_block_address(rsx_address));
			if (found != m_cache.end())
			{
				auto &range_data = found->second;
				for (auto &tex : range_data.data)
				{
					if (tex.matches(rsx_address, width, height, depth, mipmaps) && !tex.is_dirty())
					{
						return &tex;
					}
				}
			}

			return nullptr;
		}

		section_storage_type& find_cached_texture(u32 rsx_address, u32 rsx_size, bool confirm_dimensions = false, u16 width = 0, u16 height = 0, u16 depth = 0, u16 mipmaps = 0)
		{
			const u32 block_address = get_block_address(rsx_address);

			auto found = m_cache.find(block_address);
			if (found != m_cache.end())
			{
				auto &range_data = found->second;

				for (auto &tex : range_data.data)
				{
					if (tex.matches(rsx_address, rsx_size) && !tex.is_dirty())
					{
						if (!confirm_dimensions || tex.matches(rsx_address, width, height, depth, mipmaps))
						{
							if (!tex.is_locked() && tex.get_context() == texture_upload_context::framebuffer_storage)
								range_data.notify(rsx_address, rsx_size);

							return tex;
						}
						else
						{
							LOG_ERROR(RSX, "Cached object for address 0x%X was found, but it does not match stored parameters.", rsx_address);
							LOG_ERROR(RSX, "%d x %d vs %d x %d", width, height, tex.get_width(), tex.get_height());
						}
					}
				}

				for (auto &tex : range_data.data)
				{
					if (tex.is_dirty())
					{
						if (tex.exists())
						{
							m_unreleased_texture_objects--;
							free_texture_section(tex);
							m_texture_memory_in_use -= tex.get_section_size();
						}

						range_data.notify(rsx_address, rsx_size);
						return tex;
					}
				}
			}

			section_storage_type tmp;
			m_cache[block_address].add(tmp, rsx_address, rsx_size);
			return m_cache[block_address].data.back();
		}

		section_storage_type* find_flushable_section(const u32 address, const u32 range)
		{
			auto found = m_cache.find(get_block_address(address));
			if (found != m_cache.end())
			{
				auto &range_data = found->second;
				for (auto &tex : range_data.data)
				{
					if (tex.is_dirty()) continue;
					if (!tex.is_flushable() && !tex.is_flushed()) continue;

					if (tex.matches(address, range))
						return &tex;
				}
			}

			return nullptr;
		}

		template <typename ...Args>
		void lock_memory_region(image_storage_type* image, const u32 memory_address, const u32 memory_size, const u32 width, const u32 height, const u32 pitch, Args&&... extras)
		{
			writer_lock lock(m_cache_mutex);
			section_storage_type& region = find_cached_texture(memory_address, memory_size, false);

			if (!region.is_locked())
			{
				region.reset(memory_address, memory_size);
				region.set_dirty(false);
				no_access_range = region.get_min_max(no_access_range);
			}

			region.protect(utils::protection::no);
			region.create(width, height, 1, 1, nullptr, image, pitch, false, std::forward<Args>(extras)...);
			region.set_context(texture_upload_context::framebuffer_storage);
		}

		template <typename ...Args>
		bool flush_memory_to_cache(const u32 memory_address, const u32 memory_size, bool skip_synchronized, Args&&... extra)
		{
			writer_lock lock(m_cache_mutex);
			section_storage_type* region = find_flushable_section(memory_address, memory_size);

			//TODO: Make this an assertion
			if (region == nullptr)
			{
				LOG_ERROR(RSX, "Failed to find section for render target 0x%X + 0x%X", memory_address, memory_size);
				return false;
			}

			if (skip_synchronized && region->is_synchronized())
				return false;

			region->copy_texture(false, std::forward<Args>(extra)...);
			return true;
		}
		
		template <typename ...Args>
		bool load_memory_from_cache(const u32 memory_address, const u32 memory_size, Args&&... extras)
		{
			reader_lock lock(m_cache_mutex);
			section_storage_type *region = find_flushable_section(memory_address, memory_size);

			if (region && !region->is_dirty())
			{
				region->fill_texture(std::forward<Args>(extras)...);
				return true;
			}

			//No valid object found in cache
			return false;
		}

		std::tuple<bool, section_storage_type*> address_is_flushable(u32 address)
		{
			if (address < no_access_range.first ||
				address > no_access_range.second)
				return std::make_tuple(false, nullptr);

			reader_lock lock(m_cache_mutex);

			auto found = m_cache.find(get_block_address(address));
			if (found != m_cache.end())
			{
				auto &range_data = found->second;
				for (auto &tex : range_data.data)
				{
					if (tex.is_dirty()) continue;
					if (!tex.is_flushable()) continue;

					if (tex.overlaps(address, false))
						return std::make_tuple(true, &tex);
				}
			}

			for (auto &address_range : m_cache)
			{
				if (address_range.first == address)
					continue;

				auto &range_data = address_range.second;

				//Quickly discard range
				const u32 lock_base = address_range.first & ~0xfff;
				const u32 lock_limit = align(range_data.max_range + address_range.first, 4096);

				if (address < lock_base || address >= lock_limit)
					continue;

				for (auto &tex : range_data.data)
				{
					if (tex.is_dirty()) continue;
					if (!tex.is_flushable()) continue;

					if (tex.overlaps(address, false))
						return std::make_tuple(true, &tex);
				}
			}

			return std::make_tuple(false, nullptr);
		}

		template <typename ...Args>
		thrashed_set invalidate_address(u32 address, bool is_writing, bool allow_flush, Args&&... extras)
		{
			return invalidate_range(address, 4096 - (address & 4095), is_writing, false, allow_flush, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		thrashed_set invalidate_range(u32 address, u32 range, bool is_writing, bool discard, bool allow_flush, Args&&... extras)
		{
			std::pair<u32, u32> trampled_range = std::make_pair(address, address + range);

			if (trampled_range.second < read_only_range.first ||
				trampled_range.first > read_only_range.second)
			{
				//Doesnt fall in the read_only textures range; check render targets
				if (trampled_range.second < no_access_range.first ||
					trampled_range.first > no_access_range.second)
					return {};
			}

			writer_lock lock(m_cache_mutex);
			return invalidate_range_impl(address, range, is_writing, discard, allow_flush, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		bool flush_all(thrashed_set& data, Args&&... extras)
		{
			writer_lock lock(m_cache_mutex);

			std::vector<utils::protection> old_protections;
			for (int n = data.num_flushable; n < data.affected_sections.size(); ++n)
			{
				old_protections.push_back(data.affected_sections[n]->get_protection());
				data.affected_sections[n]->unprotect();
			}

			for (int n = 0, i = 0; n < data.affected_sections.size(); ++n)
			{
				if (n < data.num_flushable)
				{
					if (!data.affected_sections[n]->flush(std::forward<Args>(extras)...))
					{
						//Missed address, note this
						//TODO: Lower severity when successful to keep the cache from overworking
						record_cache_miss(*data.affected_sections[n]);
					}
				}
				else
				{
					//Restore protection on the remaining sections
					data.affected_sections[n]->protect(old_protections[i++]);
				}
			}

			return true;
		}

		void record_cache_miss(section_storage_type &tex)
		{
			const u32 memory_address = tex.get_section_base();
			const u32 memory_size = tex.get_section_size();
			const auto fmt = tex.get_format();

			auto It = m_cache_miss_statistics_table.find(memory_address);
			if (It == m_cache_miss_statistics_table.end())
			{
				m_cache_miss_statistics_table[memory_address] = { 1, memory_size, fmt };
				return;
			}

			auto &value = It->second;
			if (value.format != fmt || value.block_size != memory_size)
			{
				m_cache_miss_statistics_table[memory_address] = { 1, memory_size, fmt };
				return;
			}

			value.misses++;
		}

		template <typename ...Args>
		void flush_if_cache_miss_likely(const texture_format fmt, const u32 memory_address, const u32 memory_size, Args&&... extras)
		{
			auto It = m_cache_miss_statistics_table.find(memory_address);
			if (It == m_cache_miss_statistics_table.end())
			{
				m_cache_miss_statistics_table[memory_address] = { 0, memory_size, fmt };
				return;
			}

			auto &value = It->second;

			if (value.format != fmt || value.block_size != memory_size)
			{
				//Reset since the data has changed
				//TODO: Keep track of all this information together
				m_cache_miss_statistics_table[memory_address] = { 0, memory_size, fmt };
				return;
			}

			//Properly synchronized - no miss
			if (!value.misses) return;

			//Auto flush if this address keeps missing (not properly synchronized)
			if (value.misses > 16)
			{
				//TODO: Determine better way of setting threshold
				if (!flush_memory_to_cache(memory_address, memory_size, true, std::forward<Args>(extras)...))
					value.misses--;
			}
		}
		
		void purge_dirty()
		{
			writer_lock lock(m_cache_mutex);

			//Reclaims all graphics memory consumed by dirty textures
			std::vector<u32> empty_addresses;
			empty_addresses.resize(32);

			for (auto &address_range : m_cache)
			{
				auto &range_data = address_range.second;

				if (range_data.valid_count == 0)
					empty_addresses.push_back(address_range.first);

				for (auto &tex : range_data.data)
				{
					if (!tex.is_dirty())
						continue;

					free_texture_section(tex);
					m_texture_memory_in_use -= tex.get_section_size();
				}
			}

			//Free descriptor objects as well
			for (const auto &address : empty_addresses)
			{
				m_cache.erase(address);
			}
			
			m_unreleased_texture_objects = 0;
		}

		template <typename RsxTextureType, typename surface_store_type, typename ...Args>
		image_view_type upload_texture(commandbuffer_type& cmd, RsxTextureType& tex, surface_store_type& m_rtts, Args&&... extras)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 tex_size = (u32)get_texture_size(tex);
			const u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			const bool is_compressed_format = (format == CELL_GCM_TEXTURE_COMPRESSED_DXT1 || format == CELL_GCM_TEXTURE_COMPRESSED_DXT23 || format == CELL_GCM_TEXTURE_COMPRESSED_DXT45);

			if (!texaddr || !tex_size)
			{
				LOG_ERROR(RSX, "Texture upload requested but texture not found, (address=0x%X, size=0x%X)", texaddr, tex_size);
				return 0;
			}

			const auto extended_dimension = tex.get_extended_texture_dimension();

			if (!is_compressed_format)
			{
				//Check for sampleable rtts from previous render passes
				//TODO: When framebuffer Y compression is properly handled, this section can be removed. A more accurate framebuffer storage check exists below this block
				if (auto texptr = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
				{
					if (test_framebuffer(texaddr))
					{
						if (extended_dimension != rsx::texture_dimension_extended::texture_dimension_2d)
							LOG_ERROR(RSX, "Texture resides in render target memory, but requested type is not 2D (%d)", (u32)extended_dimension);

						for (const auto& tex : m_rtts.m_bound_render_targets)
						{
							if (std::get<0>(tex) == texaddr)
							{
								if (g_cfg.video.strict_rendering_mode)
								{
									LOG_WARNING(RSX, "Attempting to sample a currently bound render target @ 0x%x", texaddr);
									return create_temporary_subresource_view(cmd, texptr, format, 0, 0, texptr->width(), texptr->height());
								}
								else
								{
									//issue a texture barrier to ensure previous writes are visible
									insert_texture_barrier();
									break;
								}
							}
						}

						return texptr->get_view();
					}
				}

				if (auto texptr = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
				{
					if (test_framebuffer(texaddr))
					{
						if (extended_dimension != rsx::texture_dimension_extended::texture_dimension_2d)
							LOG_ERROR(RSX, "Texture resides in depth buffer memory, but requested type is not 2D (%d)", (u32)extended_dimension);

						if (texaddr == std::get<0>(m_rtts.m_bound_depth_stencil))
						{
							if (g_cfg.video.strict_rendering_mode)
							{
								LOG_WARNING(RSX, "Attempting to sample a currently bound depth surface @ 0x%x", texaddr);
								return create_temporary_subresource_view(cmd, texptr, format, 0, 0, texptr->width(), texptr->height());
							}
							else
							{
								//issue a texture barrier to ensure previous writes are visible
								insert_texture_barrier();
							}
						}

						return texptr->get_view();
					}
				}
			}

			u16 depth = 0;
			u16 tex_height = (u16)tex.height();
			u16 tex_pitch = tex.pitch();
			const u16 tex_width = tex.width();

			tex_pitch = is_compressed_format? (tex_size / tex_height) : tex_pitch; //NOTE: Compressed textures dont have a real pitch (tex_size = (w*h)/6)
			if (tex_pitch == 0) tex_pitch = tex_width * get_format_block_size_in_bytes(format);

			switch (extended_dimension)
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				tex_height = 1;
				depth = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				depth = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				depth = 6;
				break;
			case rsx::texture_dimension_extended::texture_dimension_3d:
				depth = tex.depth();
				break;
			}

			if (!is_compressed_format)
			{
				/* Check if we are re-sampling a subresource of an RTV/DSV texture, bound or otherwise
				 * This check is much stricter than the one above
				 * (Turbo: Super Stunt Squad does this; bypassing the need for a sync object)
				 * The engine does not read back the texture resource through cell, but specifies a texture location that is
				 * a bound render target. We can bypass the expensive download in this case
				 */

				//TODO: Take framebuffer Y compression into account
				const u32 native_pitch = tex_width * get_format_block_size_in_bytes(format);
				const f32 internal_scale = (f32)tex_pitch / native_pitch;
				const u32 internal_width = (const u32)(tex_width * internal_scale);

				const auto rsc = m_rtts.get_surface_subresource_if_applicable(texaddr, internal_width, tex_height, tex_pitch, true);
				if (rsc.surface && test_framebuffer(texaddr))
				{
					//TODO: Check that this region is not cpu-dirty before doing a copy
					if (extended_dimension != rsx::texture_dimension_extended::texture_dimension_2d)
					{
						LOG_ERROR(RSX, "Sampling of RTT region as non-2D texture! addr=0x%x, Type=%d, dims=%dx%d",
							texaddr, (u8)tex.get_extended_texture_dimension(), tex.width(), tex.height());
					}
					else
					{
						if (!rsc.is_bound || !g_cfg.video.strict_rendering_mode)
						{
							if (rsc.w == tex_width && rsc.h == tex_height)
							{
								if (rsc.is_bound)
								{
									LOG_WARNING(RSX, "Sampling from a currently bound render target @ 0x%x", texaddr);
									insert_texture_barrier();
								}

								return rsc.surface->get_view();
							}
							else return create_temporary_subresource_view(cmd, rsc.surface, format, rsx::apply_resolution_scale(rsc.x, false), rsx::apply_resolution_scale(rsc.y, false),
								rsx::apply_resolution_scale(rsc.w, true), rsx::apply_resolution_scale(rsc.h, true));
						}
						else
						{
							LOG_WARNING(RSX, "Attempting to sample a currently bound render target @ 0x%x", texaddr);
							return create_temporary_subresource_view(cmd, rsc.surface, format, rsx::apply_resolution_scale(rsc.x, false), rsx::apply_resolution_scale(rsc.y, false),
								rsx::apply_resolution_scale(rsc.w, true), rsx::apply_resolution_scale(rsc.h, true));
						}
					}
				}
			}

			{
				//Search in cache and upload/bind
				reader_lock lock(m_cache_mutex);

				auto cached_texture = find_texture_from_dimensions(texaddr, tex_width, tex_height, depth);
				if (cached_texture)
				{
					return cached_texture->get_raw_view();
				}

				if ((!blit_engine_incompatibility_warning_raised && g_cfg.video.use_gpu_texture_scaling) || is_hw_blit_engine_compatible(format))
				{
					//Find based on range instead
					auto overlapping_surfaces = find_texture_from_range(texaddr, tex_size);
					if (!overlapping_surfaces.empty())
					{
						for (auto surface : overlapping_surfaces)
						{
							if (surface->get_context() != rsx::texture_upload_context::blit_engine_dst)
								continue;

							if (surface->get_width() >= tex_width && surface->get_height() >= tex_height)
							{
								u16 offset_x = 0, offset_y = 0;
								if (const u32 address_offset = texaddr - surface->get_section_base())
								{
									const auto bpp = get_format_block_size_in_bytes(format);
									offset_y = address_offset / tex_pitch;
									offset_x = (address_offset % tex_pitch) / bpp;
								}

								if ((offset_x + tex_width) <= surface->get_width() &&
									(offset_y + tex_height) <= surface->get_height())
								{
									if (extended_dimension != rsx::texture_dimension_extended::texture_dimension_2d)
									{
										LOG_ERROR(RSX, "Texture resides in blit engine memory, but requested type is not 2D (%d)", (u32)extended_dimension);
										break;
									}

									if (!blit_engine_incompatibility_warning_raised && !is_hw_blit_engine_compatible(format))
									{
										LOG_ERROR(RSX, "Format 0x%X is not compatible with the hardware blit acceleration."
											" Consider turning off GPU texture scaling in the options to partially handle textures on your CPU.", format);
										blit_engine_incompatibility_warning_raised = true;
										break;
									}

									auto src_image = surface->get_raw_texture();
									if (auto result = create_temporary_subresource_view(cmd, &src_image, format, offset_x, offset_y, tex_width, tex_height))
										return result;
								}
							}
						}
					}
				}
			}

			//Do direct upload from CPU as the last resort
			writer_lock lock(m_cache_mutex);
			const bool is_swizzled = !(tex.format() & CELL_GCM_TEXTURE_LN);
			auto subresources_layout = get_subresources_layout(tex);
			auto remap_vector = tex.decoded_remap();

			//Invalidate with writing=false, discard=false, rebuild=false, native_flush=true
			invalidate_range_impl_base(texaddr, tex_size, false, false, false, true, std::forward<Args>(extras)...);

			m_texture_memory_in_use += (tex_pitch * tex_height);
			return upload_image_from_cpu(cmd, texaddr, tex_width, tex_height, depth, tex.get_exact_mipmap_count(), tex_pitch, format,
				texture_upload_context::shader_read, subresources_layout, extended_dimension, is_swizzled, remap_vector)->get_raw_view();
		}

		template <typename surface_store_type, typename blitter_type, typename ...Args>
		bool upload_scaled_image(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, commandbuffer_type& cmd, surface_store_type& m_rtts, blitter_type& blitter, Args&&... extras)
		{
			//Since we will have dst in vram, we can 'safely' ignore the swizzle flag
			//TODO: Verify correct behavior
			bool is_depth_blit = false;
			bool src_is_render_target = false;
			bool dst_is_render_target = false;
			bool dst_is_argb8 = (dst.format == rsx::blit_engine::transfer_destination_format::a8r8g8b8);
			bool src_is_argb8 = (src.format == rsx::blit_engine::transfer_source_format::a8r8g8b8);

			image_resource_type vram_texture = 0;
			image_resource_type dest_texture = 0;

			const u32 src_address = (u32)((u64)src.pixels - (u64)vm::base(0));
			const u32 dst_address = (u32)((u64)dst.pixels - (u64)vm::base(0));

			u32 framebuffer_src_address = src_address;

			float scale_x = dst.scale_x;
			float scale_y = dst.scale_y;

			//TODO: Investigate effects of compression in X axis
			if (dst.compressed_y)
				scale_y *= 0.5f;

			if (src.compressed_y)
				scale_y *= 2.f;

			//Offset in x and y for src is 0 (it is already accounted for when getting pixels_src)
			//Reproject final clip onto source...
			const u16 src_w = (const u16)((f32)dst.clip_width / scale_x);
			const u16 src_h = (const u16)((f32)dst.clip_height / scale_y);

			//Correct for tile compression
			//TODO: Investigate whether DST compression also affects alignment
			if (src.compressed_x || src.compressed_y)
			{
				const u32 x_bytes = src_is_argb8 ? (src.offset_x * 4) : (src.offset_x * 2);
				const u32 y_bytes = src.pitch * src.offset_y;

				if (src.offset_x <= 16 && src.offset_y <= 16)
					framebuffer_src_address -= (x_bytes + y_bytes);
			}

			//Check if src/dst are parts of render targets
			auto dst_subres = m_rtts.get_surface_subresource_if_applicable(dst_address, dst.width, dst.clip_height, dst.pitch, true, true, false, dst.compressed_y);
			dst_is_render_target = dst_subres.surface != nullptr;

			if (dst_is_render_target && dst_subres.surface->get_native_pitch() != dst.pitch)
			{
				//Surface pitch is invalid if it is less that the rsx pitch (usually set to 64 in such a case)
				if (dst_subres.surface->get_rsx_pitch() != dst.pitch ||
					dst.pitch < dst_subres.surface->get_native_pitch())
					dst_is_render_target = false;
			}

			//TODO: Handle cases where src or dst can be a depth texture while the other is a color texture - requires a render pass to emulate
			auto src_subres = m_rtts.get_surface_subresource_if_applicable(framebuffer_src_address, src_w, src_h, src.pitch, true, true, false, src.compressed_y);
			src_is_render_target = src_subres.surface != nullptr;

			if (src_is_render_target && src_subres.surface->get_native_pitch() != src.pitch)
			{
				//Surface pitch is invalid if it is less that the rsx pitch (usually set to 64 in such a case)
				if (src_subres.surface->get_rsx_pitch() != src.pitch ||
					src.pitch < src_subres.surface->get_native_pitch())
					src_is_render_target = false;
			}

			//Always use GPU blit if src or dst is in the surface store
			if (!g_cfg.video.use_gpu_texture_scaling && !(src_is_render_target || dst_is_render_target))
				return false;

			reader_lock lock(m_cache_mutex);

			//Check if trivial memcpy can perform the same task
			//Used to copy programs to the GPU in some cases
			if (!src_is_render_target && !dst_is_render_target && dst_is_argb8 == src_is_argb8 && !dst.swizzled)
			{
				if ((src.slice_h == 1 && dst.clip_height == 1) ||
					(dst.clip_width == src.width && dst.clip_height == src.slice_h && src.pitch == dst.pitch))
				{
					const u8 bpp = dst_is_argb8 ? 4 : 2;
					const u32 memcpy_bytes_length = dst.clip_width * bpp * dst.clip_height;

					lock.upgrade();
					invalidate_range_impl(src_address, memcpy_bytes_length, false, false, true, std::forward<Args>(extras)...);
					invalidate_range_impl(dst_address, memcpy_bytes_length, true, false, true, std::forward<Args>(extras)...);
					memcpy(dst.pixels, src.pixels, memcpy_bytes_length);
					return true;
				}
			}

			u16 max_dst_width = dst.width;
			u16 max_dst_height = dst.height;
			areai src_area = { 0, 0, src_w, src_h };
			areai dst_area = { 0, 0, dst.clip_width, dst.clip_height };

			//1024 height is a hack (for ~720p buffers)
			//It is possible to have a large buffer that goes up to around 4kx4k but anything above 1280x720 is rare
			//RSX only handles 512x512 tiles so texture 'stitching' will eventually be needed to be completely accurate
			//Sections will be submitted as (512x512 + 512x512 + 256x512 + 512x208 + 512x208 + 256x208) to blit a 720p surface to the backbuffer for example
			size2i dst_dimensions = { dst.pitch / (dst_is_argb8 ? 4 : 2), dst.height };
			if (src_is_render_target)
			{
				if (dst_dimensions.width == src_subres.surface->get_surface_width())
					dst_dimensions.height = std::max(src_subres.surface->get_surface_height(), dst.height);
				else if (dst.max_tile_h > dst.height)
					dst_dimensions.height = std::min((s32)dst.max_tile_h, 1024);
			}

			section_storage_type* cached_dest = nullptr;
			bool invalidate_dst_range = false;

			if (!dst_is_render_target)
			{
				//Check for any available region that will fit this one
				auto overlapping_surfaces = find_texture_from_range(dst_address, dst.pitch * dst.clip_height);

				for (auto surface: overlapping_surfaces)
				{
					if (surface->get_context() != rsx::texture_upload_context::blit_engine_dst)
						continue;

					const auto old_dst_area = dst_area;
					if (const u32 address_offset = dst_address - surface->get_section_base())
					{
						const u16 bpp = dst_is_argb8 ? 4 : 2;
						const u16 offset_y = address_offset / dst.pitch;
						const u16 offset_x = address_offset % dst.pitch;
						const u16 offset_x_in_block = offset_x / bpp;

						dst_area.x1 += offset_x_in_block;
						dst_area.x2 += offset_x_in_block;
						dst_area.y1 += offset_y;
						dst_area.y2 += offset_y;
					}

					//Validate clipping region
					if ((unsigned)dst_area.x2 <= surface->get_width() &&
						(unsigned)dst_area.y2 <= surface->get_height())
					{
						cached_dest = surface;
						break;
					}
					else
					{
						dst_area = old_dst_area;
					}
				}

				if (cached_dest)
				{
					dest_texture = cached_dest->get_raw_texture();

					max_dst_width = cached_dest->get_width();
					max_dst_height = cached_dest->get_height();

					//Prep surface
					enforce_surface_creation_type(*cached_dest, dst.swizzled ? rsx::texture_create_flags::swapped_native_component_order : rsx::texture_create_flags::native_component_order);
				}
				else if (overlapping_surfaces.size() > 0)
				{
					invalidate_dst_range = true;
				}
			}
			else
			{
				//TODO: Investigate effects of tile compression

				dst_area.x1 = dst_subres.x;
				dst_area.y1 = dst_subres.y;
				dst_area.x2 += dst_subres.x;
				dst_area.y2 += dst_subres.y;

				dest_texture = dst_subres.surface->get_surface();

				max_dst_width = dst_subres.surface->get_surface_width();
				max_dst_height = dst_subres.surface->get_surface_height();
			}

			//Create source texture if does not exist
			if (!src_is_render_target)
			{
				auto preloaded_texture = find_texture_from_dimensions(src_address, src.width, src.slice_h);

				if (preloaded_texture != nullptr)
				{
					vram_texture = preloaded_texture->get_raw_texture();
				}
				else
				{
					lock.upgrade();

					invalidate_range_impl(src_address, src.pitch * src.slice_h, false, false, true, std::forward<Args>(extras)...);

					const u16 pitch_in_block = src_is_argb8 ? src.pitch >> 2 : src.pitch >> 1;
					std::vector<rsx_subresource_layout> subresource_layout;
					rsx_subresource_layout subres = {};
					subres.width_in_block = src.width;
					subres.height_in_block = src.slice_h;
					subres.pitch_in_bytes = pitch_in_block;
					subres.depth = 1;
					subres.data = { (const gsl::byte*)src.pixels, src.pitch * src.slice_h };
					subresource_layout.push_back(subres);

					const u32 gcm_format = src_is_argb8 ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;
					vram_texture = upload_image_from_cpu(cmd, src_address, src.width, src.slice_h, 1, 1, src.pitch, gcm_format, texture_upload_context::blit_engine_src,
						subresource_layout, rsx::texture_dimension_extended::texture_dimension_2d, dst.swizzled, default_remap_vector)->get_raw_texture();

					m_texture_memory_in_use += src.pitch * src.slice_h;
				}
			}
			else
			{
				if (src_subres.w != dst.clip_width ||
					src_subres.h != dst.clip_height)
				{
					f32 subres_scaling_x = (f32)src.pitch / src_subres.surface->get_native_pitch();

					const int dst_width = (int)(src_subres.w * scale_x * subres_scaling_x);
					const int dst_height = (int)(src_subres.h * scale_y);

					dst_area.x2 = dst_area.x1 + dst_width;
					dst_area.y2 = dst_area.y1 + dst_height;
				}

				src_area.x2 = src_subres.w;
				src_area.y2 = src_subres.h;

				src_area.x1 = src_subres.x;
				src_area.y1 = src_subres.y;
				src_area.x2 += src_subres.x;
				src_area.y2 += src_subres.y;

				vram_texture = src_subres.surface->get_surface();
			}

			bool format_mismatch = false;

			if (src_subres.is_depth_surface)
			{
				if (dest_texture)
				{
					if (dst_is_render_target)
					{
						if (!dst_subres.is_depth_surface)
						{
							LOG_ERROR(RSX, "Depth->RGBA blit requested but not supported");
							return true;
						}
					}
					else
					{
						if (!cached_dest->has_compatible_format(src_subres.surface))
							format_mismatch = true;
					}
				}

				is_depth_blit = true;
			}

			//TODO: Check for other types of format mismatch
			if (format_mismatch)
			{
				lock.upgrade();
				invalidate_range_impl(cached_dest->get_section_base(), cached_dest->get_section_size(), true, false, true, std::forward<Args>(extras)...);

				dest_texture = 0;
				cached_dest = nullptr;
			}
			else if (invalidate_dst_range)
			{
				lock.upgrade();
				invalidate_range_impl(dst_address, dst.pitch * dst.height, true, false, true, std::forward<Args>(extras)...);
			}

			//Validate clipping region
			if ((dst.offset_x + dst.clip_x + dst.clip_width) > max_dst_width) dst.clip_x = 0;
			if ((dst.offset_y + dst.clip_y + dst.clip_height) > max_dst_height) dst.clip_y = 0;

			//Reproject clip offsets onto source to simplify blit
			if (dst.clip_x || dst.clip_y)
			{
				const u16 scaled_clip_offset_x = (const u16)((f32)dst.clip_x / scale_x);
				const u16 scaled_clip_offset_y = (const u16)((f32)dst.clip_y / scale_y);

				src_area.x1 += scaled_clip_offset_x;
				src_area.x2 += scaled_clip_offset_x;
				src_area.y1 += scaled_clip_offset_y;
				src_area.y2 += scaled_clip_offset_y;
			}

			if (dest_texture == 0)
			{
				u32 gcm_format;
				if (is_depth_blit)
					gcm_format = (dst_is_argb8) ? CELL_GCM_TEXTURE_DEPTH24_D8 : CELL_GCM_TEXTURE_DEPTH16;
				else
					gcm_format = (dst_is_argb8) ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;

				lock.upgrade();

				dest_texture = create_new_texture(cmd, dst.rsx_address, dst.pitch * dst_dimensions.height,
					dst_dimensions.width, dst_dimensions.height, 1, 1,
					gcm_format, rsx::texture_upload_context::blit_engine_dst, rsx::texture_dimension_extended::texture_dimension_2d,
					dst.swizzled? rsx::texture_create_flags::swapped_native_component_order : rsx::texture_create_flags::native_component_order,
					default_remap_vector)->get_raw_texture();

				m_texture_memory_in_use += dst.pitch * dst_dimensions.height;
			}

			const f32 scale = rsx::get_resolution_scale();
			if (src_is_render_target)
				src_area = src_area * scale;

			if (dst_is_render_target)
				dst_area = dst_area * scale;

			blitter.scale_image(vram_texture, dest_texture, src_area, dst_area, interpolate, is_depth_blit);
			return true;
		}

		virtual const u32 get_unreleased_textures_count() const
		{
			return m_unreleased_texture_objects;
		}

		virtual const u32 get_texture_memory_in_use() const
		{
			return m_texture_memory_in_use;
		}

		void tag_framebuffer(u32 texaddr)
		{
			if (!g_cfg.video.strict_rendering_mode)
				return;

			switch (get_memory_protection(texaddr))
			{
			case utils::protection::no:
				return;
			case utils::protection::ro:
				LOG_ERROR(RSX, "Framebuffer memory occupied by regular texture!");
				return;
			}

			vm::ps3::write32(texaddr, texaddr);
		}

		bool test_framebuffer(u32 texaddr)
		{
			if (!g_cfg.video.strict_rendering_mode)
				return true;

			if (g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer)
			{
				if (get_memory_protection(texaddr) == utils::protection::no)
					return true;
			}

			return vm::ps3::read32(texaddr) == texaddr;
		}
	};
}
