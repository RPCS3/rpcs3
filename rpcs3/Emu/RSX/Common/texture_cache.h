#pragma once

#include "../rsx_cache.h"
#include "../rsx_utils.h"
#include "texture_cache_predictor.h"
#include "texture_cache_utils.h"
#include "TextureUtils.h"

#include <atomic>

extern u64 get_system_time();

namespace rsx
{
	template <typename derived_type, typename _traits>
	class texture_cache
	{
	public:
		using traits = _traits;

		using commandbuffer_type   = typename traits::commandbuffer_type;
		using section_storage_type = typename traits::section_storage_type;
		using image_resource_type  = typename traits::image_resource_type;
		using image_view_type      = typename traits::image_view_type;
		using image_storage_type   = typename traits::image_storage_type;
		using texture_format       = typename traits::texture_format;

		using predictor_type       = texture_cache_predictor<traits>;
		using ranged_storage       = rsx::ranged_storage<traits>;
		using ranged_storage_block = typename ranged_storage::block_type;

	private:
		static_assert(std::is_base_of<rsx::cached_texture_section<section_storage_type, traits>, section_storage_type>::value, "section_storage_type must derive from rsx::cached_texture_section");

		/**
		 * Helper structs/enums
		 */

		// Keep track of cache misses to pre-emptively flush some addresses
		struct framebuffer_memory_characteristics
		{
			u32 misses;
			texture_format format;
		};

	public:
		//Struct to hold data on sections to be paged back onto cpu memory
		struct thrashed_set
		{
			bool violation_handled = false;
			bool flushed = false;
			invalidation_cause cause;
			std::vector<section_storage_type*> sections_to_flush; // Sections to be flushed
			std::vector<section_storage_type*> sections_to_unprotect; // These sections are to be unpotected and discarded by caller
			std::vector<section_storage_type*> sections_to_exclude; // These sections are do be excluded from protection manipulation (subtracted from other sections)
			u32 num_flushable = 0;
			u64 cache_tag = 0;

			address_range fault_range;
			address_range invalidate_range;

			void clear_sections()
			{
				sections_to_flush = {};
				sections_to_unprotect = {};
				sections_to_exclude = {};
				num_flushable = 0;
			}

			bool empty() const
			{
				return sections_to_flush.empty() && sections_to_unprotect.empty() && sections_to_exclude.empty();
			}

			bool is_flushed() const
			{
				return flushed || sections_to_flush.empty();
			}

#ifdef TEXTURE_CACHE_DEBUG
			void check_pre_sanity() const
			{
				size_t flush_and_unprotect_count = sections_to_flush.size() + sections_to_unprotect.size();
				size_t exclude_count = sections_to_exclude.size();

				//-------------------------
				// It is illegal to have only exclusions except when reading from a range with only RO sections
				ASSERT(flush_and_unprotect_count > 0 || exclude_count == 0 || cause.is_read());
				if (flush_and_unprotect_count == 0 && exclude_count > 0)
				{
					// double-check that only RO sections exists
					for (auto *tex : sections_to_exclude)
						ASSERT(tex->get_protection() == utils::protection::ro);
				}

				//-------------------------
				// Check that the number of sections we "found" matches the sections known to be in the fault range
				const auto min_overlap_fault_no_ro = tex_cache_checker.get_minimum_number_of_sections(fault_range);
				const auto min_overlap_invalidate_no_ro = tex_cache_checker.get_minimum_number_of_sections(invalidate_range);

				const u16 min_overlap_fault = min_overlap_fault_no_ro.first + (cause.is_read() ? 0 : min_overlap_fault_no_ro.second);
				const u16 min_overlap_invalidate = min_overlap_invalidate_no_ro.first + (cause.is_read() ? 0 : min_overlap_invalidate_no_ro.second);
				AUDIT(min_overlap_fault <= min_overlap_invalidate);

				const u16 min_flush_or_unprotect = min_overlap_fault;

				// we must flush or unprotect *all* sections that partially overlap the fault range
				ASSERT(flush_and_unprotect_count >= min_flush_or_unprotect);

				// result must contain *all* sections that overlap (completely or partially) the invalidation range
				ASSERT(flush_and_unprotect_count + exclude_count >= min_overlap_invalidate);
			}

			void check_post_sanity() const
			{
				AUDIT(is_flushed());

				// Check that the number of sections we "found" matches the sections known to be in the fault range
				tex_cache_checker.check_unprotected(fault_range, cause.is_read() && invalidation_keep_ro_during_read, true);

				// Check that the cache has the correct protections
				tex_cache_checker.verify();
			}
#endif // TEXTURE_CACHE_DEBUG
		};

		struct intersecting_set
		{
			std::vector<section_storage_type*> sections = {};
			address_range invalidate_range = {};
			bool has_flushables = false;
		};

		enum surface_transform : u32
		{
			identity = 0,
			argb_to_bgra = 1
		};

		struct copy_region_descriptor
		{
			image_resource_type src;
			surface_transform xform;
			u16 src_x;
			u16 src_y;
			u16 dst_x;
			u16 dst_y;
			u16 dst_z;
			u16 src_w;
			u16 src_h;
			u16 dst_w;
			u16 dst_h;
		};

		enum deferred_request_command : u32
		{
			nop = 0,
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
			deferred_request_command op = deferred_request_command::nop;
			u32 base_address = 0;
			u32 gcm_format = 0;
			u16 x = 0;
			u16 y = 0;
			u16 width = 0;
			u16 height = 0;
			u16 depth = 1;
			u16 slice_h = 1;
			bool do_not_cache = false;

			deferred_subresource() = default;

			deferred_subresource(image_resource_type _res, deferred_request_command _op, u32 _addr, u32 _fmt, u16 _x, u16 _y, u16 _w, u16 _h, u16 _d, u16 _s, texture_channel_remap_t _remap) :
				external_handle(_res), op(_op), base_address(_addr), gcm_format(_fmt), x(_x), y(_y), width(_w), height(_h), depth(_d), slice_h(_s), remap(std::move(_remap))
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

			inline address_range to_address_range() const
			{
				return address_range::start_length(real_dst_address, real_dst_size);
			}
		};

		struct sampled_image_descriptor : public sampled_image_descriptor_base
		{
			image_view_type image_handle = 0;
			deferred_subresource external_subresource_desc = {};
			bool flag = false;

			sampled_image_descriptor() = default;

			sampled_image_descriptor(image_view_type handle, texture_upload_context ctx, bool is_depth,
				f32 x_scale, f32 y_scale, rsx::texture_dimension_extended type, bool cyclic_reference = false)
			{
				image_handle = handle;
				upload_context = ctx;
				is_depth_texture = is_depth;
				is_cyclic_reference = cyclic_reference;
				scale_x = x_scale;
				scale_y = y_scale;
				image_type = type;
			}

			sampled_image_descriptor(image_resource_type external_handle, deferred_request_command reason, u32 base_address, u32 gcm_format,
				u16 x_offset, u16 y_offset, u16 width, u16 height, u16 depth, u16 slice_h,
				texture_upload_context ctx, bool is_depth, f32 x_scale, f32 y_scale,
				rsx::texture_dimension_extended type, const texture_channel_remap_t& remap)
			{
				external_subresource_desc = { external_handle, reason, base_address, gcm_format, x_offset, y_offset, width, height, depth, slice_h, remap };

				image_handle = 0;
				upload_context = ctx;
				is_depth_texture = is_depth;
				scale_x = x_scale;
				scale_y = y_scale;
				image_type = type;
			}

			void simplify()
			{
				// Optimizations in the straightforward methods copy_image_static and copy_image_dynamic make them preferred over the atlas method
				if (external_subresource_desc.op == deferred_request_command::atlas_gather &&
					external_subresource_desc.sections_to_copy.size() == 1)
				{
					// Check if the subresource fills the target, if so, change the command to copy_image_static
					const auto &cpy = external_subresource_desc.sections_to_copy.front();
					if (cpy.dst_x == 0 && cpy.dst_y == 0 &&
						cpy.dst_w == external_subresource_desc.width && cpy.dst_h == external_subresource_desc.height &&
						cpy.src_w == cpy.dst_w && cpy.src_h == cpy.dst_h)
					{
						external_subresource_desc.external_handle = cpy.src;
						external_subresource_desc.x = cpy.src_x;
						external_subresource_desc.y = cpy.src_y;
						external_subresource_desc.op = deferred_request_command::copy_image_static;
					}
				}
			}

			bool atlas_covers_target_area() const
			{
				if (external_subresource_desc.op != deferred_request_command::atlas_gather)
					return true;

				u16 min_x = external_subresource_desc.width, min_y = external_subresource_desc.height,
					max_x = 0, max_y = 0;

				// Require at least 50% coverage
				const u32 target_area = (min_x * min_y) / 2;

				for (const auto &section : external_subresource_desc.sections_to_copy)
				{
					if (section.dst_x < min_x) min_x = section.dst_x;
					if (section.dst_y < min_y) min_y = section.dst_y;

					const auto _u = section.dst_x + section.dst_w;
					const auto _v = section.dst_y + section.dst_h;
					if (_u > max_x) max_x = _u;
					if (_v > max_y) max_y = _v;

					if (const auto _w = max_x - min_x, _h = max_y - min_y;
						u32(_w * _h) >= target_area)
					{
						// Target area mostly covered, return success
						return true;
					}
				}

				return false;
			}

			u32 encoded_component_map() const override
			{
				if (image_handle)
				{
					return image_handle->encoded_component_map();
				}

				return 0;
			}

			bool validate() const
			{
				return (image_handle || external_subresource_desc.op != deferred_request_command::nop);
			}
		};


	protected:

		/**
		 * Variable declarations
		 */

		shared_mutex m_cache_mutex;
		ranged_storage m_storage;
		std::unordered_multimap<u32, std::pair<deferred_subresource, image_view_type>> m_temporary_subresource_cache;
		predictor_type m_predictor;

		std::atomic<u64> m_cache_update_tag = {0};

		address_range read_only_range;
		address_range no_access_range;

		//Map of messages to only emit once
		std::unordered_set<std::string> m_once_only_messages_set;

		//Set when a shader read-only texture data suddenly becomes contested, usually by fbo memory
		bool read_only_tex_invalidate = false;

		//Store of all objects in a flush_always state. A lazy readback is attempted every draw call
		std::unordered_map<address_range, section_storage_type*> m_flush_always_cache;
		u64 m_flush_always_update_timestamp = 0;

		//Memory usage
		const u32 m_max_zombie_objects = 64; //Limit on how many texture objects to keep around for reuse after they are invalidated

		//Other statistics
		std::atomic<u32> m_flushes_this_frame = { 0 };
		std::atomic<u32> m_misses_this_frame  = { 0 };
		std::atomic<u32> m_speculations_this_frame = { 0 };
		std::atomic<u32> m_unavoidable_hard_faults_this_frame = { 0 };
		static const u32 m_predict_max_flushes_per_frame = 50; // Above this number the predictions are disabled

		// Invalidation
		static const bool invalidation_ignore_unsynchronized = true; // If true, unsynchronized sections don't get forcefully flushed unless they overlap the fault range
		static const bool invalidation_keep_ro_during_read = true; // If true, RO sections are not invalidated during read faults




