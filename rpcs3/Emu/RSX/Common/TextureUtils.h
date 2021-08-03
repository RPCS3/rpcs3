#pragma once

#include "../RSXTexture.h"

#include <span>
#include <vector>

namespace rsx
{
	enum texture_upload_context : u32
	{
		shader_read = 1,
		blit_engine_src = 2,
		blit_engine_dst = 4,
		framebuffer_storage = 8,
		dma = 16
	};

	enum texture_colorspace : u32
	{
		rgb_linear = 0,
		srgb_nonlinear = 1
	};

	enum surface_usage_flags : u32
	{
		unknown = 0,
		attachment = 1,
		storage = 2,
	};

	enum surface_metrics : u32
	{
		pixels = 0,
		samples = 1,
		bytes = 2
	};

	class surface_access // This is simply a modified enum class
	{
	public:
		// Publicly visible enumerators
		enum
		{
			shader_read = 0,
			shader_write = 1,
			transfer_read = 2,
			transfer_write = 4,

			// Arbitrary r/w flags, use with caution.
			memory_write = 8,
			memory_read = 16,

			// Not r/w but signifies a GPU reference to this object.
			gpu_reference = 32
		};

	private:
		// Meta
		enum
		{
			all_writes = (shader_write | transfer_write | memory_write),
			all_reads = (shader_read | transfer_read | memory_read),
			all_transfer = (transfer_read | transfer_write)
		};

		u32 value_;

	public:
		// Ctor
		surface_access(u32 value) : value_(value)
		{}

		// Quick helpers
		inline bool is_read() const
		{
			return !(value_ & ~all_reads);
		}

		inline bool is_write() const
		{
			return !(value_ & ~all_writes);
		}

		inline bool is_transfer() const
		{
			return !(value_ & ~all_transfer);
		}

		inline bool is_transfer_or_read() const // Special; reads and transfers generate MSAA load operations
		{
			return !(value_ & ~(all_transfer | all_reads));
		}

		bool operator == (const surface_access& other) const
		{
			return value_ == other.value_;
		}

		bool operator == (u32 other) const
		{
			return value_ == other;
		}
	};

	// Defines how the underlying PS3-visible memory backed by a texture is accessed
	namespace format_class_
	{
		// TODO: Remove when enum import is supported by clang
		enum format_class : u8
		{
			RSX_FORMAT_CLASS_UNDEFINED = 0,
			RSX_FORMAT_CLASS_COLOR = 1,
			RSX_FORMAT_CLASS_DEPTH16_UNORM = 2,
			RSX_FORMAT_CLASS_DEPTH16_FLOAT = 4,
			RSX_FORMAT_CLASS_DEPTH24_UNORM_X8_PACK32 = 8,
			RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32 = 16,

			RSX_FORMAT_CLASS_DEPTH_FLOAT_MASK = (RSX_FORMAT_CLASS_DEPTH16_FLOAT | RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32)
		};
	}

	using namespace format_class_;

	//Sampled image descriptor
	struct sampled_image_descriptor_base
	{
		texture_upload_context upload_context = texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = texture_dimension_extended::texture_dimension_2d;
		rsx::format_class format_class = RSX_FORMAT_CLASS_UNDEFINED;
		bool is_cyclic_reference = false;
		u32 ref_address = 0;
		u64 surface_cache_tag = 0;
		f32 scale_x = 1.f;
		f32 scale_y = 1.f;
		f32 scale_z = 1.f;

		virtual ~sampled_image_descriptor_base() = default;
		virtual u32 encoded_component_map() const = 0;
	};

	struct typeless_xfer
	{
		bool src_is_typeless = false;
		bool dst_is_typeless = false;
		bool flip_vertical = false;
		bool flip_horizontal = false;

		u32 src_gcm_format = 0;
		u32 dst_gcm_format = 0;
		u32 src_native_format_override = 0;
		u32 dst_native_format_override = 0;
		f32 src_scaling_hint = 1.f;
		f32 dst_scaling_hint = 1.f;
		texture_upload_context src_context = texture_upload_context::blit_engine_src;
		texture_upload_context dst_context = texture_upload_context::blit_engine_dst;

		void analyse();
	};

	struct subresource_layout
	{
		std::span<const std::byte> data;
		u16 width_in_texel;
		u16 height_in_texel;
		u16 width_in_block;
		u16 height_in_block;
		u16 depth;
		u16 level;
		u16 layer;
		u8  border;
		u8  reserved;
		u32 pitch_in_block;
	};

	struct memory_transfer_cmd
	{
		const void* dst;
		const void* src;
		u32 length;
	};

	struct texture_memory_info
	{
		int element_size;
		int block_length;
		bool require_swap;
		bool require_deswizzle;
		bool require_upload;

		std::vector<memory_transfer_cmd> deferred_cmds;
	};

	struct texture_uploader_capabilities
	{
		bool supports_byteswap;
		bool supports_vtc_decoding;
		bool supports_hw_deswizzle;
		bool supports_zero_copy;
		usz alignment;
	};

	/**
	* Get size to store texture in a linear fashion.
	* Storage is assumed to use a rowPitchAlignment boundary for every row of texture.
	*/
	usz get_placed_texture_storage_size(u16 width, u16 height, u32 depth, u8 format, u16 mipmap, bool cubemap, usz row_pitch_alignment, usz mipmap_alignment);
	usz get_placed_texture_storage_size(const rsx::fragment_texture &texture, usz row_pitch_alignment, usz mipmap_alignment = 0x200);
	usz get_placed_texture_storage_size(const rsx::vertex_texture &texture, usz row_pitch_alignment, usz mipmap_alignment = 0x200);

	/**
	 * get all rsx::subresource_layout for texture.
	 * The subresources are ordered per layer then per mipmap level (as in rsx memory).
	 */
	std::vector<subresource_layout> get_subresources_layout(const rsx::fragment_texture &texture);
	std::vector<subresource_layout> get_subresources_layout(const rsx::vertex_texture &texture);

	texture_memory_info upload_texture_subresource(std::span<std::byte> dst_buffer, const subresource_layout &src_layout, int format, bool is_swizzled, texture_uploader_capabilities& caps);

	u8 get_format_block_size_in_bytes(int format);
	u8 get_format_block_size_in_texel(int format);
	u8 get_format_block_size_in_bytes(rsx::surface_color_format format);
	u8 get_format_block_size_in_bytes(rsx::surface_depth_format2 format);

	bool is_compressed_host_format(u32 format); // Returns true for host-compressed formats (DXT)
	u8 get_format_sample_count(rsx::surface_antialiasing antialias);
	u32 get_max_depth_value(rsx::surface_depth_format2 format);
	bool is_depth_stencil_format(rsx::surface_depth_format2 format);

	/**
	 * Returns number of texel rows encoded in one pitch-length line of bytes
	 */
	u8 get_format_texel_rows_per_line(u32 format);

	/**
	* Get number of bytes occupied by texture in RSX mem
	*/
	usz get_texture_size(const rsx::fragment_texture &texture);
	usz get_texture_size(const rsx::vertex_texture &texture);

	/**
	* Get packed pitch
	*/
	u32 get_format_packed_pitch(u32 format, u16 width, bool border = false, bool swizzled = false);

	/**
	* Reverse encoding
	*/
	u32 get_remap_encoding(const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap);

	/**
	 * Get gcm texel layout. Returns <format, byteswapped>
	 */
	std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_color_format format);
	std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_depth_format2 format);

	format_class classify_format(rsx::surface_depth_format2 format);
	format_class classify_format(u32 gcm_format);
}
