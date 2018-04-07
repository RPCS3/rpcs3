#pragma once
#include "stdafx.h"
#include "VKRenderTargets.h"
#include "VKGSRender.h"
#include "Emu/System.h"
#include "../Common/TextureUtils.h"
#include "../rsx_utils.h"
#include "Utilities/mutex.h"
#include "../Common/texture_cache.h"

extern u64 get_system_time();

namespace vk
{
	class cached_texture_section : public rsx::cached_texture_section
	{
		std::unique_ptr<vk::image_view> uploaded_image_view;
		std::unique_ptr<vk::image> managed_texture = nullptr;

		//DMA relevant data
		VkFence dma_fence = VK_NULL_HANDLE;
		u64 sync_timestamp = 0;
		u64 last_use_timestamp = 0;
		vk::render_device* m_device = nullptr;
		vk::image *vram_texture = nullptr;
		std::unique_ptr<vk::buffer> dma_buffer;

	public:
	
		cached_texture_section() {}

		void reset(u32 base, u32 length)
		{
			if (length > cpu_address_range)
				release_dma_resources();

			rsx::protection_policy policy = g_cfg.video.strict_rendering_mode ? rsx::protection_policy::protect_policy_full_range : rsx::protection_policy::protect_policy_conservative;
			rsx::buffered_section::reset(base, length, policy);
		}

		void create(u16 w, u16 h, u16 depth, u16 mipmaps, vk::image_view *view, vk::image *image, u32 rsx_pitch, bool managed, u32 gcm_format, bool pack_swap_bytes = false)
		{
			width = w;
			height = h;
			this->depth = depth;
			this->mipmaps = mipmaps;

			this->gcm_format = gcm_format;
			this->pack_unpack_swap_bytes = pack_swap_bytes;

			if (managed)
			{
				managed_texture.reset(image);
				uploaded_image_view.reset(view);
			}
			else
			{
				verify(HERE), uploaded_image_view.get() == nullptr;
			}

			vram_texture = image;

			//TODO: Properly compute these values
			if (rsx_pitch > 0)
				this->rsx_pitch = rsx_pitch;
			else
				this->rsx_pitch = cpu_address_range / height;

			real_pitch = vk::get_format_texel_width(image->info.format) * width;

			//Even if we are managing the same vram section, we cannot guarantee contents are static
			//The create method is only invoked when a new mangaged session is required
			synchronized = false;
			flushed = false;
			sync_timestamp = 0ull;
			last_use_timestamp = get_system_time();
		}

		void release_dma_resources()
		{
			if (dma_buffer.get() != nullptr)
			{
				dma_buffer.reset();

				if (dma_fence != nullptr)
				{
					vkDestroyFence(*m_device, dma_fence, nullptr);
					dma_fence = VK_NULL_HANDLE;
				}
			}
		}

		void destroy()
		{
			vram_texture = nullptr;
			release_dma_resources();
		}

		bool exists() const
		{
			return (vram_texture != nullptr);
		}

		std::unique_ptr<vk::image_view>& get_view()
		{
			return uploaded_image_view;
		}

		std::unique_ptr<vk::image>& get_texture()
		{
			return managed_texture;
		}

		vk::image_view* get_raw_view()
		{
			if (context != rsx::texture_upload_context::framebuffer_storage)
				return uploaded_image_view.get();
			else
				return static_cast<vk::render_target*>(vram_texture)->get_view(0xAAE4, rsx::default_remap_vector);
		}

		vk::image* get_raw_texture()
		{
			return managed_texture.get();
		}

		VkFormat get_format()
		{
			return vram_texture->info.format;
		}

		bool is_flushable() const
		{
			//This section is active and can be flushed to cpu
			return (protection == utils::protection::no);
		}

		bool is_flushed() const
		{
			//This memory section was flushable, but a flush has already removed protection
			return (protection == utils::protection::rw && uploaded_image_view.get() == nullptr && managed_texture.get() == nullptr);
		}

