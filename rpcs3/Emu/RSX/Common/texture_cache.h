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

	enum texture_sampler_status
	{
		status_uninitialized = 0,
		status_ready = 1
	};

	enum memory_read_flags
	{
		flush_always = 0,
		flush_once = 1
	};

	struct typeless_xfer
	{
		bool src_is_typeless = false;
		bool dst_is_typeless = false;
		bool src_is_depth = false;
		bool dst_is_depth = false;
		u32 src_gcm_format = 0;
		u32 dst_gcm_format = 0;
		f32 src_scaling_hint = 1.f;
		f32 dst_scaling_hint = 1.f;

		void analyse()
		{
			if (src_is_typeless && dst_is_typeless)
			{
				if (src_scaling_hint == dst_scaling_hint &&
					src_scaling_hint != 1.f)
				{
					if (src_is_depth == dst_is_depth)
					{
						src_is_typeless = dst_is_typeless = false;
						src_scaling_hint = dst_scaling_hint = 1.f;
					}
				}
			}
		}
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
		bool pack_unpack_swap_bytes = false;

		bool synchronized = false;
		bool flushed = false;
		u32  num_writes = 0;
		u32  required_writes = 1;

		u64 cache_tag = 0;

		memory_read_flags readback_behaviour = memory_read_flags::flush_once;
		rsx::texture_create_flags view_flags = rsx::texture_create_flags::default_component_order;
		rsx::texture_upload_context context = rsx::texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = rsx::texture_dimension_extended::texture_dimension_2d;
		rsx::texture_sampler_status sampler_status = rsx::texture_sampler_status::status_uninitialized;

		bool matches(u32 rsx_address, u32 rsx_size)
		{
			return rsx::buffered_section::matches(rsx_address, rsx_size);
		}

		bool matches(u32 rsx_address, u32 width, u32 height, u32 depth, u32 mipmaps)
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

				if (mipmaps && mipmaps > this->mipmaps)
					return false;

				return true;
			}

			return false;
		}

		void touch()
		{
			num_writes++;
		}

		void reset_write_statistics()
		{
			required_writes = num_writes;
			num_writes = 0;
		}

		void set_view_flags(rsx::texture_create_flags flags)
		{
			view_flags = flags;
		}

		void set_context(rsx::texture_upload_context upload_context)
		{
			context = upload_context;
		}

		void set_image_type(rsx::texture_dimension_extended type)
		{
			image_type = type;
		}

		void set_sampler_status(rsx::texture_sampler_status status)
		{
			sampler_status = status;
		}

		void set_gcm_format(u32 format)
		{
			gcm_format = format;
		}

		void set_memory_read_flags(memory_read_flags flags)
		{
			readback_behaviour = flags;
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

		memory_read_flags get_memory_read_flags() const
		{
			return readback_behaviour;
		}

		rsx::texture_sampler_status get_sampler_status() const
		{
			return sampler_status;
		}

		bool writes_likely_completed() const
		{
			if (context == rsx::texture_upload_context::blit_engine_dst)
				return num_writes >= required_writes;

			return true;
		}
	};

	template <typename commandbuffer_type, typename section_storage_type, typename image_resource_type, typename image_view_type, typename image_storage_type, typename texture_format>
	class texture_cache
	{
	private:

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

			void notify()
			{
				verify(HERE), valid_count >= 0;
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

			bool overlaps(u32 addr, u32 range) const
			{
				const u32 limit = addr + range;
				if (limit <= min_addr) return false;

				const u32 this_limit = max_addr + max_range;
				return (this_limit > addr);
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

		struct copy_region_descriptor
		{
			image_resource_type src;
			u16 src_x;
			u16 src_y;
			u16 dst_x;
			u16 dst_y;
			u16 dst_z;
			u16 w;
			u16 h;
		};

		enum deferred_request_command : u32
		{
			copy_image_static,
			copy_image_dynamic,
			cubemap_gather,
			cubemap_unwrap,
			atlas_gather,
			_3d_gather,
			_3d_unwrap
		};

		using texture_channel_remap_t = std::pair<std::array<u8, 4>, std::array<u8, 4>>;
		struct deferred_subresource
		{
			image_resource_type external_handle = 0;
			std::vector<copy_region_descriptor> sections_to_copy;
			texture_channel_remap_t remap;
			deferred_request_command op;
			u32 base_address = 0;
			u32 gcm_format = 0;
			u16 x = 0;
			u16 y = 0;
			u16 width = 0;
			u16 height = 0;
			u16 depth = 1;

			deferred_subresource()
			{}

			deferred_subresource(image_resource_type _res, deferred_request_command _op, u32 _addr, u32 _fmt, u16 _x, u16 _y, u16 _w, u16 _h, u16 _d, const texture_channel_remap_t& _remap) :
				external_handle(_res), op(_op), base_address(_addr), gcm_format(_fmt), x(_x), y(_y), width(_w), height(_h), depth(_d), remap(_remap)
			{}
		};

		struct blit_op_result
		{
			bool succeeded = false;
			bool is_depth = false;
			u32 real_dst_address = 0;
			u32 real_dst_size = 0;

			blit_op_result(bool success) : succeeded(success)
			{}
		};

		struct sampled_image_descriptor : public sampled_image_descriptor_base
		{
			image_view_type image_handle = 0;
			deferred_subresource external_subresource_desc = {};
			bool flag = false;

			sampled_image_descriptor()
			{}

			sampled_image_descriptor(image_view_type handle, texture_upload_context ctx, bool is_depth, f32 x_scale, f32 y_scale, rsx::texture_dimension_extended type)
			{
				image_handle = handle;
				upload_context = ctx;
				is_depth_texture = is_depth;
				scale_x = x_scale;
				scale_y = y_scale;
				image_type = type;
			}

			sampled_image_descriptor(image_resource_type external_handle, deferred_request_command reason, u32 base_address, u32 gcm_format,
				u16 x_offset, u16 y_offset, u16 width, u16 height, u16 depth, texture_upload_context ctx, bool is_depth, f32 x_scale, f32 y_scale,
				rsx::texture_dimension_extended type, const texture_channel_remap_t& remap)
			{
				external_subresource_desc = { external_handle, reason, base_address, gcm_format, x_offset, y_offset, width, height, depth, remap };

				image_handle = 0;
				upload_context = ctx;
				is_depth_texture = is_depth;
				scale_x = x_scale;
				scale_y = y_scale;
				image_type = type;
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

		//Map of messages to only emit once
		std::unordered_map<std::string, bool> m_once_only_messages_map;

		//Set when a shader read-only texture data suddenly becomes contested, usually by fbo memory
		bool read_only_tex_invalidate = false;

		//Store of all objects in a flush_always state. A lazy readback is attempted every draw call
		std::unordered_map<u32, u32> m_flush_always_cache;
		u64 m_flush_always_update_timestamp = 0;

		//Memory usage
		const s32 m_max_zombie_objects = 64; //Limit on how many texture objects to keep around for reuse after they are invalidated
		std::atomic<s32> m_unreleased_texture_objects = { 0 }; //Number of invalidated objects not yet freed from memory
		std::atomic<u32> m_texture_memory_in_use = { 0 };

		//Other statistics
		std::atomic<u32> m_num_flush_requests = { 0 };
		std::atomic<u32> m_num_cache_misses = { 0 };
		std::atomic<u32> m_num_cache_mispredictions = { 0 };

		/* Helpers */
		virtual void free_texture_section(section_storage_type&) = 0;
		virtual image_view_type create_temporary_subresource_view(commandbuffer_type&, image_resource_type* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type create_temporary_subresource_view(commandbuffer_type&, image_storage_type* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector) = 0;
		virtual section_storage_type* create_new_texture(commandbuffer_type&, u32 rsx_address, u32 rsx_size, u16 width, u16 height, u16 depth, u16 mipmaps, u32 gcm_format,
				rsx::texture_upload_context context, rsx::texture_dimension_extended type, texture_create_flags flags, rsx::texture_colorspace colorspace, const texture_channel_remap_t& remap_vector) = 0;
		virtual section_storage_type* upload_image_from_cpu(commandbuffer_type&, u32 rsx_address, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format, texture_upload_context context,
				const std::vector<rsx_subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, rsx::texture_colorspace colorspace, bool swizzled, const texture_channel_remap_t& remap_vector) = 0;
		virtual void enforce_surface_creation_type(section_storage_type& section, u32 gcm_format, texture_create_flags expected) = 0;
		virtual void set_up_remap_vector(section_storage_type& section, const texture_channel_remap_t& remap_vector) = 0;
		virtual void insert_texture_barrier(commandbuffer_type&, image_storage_type* tex) = 0;
		virtual image_view_type generate_cubemap_from_images(commandbuffer_type&, u32 gcm_format, u16 size, const std::vector<copy_region_descriptor>& sources, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type generate_3d_from_2d_images(commandbuffer_type&, u32 gcm_format, u16 width, u16 height, u16 depth, const std::vector<copy_region_descriptor>& sources, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type generate_atlas_from_images(commandbuffer_type&, u32 gcm_format, u16 width, u16 height, const std::vector<copy_region_descriptor>& sections_to_copy, const texture_channel_remap_t& remap_vector) = 0;
		virtual void update_image_contents(commandbuffer_type&, image_view_type dst, image_resource_type src, u16 width, u16 height) = 0;
		virtual bool render_target_format_is_compatible(image_storage_type* tex, u32 gcm_format) = 0;

		constexpr u32 get_block_size() const { return 0x1000000; }
		inline u32 get_block_address(u32 address) const { return (address & ~0xFFFFFF); }

		inline void update_cache_tag()
		{
			m_cache_update_tag++;
		}

		template <typename ...Args>
		void emit_once(bool error, const char* fmt, Args&&... params)
		{
			const std::string message = fmt::format(fmt, std::forward<Args>(params)...);
			if (m_once_only_messages_map.find(message) != m_once_only_messages_map.end())
				return;

			if (error)
				logs::RSX.error(message.c_str());
			else
				logs::RSX.warning(message.c_str());

			m_once_only_messages_map[message] = true;
		}

		template <typename ...Args>
		void err_once(const char* fmt, Args&&... params)
		{
			emit_once(true, fmt, std::forward<Args>(params)...);
		}

		template <typename ...Args>
		void warn_once(const char* fmt, Args&&... params)
		{
			emit_once(false, fmt, std::forward<Args>(params)...);
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

					auto overlapped = tex.overlaps_page(trampled_range, address, strict_range_check || tex.get_context() == rsx::texture_upload_context::blit_engine_dst);
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

						m_num_flush_requests++;
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

		inline bool is_hw_blit_engine_compatible(u32 format) const
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
		virtual bool is_depth_texture(u32, u32) = 0;
		virtual void on_frame_end() = 0;

		std::vector<section_storage_type*> find_texture_from_range(u32 rsx_address, u32 range)
		{
			std::vector<section_storage_type*> results;
			auto test = std::make_pair(rsx_address, range);
			for (auto &address_range : m_cache)
			{
				auto &range_data = address_range.second;
				if (!range_data.overlaps(rsx_address, range)) continue;

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
								if (!tex.is_locked())
								{
									//Data is valid from cache pov but region has been unlocked and flushed
									if (tex.get_context() == texture_upload_context::framebuffer_storage ||
										tex.get_context() == texture_upload_context::blit_engine_dst)
										range_data.notify();
								}

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

		section_storage_type* find_flushable_section(u32 address, u32 range)
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
		void lock_memory_region(image_storage_type* image, u32 memory_address, u32 memory_size, u32 width, u32 height, u32 pitch, Args&&... extras)
		{
			writer_lock lock(m_cache_mutex);
			section_storage_type& region = find_cached_texture(memory_address, memory_size, false);

			if (region.get_context() != texture_upload_context::framebuffer_storage &&
				region.exists())
			{
				//This space was being used for other purposes other than framebuffer storage
				//Delete used resources before attaching it to framebuffer memory
				read_only_tex_invalidate = true;
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
			region.set_sampler_status(rsx::texture_sampler_status::status_uninitialized);
			region.set_image_type(rsx::texture_dimension_extended::texture_dimension_2d);
			update_cache_tag();

			region.set_memory_read_flags(memory_read_flags::flush_always);
			m_flush_always_cache[memory_address] = memory_size;
		}

		void set_memory_read_flags(u32 memory_address, u32 memory_size, memory_read_flags flags)
		{
			writer_lock lock(m_cache_mutex);

			if (flags != memory_read_flags::flush_always)
				m_flush_always_cache.erase(memory_address);

			section_storage_type& region = find_cached_texture(memory_address, memory_size, false);

			if (!region.exists() || region.get_context() != texture_upload_context::framebuffer_storage)
				return;

			if (flags == memory_read_flags::flush_always)
				m_flush_always_cache[memory_address] = memory_size;

			region.set_memory_read_flags(flags);
		}

		template <typename ...Args>
		bool flush_memory_to_cache(u32 memory_address, u32 memory_size, bool skip_synchronized, Args&&... extra)
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

			if (!region->writes_likely_completed())
				return true;

			region->copy_texture(false, std::forward<Args>(extra)...);
			return true;
		}

		template <typename ...Args>
		bool load_memory_from_cache(u32 memory_address, u32 memory_size, Args&&... extras)
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
						{
							record_cache_miss(*tex);
							tex->copy_texture(true, std::forward<Args>(extras)...);
						}

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
					m_num_flush_requests++;
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
			m_num_cache_misses++;

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

			value.misses += 2;
		}

		template <typename ...Args>
		bool flush_if_cache_miss_likely(texture_format fmt, u32 memory_address, u32 memory_size, Args&&... extras)
		{
			auto It = m_cache_miss_statistics_table.find(memory_address);
			if (It == m_cache_miss_statistics_table.end())
			{
				m_cache_miss_statistics_table[memory_address] = { 0, memory_size, fmt };
				return false;
			}

			auto &value = It->second;

			if (value.format != fmt || value.block_size < memory_size)
			{
				//Reset since the data has changed
				//TODO: Keep track of all this information together
				m_cache_miss_statistics_table[memory_address] = { 0, memory_size, fmt };
				return false;
			}

			//Properly synchronized - no miss
			if (!value.misses) return false;

			//Auto flush if this address keeps missing (not properly synchronized)
			if (value.misses > 16)
			{
				//TODO: Determine better way of setting threshold
				if (!flush_memory_to_cache(memory_address, memory_size, true, std::forward<Args>(extras)...))
					value.misses--;

				return true;
			}

			return false;
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
					found_desc.op != desc.op ||
					found_desc.x != desc.x || found_desc.y != desc.y ||
					found_desc.width != desc.width || found_desc.height != desc.height)
					continue;

				if (desc.op == deferred_request_command::copy_image_dynamic)
					update_image_contents(cmd, It->second.second, desc.external_handle, desc.width, desc.height);

				return It->second.second;
			}

			image_view_type result = 0;
			switch (desc.op)
			{
			case deferred_request_command::cubemap_gather:
			{
				result = generate_cubemap_from_images(cmd, desc.gcm_format, desc.width, desc.sections_to_copy, desc.remap);
				break;
			}
			case deferred_request_command::cubemap_unwrap:
			{
				std::vector<copy_region_descriptor> sections(6);
				for (u16 n = 0; n < 6; ++n)
				{
					sections[n] = { desc.external_handle, 0, (u16)(desc.height * n), 0, 0, n, desc.width, desc.height };
				}

				result = generate_cubemap_from_images(cmd, desc.gcm_format, desc.width, sections, desc.remap);
				break;
			}
			case deferred_request_command::_3d_gather:
			{
				result = generate_3d_from_2d_images(cmd, desc.gcm_format, desc.width, desc.height, desc.depth, desc.sections_to_copy, desc.remap);
				break;
			}
			case deferred_request_command::_3d_unwrap:
			{
				std::vector<copy_region_descriptor> sections;
				sections.resize(desc.depth);
				for (u16 n = 0; n < desc.depth; ++n)
				{
					sections[n] = { desc.external_handle, 0, (u16)(desc.height * n), 0, 0, n, desc.width, desc.height };
				}

				result = generate_3d_from_2d_images(cmd, desc.gcm_format, desc.width, desc.height, desc.depth, sections, desc.remap);
				break;
			}
			case deferred_request_command::atlas_gather:
			{
				result = generate_atlas_from_images(cmd, desc.gcm_format, desc.width, desc.height, desc.sections_to_copy, desc.remap);
				break;
			}
			case deferred_request_command::copy_image_static:
			case deferred_request_command::copy_image_dynamic:
			{
				result = create_temporary_subresource_view(cmd, &desc.external_handle, desc.gcm_format, desc.x, desc.y, desc.width, desc.height, desc.remap);
				break;
			}
			default:
			{
				//Throw
				fmt::throw_exception("Invalid deferred command op 0x%X" HERE, (u32)desc.op);
			}
			}

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

		template<typename surface_store_type>
		std::vector<copy_region_descriptor> gather_texture_slices_from_framebuffers(u32 texaddr, u16 slice_w, u16 slice_h, u16 pitch, u16 count, u8 bpp, surface_store_type& m_rtts)
		{
			std::vector<copy_region_descriptor> surfaces;
			u32 current_address = texaddr;
			u32 slice_size = (pitch * slice_h);
			bool unsafe = false;

			for (u16 slice = 0; slice < count; ++slice)
			{
				auto overlapping = m_rtts.get_merged_texture_memory_region(current_address, slice_w, slice_h, pitch, bpp);
				current_address += (pitch * slice_h);

				if (overlapping.empty())
				{
					unsafe = true;
					surfaces.push_back({});
				}
				else
				{
					for (auto &section : overlapping)
					{
						surfaces.push_back
						({
							section.surface->get_surface(),
							rsx::apply_resolution_scale(section.src_x, true),
							rsx::apply_resolution_scale(section.src_y, true),
							rsx::apply_resolution_scale(section.dst_x, true),
							rsx::apply_resolution_scale(section.dst_y, true),
							slice,
							rsx::apply_resolution_scale(section.width, true),
							rsx::apply_resolution_scale(section.height, true)
						});
					}
				}
			}

			if (unsafe)
			{
				//TODO: Gather remaining sides from the texture cache or upload from cpu (too slow?)
				LOG_ERROR(RSX, "Could not gather all required slices for cubemap/3d generation");
			}

			return surfaces;
		}

		template <typename render_target_type, typename surface_store_type>
		sampled_image_descriptor process_framebuffer_resource(commandbuffer_type& cmd, render_target_type texptr, u32 texaddr, u32 gcm_format, surface_store_type& m_rtts,
				u16 tex_width, u16 tex_height, u16 tex_depth, u16 tex_pitch, rsx::texture_dimension_extended extended_dimension, bool is_depth, u32 encoded_remap, const texture_channel_remap_t& decoded_remap)
		{
			const u32 format = gcm_format & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
			const auto surface_width = texptr->get_surface_width();
			const auto surface_height = texptr->get_surface_height();

			u32 internal_width = tex_width;
			u32 internal_height = tex_height;
			get_native_dimensions(internal_width, internal_height, texptr);

			if (extended_dimension != rsx::texture_dimension_extended::texture_dimension_2d &&
				extended_dimension != rsx::texture_dimension_extended::texture_dimension_1d)
			{
				if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_cubemap)
				{
					const auto scaled_size = rsx::apply_resolution_scale(internal_width, true);
					if (surface_height == (surface_width * 6))
					{
						return{ texptr->get_surface(), deferred_request_command::cubemap_unwrap, texaddr, format, 0, 0,
								scaled_size, scaled_size, 1,
								texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
								rsx::texture_dimension_extended::texture_dimension_cubemap, decoded_remap };
					}

					sampled_image_descriptor desc = { texptr->get_surface(), deferred_request_command::cubemap_gather, texaddr, format, 0, 0,
							scaled_size, scaled_size, 1,
							texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
							rsx::texture_dimension_extended::texture_dimension_cubemap, decoded_remap };

					auto bpp = get_format_block_size_in_bytes(format);
					desc.external_subresource_desc.sections_to_copy = std::move(gather_texture_slices_from_framebuffers(texaddr, tex_width, tex_height, tex_pitch, 6, bpp, m_rtts));
					return desc;
				}
				else if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_3d && tex_depth > 1)
				{
					auto minimum_height = (tex_height * tex_depth);
					auto scaled_w = rsx::apply_resolution_scale(internal_width, true);
					auto scaled_h = rsx::apply_resolution_scale(internal_height, true);
					if (surface_height >= minimum_height && surface_width >= tex_width)
					{
						return{ texptr->get_surface(), deferred_request_command::_3d_unwrap, texaddr, format, 0, 0,
								scaled_w, scaled_h, tex_depth,
								texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
								rsx::texture_dimension_extended::texture_dimension_3d, decoded_remap };
					}

					sampled_image_descriptor desc = { texptr->get_surface(), deferred_request_command::_3d_gather, texaddr, format, 0, 0,
						scaled_w, scaled_h, tex_depth,
						texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
						rsx::texture_dimension_extended::texture_dimension_3d, decoded_remap };

					const auto bpp = get_format_block_size_in_bytes(format);
					desc.external_subresource_desc.sections_to_copy = std::move(gather_texture_slices_from_framebuffers(texaddr, tex_width, tex_height, tex_pitch, tex_depth, bpp, m_rtts));
					return desc;
				}
			}

			const bool unnormalized = (gcm_format & CELL_GCM_TEXTURE_UN) != 0;
			f32 scale_x = (unnormalized)? (1.f / tex_width) : 1.f;
			f32 scale_y = (unnormalized)? (1.f / tex_height) : 1.f;

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
			{
				internal_height = 1;
				scale_y = 0.f;
			}

			if (internal_width > surface_width || internal_height > surface_height)
			{
				auto bpp = get_format_block_size_in_bytes(format);
				auto overlapping = m_rtts.get_merged_texture_memory_region(texaddr, tex_width, tex_height, tex_pitch, bpp);

				if (overlapping.size() > 1)
				{
					const auto w = rsx::apply_resolution_scale(internal_width, true);
					const auto h = rsx::apply_resolution_scale(internal_height, true);

					sampled_image_descriptor result = { texptr->get_surface(), deferred_request_command::atlas_gather,
							texaddr, format, 0, 0, w, h, 1, texture_upload_context::framebuffer_storage, is_depth,
							scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d, decoded_remap };

					result.external_subresource_desc.sections_to_copy.reserve(overlapping.size());

					for (auto &section : overlapping)
					{
						result.external_subresource_desc.sections_to_copy.push_back
						({
							section.surface->get_surface(),
							rsx::apply_resolution_scale(section.src_x, true),
							rsx::apply_resolution_scale(section.src_y, true),
							rsx::apply_resolution_scale(section.dst_x, true),
							rsx::apply_resolution_scale(section.dst_y, true),
							0,
							rsx::apply_resolution_scale(section.width, true),
							rsx::apply_resolution_scale(section.height, true)
						});
					}

					return result;
				}
			}

			bool requires_processing = surface_width > internal_width || surface_height > internal_height;
			bool update_subresource_cache = false;
			if (!requires_processing)
			{
				//NOTE: The scale also accounts for sampling outside the RTT region, e.g render to one quadrant but send whole texture for sampling
				//In these cases, internal dimensions will exceed available surface dimensions. Account for the missing information using scaling (missing data will result in border color)
				//TODO: Proper gather and stitching without performance loss
				if (internal_width > surface_width)
					scale_x *= ((f32)internal_width / surface_width);

				if (internal_height > surface_height)
					scale_y *= ((f32)internal_height / surface_height);

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
								update_subresource_cache = true;
								break;
							}
							else
							{
								//issue a texture barrier to ensure previous writes are visible
								insert_texture_barrier(cmd, texptr);
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
							update_subresource_cache = true;
						}
						else
						{
							//issue a texture barrier to ensure previous writes are visible
							insert_texture_barrier(cmd, texptr);
						}
					}
				}
			}

			if (!requires_processing)
			{
				//Check if we need to do anything about the formats
				requires_processing = !render_target_format_is_compatible(texptr, format);
			}

			if (requires_processing)
			{
				const auto w = rsx::apply_resolution_scale(internal_width, true);
				const auto h = rsx::apply_resolution_scale(internal_height, true);

				auto command = update_subresource_cache ? deferred_request_command::copy_image_dynamic : deferred_request_command::copy_image_static;
				return { texptr->get_surface(), command, texaddr, format, 0, 0, w, h, 1,
						texture_upload_context::framebuffer_storage, is_depth, scale_x, scale_y,
						rsx::texture_dimension_extended::texture_dimension_2d, decoded_remap };
			}

			return{ texptr->get_view(encoded_remap, decoded_remap), texture_upload_context::framebuffer_storage,
					is_depth, scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d };
		}

		template <typename RsxTextureType, typename surface_store_type, typename ...Args>
		sampled_image_descriptor upload_texture(commandbuffer_type& cmd, RsxTextureType& tex, surface_store_type& m_rtts, Args&&... extras)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 tex_size = (u32)get_placed_texture_storage_size(tex, 256, 512);
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
			const u16 tex_width = tex.width();
			u16 tex_pitch = is_compressed_format? (u16)(get_texture_size(tex) / tex_height) : tex.pitch(); //NOTE: Compressed textures dont have a real pitch (tex_size = (w*h)/6)
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
				//Check for sampleable rtts from previous render passes
				//TODO: When framebuffer Y compression is properly handled, this section can be removed. A more accurate framebuffer storage check exists below this block
				if (auto texptr = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
				{
					if (test_framebuffer(texaddr))
					{
						return process_framebuffer_resource(cmd, texptr, texaddr, tex.format(), m_rtts,
								tex_width, tex_height, depth, tex_pitch, extended_dimension, false, tex.remap(),
								tex.decoded_remap());
					}
					else
					{
						m_rtts.invalidate_surface_address(texaddr, false);
						invalidate_address(texaddr, false, true, std::forward<Args>(extras)...);
					}
				}

				if (auto texptr = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
				{
					if (test_framebuffer(texaddr))
					{
						return process_framebuffer_resource(cmd, texptr, texaddr, tex.format(), m_rtts,
								tex_width, tex_height, depth, tex_pitch, extended_dimension, true, tex.remap(),
								tex.decoded_remap());
					}
					else
					{
						m_rtts.invalidate_surface_address(texaddr, true);
						invalidate_address(texaddr, false, true, std::forward<Args>(extras)...);
					}
				}
			}

			const bool unnormalized = (tex.format() & CELL_GCM_TEXTURE_UN) != 0;
			f32 scale_x = (unnormalized) ? (1.f / tex_width) : 1.f;
			f32 scale_y = (unnormalized) ? (1.f / tex_height) : 1.f;

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
				scale_y = 0.f;

			if (!is_compressed_format)
			{
				// Check if we are re-sampling a subresource of an RTV/DSV texture, bound or otherwise

				const auto rsc = m_rtts.get_surface_subresource_if_applicable(texaddr, tex_width, tex_height, tex_pitch);
				if (rsc.surface)
				{
					if (!test_framebuffer(rsc.base_address))
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
						if (!rsc.x && !rsc.y && rsc.w == internal_width && rsc.h == internal_height)
						{
							//Full sized hit from the surface cache. This should have been already found before getting here
							fmt::throw_exception("Unreachable" HERE);
						}

						internal_width = rsx::apply_resolution_scale(internal_width, true);
						internal_height = (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)? 1: rsx::apply_resolution_scale(internal_height, true);

						return{ rsc.surface->get_surface(), deferred_request_command::copy_image_static, rsc.base_address, format,
							rsx::apply_resolution_scale(rsc.x, false), rsx::apply_resolution_scale(rsc.y, false),
							internal_width, internal_height, 1, texture_upload_context::framebuffer_storage, rsc.is_depth_surface, scale_x, scale_y,
							rsx::texture_dimension_extended::texture_dimension_2d, tex.decoded_remap() };
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

						if (cached_texture->get_sampler_status() != rsx::texture_sampler_status::status_ready)
							set_up_remap_vector(*cached_texture, tex.decoded_remap());

						return{ cached_texture->get_raw_view(), cached_texture->get_context(), cached_texture->is_depth_texture(), scale_x, scale_y, cached_texture->get_image_type() };
					}
				}

				if (is_hw_blit_engine_compatible(format))
				{
					//Find based on range instead
					auto overlapping_surfaces = find_texture_from_range(texaddr, tex_size);
					if (!overlapping_surfaces.empty())
					{
						for (const auto &surface : overlapping_surfaces)
						{
							if (surface->get_context() != rsx::texture_upload_context::blit_engine_dst ||
								!surface->is_locked())
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

									if (surface->get_sampler_status() != rsx::texture_sampler_status::status_ready)
										set_up_remap_vector(*surface, tex.decoded_remap());

									auto src_image = surface->get_raw_texture();
									return{ src_image, deferred_request_command::copy_image_static, surface->get_section_base(), format, offset_x, offset_y, tex_width, tex_height, 1,
										texture_upload_context::blit_engine_dst, surface->is_depth_texture(), scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d,
										rsx::default_remap_vector };
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

			bool is_depth_format = false;
			switch (format)
			{
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
				is_depth_format = true;
				break;
			}

			//Invalidate with writing=false, discard=false, rebuild=false, native_flush=true
			invalidate_range_impl_base(texaddr, tex_size, false, false, false, true, std::forward<Args>(extras)...);

			//NOTE: SRGB correction is to be handled in the fragment shader; upload as linear RGB
			m_texture_memory_in_use += (tex_pitch * tex_height);
			return{ upload_image_from_cpu(cmd, texaddr, tex_width, tex_height, depth, tex.get_exact_mipmap_count(), tex_pitch, format,
				texture_upload_context::shader_read, subresources_layout, extended_dimension, rsx::texture_colorspace::rgb_linear, is_swizzled, remap_vector)->get_raw_view(),
				texture_upload_context::shader_read, is_depth_format, scale_x, scale_y, extended_dimension };
		}

		template <typename surface_store_type, typename blitter_type, typename ...Args>
		blit_op_result upload_scaled_image(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, commandbuffer_type& cmd, surface_store_type& m_rtts, blitter_type& blitter, Args&&... extras)
		{
			//Since we will have dst in vram, we can 'safely' ignore the swizzle flag
			//TODO: Verify correct behavior
			bool src_is_render_target = false;
			bool dst_is_render_target = false;
			bool dst_is_argb8 = (dst.format == rsx::blit_engine::transfer_destination_format::a8r8g8b8);
			bool src_is_argb8 = (src.format == rsx::blit_engine::transfer_source_format::a8r8g8b8);

			typeless_xfer typeless_info = {};
			image_resource_type vram_texture = 0;
			image_resource_type dest_texture = 0;

			const u32 src_address = (u32)((u64)src.pixels - (u64)vm::base(0));
			const u32 dst_address = (u32)((u64)dst.pixels - (u64)vm::base(0));

			f32 scale_x = dst.scale_x;
			f32 scale_y = dst.scale_y;

			//Offset in x and y for src is 0 (it is already accounted for when getting pixels_src)
			//Reproject final clip onto source...
			u16 src_w = (const u16)((f32)dst.clip_width / scale_x);
			u16 src_h = (const u16)((f32)dst.clip_height / scale_y);

			u16 dst_w = dst.clip_width;
			u16 dst_h = dst.clip_height;

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
			auto src_subres = m_rtts.get_surface_subresource_if_applicable(src_address, src_w, src_h, src.pitch, true, false, false);
			src_is_render_target = src_subres.surface != nullptr;

			if (src_is_render_target && src_subres.surface->get_native_pitch() != src.pitch)
			{
				//Surface pitch is invalid if it is less that the rsx pitch (usually set to 64 in such a case)
				if (src_subres.surface->get_rsx_pitch() != src.pitch ||
					src.pitch < src_subres.surface->get_native_pitch())
					src_is_render_target = false;
			}

			if (src_is_render_target && !test_framebuffer(src_subres.base_address))
			{
				m_rtts.invalidate_surface_address(src_subres.base_address, src_subres.is_depth_surface);
				invalidate_address(src_subres.base_address, false, true, std::forward<Args>(extras)...);
				src_is_render_target = false;
			}

			if (dst_is_render_target && !test_framebuffer(dst_subres.base_address))
			{
				m_rtts.invalidate_surface_address(dst_subres.base_address, dst_subres.is_depth_surface);
				invalidate_address(dst_subres.base_address, false, true, std::forward<Args>(extras)...);
				dst_is_render_target = false;
			}

			//Always use GPU blit if src or dst is in the surface store
			if (!g_cfg.video.use_gpu_texture_scaling && !(src_is_render_target || dst_is_render_target))
				return false;

			if (src_is_render_target)
			{
				const auto surf = src_subres.surface;
				auto src_bpp = surf->get_native_pitch() / surf->get_surface_width();
				auto expected_bpp = src_is_argb8 ? 4 : 2;
				if (src_bpp != expected_bpp)
				{
					//Enable type scaling in src
					typeless_info.src_is_typeless = true;
					typeless_info.src_is_depth = src_subres.is_depth_surface;
					typeless_info.src_scaling_hint = (f32)src_bpp / expected_bpp;
					typeless_info.src_gcm_format = src_is_argb8 ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;

					src_w = (u16)(src_w / typeless_info.src_scaling_hint);
					if (!src_subres.is_clipped)
						src_subres.w = (u16)(src_subres.w / typeless_info.src_scaling_hint);
					else
						src_subres = m_rtts.get_surface_subresource_if_applicable(src_address, src_w, src_h, src.pitch, true, false, false);

					verify(HERE), src_subres.surface != nullptr;
				}
			}

			if (dst_is_render_target)
			{
				auto dst_bpp = dst_subres.surface->get_native_pitch() / dst_subres.surface->get_surface_width();
				auto expected_bpp = dst_is_argb8 ? 4 : 2;
				if (dst_bpp != expected_bpp)
				{
					//Enable type scaling in dst
					typeless_info.dst_is_typeless = true;
					typeless_info.dst_is_depth = dst_subres.is_depth_surface;
					typeless_info.dst_scaling_hint = (f32)dst_bpp / expected_bpp;
					typeless_info.dst_gcm_format = dst_is_argb8 ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;

					dst_w = (u16)(dst_w / typeless_info.dst_scaling_hint);
					if (!dst_subres.is_clipped)
						dst_subres.w = (u16)(dst_subres.w / typeless_info.dst_scaling_hint);
					else
						dst_subres = m_rtts.get_surface_subresource_if_applicable(dst_address, dst_w, dst_h, dst.pitch, true, false, false);

					verify(HERE), dst_subres.surface != nullptr;
				}
			}

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
			areai dst_area = { 0, 0, dst_w, dst_h };

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

				for (const auto &surface : overlapping_surfaces)
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

				max_dst_width = (u16)(dst_subres.surface->get_surface_width() * typeless_info.dst_scaling_hint);
				max_dst_height = dst_subres.surface->get_surface_height();
			}

			//Create source texture if does not exist
			if (!src_is_render_target)
			{
				auto overlapping_surfaces = find_texture_from_range(src_address, src.pitch * src.height);

				auto old_src_area = src_area;
				for (const auto &surface : overlapping_surfaces)
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
					subres.pitch_in_block = pitch_in_block;
					subres.depth = 1;
					subres.data = { (const gsl::byte*)src.pixels, src.pitch * src.slice_h };
					subresource_layout.push_back(subres);

					const u32 gcm_format = src_is_argb8 ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;
					vram_texture = upload_image_from_cpu(cmd, src_address, src.width, src.slice_h, 1, 1, src.pitch, gcm_format, texture_upload_context::blit_engine_src,
						subresource_layout, rsx::texture_dimension_extended::texture_dimension_2d, rsx::texture_colorspace::rgb_linear, dst.swizzled, rsx::default_remap_vector)->get_raw_texture();

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

					const int dst_width = (int)(src_subres_w * scale_x * typeless_info.src_scaling_hint);
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

			const bool src_is_depth = src_subres.is_depth_surface;
			const bool dst_is_depth = dst_is_render_target? dst_subres.is_depth_surface :
										dest_texture ? cached_dest->is_depth_texture() : src_is_depth;

			//Type of blit decided by the source, destination use should adapt on the fly
			const bool is_depth_blit = src_is_depth;

			bool format_mismatch = (src_is_depth != dst_is_depth);
			if (format_mismatch)
			{
				if (dst_is_render_target)
				{
					LOG_ERROR(RSX, "Depth<->RGBA blit on a framebuffer requested but not supported");
					return false;
				}
			}
			else if (src_is_render_target && cached_dest)
			{
				switch (cached_dest->get_gcm_format())
				{
				case CELL_GCM_TEXTURE_A8R8G8B8:
				case CELL_GCM_TEXTURE_DEPTH24_D8:
					format_mismatch = !dst_is_argb8;
					break;
				case CELL_GCM_TEXTURE_R5G6B5:
				case CELL_GCM_TEXTURE_DEPTH16:
					format_mismatch = dst_is_argb8;
					break;
				default:
					format_mismatch = true;
					break;
				}
			}

			//TODO: Check for other types of format mismatch
			if (format_mismatch)
			{
				lock.upgrade();

				//Mark for removal as the memory is not reusable now
				if (cached_dest->is_locked())
				{
					cached_dest->unprotect();
					m_cache[get_block_address(cached_dest->get_section_base())].remove_one();
				}

				cached_dest->set_dirty(true);
				m_unreleased_texture_objects++;

				invalidate_range_impl_base(cached_dest->get_section_base(), cached_dest->get_section_size(), true, false, false, true, std::forward<Args>(extras)...);

				dest_texture = 0;
				cached_dest = nullptr;
			}
			else if (invalidate_dst_range)
			{
				lock.upgrade();
				invalidate_range_impl_base(dst_address, dst.pitch * dst.height, true, false, false, true, std::forward<Args>(extras)...);
			}

			u32 gcm_format;
			if (is_depth_blit)
				gcm_format = (dst_is_argb8) ? CELL_GCM_TEXTURE_DEPTH24_D8 : CELL_GCM_TEXTURE_DEPTH16;
			else
				gcm_format = (dst_is_argb8) ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;

			if (cached_dest)
			{
				//Prep surface
				auto channel_order = src_is_render_target ? rsx::texture_create_flags::native_component_order :
					dst_is_argb8 ? rsx::texture_create_flags::default_component_order :
					rsx::texture_create_flags::swapped_native_component_order;

				enforce_surface_creation_type(*cached_dest, gcm_format, channel_order);
			}

			//Validate clipping region
			if ((dst.offset_x + dst.clip_x + dst.clip_width) > max_dst_width) dst.clip_x = 0;
			if ((dst.offset_y + dst.clip_y + dst.clip_height) > max_dst_height) dst.clip_y = 0;

			//Reproject clip offsets onto source to simplify blit
			if (dst.clip_x || dst.clip_y)
			{
				const u16 scaled_clip_offset_x = (const u16)((f32)dst.clip_x / (scale_x * typeless_info.src_scaling_hint));
				const u16 scaled_clip_offset_y = (const u16)((f32)dst.clip_y / scale_y);

				src_area.x1 += scaled_clip_offset_x;
				src_area.x2 += scaled_clip_offset_x;
				src_area.y1 += scaled_clip_offset_y;
				src_area.y2 += scaled_clip_offset_y;
			}

			if (dest_texture == 0)
			{
				lock.upgrade();

				//render target data is already in correct swizzle layout
				auto channel_order = src_is_render_target ? rsx::texture_create_flags::native_component_order :
					dst_is_argb8 ? rsx::texture_create_flags::default_component_order :
					rsx::texture_create_flags::swapped_native_component_order;

				dest_texture = create_new_texture(cmd, dst.rsx_address, dst.pitch * dst_dimensions.height,
					dst_dimensions.width, dst_dimensions.height, 1, 1,
					gcm_format, rsx::texture_upload_context::blit_engine_dst, rsx::texture_dimension_extended::texture_dimension_2d,
					channel_order, rsx::texture_colorspace::rgb_linear, rsx::default_remap_vector)->get_raw_texture();

				m_texture_memory_in_use += dst.pitch * dst_dimensions.height;
			}
			else if (cached_dest)
			{
				if (!cached_dest->is_locked())
				{
					lock.upgrade();

					cached_dest->reprotect(utils::protection::no);
					m_cache[get_block_address(cached_dest->get_section_base())].notify();
				}
				else if (cached_dest->is_synchronized())
				{
					//Prematurely read back
					m_num_cache_mispredictions++;
				}

				cached_dest->touch();
			}

			if (rsx::get_resolution_scale_percent() != 100)
			{
				const f32 resolution_scale = rsx::get_resolution_scale();
				if (src_is_render_target)
				{
					if (src_subres.surface->get_surface_width() > g_cfg.video.min_scalable_dimension)
					{
						src_area.x1 = (u16)(src_area.x1 * resolution_scale);
						src_area.x2 = (u16)(src_area.x2 * resolution_scale);
					}

					if (src_subres.surface->get_surface_height() > g_cfg.video.min_scalable_dimension)
					{
						src_area.y1 = (u16)(src_area.y1 * resolution_scale);
						src_area.y2 = (u16)(src_area.y2 * resolution_scale);
					}
				}

				if (dst_is_render_target)
				{
					if (dst_subres.surface->get_surface_width() > g_cfg.video.min_scalable_dimension)
					{
						dst_area.x1 = (u16)(dst_area.x1 * resolution_scale);
						dst_area.x2 = (u16)(dst_area.x2 * resolution_scale);
					}

					if (dst_subres.surface->get_surface_height() > g_cfg.video.min_scalable_dimension)
					{
						dst_area.y1 = (u16)(dst_area.y1 * resolution_scale);
						dst_area.y2 = (u16)(dst_area.y2 * resolution_scale);
					}
				}
			}

			typeless_info.analyse();
			blitter.scale_image(vram_texture, dest_texture, src_area, dst_area, interpolate, is_depth_blit, typeless_info);
			notify_surface_changed(dst.rsx_address);

			blit_op_result result = true;
			result.is_depth = is_depth_blit;

			if (cached_dest)
			{
				result.real_dst_address = cached_dest->get_section_base();
				result.real_dst_size = cached_dest->get_section_size();
			}
			else
			{
				result.real_dst_address = dst.rsx_address;
				result.real_dst_size = dst.pitch * dst_dimensions.height;
			}

			return result;
		}

		void do_update()
		{
			if (m_flush_always_cache.size())
			{
				if (m_cache_update_tag.load(std::memory_order_consume) != m_flush_always_update_timestamp)
				{
					writer_lock lock(m_cache_mutex);

					for (const auto &It : m_flush_always_cache)
					{
						auto& section = find_cached_texture(It.first, It.second);
						if (section.get_protection() != utils::protection::no)
						{
							auto &range = m_cache[get_block_address(It.first)];
							section.reprotect(utils::protection::no);
							range.notify();
						}
					}

					m_flush_always_update_timestamp = m_cache_update_tag.load(std::memory_order_consume);
				}
			}
		}

		void reset_frame_statistics()
		{
			m_num_flush_requests.store(0u);
			m_num_cache_misses.store(0u);
			m_num_cache_mispredictions.store(0u);
		}

		virtual const u32 get_unreleased_textures_count() const
		{
			return m_unreleased_texture_objects;
		}

		virtual const u32 get_texture_memory_in_use() const
		{
			return m_texture_memory_in_use;
		}

		virtual u32 get_num_flush_requests() const
		{
			return m_num_flush_requests;
		}

		virtual u32 get_num_cache_mispredictions() const
		{
			return m_num_cache_mispredictions;
		}

		virtual f32 get_cache_miss_ratio() const
		{
			const auto num_flushes = m_num_flush_requests.load();
			return (num_flushes == 0u) ? 0.f : (f32)m_num_cache_misses.load() / num_flushes;
		}

		/**
		 * The read only texture invalidate flag is set if a read only texture is trampled by framebuffer memory
		 * If set, all cached read only textures are considered invalid and should be re-fetched from the texture cache
		 */
		virtual void clear_ro_tex_invalidate_intr()
		{
			read_only_tex_invalidate = false;
		}

		virtual bool get_ro_tex_invalidate_intr() const
		{
			return read_only_tex_invalidate;
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
				vm::write32(texaddr, texaddr);
				protect_info.second->protect(protect_info.first);
				return;
			}

			vm::write32(texaddr, texaddr);
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
					bool result = (vm::read32(texaddr) == texaddr);
					protect_info.second->protect(utils::protection::no);
					return result;
				}
			}

			return vm::read32(texaddr) == texaddr;
		}
	};
}
