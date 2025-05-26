#pragma once

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Core/RSXContext.h"
#include "Emu/RSX/RSXThread.h"
#include "texture_cache_utils.h"
#include "texture_cache_predictor.h"
#include "texture_cache_helpers.h"

#include <unordered_map>

#define RSX_GCM_FORMAT_IGNORED 0

namespace rsx
{
	namespace helpers = rsx::texture_cache_helpers;

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
		using viewable_image_type  = typename traits::viewable_image_type;

		using predictor_type       = texture_cache_predictor<traits>;
		using ranged_storage       = rsx::ranged_storage<traits>;
		using ranged_storage_block = typename ranged_storage::block_type;

		using copy_region_descriptor = copy_region_descriptor_base<typename traits::image_resource_type>;

	private:
		static_assert(std::is_base_of_v<rsx::cached_texture_section<section_storage_type, traits>, section_storage_type>, "section_storage_type must derive from rsx::cached_texture_section");

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
			bool invalidate_samplers = false;
			invalidation_cause cause;
			std::vector<section_storage_type*> sections_to_flush; // Sections to be flushed
			std::vector<section_storage_type*> sections_to_unprotect; // These sections are to be unpotected and discarded by caller
			std::vector<section_storage_type*> sections_to_exclude; // These sections are do be excluded from protection manipulation (subtracted from other sections)
			u32 num_flushable = 0;
			u32 num_excluded = 0;  // Sections-to-exclude + sections that would have been excluded but are false positives
			u32 num_discarded = 0;
			u64 cache_tag = 0;

			address_range32 fault_range;
			address_range32 invalidate_range;

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
				usz flush_and_unprotect_count = sections_to_flush.size() + sections_to_unprotect.size();
				usz exclude_count = sections_to_exclude.size();

				//-------------------------
				// It is illegal to have only exclusions except when reading from a range with only RO sections
				ensure(flush_and_unprotect_count > 0 || exclude_count == 0 || cause.is_read());
				if (flush_and_unprotect_count == 0 && exclude_count > 0)
				{
					// double-check that only RO sections exists
					for (auto *tex : sections_to_exclude)
						ensure(tex->get_protection() == utils::protection::ro);
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
				ensure(flush_and_unprotect_count >= min_flush_or_unprotect);

				// result must contain *all* sections that overlap (completely or partially) the invalidation range
				ensure(flush_and_unprotect_count + exclude_count >= min_overlap_invalidate);
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
			rsx::simple_array<section_storage_type*> sections = {};
			address_range32 invalidate_range = {};
			bool has_flushables = false;
		};

		struct deferred_subresource : image_section_attributes_t
		{
			image_resource_type external_handle = 0;
			std::vector<copy_region_descriptor> sections_to_copy;
			texture_channel_remap_t remap;
			deferred_request_command op = deferred_request_command::nop;
			u32 external_ref_addr = 0;
			u16 x = 0;
			u16 y = 0;

			utils::address_range32 cache_range;
			bool do_not_cache = false;

			deferred_subresource() = default;

			deferred_subresource(image_resource_type _res, deferred_request_command _op,
				const image_section_attributes_t& attr, position2u offset,
				texture_channel_remap_t _remap)
				: external_handle(_res)
				, remap(std::move(_remap))
				, op(_op)
				, x(offset.x)
				, y(offset.y)
			{
				static_cast<image_section_attributes_t&>(*this) = attr;
			}

			viewable_image_type as_viewable() const
			{
				return static_cast<viewable_image_type>(external_handle);
			}

			image_resource_type src0() const
			{
				if (external_handle)
				{
					return external_handle;
				}

				if (!sections_to_copy.empty())
				{
					return sections_to_copy[0].src;
				}

				// Return typed null
				return external_handle;
			}
		};

		struct sampled_image_descriptor : public sampled_image_descriptor_base
		{
			image_view_type image_handle = 0;
			deferred_subresource external_subresource_desc = {};
			bool flag = false;

			sampled_image_descriptor() = default;

			sampled_image_descriptor(image_view_type handle, texture_upload_context ctx, rsx::format_class ftype,
				size3f scale, rsx::texture_dimension_extended type, bool cyclic_reference = false,
				u8 msaa_samples = 1)
			{
				image_handle = handle;
				upload_context = ctx;
				format_class = ftype;
				is_cyclic_reference = cyclic_reference;
				image_type = type;
				samples = msaa_samples;

				texcoord_xform.scale[0] = scale.width;
				texcoord_xform.scale[1] = scale.height;
				texcoord_xform.scale[2] = scale.depth;
				texcoord_xform.bias[0] = 0.;
				texcoord_xform.bias[1] = 0.;
				texcoord_xform.bias[2] = 0.;
				texcoord_xform.clamp = false;
			}

			sampled_image_descriptor(image_resource_type external_handle, deferred_request_command reason,
				const image_section_attributes_t& attr, position2u src_offset,
				texture_upload_context ctx, rsx::format_class ftype, size3f scale,
				rsx::texture_dimension_extended type, const texture_channel_remap_t& remap)
			{
				external_subresource_desc = { external_handle, reason, attr, src_offset, remap };

				image_handle = 0;
				upload_context = ctx;
				format_class = ftype;
				image_type = type;

				texcoord_xform.scale[0] = scale.width;
				texcoord_xform.scale[1] = scale.height;
				texcoord_xform.scale[2] = scale.depth;
				texcoord_xform.bias[0] = 0.;
				texcoord_xform.bias[1] = 0.;
				texcoord_xform.bias[2] = 0.;
				texcoord_xform.clamp = false;
			}

			inline bool section_fills_target(const copy_region_descriptor& cpy) const
			{
				return cpy.dst_x == 0 && cpy.dst_y == 0 &&
					cpy.dst_w == external_subresource_desc.width && cpy.dst_h == external_subresource_desc.height;
			}

			inline bool section_is_transfer_only(const copy_region_descriptor& cpy) const
			{
				return cpy.src_w == cpy.dst_w && cpy.src_h == cpy.dst_h;
			}

			void simplify()
			{
				if (external_subresource_desc.op != deferred_request_command::atlas_gather)
				{
					// Only atlas simplification supported for now
					return;
				}

				auto& sections = external_subresource_desc.sections_to_copy;
				if (sections.size() > 1)
				{
					// GPU image copies are expensive, cull unnecessary transfers if possible
					for (auto idx = sections.size() - 1; idx >= 1; idx--)
					{
						if (section_fills_target(sections[idx]))
						{
							const auto remaining = sections.size() - idx;
							std::memcpy(
								sections.data(),
								&sections[idx],
								remaining * sizeof(sections[0])
							);
							sections.resize(remaining);
							break;
						}
					}
				}

				// Optimizations in the straightforward methods copy_image_static and copy_image_dynamic make them preferred over the atlas method
				if (sections.size() == 1 && section_fills_target(sections[0]))
				{
					const auto cpy = sections[0];
					external_subresource_desc.external_ref_addr = cpy.base_addr;

					if (section_is_transfer_only(cpy))
					{
						// Change the command to copy_image_static
						external_subresource_desc.external_handle = cpy.src;
						external_subresource_desc.x = cpy.src_x;
						external_subresource_desc.y = cpy.src_y;
						external_subresource_desc.width = cpy.src_w;
						external_subresource_desc.height = cpy.src_h;
						external_subresource_desc.op = deferred_request_command::copy_image_static;
					}
					else
					{
						// Blit op is a semantic variant of the copy and atlas ops.
						// We can simply reuse the atlas handler for this for now, but this allows simplification.
						external_subresource_desc.op = deferred_request_command::blit_image_static;
					}
				}
			}

			// Returns true if at least threshold% is covered in pixels
			bool atlas_covers_target_area(int threshold) const
			{
				const int target_area = (external_subresource_desc.width * external_subresource_desc.height * external_subresource_desc.depth * threshold) / 100;
				int covered_area = 0;
				areai bbox{smax, smax, 0, 0};

				for (const auto& section : external_subresource_desc.sections_to_copy)
				{
					if (section.level != 0)
					{
						// Ignore other slices other than mip0
						continue;
					}

					// Calculate virtual Y coordinate
					const auto dst_y = (section.dst_z * external_subresource_desc.height) + section.dst_y;

					// Add this slice's dimensions to the total
					covered_area += section.dst_w * section.dst_h;

					// Extend the covered bbox
					bbox.x1 = std::min<int>(section.dst_x, bbox.x1);
					bbox.x2 = std::max<int>(section.dst_x + section.dst_w, bbox.x2);
					bbox.y1 = std::min<int>(dst_y, bbox.y1);
					bbox.y2 = std::max<int>(dst_y + section.dst_h, bbox.y2);
				}

				if (covered_area < target_area)
				{
					return false;
				}

				if (const auto bounds_area = bbox.width() * bbox.height();
					bounds_area < target_area)
				{
					return false;
				}

				return true;
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

			/**
			 * Returns a boolean true/false if the descriptor is expired
			 * Optionally returns a second variable that contains the surface reference.
			 * The surface reference can be used to insert a texture barrier or inject a deferred resource
			 */
			template <typename surface_store_type, typename surface_type = typename surface_store_type::surface_type>
			std::pair<bool, surface_type> is_expired(surface_store_type& surface_cache)
			{
				if (upload_context != rsx::texture_upload_context::framebuffer_storage)
				{
					return {};
				}

				// Expired, but may still be valid. Check if the texture is still accessible
				auto ref_image = image_handle ? image_handle->image() : external_subresource_desc.external_handle;
				surface_type surface = dynamic_cast<surface_type>(ref_image);

				// Try and grab a cache reference in case of MSAA resolve target or compositing op
				if (!surface)
				{
					if (!(surface = surface_cache.get_surface_at(ref_address)))
					{
						// Surface cache does not have our image. Two possibilities:
						// 1. It was never a real RTT, just some op like dynamic/static-copy/composite request. image_ref is null in such cases.
						// 2. It was real but probably deleted some time ago and we have a bogus pointer. Discard it in this case.
						if (!ref_image)
						{
							// Compositing op. Just ignore expiry for now
							return {};
						}

						// We have a real image but surface cache says it doesn't exist any more. Force a reupload.
						// Normally the global samplers dirty flag should have been set to invalidate all references.
						ensure(external_subresource_desc.op == deferred_request_command::nop);
						rsx_log.warning("Renderer is holding a stale reference to a surface that no longer exists!");
						return { true, nullptr };
					}
				}

				ensure(surface);
				if (!ref_image || surface->get_surface(rsx::surface_access::gpu_reference) == ref_image)
				{
					// Same image, so configuration did not change.
					if (surface_cache.cache_tag <= surface_cache_tag &&
						surface->last_use_tag <= surface_cache_tag)
					{
						external_subresource_desc.do_not_cache = false;
						return {};
					}

					// Image was written to since last bind. Insert texture barrier.
					surface_cache_tag = surface->last_use_tag;
					is_cyclic_reference = surface_cache.address_is_bound(ref_address);
					external_subresource_desc.do_not_cache = is_cyclic_reference;

					switch (external_subresource_desc.op)
					{
					case deferred_request_command::copy_image_dynamic:
					case deferred_request_command::copy_image_static:
						external_subresource_desc.op = (is_cyclic_reference) ? deferred_request_command::copy_image_dynamic : deferred_request_command::copy_image_static;
						[[ fallthrough ]];
					default:
						return { false, surface };
					}
				}

				// Reupload
				return { true, nullptr };
			}
		};


	protected:

		/**
		 * Variable declarations
		 */

		shared_mutex m_cache_mutex;
		ranged_storage m_storage;
		std::unordered_multimap<u32, std::pair<deferred_subresource, image_view_type>> m_temporary_subresource_cache;
		std::vector<image_view_type> m_uncached_subresources;
		predictor_type m_predictor;

		atomic_t<u64> m_cache_update_tag = {0};

		address_range32 read_only_range;
		address_range32 no_access_range;

		//Map of messages to only emit once
		std::unordered_set<std::string> m_once_only_messages_set;

		//Set when a shader read-only texture data suddenly becomes contested, usually by fbo memory
		bool read_only_tex_invalidate = false;

		//Store of all objects in a flush_always state. A lazy readback is attempted every draw call
		std::unordered_map<address_range32, section_storage_type*> m_flush_always_cache;
		u64 m_flush_always_update_timestamp = 0;