		void copy_texture(bool manage_cb_lifetime, vk::command_buffer& cmd, vk::memory_type_mapping& memory_types, VkQueue submit_queue)
		{
			if (m_device == nullptr)
			{
				m_device = &cmd.get_command_pool().get_owner();
			}

			if (dma_fence == VK_NULL_HANDLE)
			{
				VkFenceCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				vkCreateFence(*m_device, &createInfo, nullptr, &dma_fence);
			}

			if (dma_buffer.get() == nullptr)
			{
				dma_buffer.reset(new vk::buffer(*m_device, align(cpu_address_range, 256), memory_types.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0));
			}

			if (manage_cb_lifetime)
			{
				cmd.begin();
			}

			const u16 internal_width = (context != rsx::texture_upload_context::framebuffer_storage? width : std::min(width, rsx::apply_resolution_scale(width, true)));
			const u16 internal_height = (context != rsx::texture_upload_context::framebuffer_storage? height : std::min(height, rsx::apply_resolution_scale(height, true)));

			VkImageAspectFlags aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
			switch (vram_texture->info.format)
			{
			case VK_FORMAT_D16_UNORM:
				aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			}

			//TODO: Read back stencil values (is this really necessary?)
			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = internal_width;
			copyRegion.bufferImageHeight = internal_height;
			copyRegion.imageSubresource = {aspect_flag & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1};
			copyRegion.imageOffset = {};
			copyRegion.imageExtent = {internal_width, internal_height, 1};

			VkImageSubresourceRange subresource_range = { aspect_flag, 0, 1, 0, 1 };
			
			VkImageLayout layout = vram_texture->current_layout;
			change_image_layout(cmd, vram_texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);
			vkCmdCopyImageToBuffer(cmd, vram_texture->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dma_buffer->value, 1, &copyRegion);
			change_image_layout(cmd, vram_texture, layout, subresource_range);

			if (manage_cb_lifetime)
			{
				cmd.end();
				cmd.submit(submit_queue, {}, dma_fence, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

				//Now we need to restart the command-buffer to restore it to the way it was before...
				CHECK_RESULT(vkWaitForFences(*m_device, 1, &dma_fence, VK_TRUE, UINT64_MAX));
				CHECK_RESULT(vkResetCommandBuffer(cmd, 0));
				vk::reset_fence(&dma_fence);

				if (cmd.access_hint != vk::command_buffer::access_type_hint::all)
					cmd.begin();
			}

			synchronized = true;
			sync_timestamp = get_system_time();
		}

		template<typename T, bool swapped>
		void do_memory_transfer(void *pixels_dst, const void *pixels_src)
		{
			if (sizeof(T) == 1)
				memcpy(pixels_dst, pixels_src, cpu_address_range);
			else
			{
				const u32 block_size = width * height;

				if (swapped)
				{
					auto typed_dst = (be_t<T> *)pixels_dst;
					auto typed_src = (T *)pixels_src;

					for (u32 px = 0; px < block_size; ++px)
						typed_dst[px] = typed_src[px];
				}
				else
				{
					memcpy(pixels_dst, pixels_src, block_size * sizeof(T));
				}
			}
		}

		bool flush(vk::command_buffer& cmd, vk::memory_type_mapping& memory_types, VkQueue submit_queue)
		{
			if (flushed) return true;

			if (m_device == nullptr)
			{
				m_device = &cmd.get_command_pool().get_owner();
			}

			// Return false if a flush occured 'late', i.e we had a miss
			bool result = true;

			if (!synchronized)
			{
				LOG_WARNING(RSX, "Cache miss at address 0x%X. This is gonna hurt...", cpu_address_base);
				copy_texture(true, cmd, memory_types, submit_queue);
				result = false;
			}

			flushed = true;

			void* pixels_src = dma_buffer->map(0, cpu_address_range);
			void* pixels_dst = vm::base(cpu_address_base);

			const u8 bpp = real_pitch / width;

			//We have to do our own byte swapping since the driver doesnt do it for us
			if (real_pitch == rsx_pitch)
			{
				bool is_depth_format = true;
				switch (vram_texture->info.format)
				{
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
					rsx::convert_le_f32_to_be_d24(pixels_dst, pixels_src, cpu_address_range >> 2, 1);
					break;
				case VK_FORMAT_D24_UNORM_S8_UINT:
					rsx::convert_le_d24x8_to_be_d24x8(pixels_dst, pixels_src, cpu_address_range >> 2, 1);
					break;
				default:
					is_depth_format = false;
					break;
				}

				if (!is_depth_format)
				{
					switch (bpp)
					{
					default:
						LOG_ERROR(RSX, "Invalid bpp %d", bpp);
					case 1:
						do_memory_transfer<u8, false>(pixels_dst, pixels_src);
						break;
					case 2:
						if (pack_unpack_swap_bytes)
							do_memory_transfer<u16, true>(pixels_dst, pixels_src);
						else
							do_memory_transfer<u16, false>(pixels_dst, pixels_src);
						break;
					case 4:
						if (pack_unpack_swap_bytes)
							do_memory_transfer<u32, true>(pixels_dst, pixels_src);
						else
							do_memory_transfer<u32, false>(pixels_dst, pixels_src);
						break;
					case 8:
						if (pack_unpack_swap_bytes)
							do_memory_transfer<u64, true>(pixels_dst, pixels_src);
						else
							do_memory_transfer<u64, false>(pixels_dst, pixels_src);
						break;
					case 16:
						if (pack_unpack_swap_bytes)
							do_memory_transfer<u128, true>(pixels_dst, pixels_src);
						else
							do_memory_transfer<u128, false>(pixels_dst, pixels_src);
						break;
					}
				}
			}
			else
			{
				//Scale image to fit
				//usually we can just get away with nearest filtering
				u8 samples_u = 1, samples_v = 1;
				switch (static_cast<vk::render_target*>(vram_texture)->aa_mode)
				{
				case rsx::surface_antialiasing::diagonal_centered_2_samples:
					samples_u = 2;
					break;
				case rsx::surface_antialiasing::square_centered_4_samples:
				case rsx::surface_antialiasing::square_rotated_4_samples:
					samples_u = 2;
					samples_v = 2;
					break;
				}

				rsx::scale_image_nearest(pixels_dst, pixels_src, width, height, rsx_pitch, real_pitch, bpp, samples_u, samples_v, pack_unpack_swap_bytes);

				switch (vram_texture->info.format)
				{
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
					rsx::convert_le_f32_to_be_d24(pixels_dst, pixels_dst, cpu_address_range >> 2, 1);
					break;
				case VK_FORMAT_D24_UNORM_S8_UINT:
					rsx::convert_le_d24x8_to_be_d24x8(pixels_dst, pixels_dst, cpu_address_range >> 2, 1);
					break;
				}
			}

			dma_buffer->unmap();
			reset_write_statistics();

			//Its highly likely that this surface will be reused, so we just leave resources in place
			return result;
		}

		void set_unpack_swap_bytes(bool swap_bytes)
		{
			pack_unpack_swap_bytes = swap_bytes;
		}

		void reprotect(utils::protection prot)
		{
			//Reset properties and protect again
			flushed = false;
			synchronized = false;
			sync_timestamp = 0ull;

			protect(prot);
		}

		void invalidate_cached()
		{
			synchronized = false;
		}

		bool is_synchronized() const
		{
			return synchronized;
		}

		bool sync_valid() const
		{
			return (sync_timestamp > last_use_timestamp);
		}

		bool has_compatible_format(vk::image* tex) const
		{
			return vram_texture->info.format == tex->info.format;
		}

		bool is_depth_texture() const
		{
			switch (vram_texture->info.format)
			{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
				return true;
			default:
				return false;
			}
		}

		u64 get_sync_timestamp() const
		{
			return sync_timestamp;
		}
	};
	
