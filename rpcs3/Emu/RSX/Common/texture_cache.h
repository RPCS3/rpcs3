#pragma once

#include "../rsx_cache.h"
#include "../rsx_utils.h"
#include "TextureUtils.h"

#include <atomic>

extern u64 get_system_time();

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

	//Sampled image descriptor
	struct sampled_image_descriptor_base
	{
		texture_upload_context upload_context = texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = texture_dimension_extended::texture_dimension_2d;
		bool is_depth_texture = false;
		f32 scale_x = 1.f;
		f32 scale_y = 1.f;
	};

	struct cached_texture_section : public rsx::buffered_section
	{
		u16 width;
		u16 height;
		u16 depth;
		u16 mipmaps;

		u16 real_pitch;
		u16 rsx_pitch;

		u32 gcm_format = 0;

		u64 cache_tag = 0;

		rsx::texture_create_flags view_flags = rsx::texture_create_flags::default_component_order;
		rsx::texture_upload_context context = rsx::texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = rsx::texture_dimension_extended::texture_dimension_2d;

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

		void set_image_type(const rsx::texture_dimension_extended type)
		{
			image_type = type;
		}

		void set_gcm_format(u32 format)
		{
			gcm_format = format;
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

		rsx::texture_dimension_extended get_image_type() const
		{
			return image_type;
		}

		u32 get_gcm_format() const
		{
			return gcm_format;
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

	public:
		//Struct to hold data on sections to be paged back onto cpu memory
		struct thrashed_set
		{
			bool violation_handled = false;
			std::vector<section_storage_type*> sections_to_flush; //Sections to be flushed
			std::vector<section_storage_type*> sections_to_reprotect; //Sections to be protected after flushing
			std::vector<section_storage_type*> sections_to_unprotect; //These sections are to be unpotected and discarded by caller
			int num_flushable = 0;
			u64 cache_tag = 0;
			u32 address_base = 0;
			u32 address_range = 0;
		};

		struct deferred_subresource
		{
			image_resource_type external_handle = 0;
			std::array<image_resource_type, 6> external_cubemap_sources;
			u32 base_address = 0;
			u32 gcm_format = 0;
			u16 x = 0;
			u16 y = 0;
			u16 width = 0;
			u16 height = 0;
			bool is_cubemap = false;

			deferred_subresource()
			{}

			deferred_subresource(image_resource_type _res, u32 _addr, u32 _fmt, u16 _x, u16 _y, u16 _w, u16 _h) :
				external_handle(_res), base_address(_addr), gcm_format(_fmt), x(_x), y(_y), width(_w), height(_h)
			{}
		};

		struct sampled_image_descriptor : public sampled_image_descriptor_base
		{
			image_view_type image_handle = 0;
			deferred_subresource external_subresource_desc = {};
			bool flag = false;

			sampled_image_descriptor()
			{}

			sampled_image_descriptor(image_view_type handle, const texture_upload_context ctx, const bool is_depth, const f32 x_scale, const f32 y_scale, const rsx::texture_dimension_extended type)
			{
				image_handle = handle;
				upload_context = ctx;
				is_depth_texture = is_depth;
				scale_x = x_scale;
				scale_y = y_scale;
				image_type = type;
			}

			sampled_image_descriptor(image_resource_type external_handle, u32 base_address, u32 gcm_format, u16 x_offset, u16 y_offset, u16 width, u16 height,
				const texture_upload_context ctx, const bool is_depth, const f32 x_scale, const f32 y_scale, const rsx::texture_dimension_extended type)
			{
				external_subresource_desc = { external_handle, base_address, gcm_format, x_offset, y_offset, width, height };

				image_handle = 0;
				upload_context = ctx;
				is_depth_texture = is_depth;
				scale_x = x_scale;
				scale_y = y_scale;
				image_type = type;
			}

			void set_external_cubemap_resources(std::array<image_resource_type, 6> images)
			{
				external_subresource_desc.external_cubemap_sources = images;
				external_subresource_desc.is_cubemap = true;
			}
		};

	protected:
		shared_mutex m_cache_mutex;
		std::unordered_map<u32, ranged_storage> m_cache;
		std::unordered_multimap<u32, std::pair<deferred_subresource, image_view_type>> m_temporary_subresource_cache;

		std::atomic<u64> m_cache_update_tag = {0};

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
		virtual image_view_type generate_cubemap_from_images(commandbuffer_type&, const u32 gcm_format, u16 size, std::array<image_resource_type, 6>& sources) = 0;

		constexpr u32 get_block_size() const { return 0x1000000; }
		inline u32 get_block_address(u32 address) const { return (address & ~0xFFFFFF); }

		inline void update_cache_tag()
		{
			m_cache_update_tag++;
		}

	private:
		//Internal implementation methods and helpers

		std::pair<utils::protection, section_storage_type*> get_memory_protection(u32 address)
		{
			auto found = m_cache.find(get_block_address(address));
			if (found != m_cache.end())
			{
				for (auto &tex : found->second.data)
				{
					if (tex.is_locked() && tex.overlaps(address, false))
						return{ tex.get_protection(), &tex };
				}
			}

			//Get the preceding block and check if any hits are found
			found = m_cache.find(get_block_address(address) - get_block_size());
			if (found != m_cache.end())
			{
				for (auto &tex : found->second.data)
				{
					if (tex.is_locked() && tex.overlaps(address, false))
						return{ tex.get_protection(), &tex };
				}
			}

			return{ utils::protection::rw, nullptr };
		}

		inline bool region_intersects_cache(u32 address, u32 range, bool is_writing) const
		{
			std::pair<u32, u32> test_range = std::make_pair(address, address + range);
			if (!is_writing)
			{
				if (no_access_range.first > no_access_range.second ||
					test_range.second < no_access_range.first ||
					test_range.first > no_access_range.second)
					return false;
			}
			else
			{
				if (test_range.second < read_only_range.first ||
					test_range.first > read_only_range.second)
				{
					//Doesnt fall in the read_only textures range; check render targets
					if (test_range.second < no_access_range.first ||
						test_range.first > no_access_range.second)
						return false;
				}
			}

			return true;
		}

		//Get intersecting set - Returns all objects intersecting a given range and their owning blocks
		std::vector<std::pair<section_storage_type*, ranged_storage*>> get_intersecting_set(u32 address, u32 range)
		{
			std::vector<std::pair<section_storage_type*, ranged_storage*>> result;
			u32 last_dirty_block = UINT32_MAX;
			const u64 cache_tag = get_system_time();

			std::pair<u32, u32> trampled_range = std::make_pair(address, address + range);
			const bool strict_range_check = g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer;

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

					auto overlapped = tex.overlaps_page(trampled_range, address, strict_range_check);
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
			if (!region_intersects_cache(address, range, is_writing))
				return {};

			auto trampled_set = get_intersecting_set(address, range);

			if (trampled_set.size() > 0)
			{
				update_cache_tag();
				bool deferred_flush = false;

				thrashed_set result = {};
				result.violation_handled = true;

				if (!discard_only && !allow_flush)
				{
					for (auto &obj : trampled_set)
					{
						if (obj.first->is_flushable())
						{
							deferred_flush = true;
							break;
						}
					}
				}

				std::vector<utils::protection> reprotections;
				for (auto &obj : trampled_set)
				{
					bool to_reprotect = false;

					if (!deferred_flush && !discard_only)
					{
						if (!is_writing && obj.first->get_protection() != utils::protection::no)
						{
							to_reprotect = true;
						}
						else
						{
							if (rebuild_cache && allow_flush && obj.first->is_flushable())
							{
								const std::pair<u32, u32> null_check = std::make_pair(UINT32_MAX, 0);
								to_reprotect = !std::get<0>(obj.first->overlaps_page(null_check, address, true));
							}
						}
					}

					if (to_reprotect)
					{
						result.sections_to_reprotect.push_back(obj.first);
						reprotections.push_back(obj.first->get_protection());
					}
					else if (obj.first->is_flushable())
					{
						result.sections_to_flush.push_back(obj.first);
					}
					else if (!deferred_flush)
					{
						obj.first->set_dirty(true);
						m_unreleased_texture_objects++;
					}
					else
					{
						result.sections_to_unprotect.push_back(obj.first);
					}

					if (deferred_flush)
						continue;

					if (discard_only)
						obj.first->discard();
					else
						obj.first->unprotect();

					if (!to_reprotect)
					{
						obj.second->remove_one();
					}
				}

				if (deferred_flush)
				{
					result.num_flushable = static_cast<int>(result.sections_to_flush.size());
					result.address_base = address;
					result.address_range = range;
					result.cache_tag = m_cache_update_tag.load(std::memory_order_consume);
					return result;
				}

				if (result.sections_to_flush.size() > 0)
				{
					verify(HERE), allow_flush;

					// Flush here before 'reprotecting' since flushing will write the whole span
					for (const auto &tex : result.sections_to_flush)
					{
						if (!tex->flush(std::forward<Args>(extras)...))
						{
							//Missed address, note this
							//TODO: Lower severity when successful to keep the cache from overworking
							record_cache_miss(*tex);
						}
					}
				}

				int n = 0;
				for (auto &tex: result.sections_to_reprotect)
				{
					tex->discard();
					tex->protect(reprotections[n++]);
					tex->set_dirty(false);
				}

				//Everything has been handled
				result = {};
				result.violation_handled = true;
				return result;
			}

			return {};
		}

		inline bool is_hw_blit_engine_compatible(const u32 format) const
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

		/**
		 * Scaling helpers
		 * - get_native_dimensions() returns w and h for the native texture given rsx dimensions
		 *   on rsx a 512x512 texture with 4x AA is treated as a 1024x1024 texture for example
		 * - get_rsx_dimensions() inverse, return rsx w and h given a real texture w and h
		 * - get_internal_scaling_x/y() returns a scaling factor to be multiplied by 1/size
		 *   when sampling with unnormalized coordinates. tcoords passed to rsx will be in rsx dimensions
		 */
		template <typename T, typename U>
		inline void get_native_dimensions(T &width, T &height, U surface)
		{
			switch (surface->aa_mode)
			{
			case rsx::surface_antialiasing::center_1_sample:
				return;
			case rsx::surface_antialiasing::diagonal_centered_2_samples:
				width /= 2;
				return;
			case rsx::surface_antialiasing::square_centered_4_samples:
			case rsx::surface_antialiasing::square_rotated_4_samples:
				width /= 2;
				height /= 2;
				return;
			}
		}

		template <typename T, typename U>
		inline void get_rsx_dimensions(T &width, T &height, U surface)
		{
			switch (surface->aa_mode)
			{
			case rsx::surface_antialiasing::center_1_sample:
				return;
			case rsx::surface_antialiasing::diagonal_centered_2_samples:
				width *= 2;
				return;
			case rsx::surface_antialiasing::square_centered_4_samples:
			case rsx::surface_antialiasing::square_rotated_4_samples:
				width *= 2;
				height *= 2;
				return;
			}
		}

		template <typename T>
		inline f32 get_internal_scaling_x(T surface)
		{
			switch (surface->aa_mode)
			{
			default:
			case rsx::surface_antialiasing::center_1_sample:
				return 1.f;
			case rsx::surface_antialiasing::diagonal_centered_2_samples:
			case rsx::surface_antialiasing::square_centered_4_samples:
			case rsx::surface_antialiasing::square_rotated_4_samples:
				return 0.5f;
			}
		}

		template <typename T>
		inline f32 get_internal_scaling_y(T surface)
		{
			switch (surface->aa_mode)
			{
			default:
			case rsx::surface_antialiasing::center_1_sample:
			case rsx::surface_antialiasing::diagonal_centered_2_samples:
				return 1.f;
			case rsx::surface_antialiasing::square_centered_4_samples:
			case rsx::surface_antialiasing::square_rotated_4_samples:
				return 0.5f;
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
				std::pair<section_storage_type*, ranged_storage*> best_fit = {};

				for (auto &tex : range_data.data)
				{
					if (tex.matches(rsx_address, rsx_size))
					{
						if (!tex.is_dirty())
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
						else if (!best_fit.first)
						{
							//By grabbing a ref to a matching entry, duplicates are avoided
							best_fit = { &tex, &range_data };
						}
					}
				}

				if (best_fit.first)
				{
					if (best_fit.first->exists())
					{
						if (best_fit.first->get_context() != rsx::texture_upload_context::framebuffer_storage)
							m_texture_memory_in_use -= best_fit.first->get_section_size();

						m_unreleased_texture_objects--;
						free_texture_section(*best_fit.first);
					}

					best_fit.second->notify(rsx_address, rsx_size);
					return *best_fit.first;
				}

				for (auto &tex : range_data.data)
				{
					if (tex.is_dirty())
					{
						if (tex.exists())
						{
							if (tex.get_context() != rsx::texture_upload_context::framebuffer_storage)
								m_texture_memory_in_use -= tex.get_section_size();

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

			if (region.get_context() != texture_upload_context::framebuffer_storage &&
				region.exists())
			{
				//This space was being used for other purposes other than framebuffer storage
				//Delete used resources before attaching it to framebuffer memory
				free_texture_section(region);
				m_texture_memory_in_use -= region.get_section_size();
			}

			if (!region.is_locked())
			{
				region.reset(memory_address, memory_size);
				region.set_dirty(false);
				no_access_range = region.get_min_max(no_access_range);
			}

			region.protect(utils::protection::no);
			region.create(width, height, 1, 1, nullptr, image, pitch, false, std::forward<Args>(extras)...);
			region.set_context(texture_upload_context::framebuffer_storage);
			region.set_image_type(rsx::texture_dimension_extended::texture_dimension_2d);
			update_cache_tag();
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
			//Test before trying to acquire the lock
			const auto range = 4096 - (address & 4095);
			if (!region_intersects_cache(address, range, is_writing))
				return{};

			writer_lock lock(m_cache_mutex);
			return invalidate_range_impl_base(address, range, is_writing, false, true, allow_flush, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		thrashed_set invalidate_range(u32 address, u32 range, bool is_writing, bool discard, bool allow_flush, Args&&... extras)
		{
			//Test before trying to acquire the lock
			if (!region_intersects_cache(address, range, is_writing))
				return {};

			writer_lock lock(m_cache_mutex);
			return invalidate_range_impl_base(address, range, is_writing, discard, false, allow_flush, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		bool flush_all(thrashed_set& data, Args&&... extras)
		{
			writer_lock lock(m_cache_mutex);

			if (m_cache_update_tag.load(std::memory_order_consume) == data.cache_tag)
			{
				std::vector<utils::protection> old_protections;
				for (auto &tex : data.sections_to_reprotect)
				{
					if (tex->is_locked())
					{
						old_protections.push_back(tex->get_protection());
						tex->unprotect();
					}
					else
					{
						old_protections.push_back(utils::protection::rw);
					}
				}

				for (auto &tex : data.sections_to_unprotect)
				{
					if (tex->is_locked())
					{
						tex->set_dirty(true);
						tex->unprotect();
						m_cache[get_block_address(tex->get_section_base())].remove_one();
					}
				}

				//TODO: This bit can cause race conditions if other threads are accessing this memory
				//1. Force readback if surface is not synchronized yet to make unlocked part finish quickly
				for (auto &tex : data.sections_to_flush)
				{
					if (tex->is_locked())
					{
						if (!tex->is_synchronized())
							tex->copy_texture(true, std::forward<Args>(extras)...);

						m_cache[get_block_address(tex->get_section_base())].remove_one();
					}
				}

				//TODO: Acquire global io lock here

				//2. Unprotect all the memory
				for (auto &tex : data.sections_to_flush)
				{
					tex->unprotect();
				}

				//3. Write all the memory
				for (auto &tex : data.sections_to_flush)
				{
					tex->flush(std::forward<Args>(extras)...);
				}

				//Restore protection on the sections to reprotect
				int n = 0;
				for (auto &tex : data.sections_to_reprotect)
				{
					if (old_protections[n] != utils::protection::rw)
					{
						tex->discard();
						tex->protect(old_protections[n++]);
					}
				}
			}
			else
			{
				//The cache contents have changed between the two readings. This means the data held is useless
				update_cache_tag();
				invalidate_range_impl_base(data.address_base, data.address_range, true, false, true, true, std::forward<Args>(extras)...);
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

					if (tex.exists() &&
						tex.get_context() != rsx::texture_upload_context::framebuffer_storage)
					{
						free_texture_section(tex);
						m_texture_memory_in_use -= tex.get_section_size();
					}
				}
			}

			//Free descriptor objects as well
			for (const auto &address : empty_addresses)
			{
				m_cache.erase(address);
			}

			m_unreleased_texture_objects = 0;
		}

		image_view_type create_temporary_subresource(commandbuffer_type &cmd, deferred_subresource& desc)
		{
			const auto found = m_temporary_subresource_cache.equal_range(desc.base_address);
			for (auto It = found.first; It != found.second; ++It)
			{
				const auto& found_desc = It->second.first;
				if (found_desc.external_handle != desc.external_handle ||
					found_desc.is_cubemap != desc.is_cubemap ||
					found_desc.x != desc.x || found_desc.y != desc.y ||
					found_desc.width != desc.width || found_desc.height != desc.height)
					continue;

				return It->second.second;
			}

			image_view_type result = 0;
			if (!desc.is_cubemap)
				result = create_temporary_subresource_view(cmd, &desc.external_handle, desc.gcm_format, desc.x, desc.y, desc.width, desc.height);
			else
				result = generate_cubemap_from_images(cmd, desc.gcm_format, desc.width, desc.external_cubemap_sources);

			if (result)
			{
				m_temporary_subresource_cache.insert({ desc.base_address,{ desc, result } });
			}

			return result;
		}

		void notify_surface_changed(u32 base_address)
		{
			m_temporary_subresource_cache.erase(base_address);
		}

		template <typename render_target_type, typename surface_store_type>
		sampled_image_descriptor process_framebuffer_resource(render_target_type texptr, const u32 texaddr, const u32 gcm_format, surface_store_type& m_rtts,
				const u16 tex_width, const u16 tex_height, const rsx::texture_dimension_extended extended_dimension, const bool is_depth)
		{
			const u32 format = gcm_format & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
			const auto surface_width = texptr->get_surface_width();
			const auto surface_height = texptr->get_surface_height();

			if (extended_dimension != rsx::texture_dimension_extended::texture_dimension_2d &&
				extended_dimension != rsx::texture_dimension_extended::texture_dimension_1d)
			{
				if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_cubemap)
				{
					if (surface_height == (surface_width * 6))
					{
						//TODO: Unwrap long RTT block into 6 images and generate cubemap from them
						LOG_ERROR(RSX, "Unwrapping block rtt to cubemap is unimplemented!");
						return{};
					}

					std::array<image_resource_type, 6> image_array;
					image_array[0] = texptr->get_surface();
					bool safe_cast = true;

					u32 image_size = texptr->get_rsx_pitch() * texptr->get_surface_height();
					u32 image_address = texaddr + image_size;

					for (int n = 1; n < 6; ++n)
					{
						render_target_type img = nullptr;
						image_array[n] = 0;

						if (!!(img = m_rtts.get_texture_from_render_target_if_applicable(image_address)) ||
							!!(img = m_rtts.get_texture_from_depth_stencil_if_applicable(image_address)))
						{
							if (img->get_surface_width() != surface_width ||
								img->get_surface_width() != img->get_surface_height())
							{
								safe_cast = false;
							}
							else
							{
								image_array[n] = img->get_surface();
							}
						}
						else
						{
							safe_cast = false;
						}

						image_address += image_size;
					}

					if (!safe_cast)
					{
						//TODO: Lower to warning
						//TODO: Gather remaining sides from the texture cache or upload from cpu (too slow?)
						LOG_ERROR(RSX, "Could not gather all required surfaces for cubemap generation");
					}

					sampled_image_descriptor desc = { texptr->get_surface(), texaddr, format, 0, 0, rsx::apply_resolution_scale(surface_width, true),
							rsx::apply_resolution_scale(surface_height, true), texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
							rsx::texture_dimension_extended::texture_dimension_cubemap };

					desc.set_external_cubemap_resources(image_array);
					return desc;
				}

				LOG_ERROR(RSX, "Texture resides in render target memory, but requested type is not 2D (%d)", (u32)extended_dimension);
			}

			u32 internal_width = tex_width;
			u32 internal_height = tex_height;
			get_native_dimensions(internal_width, internal_height, texptr);

			if (internal_width > surface_width || internal_height > surface_height)
			{
				//An AA flag is likely missing
				//HACK
				auto aa_mode = texptr->aa_mode;
				if ((internal_width >> 1) == surface_width)
				{
					if (internal_height > surface_height)
						texptr->aa_mode = rsx::surface_antialiasing::square_centered_4_samples;
					else
						texptr->aa_mode = rsx::surface_antialiasing::diagonal_centered_2_samples;

					internal_width = tex_width;
					internal_height = tex_height;
					get_native_dimensions(internal_width, internal_height, texptr);
				}

				internal_width = std::min(internal_width, (u32)surface_width);
				internal_height = std::min(internal_height, (u32)surface_height);
				texptr->aa_mode = aa_mode;
			}

			const bool unnormalized = (gcm_format & CELL_GCM_TEXTURE_UN) != 0;
			f32 scale_x = (unnormalized)? (1.f / tex_width) : 1.f;
			f32 scale_y = (unnormalized)? (1.f / tex_height) : 1.f;

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
			{
				internal_height = 1;
				scale_y = 0.f;
			}

			bool requires_processing = surface_width != internal_width || surface_height != internal_height;
			if (!requires_processing)
			{
				if (!is_depth)
				{
					for (const auto& tex : m_rtts.m_bound_render_targets)
					{
						if (std::get<0>(tex) == texaddr)
						{
							if (g_cfg.video.strict_rendering_mode)
							{
								LOG_WARNING(RSX, "Attempting to sample a currently bound render target @ 0x%x", texaddr);
								requires_processing = true;
								break;
							}
							else
							{
								//issue a texture barrier to ensure previous writes are visible
								insert_texture_barrier();
								break;
							}
						}
					}
				}
				else
				{
					if (texaddr == std::get<0>(m_rtts.m_bound_depth_stencil))
					{
						if (g_cfg.video.strict_rendering_mode)
						{
							LOG_WARNING(RSX, "Attempting to sample a currently bound depth surface @ 0x%x", texaddr);
							requires_processing = true;
						}
						else
						{
							//issue a texture barrier to ensure previous writes are visible
							insert_texture_barrier();
						}
					}
				}
			}

			if (requires_processing)
			{
				const auto w = rsx::apply_resolution_scale(internal_width, true);
				const auto h = rsx::apply_resolution_scale(internal_height, true);
				return{ texptr->get_surface(), texaddr, format, 0, 0, w, h, texture_upload_context::framebuffer_storage,
					is_depth, scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d };
			}

			return{ texptr->get_view(), texture_upload_context::framebuffer_storage, is_depth, scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d };
		}

		template <typename RsxTextureType, typename surface_store_type, typename ...Args>
		sampled_image_descriptor upload_texture(commandbuffer_type& cmd, RsxTextureType& tex, surface_store_type& m_rtts, Args&&... extras)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 tex_size = (u32)get_texture_size(tex);
			const u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			const bool is_compressed_format = (format == CELL_GCM_TEXTURE_COMPRESSED_DXT1 || format == CELL_GCM_TEXTURE_COMPRESSED_DXT23 || format == CELL_GCM_TEXTURE_COMPRESSED_DXT45);

			if (!texaddr || !tex_size)
			{
				LOG_ERROR(RSX, "Texture upload requested but texture not found, (address=0x%X, size=0x%X, w=%d, h=%d, p=%d, format=0x%X)", texaddr, tex_size, tex.width(), tex.height(), tex.pitch(), tex.format());
				return {};
			}

			const auto extended_dimension = tex.get_extended_texture_dimension();
			u16 depth = 0;
			u16 tex_height = (u16)tex.height();
			u16 tex_pitch = tex.pitch();
			const u16 tex_width = tex.width();

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
				//Check for sampleable rtts from previous render passes
				//TODO: When framebuffer Y compression is properly handled, this section can be removed. A more accurate framebuffer storage check exists below this block
				if (auto texptr = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
				{
					if (test_framebuffer(texaddr + texptr->raster_address_offset))
					{
						return process_framebuffer_resource(texptr, texaddr, tex.format(), m_rtts, tex_width, tex_height, extended_dimension, false);
					}
					else
					{
						m_rtts.invalidate_surface_address(texaddr, false);
						invalidate_address(texaddr, false, true, std::forward<Args>(extras)...);
					}
				}

				if (auto texptr = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
				{
					if (test_framebuffer(texaddr + texptr->raster_address_offset))
					{
						return process_framebuffer_resource(texptr, texaddr, tex.format(), m_rtts, tex_width, tex_height, extended_dimension, true);
					}
					else
					{
						m_rtts.invalidate_surface_address(texaddr, true);
						invalidate_address(texaddr, false, true, std::forward<Args>(extras)...);
					}
				}
			}

			tex_pitch = is_compressed_format? (tex_size / tex_height) : tex_pitch; //NOTE: Compressed textures dont have a real pitch (tex_size = (w*h)/6)
			if (tex_pitch == 0) tex_pitch = tex_width * get_format_block_size_in_bytes(format);

			const bool unnormalized = (tex.format() & CELL_GCM_TEXTURE_UN) != 0;
			f32 scale_x = (unnormalized) ? (1.f / tex_width) : 1.f;
			f32 scale_y = (unnormalized) ? (1.f / tex_height) : 1.f;

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
				scale_y = 0.f;

			if (!is_compressed_format)
			{
				/* Check if we are re-sampling a subresource of an RTV/DSV texture, bound or otherwise
				 * This check is much stricter than the one above
				 * (Turbo: Super Stunt Squad does this; bypassing the need for a sync object)
				 * The engine does not read back the texture resource through cell, but specifies a texture location that is
				 * a bound render target. We can bypass the expensive download in this case
				 */

				//TODO: Take framebuffer Y compression into account
				const auto rsc = m_rtts.get_surface_subresource_if_applicable(texaddr, tex_width, tex_height, tex_pitch);
				if (rsc.surface)
				{
					//TODO: Check that this region is not cpu-dirty before doing a copy
					if (!test_framebuffer(rsc.base_address + rsc.surface->raster_address_offset))
					{
						m_rtts.invalidate_surface_address(rsc.base_address, rsc.is_depth_surface);
						invalidate_address(rsc.base_address, false, true, std::forward<Args>(extras)...);
					}
					else if (extended_dimension != rsx::texture_dimension_extended::texture_dimension_2d &&
							 extended_dimension != rsx::texture_dimension_extended::texture_dimension_1d)
					{
						LOG_ERROR(RSX, "Sampling of RTT region as non-2D texture! addr=0x%x, Type=%d, dims=%dx%d",
							texaddr, (u8)tex.get_extended_texture_dimension(), tex.width(), tex.height());
					}
					else
					{
						u16 internal_width = tex_width;
						u16 internal_height = tex_height;

						get_native_dimensions(internal_width, internal_height, rsc.surface);
						if (!rsc.is_bound || !g_cfg.video.strict_rendering_mode)
						{
							if (!rsc.x && !rsc.y && rsc.w == internal_width && rsc.h == internal_height)
							{
								if (rsc.is_bound)
								{
									LOG_WARNING(RSX, "Sampling from a currently bound render target @ 0x%x", texaddr);
									insert_texture_barrier();
								}

								return{ rsc.surface->get_view(), texture_upload_context::framebuffer_storage, rsc.is_depth_surface,
									scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d };
							}
						}
						else
						{
							LOG_WARNING(RSX, "Attempting to sample a currently bound render target @ 0x%x", texaddr);
						}

						internal_width = rsx::apply_resolution_scale(internal_width, true);
						internal_height = (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)? 1: rsx::apply_resolution_scale(internal_height, true);

						return{ rsc.surface->get_surface(), rsc.base_address, format, rsx::apply_resolution_scale(rsc.x, false), rsx::apply_resolution_scale(rsc.y, false),
							internal_width, internal_height, texture_upload_context::framebuffer_storage, rsc.is_depth_surface, scale_x, scale_y,
							rsx::texture_dimension_extended::texture_dimension_2d };
					}
				}
			}

			{
				//Search in cache and upload/bind
				reader_lock lock(m_cache_mutex);

				auto cached_texture = find_texture_from_dimensions(texaddr, tex_width, tex_height, depth);
				if (cached_texture)
				{
					//TODO: Handle invalidated framebuffer textures better. This is awful
					if (cached_texture->get_context() == rsx::texture_upload_context::framebuffer_storage)
					{
						if (!cached_texture->is_locked())
						{
							cached_texture->set_dirty(true);
							m_unreleased_texture_objects++;
						}
					}
					else
					{
						if (cached_texture->get_image_type() == rsx::texture_dimension_extended::texture_dimension_1d)
							scale_y = 0.f;

						return{ cached_texture->get_raw_view(), cached_texture->get_context(), cached_texture->is_depth_texture(), scale_x, scale_y, cached_texture->get_image_type() };
					}
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
									if (extended_dimension != rsx::texture_dimension_extended::texture_dimension_2d &&
										extended_dimension != rsx::texture_dimension_extended::texture_dimension_1d)
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
									return{ src_image, surface->get_section_base(), format, offset_x, offset_y, tex_width, tex_height, texture_upload_context::blit_engine_dst,
											surface->is_depth_texture(), scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d };
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
			return{ upload_image_from_cpu(cmd, texaddr, tex_width, tex_height, depth, tex.get_exact_mipmap_count(), tex_pitch, format,
				texture_upload_context::shader_read, subresources_layout, extended_dimension, is_swizzled, remap_vector)->get_raw_view(),
				texture_upload_context::shader_read, false, scale_x, scale_y, extended_dimension };
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
			auto dst_subres = m_rtts.get_surface_subresource_if_applicable(dst_address, dst.width, dst.clip_height, dst.pitch, true, false, false);
			dst_is_render_target = dst_subres.surface != nullptr;

			if (dst_is_render_target && dst_subres.surface->get_native_pitch() != dst.pitch)
			{
				//Surface pitch is invalid if it is less that the rsx pitch (usually set to 64 in such a case)
				if (dst_subres.surface->get_rsx_pitch() != dst.pitch ||
					dst.pitch < dst_subres.surface->get_native_pitch())
					dst_is_render_target = false;
			}

			//TODO: Handle cases where src or dst can be a depth texture while the other is a color texture - requires a render pass to emulate
			auto src_subres = m_rtts.get_surface_subresource_if_applicable(framebuffer_src_address, src_w, src_h, src.pitch, true, false, false);
			src_is_render_target = src_subres.surface != nullptr;

			if (src_is_render_target && src_subres.surface->get_native_pitch() != src.pitch)
			{
				//Surface pitch is invalid if it is less that the rsx pitch (usually set to 64 in such a case)
				if (src_subres.surface->get_rsx_pitch() != src.pitch ||
					src.pitch < src_subres.surface->get_native_pitch())
					src_is_render_target = false;
			}

			if (src_is_render_target && !test_framebuffer(src_subres.base_address + src_subres.surface->raster_address_offset))
			{
				m_rtts.invalidate_surface_address(src_subres.base_address, src_subres.is_depth_surface);
				invalidate_address(src_subres.base_address, false, true, std::forward<Args>(extras)...);
				src_is_render_target = false;
			}

			if (dst_is_render_target && !test_framebuffer(dst_subres.base_address + dst_subres.surface->raster_address_offset))
			{
				m_rtts.invalidate_surface_address(dst_subres.base_address, dst_subres.is_depth_surface);
				invalidate_address(dst_subres.base_address, false, true, std::forward<Args>(extras)...);
				dst_is_render_target = false;
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
					invalidate_range_impl_base(src_address, memcpy_bytes_length, false, false, false, true, std::forward<Args>(extras)...);
					invalidate_range_impl_base(dst_address, memcpy_bytes_length, true, false, false, true, std::forward<Args>(extras)...);
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
				auto overlapping_surfaces = find_texture_from_range(src_address, src.pitch * src.height);

				auto old_src_area = src_area;
				for (auto &surface : overlapping_surfaces)
				{
					//look for any that will fit, unless its a shader read surface or framebuffer_storage
					if (surface->get_context() == rsx::texture_upload_context::shader_read ||
						surface->get_context() == rsx::texture_upload_context::framebuffer_storage)
						continue;

					if (const u32 address_offset = src_address - surface->get_section_base())
					{
						const u16 bpp = dst_is_argb8 ? 4 : 2;
						const u16 offset_y = address_offset / src.pitch;
						const u16 offset_x = address_offset % src.pitch;
						const u16 offset_x_in_block = offset_x / bpp;

						src_area.x1 += offset_x_in_block;
						src_area.x2 += offset_x_in_block;
						src_area.y1 += offset_y;
						src_area.y2 += offset_y;
					}

					if (src_area.x2 <= surface->get_width() &&
						src_area.y2 <= surface->get_height())
					{
						vram_texture = surface->get_raw_texture();
						break;
					}

					src_area = old_src_area;
				}

				if (!vram_texture)
				{
					lock.upgrade();

					invalidate_range_impl_base(src_address, src.pitch * src.slice_h, false, false, false, true, std::forward<Args>(extras)...);

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
				if (!dst_is_render_target)
				{
					u16 src_subres_w = src_subres.w;
					u16 src_subres_h = src_subres.h;
					get_rsx_dimensions(src_subres_w, src_subres_h, src_subres.surface);

					const int dst_width = (int)(src_subres_w * scale_x);
					const int dst_height = (int)(src_subres_h * scale_y);

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
				invalidate_range_impl_base(cached_dest->get_section_base(), cached_dest->get_section_size(), true, false, false, true, std::forward<Args>(extras)...);

				dest_texture = 0;
				cached_dest = nullptr;
			}
			else if (invalidate_dst_range)
			{
				lock.upgrade();
				invalidate_range_impl_base(dst_address, dst.pitch * dst.height, true, false, false, true, std::forward<Args>(extras)...);
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
			notify_surface_changed(dst.rsx_address);
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

			writer_lock lock(m_cache_mutex);

			const auto protect_info = get_memory_protection(texaddr);
			if (protect_info.first != utils::protection::rw)
			{
				if (protect_info.second->overlaps(texaddr, true))
				{
					if (protect_info.first == utils::protection::no)
						return;

					if (protect_info.second->get_context() != texture_upload_context::blit_engine_dst)
					{
						//TODO: Invalidate this section
						LOG_TRACE(RSX, "Framebuffer memory occupied by regular texture!");
					}
				}

				protect_info.second->unprotect();
				vm::ps3::write32(texaddr, texaddr);
				protect_info.second->protect(protect_info.first);
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
				writer_lock lock(m_cache_mutex);
				auto protect_info = get_memory_protection(texaddr);
				if (protect_info.first == utils::protection::no)
				{
					if (protect_info.second->overlaps(texaddr, true))
						return true;

					//Address isnt actually covered by the region, it only shares a page with it
					protect_info.second->unprotect();
					bool result = (vm::ps3::read32(texaddr) == texaddr);
					protect_info.second->protect(utils::protection::no);
					return result;
				}
			}

			return vm::ps3::read32(texaddr) == texaddr;
		}
	};
}
