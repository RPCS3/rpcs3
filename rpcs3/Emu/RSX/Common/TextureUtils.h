#pragma once

#include "io_buffer.h"
#include "../color_utils.h"
#include "../RSXTexture.h"

#include <stack>
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
			shader_read    = (1 << 0),
			shader_write   = (1 << 1),
			transfer_read  = (1 << 2),
			transfer_write = (1 << 3),

			// Arbitrary r/w flags, use with caution.
			memory_write   = (1 << 4),
			memory_read    = (1 << 5),

			// Not r/w but signifies a GPU reference to this object.
			gpu_reference  = (1 << 6),
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

			RSX_FORMAT_CLASS_DEPTH_FLOAT_MASK = (RSX_FORMAT_CLASS_DEPTH16_FLOAT | RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32),
			RSX_FORMAT_CLASS_DONT_CARE = RSX_FORMAT_CLASS_UNDEFINED,
		};
	}

	using namespace format_class_;

	// Sampled image descriptor
	class sampled_image_descriptor_base
	{
#pragma pack(push, 1)
		struct texcoord_xform_t
		{
			f32 scale[3];
			f32 bias[3];
			f32 clamp_min[2];
			f32 clamp_max[2];
			bool clamp = false;
		};
#pragma pack(pop)

		// Texure matrix stack
		std::stack<texcoord_xform_t> m_texcoord_xform_stack;

	public:
		virtual ~sampled_image_descriptor_base() = default;
		virtual u32 encoded_component_map() const = 0;

		void push_texcoord_xform()
		{
			m_texcoord_xform_stack.push(texcoord_xform);
		}

		void pop_texcoord_xform()
		{
			ensure(!m_texcoord_xform_stack.empty());
			std::memcpy(&texcoord_xform, &m_texcoord_xform_stack.top(), sizeof(texcoord_xform_t));
			m_texcoord_xform_stack.pop();
		}

		texture_upload_context upload_context = texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = texture_dimension_extended::texture_dimension_2d;
		rsx::format_class format_class = RSX_FORMAT_CLASS_UNDEFINED;
		bool is_cyclic_reference = false;
		u8 samples = 1;
		u32 ref_address = 0;
		u64 surface_cache_tag = 0;

		texcoord_xform_t texcoord_xform;
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
		rsx::io_buffer data;
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
		bool supports_dxt;
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

	texture_memory_info upload_texture_subresource(rsx::io_buffer& dst_buffer, const subresource_layout &src_layout, int format, bool is_swizzled, texture_uploader_capabilities& caps);

	u8 get_format_block_size_in_bytes(int format);
	u8 get_format_block_size_in_texel(int format);
	u8 get_format_block_size_in_bytes(rsx::surface_color_format format);
	u8 get_format_block_size_in_bytes(rsx::surface_depth_format2 format);

	bool is_compressed_host_format(const texture_uploader_capabilities& caps, u32 format); // Returns true for host-compressed formats (DXT)
	u8 get_format_sample_count(rsx::surface_antialiasing antialias);
	u32 get_max_depth_value(rsx::surface_depth_format2 format);
	bool is_depth_stencil_format(rsx::surface_depth_format2 format);
	bool is_int8_remapped_format(u32 format); // Returns true if the format is treated as INT8 by the RSX remapper.

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
	u32 get_remap_encoding(const texture_channel_remap_t& remap);

	/**
	 * Get gcm texel layout. Returns <format, byteswapped>
	 */
	std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_color_format format);
	std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_depth_format2 format);

	format_class classify_format(rsx::surface_depth_format2 format);
	format_class classify_format(u32 gcm_format);

	bool is_texcoord_wrapping_mode(rsx::texture_wrap_mode mode);
	bool is_border_clamped_texture(rsx::texture_wrap_mode wrap_s, rsx::texture_wrap_mode wrap_t, rsx::texture_wrap_mode wrap_r, rsx::texture_dimension dimension);

	template <typename TextureType>
	bool is_border_clamped_texture(const TextureType& tex)
	{
		return is_border_clamped_texture(tex.wrap_s(), tex.wrap_t(), tex.wrap_r(), tex.dimension());
	}
}