	struct discarded_storage
	{
		std::unique_ptr<vk::image_view> view;
		std::unique_ptr<vk::image> img;

		//Memory held by this temp storage object
		u32 block_size = 0;

		//Frame id tag
		const u64 frame_tag = vk::get_current_frame_id();

		discarded_storage(std::unique_ptr<vk::image_view>& _view)
		{
			view = std::move(_view);
		}

		discarded_storage(std::unique_ptr<vk::image>& _img)
		{
			img = std::move(_img);
		}

		discarded_storage(std::unique_ptr<vk::image>& _img, std::unique_ptr<vk::image_view>& _view)
		{
			img = std::move(_img);
			view = std::move(_view);
		}

		discarded_storage(cached_texture_section& tex)
		{
			view = std::move(tex.get_view());
			img = std::move(tex.get_texture());
			block_size = tex.get_section_size();
		}

		const bool test(u64 ref_frame) const
		{
			return ref_frame > 0 && frame_tag <= ref_frame;
		}
	};

	class texture_cache : public rsx::texture_cache<vk::command_buffer, cached_texture_section, vk::image*, vk::image_view*, vk::image, VkFormat>
	{
	private:
		//Vulkan internals
		vk::render_device* m_device;
		vk::memory_type_mapping m_memory_types;
		vk::gpu_formats_support m_formats_support;
		VkQueue m_submit_queue;
		vk_data_heap* m_texture_upload_heap;

		//Stuff that has been dereferenced goes into these
		std::list<discarded_storage> m_discardable_storage;
		std::atomic<u32> m_discarded_memory_size = { 0 };

		void purge_cache()
		{
			for (auto &address_range : m_cache)
			{
				auto &range_data = address_range.second;
				for (auto &tex : range_data.data)
				{
					if (tex.exists())
					{
						m_discardable_storage.push_back(tex);
					}

					if (tex.is_locked())
						tex.unprotect();

					tex.release_dma_resources();
				}

				range_data.data.resize(0);
			}

			m_discardable_storage.clear();
			m_unreleased_texture_objects = 0;
			m_texture_memory_in_use = 0;
			m_discarded_memory_size = 0;
		}

		VkComponentMapping apply_component_mapping_flags(u32 gcm_format, rsx::texture_create_flags flags, const texture_channel_remap_t& remap_vector)
		{
			//NOTE: Depth textures should always read RRRR
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			{
				return{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
			}
			default:
				break;
			}

			VkComponentMapping mapping = {};
			switch (flags)
			{
			case rsx::texture_create_flags::default_component_order:
			{
				mapping = vk::apply_swizzle_remap(vk::get_component_mapping(gcm_format), remap_vector);
				break;
			}
			case rsx::texture_create_flags::native_component_order:
			{
				mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
				break;
			}
			case rsx::texture_create_flags::swapped_native_component_order:
			{
				mapping = { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B };
				break;
			}
			default:
				break;
			}

			return mapping;
		}
		
	protected:

		void free_texture_section(cached_texture_section& tex) override
		{
			m_discarded_memory_size += tex.get_section_size();
			m_discardable_storage.push_back(tex);
			tex.destroy();
		}