		/**
		 * Virtual Methods
		 */
		virtual image_view_type create_temporary_subresource_view(commandbuffer_type&, image_resource_type* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type create_temporary_subresource_view(commandbuffer_type&, image_storage_type* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector) = 0;
		virtual section_storage_type* create_new_texture(commandbuffer_type&, const address_range &rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format,
			rsx::texture_upload_context context, rsx::texture_dimension_extended type, texture_create_flags flags) = 0;
		virtual section_storage_type* upload_image_from_cpu(commandbuffer_type&, const address_range &rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format, texture_upload_context context,
			const std::vector<rsx_subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, bool swizzled) = 0;
		virtual void enforce_surface_creation_type(section_storage_type& section, u32 gcm_format, texture_create_flags expected) = 0;
		virtual void insert_texture_barrier(commandbuffer_type&, image_storage_type* tex) = 0;
		virtual image_view_type generate_cubemap_from_images(commandbuffer_type&, u32 gcm_format, u16 size, const std::vector<copy_region_descriptor>& sources, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type generate_3d_from_2d_images(commandbuffer_type&, u32 gcm_format, u16 width, u16 height, u16 depth, const std::vector<copy_region_descriptor>& sources, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type generate_atlas_from_images(commandbuffer_type&, u32 gcm_format, u16 width, u16 height, const std::vector<copy_region_descriptor>& sections_to_copy, const texture_channel_remap_t& remap_vector) = 0;
		virtual void update_image_contents(commandbuffer_type&, image_view_type dst, image_resource_type src, u16 width, u16 height) = 0;
		virtual bool render_target_format_is_compatible(image_storage_type* tex, u32 gcm_format) = 0;
		virtual void prepare_for_dma_transfers(commandbuffer_type&) = 0;
		virtual void cleanup_after_dma_transfers(commandbuffer_type&) = 0;

	public:
		virtual void destroy() = 0;
		virtual bool is_depth_texture(u32, u32) = 0;
		virtual void on_section_destroyed(section_storage_type& /*section*/)
		{}


	protected:
		/**
		 * Helpers
		 */
		inline void update_cache_tag()
		{
			m_cache_update_tag = rsx::get_shared_tag();
		}

		template <typename... Args>
		void emit_once(bool error, const char* fmt, const Args&... params)
		{
			const auto result = m_once_only_messages_set.emplace(fmt::format(fmt, params...));
			if (!result.second)
				return;

			if (error)
				LOG_ERROR(RSX, "%s", *result.first);
			else
				LOG_WARNING(RSX, "%s", *result.first);
		}

		template <typename... Args>
		void err_once(const char* fmt, const Args&... params)
		{
			emit_once(true, fmt, params...);
		}

		template <typename... Args>
		void warn_once(const char* fmt, const Args&... params)
		{
			emit_once(false, fmt, params...);
		}

		/**
		 * Internal implementation methods and helpers
		 */

		inline bool region_intersects_cache(const address_range &test_range, bool is_writing)
		{
			AUDIT(test_range.valid());

			// Quick range overlaps with cache tests
			if (!is_writing)
			{
				if (!no_access_range.valid() || !test_range.overlaps(no_access_range))
					return false;
			}
			else
			{
				if (!read_only_range.valid() || !test_range.overlaps(read_only_range))
				{
					//Doesnt fall in the read_only textures range; check render targets
					if (!no_access_range.valid() || !test_range.overlaps(no_access_range))
						return false;
				}
			}

			// Check that there is at least one valid (locked) section in the test_range
			reader_lock lock(m_cache_mutex);
			if (m_storage.range_begin(test_range, locked_range, true) == m_storage.range_end())
				return false;

			// We do intersect the cache
			return true;
		}

		/**
		 * Section invalidation
		 */
	private:
		template <typename ...Args>
		void flush_set(commandbuffer_type& cmd, thrashed_set& data, Args&&... extras)
		{
			AUDIT(!data.flushed);

			if (data.sections_to_flush.size() > 1)
			{
				// Sort with oldest data first
				// Ensures that new data tramples older data
				std::sort(data.sections_to_flush.begin(), data.sections_to_flush.end(), [](const auto& a, const auto& b)
				{
					return (a->last_write_tag < b->last_write_tag);
				});
			}

			rsx::simple_array<section_storage_type*> sections_to_transfer;
			for (auto &surface : data.sections_to_flush)
			{
				if (!surface->is_synchronized())
				{
					sections_to_transfer.push_back(surface);
				}
				else if (surface->get_memory_read_flags() == rsx::memory_read_flags::flush_always)
				{
					// This region is set to always read from itself (unavoidable hard sync)
					const auto ROP_timestamp = rsx::get_current_renderer()->ROP_sync_timestamp;
					if (ROP_timestamp > surface->get_sync_timestamp())
					{
						sections_to_transfer.push_back(surface);
					}
				}
			}

			if (!sections_to_transfer.empty())
			{
				// Batch all hard faults together
				prepare_for_dma_transfers(cmd);

				for (auto &surface : sections_to_transfer)
				{
					surface->copy_texture(cmd, true, std::forward<Args>(extras)...);
				}

				cleanup_after_dma_transfers(cmd);
			}

			for (auto &surface : data.sections_to_flush)
			{
				surface->flush();

				// Exclude this region when flushing other sections that should not trample it
				// If we overlap an excluded RO, set it as dirty
				for (auto &other : data.sections_to_exclude)
				{
					AUDIT(other != surface);
					if (!other->is_flushable())
					{
						if (other->overlaps(*surface, section_bounds::full_range))
						{
							other->set_dirty(true);
						}
					}
					else if(surface->last_write_tag > other->last_write_tag)
					{
						other->add_flush_exclusion(surface->get_confirmed_range());
					}
				}
			}

			data.flushed = true;
		}


		// Merges the protected ranges of the sections in "sections" into "result"
		void merge_protected_ranges(address_range_vector &result, const std::vector<section_storage_type*> &sections)
		{
			result.reserve(result.size() + sections.size());

			// Copy ranges to result, merging them if possible
			for (const auto &section : sections)
			{
				const auto &new_range = section->get_locked_range();
				AUDIT(new_range.is_page_range());

				result.merge(new_range);
			}
		}

		// NOTE: It is *very* important that data contains exclusions for *all* sections that overlap sections_to_unprotect/flush
		//       Otherwise the page protections will end up incorrect and things will break!
		void unprotect_set(thrashed_set& data)
		{
			auto protect_ranges = [this](address_range_vector& _set, utils::protection _prot)
			{
				u32 count = 0;
				for (auto &range : _set)
				{
					if (range.valid())
					{
						rsx::memory_protect(range, _prot);
						count++;
					}
				}
				//LOG_ERROR(RSX, "Set protection of %d blocks to 0x%x", count, static_cast<u32>(prot));
			};

			auto discard_set = [this](std::vector<section_storage_type*>& _set)
			{
				for (auto* section : _set)
				{
					verify(HERE), section->is_flushed() || section->is_dirty();

					section->discard(/*set_dirty*/ false);
				}
			};

			// Sanity checks
			AUDIT(data.fault_range.is_page_range());
			AUDIT(data.invalidate_range.is_page_range());
			AUDIT(data.is_flushed());

			// Merge ranges to unprotect
			address_range_vector ranges_to_unprotect;
			address_range_vector ranges_to_protect_ro;
			ranges_to_unprotect.reserve(data.sections_to_unprotect.size() + data.sections_to_flush.size() + data.sections_to_exclude.size());

			merge_protected_ranges(ranges_to_unprotect, data.sections_to_unprotect);
			merge_protected_ranges(ranges_to_unprotect, data.sections_to_flush);
			AUDIT(!ranges_to_unprotect.empty());

			// Apply exclusions and collect ranges of excluded pages that need to be reprotected RO (i.e. only overlap RO regions)
			if (!data.sections_to_exclude.empty())
			{
				ranges_to_protect_ro.reserve(data.sections_to_exclude.size());

				u32 no_access_count = 0;
				for (const auto &excluded : data.sections_to_exclude)
				{
					address_range exclusion_range = excluded->get_locked_range();

					// We need to make sure that the exclusion range is *inside* invalidate range
					exclusion_range.intersect(data.invalidate_range);

					// Sanity checks
					AUDIT(exclusion_range.is_page_range());
					AUDIT(data.cause.is_read() && !excluded->is_flushable() || data.cause.skip_fbos() || !exclusion_range.overlaps(data.fault_range));

					// Apply exclusion
					ranges_to_unprotect.exclude(exclusion_range);

					// Keep track of RO exclusions
					// TODO ruipin: Bug here, we cannot add the whole exclusion range to ranges_to_reprotect, only the part inside invalidate_range
					utils::protection prot = excluded->get_protection();
					if (prot == utils::protection::ro)
					{
						ranges_to_protect_ro.merge(exclusion_range);
					}
					else if (prot == utils::protection::no)
					{
						no_access_count++;
					}
					else
					{
						fmt::throw_exception("Unreachable" HERE);
					}
				}

				// Exclude NA ranges from ranges_to_reprotect_ro
				if (no_access_count > 0 && !ranges_to_protect_ro.empty())
				{
					for (auto &exclusion : data.sections_to_exclude)
					{
						if (exclusion->get_protection() != utils::protection::ro)
						{
							ranges_to_protect_ro.exclude(exclusion->get_locked_range());
						}
					}
				}
			}
			AUDIT(!ranges_to_unprotect.empty());

			// Exclude the fault range if told to do so (this means the fault_range got unmapped or is otherwise invalid)
			if (data.cause.keep_fault_range_protection())
			{
				ranges_to_unprotect.exclude(data.fault_range);
				ranges_to_protect_ro.exclude(data.fault_range);

				AUDIT(!ranges_to_unprotect.overlaps(data.fault_range));
				AUDIT(!ranges_to_protect_ro.overlaps(data.fault_range));
			}
			else
			{
				AUDIT(ranges_to_unprotect.inside(data.invalidate_range));
				AUDIT(ranges_to_protect_ro.inside(data.invalidate_range));
			}
			AUDIT(!ranges_to_protect_ro.overlaps(ranges_to_unprotect));

			// Unprotect and discard
			protect_ranges(ranges_to_unprotect, utils::protection::rw);
			protect_ranges(ranges_to_protect_ro, utils::protection::ro);
			discard_set(data.sections_to_unprotect);
			discard_set(data.sections_to_flush);

#ifdef TEXTURE_CACHE_DEBUG
			// Check that the cache looks sane
			data.check_post_sanity();
#endif // TEXTURE_CACHE_DEBUG
		}

		// Return a set containing all sections that should be flushed/unprotected/reprotected
		std::atomic<u64> m_last_section_cache_tag = 0;
		intersecting_set get_intersecting_set(const address_range &fault_range)
		{
			AUDIT(fault_range.is_page_range());

			const u64 cache_tag = ++m_last_section_cache_tag;

			intersecting_set result = {};
			address_range &invalidate_range = result.invalidate_range;
			invalidate_range = fault_range; // Sections fully inside this range will be invalidated, others will be deemed false positives

			// Loop through cache and find pages that overlap the invalidate_range
			u32 last_dirty_block = UINT32_MAX;
			bool repeat_loop = false;

			// Not having full-range protections means some textures will check the confirmed range and not the locked range
			const bool not_full_range_protected = (buffered_section::guard_policy != protection_policy::protect_policy_full_range);
			section_bounds range_it_bounds = not_full_range_protected ? confirmed_range : locked_range;

			auto It = m_storage.range_begin(invalidate_range, range_it_bounds, true); // will iterate through locked sections only
			while (It != m_storage.range_end())
			{
				const u32 base = It.get_block().get_start();

				// On the last loop, we stop once we're done with the last dirty block
				if (!repeat_loop && base > last_dirty_block) // note: blocks are iterated in order from lowest to highest base address
					break;

				auto &tex = *It;

				AUDIT(tex.is_locked()); // we should be iterating locked sections only, but just to make sure...
				AUDIT(tex.cache_tag != cache_tag || last_dirty_block != UINT32_MAX); // cache tag should not match during the first loop

				if (tex.cache_tag != cache_tag) //flushable sections can be 'clean' but unlocked. TODO: Handle this better
				{
					const rsx::section_bounds bounds = tex.get_overlap_test_bounds();

					if (range_it_bounds == bounds || tex.overlaps(invalidate_range, bounds))
					{
						const auto new_range = tex.get_min_max(invalidate_range, bounds).to_page_range();
						AUDIT(new_range.is_page_range() && invalidate_range.inside(new_range));

						// The various chaining policies behave differently
						bool extend_invalidate_range = tex.overlaps(fault_range, bounds);

						// Extend the various ranges
						if (extend_invalidate_range && new_range != invalidate_range)
						{
							if (new_range.end > invalidate_range.end)
								It.set_end(new_range.end);

							invalidate_range = new_range;
							repeat_loop = true; // we will need to repeat the loop again
							last_dirty_block = base; // stop the repeat loop once we finish this block
						}

						// Add texture to result, and update its cache tag
						tex.cache_tag = cache_tag;
						result.sections.push_back(&tex);

						if (tex.is_flushable())
						{
							result.has_flushables = true;
						}
					}
				}

				// Iterate
				It++;

				// repeat_loop==true means some blocks are still dirty and we need to repeat the loop again
				if (repeat_loop && It == m_storage.range_end())
				{
					It = m_storage.range_begin(invalidate_range, range_it_bounds, true);
					repeat_loop = false;
				}
			}

			AUDIT(result.invalidate_range.is_page_range());

#ifdef TEXTURE_CACHE_DEBUG
			// naive check that sections are not duplicated in the results
			for (auto &section1 : result.sections)
			{
				size_t count = 0;
				for (auto &section2 : result.sections)
				{
					if (section1 == section2) count++;
				}
				verify(HERE), count == 1;
			}
#endif //TEXTURE_CACHE_DEBUG

			return result;
		}


		//Invalidate range base implementation
		template <typename ...Args>
		thrashed_set invalidate_range_impl_base(commandbuffer_type& cmd, const address_range &fault_range_in, invalidation_cause cause, Args&&... extras)
		{
#ifdef TEXTURE_CACHE_DEBUG
			// Check that the cache has the correct protections
			tex_cache_checker.verify();
#endif // TEXTURE_CACHE_DEBUG

			AUDIT(cause.valid());
			AUDIT(fault_range_in.valid());
			address_range fault_range = fault_range_in.to_page_range();

			intersecting_set trampled_set = std::move(get_intersecting_set(fault_range));

			thrashed_set result = {};
			result.cause = cause;
			result.fault_range = fault_range;
			result.invalidate_range = trampled_set.invalidate_range;

			// Fast code-path for keeping the fault range protection when not flushing anything
			if (cause.keep_fault_range_protection() && cause.skip_flush() && !trampled_set.sections.empty())
			{
				verify(HERE), cause != invalidation_cause::committed_as_fbo;

				// We discard all sections fully inside fault_range
				for (auto &obj : trampled_set.sections)
				{
					auto &tex = *obj;
					if (tex.overlaps(fault_range, section_bounds::locked_range))
					{
						if (cause == invalidation_cause::superseded_by_fbo &&
							tex.get_context() == texture_upload_context::framebuffer_storage &&
							tex.get_section_base() != fault_range_in.start)
						{
							// HACK: When being superseded by an fbo, we preserve other overlapped fbos unless the start addresses match
							continue;
						}
						else if (tex.inside(fault_range, section_bounds::locked_range))
						{
							// Discard - this section won't be needed any more
							tex.discard(/* set_dirty */ true);
						}
						else if (g_cfg.video.strict_texture_flushing && tex.is_flushable())
						{
							tex.add_flush_exclusion(fault_range);
						}
						else
						{
							tex.set_dirty(true);
						}
					}
				}

#ifdef TEXTURE_CACHE_DEBUG
				// Notify the checker that fault_range got discarded
				tex_cache_checker.discard(fault_range);
#endif

				// If invalidate_range is fault_range, we can stop now
				const address_range invalidate_range = trampled_set.invalidate_range;
				if (invalidate_range == fault_range)
				{
					result.violation_handled = true;
#ifdef TEXTURE_CACHE_DEBUG
					// Post-check the result
					result.check_post_sanity();
#endif
					return result;
				}
				AUDIT(fault_range.inside(invalidate_range));
			}


			// Decide which sections to flush, unprotect, and exclude
			if (!trampled_set.sections.empty())
			{
				update_cache_tag();

				for (auto &obj : trampled_set.sections)
				{
					auto &tex = *obj;

					if (!tex.is_locked())
						continue;

					const rsx::section_bounds bounds = tex.get_overlap_test_bounds();
					const bool overlaps_fault_range = tex.overlaps(fault_range, bounds);

					if (
						// RO sections during a read invalidation can be ignored (unless there are flushables in trampled_set, since those could overwrite RO data)
						(invalidation_keep_ro_during_read && !trampled_set.has_flushables && cause.is_read() && !tex.is_flushable()) ||
						// Sections that are not fully contained in invalidate_range can be ignored
						!tex.inside(trampled_set.invalidate_range, bounds) ||
						// Unsynchronized sections (or any flushable when skipping flushes) that do not overlap the fault range directly can also be ignored
						(invalidation_ignore_unsynchronized && tex.is_flushable() && (cause.skip_flush() || !tex.is_synchronized()) && !overlaps_fault_range) ||
						// HACK: When being superseded by an fbo, we preserve other overlapped fbos unless the start addresses match
						// If region is committed as fbo, all non-fbo data is removed but all fbos in the region must be preserved if possible
						(overlaps_fault_range && tex.get_context() == texture_upload_context::framebuffer_storage && cause.skip_fbos() && (tex.get_section_base() != fault_range_in.start || cause == invalidation_cause::committed_as_fbo))
					   )
					{
						// False positive
						result.sections_to_exclude.push_back(&tex);
						continue;
					}

					if (tex.is_flushable())
					{
						// Write if and only if no one else has trashed section memory already
						// TODO: Proper section management should prevent this from happening
						// TODO: Blit engine section merge support and/or partial texture memory buffering
						if (tex.is_dirty() || !tex.test_memory_head() || !tex.test_memory_tail())
						{
							// Contents clobbered, destroy this
							if (!tex.is_dirty())
							{
								tex.set_dirty(true);
							}

							result.sections_to_unprotect.push_back(&tex);
						}
						else
						{
							result.sections_to_flush.push_back(&tex);
						}

						continue;
					}
					else
					{
						// deferred_flush = true and not synchronized
						if (!tex.is_dirty())
						{
							AUDIT(tex.get_memory_read_flags() != memory_read_flags::flush_always);
							tex.set_dirty(true);
						}

						result.sections_to_unprotect.push_back(&tex);
						continue;
					}

					fmt::throw_exception("Unreachable " HERE);
				}


				result.violation_handled = true;
#ifdef TEXTURE_CACHE_DEBUG
				// Check that result makes sense
				result.check_pre_sanity();
#endif // TEXTURE_CACHE_DEBUG

				const bool has_flushables = !result.sections_to_flush.empty();
				const bool has_unprotectables = !result.sections_to_unprotect.empty();

				if (cause.deferred_flush() && has_flushables)
				{
					// There is something to flush, but we've been asked to defer it
					result.num_flushable = static_cast<int>(result.sections_to_flush.size());
					result.cache_tag = m_cache_update_tag.load(std::memory_order_consume);
					return result;
				}
				else if (has_flushables || has_unprotectables)
				{
					AUDIT(!has_flushables || !cause.deferred_flush());

					// We have something to flush and are allowed to flush now
					// or there is nothing to flush but we have something to unprotect
					if (has_flushables && !cause.skip_flush())
					{
						flush_set(cmd, result, std::forward<Args>(extras)...);
					}

					unprotect_set(result);

					// Everything has been handled
					result.clear_sections();
				}
				else
				{
					// This is a read and all overlapping sections were RO and were excluded (except for cause == superseded_by_fbo)
					AUDIT(cause.skip_fbos() || cause.is_read() && !result.sections_to_exclude.empty());

					// We did not handle this violation
					result.clear_sections();
					result.violation_handled = false;
				}

#ifdef TEXTURE_CACHE_DEBUG
				// Post-check the result
				result.check_post_sanity();
#endif // TEXTURE_CACHE_DEBUG

				return result;
			}

			return {};
		}

	protected:
		inline bool is_gcm_depth_format(u32 format) const
		{
			switch (format)
			{
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
				return true;
			default:
				return false;
			}
		}

		inline u32 get_compatible_depth_format(u32 gcm_format) const
		{
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_A8R8G8B8:
				return CELL_GCM_TEXTURE_DEPTH24_D8;
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			case CELL_GCM_TEXTURE_X16:
			//case CELL_GCM_TEXTURE_A4R4G4B4:
			//case CELL_GCM_TEXTURE_G8B8:
			//case CELL_GCM_TEXTURE_A1R5G5B5:
			//case CELL_GCM_TEXTURE_R5G5B5A1:
			//case CELL_GCM_TEXTURE_R5G6B5:
			//case CELL_GCM_TEXTURE_R6G5B5:
				return CELL_GCM_TEXTURE_DEPTH16;
			}

			LOG_ERROR(RSX, "Unsupported depth conversion (0x%X)", gcm_format);
			return gcm_format;
		}

		inline bool is_compressed_gcm_format(u32 format)
		{
			switch (format)
			{
			default:
				return false;
			case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
			case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
			case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
			case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
				return true;
			}
		}

	public:

		texture_cache() : m_storage(this), m_predictor(this) {}
		~texture_cache() = default;

		void clear()
		{
			m_storage.clear();
			m_predictor.clear();
		}

		virtual void on_frame_end()
		{
			m_temporary_subresource_cache.clear();
			m_predictor.on_frame_end();
			reset_frame_statistics();
		}

		template <bool check_unlocked = false>
		std::vector<section_storage_type*> find_texture_from_range(const address_range &test_range, u16 required_pitch = 0, u32 context_mask = 0xFF)
		{
			std::vector<section_storage_type*> results;

			for (auto It = m_storage.range_begin(test_range, full_range); It != m_storage.range_end(); It++)
			{
				auto &tex = *It;

				if (!tex.is_dirty() && (context_mask & (u32)tex.get_context()))
				{
					if constexpr (check_unlocked)
					{
						if (!tex.is_locked())
							continue;
					}

					if (required_pitch && !rsx::pitch_compatible<false>(&tex, required_pitch, UINT16_MAX))
					{
						continue;
					}

					results.push_back(&tex);
				}
			}

			return results;
		}

		template <bool check_unlocked = false>
		section_storage_type *find_texture_from_dimensions(u32 rsx_address, u32 format, u16 width = 0, u16 height = 0, u16 depth = 0, u16 mipmaps = 0)
		{
			auto &block = m_storage.block_for(rsx_address);
			for (auto &tex : block)
			{
				if constexpr (check_unlocked)
				{
					if (!tex.is_locked())
						continue;
				}

				if (!tex.is_dirty() && tex.matches(rsx_address, format, width, height, depth, mipmaps))
				{
					return &tex;
				}
			}

			return nullptr;
		}

		section_storage_type* find_cached_texture(const address_range &range, bool create_if_not_found, bool confirm_dimensions, u16 width = 0, u16 height = 0, u16 depth = 0, u16 mipmaps = 0)
		{
			auto &block = m_storage.block_for(range);

			section_storage_type *dimensions_mismatch = nullptr;
			section_storage_type *best_fit = nullptr;
			section_storage_type *reuse = nullptr;
#ifdef TEXTURE_CACHE_DEBUG
			section_storage_type *res = nullptr;
#endif

			// Try to find match in block
			for (auto &tex : block)
			{
				if (tex.matches(range))
				{
					if (!tex.is_dirty())
					{
						if (!confirm_dimensions || tex.matches_dimensions(width, height, depth, mipmaps))
						{
#ifndef TEXTURE_CACHE_DEBUG
							return &tex;
#else
							ASSERT(res == nullptr);
							res = &tex;
#endif
						}
						else if (dimensions_mismatch == nullptr)
						{
							dimensions_mismatch = &tex;
						}
					}
					else if (best_fit == nullptr && tex.can_be_reused())
					{
						//By grabbing a ref to a matching entry, duplicates are avoided
						best_fit = &tex;
					}
				}
				else if (reuse == nullptr && tex.can_be_reused())
				{
					reuse = &tex;
				}
			}

#ifdef TEXTURE_CACHE_DEBUG
			if (res != nullptr)
				return res;
#endif

			if (dimensions_mismatch != nullptr)
			{
				auto &tex = *dimensions_mismatch;
				LOG_WARNING(RSX, "Cached object for address 0x%X was found, but it does not match stored parameters (width=%d vs %d; height=%d vs %d; depth=%d vs %d; mipmaps=%d vs %d)",
					range.start, width, tex.get_width(), height, tex.get_height(), depth, tex.get_depth(), mipmaps, tex.get_mipmaps());
			}

			if (!create_if_not_found)
				return nullptr;

			// If found, use the best fitting section
			if (best_fit != nullptr)
			{
				if (best_fit->exists())
				{
					best_fit->destroy();
				}

				return best_fit;
			}

			// Return the first dirty section found, if any
			if (reuse != nullptr)
			{
				if (reuse->exists())
				{
					reuse->destroy();
				}

				return reuse;
			}

			// Create and return a new section
			update_cache_tag();
			auto tex = &block.create_section();
			return tex;
		}

		section_storage_type* find_flushable_section(const address_range &memory_range)
		{
			auto &block = m_storage.block_for(memory_range);
			for (auto &tex : block)
			{
				if (tex.is_dirty()) continue;
				if (!tex.is_flushable() && !tex.is_flushed()) continue;

				if (tex.matches(memory_range))
					return &tex;
			}

			return nullptr;
		}

		template <typename ...FlushArgs, typename ...Args>
		void lock_memory_region(commandbuffer_type& cmd, image_storage_type* image, const address_range &rsx_range, bool is_active_surface, u32 width, u32 height, u32 pitch, Args&&... extras)
		{
			AUDIT(g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer); // this method is only called when either WCB or WDB are enabled

			std::lock_guard lock(m_cache_mutex);

			// Find a cached section to use
			section_storage_type& region = *find_cached_texture(rsx_range, true, true, width, height);

			// Prepare and initialize fbo region
			if (region.exists() && region.get_context() != texture_upload_context::framebuffer_storage)
			{
				//This space was being used for other purposes other than framebuffer storage
				//Delete used resources before attaching it to framebuffer memory
				read_only_tex_invalidate = true;
			}

			if (!region.is_locked() || region.get_context() != texture_upload_context::framebuffer_storage)
			{
				// Invalidate sections from surface cache occupying same address range
				invalidate_range_impl_base(cmd, rsx_range, invalidation_cause::superseded_by_fbo);
			}

			if (!region.is_locked() || region.can_be_reused())
			{
				// New region, we must prepare it
				region.reset(rsx_range);
				no_access_range = region.get_min_max(no_access_range, rsx::section_bounds::locked_range);
				region.set_context(texture_upload_context::framebuffer_storage);
				region.set_image_type(rsx::texture_dimension_extended::texture_dimension_2d);
			}
			else
			{
				// Re-using clean fbo region
				ASSERT(region.matches(rsx_range));
				ASSERT(region.get_context() == texture_upload_context::framebuffer_storage);
				ASSERT(region.get_image_type() == rsx::texture_dimension_extended::texture_dimension_2d);
			}

			region.create(width, height, 1, 1, image, pitch, false, std::forward<Args>(extras)...);
			region.reprotect(utils::protection::no, { 0, rsx_range.length() });

			region.set_dirty(false);
			region.touch(m_cache_update_tag);

			if (is_active_surface)
			{
				// Add to flush always cache
				if (region.get_memory_read_flags() != memory_read_flags::flush_always)
				{
					region.set_memory_read_flags(memory_read_flags::flush_always, false);
					update_flush_always_cache(region, true);
				}
				else
				{
					AUDIT(m_flush_always_cache.find(region.get_section_range()) != m_flush_always_cache.end());
				}
			}

			update_cache_tag();

#ifdef TEXTURE_CACHE_DEBUG
			// Check that the cache makes sense
			tex_cache_checker.verify();
#endif // TEXTURE_CACHE_DEBUG
		}

		template <typename ...Args>
		void commit_framebuffer_memory_region(commandbuffer_type& cmd, const address_range &rsx_range, Args&&... extras)
		{
			AUDIT(!g_cfg.video.write_color_buffers && !g_cfg.video.write_depth_buffer);

			if (!region_intersects_cache(rsx_range, true))
				return;

			std::lock_guard lock(m_cache_mutex);
			invalidate_range_impl_base(cmd, rsx_range, invalidation_cause::committed_as_fbo, std::forward<Args>(extras)...);
		}

		void set_memory_read_flags(const address_range &memory_range, memory_read_flags flags)
		{
			std::lock_guard lock(m_cache_mutex);

			auto* region_ptr = find_cached_texture(memory_range, false, false);
			if (region_ptr == nullptr)
			{
				AUDIT(m_flush_always_cache.find(memory_range) == m_flush_always_cache.end());
				LOG_ERROR(RSX, "set_memory_flags(0x%x, 0x%x, %d): region_ptr == nullptr", memory_range.start, memory_range.end, static_cast<u32>(flags));
				return;
			}

			auto& region = *region_ptr;

			if (!region.exists() || region.is_dirty() || region.get_context() != texture_upload_context::framebuffer_storage)
			{
#ifdef TEXTURE_CACHE_DEBUG
				if (!region.is_dirty())
				{
					if (flags == memory_read_flags::flush_once)
						verify(HERE), m_flush_always_cache.find(memory_range) == m_flush_always_cache.end();
					else
						verify(HERE), m_flush_always_cache[memory_range] == &region;
				}
#endif // TEXTURE_CACHE_DEBUG
				return;
			}

			update_flush_always_cache(region, flags == memory_read_flags::flush_always);
			region.set_memory_read_flags(flags, false);
		}

		virtual void on_memory_read_flags_changed(section_storage_type &section, rsx::memory_read_flags flags)
		{
#ifdef TEXTURE_CACHE_DEBUG
			const auto &memory_range = section.get_section_range();
			if (flags == memory_read_flags::flush_once)
				verify(HERE), m_flush_always_cache[memory_range] == &section;
			else
				verify(HERE), m_flush_always_cache.find(memory_range) == m_flush_always_cache.end();
#endif
			update_flush_always_cache(section, flags == memory_read_flags::flush_always);
		}

	private:
		inline void update_flush_always_cache(section_storage_type &section, bool add)
		{
			const address_range& range = section.get_section_range();
			if (add)
			{
				// Add to m_flush_always_cache
				AUDIT(m_flush_always_cache.find(range) == m_flush_always_cache.end());
				m_flush_always_cache[range] = &section;
			}
			else
			{
				// Remove from m_flush_always_cache
				AUDIT(m_flush_always_cache[range] == &section);
				m_flush_always_cache.erase(range);
			}
		}

	public:
		template <typename ...Args>
		bool load_memory_from_cache(const address_range &memory_range, Args&&... extras)
		{
			reader_lock lock(m_cache_mutex);
			section_storage_type *region = find_flushable_section(memory_range);

			if (region && !region->is_dirty())
			{
				region->fill_texture(std::forward<Args>(extras)...);
				return true;
			}

			//No valid object found in cache
			return false;
		}

		template <typename ...Args>
		thrashed_set invalidate_address(commandbuffer_type& cmd, u32 address, invalidation_cause cause, Args&&... extras)
		{
			//Test before trying to acquire the lock
			const auto range = page_for(address);
			if (!region_intersects_cache(range, !cause.is_read()))
				return{};

			std::lock_guard lock(m_cache_mutex);
			return invalidate_range_impl_base(cmd, range, cause, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		thrashed_set invalidate_range(commandbuffer_type& cmd, const address_range &range, invalidation_cause cause, Args&&... extras)
		{
			//Test before trying to acquire the lock
			if (!region_intersects_cache(range, !cause.is_read()))
				return {};

			std::lock_guard lock(m_cache_mutex);
			return invalidate_range_impl_base(cmd, range, cause, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		bool flush_all(commandbuffer_type& cmd, thrashed_set& data, Args&&... extras)
		{
			std::lock_guard lock(m_cache_mutex);

			AUDIT(data.cause.deferred_flush());
			AUDIT(!data.flushed);

			if (m_cache_update_tag.load(std::memory_order_consume) == data.cache_tag)
			{
				//1. Write memory to cpu side
				flush_set(cmd, data, std::forward<Args>(extras)...);

				//2. Release all obsolete sections
				unprotect_set(data);
			}
			else
			{
				// The cache contents have changed between the two readings. This means the data held is useless
				invalidate_range_impl_base(cmd, data.fault_range, data.cause.undefer(), std::forward<Args>(extras)...);
			}

			return true;
		}

		template <typename ...Args>
		bool flush_if_cache_miss_likely(commandbuffer_type& cmd, const address_range &range, Args&&... extras)
		{
			u32 cur_flushes_this_frame = (m_flushes_this_frame + m_speculations_this_frame);

			if (cur_flushes_this_frame > m_predict_max_flushes_per_frame)
				return false;

			auto& block = m_storage.block_for(range);
			if (block.empty())
				return false;

			reader_lock lock(m_cache_mutex);

			// Try to find matching regions
			bool result = false;
			for (auto &region : block)
			{
				if (region.is_dirty() || region.is_synchronized() || !region.is_flushable())
					continue;

				if (!region.matches(range))
					continue;

				if (!region.tracked_by_predictor())
					continue;

				if (!m_predictor.predict(region))
					continue;

				lock.upgrade();

				region.copy_texture(cmd, false, std::forward<Args>(extras)...);
				result = true;

				cur_flushes_this_frame++;
				if (cur_flushes_this_frame > m_predict_max_flushes_per_frame)
					return result;
			}

			return result;
		}

		void purge_unreleased_sections()
		{
			std::lock_guard lock(m_cache_mutex);

			m_storage.purge_unreleased_sections();
		}

		image_view_type create_temporary_subresource(commandbuffer_type &cmd, deferred_subresource& desc)
		{
			if (!desc.do_not_cache)
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
					sections[n] =
					{
						desc.external_handle,
						surface_transform::identity,
						0, (u16)(desc.slice_h * n),
						0, 0, n,
						desc.width, desc.height,
						desc.width, desc.height
					};
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
					sections[n] =
					{
						desc.external_handle,
						surface_transform::identity,
						0, (u16)(desc.slice_h * n),
						0, 0, n,
						desc.width, desc.height,
						desc.width, desc.height
					};
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

		void notify_surface_changed(const utils::address_range& range)
		{
			for (auto It = m_temporary_subresource_cache.begin(); It != m_temporary_subresource_cache.end();)
			{
				if (range.overlaps(It->first))
				{
					It = m_temporary_subresource_cache.erase(It);
				}
				else
				{
					++It;
				}
			}
		}

		template<typename surface_store_list_type>
		std::vector<copy_region_descriptor> gather_texture_slices(commandbuffer_type& cmd,
			const surface_store_list_type& fbos, const std::vector<section_storage_type*>& local,
			u32 texaddr, u16 slice_w, u16 slice_h, u16 src_padding, u16 pitch, u16 count, u8 bpp, bool is_depth)
		{
			// Need to preserve sorting order
			struct sort_helper
			{
				u64 tag;   // Timestamp
				u32 list;  // List source, 0 = fbo, 1 = local
				u32 index; // Index in list
			};

			std::vector<copy_region_descriptor> surfaces;
			std::vector<sort_helper> sort_list;
			const u16 src_slice_h = slice_h + src_padding;

			if (!fbos.empty() && !local.empty())
			{
				// Generate sorting tree if both resources are available and overlapping
				sort_list.reserve(fbos.size() + local.size());

				for (u32 index = 0; index < fbos.size(); ++index)
				{
					sort_list.push_back({ fbos[index].surface->last_use_tag, 0, index });
				}

				for (u32 index = 0; index < local.size(); ++index)
				{
					if (local[index]->get_context() != rsx::texture_upload_context::blit_engine_dst)
						continue;

					sort_list.push_back({ local[index]->last_write_tag, 1, index });
				}

				std::sort(sort_list.begin(), sort_list.end(), [](const auto &a, const auto &b)
				{
					return (a.tag < b.tag);
				});
			}

			auto add_rtt_resource = [&](auto& section, u16 slice)
			{
				if (section.is_depth != is_depth)
				{
					// TODO
					return;
				}

				const auto slice_begin = (slice * src_slice_h);
				const auto slice_end = (slice_begin + slice_h);

				const auto section_end = section.dst_y + section.height;
				if (section.dst_y >= slice_end || section_end <= slice_begin)
				{
					// Belongs to a different slice
					return;
				}

				// How much of this slice to read?
				int rebased = int(section.dst_y) - slice_begin;
				const auto src_x = section.src_x;
				const auto dst_x = section.dst_x;
				auto src_y = section.src_y;
				auto dst_y = section.dst_y;

				if (rebased < 0)
				{
					const u16 delta = u16(-rebased);
					src_y += delta;
					dst_y += delta;
				}

				verify(HERE), dst_y >= slice_begin;
				dst_y = (dst_y - slice_begin);

				const auto h = std::min(section_end, slice_end) - section.dst_y;
				const auto src_width = rsx::apply_resolution_scale(section.width, true);
				const auto src_height = rsx::apply_resolution_scale(h, true);
				const auto dst_width = src_width;
				const auto dst_height = src_height;

				surfaces.push_back
				({
					section.surface->get_surface(rsx::surface_access::read),
					surface_transform::identity,
					rsx::apply_resolution_scale(src_x, true),
					rsx::apply_resolution_scale(src_y, true),
					rsx::apply_resolution_scale(dst_x, true),
					rsx::apply_resolution_scale(dst_y, true),
					slice,
					src_width, src_height,
					dst_width, dst_height
				});
			};

			auto add_local_resource = [&](auto& section, u32 address, u16 slice, bool scaling = true)
			{
				if (section->is_depth_texture() != is_depth)
				{
					// TODO
					return;
				}

				// Intersect this resource with the original one
				const auto section_bpp = get_format_block_size_in_bytes(section->get_gcm_format());
				const auto normalized_width = (section->get_width() * section_bpp) / bpp;

				const auto clipped = rsx::intersect_region(
					section->get_section_base(), normalized_width, section->get_height(), section_bpp, /* parent region (extractee) */
					address, slice_w, slice_h, bpp, /* child region (extracted) */
					pitch);

				// Rect intersection test
				// TODO: Make the intersection code cleaner with proper 2D regions
				if (std::get<0>(clipped).x >= section->get_width())
				{
					// Overlap lies outside the image area!
					return;
				}

				const auto slice_begin = u32(slice * src_slice_h);
				const auto slice_end = u32(slice_begin + slice_h);

				const auto dst_y = std::get<1>(clipped).y;
				const auto dst_h = std::get<2>(clipped).height;

				const auto section_end = dst_y + dst_h;
				if (dst_y >= slice_end || section_end <= slice_begin)
				{
					// Belongs to a different slice
					return;
				}

				const u16 internal_clip_width = u16(std::get<2>(clipped).width * bpp) / section_bpp;
				if (scaling)
				{
					// Since output is upscaled, also upscale on dst
					surfaces.push_back
					({
						section->get_raw_texture(),
						is_depth ? surface_transform::identity : surface_transform::argb_to_bgra,
						(u16)std::get<0>(clipped).x,
						(u16)std::get<0>(clipped).y,
						rsx::apply_resolution_scale((u16)std::get<1>(clipped).x, true),
						rsx::apply_resolution_scale((u16)std::get<1>(clipped).y, true),
						slice,
						internal_clip_width,
						(u16)std::get<2>(clipped).height,
						rsx::apply_resolution_scale(internal_clip_width, true),
						rsx::apply_resolution_scale((u16)std::get<2>(clipped).height, true),
					});
				}
				else
				{
					const auto src_width = internal_clip_width, dst_width = src_width;
					const auto src_height = (u16)std::get<2>(clipped).height, dst_height = src_height;
					surfaces.push_back
					({
						section->get_raw_texture(),
						is_depth ? surface_transform::identity : surface_transform::argb_to_bgra,
						(u16)std::get<0>(clipped).x,
						(u16)std::get<0>(clipped).y,
						(u16)std::get<1>(clipped).x,
						(u16)std::get<1>(clipped).y,
						0,
						src_width,
						src_height,
						dst_width,
						dst_height,
					});
				}
			};

			u32 current_address = texaddr;
			u16 current_src_offset = 0;
			u16 current_dst_offset = 0;
			u32 slice_size = (pitch * src_slice_h);

			surfaces.reserve(count);
			u16 found_slices = 0;

			for (u16 slice = 0; slice < count; ++slice)
			{
				auto num_surface = surfaces.size();

				if (LIKELY(local.empty()))
				{
					for (auto &section : fbos)
					{
						add_rtt_resource(section, slice);
					}
				}
				else if (fbos.empty())
				{
					for (auto &section : local)
					{
						add_local_resource(section, current_address, slice, false);
					}
				}
				else
				{
					for (const auto &e : sort_list)
					{
						if (e.list == 0)
						{
							add_rtt_resource(fbos[e.index], slice);
						}
						else
						{
							add_local_resource(local[e.index], current_address, slice);
						}
					}
				}

				current_address += slice_size;
				if (surfaces.size() != num_surface)
				{
					found_slices++;
				}
			}

			if (found_slices < count)
			{
				if (found_slices > 0)
				{
					//TODO: Gather remaining sides from the texture cache or upload from cpu (too slow?)
					LOG_ERROR(RSX, "Could not gather all required slices for cubemap/3d generation");
				}
				else
				{
					LOG_WARNING(RSX, "Could not gather textures into an atlas; using CPU fallback...");
				}
			}

			return surfaces;
		}

		template<typename render_target_type>
		bool check_framebuffer_resource(commandbuffer_type& cmd, render_target_type texptr,
			u16 tex_width, u16 tex_height, u16 tex_depth, u16 tex_pitch, u16 slice_h,
			rsx::texture_dimension_extended extended_dimension)
		{
			if (!rsx::pitch_compatible(texptr, tex_pitch, tex_height))
			{
				return false;
			}

			const auto surface_width = texptr->get_surface_width(rsx::surface_metrics::samples);
			const auto surface_height = texptr->get_surface_height(rsx::surface_metrics::samples);

			switch (extended_dimension)
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				return (surface_width >= tex_width);
			case rsx::texture_dimension_extended::texture_dimension_2d:
				return (surface_width >= tex_width && surface_height >= tex_height);
			case rsx::texture_dimension_extended::texture_dimension_3d:
				return (surface_width >= tex_width && surface_height >= (slice_h * tex_depth));
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				return (surface_width == tex_height && surface_width >= tex_width && surface_height >= (slice_h * 6));
			}

			return false;
		}

		template <typename render_target_type>
		sampled_image_descriptor process_framebuffer_resource_fast(commandbuffer_type& cmd, render_target_type texptr,
			u32 texaddr, u32 format,
			u16 tex_width, u16 tex_height, u16 tex_depth, u16 slice_h,
			f32 scale_x, f32 scale_y,
			rsx::texture_dimension_extended extended_dimension,
			u32 encoded_remap, const texture_channel_remap_t& decoded_remap,
			bool surface_is_rop_target)
		{
			texptr->read_barrier(cmd);

			const auto surface_width = texptr->get_surface_width(rsx::surface_metrics::samples);
			const auto surface_height = texptr->get_surface_height(rsx::surface_metrics::samples);

			bool is_depth = texptr->is_depth_surface();
			const bool force_convert = !render_target_format_is_compatible(texptr, format);

			if (const bool gcm_format_is_depth = is_gcm_depth_format(format);
				gcm_format_is_depth != is_depth)
			{
				if (force_convert)
				{
					is_depth = gcm_format_is_depth;
				}
				else
				{
					format = get_compatible_depth_format(format);
				}

				// Always make sure the conflict is resolved!
				verify(HERE), is_gcm_depth_format(format) == is_depth;
			}

			if (LIKELY(extended_dimension == rsx::texture_dimension_extended::texture_dimension_2d ||
				extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d))
			{
				if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
				{
					tex_height = 1;
				}

				if ((surface_is_rop_target && g_cfg.video.strict_rendering_mode) ||
					tex_width < surface_width ||
					tex_height < surface_height ||
					force_convert)
				{
					const auto scaled_w = rsx::apply_resolution_scale(tex_width, true);
					const auto scaled_h = rsx::apply_resolution_scale(tex_height, true);

					const auto command = surface_is_rop_target ? deferred_request_command::copy_image_dynamic : deferred_request_command::copy_image_static;
					return { texptr->get_surface(rsx::surface_access::read), command, texaddr, format, 0, 0, scaled_w, scaled_h, 1, 0,
							texture_upload_context::framebuffer_storage, is_depth, scale_x, scale_y,
							extended_dimension, decoded_remap };
				}

				if (surface_is_rop_target)
				{
					insert_texture_barrier(cmd, texptr);
				}

				return{ texptr->get_view(encoded_remap, decoded_remap), texture_upload_context::framebuffer_storage,
					is_depth, scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d, surface_is_rop_target };
			}

			const auto scaled_w = rsx::apply_resolution_scale(tex_width, true);
			const auto scaled_h = rsx::apply_resolution_scale(tex_height, true);

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_3d)
			{
				return{ texptr->get_surface(rsx::surface_access::read), deferred_request_command::_3d_unwrap, texaddr, format, 0, 0,
						scaled_w, scaled_h, tex_depth, slice_h,
						texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
						rsx::texture_dimension_extended::texture_dimension_3d, decoded_remap };
			}

			verify(HERE), extended_dimension == rsx::texture_dimension_extended::texture_dimension_cubemap;
			return{ texptr->get_surface(rsx::surface_access::read), deferred_request_command::cubemap_unwrap, texaddr, format, 0, 0,
					scaled_w, scaled_h, 1, slice_h,
					texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
					rsx::texture_dimension_extended::texture_dimension_cubemap, decoded_remap };
		}

		template <typename surface_store_list_type>
		sampled_image_descriptor merge_cache_resources(commandbuffer_type& cmd, const surface_store_list_type& fbos, const std::vector<section_storage_type*>& local,
				u32 texaddr, u32 format,
				u16 tex_width, u16 tex_height, u16 tex_depth, u16 tex_pitch, u16 slice_h,
				f32 scale_x, f32 scale_y,
				rsx::texture_dimension_extended extended_dimension,
				u32 encoded_remap, const texture_channel_remap_t& decoded_remap,
				int select_hint = -1)
		{
			verify(HERE), (select_hint & 0x1) == select_hint;

			bool is_depth;
			if (is_depth = (select_hint == 0) ? fbos.back().is_depth : local.back()->is_depth_texture();
				is_depth)
			{
				if (const auto suggested_format = get_compatible_depth_format(format);
					!is_gcm_depth_format(suggested_format))
				{
					// Failed!
					is_depth = false;
				}
				else
				{
					format = suggested_format;
				}
			}

			// If this method was called, there is no easy solution, likely means atlas gather is needed
			auto scaled_w = rsx::apply_resolution_scale(tex_width, true);
			auto scaled_h = rsx::apply_resolution_scale(tex_height, true);

			const auto bpp = get_format_block_size_in_bytes(format);

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_cubemap)
			{
				sampled_image_descriptor desc = { nullptr, deferred_request_command::cubemap_gather, texaddr, format, 0, 0,
						scaled_w, scaled_w, 1, slice_h,
						texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
						rsx::texture_dimension_extended::texture_dimension_cubemap, decoded_remap };

				u16 padding = u16(slice_h - tex_width);
				desc.external_subresource_desc.sections_to_copy = std::move(gather_texture_slices(cmd, fbos, local, texaddr, tex_width, tex_height, padding, tex_pitch, 6, bpp, is_depth));
				return desc;
			}
			else if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_3d && tex_depth > 1)
			{
				sampled_image_descriptor desc = { nullptr, deferred_request_command::_3d_gather, texaddr, format, 0, 0,
					scaled_w, scaled_h, tex_depth, slice_h,
					texture_upload_context::framebuffer_storage, is_depth, 1.f, 1.f,
					rsx::texture_dimension_extended::texture_dimension_3d, decoded_remap };

				u16 padding = u16(slice_h - tex_height);
				desc.external_subresource_desc.sections_to_copy = std::move(gather_texture_slices(cmd, fbos, local, texaddr, tex_width, tex_height, padding, tex_pitch, tex_depth, bpp, is_depth));
				return desc;
			}

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
			{
				verify(HERE), tex_height == 1;
			}

			const auto w = fbos.empty()? tex_width : rsx::apply_resolution_scale(tex_width, true);
			const auto h = fbos.empty()? tex_height : rsx::apply_resolution_scale(tex_height, true);

			sampled_image_descriptor result = { nullptr, deferred_request_command::atlas_gather,
					texaddr, format, 0, 0, w, h, 1, slice_h, texture_upload_context::framebuffer_storage, is_depth,
					scale_x, scale_y, rsx::texture_dimension_extended::texture_dimension_2d, decoded_remap };

			result.external_subresource_desc.sections_to_copy = gather_texture_slices(cmd, fbos, local, texaddr, tex_width, tex_height, 0, tex_pitch, 1, bpp, is_depth);
			result.simplify();
			return result;
		}

		template <typename RsxTextureType, typename surface_store_type, typename ...Args>
		sampled_image_descriptor upload_texture(commandbuffer_type& cmd, RsxTextureType& tex, surface_store_type& m_rtts, Args&&... extras)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 tex_size = (u32)get_texture_size(tex);
			const address_range tex_range = address_range::start_length(texaddr, tex_size);
			const u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			const bool is_compressed_format = is_compressed_gcm_format(format);
			const bool unnormalized = (tex.format() & CELL_GCM_TEXTURE_UN) != 0;
			const bool linear = !!(tex.format() & CELL_GCM_TEXTURE_LN);

			const auto extended_dimension = tex.get_extended_texture_dimension();
			u16 tex_width = tex.width();
			u16 tex_height = tex.height();
			u16 tex_pitch = linear? (u16)tex.pitch() : get_format_packed_pitch(format, tex_width, !tex.border_type(), !linear);

			u16 depth;
			u32 required_surface_height, slice_h;
			switch (extended_dimension)
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				depth = 1;
				slice_h = required_surface_height = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				depth = 1;
				slice_h = required_surface_height = tex_height;
				break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				depth = 6;
				required_surface_height = tex_range.length() / tex_pitch;
				slice_h = required_surface_height / depth;
				break;
			case rsx::texture_dimension_extended::texture_dimension_3d:
				depth = tex.depth();
				required_surface_height = tex_range.length() / tex_pitch;
				slice_h = required_surface_height / depth;
				break;
			}

			f32 scale_x = (unnormalized) ? (1.f / tex_width) : 1.f;
			f32 scale_y = (unnormalized) ? (1.f / tex_height) : 1.f;

			if (!tex_pitch)
			{
				// Linear scanning with pitch of 0, read only texel (0,0)
				tex_pitch = get_format_packed_pitch(format, tex_width, !tex.border_type(), !linear);
				scale_x = 0.f;
				scale_y = 0.f;
			}
			else if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
			{
				scale_y = 0.f;
			}

			// Sanity check
			if (UNLIKELY(unnormalized && extended_dimension > rsx::texture_dimension_extended::texture_dimension_2d))
			{
				LOG_ERROR(RSX, "Unimplemented unnormalized sampling for texture type %d", (u32)extended_dimension);
			}

			if (UNLIKELY(m_rtts.address_is_bound(texaddr)))
			{
				if (auto texptr = m_rtts.get_surface_at(texaddr);
					check_framebuffer_resource(cmd, texptr, tex_width, tex_height, depth, tex_pitch, slice_h, extended_dimension))
				{
					return process_framebuffer_resource_fast(cmd, texptr, texaddr, format, tex_width, tex_height, depth, slice_h,
						scale_x, scale_y, extended_dimension, tex.remap(), tex.decoded_remap(), true);
				}
			}

			reader_lock lock(m_cache_mutex);

			if (LIKELY(is_compressed_format))
			{
				// Most mesh textures are stored as compressed to make the most of the limited memory
				if (auto cached_texture = find_texture_from_dimensions(texaddr, format, tex_width, tex_height, depth))
				{
					return{ cached_texture->get_view(tex.remap(), tex.decoded_remap()), cached_texture->get_context(), cached_texture->is_depth_texture(), scale_x, scale_y, cached_texture->get_image_type() };
				}
			}
			else
			{
				// Check shader_read storage. In a given scene, reads from local memory far outnumber reads from the surface cache
				const u32 lookup_mask = (is_compressed_format) ? rsx::texture_upload_context::shader_read :
					rsx::texture_upload_context::shader_read | rsx::texture_upload_context::blit_engine_dst | rsx::texture_upload_context::blit_engine_src;

				auto lookup_range = tex_range;
				if (LIKELY(extended_dimension <= rsx::texture_dimension_extended::texture_dimension_2d))
				{
					// Optimize the range a bit by only searching for mip0, layer0 to avoid false positives
					const auto texel_rows_per_line = get_format_texel_rows_per_line(format);
					const auto num_rows = (tex_height + texel_rows_per_line - 1) / texel_rows_per_line;
					if (const auto length = u32(num_rows * tex_pitch); length < tex_range.length())
					{
						lookup_range = utils::address_range::start_length(texaddr, length);
					}
				}

				const auto overlapping_locals = find_texture_from_range<true>(lookup_range, tex_height > 1? tex_pitch : 0, lookup_mask);
				for (auto& cached_texture : overlapping_locals)
				{
					if (cached_texture->matches(texaddr, format, tex_width, tex_height, depth, 0))
					{
						return{ cached_texture->get_view(tex.remap(), tex.decoded_remap()), cached_texture->get_context(), cached_texture->is_depth_texture(), scale_x, scale_y, cached_texture->get_image_type() };
					}
				}

				// Next, attempt to merge blit engine and surface store
				// Blit sources contain info from any shader-read stuff in range
				// NOTE: Compressed formats require a reupload, facilitated by blit synchronization and/or WCB and are not handled here

				const auto bpp = get_format_block_size_in_bytes(format);
				const auto overlapping_fbos = m_rtts.get_merged_texture_memory_region(cmd, texaddr, tex_width, required_surface_height, tex_pitch, bpp, rsx::surface_access::read);

				if (!overlapping_fbos.empty() || !overlapping_locals.empty())
				{
					int _pool = -1;
					if (LIKELY(overlapping_locals.empty()))
					{
						_pool = 0;
					}
					else if (overlapping_fbos.empty())
					{
						_pool = 1;
					}
					else
					{
						_pool = (overlapping_locals.back()->last_write_tag < overlapping_fbos.back().surface->last_use_tag) ? 0 : 1;
					}

					if (_pool == 0)
					{
						// Surface cache data is newer, check if this thing fits our search parameters
						const auto& last = overlapping_fbos.back();
						if (last.src_x == 0 && last.src_y == 0)
						{
							u16 normalized_width = u16(last.width * last.surface->get_bpp()) / bpp;
							if (normalized_width >= tex_width && last.height >= required_surface_height)
							{
								return process_framebuffer_resource_fast(cmd, last.surface, texaddr, format, tex_width, tex_height, depth, slice_h,
									scale_x, scale_y, extended_dimension, tex.remap(), tex.decoded_remap(), false);
							}
						}
					}
					else if (extended_dimension <= rsx::texture_dimension_extended::texture_dimension_2d)
					{
						const auto last = overlapping_locals.back();
						const auto normalized_width = u16(last->get_width() * get_format_block_size_in_bytes(last->get_gcm_format())) / bpp;

						if (last->get_section_base() == texaddr &&
							normalized_width >= tex_width && last->get_height() >= tex_height)
						{
							bool is_depth = last->is_depth_texture();
							u32  gcm_format = format;

							if (const auto gcm_format_is_depth = is_gcm_depth_format(format);
								is_depth != gcm_format_is_depth)
							{
								// Conflict, resolve
								if (gcm_format_is_depth)
								{
									is_depth = true;
								}
								else
								{
									const auto actual_format = last->get_gcm_format();
									bool  resolved = false;

									switch (format)
									{
									case CELL_GCM_TEXTURE_A8R8G8B8:
									case CELL_GCM_TEXTURE_D8R8G8B8:
									{
										// Compatible with D24S8_UINT
										if (actual_format == CELL_GCM_TEXTURE_DEPTH24_D8)
										{
											gcm_format = CELL_GCM_TEXTURE_DEPTH24_D8;
											resolved = true;
											is_depth = true;
										}
										break;
									}
									case CELL_GCM_TEXTURE_X16:
									{
										// Compatible with DEPTH16_UNORM
										if (actual_format == CELL_GCM_TEXTURE_DEPTH16)
										{
											gcm_format = CELL_GCM_TEXTURE_DEPTH16;
											resolved = true;
											is_depth = true;
										}
										break;
									}
									}

									if (!resolved)
									{
										LOG_ERROR(RSX, "Reading texture with gcm format 0x%x as unexpected cast with format 0x%x",
											actual_format, format);

										is_depth = gcm_format_is_depth;
									}
								}
							}

							return { last->get_raw_texture(), deferred_request_command::copy_image_static, texaddr, gcm_format, 0, 0,
									tex_width, tex_height, 1, 0, last->get_context(), is_depth,
									scale_x, scale_y, extended_dimension, tex.decoded_remap() };
						}
					}

					auto result = merge_cache_resources(cmd, overlapping_fbos, overlapping_locals,
						texaddr, format, tex_width, tex_height, depth, tex_pitch, slice_h,
						scale_x, scale_y, extended_dimension, tex.remap(), tex.decoded_remap(), _pool);

					if (!result.external_subresource_desc.sections_to_copy.empty() &&
						(_pool == 0 || result.atlas_covers_target_area()))
					{
						// TODO: Overlapped section persistance is required for framebuffer resources to work with this!
						// Yellow filter in SCV is because of a 384x384 surface being reused as 160x90 (and likely not getting written to)
						// Its then sampled again here as 384x384 and this does not work! (obviously)

						// Optionally disallow caching if resource is being written to as it is being read from
						for (const auto &section : overlapping_fbos)
						{
							if (m_rtts.address_is_bound(section.base_address))
							{
								if (result.external_subresource_desc.op == deferred_request_command::copy_image_static)
								{
									result.external_subresource_desc.op = deferred_request_command::copy_image_dynamic;
								}
								else
								{
									result.external_subresource_desc.do_not_cache = true;
								}

								break;
							}
						}

						return result;
					}
#ifdef TEXTURE_CACHE_DEBUG
					else
					{
						LOG_ERROR(RSX, "Area merge failed! addr=0x%x, w=%d, h=%d, gcm_format=0x%x[sz=%d]", texaddr, tex_width, tex_height, format, !(tex.format() & CELL_GCM_TEXTURE_LN));
						for (const auto &s : overlapping_locals)
						{
							if (s->get_context() == rsx::texture_upload_context::blit_engine_dst)
							{
								LOG_ERROR(RSX, "Btw, you're about to lose a blit surface at 0x%x", s->get_section_base());
							}
						}
						//LOG_TRACE(RSX, "Partial memory recovered from cache; may require WCB/WDB to properly gather all the data");
					}
#endif // TEXTURE_CACHE_DEBUG
				}
			}

			// Do direct upload from CPU as the last resort
			const bool is_swizzled = !(tex.format() & CELL_GCM_TEXTURE_LN);
			auto subresources_layout = get_subresources_layout(tex);

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

			lock.upgrade();

			//Invalidate
			invalidate_range_impl_base(cmd, tex_range, invalidation_cause::read, std::forward<Args>(extras)...);

			//NOTE: SRGB correction is to be handled in the fragment shader; upload as linear RGB
			return{ upload_image_from_cpu(cmd, tex_range, tex_width, tex_height, depth, tex.get_exact_mipmap_count(), tex_pitch, format,
				texture_upload_context::shader_read, subresources_layout, extended_dimension, is_swizzled)->get_view(tex.remap(), tex.decoded_remap()),
				texture_upload_context::shader_read, is_depth_format, scale_x, scale_y, extended_dimension };
		}

		template <typename surface_store_type, typename blitter_type, typename ...Args>
		blit_op_result upload_scaled_image(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, commandbuffer_type& cmd, surface_store_type& m_rtts, blitter_type& blitter, Args&&... extras)
		{
			// Since we will have dst in vram, we can 'safely' ignore the swizzle flag
			// TODO: Verify correct behavior
			bool src_is_render_target = false;
			bool dst_is_render_target = false;
			const bool dst_is_argb8 = (dst.format == rsx::blit_engine::transfer_destination_format::a8r8g8b8);
			const bool src_is_argb8 = (src.format == rsx::blit_engine::transfer_source_format::a8r8g8b8);
			const u8 src_bpp = src_is_argb8 ? 4 : 2;
			const u8 dst_bpp = dst_is_argb8 ? 4 : 2;

			typeless_xfer typeless_info = {};
			image_resource_type vram_texture = 0;
			image_resource_type dest_texture = 0;

			const u32 dst_address = vm::get_addr(dst.pixels);
			u32 src_address = vm::get_addr(src.pixels);

			const f32 scale_x = fabsf(dst.scale_x);
			const f32 scale_y = fabsf(dst.scale_y);

			// Offset in x and y for src is 0 (it is already accounted for when getting pixels_src)
			// Reproject final clip onto source...
			u16 src_w = (u16)((f32)dst.clip_width / scale_x);
			u16 src_h = (u16)((f32)dst.clip_height / scale_y);

			u16 dst_w = dst.clip_width;
			u16 dst_h = dst.clip_height;

			if (true) // This block is a debug/sanity check and should be optionally disabled with a config option
			{
				// Do subpixel correction in the special case of reverse scanning
				// When reverse scanning, pixel0 is at offset = (dimension - 1)
				if (dst.scale_y < 0.f && src.offset_y)
				{
					if (src.offset_y = (src.height - src.offset_y);
						src.offset_y == 1)
					{
						src.offset_y = 0;
					}
				}

				if (dst.scale_x < 0.f && src.offset_x)
				{
					if (src.offset_x = (src.width - src.offset_x);
						src.offset_x == 1)
					{
						src.offset_x = 0;
					}
				}

				if (UNLIKELY((src_h + src.offset_y) > src.height))
				{
					// TODO: Special case that needs wrapping around (custom blit)
					LOG_ERROR(RSX, "Transfer cropped in Y, src_h=%d, offset_y=%d, block_h=%d", src_h, src.offset_y, src.height);

					src_h = src.height - src.offset_y;
					dst_h = u16(src_h * scale_y + 0.000001f);
				}

				if (UNLIKELY((src_w + src.offset_x) > src.width))
				{
					// TODO: Special case that needs wrapping around (custom blit)
					LOG_ERROR(RSX, "Transfer cropped in X, src_w=%d, offset_x=%d, block_w=%d", src_w, src.offset_x, src.width);

					src_w = src.width - src.offset_x;
					dst_w = u16(src_w * scale_x + 0.000001f);
				}
			}

			if (dst.scale_y < 0.f)
			{
				typeless_info.flip_vertical = true;
				src_address -= (src.pitch * (src_h - 1));
			}

			if (dst.scale_x < 0.f)
			{
				typeless_info.flip_horizontal = true;
				src_address += (src.width - src_w) * src_bpp;
			}

			auto rtt_lookup = [&m_rtts, &cmd, &scale_x, &scale_y, this](u32 address, u32 width, u32 height, u32 pitch, u8 bpp, bool allow_clipped) -> typename surface_store_type::surface_overlap_info
			{
				const auto list = m_rtts.get_merged_texture_memory_region(cmd, address, width, height, pitch, bpp, rsx::surface_access::transfer);
				if (list.empty())
				{
					return {};
				}

				for (auto It = list.rbegin(); It != list.rend(); ++It)
				{
					if (!(It->surface->memory_usage_flags & rsx::surface_usage_flags::attachment))
					{
						// HACK
						// TODO: Properly analyse the input here to determine if it can properly fit what we need
						// This is a problem due to chunked transfer
						// First 2 512x720 blocks go into a cpu-side buffer but suddenly when its time to render the final 256x720
						// it falls onto some storage buffer in surface cache that has bad dimensions
						// Proper solution is to always merge when a cpu resource is created (it should absorb the render targets in range)
						// We then should not have any 'dst-is-rendertarget' surfaces in use
						// Option 2: Make surfaces here part of surface cache and do not pad them for optimization
						// Surface cache is good at merging for resolve operations. This keeps integrity even when drawing to the rendertgargets
						// This option needs a lot more work
						continue;
					}

					if (!It->is_clipped || allow_clipped)
					{
						return *It;
					}

					auto _w = u32(It->width * It->surface->get_bpp()) / bpp;
					auto _h = u32(It->height);

					if (_w < width)
					{
						if ((_w * scale_x) <= 1.f)
							continue;
					}

					if (_h < height)
					{
						if ((_h * scale_y) <= 1.f)
							continue;
					}

					// Some surface exists, but its size is questionable
					// Opt to re-upload (needs WCB/WDB to work properly)
					break;
				}

				return {};
			};

			// Check if src/dst are parts of render targets
			typename surface_store_type::surface_overlap_info dst_subres;
			if (dst_address > 0xc0000000)
			{
				// TODO: HACK
				// After writing, it is required to lock the memory range from access!
				dst_subres = rtt_lookup(dst_address, dst_w, dst_h, dst.pitch, dst_bpp, false);
				dst_is_render_target = dst_subres.surface != nullptr;
			}
			else
			{
				// Surface exists in local memory.
				// 1. Invalidate surfaces in range
				// 2. Proceed as normal, blit into a 'normal' surface and any upload routines should catch it
				m_rtts.invalidate_range(utils::address_range::start_length(dst_address, dst.pitch * dst_h));
			}

			// TODO: Handle cases where src or dst can be a depth texture while the other is a color texture - requires a render pass to emulate
			auto src_subres = rtt_lookup(src_address, src_w, src_h, src.pitch, src_bpp, false);
			src_is_render_target = src_subres.surface != nullptr;

			// Always use GPU blit if src or dst is in the surface store
			if (!g_cfg.video.use_gpu_texture_scaling && !(src_is_render_target || dst_is_render_target))
				return false;

			// Check if trivial memcpy can perform the same task
			// Used to copy programs and arbitrary data to the GPU in some cases
			if (!src_is_render_target && !dst_is_render_target && dst_is_argb8 == src_is_argb8 && !dst.swizzled)
			{
				if ((src_h == 1 && dst_h == 1) || (dst_w == src_w && dst_h == src_h && src.pitch == dst.pitch))
				{
					if (dst.scale_x > 0.f && dst.scale_y > 0.f)
					{
						const u32 memcpy_bytes_length = dst.clip_width * dst_bpp * dst.clip_height;

						std::lock_guard lock(m_cache_mutex);
						invalidate_range_impl_base(cmd, address_range::start_length(src_address, memcpy_bytes_length), invalidation_cause::read, std::forward<Args>(extras)...);
						invalidate_range_impl_base(cmd, address_range::start_length(dst_address, memcpy_bytes_length), invalidation_cause::write, std::forward<Args>(extras)...);
						memcpy(dst.pixels, src.pixels, memcpy_bytes_length);
						return true;
					}
					else
					{
						// Rotation transform applied, use fallback
						return false;
					}
				}
			}

			// Sanity and format compatibility checks
			if (dst_is_render_target)
			{
				if (src_subres.is_depth != dst_subres.is_depth)
				{
					// Create a cache-local resource to resolve later
					// TODO: Support depth->RGBA typeless transfer for vulkan
					dst_is_render_target = false;
				}
			}

			if (src_is_render_target)
			{
				const auto surf = src_subres.surface;
				const auto bpp = surf->get_bpp();
				if (bpp != src_bpp)
				{
					//Enable type scaling in src
					typeless_info.src_is_typeless = true;
					typeless_info.src_is_depth = src_subres.is_depth;
					typeless_info.src_scaling_hint = (f32)bpp / src_bpp;
					typeless_info.src_gcm_format = src_is_argb8 ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;
				}
			}

			if (dst_is_render_target)
			{
				auto bpp = dst_subres.surface->get_bpp();
				if (bpp != dst_bpp)
				{
					//Enable type scaling in dst
					typeless_info.dst_is_typeless = true;
					typeless_info.dst_is_depth = dst_subres.is_depth;
					typeless_info.dst_scaling_hint = (f32)bpp / dst_bpp;
					typeless_info.dst_gcm_format = dst_is_argb8 ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;
				}
			}

			section_storage_type* cached_dest = nullptr;
			u16 max_dst_width = dst.width;
			u16 max_dst_height = dst.height;
			areai src_area = { 0, 0, src_w, src_h };
			areai dst_area = { 0, 0, dst_w, dst_h };

			size2i dst_dimensions = { dst.pitch / dst_bpp, dst.height };
			if (src_is_render_target)
			{
				if (dst_dimensions.width == src_subres.surface->get_surface_width(rsx::surface_metrics::samples))
				{
					dst_dimensions.height = std::max(src_subres.surface->get_surface_height(rsx::surface_metrics::samples), dst.height);
				}
				else if (LIKELY(dst_dimensions.width == 1280 || dst_dimensions.width == 2560))
				{
					// Optimizations table based on common width/height pairings. If we guess wrong, the upload resolver will fix it anyway
					// TODO: Add more entries based on empirical data
					dst_dimensions.height = std::max<s32>(dst.height, 720);
				}
				else
				{
					//LOG_TRACE(RSX, "Blit transfer to surface with dims %dx%d", dst_dimensions.width, dst.height);
				}
			}

			reader_lock lock(m_cache_mutex);

			const auto old_dst_area = dst_area;
			if (!dst_is_render_target)
			{
				// Check for any available region that will fit this one
				auto overlapping_surfaces = find_texture_from_range(address_range::start_length(dst_address, dst.pitch * dst.clip_height), dst.pitch, rsx::texture_upload_context::blit_engine_dst);
				for (const auto &surface : overlapping_surfaces)
				{
					if (!surface->is_locked())
					{
						// Discard
						surface->set_dirty(true);
						continue;
					}

					if (cached_dest)
					{
						// Nothing to do
						continue;
					}

					const auto this_address = surface->get_section_base();
					if (this_address > dst_address)
					{
						continue;
					}

					if (const u32 address_offset = dst_address - this_address)
					{
						const u16 offset_y = address_offset / dst.pitch;
						const u16 offset_x = address_offset % dst.pitch;
						const u16 offset_x_in_block = offset_x / dst_bpp;

						dst_area.x1 += offset_x_in_block;
						dst_area.x2 += offset_x_in_block;
						dst_area.y1 += offset_y;
						dst_area.y2 += offset_y;
					}

					// Validate clipping region
					if ((unsigned)dst_area.x2 <= surface->get_width() &&
						(unsigned)dst_area.y2 <= surface->get_height())
					{
						cached_dest = surface;
						dest_texture = cached_dest->get_raw_texture();
						typeless_info.dst_context = cached_dest->get_context();

						max_dst_width = cached_dest->get_width();
						max_dst_height = cached_dest->get_height();
						continue;
					}

					dst_area = old_dst_area;
				}
			}
			else
			{
				// Destination dimensions are relaxed (true)
				dst_area = dst_subres.get_src_area();

				dest_texture = dst_subres.surface->get_surface(rsx::surface_access::transfer);
				typeless_info.dst_context = texture_upload_context::framebuffer_storage;

				max_dst_width = (u16)(dst_subres.surface->get_surface_width(rsx::surface_metrics::samples) * typeless_info.dst_scaling_hint);
				max_dst_height = dst_subres.surface->get_surface_height(rsx::surface_metrics::samples);
			}

			// Check if available target is acceptable
			// TODO: Check for other types of format mismatch
			bool format_mismatch = false;
			if (cached_dest)
			{
				if (cached_dest->is_depth_texture() != src_subres.is_depth)
				{
					// Dest surface has the wrong 'aspect'
					format_mismatch = true;
				}
				else
				{
					// Check if it matches the transfer declaration
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
			}

			if (format_mismatch)
			{
				// The invalidate call before creating a new target will remove this section
				cached_dest = nullptr;
				dest_texture = 0;
				dst_area = old_dst_area;
			}

			// Create source texture if does not exist
			if (!src_is_render_target)
			{
				const u32 lookup_mask = rsx::texture_upload_context::blit_engine_src | rsx::texture_upload_context::blit_engine_dst;
				auto overlapping_surfaces = find_texture_from_range<false>(address_range::start_length(src_address, src.pitch * src.height), src.pitch, lookup_mask);

				auto old_src_area = src_area;
				for (const auto &surface : overlapping_surfaces)
				{
					if (!surface->is_locked())
					{
						// TODO: Rejecting unlocked blit_engine dst causes stutter in SCV
						// Surfaces marked as dirty have already been removed, leaving only flushed blit_dst data
						continue;
					}

					const auto this_address = surface->get_section_base();
					if (this_address > src_address)
					{
						continue;
					}

					if (const u32 address_offset = src_address - this_address)
					{
						const u16 offset_y = address_offset / src.pitch;
						const u16 offset_x = address_offset % src.pitch;
						const u16 offset_x_in_block = offset_x / src_bpp;

						src_area.x1 += offset_x_in_block;
						src_area.x2 += offset_x_in_block;
						src_area.y1 += offset_y;
						src_area.y2 += offset_y;
					}

					if (src_area.x2 <= surface->get_width() &&
						src_area.y2 <= surface->get_height())
					{
						vram_texture = surface->get_raw_texture();
						typeless_info.src_context = surface->get_context();
						break;
					}

					src_area = old_src_area;
				}

				if (!vram_texture)
				{
					const u16 full_width = src.pitch / src_bpp;
					u32 image_base = src.rsx_address;
					u16 image_width = full_width;
					u16 image_height = src.height;

					if (dst.scale_x > 0.f && dst.scale_y > 0.f)
					{
						// Loading full image from the corner address
						// Translate src_area into the declared block
						src_area.x1 += src.offset_x;
						src_area.x2 += src.offset_x;
						src_area.y1 += src.offset_y;
						src_area.y2 += src.offset_y;
					}
					else if (!src.offset_x && !src.offset_y)
					{
						if (dst.scale_y < 0.f)
						{
							image_base = src.rsx_address - (src.pitch * src.height);
						}
						else
						{
							// Reverse X without reverse Y and no offset in X. Is this even possible?
							LOG_ERROR(RSX, "Unexpected scaling parameters: reversed X without reverse Y");
							image_base = src.rsx_address - src.pitch;
						}
					}
					else
					{
						// It is difficult to determine the transfer region
						image_base = src_address;
						image_width = src_w;
						image_height = src_h;
					}

					lock.upgrade();

					const auto rsx_range = address_range::start_length(image_base, src.pitch * image_height);
					invalidate_range_impl_base(cmd, rsx_range, invalidation_cause::read, std::forward<Args>(extras)...);

					std::vector<rsx_subresource_layout> subresource_layout;
					rsx_subresource_layout subres = {};
					subres.width_in_block = image_width;
					subres.height_in_block = image_height;
					subres.pitch_in_block = full_width;
					subres.depth = 1;
					subres.data = { reinterpret_cast<const gsl::byte*>(vm::base(image_base)), src.pitch * image_height };
					subresource_layout.push_back(subres);

					const u32 gcm_format = src_is_argb8 ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;
					vram_texture = upload_image_from_cpu(cmd, rsx_range, image_width, image_height, 1, 1, src.pitch, gcm_format, texture_upload_context::blit_engine_src,
						subresource_layout, rsx::texture_dimension_extended::texture_dimension_2d, dst.swizzled)->get_raw_texture();

					typeless_info.src_context = texture_upload_context::blit_engine_src;
				}
			}
			else
			{
				src_area = src_subres.get_src_area();
				vram_texture = src_subres.surface->get_surface(rsx::surface_access::read);
				typeless_info.src_context = texture_upload_context::framebuffer_storage;
			}

			// Type of blit decided by the source, destination use should adapt on the fly
			const bool is_depth_blit = src_subres.is_depth;
			u32 gcm_format;

			if (is_depth_blit)
				gcm_format = (dst_is_argb8) ? CELL_GCM_TEXTURE_DEPTH24_D8 : CELL_GCM_TEXTURE_DEPTH16;
			else
				gcm_format = (dst_is_argb8) ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;

			if (cached_dest)
			{
				// Prep surface
				auto channel_order = src_is_render_target ? rsx::texture_create_flags::native_component_order :
					dst_is_argb8 ? rsx::texture_create_flags::default_component_order :
					rsx::texture_create_flags::swapped_native_component_order;

				enforce_surface_creation_type(*cached_dest, gcm_format, channel_order);
			}

			// Validate clipping region
			if ((dst.offset_x + dst.clip_x + dst.clip_width) > max_dst_width) dst.clip_x = 0;
			if ((dst.offset_y + dst.clip_y + dst.clip_height) > max_dst_height) dst.clip_y = 0;

			// Reproject clip offsets onto source to simplify blit
			if (dst.clip_x || dst.clip_y)
			{
				const u16 scaled_clip_offset_x = (const u16)((f32)dst.clip_x / (scale_x * typeless_info.src_scaling_hint));
				const u16 scaled_clip_offset_y = (const u16)((f32)dst.clip_y / scale_y);

				src_area.x1 += scaled_clip_offset_x;
				src_area.x2 += scaled_clip_offset_x;
				src_area.y1 += scaled_clip_offset_y;
				src_area.y2 += scaled_clip_offset_y;
			}

			// Calculate number of bytes actually modified
			u32 mem_base, mem_length;
			if (dst_is_render_target)
			{
				mem_base = dst_address - dst_subres.base_address;
			}
			else
			{
				mem_base = dst_address - dst.rsx_address;
			}

			if (dst.clip_height == 1)
			{
				mem_length = dst.clip_width * dst_bpp;
			}
			else
			{
				const u32 mem_excess = mem_base % dst.pitch;
				mem_length = (dst.pitch * dst.clip_height) - mem_excess;
			}

			const auto modified_range = utils::address_range::start_length(dst_address, mem_length);

			if (dest_texture == 0)
			{
				verify(HERE), !dst_is_render_target;

				// Need to calculate the minium required size that will fit the data, anchored on the rsx_address
				// If the application starts off with an 'inseted' section, the guessed dimensions may not fit!
				const u32 write_end = dst_address + (dst.pitch * dst_h);
				const u32 expected_end = dst.rsx_address + (dst.pitch * dst_dimensions.height);

				const u32 section_length = std::max(write_end, expected_end) - dst.rsx_address;
				dst_dimensions.height = section_length / dst.pitch;

				// render target data is already in correct swizzle layout
				auto channel_order = src_is_render_target ? rsx::texture_create_flags::native_component_order :
					dst_is_argb8 ? rsx::texture_create_flags::default_component_order :
					rsx::texture_create_flags::swapped_native_component_order;

				// Translate dst_area into the 'full' dst block based on dst.rsx_address as (0, 0)
				dst_area.x1 += dst.offset_x;
				dst_area.x2 += dst.offset_x;
				dst_area.y1 += dst.offset_y;
				dst_area.y2 += dst.offset_y;

				lock.upgrade();

				// NOTE: Write flag set to remove all other overlapping regions (e.g shader_read or blit_src)
				const auto rsx_range = address_range::start_length(dst.rsx_address, section_length);
				invalidate_range_impl_base(cmd, rsx_range, invalidation_cause::write, std::forward<Args>(extras)...);

				if (!dst_area.x1 && !dst_area.y1 && dst_area.x2 == dst_dimensions.width && dst_area.y2 == dst_dimensions.height)
				{
					cached_dest = create_new_texture(cmd, rsx_range, dst_dimensions.width, dst_dimensions.height, 1, 1, dst.pitch,
						gcm_format, rsx::texture_upload_context::blit_engine_dst, rsx::texture_dimension_extended::texture_dimension_2d,
						channel_order);
				}
				else
				{
					// HACK: workaround for data race with Cell
					// Pre-lock the memory range we'll be touching, then load with super_ptr
					const auto prot_range = modified_range.to_page_range();
					utils::memory_protect(vm::base(prot_range.start), prot_range.length(), utils::protection::no);

					const u16 pitch_in_block = dst.pitch / dst_bpp;
					std::vector<rsx_subresource_layout> subresource_layout;
					rsx_subresource_layout subres = {};
					subres.width_in_block = dst_dimensions.width;
					subres.height_in_block = dst_dimensions.height;
					subres.pitch_in_block = pitch_in_block;
					subres.depth = 1;
					subres.data = { reinterpret_cast<const gsl::byte*>(vm::get_super_ptr(dst.rsx_address)), dst.pitch * dst_dimensions.height };
					subresource_layout.push_back(subres);

					cached_dest = upload_image_from_cpu(cmd, rsx_range, dst_dimensions.width, dst_dimensions.height, 1, 1, dst.pitch,
						gcm_format, rsx::texture_upload_context::blit_engine_dst, subresource_layout,
						rsx::texture_dimension_extended::texture_dimension_2d, false);

					enforce_surface_creation_type(*cached_dest, gcm_format, channel_order);
				}

				dest_texture = cached_dest->get_raw_texture();
				typeless_info.dst_context = texture_upload_context::blit_engine_dst;
			}

			verify(HERE), cached_dest || dst_is_render_target;

			// Invalidate any cached subresources in modified range
			notify_surface_changed(modified_range);

			if (cached_dest)
			{
				lock.upgrade();

				verify(HERE), (mem_base + mem_length) <= cached_dest->get_section_size();

				cached_dest->reprotect(utils::protection::no, { mem_base, mem_length });
				cached_dest->touch(m_cache_update_tag);
				update_cache_tag();
			}
			else
			{
				// NOTE: This doesn't work very well in case of Cell access
				// Need to lock the affected memory range and actually attach this subres to a locked_region
				dst_subres.surface->on_write_copy(rsx::get_shared_tag());
				m_rtts.notify_memory_structure_changed();
			}

			if (rsx::get_resolution_scale_percent() != 100)
			{
				const f32 resolution_scale = rsx::get_resolution_scale();
				if (src_is_render_target)
				{
					if (src_subres.surface->get_surface_width(rsx::surface_metrics::pixels) > g_cfg.video.min_scalable_dimension)
					{
						src_area.x1 = (u16)(src_area.x1 * resolution_scale);
						src_area.x2 = (u16)(src_area.x2 * resolution_scale);
					}

					if (src_subres.surface->get_surface_height(rsx::surface_metrics::pixels) > g_cfg.video.min_scalable_dimension)
					{
						src_area.y1 = (u16)(src_area.y1 * resolution_scale);
						src_area.y2 = (u16)(src_area.y2 * resolution_scale);
					}
				}

				if (dst_is_render_target)
				{
					if (dst_subres.surface->get_surface_width(rsx::surface_metrics::pixels) > g_cfg.video.min_scalable_dimension)
					{
						dst_area.x1 = (u16)(dst_area.x1 * resolution_scale);
						dst_area.x2 = (u16)(dst_area.x2 * resolution_scale);
					}

					if (dst_subres.surface->get_surface_height(rsx::surface_metrics::pixels) > g_cfg.video.min_scalable_dimension)
					{
						dst_area.y1 = (u16)(dst_area.y1 * resolution_scale);
						dst_area.y2 = (u16)(dst_area.y2 * resolution_scale);
					}
				}
			}

			if (src_is_render_target)
			{
				// The resource is of surface type; possibly disabled AA emulation
				src_subres.surface->transform_blit_coordinates(rsx::surface_access::transfer, src_area);
			}

			if (dst_is_render_target)
			{
				// The resource is of surface type; possibly disabled AA emulation
				dst_subres.surface->transform_blit_coordinates(rsx::surface_access::transfer, dst_area);
			}

			typeless_info.analyse();
			blitter.scale_image(cmd, vram_texture, dest_texture, src_area, dst_area, interpolate, is_depth_blit, typeless_info);

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
			if (!m_flush_always_cache.empty())
			{
				if (m_cache_update_tag.load(std::memory_order_consume) != m_flush_always_update_timestamp)
				{
					std::lock_guard lock(m_cache_mutex);
					bool update_tag = false;

					for (const auto &It : m_flush_always_cache)
					{
						auto& section = *(It.second);
						if (section.get_protection() != utils::protection::no)
						{
							verify(HERE), section.exists();
							AUDIT(section.get_context() == texture_upload_context::framebuffer_storage);
							AUDIT(section.get_memory_read_flags() == memory_read_flags::flush_always);

							section.reprotect(utils::protection::no);
							update_tag = true;
						}
					}

					if (update_tag) update_cache_tag();
					m_flush_always_update_timestamp = m_cache_update_tag.load(std::memory_order_consume);

#ifdef TEXTURE_CACHE_DEBUG
					// Check that the cache has the correct protections
					m_storage.verify_protection();
#endif // TEXTURE_CACHE_DEBUG
				}
			}
		}

		predictor_type& get_predictor()
		{
			return m_predictor;
		}


		/**
		 * The read only texture invalidate flag is set if a read only texture is trampled by framebuffer memory
		 * If set, all cached read only textures are considered invalid and should be re-fetched from the texture cache
		 */
		void clear_ro_tex_invalidate_intr()
		{
			read_only_tex_invalidate = false;
		}

		bool get_ro_tex_invalidate_intr() const
		{
			return read_only_tex_invalidate;
		}


		/**
		 * Per-frame statistics
		 */
		void reset_frame_statistics()
		{
			m_flushes_this_frame.store(0u);
			m_misses_this_frame.store(0u);
			m_speculations_this_frame.store(0u);
			m_unavoidable_hard_faults_this_frame.store(0u);
		}

		void on_flush()
		{
			m_flushes_this_frame++;
		}

		void on_speculative_flush()
		{
			m_speculations_this_frame++;
		}

		void on_misprediction()
		{
			m_predictor.on_misprediction();
		}

		void on_miss(const section_storage_type& section)
		{
			m_misses_this_frame++;

			if (section.get_memory_read_flags() == memory_read_flags::flush_always)
			{
				m_unavoidable_hard_faults_this_frame++;
			}
		}

		virtual const u32 get_unreleased_textures_count() const
		{
			return m_storage.m_unreleased_texture_objects;
		}

		const u64 get_texture_memory_in_use() const
		{
			return m_storage.m_texture_memory_in_use;
		}

		u32 get_num_flush_requests() const
		{
			return m_flushes_this_frame;
		}

		u32 get_num_cache_mispredictions() const
		{
			return m_predictor.m_mispredictions_this_frame;
		}

		u32 get_num_cache_speculative_writes() const
		{
			return m_speculations_this_frame;
		}

		u32 get_num_cache_misses() const
		{
			return m_misses_this_frame;
		}

		u32 get_num_unavoidable_hard_faults() const
		{
			return m_unavoidable_hard_faults_this_frame;
		}

		f32 get_cache_miss_ratio() const
		{
			const auto num_flushes = m_flushes_this_frame.load();
			return (num_flushes == 0u) ? 0.f : (f32)m_misses_this_frame.load() / num_flushes;
		}
	};
}
