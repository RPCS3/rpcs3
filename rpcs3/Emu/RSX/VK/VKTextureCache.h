#pragma once
#include "stdafx.h"
#include "VKRenderTargets.h"
#include "VKGSRender.h"
#include "VKCompute.h"
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
		std::unique_ptr<vk::viewable_image> managed_texture = nullptr;

		//DMA relevant data
		VkFence dma_fence = VK_NULL_HANDLE;
		vk::render_device* m_device = nullptr;
		vk::viewable_image *vram_texture = nullptr;
		std::unique_ptr<vk::buffer> dma_buffer;

	public:

		cached_texture_section() {}

		void reset(u32 base, u32 length)
		{
			if (length > cpu_address_range)
				release_dma_resources();

			rsx::cached_texture_section::reset(base, length);
		}

		void create(u16 w, u16 h, u16 depth, u16 mipmaps, vk::image *image, u32 rsx_pitch, bool managed, u32 gcm_format, bool pack_swap_bytes = false)
		{
			width = w;
			height = h;
			this->depth = depth;
			this->mipmaps = mipmaps;

			this->gcm_format = gcm_format;
			this->pack_unpack_swap_bytes = pack_swap_bytes;

			vram_texture = static_cast<vk::viewable_image*>(image);

			if (managed)
			{
				managed_texture.reset(vram_texture);
			}

			//TODO: Properly compute these values
			if (rsx_pitch > 0)
				this->rsx_pitch = rsx_pitch;
			else
				this->rsx_pitch = cpu_address_range / height;

			//Even if we are managing the same vram section, we cannot guarantee contents are static
			//The create method is only invoked when a new mangaged session is required
			synchronized = false;
			flushed = false;
			sync_timestamp = 0ull;
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

		vk::image_view* get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap)
		{
			return vram_texture->get_view(remap_encoding, remap);
		}

		vk::image_view* get_raw_view()
		{
			return vram_texture->get_view(0xAAE4, rsx::default_remap_vector);
		}

		vk::image* get_raw_texture()
		{
			return managed_texture.get();
		}

		std::unique_ptr<vk::viewable_image>& get_texture()
		{
			return managed_texture;
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
			return flushed;
		}

		void copy_texture(bool manage_cb_lifetime, vk::command_buffer& cmd, VkQueue submit_queue)
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
				auto memory_type = m_device->get_memory_mapping().host_visible_coherent;
				dma_buffer.reset(new vk::buffer(*m_device, align(cpu_address_range, 256), memory_type, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0));
			}

			if (manage_cb_lifetime)
			{
				cmd.begin();
			}

			vk::image *target = vram_texture;
			real_pitch = vk::get_format_texel_width(vram_texture->info.format) * vram_texture->width();

			VkImageAspectFlags aspect_flag = vk::get_aspect_flags(vram_texture->info.format);
			VkImageSubresourceRange subresource_range = { aspect_flag, 0, 1, 0, 1 };
			u32 transfer_width = width;
			u32 transfer_height = height;

			VkImageLayout old_layout = vram_texture->current_layout;
			change_image_layout(cmd, vram_texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

			if ((rsx::get_resolution_scale_percent() != 100 && context == rsx::texture_upload_context::framebuffer_storage) ||
				(real_pitch != rsx_pitch))
			{
				if (context == rsx::texture_upload_context::framebuffer_storage)
				{
					switch (static_cast<vk::render_target*>(vram_texture)->read_aa_mode)
					{
					case rsx::surface_antialiasing::center_1_sample:
						break;
					case rsx::surface_antialiasing::diagonal_centered_2_samples:
						transfer_width *= 2;
						break;
					default:
						transfer_width *= 2;
						transfer_height *= 2;
						break;
					}
				}

				if (transfer_width != vram_texture->width() || transfer_height != vram_texture->height())
				{
					// TODO: Synchronize access to typeles textures
					target = vk::get_typeless_helper(vram_texture->info.format);
					change_image_layout(cmd, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

					// Allow bilinear filtering on color textures where compatibility is likely
					const auto filter = (aspect_flag == VK_IMAGE_ASPECT_COLOR_BIT) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

					vk::copy_scaled_image(cmd, vram_texture->value, target->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target->current_layout,
						0, 0, vram_texture->width(), vram_texture->height(), 0, 0, transfer_width, transfer_height, 1, aspect_flag, true, filter,
						vram_texture->info.format, target->info.format);
				}
			}

			if (target->current_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				// Using a scaled intermediary
				verify(HERE), target != vram_texture;
				change_image_layout(cmd, target, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);
			}

			// Handle any format conversions using compute tasks
			vk::cs_shuffle_base *shuffle_kernel = nullptr;

			if (vram_texture->info.format == VK_FORMAT_D24_UNORM_S8_UINT)
			{
				shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_se_d24x8>();
			}
			else if (vram_texture->info.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
			{
				shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_se_f32_d24x8>();
			}
			else if (pack_unpack_swap_bytes)
			{
				const auto texel_layout = vk::get_format_element_size(vram_texture->info.format);
				const auto elem_size = texel_layout.first;

				if (elem_size == 2)
				{
					shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_16>();
				}
				else if (elem_size == 4)
				{
					shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_32>();
				}
			}

			// Do not run the compute task on host visible memory
			vk::buffer* mem_target = shuffle_kernel ? vk::get_scratch_buffer() : dma_buffer.get();

			// TODO: Read back stencil values (is this really necessary?)
			VkBufferImageCopy region = {};
			region.imageSubresource = {aspect_flag & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1};
			region.imageExtent = {transfer_width, transfer_height, 1};
			vkCmdCopyImageToBuffer(cmd, target->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mem_target->value, 1, &region);

			change_image_layout(cmd, vram_texture, old_layout, subresource_range);
			real_pitch = vk::get_format_texel_width(vram_texture->info.format) * transfer_width;

			if (shuffle_kernel)
			{
				verify (HERE), mem_target->value != dma_buffer->value;

				vk::insert_buffer_memory_barrier(cmd, mem_target->value, 0, cpu_address_range,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

				shuffle_kernel->run(cmd, mem_target, cpu_address_range);

				vk::insert_buffer_memory_barrier(cmd, mem_target->value, 0, cpu_address_range,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

				VkBufferCopy copy = {};
				copy.size = cpu_address_range;
				vkCmdCopyBuffer(cmd, mem_target->value, dma_buffer->value, 1, &copy);
			}

			if (manage_cb_lifetime)
			{
				cmd.end();
				cmd.submit(submit_queue, {}, dma_fence, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

				//Now we need to restart the command-buffer to restore it to the way it was before...
				vk::wait_for_fence(dma_fence);
				vk::reset_fence(&dma_fence);
				CHECK_RESULT(vkResetCommandBuffer(cmd, 0));

				if (cmd.access_hint != vk::command_buffer::access_type_hint::all)
					cmd.begin();
			}

			synchronized = true;
			sync_timestamp = get_system_time();
		}

		bool flush(vk::command_buffer& cmd, VkQueue submit_queue)
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
				copy_texture(true, cmd, submit_queue);
				result = false;
			}

			verify(HERE), real_pitch > 0;
			flushed = true;

			const auto valid_range = get_confirmed_range();
			void* pixels_src = dma_buffer->map(valid_range.first, valid_range.second);
			void* pixels_dst = get_raw_ptr(valid_range.first, true);

			if (real_pitch >= rsx_pitch || valid_range.second <= rsx_pitch)
			{
				memcpy(pixels_dst, pixels_src, valid_range.second);
			}
			else
			{
				if (valid_range.second % rsx_pitch)
				{
					fmt::throw_exception("Unreachable" HERE);
				}

				const u32 num_rows = valid_range.second / rsx_pitch;
				auto _src = (u8*)pixels_src;
				auto _dst = (u8*)pixels_dst;

				for (u32 y = 0; y < num_rows; ++y)
				{
					memcpy(_dst, _src, real_pitch);
					_src += real_pitch;
					_dst += real_pitch;
				}
			}

			flush_io(valid_range.first, valid_range.second);
			dma_buffer->unmap();
			reset_write_statistics();

			//Its highly likely that this surface will be reused, so we just leave resources in place
			return result;
		}

		void set_unpack_swap_bytes(bool swap_bytes)
		{
			pack_unpack_swap_bytes = swap_bytes;
		}

		bool is_synchronized() const
		{
			return synchronized;
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
	};

	struct discarded_storage
	{
		std::unique_ptr<vk::viewable_image> combined_image;
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
			combined_image = std::move(tex.get_texture());
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
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				//Dont bother letting this propagate
				return{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
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
			std::unique_ptr<vk::image> image;
			std::unique_ptr<vk::image_view> view;

			VkImageAspectFlags aspect;
			VkImageCreateFlags image_flags;
			VkFormat dst_format = vk::get_compatible_sampler_format(m_formats_support, gcm_format);

			if (source)
			{
				aspect = vk::get_aspect_flags(source->info.format);
				if (aspect & VK_IMAGE_ASPECT_DEPTH_BIT ||
					vk::get_format_texel_width(dst_format) != vk::get_format_texel_width(source->info.format))
				{
					//HACK! Should use typeless transfer
					dst_format = source->info.format;
				}

				image_flags = source->info.flags;
			}
			else
			{
				aspect = vk::get_aspect_flags(dst_format);
				image_flags = (view_type == VK_IMAGE_VIEW_TYPE_CUBE)? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
			}

			image.reset(new vk::image(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				dst_format,
				w, h, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, image_flags));

			//This method is almost exclusively used to work on framebuffer resources
			//Keep the original swizzle layout unless there is data format conversion
			VkComponentMapping view_swizzle;
			if (!source || dst_format != source->info.format)
			{
				//This is a data cast operation
				//Use native mapping for the new type
				//TODO: Also simulate the readback+reupload step (very tricky)
				const auto remap = get_component_mapping(gcm_format);
				view_swizzle = { remap[1], remap[2], remap[3], remap[0] };
			}
			else
			{
				view_swizzle = source->native_component_map;
			}

			if (memcmp(remap_vector.first.data(), rsx::default_remap_vector.first.data(), 4) ||
				memcmp(remap_vector.second.data(), rsx::default_remap_vector.second.data(), 4))
				view_swizzle = vk::apply_swizzle_remap({view_swizzle.a, view_swizzle.r, view_swizzle.g, view_swizzle.b}, remap_vector);

			VkImageSubresourceRange view_range = { aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 1 };
			view.reset(new vk::image_view(*vk::get_current_renderer(), image->value, view_type, dst_format, view_swizzle, view_range));

			if (copy)
			{
				VkImageSubresourceRange subresource_range = { aspect, 0, 1, 0, 1 };
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

			VkFormat dst_format = vk::get_compatible_sampler_format(m_formats_support, gcm_format);
			VkImageAspectFlags dst_aspect = vk::get_aspect_flags(dst_format);

			image.reset(new vk::image(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				dst_format,
				size, size, 1, 1, 6, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT));

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
					VkImageAspectFlags src_aspect = vk::get_aspect_flags(section.src->info.format);
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

			VkFormat dst_format = vk::get_compatible_sampler_format(m_formats_support, gcm_format);
			VkImageAspectFlags dst_aspect = vk::get_aspect_flags(dst_format);

			image.reset(new vk::image(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_3D,
				dst_format,
				width, height, depth, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0));

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
					VkImageAspectFlags src_aspect = vk::get_aspect_flags(section.src->info.format);
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
			auto result = create_temporary_subresource_view_impl(cmd, nullptr, VK_IMAGE_TYPE_2D,
					VK_IMAGE_VIEW_TYPE_2D, gcm_format, 0, 0, width, height, remap_vector, false);

			VkImage dst = result->info.image;
			VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
			VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 1 };
			vk::change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

			for (const auto &region : sections_to_copy)
			{
				VkImageAspectFlags src_aspect = vk::get_aspect_flags(region.src->info.format);
				VkImageSubresourceRange src_range = { src_aspect, 0, 1, 0, 1 };
				VkImageLayout old_src_layout = region.src->current_layout;
				vk::change_image_layout(cmd, region.src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src_range);

				VkImageCopy copy_rgn;
				copy_rgn.srcOffset = { region.src_x, region.src_y, 0 };
				copy_rgn.dstOffset = { region.dst_x, region.dst_y, 0 };
				copy_rgn.dstSubresource = { dst_aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1 };
				copy_rgn.srcSubresource = { src_aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 0, 1 };
				copy_rgn.extent = { region.w, region.h, 1 };

				vkCmdCopyImage(cmd, region.src->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &copy_rgn);

				vk::change_image_layout(cmd, region.src, old_src_layout, src_range);
			}

			vk::change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
			return result;
		}

		void update_image_contents(vk::command_buffer& cmd, vk::image_view* dst_view, vk::image* src, u16 width, u16 height) override
		{
			VkImage dst = dst_view->info.image;
			VkImageAspectFlags aspect = vk::get_aspect_flags(src->info.format);
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
				rsx::texture_upload_context context, rsx::texture_dimension_extended type, rsx::texture_create_flags flags) override
		{
			const u16 section_depth = depth;
			const bool is_cubemap = type == rsx::texture_dimension_extended::texture_dimension_cubemap;
			VkFormat vk_format;
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
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
				aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				vk_format = m_formats_support.d24_unorm_s8? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D32_SFLOAT_S8_UINT;
				break;
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
				usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				vk_format = VK_FORMAT_D16_UNORM;
				break;
			default:
				aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
				vk_format = get_compatible_sampler_format(m_formats_support, gcm_format);
				break;
			}

			auto *image = new vk::viewable_image(*m_device, m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				vk_format,
				width, height, depth, mipmaps, layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, usage_flags, is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0);

			image->native_component_map = apply_component_mapping_flags(gcm_format, flags, rsx::default_remap_vector);

			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { aspect_flags, 0, mipmaps, 0, layer });

			cached_texture_section& region = find_cached_texture(rsx_address, rsx_size, true, width, height, section_depth);
			region.reset(rsx_address, rsx_size);
			region.create(width, height, section_depth, mipmaps, image, 0, true, gcm_format);
			region.set_dirty(false);
			region.set_context(context);
			region.set_gcm_format(gcm_format);
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
				//NOTE: Protection is handled by the caller
				region.set_unpack_swap_bytes((aspect_flags & VK_IMAGE_ASPECT_COLOR_BIT) == VK_IMAGE_ASPECT_COLOR_BIT);
				no_access_range = region.get_min_max(no_access_range);
			}

			update_cache_tag();
			return &region;
		}

		cached_texture_section* upload_image_from_cpu(vk::command_buffer& cmd, u32 rsx_address, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format,
			rsx::texture_upload_context context, const std::vector<rsx_subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, bool swizzled) override
		{
			auto section = create_new_texture(cmd, rsx_address, pitch * height, width, height, depth, mipmaps, gcm_format, context, type,
					rsx::texture_create_flags::default_component_order);

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
			}

			vk::copy_mipmaped_image_using_buffer(cmd, image, subresource_layout, gcm_format, input_swizzled, mipmaps, subres_range.aspectMask,
				*m_texture_upload_heap);

			vk::leave_uninterruptible();

			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subres_range);

			return section;
		}

		void enforce_surface_creation_type(cached_texture_section& section, u32 gcm_format, rsx::texture_create_flags expected_flags) override
		{
			if (expected_flags == section.get_view_flags())
				return;

			const VkComponentMapping mapping = apply_component_mapping_flags(gcm_format, expected_flags, rsx::default_remap_vector);
			auto image = static_cast<vk::viewable_image*>(section.get_raw_texture());

			verify(HERE), image != nullptr;
			image->set_native_component_layout(mapping);

			section.set_view_flags(expected_flags);
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
				warn_once("Format incompatibility detected, reporting failure to force data copy (VK_FORMAT=0x%X, GCM_FORMAT=0x%X)", (u32)vk_format, gcm_format);
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

		void initialize(vk::render_device& device, VkQueue submit_queue, vk::vk_data_heap& upload_heap)
		{
			m_device = &device;
			m_memory_types = device.get_memory_mapping();
			m_formats_support = device.get_formats_support();
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

			//if (found->second.valid_count == 0)
				//return false;

			for (auto& tex : found->second.data)
			{
				if (tex.is_dirty())
					continue;

				if (!tex.overlaps(rsx_address, rsx::overlap_test_bounds::full_range))
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
			return upload_texture(cmd, tex, m_rtts, cmd, const_cast<const VkQueue>(m_submit_queue));
		}

		vk::image *upload_image_simple(vk::command_buffer& cmd, u32 address, u32 width, u32 height)
		{
			if (!m_formats_support.bgra8_linear)
			{
				return nullptr;
			}

			// Uploads a linear memory range as a BGRA8 texture
			auto image = std::make_unique<vk::image>(*m_device, m_memory_types.host_visible_coherent,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_IMAGE_TYPE_2D,
				VK_FORMAT_B8G8R8A8_UNORM,
				width, height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0);

			VkImageSubresource subresource{};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkSubresourceLayout layout{};
			vkGetImageSubresourceLayout(*m_device, image->value, &subresource, &layout);

			void* mem = image->memory->map(0, layout.rowPitch * height);

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

			image->memory->unmap();

			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			auto result = image.get();
			const u32 resource_memory = width * height * 4; //Rough approximate
			m_discardable_storage.push_back(image);
			m_discardable_storage.back().block_size = resource_memory;
			m_discarded_memory_size += resource_memory;

			return result;
		}

		bool blit(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, rsx::vk_render_targets& m_rtts, vk::command_buffer& cmd)
		{
			struct blit_helper
			{
				vk::command_buffer* commands;
				VkFormat format;
				blit_helper(vk::command_buffer *c) : commands(c) {}

				void scale_image(vk::image* src, vk::image* dst, areai src_area, areai dst_area, bool interpolate, bool /*is_depth*/, const rsx::typeless_xfer& xfer_info)
				{
					const auto src_aspect = vk::get_aspect_flags(src->info.format);
					const auto dst_aspect = vk::get_aspect_flags(dst->info.format);

					vk::image* real_src = src;
					vk::image* real_dst = dst;

					if (xfer_info.src_is_typeless)
					{
						auto internal_width = src->width() * xfer_info.src_scaling_hint;
						auto format = vk::get_compatible_sampler_format(vk::get_current_renderer()->get_formats_support(), xfer_info.src_gcm_format);

						// Transfer bits from src to typeless src
						real_src = vk::get_typeless_helper(format);
						src_area.x1 = (u16)(src_area.x1 * xfer_info.src_scaling_hint);
						src_area.x2 = (u16)(src_area.x2 * xfer_info.src_scaling_hint);

						vk::copy_image_typeless(*commands, src, real_src, { 0, 0, (s32)src->width(), (s32)src->height() }, { 0, 0, (s32)internal_width, (s32)src->height() }, 1,
							vk::get_aspect_flags(src->info.format), vk::get_aspect_flags(format));
					}

					if (xfer_info.dst_is_typeless)
					{
						auto internal_width = dst->width() * xfer_info.dst_scaling_hint;
						auto format = vk::get_compatible_sampler_format(vk::get_current_renderer()->get_formats_support(), xfer_info.dst_gcm_format);

						// Transfer bits from dst to typeless dst
						real_dst = vk::get_typeless_helper(format);
						dst_area.x1 = (u16)(dst_area.x1 * xfer_info.dst_scaling_hint);
						dst_area.x2 = (u16)(dst_area.x2 * xfer_info.dst_scaling_hint);

						vk::copy_image_typeless(*commands, dst, real_dst, { 0, 0, (s32)dst->width(), (s32)dst->height() }, { 0, 0, (s32)internal_width, (s32)dst->height() }, 1,
							vk::get_aspect_flags(dst->info.format), vk::get_aspect_flags(format));
					}
					else if (xfer_info.dst_context == rsx::texture_upload_context::framebuffer_storage)
					{
						if (xfer_info.src_context != rsx::texture_upload_context::blit_engine_dst &&
							xfer_info.src_context != rsx::texture_upload_context::framebuffer_storage)
						{
							// Data moving to rendertarget, where byte ordering has to be preserved
							// NOTE: This is a workaround, true accuracy would require all RTT<->cache transfers to invoke this step but thats too slow
							// Sampling is ok; image view swizzle will work around it
							if (dst->info.format == VK_FORMAT_B8G8R8A8_UNORM)
							{
								// For this specific format, channel ordering is faked via custom remap, undo this before transfer
								VkBufferImageCopy copy{};
								copy.imageExtent = src->info.extent;
								copy.imageOffset = { 0, 0, 0 };
								copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

								const auto scratch_buf = vk::get_scratch_buffer();
								const auto data_length = src->info.extent.width * src->info.extent.height * 4;

								const auto current_layout = src->current_layout;
								vk::change_image_layout(*commands, real_src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
								vkCmdCopyImageToBuffer(*commands, src->value, src->current_layout, scratch_buf->value, 1, &copy);
								vk::change_image_layout(*commands, real_src, current_layout, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

								vk::insert_buffer_memory_barrier(*commands, scratch_buf->value, 0, data_length,
									VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
									VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

								vk::get_compute_task<vk::cs_shuffle_32>()->run(*commands, scratch_buf, data_length);

								vk::insert_buffer_memory_barrier(*commands, scratch_buf->value, 0, data_length,
									VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
									VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

								real_src = vk::get_typeless_helper(src->info.format);
								vk::change_image_layout(*commands, real_src, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

								vkCmdCopyBufferToImage(*commands, scratch_buf->value, real_src->value, real_src->current_layout, 1, &copy);
							}
						}
					}

					// Checks
					if (src_area.x2 <= src_area.x1 || src_area.y2 <= src_area.y1 || dst_area.x2 <= dst_area.x1 || dst_area.y2 <= dst_area.y1)
					{
						LOG_ERROR(RSX, "Blit request consists of an empty region descriptor!");
						return;
					}

					if (src_area.x1 < 0 || src_area.x2 >(s32)real_src->width() || src_area.y1 < 0 || src_area.y2 >(s32)real_src->height())
					{
						LOG_ERROR(RSX, "Blit request denied because the source region does not fit!");
						return;
					}

					if (dst_area.x1 < 0 || dst_area.x2 >(s32)real_dst->width() || dst_area.y1 < 0 || dst_area.y2 >(s32)real_dst->height())
					{
						LOG_ERROR(RSX, "Blit request denied because the destination region does not fit!");
						return;
					}

					const auto src_width = src_area.x2 - src_area.x1;
					const auto src_height = src_area.y2 - src_area.y1;
					const auto dst_width = dst_area.x2 - dst_area.x1;
					const auto dst_height = dst_area.y2 - dst_area.y1;

					copy_scaled_image(*commands, real_src->value, real_dst->value, real_src->current_layout, real_dst->current_layout, src_area.x1, src_area.y1, src_width, src_height,
						dst_area.x1, dst_area.y1, dst_width, dst_height, 1, dst_aspect, real_src->info.format == real_dst->info.format,
						interpolate ? VK_FILTER_LINEAR : VK_FILTER_NEAREST, real_src->info.format, real_dst->info.format);

					if (real_dst != dst)
					{
						auto internal_width = dst->width() * xfer_info.dst_scaling_hint;
						vk::copy_image_typeless(*commands, real_dst, dst, { 0, 0, (s32)internal_width, (s32)dst->height() }, { 0, 0, (s32)dst->width(), (s32)dst->height() }, 1,
							vk::get_aspect_flags(real_dst->info.format), vk::get_aspect_flags(dst->info.format));
					}

					change_image_layout(*commands, dst, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {(VkImageAspectFlags)dst_aspect, 0, dst->info.mipLevels, 0, dst->info.arrayLayers});
					format = dst->info.format;
				}
			}
			helper(&cmd);

			auto reply = upload_scaled_image(src, dst, interpolate, cmd, m_rtts, helper, cmd, const_cast<const VkQueue>(m_submit_queue));

			if (reply.succeeded)
			{
				if (reply.real_dst_size)
				{
					flush_if_cache_miss_likely(helper.format, reply.real_dst_address, reply.real_dst_size, cmd, m_submit_queue);
				}

				return true;
			}

			return false;
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