		vk::image_view* create_temporary_subresource_view_impl(vk::command_buffer& cmd, vk::image* source, VkImageType image_type, VkImageViewType view_type,
			u32 gcm_format, u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector, bool copy)
		{
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

			switch (source->info.format)
			{
			case VK_FORMAT_D16_UNORM:
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			}

			VkFormat dst_format = vk::get_compatible_sampler_format(gcm_format);
			if (aspect & VK_IMAGE_ASPECT_DEPTH_BIT ||
				vk::get_format_texel_width(dst_format) != vk::get_format_texel_width(source->info.format))
			{
				dst_format = source->info.format;
			}

			VkImageSubresourceRange subresource_range = { aspect, 0, 1, 0, 1 };

			std::unique_ptr<vk::image> image;
			std::unique_ptr<vk::image_view> view;

			image.reset(new vk::image(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				dst_format,
				w, h, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, source->info.flags));

			//This method is almost exclusively used to work on framebuffer resources
			//Keep the original swizzle layout unless there is data format conversion
			VkComponentMapping view_swizzle = source->native_component_map;
			if (dst_format != source->info.format)
			{
				//This is a data cast operation
				//Use native mapping for the new type
				//TODO: Also simlulate the readback+reupload step (very tricky)
				const auto remap = get_component_mapping(gcm_format);
				view_swizzle = { remap[1], remap[2], remap[3], remap[0] };
			}

			if (memcmp(remap_vector.first.data(), rsx::default_remap_vector.first.data(), 4) ||
				memcmp(remap_vector.second.data(), rsx::default_remap_vector.second.data(), 4))
				view_swizzle = vk::apply_swizzle_remap({view_swizzle.a, view_swizzle.r, view_swizzle.g, view_swizzle.b}, remap_vector);

			VkImageSubresourceRange view_range = { aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 1 };
			view.reset(new vk::image_view(*vk::get_current_renderer(), image->value, view_type, dst_format, view_swizzle, view_range));

			if (copy)
			{
				VkImageLayout old_src_layout = source->current_layout;

				vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);
				vk::change_image_layout(cmd, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

				VkImageCopy copy_rgn;
				copy_rgn.srcOffset = { (s32)x, (s32)y, 0 };
				copy_rgn.dstOffset = { (s32)0, (s32)0, 0 };
				copy_rgn.dstSubresource = { aspect, 0, 0, 1 };
				copy_rgn.srcSubresource = { aspect, 0, 0, 1 };
				copy_rgn.extent = { w, h, 1 };

				vkCmdCopyImage(cmd, source->value, source->current_layout, image->value, image->current_layout, 1, &copy_rgn);
				vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);
				vk::change_image_layout(cmd, source, old_src_layout, subresource_range);
			}

			const u32 resource_memory = w * h * 4; //Rough approximate
			m_discardable_storage.push_back({ image, view });
			m_discardable_storage.back().block_size = resource_memory;
			m_discarded_memory_size += resource_memory;

			return m_discardable_storage.back().view.get();
		}

		vk::image_view* create_temporary_subresource_view(vk::command_buffer& cmd, vk::image* source, u32 gcm_format,
				u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_view_impl(cmd, source, source->info.imageType, VK_IMAGE_VIEW_TYPE_2D,
					gcm_format, x, y, w, h, remap_vector, true);
		}

		vk::image_view* create_temporary_subresource_view(vk::command_buffer& cmd, vk::image** source, u32 gcm_format,
				u16 x, u16 y, u16 w, u16 h, const texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_view(cmd, *source, gcm_format, x, y, w, h, remap_vector);
		}

		vk::image_view* generate_cubemap_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 size,
				const std::vector<copy_region_descriptor>& sections_to_copy, const texture_channel_remap_t& /*remap_vector*/) override
		{
			std::unique_ptr<vk::image> image;
			std::unique_ptr<vk::image_view> view;

			VkImageAspectFlags dst_aspect;
			VkFormat dst_format = vk::get_compatible_sampler_format(gcm_format);

			image.reset(new vk::image(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				dst_format,
				size, size, 1, 1, 6, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT));

			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH16:
				dst_aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case CELL_GCM_TEXTURE_DEPTH24_D8:
				dst_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			default:
				dst_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
				break;
			}

			VkImageSubresourceRange view_range = { dst_aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 6 };
			view.reset(new vk::image_view(*vk::get_current_renderer(), image->value, VK_IMAGE_VIEW_TYPE_CUBE, image->info.format, image->native_component_map, view_range));

			VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 6 };
			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

			if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
			{
				VkClearColorValue clear = {};
				vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
			}
			else
			{
				VkClearDepthStencilValue clear = { 1.f, 0 };
				vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
			}

			for (const auto &section : sections_to_copy)
			{
				if (section.src)
				{
					VkImageAspectFlags src_aspect;
					switch (section.src->info.format)
					{
					case VK_FORMAT_D16_UNORM:
						src_aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
						break;
					case VK_FORMAT_D24_UNORM_S8_UINT:
					case VK_FORMAT_D32_SFLOAT_S8_UINT:
						src_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
						break;
					default:
						src_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
						break;
					}

					VkImageSubresourceRange src_range = { src_aspect, 0, 1, 0, 1 };
					VkImageLayout old_src_layout = section.src->current_layout;
					vk::change_image_layout(cmd, section.src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src_range);

					VkImageCopy copy_rgn;
					copy_rgn.srcOffset = { section.src_x, section.src_y, 0 };
					copy_rgn.dstOffset = { section.dst_x, section.dst_y, 0 };
					copy_rgn.dstSubresource = { dst_aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, section.dst_z, 1 };
					copy_rgn.srcSubresource = { src_aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1 };
					copy_rgn.extent = { section.w, section.h, 1 };

					vkCmdCopyImage(cmd, section.src->value, section.src->current_layout, image->value, image->current_layout, 1, &copy_rgn);
					vk::change_image_layout(cmd, section.src, old_src_layout, src_range);
				}
			}

			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);

			const u32 resource_memory = size * size * 6 * 4; //Rough approximate
			m_discardable_storage.push_back({ image, view });
			m_discardable_storage.back().block_size = resource_memory;
			m_discarded_memory_size += resource_memory;

			return m_discardable_storage.back().view.get();
		}

		vk::image_view* generate_3d_from_2d_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height, u16 depth,
			const std::vector<copy_region_descriptor>& sections_to_copy, const texture_channel_remap_t& /*remap_vector*/) override
		{
			std::unique_ptr<vk::image> image;
			std::unique_ptr<vk::image_view> view;

			VkImageAspectFlags dst_aspect;
			VkFormat dst_format = vk::get_compatible_sampler_format(gcm_format);

			image.reset(new vk::image(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_3D,
				dst_format,
				width, height, depth, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0));

			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH16:
				dst_aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case CELL_GCM_TEXTURE_DEPTH24_D8:
				dst_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			default:
				dst_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
				break;
			}

			VkImageSubresourceRange view_range = { dst_aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 1 };
			view.reset(new vk::image_view(*vk::get_current_renderer(), image->value, VK_IMAGE_VIEW_TYPE_3D, image->info.format, image->native_component_map, view_range));

			VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 1 };
			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

			if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
			{
				VkClearColorValue clear = {};
				vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
			}
			else
			{
				VkClearDepthStencilValue clear = { 1.f, 0 };
				vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
			}

			for (const auto &section : sections_to_copy)
			{
				if (section.src)
				{
					VkImageAspectFlags src_aspect;
					switch (section.src->info.format)
					{
					case VK_FORMAT_D16_UNORM:
						src_aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
						break;
					case VK_FORMAT_D24_UNORM_S8_UINT:
					case VK_FORMAT_D32_SFLOAT_S8_UINT:
						src_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
						break;
					default:
						src_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
						break;
					}

					VkImageSubresourceRange src_range = { src_aspect, 0, 1, 0, 1 };
					VkImageLayout old_src_layout = section.src->current_layout;
					vk::change_image_layout(cmd, section.src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src_range);

					VkImageCopy copy_rgn;
					copy_rgn.srcOffset = { section.src_x, section.src_y, 0 };
					copy_rgn.dstOffset = { section.dst_x, section.dst_y, section.dst_z };
					copy_rgn.dstSubresource = { dst_aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1 };
					copy_rgn.srcSubresource = { src_aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1 };
					copy_rgn.extent = { section.w, section.h, 1 };

					vkCmdCopyImage(cmd, section.src->value, section.src->current_layout, image->value, image->current_layout, 1, &copy_rgn);
					vk::change_image_layout(cmd, section.src, old_src_layout, src_range);
				}
			}

			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);

			const u32 resource_memory = width * height * depth * 4; //Rough approximate
			m_discardable_storage.push_back({ image, view });
			m_discardable_storage.back().block_size = resource_memory;
			m_discarded_memory_size += resource_memory;

			return m_discardable_storage.back().view.get();
		}

		vk::image_view* generate_atlas_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
				const std::vector<copy_region_descriptor>& sections_to_copy, const texture_channel_remap_t& remap_vector) override
		{
			auto result = create_temporary_subresource_view_impl(cmd, sections_to_copy.front().src, VK_IMAGE_TYPE_2D,
					VK_IMAGE_VIEW_TYPE_2D, gcm_format, 0, 0, width, height, remap_vector, false);

			VkImage dst = result->info.image;
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

			switch (sections_to_copy.front().src->info.format)
			{
			case VK_FORMAT_D16_UNORM:
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			}

			VkImageSubresourceRange subresource_range = { aspect, 0, 1, 0, 1 };
			vk::change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

			for (const auto &region : sections_to_copy)
			{
				VkImageLayout old_src_layout = region.src->current_layout;
				vk::change_image_layout(cmd, region.src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

				VkImageCopy copy_rgn;
				copy_rgn.srcOffset = { region.src_x, region.src_y, 0 };
				copy_rgn.dstOffset = { region.dst_x, region.dst_y, 0 };
				copy_rgn.dstSubresource = { aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1 };
				copy_rgn.srcSubresource = { aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1 };
				copy_rgn.extent = { region.w, region.h, 1 };

				vkCmdCopyImage(cmd, region.src->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &copy_rgn);

				vk::change_image_layout(cmd, region.src, old_src_layout, subresource_range);
			}

			vk::change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);
			return result;
		}

		void update_image_contents(vk::command_buffer& cmd, vk::image_view* dst_view, vk::image* src, u16 width, u16 height) override
		{
			VkImage dst = dst_view->info.image;
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

			switch (src->info.format)
			{
			case VK_FORMAT_D16_UNORM:
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			}

			VkImageSubresourceRange subresource_range = { aspect, 0, 1, 0, 1 };
			vk::change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

			VkImageLayout old_src_layout = src->current_layout;
			vk::change_image_layout(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

			VkImageCopy copy_rgn;
			copy_rgn.srcOffset = { 0, 0, 0 };
			copy_rgn.dstOffset = { 0, 0, 0 };
			copy_rgn.dstSubresource = { aspect & ~(VK_IMAGE_ASPECT_DEPTH_BIT), 0, 0, 1 };
			copy_rgn.srcSubresource = { aspect & ~(VK_IMAGE_ASPECT_DEPTH_BIT), 0, 0, 1 };
			copy_rgn.extent = { width, height, 1 };

			vkCmdCopyImage(cmd, src->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_rgn);

			vk::change_image_layout(cmd, src, old_src_layout, subresource_range);
			vk::change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);
		}

		cached_texture_section* create_new_texture(vk::command_buffer& cmd, u32 rsx_address, u32 rsx_size, u16 width, u16 height, u16 depth, u16 mipmaps, u32 gcm_format,
				rsx::texture_upload_context context, rsx::texture_dimension_extended type, rsx::texture_create_flags flags,
				rsx::texture_colorspace colorspace, const texture_channel_remap_t& remap_vector) override
		{
			const u16 section_depth = depth;
			const bool is_cubemap = type == rsx::texture_dimension_extended::texture_dimension_cubemap;
			VkFormat vk_format;
			VkComponentMapping mapping;
			VkImageAspectFlags aspect_flags;
			VkImageType image_type;
			VkImageViewType image_view_type;
			VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			u8 layer = 0;

			switch (type)
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				image_type = VK_IMAGE_TYPE_1D;
				image_view_type = VK_IMAGE_VIEW_TYPE_1D;
				height = 1;
				depth = 1;
				layer = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				image_type = VK_IMAGE_TYPE_2D;
				image_view_type = VK_IMAGE_VIEW_TYPE_2D;
				depth = 1;
				layer = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				image_type = VK_IMAGE_TYPE_2D;
				image_view_type = VK_IMAGE_VIEW_TYPE_CUBE;
				depth = 1;
				layer = 6;
				break;
			case rsx::texture_dimension_extended::texture_dimension_3d:
				image_type = VK_IMAGE_TYPE_3D;
				image_view_type = VK_IMAGE_VIEW_TYPE_3D;
				layer = 1;
				break;
			}

			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
				aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				vk_format = m_formats_support.d24_unorm_s8? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D32_SFLOAT_S8_UINT;
				break;
			case CELL_GCM_TEXTURE_DEPTH16:
				aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
				usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				vk_format = VK_FORMAT_D16_UNORM;
				break;
			default:
				aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
				vk_format = get_compatible_sampler_format(gcm_format);

				if (colorspace != rsx::texture_colorspace::rgb_linear)
					vk_format = get_compatible_srgb_format(vk_format);
				break;
			}

			vk::image *image = new vk::image(*m_device, m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				vk_format,
				width, height, depth, mipmaps, layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, usage_flags, is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0);

			mapping = apply_component_mapping_flags(gcm_format, flags, remap_vector);

			vk::image_view *view = new vk::image_view(*m_device, image->value, image_view_type, vk_format,
				mapping, { (aspect_flags & ~VK_IMAGE_ASPECT_STENCIL_BIT), 0, mipmaps, 0, layer});

			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { aspect_flags, 0, mipmaps, 0, layer });

			cached_texture_section& region = find_cached_texture(rsx_address, rsx_size, true, width, height, section_depth);
			region.reset(rsx_address, rsx_size);
			region.create(width, height, section_depth, mipmaps, view, image, 0, true, gcm_format);
			region.set_dirty(false);
			region.set_context(context);
			region.set_sampler_status(rsx::texture_sampler_status::status_uninitialized);
			region.set_image_type(type);

			//Its not necessary to lock blit dst textures as they are just reused as necessary
			if (context != rsx::texture_upload_context::blit_engine_dst)
			{
				region.protect(utils::protection::ro);
				read_only_range = region.get_min_max(read_only_range);
			}
			else
			{
				//TODO: Confirm byte swap patterns
				region.protect(utils::protection::no);
				region.set_unpack_swap_bytes((aspect_flags & VK_IMAGE_ASPECT_COLOR_BIT) == VK_IMAGE_ASPECT_COLOR_BIT);
				no_access_range = region.get_min_max(no_access_range);
			}

			update_cache_tag();
			return &region;
		}

		cached_texture_section* upload_image_from_cpu(vk::command_buffer& cmd, u32 rsx_address, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format,
			rsx::texture_upload_context context, const std::vector<rsx_subresource_layout>& subresource_layout, rsx::texture_dimension_extended type,
			rsx::texture_colorspace colorspace, bool swizzled, const texture_channel_remap_t& remap_vector) override
		{
			auto section = create_new_texture(cmd, rsx_address, pitch * height, width, height, depth, mipmaps, gcm_format, context, type,
					rsx::texture_create_flags::default_component_order, colorspace, remap_vector);

			auto image = section->get_raw_texture();
			auto subres_range = section->get_raw_view()->info.subresourceRange;

			switch (image->info.format)
			{
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
				subres_range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			}

			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subres_range);

			vk::enter_uninterruptible();

			bool input_swizzled = swizzled;
			if (context == rsx::texture_upload_context::blit_engine_src)
			{
				//Swizzling is ignored for blit engine copy and emulated using remapping
				input_swizzled = false;
				section->set_sampler_status(rsx::texture_sampler_status::status_uninitialized);
			}
			else
			{
				//Generic upload - sampler status will be set on upload
				section->set_sampler_status(rsx::texture_sampler_status::status_ready);
			}

			vk::copy_mipmaped_image_using_buffer(cmd, image->value, subresource_layout, gcm_format, input_swizzled, mipmaps, subres_range.aspectMask,
				*m_texture_upload_heap);

			vk::leave_uninterruptible();

			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subres_range);

			return section;
		}

		void enforce_surface_creation_type(cached_texture_section& section, u32 gcm_format, rsx::texture_create_flags expected_flags) override
		{
			if (expected_flags == section.get_view_flags())
				return;

			vk::image* image = section.get_raw_texture();
			auto& view = section.get_view();

			VkComponentMapping mapping = apply_component_mapping_flags(gcm_format, expected_flags, rsx::default_remap_vector);

			if (mapping.a != view->info.components.a ||
				mapping.b != view->info.components.b ||
				mapping.g != view->info.components.g ||
				mapping.r != view->info.components.r)
			{
				//Replace view map
				vk::image_view *new_view = new vk::image_view(*m_device, image->value, view->info.viewType, view->info.format,
					mapping, view->info.subresourceRange);

				view.reset(new_view);
			}

			section.set_view_flags(expected_flags);
			section.set_sampler_status(rsx::texture_sampler_status::status_uninitialized);
		}

		void set_up_remap_vector(cached_texture_section& section, const texture_channel_remap_t& remap_vector) override
		{
			auto& view = section.get_view();
			auto& original_remap = view->info.components;
			std::array<VkComponentSwizzle, 4> base_remap = {original_remap.a, original_remap.r, original_remap.g, original_remap.b};

			auto final_remap = vk::apply_swizzle_remap(base_remap, remap_vector);
			if (final_remap.a != original_remap.a ||
				final_remap.r != original_remap.r ||
				final_remap.g != original_remap.g ||
				final_remap.b != original_remap.b)
			{
				vk::image_view *new_view = new vk::image_view(*m_device, view->info.image, view->info.viewType, view->info.format,
					final_remap, view->info.subresourceRange);

				view.reset(new_view);
			}
			section.set_sampler_status(rsx::texture_sampler_status::status_ready);
		}

		void insert_texture_barrier(vk::command_buffer& cmd, vk::image* tex) override
		{
			vk::insert_texture_barrier(cmd, tex);
		}

		bool render_target_format_is_compatible(vk::image* tex, u32 gcm_format) override
		{
			auto vk_format = tex->info.format;
			switch (gcm_format)
			{
			default:
				//TODO
				err_once("Format incompatibility detected, reporting failure to force data copy (VK_FORMAT=0x%X, GCM_FORMAT=0x%X)", (u32)vk_format, gcm_format);
				return false;
			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
				return (vk_format == VK_FORMAT_R16G16B16A16_SFLOAT);
			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
				return (vk_format == VK_FORMAT_R32G32B32A32_SFLOAT);
			case CELL_GCM_TEXTURE_X32_FLOAT:
				return (vk_format == VK_FORMAT_R32_SFLOAT);
			case CELL_GCM_TEXTURE_R5G6B5:
				return (vk_format == VK_FORMAT_R5G6B5_UNORM_PACK16);
			case CELL_GCM_TEXTURE_A8R8G8B8:
				return (vk_format == VK_FORMAT_B8G8R8A8_UNORM || vk_format == VK_FORMAT_D24_UNORM_S8_UINT || vk_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
			case CELL_GCM_TEXTURE_B8:
				return (vk_format == VK_FORMAT_R8_UNORM);
			case CELL_GCM_TEXTURE_G8B8:
				return (vk_format == VK_FORMAT_R8G8_UNORM);
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
				return (vk_format == VK_FORMAT_D24_UNORM_S8_UINT || vk_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				return (vk_format == VK_FORMAT_D16_UNORM);
			}
		}

	public:

		struct vk_blit_op_result : public blit_op_result
		{
			bool deferred = false;
			vk::image *src_image = nullptr;
			vk::image *dst_image = nullptr;
			vk::image_view *src_view = nullptr;

			using blit_op_result::blit_op_result;
		};

	public:

		void initialize(vk::render_device& device, vk::memory_type_mapping& memory_types, vk::gpu_formats_support& formats_support,
					VkQueue submit_queue, vk::vk_data_heap& upload_heap)
		{
			m_memory_types = memory_types;
			m_formats_support = formats_support;
			m_device = &device;
			m_submit_queue = submit_queue;
			m_texture_upload_heap = &upload_heap;
		}

		void destroy() override
		{
			purge_cache();
		}

		bool is_depth_texture(u32 rsx_address, u32 rsx_size) override
		{
			reader_lock lock(m_cache_mutex);

			auto found = m_cache.find(get_block_address(rsx_address));
			if (found == m_cache.end())
				return false;

			if (found->second.valid_count == 0)
				return false;

			for (auto& tex : found->second.data)
			{
				if (tex.is_dirty())
					continue;

				if (!tex.overlaps(rsx_address, true))
					continue;

				if ((rsx_address + rsx_size - tex.get_section_base()) <= tex.get_section_size())
				{
					switch (tex.get_format())
					{
					case VK_FORMAT_D16_UNORM:
					case VK_FORMAT_D32_SFLOAT_S8_UINT:
					case VK_FORMAT_D24_UNORM_S8_UINT:
						return true;
					default:
						return false;
					}
				}
			}

			//Unreachable; silence compiler warning anyway
			return false;
		}

		void on_frame_end() override
		{
			if (m_unreleased_texture_objects >= m_max_zombie_objects ||
				m_discarded_memory_size > 0x4000000) //If already holding over 64M in discardable memory, be frugal with memory resources
			{
				purge_dirty();
			}

			const u64 last_complete_frame = vk::get_last_completed_frame_id();
			m_discardable_storage.remove_if([&](const discarded_storage& o)
			{
				if (o.test(last_complete_frame))
				{
					m_discarded_memory_size -= o.block_size;
					return true;
				}
				return false;
			});

			m_temporary_subresource_cache.clear();
			reset_frame_statistics();
		}

		template<typename RsxTextureType>
		sampled_image_descriptor _upload_texture(vk::command_buffer& cmd, RsxTextureType& tex, rsx::vk_render_targets& m_rtts)
		{
			return upload_texture(cmd, tex, m_rtts, cmd, m_memory_types, const_cast<const VkQueue>(m_submit_queue));
		}

		vk::image *upload_image_simple(vk::command_buffer& /*cmd*/, u32 address, u32 width, u32 height)
		{
			//Uploads a linear memory range as a BGRA8 texture
			auto image = std::make_unique<vk::image>(*m_device, m_memory_types.host_visible_coherent,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_IMAGE_TYPE_2D,
				VK_FORMAT_B8G8R8A8_UNORM,
				width, height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0);

			VkImageSubresource subresource{};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkSubresourceLayout layout{};
			vkGetImageSubresourceLayout(*m_device, image->value, &subresource, &layout);

			void* mem = nullptr;
			vkMapMemory(*m_device, image->memory->memory, 0, layout.rowPitch * height, 0, &mem);

			u32 row_pitch = width * 4;
			char *src = (char *)vm::base(address);
			char *dst = (char *)mem;

			//TODO: SSE optimization
			for (u32 row = 0; row < height; ++row)
			{
				be_t<u32>* casted_src = (be_t<u32>*)src;
				u32* casted_dst = (u32*)dst;

				for (u32 col = 0; col < width; ++col)
					casted_dst[col] = casted_src[col];

				src += row_pitch;
				dst += layout.rowPitch;
			}

			vkUnmapMemory(*m_device, image->memory->memory);

			auto result = image.get();
			const u32 resource_memory = width * height * 4; //Rough approximate
			m_discardable_storage.push_back(image);
			m_discardable_storage.back().block_size = resource_memory;
			m_discarded_memory_size += resource_memory;

			return result;
		}

		vk_blit_op_result blit(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, rsx::vk_render_targets& m_rtts, vk::command_buffer& cmd)
		{
			struct blit_helper
			{
				vk::command_buffer* commands;
				blit_helper(vk::command_buffer *c) : commands(c) {}

				bool deferred = false;
				vk::image* deferred_op_src = nullptr;
				vk::image* deferred_op_dst = nullptr;

				void scale_image(vk::image* src, vk::image* dst, areai src_area, areai dst_area, bool /*interpolate*/, bool is_depth)
				{
					VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;
					if (is_depth) aspect = (VkImageAspectFlagBits)(src->info.format == VK_FORMAT_D16_UNORM ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

					//Checks
					if (src_area.x2 <= src_area.x1 || src_area.y2 <= src_area.y1 || dst_area.x2 <= dst_area.x1 || dst_area.y2 <= dst_area.y1)
					{
						LOG_ERROR(RSX, "Blit request consists of an empty region descriptor!");
						return;
					}

					if (src_area.x1 < 0 || src_area.x2 > (s32)src->width() || src_area.y1 < 0 || src_area.y2 > (s32)src->height())
					{
						LOG_ERROR(RSX, "Blit request denied because the source region does not fit!");
						return;
					}

					if (dst_area.x1 < 0 || dst_area.x2 > (s32)dst->width() || dst_area.y1 < 0 || dst_area.y2 > (s32)dst->height())
					{
						LOG_ERROR(RSX, "Blit request denied because the destination region does not fit!");
						return;
					}

					const auto src_width = src_area.x2 - src_area.x1;
					const auto src_height = src_area.y2 - src_area.y1;
					const auto dst_width = dst_area.x2 - dst_area.x1;
					const auto dst_height = dst_area.y2 - dst_area.y1;

					deferred_op_src = src;
					deferred_op_dst = dst;

					if (aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
					{
						if (src_width != dst_width || src_height != dst_height || src->info.format != dst->info.format)
						{
							//Scaled depth scaling
							deferred = true;
						}
					}

					if (!deferred)
					{
						copy_scaled_image(*commands, src->value, dst->value, src->current_layout, dst->current_layout, src_area.x1, src_area.y1, src_width, src_height,
							dst_area.x1, dst_area.y1, dst_width, dst_height, 1, aspect, src->info.format == dst->info.format);
					}

					change_image_layout(*commands, dst, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {(VkImageAspectFlags)aspect, 0, dst->info.mipLevels, 0, dst->info.arrayLayers});
				}
			}
			helper(&cmd);

			auto reply = upload_scaled_image(src, dst, interpolate, cmd, m_rtts, helper, cmd, m_memory_types, const_cast<const VkQueue>(m_submit_queue));

			vk_blit_op_result result = reply.succeeded;
			result.real_dst_address = reply.real_dst_address;
			result.real_dst_size = reply.real_dst_size;
			result.is_depth = reply.is_depth;
			result.deferred = helper.deferred;
			result.dst_image = helper.deferred_op_dst;
			result.src_image = helper.deferred_op_src;

			if (!helper.deferred)
				return result;

			VkImageSubresourceRange view_range = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
			auto tmp_view = std::make_unique<vk::image_view>(*vk::get_current_renderer(), helper.deferred_op_src->value, VK_IMAGE_VIEW_TYPE_2D,
					helper.deferred_op_src->info.format, helper.deferred_op_src->native_component_map, view_range);

			result.src_view = tmp_view.get();
			m_discardable_storage.push_back(tmp_view);
			return result;
		}

		const u32 get_unreleased_textures_count() const override
		{
			return m_unreleased_texture_objects + (u32)m_discardable_storage.size();
		}

		const u32 get_texture_memory_in_use() const override
		{
			return m_texture_memory_in_use;
		}

		const u32 get_temporary_memory_in_use()
		{
			return m_discarded_memory_size;
		}
	};
}
