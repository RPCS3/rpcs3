#pragma once

#include "../rsx_cache.h"
#include "../rsx_utils.h"

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
		blit_engine_dst = 2
	};

	struct cached_texture_section : public rsx::buffered_section
	{
		u16 width;
		u16 height;
		u16 depth;
		u16 mipmaps;

		u16 real_pitch;
		u16 rsx_pitch;

		rsx::texture_create_flags view_flags = rsx::texture_create_flags::default_component_order;
		rsx::texture_upload_context context = rsx::texture_upload_context::shader_read;

		bool matches(const u32 rsx_address, const u32 rsx_size)
		{
			return rsx::buffered_section::matches(rsx_address, rsx_size);
		}

		bool matches(const u32 rsx_address, const u32 width, const u32 height, const u32 mipmaps)
		{
			if (rsx_address == cpu_address_base)
			{
				if (!width && !height && !mipmaps)
					return true;

				if (width && width != this->width)
					return false;

				if (height && height != this->height)
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

		u32 get_width() const
		{
			return width;
		}

		u32 get_height() const
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
				max_range = std::max(data_size, max_range);
				max_addr = std::max(max_addr, addr);
				min_addr = std::min(min_addr, addr);
				valid_count++;
			}

			void add(section_storage_type& section, u32 addr, u32 data_size)
			{
				verify(HERE), valid_count >= 0;
				max_range = std::max(data_size, max_range);
				max_addr = std::max(max_addr, addr);
				min_addr = std::min(min_addr, addr);
				valid_count++;

				data.push_back(std::move(section));
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
		const s32 m_max_zombie_objects = 128; //Limit on how many texture objects to keep around for reuse after they are invalidated
		s32 m_unreleased_texture_objects = 0; //Number of invalidated objects not yet freed from memory
		
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

	private:
		//Internal implementation methods
		bool invalidate_range_impl(u32 address, u32 range, bool unprotect)
		{
			bool response = false;
			u32 last_dirty_block = UINT32_MAX;
			std::pair<u32, u32> trampled_range = std::make_pair(address, address + range);

			for (auto It = m_cache.begin(); It != m_cache.end(); It++)
			{
				auto &range_data = It->second;
				const u32 base = It->first;
				bool range_reset = false;

				if (base == last_dirty_block && range_data.valid_count == 0)
					continue;

				if (trampled_range.first < trampled_range.second)
				{
					//Only if a valid range, ignore empty sets
					if (trampled_range.first >= (range_data.max_addr + range_data.max_range) || range_data.min_addr >= trampled_range.second)
						continue;
				}

				for (int i = 0; i < range_data.data.size(); i++)
				{
					auto &tex = range_data.data[i];

					if (tex.is_dirty()) continue;
					if (!tex.is_locked()) continue;	//flushable sections can be 'clean' but unlocked. TODO: Handle this better

					auto overlapped = tex.overlaps_page(trampled_range, address);
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

						if (unprotect)
						{
							tex.set_dirty(true);
							tex.unprotect();
						}
						else
						{
							tex.discard();
						}

						m_unreleased_texture_objects++;
						range_data.remove_one();
						response = true;
					}
				}

				if (range_reset)
				{
					last_dirty_block = base;
					It = m_cache.begin();
				}
			}

			return response;
		}

		template <typename ...Args>
		bool flush_address_impl(u32 address, Args&&... extras)
		{
			bool response = false;
			u32 last_dirty_block = UINT32_MAX;
			std::pair<u32, u32> trampled_range = std::make_pair(0xffffffff, 0x0);
			std::vector<section_storage_type*> sections_to_flush;

			for (auto It = m_cache.begin(); It != m_cache.end(); It++)
			{
				auto &range_data = It->second;
				const u32 base = It->first;
				bool range_reset = false;

				if (base == last_dirty_block && range_data.valid_count == 0)
					continue;

				if (trampled_range.first < trampled_range.second)
				{
					//Only if a valid range, ignore empty sets
					if (trampled_range.first >= (range_data.max_addr + range_data.max_range) || range_data.min_addr >= trampled_range.second)
						continue;
				}

				for (int i = 0; i < range_data.data.size(); i++)
				{
					auto &tex = range_data.data[i];

					if (tex.is_dirty()) continue;
					if (!tex.is_flushable()) continue;

					auto overlapped = tex.overlaps_page(trampled_range, address);
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

						//Defer actual flush operation until all affected regions are cleared to prevent recursion
						tex.unprotect();
						sections_to_flush.push_back(&tex);

						response = true;
						range_data.remove_one();
					}
				}

				if (range_reset)
				{
					It = m_cache.begin();
				}
			}

			for (auto tex : sections_to_flush)
			{
				if (!tex->flush(std::forward<Args>(extras)...))
				{
					//Missed address, note this
					//TODO: Lower severity when successful to keep the cache from overworking
					record_cache_miss(*tex);
				}
			}

			return response;
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

		section_storage_type *find_texture_from_dimensions(u32 rsx_address, u16 width = 0, u16 height = 0, u16 mipmaps = 0)
		{
			auto found = m_cache.find(get_block_address(rsx_address));
			if (found != m_cache.end())
			{
				auto &range_data = found->second;
				for (auto &tex : range_data.data)
				{
					if (tex.matches(rsx_address, width, height, mipmaps) && !tex.is_dirty())
					{
						return &tex;
					}
				}
			}

			return nullptr;
		}

		section_storage_type& find_cached_texture(u32 rsx_address, u32 rsx_size, bool confirm_dimensions = false, u16 width = 0, u16 height = 0, u16 mipmaps = 0)
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
						if (!confirm_dimensions || tex.matches(rsx_address, width, height, mipmaps))
						{
							if (!tex.is_locked())
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
		bool flush_address(u32 address, Args&&... extras)
		{
			if (address < no_access_range.first ||
				address > no_access_range.second)
				return false;

			writer_lock lock(m_cache_mutex);
			return flush_address_impl(address, std::forward<Args>(extras)...);
		}

		bool invalidate_address(u32 address)
		{
			return invalidate_range(address, 4096 - (address & 4095));
		}

		bool invalidate_range(u32 address, u32 range, bool unprotect = true)
		{
			std::pair<u32, u32> trampled_range = std::make_pair(address, address + range);

			if (trampled_range.second < read_only_range.first ||
				trampled_range.first > read_only_range.second)
			{
				//Doesnt fall in the read_only textures range; check render targets
				if (trampled_range.second < no_access_range.first ||
					trampled_range.first > no_access_range.second)
					return false;
			}

			writer_lock lock(m_cache_mutex);
			return invalidate_range_impl(address, range, unprotect);
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
				}
			}

			//Free descriptor objects as well
			for (const auto &address : empty_addresses)
			{
				m_cache.erase(address);
			}
			
			m_unreleased_texture_objects = 0;
		}

		template <typename RsxTextureType, typename surface_store_type>
		image_view_type upload_texture(commandbuffer_type& cmd, RsxTextureType& tex, surface_store_type& m_rtts)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 tex_size = (u32)get_texture_size(tex);

			const u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			const u32 tex_width = tex.width();
			const u32 tex_height = tex.height();
			const u32 tex_pitch = (tex_size / tex_height); //NOTE: Compressed textures dont have a real pitch (tex_size = (w*h)/6)

			if (!texaddr || !tex_size)
			{
				LOG_ERROR(RSX, "Texture upload requested but texture not found, (address=0x%X, size=0x%X)", texaddr, tex_size);
				return 0;
			}

			//Check for sampleable rtts from previous render passes
			if (auto texptr = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
			{
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

			if (auto texptr = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
			{
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

			{
				//Search in cache and upload/bind
				reader_lock lock(m_cache_mutex);

				auto cached_texture = find_texture_from_dimensions(texaddr, tex_width, tex_height);
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

			/* Check if we are re-sampling a subresource of an RTV/DSV texture, bound or otherwise
			 * (Turbo: Super Stunt Squad does this; bypassing the need for a sync object)
			 * The engine does not read back the texture resource through cell, but specifies a texture location that is
			 * a bound render target. We can bypass the expensive download in this case
			 */

			const u32 native_pitch = tex_width * get_format_block_size_in_bytes(format);
			const f32 internal_scale = (f32)tex_pitch / native_pitch;
			const u32 internal_width = (const u32)(tex_width * internal_scale);

			const auto rsc = m_rtts.get_surface_subresource_if_applicable(texaddr, internal_width, tex_height, tex_pitch, true);
			if (rsc.surface)
			{
				//TODO: Check that this region is not cpu-dirty before doing a copy
				if (tex.get_extended_texture_dimension() != rsx::texture_dimension_extended::texture_dimension_2d)
				{
					LOG_ERROR(RSX, "Sampling of RTT region as non-2D texture! addr=0x%x, Type=%d, dims=%dx%d",
						texaddr, (u8)tex.get_extended_texture_dimension(), tex.width(), tex.height());
				}
				else
				{
					if (format == CELL_GCM_TEXTURE_COMPRESSED_DXT1 || format == CELL_GCM_TEXTURE_COMPRESSED_DXT23 || format == CELL_GCM_TEXTURE_COMPRESSED_DXT45)
					{
						LOG_WARNING(RSX, "Performing an RTT blit but request is for a compressed texture");
					}

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
						else
							return create_temporary_subresource_view(cmd, rsc.surface, format, rsc.x, rsc.y, rsc.w, rsc.h);
					}
					else
					{
						LOG_WARNING(RSX, "Attempting to sample a currently bound render target @ 0x%x", texaddr);
						return create_temporary_subresource_view(cmd, rsc.surface, format, rsc.x, rsc.y, rsc.w, rsc.h);
					}
				}
			}

			//Do direct upload from CPU as the last resort
			const auto extended_dimension = tex.get_extended_texture_dimension();
			u16 height = 0;
			u16 depth = 0;

			switch (extended_dimension)
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				height = 1;
				depth = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				height = tex_height;
				depth = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				height = tex_height;
				depth = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_3d:
				height = tex_height;
				depth = tex.depth();
				break;
			}

			writer_lock lock(m_cache_mutex);
			const bool is_swizzled = !(tex.format() & CELL_GCM_TEXTURE_LN);
			auto subresources_layout = get_subresources_layout(tex);
			auto remap_vector = tex.decoded_remap();

			return upload_image_from_cpu(cmd, texaddr, tex_width, height, depth, tex.get_exact_mipmap_count(), tex_pitch, format,
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

			//Correct for tile compression
			//TODO: Investigate whether DST compression also affects alignment
			if (src.compressed_x || src.compressed_y)
			{
				const u32 x_bytes = src_is_argb8 ? (src.offset_x * 4) : (src.offset_x * 2);
				const u32 y_bytes = src.pitch * src.offset_y;

				if (src.offset_x <= 16 && src.offset_y <= 16)
					framebuffer_src_address -= (x_bytes + y_bytes);
			}

			u16 max_dst_width = dst.width;
			u16 max_dst_height = dst.height;

			float scale_x = dst.scale_x;
			float scale_y = dst.scale_y;

			//TODO: Investigate effects of compression in X axis
			if (dst.compressed_y)
			{
				scale_y *= 0.5f;
			}

			if (src.compressed_y)
			{
				scale_y *= 2.f;
			}

			//Offset in x and y for src is 0 (it is already accounted for when getting pixels_src)
			//Reproject final clip onto source...
			const u16 src_w = (const u16)((f32)dst.clip_width / scale_x);
			const u16 src_h = (const u16)((f32)dst.clip_height / scale_y);

			areai src_area = { 0, 0, src_w, src_h };
			areai dst_area = { 0, 0, dst.clip_width, dst.clip_height };

			//Check if src/dst are parts of render targets
			auto dst_subres = m_rtts.get_surface_subresource_if_applicable(dst_address, dst.width, dst.clip_height, dst.pitch, true, true, false, dst.compressed_y);
			dst_is_render_target = dst_subres.surface != nullptr;

			//TODO: Handle cases where src or dst can be a depth texture while the other is a color texture - requires a render pass to emulate
			auto src_subres = m_rtts.get_surface_subresource_if_applicable(framebuffer_src_address, src_w, src_h, src.pitch, true, true, false, src.compressed_y);
			src_is_render_target = src_subres.surface != nullptr;

			//Always use GPU blit if src or dst is in the surface store
			if (!g_cfg.video.use_gpu_texture_scaling && !(src_is_render_target || dst_is_render_target))
				return false;

			//1024 height is a hack (for ~720p buffers)
			//It is possible to have a large buffer that goes up to around 4kx4k but anything above 1280x720 is rare
			//RSX only handles 512x512 tiles so texture 'stitching' will eventually be needed to be completely accurate
			//Sections will be submitted as (512x512 + 512x512 + 256x512 + 512x208 + 512x208 + 256x208) to blit a 720p surface to the backbuffer for example
			int practical_height;
			if (dst.max_tile_h < dst.height || !src_is_render_target)
				practical_height = (s32)dst.height;
			else
			{
				//Hack
				practical_height = std::min((s32)dst.max_tile_h, 1024);
			}

			size2i dst_dimensions = { dst.pitch / (dst_is_argb8 ? 4 : 2), practical_height };

			//Check if trivial memcpy can perform the same task
			//Used to copy programs to the GPU in some cases
			bool is_memcpy = false;
			u32 memcpy_bytes_length = 0;

			if (!src_is_render_target && !dst_is_render_target && dst_is_argb8 == src_is_argb8 && !dst.swizzled)
			{
				if ((src.slice_h == 1 && dst.clip_height == 1) ||
					(dst.clip_width == src.width && dst.clip_height == src.slice_h && src.pitch == dst.pitch))
				{
					const u8 bpp = dst_is_argb8 ? 4 : 2;
					is_memcpy = true;
					memcpy_bytes_length = dst.clip_width * bpp * dst.clip_height;
				}
			}

			reader_lock lock(m_cache_mutex);
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
				else if (is_memcpy)
				{
					lock.upgrade();
					flush_address_impl(src_address, std::forward<Args>(extras)...);
					invalidate_range_impl(dst_address, memcpy_bytes_length, true);
					memcpy(dst.pixels, src.pixels, memcpy_bytes_length);
					return true;
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

				if (is_memcpy)
				{
					//Some render target descriptions are actually invalid
					//Confirm this is a flushable RTT
					const auto rsx_pitch = dst_subres.surface->get_rsx_pitch();
					const auto native_pitch = dst_subres.surface->get_native_pitch();

					if (rsx_pitch <= 64 && native_pitch != rsx_pitch)
					{
						lock.upgrade();
						flush_address_impl(src_address, std::forward<Args>(extras)...);
						invalidate_range_impl(dst_address, memcpy_bytes_length, true);
						memcpy(dst.pixels, src.pixels, memcpy_bytes_length);
						return true;
					}
				}
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

					flush_address_impl(src_address, std::forward<Args>(extras)...);

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
					if (dst_is_render_target && !dst_subres.is_depth_surface)
					{
						LOG_ERROR(RSX, "Depth->RGBA blit requested but not supported");
						return true;
					}

					if (!cached_dest->has_compatible_format(src_subres.surface))
						format_mismatch = true;
				}

				is_depth_blit = true;
			}

			//TODO: Check for other types of format mismatch
			if (format_mismatch)
			{
				lock.upgrade();
				invalidate_range_impl(cached_dest->get_section_base(), cached_dest->get_section_size(), true);

				dest_texture = 0;
				cached_dest = nullptr;
			}
			else if (invalidate_dst_range)
			{
				lock.upgrade();
				invalidate_range_impl(dst_address, dst.pitch * dst.height, true);
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
			}

			blitter.scale_image(vram_texture, dest_texture, src_area, dst_area, interpolate, is_depth_blit);
			return true;
		}
	};
}