		//Memory usage
		const u32 m_max_zombie_objects = 64; //Limit on how many texture objects to keep around for reuse after they are invalidated

		//Other statistics
		atomic_t<u32> m_flushes_this_frame = { 0 };
		atomic_t<u32> m_misses_this_frame  = { 0 };
		atomic_t<u32> m_speculations_this_frame = { 0 };
		atomic_t<u32> m_unavoidable_hard_faults_this_frame = { 0 };
		atomic_t<u32> m_texture_upload_calls_this_frame = { 0 };
		atomic_t<u32> m_texture_upload_misses_this_frame = { 0 };
		atomic_t<u32> m_texture_copies_ellided_this_frame = { 0 };
		static const u32 m_predict_max_flushes_per_frame = 50; // Above this number the predictions are disabled

		// Invalidation
		static const bool invalidation_ignore_unsynchronized = true; // If true, unsynchronized sections don't get forcefully flushed unless they overlap the fault range
		static const bool invalidation_keep_ro_during_read = true; // If true, RO sections are not invalidated during read faults




		/**
		 * Virtual Methods
		 */
		virtual image_view_type create_temporary_subresource_view(commandbuffer_type&, image_resource_type* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type create_temporary_subresource_view(commandbuffer_type&, image_storage_type* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector) = 0;
		virtual void release_temporary_subresource(image_view_type rsc) = 0;
		virtual section_storage_type* create_new_texture(commandbuffer_type&, const address_range32 &rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u32 pitch, u32 gcm_format,
			rsx::texture_upload_context context, rsx::texture_dimension_extended type, bool swizzled, component_order swizzle_flags, rsx::flags32_t flags) = 0;
		virtual section_storage_type* upload_image_from_cpu(commandbuffer_type&, const address_range32 &rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u32 pitch, u32 gcm_format, texture_upload_context context,
			const std::vector<rsx::subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, bool swizzled) = 0;
		virtual section_storage_type* create_nul_section(commandbuffer_type&, const address_range32 &rsx_range, const image_section_attributes_t& attrs, const GCM_tile_reference& tile, bool memory_load) = 0;
		virtual void set_component_order(section_storage_type& section, u32 gcm_format, component_order expected) = 0;
		virtual void insert_texture_barrier(commandbuffer_type&, image_storage_type* tex, bool strong_ordering = true) = 0;
		virtual image_view_type generate_cubemap_from_images(commandbuffer_type&, u32 gcm_format, u16 size, const std::vector<copy_region_descriptor>& sources, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type generate_3d_from_2d_images(commandbuffer_type&, u32 gcm_format, u16 width, u16 height, u16 depth, const std::vector<copy_region_descriptor>& sources, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type generate_atlas_from_images(commandbuffer_type&, u32 gcm_format, u16 width, u16 height, const std::vector<copy_region_descriptor>& sections_to_copy, const texture_channel_remap_t& remap_vector) = 0;
		virtual image_view_type generate_2d_mipmaps_from_images(commandbuffer_type&, u32 gcm_format, u16 width, u16 height, const std::vector<copy_region_descriptor>& sections_to_copy, const texture_channel_remap_t& remap_vector) = 0;
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

		template <typename CharT, usz N, typename... Args>
		void emit_once(bool error, const CharT(&fmt)[N], const Args&... params)
		{
			const auto result = m_once_only_messages_set.emplace(fmt::format(fmt, params...));
			if (!result.second)
				return;

			if (error)
				rsx_log.error("%s", *result.first);
			else
				rsx_log.warning("%s", *result.first);
		}

		template <typename CharT, usz N, typename... Args>
		void err_once(const CharT(&fmt)[N], const Args&... params)
		{
			emit_once(true, fmt, params...);
		}

		template <typename CharT, usz N, typename... Args>
		void warn_once(const CharT(&fmt)[N], const Args&... params)
		{
			emit_once(false, fmt, params...);
		}

		/**
		 * Internal implementation methods and helpers
		 */

		inline bool region_intersects_cache(const address_range32 &test_range, bool is_writing)
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
		void flush_set(commandbuffer_type& cmd, thrashed_set& data, std::function<void()> on_data_transfer_completed, Args&&... extras)
		{
			AUDIT(!data.flushed);

			if (data.sections_to_flush.size() > 1)
			{
				// Sort with oldest data first
				// Ensures that new data tramples older data
				std::sort(data.sections_to_flush.begin(), data.sections_to_flush.end(), FN(x->last_write_tag < y->last_write_tag));
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

			if (on_data_transfer_completed)
			{
				on_data_transfer_completed();
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
						if (other->overlaps(*surface, section_bounds::confirmed_range))
						{
							// This should never happen. It will raise exceptions later due to a dirty region being locked
							rsx_log.error("Excluded region overlaps with flushed surface!");
							other->set_dirty(true);
						}
					}
					else if(surface->last_write_tag > other->last_write_tag)
					{
						other->add_flush_exclusion(surface->get_confirmed_range());
					}
				}
			}

			// Resync any exclusions that do not require flushing
			std::vector<section_storage_type*> surfaces_to_inherit;
			for (auto& surface : data.sections_to_exclude)
			{
				if (surface->get_context() != texture_upload_context::framebuffer_storage)
				{
					continue;
				}

				// Check for any 'newer' flushed overlaps. Memory must be re-acquired to avoid holding stale contents
				// Note that the surface cache inheritance will minimize the impact
				surfaces_to_inherit.clear();

				for (auto& flushed_surface : data.sections_to_flush)
				{
					if (flushed_surface->get_context() != texture_upload_context::framebuffer_storage ||
						flushed_surface->last_write_tag <= surface->last_write_tag ||
						!flushed_surface->get_confirmed_range().overlaps(surface->get_confirmed_range()))
					{
						continue;
					}

					surfaces_to_inherit.push_back(flushed_surface);
				}

				surface->sync_surface_memory(surfaces_to_inherit);
			}

			data.flushed = true;
			update_cache_tag();
		}


		// Merges the protected ranges of the sections in "sections" into "result"
		void merge_protected_ranges(address_range_vector32 &result, const std::vector<section_storage_type*> &sections)
		{
			result.reserve(result.size() + sections.size());

			// Copy ranges to result, merging them if possible
			for (const auto &section : sections)
			{
				ensure(section->is_locked(true));
				const auto &new_range = section->get_locked_range();
				AUDIT(new_range.is_page_range());

				result.merge(new_range);
			}
		}

		// NOTE: It is *very* important that data contains exclusions for *all* sections that overlap sections_to_unprotect/flush
		//       Otherwise the page protections will end up incorrect and things will break!
		void unprotect_set(thrashed_set& data)
		{
			auto protect_ranges = [](address_range_vector32& _set, utils::protection _prot)
			{
				//u32 count = 0;
				for (auto &range : _set)
				{
					if (range.valid())
					{
						rsx::memory_protect(range, _prot);
						//count++;
					}
				}
				//rsx_log.error("Set protection of %d blocks to 0x%x", count, static_cast<u32>(prot));
			};

			auto discard_set = [](std::vector<section_storage_type*>& _set)
			{
				for (auto* section : _set)
				{
					ensure(section->is_flushed() || section->is_dirty());

					section->discard(/*set_dirty*/ false);
				}
			};

			// Sanity checks
			AUDIT(data.fault_range.is_page_range());
			AUDIT(data.invalidate_range.is_page_range());
			AUDIT(data.is_flushed());

			// Merge ranges to unprotect
			address_range_vector32 ranges_to_unprotect;
			address_range_vector32 ranges_to_protect_ro;
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
					ensure(excluded->is_locked(true));
					address_range32 exclusion_range = excluded->get_locked_range();

					// We need to make sure that the exclusion range is *inside* invalidate range
					exclusion_range.intersect(data.invalidate_range);

					// Sanity checks
					AUDIT(exclusion_range.is_page_range());
					AUDIT((data.cause.is_read() && !excluded->is_flushable()) || data.cause.skip_fbos() || !exclusion_range.overlaps(data.fault_range));

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
						fmt::throw_exception("Unreachable");
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
		atomic_t<u64> m_last_section_cache_tag = 0;
		intersecting_set get_intersecting_set(const address_range32 &fault_range)
		{
			AUDIT(fault_range.is_page_range());

			const u64 cache_tag = ++m_last_section_cache_tag;

			intersecting_set result = {};
			address_range32 &invalidate_range = result.invalidate_range;
			invalidate_range = fault_range; // Sections fully inside this range will be invalidated, others will be deemed false positives

			// Loop through cache and find pages that overlap the invalidate_range
			u32 last_dirty_block = -1;
			bool repeat_loop = false;

			auto It = m_storage.range_begin(invalidate_range, locked_range, true); // will iterate through locked sections only
			while (It != m_storage.range_end())
			{
				const u32 base = It.get_block().get_start();

				// On the last loop, we stop once we're done with the last dirty block
				if (!repeat_loop && base > last_dirty_block) // note: blocks are iterated in order from lowest to highest base address
					break;

				auto &tex = *It;

				AUDIT(tex.is_locked()); // we should be iterating locked sections only, but just to make sure...
				AUDIT(tex.cache_tag != cache_tag || last_dirty_block != umax); // cache tag should not match during the first loop

				if (tex.cache_tag != cache_tag) //flushable sections can be 'clean' but unlocked. TODO: Handle this better
				{
					const rsx::section_bounds bounds = tex.get_overlap_test_bounds();

					if (locked_range == bounds || tex.overlaps(invalidate_range, bounds))
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
					It = m_storage.range_begin(invalidate_range, locked_range, true);
					repeat_loop = false;
				}
			}

			AUDIT(result.invalidate_range.is_page_range());

#ifdef TEXTURE_CACHE_DEBUG
			// naive check that sections are not duplicated in the results
			for (auto &section1 : result.sections)
			{
				usz count = 0;
				for (auto &section2 : result.sections)
				{
					if (section1 == section2) count++;
				}
				ensure(count == 1);
			}
#endif //TEXTURE_CACHE_DEBUG

			return result;
		}


		//Invalidate range base implementation
		template <typename ...Args>
		thrashed_set invalidate_range_impl_base(
			commandbuffer_type& cmd,
			const address_range32 &fault_range_in,
			invalidation_cause cause,
			std::function<void()> on_data_transfer_completed = {},
			Args&&... extras)
		{
#ifdef TEXTURE_CACHE_DEBUG
			// Check that the cache has the correct protections
			tex_cache_checker.verify();
#endif // TEXTURE_CACHE_DEBUG

			AUDIT(cause.valid());
			AUDIT(fault_range_in.valid());
			address_range32 fault_range = fault_range_in.to_page_range();

			intersecting_set trampled_set = get_intersecting_set(fault_range);

			thrashed_set result = {};
			result.cause = cause;
			result.fault_range = fault_range;
			result.invalidate_range = trampled_set.invalidate_range;

			if (cause.use_strict_data_bounds())
			{
				// Drop all sections outside the actual target range. This is useful when we simply need to tag that we'll be updating some memory content on the CPU
				// But we don't really care about writeback or invalidation of anything outside the update range.
				if (trampled_set.sections.erase_if(FN(!x->overlaps(fault_range_in, section_bounds::full_range))))
				{
					trampled_set.has_flushables = trampled_set.sections.any(FN(x->is_flushable()));
				}
			}

			if (trampled_set.sections.empty())
			{
				return {};
			}

			// Fast code-path for keeping the fault range protection when not flushing anything
			if (cause.keep_fault_range_protection() && cause.skip_flush() && !trampled_set.sections.empty())
			{
				ensure(cause != invalidation_cause::committed_as_fbo);

				// We discard all sections fully inside fault_range
				for (auto &obj : trampled_set.sections)
				{
					auto &tex = *obj;
					if (tex.overlaps(fault_range, section_bounds::locked_range))
					{
						if (cause == invalidation_cause::superseded_by_fbo &&
							tex.is_flushable() &&
							tex.get_section_base() != fault_range_in.start)
						{
							// HACK: When being superseded by an fbo, we preserve overlapped flushables unless the start addresses match
							continue;
						}
						else if (tex.inside(fault_range, section_bounds::locked_range))
						{
							// Discard - this section won't be needed any more
							tex.discard(/* set_dirty */ true);
							result.invalidate_samplers = true;
							result.num_discarded++;
						}
						else if (g_cfg.video.strict_texture_flushing && tex.is_flushable())
						{
							tex.add_flush_exclusion(fault_range);
						}
						else
						{
							tex.set_dirty(true);
							result.invalidate_samplers = true;
						}

						if (tex.is_dirty() && tex.get_context() == rsx::texture_upload_context::framebuffer_storage)
						{
							// Make sure the region is not going to get immediately reprotected
							m_flush_always_cache.erase(tex.get_section_range());
						}
					}
				}

#ifdef TEXTURE_CACHE_DEBUG
				// Notify the checker that fault_range got discarded
				tex_cache_checker.discard(fault_range);
#endif

				// If invalidate_range is fault_range, we can stop now
				const address_range32 invalidate_range = trampled_set.invalidate_range;
				if (invalidate_range == fault_range)
				{
					result.violation_handled = true;
					result.invalidate_samplers = true;
#ifdef TEXTURE_CACHE_DEBUG
					// Post-check the result
					result.check_post_sanity();
#endif
					return result;
				}

				AUDIT(fault_range.inside(invalidate_range));
			}


			// Decide which sections to flush, unprotect, and exclude
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
					// HACK: When being superseded by an fbo, we preserve other overlapped flushables unless the start addresses match
					// If region is committed as fbo, all non-flushable data is removed but all flushables in the region must be preserved if possible
					(overlaps_fault_range && tex.is_flushable() && cause.skip_fbos() && tex.get_section_base() != fault_range_in.start)
					)
				{
					// False positive
					if (tex.is_locked(true))
					{
						// Do not exclude hashed pages from unprotect! They will cause protection holes
						result.sections_to_exclude.push_back(&tex);
					}
					result.num_excluded++;
					continue;
				}

				if (tex.is_flushable())
				{
					// Write if and only if no one else has trashed section memory already
					// TODO: Proper section management should prevent this from happening
					// TODO: Blit engine section merge support and/or partial texture memory buffering
					if (tex.is_dirty())
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

					if (tex.is_locked(true))
					{
						result.sections_to_unprotect.push_back(&tex);
					}
					else
					{
						// No need to waste resources on hashed section, just discard immediately
						tex.discard(true);
						result.invalidate_samplers = true;
						result.num_discarded++;
					}

					continue;
				}

				fmt::throw_exception("Unreachable");
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
				result.cache_tag = m_cache_update_tag.load();
				return result;
			}
			else if (has_flushables || has_unprotectables)
			{
				AUDIT(!has_flushables || !cause.deferred_flush());

				// We have something to flush and are allowed to flush now
				// or there is nothing to flush but we have something to unprotect
				if (has_flushables && !cause.skip_flush())
				{
					flush_set(cmd, result, on_data_transfer_completed, std::forward<Args>(extras)...);
				}

				unprotect_set(result);

				// Everything has been handled
				result.clear_sections();
			}
			else
			{
				// This is a read and all overlapping sections were RO and were excluded (except for cause == superseded_by_fbo)
				// Can also happen when we have hash strat in use, since we "unlock" sections by just discarding
				AUDIT(cause.skip_fbos() || (cause.is_read() && result.num_excluded > 0) || result.num_discarded > 0);

				// We did not handle this violation
				result.clear_sections();
				result.violation_handled = false;
			}

			result.invalidate_samplers |= result.violation_handled;

#ifdef TEXTURE_CACHE_DEBUG
			// Post-check the result
			result.check_post_sanity();
#endif // TEXTURE_CACHE_DEBUG

			return result;
		}

	public:

		texture_cache() : m_storage(this), m_predictor(this) {}
		~texture_cache() = default;

		void clear()
		{
			// Release objects used for frame data
			on_frame_end();

			// Nuke the permanent storage pool
			m_storage.clear();
			m_predictor.clear();
		}

		virtual void on_frame_end()
		{
			// Must manually release each cached entry
			for (auto& entry : m_temporary_subresource_cache)
			{
				release_temporary_subresource(entry.second.second);
			}

			m_temporary_subresource_cache.clear();
			m_predictor.on_frame_end();
			reset_frame_statistics();
		}

		template <bool check_unlocked = false>
		std::vector<section_storage_type*> find_texture_from_range(const address_range32 &test_range, u32 required_pitch = 0, u32 context_mask = 0xFF)
		{
			std::vector<section_storage_type*> results;

			for (auto It = m_storage.range_begin(test_range, full_range, check_unlocked); It != m_storage.range_end(); It++)
			{
				auto &tex = *It;

				if (!tex.is_dirty() && (context_mask & static_cast<u32>(tex.get_context())))
				{
					if (required_pitch && !rsx::pitch_compatible<false>(&tex, required_pitch, -1))
					{
						continue;
					}

					if (!tex.sync_protection())
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
					{
						continue;
					}
				}

				if (!tex.is_dirty() &&
					tex.matches(rsx_address, format, width, height, depth, mipmaps) &&
					tex.sync_protection())
				{
					return &tex;
				}
			}

			return nullptr;
		}

		section_storage_type* find_cached_texture(const address_range32 &range, const image_section_attributes_t& attr, bool create_if_not_found, bool confirm_dimensions, bool allow_dirty)
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
					// We must validate
					tex.sync_protection();

					if (allow_dirty || !tex.is_dirty())
					{
						if (!confirm_dimensions || tex.matches(attr.gcm_format, attr.width, attr.height, attr.depth, attr.mipmaps))
						{
#ifndef TEXTURE_CACHE_DEBUG
							return &tex;
#else
							ensure(res == nullptr);
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
						// By grabbing a ref to a matching entry, duplicates are avoided
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
				rsx_log.warning("Cached object for address 0x%X was found, but it does not match stored parameters (width=%d vs %d; height=%d vs %d; depth=%d vs %d; mipmaps=%d vs %d)",
					range.start, attr.width, tex.get_width(), attr.height, tex.get_height(), attr.depth, tex.get_depth(), attr.mipmaps, tex.get_mipmaps());
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

		section_storage_type* find_flushable_section(const address_range32 &memory_range)
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
		void lock_memory_region(commandbuffer_type& cmd, image_storage_type* image, const address_range32 &rsx_range, bool is_active_surface, u16 width, u16 height, u32 pitch, Args&&... extras)
		{
			AUDIT(g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer); // this method is only called when either WCB or WDB are enabled

			std::lock_guard lock(m_cache_mutex);

			// Find a cached section to use
			image_section_attributes_t search_desc = { .gcm_format = RSX_GCM_FORMAT_IGNORED, .width = width, .height = height };
			section_storage_type& region = *find_cached_texture(rsx_range, search_desc, true, true, false);

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
				ensure(region.matches(rsx_range));
				ensure(region.get_context() == texture_upload_context::framebuffer_storage);
				ensure(region.get_image_type() == rsx::texture_dimension_extended::texture_dimension_2d);
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
		void commit_framebuffer_memory_region(commandbuffer_type& cmd, const address_range32 &rsx_range, Args&&... extras)
		{
			AUDIT(!g_cfg.video.write_color_buffers || !g_cfg.video.write_depth_buffer);

			if (!region_intersects_cache(rsx_range, true))
				return;

			std::lock_guard lock(m_cache_mutex);
			invalidate_range_impl_base(cmd, rsx_range, invalidation_cause::committed_as_fbo, {}, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		void discard_framebuffer_memory_region(commandbuffer_type& /*cmd*/, const address_range32& rsx_range, Args&&... /*extras*/)
		{
			if (g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer)
			{
				auto* region_ptr = find_cached_texture(rsx_range, { .gcm_format = RSX_GCM_FORMAT_IGNORED }, false, false, false);
				if (region_ptr && region_ptr->is_locked() && region_ptr->get_context() == texture_upload_context::framebuffer_storage)
				{
					ensure(region_ptr->get_protection() == utils::protection::no);
					region_ptr->discard(false);
				}
			}
		}

		void set_memory_read_flags(const address_range32 &memory_range, memory_read_flags flags)
		{
			std::lock_guard lock(m_cache_mutex);

			auto* region_ptr = find_cached_texture(memory_range, { .gcm_format = RSX_GCM_FORMAT_IGNORED }, false, false, true);
			if (region_ptr == nullptr)
			{
				AUDIT(m_flush_always_cache.find(memory_range) == m_flush_always_cache.end());
				rsx_log.error("set_memory_flags(0x%x, 0x%x, %d): region_ptr == nullptr", memory_range.start, memory_range.end, static_cast<u32>(flags));
				return;
			}

			if (region_ptr->is_dirty())
			{
				// Previously invalidated
				return;
			}

			auto& region = *region_ptr;

			if (!region.exists() || region.is_dirty() || region.get_context() != texture_upload_context::framebuffer_storage)
			{
#ifdef TEXTURE_CACHE_DEBUG
				if (!region.is_dirty())
				{
					if (flags == memory_read_flags::flush_once)
						ensure(m_flush_always_cache.find(memory_range) == m_flush_always_cache.end());
					else
						ensure(m_flush_always_cache[memory_range] == &region);
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
				ensure(m_flush_always_cache[memory_range] == &section);
			else
				ensure(m_flush_always_cache.find(memory_range) == m_flush_always_cache.end());
#endif
			update_flush_always_cache(section, flags == memory_read_flags::flush_always);
		}

	private:
		inline void update_flush_always_cache(section_storage_type &section, bool add)
		{
			const address_range32& range = section.get_section_range();
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
		thrashed_set invalidate_address(
			commandbuffer_type& cmd,
			u32 address,
			invalidation_cause cause,
			std::function<void()> on_data_transfer_completed = {},
			Args&&... extras)
		{
			// Test before trying to acquire the lock
			const auto range = page_for(address);
			if (!region_intersects_cache(range, !cause.is_read()))
				return{};

			std::lock_guard lock(m_cache_mutex);
			return invalidate_range_impl_base(cmd, range, cause, on_data_transfer_completed, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		thrashed_set invalidate_range(
			commandbuffer_type& cmd,
			const address_range32 &range,
			invalidation_cause cause,
			std::function<void()> on_data_transfer_completed = {},
			Args&&... extras)
		{
			// Test before trying to acquire the lock
			if (!region_intersects_cache(range, !cause.is_read()))
				return {};

			std::lock_guard lock(m_cache_mutex);
			return invalidate_range_impl_base(cmd, range, cause, on_data_transfer_completed, std::forward<Args>(extras)...);
		}

		template <typename ...Args>
		bool flush_all(commandbuffer_type& cmd, thrashed_set& data, std::function<void()> on_data_transfer_completed = {}, Args&&... extras)
		{
			std::lock_guard lock(m_cache_mutex);

			AUDIT(data.cause.deferred_flush());
			AUDIT(!data.flushed);

			if (m_cache_update_tag.load() == data.cache_tag)
			{
				//1. Write memory to cpu side
				flush_set(cmd, data, on_data_transfer_completed, std::forward<Args>(extras)...);

				//2. Release all obsolete sections
				unprotect_set(data);
			}
			else
			{
				// The cache contents have changed between the two readings. This means the data held is useless
				invalidate_range_impl_base(cmd, data.fault_range, data.cause.undefer(), on_data_transfer_completed, std::forward<Args>(extras)...);
			}

			return true;
		}

		template <typename ...Args>
		bool flush_if_cache_miss_likely(commandbuffer_type& cmd, const address_range32 &range, Args&&... extras)
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

		virtual bool handle_memory_pressure(problem_severity severity)
		{
			if (m_storage.m_unreleased_texture_objects)
			{
				m_storage.purge_unreleased_sections();
				return true;
			}

			if (severity >= problem_severity::severe)
			{
				// Things are bad, previous check should have released 'unreleased' pool
				return m_storage.purge_unlocked_sections();
			}

			return false;
		}

		void trim_sections()
		{
			std::lock_guard lock(m_cache_mutex);

			m_storage.trim_sections();
		}

		bool evict_unused(const std::set<u32>& exclusion_list)
		{
			// Some sanity checks. Do not evict if the cache is currently in use.
			ensure(rsx::get_current_renderer()->is_current_thread());
			std::unique_lock lock(m_cache_mutex, std::defer_lock);

			if (!lock.try_lock())
			{
				rsx_log.warning("Unable to evict the texture cache because we're faulting from within in the texture cache!");
				return false;
			}

			rsx_log.warning("[PERFORMANCE WARNING] Texture cache is running eviction routine. This will affect performance.");

			thrashed_set evicted_set;
			const u32 type_to_evict = rsx::texture_upload_context::shader_read | rsx::texture_upload_context::blit_engine_src;
			for (auto It = m_storage.begin(); It != m_storage.end(); ++It)
			{
				auto& block = *It;
				if (block.empty())
				{
					continue;
				}

				for (auto& region : block)
				{
					if (region.is_dirty() || !(region.get_context() & type_to_evict))
					{
						continue;
					}

					ensure(region.is_locked());

					const u32 this_address = region.get_section_base();
					if (exclusion_list.contains(this_address))
					{
						continue;
					}

					evicted_set.violation_handled = true;
					region.set_dirty(true);

					if (region.is_locked(true))
					{
						evicted_set.sections_to_unprotect.push_back(&region);
					}
					else
					{
						region.discard(true);
					}
				}
			}

			unprotect_set(evicted_set);
			return evicted_set.violation_handled;
		}

		image_view_type create_temporary_subresource(commandbuffer_type &cmd, deferred_subresource& desc)
		{
			if (!desc.do_not_cache) [[likely]]
			{
				const auto found = m_temporary_subresource_cache.equal_range(desc.address);
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

			std::lock_guard lock(m_cache_mutex);
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
						.src = desc.external_handle,
						.xform = surface_transform::coordinate_transform,
						.level = 0,
						.src_x = 0,
						.src_y = static_cast<u16>(desc.slice_h * n),
						.dst_x = 0,
						.dst_y = 0,
						.dst_z = n,
						.src_w = desc.width,
						.src_h = desc.height,
						.dst_w = desc.width,
						.dst_h = desc.height
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
						.src = desc.external_handle,
						.xform = surface_transform::coordinate_transform,
						.level = 0,
						.src_x = 0,
						.src_y = static_cast<u16>(desc.slice_h * n),
						.dst_x = 0,
						.dst_y = 0,
						.dst_z = n,
						.src_w = desc.width,
						.src_h = desc.height,
						.dst_w = desc.width,
						.dst_h = desc.height
					};
				}

				result = generate_3d_from_2d_images(cmd, desc.gcm_format, desc.width, desc.height, desc.depth, sections, desc.remap);
				break;
			}
			case deferred_request_command::atlas_gather:
			case deferred_request_command::blit_image_static:
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
			case deferred_request_command::mipmap_gather:
			{
				result = generate_2d_mipmaps_from_images(cmd, desc.gcm_format, desc.width, desc.height, desc.sections_to_copy, desc.remap);
				break;
			}
			default:
			{
				//Throw
				fmt::throw_exception("Invalid deferred command op 0x%X", static_cast<u32>(desc.op));
			}
			}

			if (result) [[likely]]
			{
				if (!desc.do_not_cache) [[likely]]
				{
					m_temporary_subresource_cache.insert({ desc.address,{ desc, result } });
				}
				else
				{
					m_uncached_subresources.push_back(result);
				}
			}

			return result;
		}

		void release_uncached_temporary_subresources()
		{
			for (auto& view : m_uncached_subresources)
			{
				release_temporary_subresource(view);
			}

			m_uncached_subresources.clear();
		}

		void notify_surface_changed(const utils::address_range32& range)
		{
			for (auto It = m_temporary_subresource_cache.begin(); It != m_temporary_subresource_cache.end();)
			{
				const auto& desc = It->second.first;
				if (range.overlaps(desc.cache_range))
				{
					release_temporary_subresource(It->second.second);
					It = m_temporary_subresource_cache.erase(It);
				}
				else
				{
					++It;
				}
			}
		}

		template <typename SurfaceStoreType, typename... Args>
		sampled_image_descriptor fast_texture_search(
			commandbuffer_type& cmd,
			const image_section_attributes_t& attr,
			const size3f& scale,
			const texture_channel_remap_t& remap,
			const texture_cache_search_options& options,
			const utils::address_range32& memory_range,
			rsx::texture_dimension_extended extended_dimension,
			SurfaceStoreType& m_rtts, Args&&... /*extras*/)
		{
			if (options.is_compressed_format) [[likely]]
			{
				// Most mesh textures are stored as compressed to make the most of the limited memory
				if (auto cached_texture = find_texture_from_dimensions(attr.address, attr.gcm_format, attr.width, attr.height, attr.depth))
				{
					return{ cached_texture->get_view(remap), cached_texture->get_context(), cached_texture->get_format_class(), scale, cached_texture->get_image_type() };
				}
			}
			else
			{
				// Fast lookup for cyclic reference
				if (m_rtts.address_is_bound(attr.address)) [[unlikely]]
				{
					if (auto texptr = m_rtts.get_surface_at(attr.address);
						helpers::check_framebuffer_resource(texptr, attr, extended_dimension))
					{
						const bool force_convert = !render_target_format_is_compatible(texptr, attr.gcm_format);

						auto result = helpers::process_framebuffer_resource_fast<sampled_image_descriptor>(
							cmd, texptr, attr, scale, extended_dimension, remap, true, force_convert);

						if (!options.skip_texture_barriers && result.is_cyclic_reference)
						{
							// A texture barrier is only necessary when the rendertarget is going to be bound as a shader input.
							// If a temporary copy is to be made, this should not be invoked
							insert_texture_barrier(cmd, texptr);
						}

						return result;
					}
				}

				std::vector<typename SurfaceStoreType::surface_overlap_info> overlapping_fbos;
				std::vector<section_storage_type*> overlapping_locals;

				auto fast_fbo_check = [&]() -> sampled_image_descriptor
				{
					const auto& last = overlapping_fbos.back();
					if (last.src_area.x == 0 && last.src_area.y == 0 && !last.is_clipped)
					{
						const bool force_convert = !render_target_format_is_compatible(last.surface, attr.gcm_format);

						return helpers::process_framebuffer_resource_fast<sampled_image_descriptor>(
							cmd, last.surface, attr, scale, extended_dimension, remap, false, force_convert);
					}

					return {};
				};

				// Check surface cache early if the option is enabled
				if (options.prefer_surface_cache)
				{
					const u16 block_h = (attr.depth * attr.slice_h);
					overlapping_fbos = m_rtts.get_merged_texture_memory_region(cmd, attr.address, attr.width, block_h, attr.pitch, attr.bpp, rsx::surface_access::shader_read);

					if (!overlapping_fbos.empty())
					{
						if (auto result = fast_fbo_check(); result.validate())
						{
							return result;
						}

						if (options.skip_texture_merge)
						{
							overlapping_fbos.clear();
						}
					}
				}

				// Check shader_read storage. In a given scene, reads from local memory far outnumber reads from the surface cache
				const u32 lookup_mask = rsx::texture_upload_context::shader_read | rsx::texture_upload_context::blit_engine_dst | rsx::texture_upload_context::blit_engine_src;
				overlapping_locals = find_texture_from_range<true>(memory_range, attr.height > 1 ? attr.pitch : 0, lookup_mask & options.lookup_mask);

				// Search for exact match if possible
				for (auto& cached_texture : overlapping_locals)
				{
					if (cached_texture->matches(attr.address, attr.gcm_format, attr.width, attr.height, attr.depth, 0))
					{
#ifdef TEXTURE_CACHE_DEBUG
						if (!memory_range.inside(cached_texture->get_confirmed_range()))
						{
							// TODO. This is easily possible for blit_dst textures if the blit is incomplete in Y
							// The possibility that a texture will be split into parts on the CPU like this is very rare
							continue;
						}
#endif
						if (attr.swizzled != cached_texture->is_swizzled())
						{
							// We can have the correct data in cached_texture but it needs decoding before it can be sampled.
							// Usually a sign of a game bug where the developer forgot to mark the texture correctly the first time we see it.
							// TODO: This section should execute under an exclusive lock, but we're not actually modifying any object references, only flags
							rsx_log.warning("A texture was found in cache for address 0x%x, but swizzle flag does not match", attr.address);
							cached_texture->unprotect();
							cached_texture->set_dirty(true);
							break;
						}

						return{ cached_texture->get_view(remap), cached_texture->get_context(), cached_texture->get_format_class(), scale, cached_texture->get_image_type() };
					}
				}

				if (!overlapping_locals.empty())
				{
					// Remove everything that is not a transfer target
					overlapping_locals.erase
					(
						std::remove_if(overlapping_locals.begin(), overlapping_locals.end(), [](const auto& e)
						{
							return e->is_dirty() || (e->get_context() != rsx::texture_upload_context::blit_engine_dst);
						}),
						overlapping_locals.end()
					);
				}

				if (!options.prefer_surface_cache)
				{
					// Now check for surface cache hits
					const u16 block_h = (attr.depth * attr.slice_h);
					overlapping_fbos = m_rtts.get_merged_texture_memory_region(cmd, attr.address, attr.width, block_h, attr.pitch, attr.bpp, rsx::surface_access::shader_read);
				}

				if (!overlapping_fbos.empty() || !overlapping_locals.empty())
				{
					int _pool = -1;
					if (overlapping_locals.empty()) [[likely]]
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
						if (!options.prefer_surface_cache)
						{
							if (auto result = fast_fbo_check(); result.validate())
							{
								return result;
							}
						}
					}
					else if (extended_dimension <= rsx::texture_dimension_extended::texture_dimension_2d)
					{
						const auto last = overlapping_locals.back();
						const auto normalized_width = u16(last->get_width() * get_format_block_size_in_bytes(last->get_gcm_format())) / attr.bpp;

						if (last->get_section_base() == attr.address &&
							normalized_width >= attr.width && last->get_height() >= attr.height)
						{
							u32  gcm_format = attr.gcm_format;
							const bool gcm_format_is_depth = helpers::is_gcm_depth_format(attr.gcm_format);

							if (!gcm_format_is_depth && last->is_depth_texture())
							{
								// While the copy routines can perform a typeless cast, prefer to not cross the aspect barrier if possible
								gcm_format = helpers::get_compatible_depth_format(attr.gcm_format);
							}

							auto new_attr = attr;
							new_attr.gcm_format = gcm_format;

							if (last->get_gcm_format() == attr.gcm_format && attr.edge_clamped)
							{
								// Clipped view
								auto viewed_image = last->get_raw_texture();
								sampled_image_descriptor result = { viewed_image->get_view(remap), last->get_context(),
									viewed_image->format_class(), scale, extended_dimension, false, viewed_image->samples() };

								helpers::calculate_sample_clip_parameters(result, position2i(0, 0), size2i(attr.width, attr.height), size2i(normalized_width, last->get_height()));
								return result;
							}

							return { last->get_raw_texture(), deferred_request_command::copy_image_static, new_attr, {},
									last->get_context(), classify_format(gcm_format), scale, extended_dimension, remap };
						}
					}

					auto result = helpers::merge_cache_resources<sampled_image_descriptor>(
						cmd, overlapping_fbos, overlapping_locals, attr, scale, extended_dimension, remap, _pool);

					const bool is_simple_subresource_copy =
						(result.external_subresource_desc.op == deferred_request_command::copy_image_static) ||
						(result.external_subresource_desc.op == deferred_request_command::copy_image_dynamic) ||
						(result.external_subresource_desc.op == deferred_request_command::blit_image_static);

					if (attr.edge_clamped &&
						!g_cfg.video.strict_rendering_mode &&
						is_simple_subresource_copy &&
						render_target_format_is_compatible(result.external_subresource_desc.src0(), attr.gcm_format))
					{
						if (result.external_subresource_desc.op != deferred_request_command::blit_image_static) [[ likely ]]
						{
							helpers::convert_image_copy_to_clip_descriptor(
								result,
								position2i(result.external_subresource_desc.x, result.external_subresource_desc.y),
								size2i(result.external_subresource_desc.width, result.external_subresource_desc.height),
								size2i(result.external_subresource_desc.external_handle->width(), result.external_subresource_desc.external_handle->height()),
								remap, false);
						}
						else
						{
							helpers::convert_image_blit_to_clip_descriptor(
								result,
								remap,
								false);
						}

						if (!!result.ref_address && m_rtts.address_is_bound(result.ref_address))
						{
							result.is_cyclic_reference = true;

							auto texptr = ensure(m_rtts.get_surface_at(result.ref_address));
							insert_texture_barrier(cmd, texptr);
						}

						return result;
					}

					if (options.skip_texture_merge)
					{
						if (is_simple_subresource_copy)
						{
							return result;
						}

						return {};
					}

					if (const auto section_count = result.external_subresource_desc.sections_to_copy.size();
						section_count > 0)
					{
						bool result_is_valid;
						if (_pool == 0 && !g_cfg.video.write_color_buffers && !g_cfg.video.write_depth_buffer)
						{
							// HACK: Avoid WCB requirement for some games with wrongly declared sampler dimensions.
							// TODO: Some games may render a small region (e.g 1024x256x2) and sample a huge texture (e.g 1024x1024).
							// Seen in APF2k8 - this causes missing bits to be reuploaded from CPU which can cause WCB requirement.
							// Properly fix this by introducing partial data upload into the surface cache in such cases and making RCB/RDB
							// enabled by default. Blit engine already handles this correctly.
							result_is_valid = true;
						}
						else
						{
							result_is_valid = result.atlas_covers_target_area(section_count == 1 ? 99 : 90);
						}

						if (result_is_valid)
						{
							// Check for possible duplicates
							usz max_overdraw_ratio = u32{ umax };
							usz max_safe_sections = u32{ umax };

							switch (result.external_subresource_desc.op)
							{
							case deferred_request_command::atlas_gather:
								max_overdraw_ratio = 150;
								max_safe_sections = 8 + 2 * attr.mipmaps;
								break;
							case deferred_request_command::cubemap_gather:
								max_overdraw_ratio = 150;
								max_safe_sections = 6 * 2 * attr.mipmaps;
								break;
							case deferred_request_command::_3d_gather:
								// 3D gather can have very many input sections, try to keep section count low
								max_overdraw_ratio = 125;
								max_safe_sections = (attr.depth * attr.mipmaps * 110) / 100;
								break;
							default:
								break;
							}

							if (overlapping_fbos.size() > max_safe_sections)
							{
								// Are we really over-budget?
								u32 coverage_size = 0;
								for (const auto& section : overlapping_fbos)
								{
									const auto area = section.surface->get_native_pitch() * section.surface->template get_surface_height<rsx::surface_metrics::bytes>();
									coverage_size += area;
								}

								if (const auto coverage_ratio = (coverage_size * 100ull) / memory_range.length();
									coverage_ratio > max_overdraw_ratio)
								{
									rsx_log.warning("[Performance warning] Texture gather routine encountered too many objects! Operation=%d, Mipmaps=%d, Depth=%d, Sections=%zu, Ratio=%llu%",
										static_cast<int>(result.external_subresource_desc.op), attr.mipmaps, attr.depth, overlapping_fbos.size(), coverage_ratio);
									m_rtts.check_for_duplicates(overlapping_fbos);
								}
							}

							// Optionally disallow caching if resource is being written to as it is being read from
							for (const auto& section : overlapping_fbos)
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
					}
				}
			}

			return {};
		}

		template <typename surface_store_type, typename RsxTextureType>
		bool test_if_descriptor_expired(commandbuffer_type& cmd, surface_store_type& surface_cache, sampled_image_descriptor* descriptor, const RsxTextureType& tex)
		{
			auto result = descriptor->is_expired(surface_cache);
			if (result.second && descriptor->is_cyclic_reference)
			{
				/* NOTE: All cyclic descriptors updated via fast update must have a barrier check
				 * It is possible for the following sequence of events to break common-sense tests
				 * 1. Cyclic ref occurs normally in upload_texture
				 * 2. Surface is swappd out, but texture is not updated
				 * 3. Surface is swapped back in. Surface cache resets layout to optimal rasterization layout
				 * 4. During bind, the surface is converted to shader layout because it is not in GENERAL layout
				 */
				if (!texture_cache_helpers::force_strict_fbo_sampling(result.second->samples()))
				{
					insert_texture_barrier(cmd, result.second, false);
				}
				else if (descriptor->image_handle)
				{
					// Rebuild duplicate surface
					auto src = descriptor->image_handle->image();
					rsx::image_section_attributes_t attr;
					attr.address = descriptor->ref_address;
					attr.gcm_format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
					attr.width = src->width();
					attr.height = src->height();
					attr.depth = 1;
					attr.mipmaps = 1;
					attr.pitch = 0;  // Unused
					attr.slice_h = src->height();
					attr.bpp = get_format_block_size_in_bytes(attr.gcm_format);
					attr.swizzled = false;

					// Sanity checks
					const bool gcm_format_is_depth = helpers::is_gcm_depth_format(attr.gcm_format);
					const bool bound_surface_is_depth = surface_cache.m_bound_depth_stencil.first == attr.address;
					if (!gcm_format_is_depth && bound_surface_is_depth)
					{
						// While the copy routines can perform a typeless cast, prefer to not cross the aspect barrier if possible
						// This avoids messing with other solutions such as texture redirection as well
						attr.gcm_format = helpers::get_compatible_depth_format(attr.gcm_format);
					}

					descriptor->external_subresource_desc =
					{
						src,
						rsx::deferred_request_command::copy_image_dynamic,
						attr,
						{},
						rsx::default_remap_vector
					};

					descriptor->external_subresource_desc.do_not_cache = true;
					descriptor->image_handle = nullptr;
				}
				else
				{
					// Force reupload
					return true;
				}
			}

			return result.first;
		}

		template <typename RsxTextureType, typename surface_store_type, typename ...Args>
		sampled_image_descriptor upload_texture(commandbuffer_type& cmd, const RsxTextureType& tex, surface_store_type& m_rtts, Args&&... extras)
		{
			m_texture_upload_calls_this_frame++;

			image_section_attributes_t attributes{};
			texture_cache_search_options options{};
			attributes.address = rsx::get_address(tex.offset(), tex.location());
			attributes.gcm_format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			attributes.bpp = get_format_block_size_in_bytes(attributes.gcm_format);
			attributes.width = tex.width();
			attributes.height = tex.height();
			attributes.mipmaps = tex.get_exact_mipmap_count();
			attributes.swizzled = !(tex.format() & CELL_GCM_TEXTURE_LN);

			const bool is_unnormalized = !!(tex.format() & CELL_GCM_TEXTURE_UN);
			auto extended_dimension = tex.get_extended_texture_dimension();

			options.is_compressed_format = helpers::is_compressed_gcm_format(attributes.gcm_format);

			u32 tex_size = 0, required_surface_height = 1;
			u8 subsurface_count = 1;
			size3f scale{ 1.f, 1.f, 1.f };

			if (is_unnormalized)
			{
				switch (extended_dimension)
				{
				case rsx::texture_dimension_extended::texture_dimension_3d:
				case rsx::texture_dimension_extended::texture_dimension_cubemap:
					scale.depth /= attributes.depth;
					[[ fallthrough ]];
				case rsx::texture_dimension_extended::texture_dimension_2d:
					scale.height /= attributes.height;
					[[ fallthrough ]];
				default:
					scale.width /= attributes.width;
					break;
				}
			}

			const auto packed_pitch = get_format_packed_pitch(attributes.gcm_format, attributes.width, !tex.border_type(), attributes.swizzled);
			if (!attributes.swizzled) [[likely]]
			{
				if (attributes.pitch = tex.pitch(); !attributes.pitch)
				{
					attributes.pitch = packed_pitch;
					scale = { 1.f, 0.f, 0.f };
				}
				else if (packed_pitch > attributes.pitch && !options.is_compressed_format)
				{
					scale.width *= f32(packed_pitch) / attributes.pitch;
					attributes.width = attributes.pitch / attributes.bpp;
				}
			}
			else
			{
				attributes.pitch = packed_pitch;
			}

			switch (extended_dimension)
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				attributes.depth = 1;
				attributes.height = 1;
				attributes.slice_h = 1;
				attributes.edge_clamped = (tex.wrap_s() == rsx::texture_wrap_mode::clamp_to_edge);
				scale.height = scale.depth = 0.f;
				subsurface_count = 1;
				required_surface_height = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				attributes.depth = 1;
				attributes.edge_clamped = (tex.wrap_s() == rsx::texture_wrap_mode::clamp_to_edge && tex.wrap_t() == rsx::texture_wrap_mode::clamp_to_edge);
				scale.depth = 0.f;
				subsurface_count = options.is_compressed_format? 1 : tex.get_exact_mipmap_count();
				attributes.slice_h = required_surface_height = attributes.height;
				break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				attributes.depth = 6;
				subsurface_count = 1;
				tex_size = static_cast<u32>(get_texture_size(tex));
				required_surface_height = tex_size / attributes.pitch;
				attributes.slice_h = required_surface_height / attributes.depth;
				break;
			case rsx::texture_dimension_extended::texture_dimension_3d:
				attributes.depth = tex.depth();
				subsurface_count = 1;
				tex_size = static_cast<u32>(get_texture_size(tex));
				required_surface_height = tex_size / attributes.pitch;
				attributes.slice_h = required_surface_height / attributes.depth;
				break;
			default:
				fmt::throw_exception("Unsupported texture dimension %d", static_cast<int>(extended_dimension));
			}

			// Validation
			if (!attributes.width || !attributes.height || !attributes.depth)
			{
				rsx_log.warning("Image at address 0x%x has invalid dimensions. Type=%d, Dims=%dx%dx%d",
					attributes.address, static_cast<s32>(extended_dimension),
					attributes.width, attributes.height, attributes.depth);
				return {};
			}

			if (options.is_compressed_format)
			{
				// Compressed textures cannot be 1D in some APIs
				extended_dimension = std::max(extended_dimension, rsx::texture_dimension_extended::texture_dimension_2d);
			}

			const auto lookup_range = utils::address_range32::start_length(attributes.address, attributes.pitch * required_surface_height);
			reader_lock lock(m_cache_mutex);

			auto result = fast_texture_search(cmd, attributes, scale, tex.decoded_remap(),
				options, lookup_range, extended_dimension, m_rtts,
				std::forward<Args>(extras)...);

			if (result.validate())
			{
				if (!result.image_handle) [[unlikely]]
				{
					// Deferred reconstruct
					result.external_subresource_desc.cache_range = lookup_range;
				}
				else if (result.texcoord_xform.clamp)
				{
					m_texture_copies_ellided_this_frame++;
				}

				if (!result.ref_address)
				{
					result.ref_address = attributes.address;
				}

				result.surface_cache_tag = m_rtts.write_tag;

				if (subsurface_count == 1)
				{
					return result;
				}

				switch (result.upload_context)
				{
				case rsx::texture_upload_context::blit_engine_dst:
				case rsx::texture_upload_context::framebuffer_storage:
					break;
				case rsx::texture_upload_context::shader_read:
					if (!result.image_handle)
						break;
					[[fallthrough]];
				default:
					return result;
				}

				// Traverse the mipmap tree
				// Some guarantees here include:
				// 1. Only 2D images will invoke this routine
				// 2. The image has to have been generated on the GPU (fbo or blit target only)

				std::vector<copy_region_descriptor> sections;
				const bool use_upscaling = (result.upload_context == rsx::texture_upload_context::framebuffer_storage && g_cfg.video.resolution_scale_percent != 100);

				if (!helpers::append_mipmap_level(sections, result, attributes, 0, use_upscaling, attributes)) [[unlikely]]
				{
					// Abort if mip0 is not compatible
					return result;
				}

				auto attr2 = attributes;
				sections.reserve(subsurface_count);

				options.skip_texture_merge = true;
				options.skip_texture_barriers = true;
				options.prefer_surface_cache = (result.upload_context == rsx::texture_upload_context::framebuffer_storage);

				for (u8 subsurface = 1; subsurface < subsurface_count; ++subsurface)
				{
					attr2.address += (attr2.pitch * attr2.height);
					attr2.width = std::max(attr2.width / 2, 1);
					attr2.height = std::max(attr2.height / 2, 1);
					attr2.slice_h = attr2.height;

					if (attributes.swizzled)
					{
						attr2.pitch = attr2.width * attr2.bpp;
					}

					const auto range = utils::address_range32::start_length(attr2.address, attr2.pitch * attr2.height);
					auto ret = fast_texture_search(cmd, attr2, scale, tex.decoded_remap(),
						options, range, extended_dimension, m_rtts, std::forward<Args>(extras)...);

					if (!ret.validate() ||
						!helpers::append_mipmap_level(sections, ret, attr2, subsurface, use_upscaling, attributes))
					{
						// Abort
						break;
					}
				}

				if (sections.size() == 1) [[unlikely]]
				{
					return result;
				}
				else
				{
					// NOTE: Do not disable 'cyclic ref' since the texture_barrier may have already been issued!
					result.image_handle = 0;
					result.external_subresource_desc = { 0, deferred_request_command::mipmap_gather, attributes, {}, tex.decoded_remap() };
					result.format_class = rsx::classify_format(attributes.gcm_format);

					if (result.texcoord_xform.clamp)
					{
						// Revert clamp configuration
						result.pop_texcoord_xform();
					}

					if (use_upscaling)
					{
						// Grab the correct image dimensions from the base mipmap level
						const auto& mip0 = sections.front();
						result.external_subresource_desc.width = mip0.dst_w;
						result.external_subresource_desc.height = mip0.dst_h;
					}

					const u32 cache_end = attr2.address + (attr2.pitch * attr2.height);
					result.external_subresource_desc.cache_range = utils::address_range32::start_end(attributes.address, cache_end);

					result.external_subresource_desc.sections_to_copy = std::move(sections);
					return result;
				}
			}

			// Do direct upload from CPU as the last resort
			m_texture_upload_misses_this_frame++;

			const auto subresources_layout = get_subresources_layout(tex);
			const auto format_class = classify_format(attributes.gcm_format);

			if (!tex_size)
			{
				tex_size = static_cast<u32>(get_texture_size(tex));
			}

			lock.upgrade();

			// Invalidate
			const address_range32 tex_range = address_range32::start_length(attributes.address, tex_size);
			invalidate_range_impl_base(cmd, tex_range, invalidation_cause::read, {}, std::forward<Args>(extras)...);

			// Upload from CPU. Note that sRGB conversion is handled in the FS
			auto uploaded = upload_image_from_cpu(cmd, tex_range, attributes.width, attributes.height, attributes.depth, tex.get_exact_mipmap_count(), attributes.pitch, attributes.gcm_format,
				texture_upload_context::shader_read, subresources_layout, extended_dimension, attributes.swizzled);

			return{ uploaded->get_view(tex.decoded_remap()),
					texture_upload_context::shader_read, format_class, scale, extended_dimension };
		}

		// FIXME: This function is way too large and needs an urgent refactor.
		template <typename surface_store_type, typename blitter_type, typename ...Args>
		blit_op_result upload_scaled_image(const rsx::blit_src_info& src_info, const rsx::blit_dst_info& dst_info, bool interpolate, commandbuffer_type& cmd, surface_store_type& m_rtts, blitter_type& blitter, Args&&... extras)
		{
			// Local working copy. We may modify the descriptors for optimization purposes
			auto src = src_info;
			auto dst = dst_info;

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

			const bool is_copy_op = (fcmp(scale_x, 1.f) && fcmp(scale_y, 1.f));
			const bool is_format_convert = (dst_is_argb8 != src_is_argb8);
			bool skip_if_collision_exists = false;

			// Offset in x and y for src is 0 (it is already accounted for when getting pixels_src)
			// Reproject final clip onto source...
			u16 src_w = static_cast<u16>(dst.clip_width / scale_x);
			u16 src_h = static_cast<u16>(dst.clip_height / scale_y);

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

				if ((src_h + src.offset_y) > src.height) [[unlikely]]
				{
					// TODO: Special case that needs wrapping around (custom blit)
					rsx_log.error("Transfer cropped in Y, src_h=%d, offset_y=%d, block_h=%d", src_h, src.offset_y, src.height);
					src_h = src.height - src.offset_y;
				}

				if ((src_w + src.offset_x) > src.width) [[unlikely]]
				{
					// TODO: Special case that needs wrapping around (custom blit)
					rsx_log.error("Transfer cropped in X, src_w=%d, offset_x=%d, block_w=%d", src_w, src.offset_x, src.width);
					src_w = src.width - src.offset_x;
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

			const auto get_tiled_region = [&](const utils::address_range32& range)
			{
				auto rsxthr = rsx::get_current_renderer();
				return rsxthr->get_tiled_memory_region(range);
			};

			auto rtt_lookup = [&m_rtts, &cmd, &scale_x, &scale_y](u32 address, u32 width, u32 height, u32 pitch, u8 bpp, rsx::flags32_t access, bool allow_clipped) -> typename surface_store_type::surface_overlap_info
			{
				const auto list = m_rtts.get_merged_texture_memory_region(cmd, address, width, height, pitch, bpp, access);
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

					const auto _w = It->dst_area.width;
					const auto _h = It->dst_area.height;

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

			auto validate_memory_range = [](u32 base_address, u32 write_end, u32 heuristic_end)
			{
				if (heuristic_end <= write_end)
				{
					return true;
				}

				// Confirm if the pages actually exist in vm
				if (get_location(base_address) == CELL_GCM_LOCATION_LOCAL)
				{
					const auto vram_end = rsx::get_current_renderer()->local_mem_size + rsx::constants::local_mem_base;
					if (heuristic_end > vram_end)
					{
						// Outside available VRAM area
						return false;
					}
				}
				else
				{
					if (!vm::check_addr(write_end, vm::page_readable, (heuristic_end - write_end)))
					{
						// Enforce strict allocation size!
						return false;
					}
				}

				return true;
			};

			auto validate_fbo_integrity = [&](const utils::address_range32& range, bool is_depth_texture)
			{
				const bool will_upload = is_depth_texture ? !!g_cfg.video.read_depth_buffer : !!g_cfg.video.read_color_buffers;
				if (!will_upload)
				{
					// Give a pass. The data is lost anyway.
					return true;
				}

				const bool should_be_locked = is_depth_texture ? !!g_cfg.video.write_depth_buffer : !!g_cfg.video.write_color_buffers;
				if (!should_be_locked)
				{
					// Data is lost anyway.
					return true;
				}

				// Optimal setup. We have ideal conditions presented so we can correctly decide what to do here.
				const auto section = find_cached_texture(range, { .gcm_format = RSX_GCM_FORMAT_IGNORED }, false, false, false);
				return section && section->is_locked();
			};

			// Check tiled mem
			const auto dst_tile = get_tiled_region(utils::address_range32::start_length(dst_address, dst.pitch * dst.clip_height));
			const auto src_tile = get_tiled_region(utils::address_range32::start_length(src_address, src.pitch * src.height));
			const auto dst_is_tiled = !!dst_tile;
			const auto src_is_tiled = !!src_tile;

			// Check if src/dst are parts of render targets
			typename surface_store_type::surface_overlap_info dst_subres;
			bool use_null_region = false;

			// TODO: Handle cases where src or dst can be a depth texture while the other is a color texture - requires a render pass to emulate
			// NOTE: Grab the src first as requirements for reading are more strict than requirements for writing
			auto src_subres = rtt_lookup(src_address, src_w, src_h, src.pitch, src_bpp, surface_access::transfer_read, false);
			src_is_render_target = src_subres.surface != nullptr;

			if (get_location(dst_address) == CELL_GCM_LOCATION_LOCAL)
			{
				// TODO: HACK
				// After writing, it is required to lock the memory range from access!
				dst_subres = rtt_lookup(dst_address, dst_w, dst_h, dst.pitch, dst_bpp, surface_access::transfer_write, false);
				dst_is_render_target = dst_subres.surface != nullptr;
			}
			else
			{
				// Surface exists in main memory.
				use_null_region = (is_copy_op && !is_format_convert);

				// Now we have a blit write into main memory. This really could be anything, so we need to be careful here.
				// If we have a pitched write, or a suspiciously large transfer, we likely have a valid write.

				// Invalidate surfaces in range. Sample tests should catch overlaps in theory.
				m_rtts.invalidate_range(utils::address_range32::start_length(dst_address, dst.pitch * dst_h));
			}

			// FBO re-validation. It is common for GPU and CPU data to desync as we do not have a way to share memory pages directly between the two (in most setups)
			// To avoid losing data, we need to do some gymnastics
			if (src_is_render_target && !validate_fbo_integrity(src_subres.surface->get_memory_range(), src_subres.is_depth))
			{
				src_is_render_target = false;
				src_subres.surface = nullptr;
			}

			if (dst_is_render_target && !validate_fbo_integrity(dst_subres.surface->get_memory_range(), dst_subres.is_depth))
			{
				// This is a lot more serious that the src case. We have to signal surface cache to reload the memory and discard what we have GPU-side.
				// Do the transfer CPU side and we should eventually "read" the data on RCB/RDB barrier.
				dst_subres.surface->invalidate_GPU_memory();
				return false;
			}

			if (src_is_render_target)
			{
				const auto surf = src_subres.surface;
				const auto bpp = surf->get_bpp();
				const bool typeless = (bpp != src_bpp || is_format_convert);

				if (!typeless) [[likely]]
				{
					// Use format as-is
					typeless_info.src_gcm_format = helpers::get_sized_blit_format(src_is_argb8, src_subres.is_depth, false);
				}
				else
				{
					// Enable type scaling in src
					typeless_info.src_is_typeless = true;
					typeless_info.src_scaling_hint = static_cast<f32>(bpp) / src_bpp;
					typeless_info.src_gcm_format = helpers::get_sized_blit_format(src_is_argb8, false, is_format_convert);
				}

				if (surf->template get_surface_width<rsx::surface_metrics::pixels>() != surf->width() ||
					surf->template get_surface_height<rsx::surface_metrics::pixels>() != surf->height())
				{
					// Must go through a scaling operation due to resolution scaling being present
					ensure(g_cfg.video.resolution_scale_percent != 100);
					use_null_region = false;
				}
			}
			else
			{
				// Determine whether to perform this transfer on CPU or GPU (src data may not be graphical)
				const bool is_trivial_copy = is_copy_op && !is_format_convert && !dst.swizzled && !dst_is_tiled && !src_is_tiled;
				const bool is_block_transfer = (dst_w == src_w && dst_h == src_h && (src.pitch == dst.pitch || src_h == 1));
				const bool is_mirror_op = (dst.scale_x < 0.f || dst.scale_y < 0.f);

				if (dst_is_render_target)
				{
					if (is_trivial_copy && src_h == 1)
					{
						dst_is_render_target = false;
						dst_subres = {};
					}
				}

				// Always use GPU blit if src or dst is in the surface store
				if (!dst_is_render_target)
				{
					if (is_trivial_copy)
					{
						// Check if trivial memcpy can perform the same task
						// Used to copy programs and arbitrary data to the GPU in some cases
						// NOTE: This case overrides the GPU texture scaling option
						if (is_block_transfer && !is_mirror_op)
						{
							return false;
						}

						// If a matching section exists with a different use-case, fall back to CPU memcpy
						skip_if_collision_exists = true;
					}

					if (!g_cfg.video.use_gpu_texture_scaling && !dst_is_tiled && !src_is_tiled)
					{
						if (dst.swizzled)
						{
							// Swizzle operation requested. Use fallback
							return false;
						}

						if (is_trivial_copy && get_location(dst_address) != CELL_GCM_LOCATION_LOCAL)
						{
							// Trivial copy and the destination is in XDR memory
							return false;
						}
					}
				}
			}

			if (dst_is_render_target)
			{
				const auto bpp = dst_subres.surface->get_bpp();
				const bool typeless = (bpp != dst_bpp || is_format_convert);

				if (!typeless) [[likely]]
				{
					typeless_info.dst_gcm_format = helpers::get_sized_blit_format(dst_is_argb8, dst_subres.is_depth, false);
				}
				else
				{
					// Enable type scaling in dst
					typeless_info.dst_is_typeless = true;
					typeless_info.dst_scaling_hint = static_cast<f32>(bpp) / dst_bpp;
					typeless_info.dst_gcm_format = helpers::get_sized_blit_format(dst_is_argb8, false, is_format_convert);
				}
			}

			section_storage_type* cached_dest = nullptr;
			section_storage_type* cached_src = nullptr;
			bool dst_is_depth_surface = false;
			u16 max_dst_width = dst.width;
			u16 max_dst_height = dst.height;
			areai src_area = { 0, 0, src_w, src_h };
			areai dst_area = { 0, 0, dst_w, dst_h };

			size2i dst_dimensions = { static_cast<s32>(dst.pitch / dst_bpp), dst.height };
			position2i dst_offset = { dst.offset_x, dst.offset_y };
			u32 dst_base_address = dst.rsx_address;

			const auto src_payload_length = (src.pitch * (src_h - 1) + (src_w * src_bpp));
			const auto dst_payload_length = (dst.pitch * (dst_h - 1) + (dst_w * dst_bpp));
			const auto dst_range = address_range32::start_length(dst_address, dst_payload_length);

			if (!use_null_region && !dst_is_render_target)
			{
				size2u src_dimensions = { 0, 0 };
				if (src_is_render_target)
				{
					src_dimensions.width = src_subres.surface->template get_surface_width<rsx::surface_metrics::samples>();
					src_dimensions.height = src_subres.surface->template get_surface_height<rsx::surface_metrics::samples>();
				}

				const auto props = texture_cache_helpers::get_optimal_blit_target_properties(
					src_is_render_target,
					dst_range,
					dst.pitch,
					src_dimensions,
					static_cast<size2u>(dst_dimensions)
				);

				if (props.use_dma_region)
				{
					// Try to use a dma flush
					use_null_region = (is_copy_op && !is_format_convert);
				}
				else
				{
					if (props.offset)
					{
						// Calculate new offsets
						dst_base_address = props.offset;
						const auto new_offset = (dst_address - dst_base_address);

						// Generate new offsets
						dst_offset.y = new_offset / dst.pitch;
						dst_offset.x = (new_offset % dst.pitch) / dst_bpp;
					}

					dst_dimensions.width = static_cast<s32>(props.width);
					dst_dimensions.height = static_cast<s32>(props.height);
				}
			}

			reader_lock lock(m_cache_mutex);

			const auto old_dst_area = dst_area;
			if (!dst_is_render_target)
			{
				// Check for any available region that will fit this one
				u32 required_type_mask;
				if (use_null_region)
				{
					required_type_mask = texture_upload_context::dma;
				}
				else
				{
					required_type_mask = texture_upload_context::blit_engine_dst;
					if (skip_if_collision_exists) required_type_mask |= texture_upload_context::shader_read;
				}

				auto overlapping_surfaces = find_texture_from_range(dst_range, dst.pitch, required_type_mask);
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

					if (!dst_range.inside(surface->get_section_range()))
					{
						// Hit test failed
						continue;
					}

					if (use_null_region)
					{
						// Attach to existing region
						cached_dest = surface;

						// Technically it is totally possible to just extend a pre-existing section
						// Will leave this as a TODO
						continue;
					}

					if (skip_if_collision_exists) [[unlikely]]
					{
						if (surface->get_context() != texture_upload_context::blit_engine_dst)
						{
							// This section is likely to be 'flushed' to CPU for reupload soon anyway
							return false;
						}
					}

					// Prefer formats which will not trigger a typeless conversion later
					// Only color formats are supported as destination as most access from blit engine will be color
					switch (surface->get_gcm_format())
					{
					case CELL_GCM_TEXTURE_A8R8G8B8:
						if (!dst_is_argb8) continue;
						break;
					case CELL_GCM_TEXTURE_R5G6B5:
						if (dst_is_argb8) continue;
						break;
					default:
						continue;
					}

					if (const auto this_address = surface->get_section_base();
						const u32 address_offset = dst_address - this_address)
					{
						const u32 offset_y = address_offset / dst.pitch;
						const u32 offset_x = address_offset % dst.pitch;
						const u32 offset_x_in_block = offset_x / dst_bpp;

						dst_area.x1 += offset_x_in_block;
						dst_area.x2 += offset_x_in_block;
						dst_area.y1 += offset_y;
						dst_area.y2 += offset_y;
					}

					// Validate clipping region
					if (static_cast<uint>(dst_area.x2) <= surface->get_width() &&
						static_cast<uint>(dst_area.y2) <= surface->get_height())
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

				if (cached_dest && cached_dest->get_context() != texture_upload_context::dma)
				{
					// NOTE: DMA sections are plain memory blocks with no format!
					if (cached_dest) [[likely]]
					{
						typeless_info.dst_gcm_format = cached_dest->get_gcm_format();
						dst_is_depth_surface = cached_dest->is_depth_texture();
					}
				}
			}
			else
			{
				// Destination dimensions are relaxed (true)
				dst_area = dst_subres.src_area;

				dest_texture = dst_subres.surface->get_surface(rsx::surface_access::transfer_write);
				typeless_info.dst_context = texture_upload_context::framebuffer_storage;
				dst_is_depth_surface = typeless_info.dst_is_typeless ? false : dst_subres.is_depth;

				max_dst_width = static_cast<u16>(dst_subres.surface->template get_surface_width<rsx::surface_metrics::samples>() * typeless_info.dst_scaling_hint);
				max_dst_height = dst_subres.surface->template get_surface_height<rsx::surface_metrics::samples>();
			}

			// Create source texture if does not exist
			// TODO: This can be greatly improved with DMA optimizations. Most transfer operations here are actually non-graphical (no transforms applied)
			if (!src_is_render_target)
			{
				// NOTE: Src address already takes into account the flipped nature of the overlap!
				const u32 lookup_mask = rsx::texture_upload_context::blit_engine_src | rsx::texture_upload_context::blit_engine_dst | rsx::texture_upload_context::shader_read;
				auto overlapping_surfaces = find_texture_from_range<false>(address_range32::start_length(src_address, src_payload_length), src.pitch, lookup_mask);
				auto old_src_area = src_area;

				for (const auto &surface : overlapping_surfaces)
				{
					if (!surface->is_locked())
					{
						// TODO: Rejecting unlocked blit_engine dst causes stutter in SCV
						// Surfaces marked as dirty have already been removed, leaving only flushed blit_dst data
						continue;
					}

					// Force format matching; only accept 16-bit data for 16-bit transfers, 32-bit for 32-bit transfers
					switch (surface->get_gcm_format())
					{
					case CELL_GCM_TEXTURE_X32_FLOAT:
					case CELL_GCM_TEXTURE_Y16_X16:
					case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
					{
						// Should be copy compatible but not scaling compatible
						if (src_is_argb8 && (is_copy_op || dst_is_render_target)) break;
						continue;
					}
					case CELL_GCM_TEXTURE_DEPTH24_D8:
					case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
					{
						// Should be copy compatible but not scaling compatible
						if (src_is_argb8 && (is_copy_op || !dst_is_render_target)) break;
						continue;
					}
					case CELL_GCM_TEXTURE_A8R8G8B8:
					case CELL_GCM_TEXTURE_D8R8G8B8:
					{
						// Perfect match
						if (src_is_argb8) break;
						continue;
					}
					case CELL_GCM_TEXTURE_X16:
					case CELL_GCM_TEXTURE_G8B8:
					case CELL_GCM_TEXTURE_A1R5G5B5:
					case CELL_GCM_TEXTURE_A4R4G4B4:
					case CELL_GCM_TEXTURE_D1R5G5B5:
					case CELL_GCM_TEXTURE_R5G5B5A1:
					{
						// Copy compatible
						if (!src_is_argb8 && (is_copy_op || dst_is_render_target)) break;
						continue;
					}
					case CELL_GCM_TEXTURE_DEPTH16:
					case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
					{
						// Copy compatible
						if (!src_is_argb8 && (is_copy_op || !dst_is_render_target)) break;
						continue;
					}
					case CELL_GCM_TEXTURE_R5G6B5:
					{
						// Perfect match
						if (!src_is_argb8) break;
						continue;
					}
					default:
					{
						continue;
					}
					}

					const auto this_address = surface->get_section_base();
					if (this_address > src_address)
					{
						continue;
					}

					if (const u32 address_offset = src_address - this_address)
					{
						const u32 offset_y = address_offset / src.pitch;
						const u32 offset_x = address_offset % src.pitch;
						const u32 offset_x_in_block = offset_x / src_bpp;

						src_area.x1 += offset_x_in_block;
						src_area.x2 += offset_x_in_block;
						src_area.y1 += offset_y;
						src_area.y2 += offset_y;
					}

					if (src_area.x2 <= surface->get_width() &&
						src_area.y2 <= surface->get_height())
					{
						cached_src = surface;
						break;
					}

					src_area = old_src_area;
				}

				if (!cached_src)
				{
					const u16 full_width = src.pitch / src_bpp;
					u32 image_base = src.rsx_address;
					u16 image_width = full_width;
					u16 image_height = src.height;

					// Check if memory is valid
					const bool use_full_range = validate_memory_range(
						image_base,
						(src_address + src_payload_length),
						image_base + (image_height * src.pitch));

					if (use_full_range && dst.scale_x > 0.f && dst.scale_y > 0.f) [[likely]]
					{
						// Loading full image from the corner address
						// Translate src_area into the declared block
						src_area.x1 += src.offset_x;
						src_area.x2 += src.offset_x;
						src_area.y1 += src.offset_y;
						src_area.y2 += src.offset_y;
					}
					else
					{
						image_base = src_address;
						image_height = src_h;
					}

					std::vector<rsx::subresource_layout> subresource_layout;
					rsx::subresource_layout subres = {};
					subres.width_in_block = subres.width_in_texel = image_width;
					subres.height_in_block = subres.height_in_texel = image_height;
					subres.pitch_in_block = full_width;
					subres.depth = 1;
					subres.data = { vm::_ptr<const std::byte>(image_base), static_cast<std::span<const std::byte>::size_type>(src.pitch * image_height) };
					subresource_layout.push_back(subres);

					const u32 gcm_format = helpers::get_sized_blit_format(src_is_argb8, dst_is_depth_surface, is_format_convert);
					const auto rsx_range = address_range32::start_length(image_base, src.pitch * image_height);

					lock.upgrade();

					invalidate_range_impl_base(cmd, rsx_range, invalidation_cause::read, {}, std::forward<Args>(extras)...);

					cached_src = upload_image_from_cpu(cmd, rsx_range, image_width, image_height, 1, 1, src.pitch, gcm_format, texture_upload_context::blit_engine_src,
						subresource_layout, rsx::texture_dimension_extended::texture_dimension_2d, dst.swizzled);

					typeless_info.src_gcm_format = gcm_format;
				}
				else
				{
					typeless_info.src_gcm_format = cached_src->get_gcm_format();
				}

				cached_src->add_ref();
				vram_texture = cached_src->get_raw_texture();
				typeless_info.src_context = cached_src->get_context();
			}
			else
			{
				src_area = src_subres.src_area;
				vram_texture = src_subres.surface->get_surface(rsx::surface_access::transfer_read);
				typeless_info.src_context = texture_upload_context::framebuffer_storage;
			}

			//const auto src_is_depth_format = helpers::is_gcm_depth_format(typeless_info.src_gcm_format);
			const auto preferred_dst_format = helpers::get_sized_blit_format(dst_is_argb8, false, is_format_convert);

			if (cached_dest && !use_null_region)
			{
				// Prep surface
				auto channel_order = src_is_render_target ? rsx::component_order::native :
					dst_is_argb8 ? rsx::component_order::default_ :
					rsx::component_order::swapped_native;

				set_component_order(*cached_dest, preferred_dst_format, channel_order);
			}

			// Validate clipping region
			if ((dst.offset_x + dst.clip_x + dst.clip_width) > max_dst_width) dst.clip_x = 0;
			if ((dst.offset_y + dst.clip_y + dst.clip_height) > max_dst_height) dst.clip_y = 0;

			// Reproject clip offsets onto source to simplify blit
			if (dst.clip_x || dst.clip_y)
			{
				const u16 scaled_clip_offset_x = static_cast<u16>(dst.clip_x / (scale_x * typeless_info.src_scaling_hint));
				const u16 scaled_clip_offset_y = static_cast<u16>(dst.clip_y / scale_y);

				src_area.x1 += scaled_clip_offset_x;
				src_area.x2 += scaled_clip_offset_x;
				src_area.y1 += scaled_clip_offset_y;
				src_area.y2 += scaled_clip_offset_y;
			}

			if (!cached_dest && !dst_is_render_target)
			{
				ensure(!dest_texture);

				// Need to calculate the minimum required size that will fit the data, anchored on the rsx_address
				// If the application starts off with an 'inseted' section, the guessed dimensions may not fit!
				const u32 write_end = dst_address + dst_payload_length;
				u32 block_end = dst_base_address + (dst.pitch * dst_dimensions.height);

				// Confirm if the pages actually exist in vm
				if (!validate_memory_range(dst_base_address, write_end, block_end))
				{
					block_end = write_end;
				}

				const u32 usable_section_length = std::max(write_end, block_end) - dst_base_address;
				dst_dimensions.height = align2(usable_section_length, dst.pitch) / dst.pitch;

				const u32 full_section_length = ((dst_dimensions.height - 1) * dst.pitch) + (dst_dimensions.width * dst_bpp);
				const auto rsx_range = address_range32::start_length(dst_base_address, full_section_length);

				lock.upgrade();

				// NOTE: Write flag set to remove all other overlapping regions (e.g shader_read or blit_src)
				// NOTE: This step can potentially invalidate the newly created src image as well.
				invalidate_range_impl_base(cmd, rsx_range, invalidation_cause::cause_is_write | invalidation_cause::cause_uses_strict_data_bounds, {}, std::forward<Args>(extras)...);

				if (use_null_region) [[likely]]
				{
					bool force_dma_load = false;
					if ((dst_w * dst_bpp) != dst.pitch)
					{
						// Keep Cell from touching the range we need
						const auto prot_range = dst_range.to_page_range();
						utils::memory_protect(vm::base(prot_range.start), prot_range.length(), utils::protection::no);

						force_dma_load = true;
					}

					const image_section_attributes_t attrs =
					{
						.pitch = dst.pitch,
						.width = static_cast<u16>(dst_dimensions.width),
						.height = static_cast<u16>(dst_dimensions.height),
						.bpp = dst_bpp
					};
					cached_dest = create_nul_section(cmd, rsx_range, attrs, dst_tile, force_dma_load);
				}
				else
				{
					// render target data is already in correct swizzle layout
					auto channel_order = src_is_render_target ? rsx::component_order::native :
						dst_is_argb8 ? rsx::component_order::default_ :
						rsx::component_order::swapped_native;

					// Translate dst_area into the 'full' dst block based on dst.rsx_address as (0, 0)
					dst_area.x1 += dst_offset.x;
					dst_area.x2 += dst_offset.x;
					dst_area.y1 += dst_offset.y;
					dst_area.y2 += dst_offset.y;

					if (!dst_area.x1 && !dst_area.y1 && dst_area.x2 == dst_dimensions.width && dst_area.y2 == dst_dimensions.height)
					{
						cached_dest = create_new_texture(cmd, rsx_range, dst_dimensions.width, dst_dimensions.height, 1, 1, dst.pitch,
							preferred_dst_format, rsx::texture_upload_context::blit_engine_dst, rsx::texture_dimension_extended::texture_dimension_2d,
							dst.swizzled, channel_order, 0);
					}
					else
					{
						// HACK: workaround for data race with Cell
						// Pre-lock the memory range we'll be touching, then load with super_ptr
						const auto prot_range = dst_range.to_page_range();
						utils::memory_protect(vm::base(prot_range.start), prot_range.length(), utils::protection::no);

						const auto pitch_in_block = dst.pitch / dst_bpp;
						std::vector<rsx::subresource_layout> subresource_layout;
						rsx::subresource_layout subres = {};
						subres.width_in_block = subres.width_in_texel = dst_dimensions.width;
						subres.height_in_block = subres.height_in_texel = dst_dimensions.height;
						subres.pitch_in_block = pitch_in_block;
						subres.depth = 1;
						subres.data = { vm::get_super_ptr<const std::byte>(dst_base_address), static_cast<std::span<const std::byte>::size_type>(dst.pitch * dst_dimensions.height) };
						subresource_layout.push_back(subres);

						cached_dest = upload_image_from_cpu(cmd, rsx_range, dst_dimensions.width, dst_dimensions.height, 1, 1, dst.pitch,
							preferred_dst_format, rsx::texture_upload_context::blit_engine_dst, subresource_layout,
							rsx::texture_dimension_extended::texture_dimension_2d, dst.swizzled);

						set_component_order(*cached_dest, preferred_dst_format, channel_order);
					}

					dest_texture = cached_dest->get_raw_texture();
					typeless_info.dst_context = texture_upload_context::blit_engine_dst;
					typeless_info.dst_gcm_format = preferred_dst_format;
				}
			}

			ensure(cached_dest || dst_is_render_target);

			// Invalidate any cached subresources in modified range
			notify_surface_changed(dst_range);

			// What type of data is being moved?
			const auto raster_type = src_is_render_target ? src_subres.surface->raster_type : rsx::surface_raster_type::undefined;

			if (cached_dest)
			{
				// Validate modified range
				u32 mem_offset = dst_address - cached_dest->get_section_base();
				ensure((mem_offset + dst_payload_length) <= cached_dest->get_section_size());

				lock.upgrade();

				cached_dest->reprotect(utils::protection::no, { mem_offset, dst_payload_length });
				cached_dest->touch(m_cache_update_tag);
				update_cache_tag();

				// Set swizzle flag
				cached_dest->set_swizzled(raster_type == rsx::surface_raster_type::swizzle || dst.swizzled);
			}
			else
			{
				// NOTE: This doesn't work very well in case of Cell access
				// Need to lock the affected memory range and actually attach this subres to a locked_region
				dst_subres.surface->on_write_copy(rsx::get_shared_tag(), false, raster_type);

				// Reset this object's synchronization status if it is locked
				lock.upgrade();

				if (const auto found = find_cached_texture(dst_subres.surface->get_memory_range(), { .gcm_format = RSX_GCM_FORMAT_IGNORED }, false, false, false))
				{
					if (found->is_locked())
					{
						if (found->get_rsx_pitch() == dst.pitch)
						{
							// It is possible for other resource types to overlap this fbo if it only covers a small section of its max width.
							// Blit engine read and write resources do not allow clipping and would have been recreated at the same address.
							// TODO: In cases of clipped data, generate the blit resources in the surface cache instead.
							if (found->get_context() == rsx::texture_upload_context::framebuffer_storage)
							{
								found->touch(m_cache_update_tag);
								update_cache_tag();
							}
						}
						else
						{
							// Unlikely situation, but the only one which would allow re-upload from CPU to overlap this section.
							if (found->is_flushable())
							{
								// Technically this is possible in games that may change surface pitch at random (insomniac engine)
								// FIXME: A proper fix includes pitch conversion and surface inheritance chains between surface targets and blit targets (unified cache) which is a very long-term thing.
								const auto range = found->get_section_range();
								rsx_log.error("[Pitch Mismatch] GPU-resident data at 0x%x->0x%x is discarded due to surface cache data clobbering it.", range.start, range.end);
							}
							found->discard(true);
						}
					}
				}
			}

			if (src_is_render_target)
			{
				const auto surface_width = src_subres.surface->template get_surface_width<rsx::surface_metrics::pixels>();
				const auto surface_height = src_subres.surface->template get_surface_height<rsx::surface_metrics::pixels>();
				std::tie(src_area.x1, src_area.y1) = rsx::apply_resolution_scale<false>(src_area.x1, src_area.y1, surface_width, surface_height);
				std::tie(src_area.x2, src_area.y2) = rsx::apply_resolution_scale<true>(src_area.x2, src_area.y2, surface_width, surface_height);

				// The resource is of surface type; possibly disabled AA emulation
				src_subres.surface->transform_blit_coordinates(rsx::surface_access::transfer_read, src_area);
			}

			if (dst_is_render_target)
			{
				const auto surface_width = dst_subres.surface->template get_surface_width<rsx::surface_metrics::pixels>();
				const auto surface_height = dst_subres.surface->template get_surface_height<rsx::surface_metrics::pixels>();
				std::tie(dst_area.x1, dst_area.y1) = rsx::apply_resolution_scale<false>(dst_area.x1, dst_area.y1, surface_width, surface_height);
				std::tie(dst_area.x2, dst_area.y2) = rsx::apply_resolution_scale<true>(dst_area.x2, dst_area.y2, surface_width, surface_height);

				// The resource is of surface type; possibly disabled AA emulation
				dst_subres.surface->transform_blit_coordinates(rsx::surface_access::transfer_write, dst_area);
			}

			if (helpers::is_gcm_depth_format(typeless_info.src_gcm_format) !=
				helpers::is_gcm_depth_format(typeless_info.dst_gcm_format))
			{
				// Make the depth side typeless because the other side is guaranteed to be color
				if (helpers::is_gcm_depth_format(typeless_info.src_gcm_format))
				{
					// SRC is depth, transfer must be done typelessly
					if (!typeless_info.src_is_typeless)
					{
						typeless_info.src_is_typeless = true;
						typeless_info.src_gcm_format = helpers::get_sized_blit_format(src_is_argb8, false, false);
					}
				}
				else
				{
					// DST is depth, transfer must be done typelessly
					if (!typeless_info.dst_is_typeless)
					{
						typeless_info.dst_is_typeless = true;
						typeless_info.dst_gcm_format = helpers::get_sized_blit_format(dst_is_argb8, false, false);
					}
				}
			}

			if (!use_null_region) [[likely]]
			{
				// Do preliminary analysis
				typeless_info.analyse();

				blitter.scale_image(cmd, vram_texture, dest_texture, src_area, dst_area, interpolate, typeless_info);
			}
			else if (cached_dest)
			{
				cached_dest->dma_transfer(cmd, vram_texture, src_area, dst_range, dst.pitch);
			}

			if (cached_src)
			{
				cached_src->release();
			}

			blit_op_result result = true;

			if (cached_dest)
			{
				result.real_dst_address = cached_dest->get_section_base();
				result.real_dst_size = cached_dest->get_section_size();
			}
			else
			{
				result.real_dst_address = dst_base_address;
				result.real_dst_size = dst.pitch * dst_dimensions.height;
			}

			return result;
		}

		void do_update()
		{
			if (!m_flush_always_cache.empty())
			{
				if (m_cache_update_tag.load() != m_flush_always_update_timestamp)
				{
					std::lock_guard lock(m_cache_mutex);
					bool update_tag = false;

					for (const auto &It : m_flush_always_cache)
					{
						auto& section = *(It.second);
						if (section.get_protection() != utils::protection::no)
						{
							ensure(section.exists());
							AUDIT(section.get_context() == texture_upload_context::framebuffer_storage);
							AUDIT(section.get_memory_read_flags() == memory_read_flags::flush_always);

							section.reprotect(utils::protection::no);
							update_tag = true;
						}
					}

					if (update_tag) update_cache_tag();
					m_flush_always_update_timestamp = m_cache_update_tag.load();

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

		bool is_protected(u32 section_base_address, const address_range32& test_range, rsx::texture_upload_context context)
		{
			reader_lock lock(m_cache_mutex);

			const auto& block = m_storage.block_for(section_base_address);
			for (const auto& tex : block)
			{
				if (tex.get_section_base() == section_base_address)
				{
					return tex.get_context() == context &&
						tex.is_locked() &&
						test_range.inside(tex.get_section_range());
				}
			}

			return false;
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
			m_texture_upload_calls_this_frame.store(0u);
			m_texture_upload_misses_this_frame.store(0u);
			m_texture_copies_ellided_this_frame.store(0u);
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

		virtual u32 get_unreleased_textures_count() const
		{
			return m_storage.m_unreleased_texture_objects;
		}

		u64 get_texture_memory_in_use() const
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
			return (num_flushes == 0u) ? 0.f : static_cast<f32>(m_misses_this_frame.load()) / num_flushes;
		}

		u32 get_texture_upload_calls_this_frame() const
		{
			return m_texture_upload_calls_this_frame;
		}

		u32 get_texture_upload_misses_this_frame() const
		{
			return m_texture_upload_misses_this_frame;
		}

		u32 get_texture_upload_miss_percentage() const
		{
			return (m_texture_upload_calls_this_frame)? (m_texture_upload_misses_this_frame * 100 / m_texture_upload_calls_this_frame) : 0;
		}

		u32 get_texture_copies_ellided_this_frame() const
		{
			return m_texture_copies_ellided_this_frame;
		}
	};
}
