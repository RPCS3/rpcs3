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
		bool synchronized = false;
		bool flushed = false;
		bool pack_unpack_swap_bytes = false;
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

		void create(const u16 w, const u16 h, const u16 depth, const u16 mipmaps, vk::image_view *view, vk::image *image, const u32 rsx_pitch, bool managed, const u32 gcm_format, bool pack_swap_bytes = false)
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
				return static_cast<vk::render_target*>(vram_texture)->get_view();
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

			const u16 internal_width = std::min(width, rsx::apply_resolution_scale(width, true));
			const u16 internal_height = std::min(height, rsx::apply_resolution_scale(height, true));

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

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = internal_width;
			copyRegion.bufferImageHeight = internal_height;
			copyRegion.imageSubresource = {aspect_flag, 0, 0, 1};
			copyRegion.imageOffset = {};
			copyRegion.imageExtent = {internal_width, internal_height, 1};

			VkImageSubresourceRange subresource_range = { aspect_flag & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 1 };
			
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
				CHECK_RESULT(vkResetFences(*m_device, 1, &dma_fence));

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
					auto typed_dst = (T *)pixels_dst;
					auto typed_src = (T *)pixels_src;

					for (u32 px = 0; px < block_size; ++px)
						typed_dst[px] = typed_src[px];
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
			}

			dma_buffer->unmap();
			//Its highly likely that this surface will be reused, so we just leave resources in place

			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
				rsx::shuffle_texel_data_wzyx<u16>(pixels_dst, rsx_pitch, width, height);
				break;
			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
				rsx::shuffle_texel_data_wzyx<u32>(pixels_dst, rsx_pitch, width, height);
				break;
			}

			return result;
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
		vk::buffer* m_texture_upload_buffer;

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
		
	protected:

		void free_texture_section(cached_texture_section& tex) override
		{
			m_discarded_memory_size += tex.get_section_size();
			m_discardable_storage.push_back(tex);
			tex.destroy();
		}

		vk::image_view* create_temporary_subresource_view_impl(vk::command_buffer& cmd, vk::image* source, VkImageType image_type, VkImageViewType view_type, u32 gcm_format, u16 x, u16 y, u16 w, u16 h)
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

			VkImageSubresourceRange view_range = { aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 1 };
			view.reset(new vk::image_view(*vk::get_current_renderer(), image->value, view_type, dst_format, source->native_component_map, view_range));

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

			const u32 resource_memory = w * h * 4; //Rough approximate
			m_discardable_storage.push_back({ image, view });
			m_discardable_storage.back().block_size = resource_memory;
			m_discarded_memory_size += resource_memory;

			return m_discardable_storage.back().view.get();
		}

		vk::image_view* create_temporary_subresource_view(vk::command_buffer& cmd, vk::image* source, u32 gcm_format, u16 x, u16 y, u16 w, u16 h) override
		{
			return create_temporary_subresource_view_impl(cmd, source, source->info.imageType, VK_IMAGE_VIEW_TYPE_2D, gcm_format, x, y, w, h);
		}

		vk::image_view* create_temporary_subresource_view(vk::command_buffer& cmd, vk::image** source, u32 gcm_format, u16 x, u16 y, u16 w, u16 h) override
		{
			return create_temporary_subresource_view(cmd, *source, gcm_format, x, y, w, h);
		}

		vk::image_view* generate_cubemap_from_images(vk::command_buffer& cmd, const u32 gcm_format, u16 size, std::array<vk::image*, 6>& sources) override
		{
			std::unique_ptr<vk::image> image;
			std::unique_ptr<vk::image_view> view;

			image.reset(new vk::image(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				vk::get_compatible_sampler_format(gcm_format),
				size, size, 1, 1, 6, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT));

			VkImageSubresourceRange subresource_range = {};
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

			for (u32 n = 0; n < 6; ++n)
			{
				if (!view)
				{
					switch (sources[0]->info.format)
					{
					case VK_FORMAT_D16_UNORM:
						aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
						break;
					case VK_FORMAT_D24_UNORM_S8_UINT:
					case VK_FORMAT_D32_SFLOAT_S8_UINT:
						aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
						break;
					}

					VkImageSubresourceRange view_range = { aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 6 };
					view.reset(new vk::image_view(*vk::get_current_renderer(), image->value, VK_IMAGE_VIEW_TYPE_CUBE, image->info.format, image->native_component_map, view_range));
					subresource_range = view_range;
					vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);
					subresource_range.layerCount = 1;
				}

				if (sources[n])
				{
					VkImageLayout old_src_layout = sources[n]->current_layout;
					vk::change_image_layout(cmd, sources[n], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

					VkImageCopy copy_rgn;
					copy_rgn.srcOffset = { 0, 0, 0 };
					copy_rgn.dstOffset = { 0, 0, 0 };
					copy_rgn.dstSubresource = { aspect, 0, n, 1 };
					copy_rgn.srcSubresource = { aspect, 0, 0, 1 };
					copy_rgn.extent = { size, size, 1 };

					vkCmdCopyImage(cmd, sources[n]->value, sources[n]->current_layout, image->value, image->current_layout, 1, &copy_rgn);
					vk::change_image_layout(cmd, sources[n], old_src_layout, subresource_range);
				}
				else
				{
					//Clear to black
					VkClearColorValue clear_color{};
					auto range = subresource_range;
					range.baseArrayLayer = n;
					vkCmdClearColorImage(cmd, image->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);
				}
			}

			subresource_range.layerCount = 6;
			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);

			const u32 resource_memory = size * size * 6 * 4; //Rough approximate
			m_discardable_storage.push_back({ image, view });
			m_discardable_storage.back().block_size = resource_memory;
			m_discarded_memory_size += resource_memory;

			return m_discardable_storage.back().view.get();
		}

		cached_texture_section* create_new_texture(vk::command_buffer& cmd, u32 rsx_address, u32 rsx_size, u16 width, u16 height, u16 depth, u16 mipmaps, const u32 gcm_format,
				const rsx::texture_upload_context context, const rsx::texture_dimension_extended type, const rsx::texture_create_flags flags,
				std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap_vector) override
		{
			const u16 section_depth = depth;
			const bool is_cubemap = type == rsx::texture_dimension_extended::texture_dimension_cubemap;
			VkFormat vk_format;
			VkComponentMapping mapping;
			VkImageAspectFlags aspect_flags;
			VkImageType image_type;
			VkImageViewType image_view_type;
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
				vk_format = m_formats_support.d24_unorm_s8? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D32_SFLOAT_S8_UINT;
				break;
			case CELL_GCM_TEXTURE_DEPTH16:
				aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
				vk_format = VK_FORMAT_D16_UNORM;
				break;
			default:
				aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
				vk_format = get_compatible_sampler_format(gcm_format);
				break;
			}

			vk::image *image = new vk::image(*m_device, m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				vk_format,
				width, height, depth, mipmaps, layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0);

			switch (flags)
			{
			case rsx::texture_create_flags::default_component_order:
			{
				auto native_mapping = vk::get_component_mapping(gcm_format);
				VkComponentSwizzle final_mapping[4] = {};

				for (u8 channel = 0; channel < 4; ++channel)
				{
					switch (remap_vector.second[channel])
					{
					case CELL_GCM_TEXTURE_REMAP_ONE:
						final_mapping[channel] = VK_COMPONENT_SWIZZLE_ONE;
						break;
					case CELL_GCM_TEXTURE_REMAP_ZERO:
						final_mapping[channel] = VK_COMPONENT_SWIZZLE_ZERO;
						break;
					case CELL_GCM_TEXTURE_REMAP_REMAP:
						final_mapping[channel] = native_mapping[remap_vector.first[channel]];
						break;
					default:
						LOG_ERROR(RSX, "Unknown remap lookup value %d", remap_vector.second[channel]);
					}
				}

				mapping = { final_mapping[1], final_mapping[2], final_mapping[3], final_mapping[0] };
				break;
			}
			case rsx::texture_create_flags::native_component_order:
				mapping = image->native_component_map;
				break;
			case rsx::texture_create_flags::swapped_native_component_order:
				mapping = {VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A};
				break;
			default:
				fmt::throw_exception("Unknown create flags 0x%X", (u32)flags);
			}

			vk::image_view *view = new vk::image_view(*m_device, image->value, image_view_type, vk_format,
				mapping, { (aspect_flags & ~VK_IMAGE_ASPECT_STENCIL_BIT), 0, mipmaps, 0, layer});

			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { aspect_flags, 0, mipmaps, 0, layer });

			cached_texture_section& region = find_cached_texture(rsx_address, rsx_size, true, width, height, section_depth);
			region.reset(rsx_address, rsx_size);
			region.create(width, height, section_depth, mipmaps, view, image, 0, true, gcm_format);
			region.set_dirty(false);
			region.set_context(context);
			region.set_image_type(type);

			//Its not necessary to lock blit dst textures as they are just reused as necessary
			if (context != rsx::texture_upload_context::blit_engine_dst || g_cfg.video.strict_rendering_mode)
			{
				region.protect(utils::protection::ro);
				update_cache_tag();
			}

			read_only_range = region.get_min_max(read_only_range);
			return &region;
		}

		cached_texture_section* upload_image_from_cpu(vk::command_buffer& cmd, u32 rsx_address, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, const u32 gcm_format,
			const rsx::texture_upload_context context, std::vector<rsx_subresource_layout>& subresource_layout, const rsx::texture_dimension_extended type, const bool swizzled,
			std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap_vector) override
		{
			auto section = create_new_texture(cmd, rsx_address, pitch * height, width, height, depth, mipmaps, gcm_format, context, type,
					rsx::texture_create_flags::default_component_order, remap_vector);

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

			//Swizzling is ignored for blit engine copy and emulated using a swapped order image view
			bool input_swizzled = (context == rsx::texture_upload_context::blit_engine_src) ? false : swizzled;

			vk::copy_mipmaped_image_using_buffer(cmd, image->value, subresource_layout, gcm_format, input_swizzled, mipmaps, subres_range.aspectMask,
				*m_texture_upload_heap, m_texture_upload_buffer);

			vk::leave_uninterruptible();

			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subres_range);

			return section;
		}

		void enforce_surface_creation_type(cached_texture_section& section, const rsx::texture_create_flags expected_flags) override
		{
			VkComponentMapping mapping;
			vk::image* image = section.get_raw_texture();
			auto& view = section.get_view();

			switch (expected_flags)
			{
			case rsx::texture_create_flags::native_component_order:
				mapping = image->native_component_map;
				break;
			case rsx::texture_create_flags::swapped_native_component_order:
				mapping = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
				break;
			default:
				return;
			}

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
		}

		void insert_texture_barrier() override
		{}

	public:

		void initialize(vk::render_device& device, vk::memory_type_mapping& memory_types, vk::gpu_formats_support& formats_support,
					VkQueue submit_queue, vk::vk_data_heap& upload_heap, vk::buffer* upload_buffer)
		{
			m_memory_types = memory_types;
			m_formats_support = formats_support;
			m_device = &device;
			m_submit_queue = submit_queue;
			m_texture_upload_heap = &upload_heap;
			m_texture_upload_buffer = upload_buffer;
		}

		void destroy() override
		{
			purge_cache();
		}

		bool is_depth_texture(const u32 rsx_address, const u32 rsx_size) override
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
		}

		template<typename RsxTextureType>
		sampled_image_descriptor _upload_texture(vk::command_buffer& cmd, RsxTextureType& tex, rsx::vk_render_targets& m_rtts)
		{
			return upload_texture(cmd, tex, m_rtts, cmd, m_memory_types, const_cast<const VkQueue>(m_submit_queue));
		}

		bool blit(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, rsx::vk_render_targets& m_rtts, vk::command_buffer& cmd)
		{
			struct blit_helper
			{
				vk::command_buffer* commands;
				blit_helper(vk::command_buffer *c) : commands(c) {}
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

					copy_scaled_image(*commands, src->value, dst->value, src->current_layout, dst->current_layout, src_area.x1, src_area.y1, src_area.x2 - src_area.x1, src_area.y2 - src_area.y1,
						dst_area.x1, dst_area.y1, dst_area.x2 - dst_area.x1, dst_area.y2 - dst_area.y1, 1, aspect, src->info.format == dst->info.format);

					change_image_layout(*commands, dst, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {(VkImageAspectFlags)aspect, 0, dst->info.mipLevels, 0, dst->info.arrayLayers});
				}
			}
			helper(&cmd);

			return upload_scaled_image(src, dst, interpolate, cmd, m_rtts, helper, cmd, m_memory_types, const_cast<const VkQueue>(m_submit_queue));
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
