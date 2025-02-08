#pragma once

#include "common.h"

#include "Utilities/geometry.h"
#include "Emu/RSX/Common/TextureUtils.h"

//using enum rsx::format_class;
using namespace ::rsx::format_class_;

namespace gl
{
#define GL_BGRA8        0x80E1 // Enumerant of GL_BGRA8_EXT from the GL_EXT_texture_format_BGRA8888
#define GL_BGR5_A1      0x99F0 // Unused enum 0x96xx is the last official GL enumerant

	class buffer;
	class buffer_view;
	class command_context;
	class pixel_pack_settings;
	class pixel_unpack_settings;

	enum image_aspect : u32
	{
		color = 1,
		depth = 2,
		stencil = 4
	};

	enum class filter
	{
		nearest = GL_NEAREST,
		linear = GL_LINEAR
	};

	enum class min_filter
	{
		nearest = GL_NEAREST,
		linear = GL_LINEAR,
		nearest_mipmap_nearest = GL_NEAREST_MIPMAP_NEAREST,
		nearest_mipmap_linear = GL_NEAREST_MIPMAP_LINEAR,
		linear_mipmap_nearest = GL_LINEAR_MIPMAP_NEAREST,
		linear_mipmap_linear = GL_LINEAR_MIPMAP_LINEAR
	};

	enum remap_constants : u32
	{
		GL_REMAP_IDENTITY = 0xCAFEBABE,
		GL_REMAP_BGRA = 0x0000AA6C,
		GL_REMAP_VIEW_MULTISAMPLED = 0xDEADBEEF
	};

	struct subresource_range
	{
		GLenum aspect_mask;
		GLuint min_level;
		GLuint num_levels;
		GLuint min_layer;
		GLuint num_layers;
	};

	class texture
	{
		friend class texture_view;

	public:
		enum class type
		{
			ubyte = GL_UNSIGNED_BYTE,
			ushort = GL_UNSIGNED_SHORT,
			uint = GL_UNSIGNED_INT,

			ubyte_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
			ubyte_2_3_3_rev = GL_UNSIGNED_BYTE_2_3_3_REV,

			ushort_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
			ushort_5_6_5_rev = GL_UNSIGNED_SHORT_5_6_5_REV,
			ushort_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
			ushort_4_4_4_4_rev = GL_UNSIGNED_SHORT_4_4_4_4_REV,
			ushort_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
			ushort_1_5_5_5_rev = GL_UNSIGNED_SHORT_1_5_5_5_REV,

			uint_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
			uint_8_8_8_8_rev = GL_UNSIGNED_INT_8_8_8_8_REV,
			uint_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
			uint_2_10_10_10_rev = GL_UNSIGNED_INT_2_10_10_10_REV,
			uint_24_8 = GL_UNSIGNED_INT_24_8,
			float32_uint8 = GL_FLOAT_32_UNSIGNED_INT_24_8_REV,

			sbyte = GL_BYTE,
			sshort = GL_SHORT,
			sint = GL_INT,
			f16 = GL_HALF_FLOAT,
			f32 = GL_FLOAT,
			f64 = GL_DOUBLE,
		};

		enum class channel
		{
			zero = GL_ZERO,
			one = GL_ONE,
			r = GL_RED,
			g = GL_GREEN,
			b = GL_BLUE,
			a = GL_ALPHA,
		};

		enum class format
		{
			r = GL_RED,
			rg = GL_RG,
			rgb = GL_RGB,
			rgba = GL_RGBA,

			bgr = GL_BGR,
			bgra = GL_BGRA,

			stencil = GL_STENCIL_INDEX,
			depth = GL_DEPTH_COMPONENT,
			depth_stencil = GL_DEPTH_STENCIL
		};

		enum class internal_format
		{
			stencil8 = GL_STENCIL_INDEX8,
			depth16 = GL_DEPTH_COMPONENT16,
			depth32f = GL_DEPTH_COMPONENT32F,
			depth24_stencil8 = GL_DEPTH24_STENCIL8,
			depth32f_stencil8 = GL_DEPTH32F_STENCIL8,

			compressed_rgb_s3tc_dxt1 = GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
			compressed_rgba_s3tc_dxt1 = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
			compressed_rgba_s3tc_dxt3 = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
			compressed_rgba_s3tc_dxt5 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,

			//Sized internal formats, see opengl spec document on glTexImage2D, table 3
			rgba8 = GL_RGBA8,
			bgra8 = GL_BGRA8,
			rgb565 = GL_RGB565,
			rgb5a1 = GL_RGB5_A1,
			bgr5a1 = GL_BGR5_A1,
			rgba4 = GL_RGBA4,
			r8 = GL_R8,
			r16 = GL_R16,
			r32f = GL_R32F,
			rg8 = GL_RG8,
			rg16 = GL_RG16,
			rg16f = GL_RG16F,
			rgba16f = GL_RGBA16F,
			rgba32f = GL_RGBA32F,

			rg8_snorm = GL_RG8_SNORM
		};

		enum class wrap
		{
			repeat = GL_REPEAT,
			mirrored_repeat = GL_MIRRORED_REPEAT,
			clamp_to_edge = GL_CLAMP_TO_EDGE,
			clamp_to_border = GL_CLAMP_TO_BORDER,
			mirror_clamp = GL_MIRROR_CLAMP_EXT,
			//mirror_clamp_to_edge = GL_MIRROR_CLAMP_TO_EDGE,
			mirror_clamp_to_border = GL_MIRROR_CLAMP_TO_BORDER_EXT
		};

		enum class compare_mode
		{
			none = GL_NONE,
			ref_to_texture = GL_COMPARE_REF_TO_TEXTURE
		};

		enum class target
		{
			texture1D = GL_TEXTURE_1D,
			texture2D = GL_TEXTURE_2D,
			texture3D = GL_TEXTURE_3D,
			textureCUBE = GL_TEXTURE_CUBE_MAP,
			textureBuffer = GL_TEXTURE_BUFFER,
			texture2DArray = GL_TEXTURE_2D_ARRAY,
			texture2DMS = GL_TEXTURE_2D_MULTISAMPLE
		};

	protected:
		GLuint m_id = GL_NONE;
		GLuint m_width = 0;
		GLuint m_height = 0;
		GLuint m_depth = 0;
		GLuint m_mipmaps = 0;
		GLubyte m_samples = 0;
		GLuint m_pitch = 0;
		GLuint m_compressed = GL_FALSE;
		GLuint m_aspect_flags = 0;

		target m_target = target::texture2D;
		internal_format m_internal_format = internal_format::rgba8;
		std::array<GLenum, 4> m_component_layout;

		rsx::format_class m_format_class = RSX_FORMAT_CLASS_UNDEFINED;

	public:
		texture(const texture&) = delete;
		texture(texture&& texture_) = delete;

		texture(GLenum target, GLuint width, GLuint height, GLuint depth, GLuint mipmaps, GLubyte samples, GLenum sized_format, rsx::format_class format_class);
		virtual ~texture();

		// Getters/setters
		void set_native_component_layout(const std::array<GLenum, 4>& layout)
		{
			m_component_layout[0] = layout[0];
			m_component_layout[1] = layout[1];
			m_component_layout[2] = layout[2];
			m_component_layout[3] = layout[3];
		}

		target get_target() const noexcept
		{
			return m_target;
		}

		static bool compressed_format(internal_format format_) noexcept
		{
			switch (format_)
			{
			case internal_format::compressed_rgb_s3tc_dxt1:
			case internal_format::compressed_rgba_s3tc_dxt1:
			case internal_format::compressed_rgba_s3tc_dxt3:
			case internal_format::compressed_rgba_s3tc_dxt5:
				return true;
			default:
				return false;
			}
		}

		uint id() const noexcept
		{
			return m_id;
		}

		explicit operator bool() const noexcept
		{
			return (m_id != GL_NONE);
		}

		GLuint width() const
		{
			return m_width;
		}

		GLuint height() const
		{
			return m_height;
		}

		GLuint depth() const
		{
			return m_depth;
		}

		GLuint levels() const
		{
			return m_mipmaps;
		}

		GLuint layers() const
		{
			switch (m_target)
			{
			case target::textureCUBE:
				return 6;
			case target::texture2DArray:
				return m_depth;
			default:
				return 1;
			}
		}

		GLuint pitch() const
		{
			return m_pitch;
		}

		GLubyte samples() const
		{
			return m_samples;
		}

		GLboolean compressed() const
		{
			return m_compressed;
		}

		GLuint aspect() const
		{
			return m_aspect_flags;
		}

		rsx::format_class format_class() const
		{
			return m_format_class;
		}

		sizeu size2D() const
		{
			return{ m_width, m_height };
		}

		size3u size3D() const
		{
			const auto depth = (m_target == target::textureCUBE) ? 6 : m_depth;
			return{ m_width, m_height, depth };
		}

		texture::internal_format get_internal_format() const
		{
			return m_internal_format;
		}

		std::array<GLenum, 4> get_native_component_layout() const
		{
			return m_component_layout;
		}

		// Data management
		void copy_from(const void* src, texture::format format, texture::type type, int level, const coord3u region, const pixel_unpack_settings& pixel_settings);

		void copy_from(buffer& buf, u32 gl_format_type, u32 offset, u32 length);

		void copy_from(buffer_view& view);

		void copy_to(void* dst, texture::format format, texture::type type, int level, const coord3u& region, const pixel_pack_settings& pixel_settings) const;

		// Convenience wrappers
		void copy_from(const void* src, texture::format format, texture::type type, const pixel_unpack_settings& pixel_settings)
		{
			const coord3u region = { {}, size3D() };
			copy_from(src, format, type, 0, region, pixel_settings);
		}

		void copy_to(void* dst, texture::format format, texture::type type, const pixel_pack_settings& pixel_settings) const
		{
			const coord3u region = { {}, size3D() };
			copy_to(dst, format, type, 0, region, pixel_settings);
		}
	};

	class texture_view
	{
	protected:
		GLuint m_id = GL_NONE;
		GLenum m_target = 0;
		GLenum m_format = 0;
		GLenum m_view_format = 0;
		GLenum m_aspect_flags = 0;
		texture* m_image_data = nullptr;

		GLenum component_swizzle[4];

		texture_view() = default;

		void create(texture* data, GLenum target, GLenum sized_format, const subresource_range& range, const GLenum* argb_swizzle = nullptr);

	public:
		texture_view(const texture_view&) = delete;
		texture_view(texture_view&&) = delete;

		texture_view(texture* data, GLenum target, GLenum sized_format,
			const GLenum* argb_swizzle = nullptr,
			GLenum aspect_flags = image_aspect::color | image_aspect::depth)
		{
			create(data, target, sized_format, { aspect_flags, 0, data->levels(), 0, data->layers() }, argb_swizzle);
		}

		texture_view(texture* data, const GLenum* argb_swizzle = nullptr,
			GLenum aspect_flags = image_aspect::color | image_aspect::depth)
		{
			GLenum target = static_cast<GLenum>(data->get_target());
			GLenum sized_format = static_cast<GLenum>(data->get_internal_format());
			create(data, target, sized_format, { aspect_flags, 0, data->levels(), 0, data->layers() }, argb_swizzle);
		}

		texture_view(texture* data, const subresource_range& range,
			const GLenum* argb_swizzle = nullptr)
		{
			GLenum target = static_cast<GLenum>(data->get_target());
			GLenum sized_format = static_cast<GLenum>(data->get_internal_format());
			create(data, target, sized_format, range, argb_swizzle);
		}

		texture_view(texture* data, GLenum target, const subresource_range& range,
			const GLenum* argb_swizzle = nullptr)
		{
			GLenum sized_format = static_cast<GLenum>(data->get_internal_format());
			create(data, target, sized_format, range, argb_swizzle);
		}

		virtual ~texture_view();

		GLuint id() const
		{
			return m_id;
		}

		GLenum target() const
		{
			return m_target;
		}

		GLenum internal_format() const
		{
			return m_format;
		}

		GLenum view_format() const
		{
			return m_view_format;
		}

		GLenum aspect() const
		{
			return m_aspect_flags;
		}

		bool compare_swizzle(const GLenum* argb_swizzle) const
		{
			return (argb_swizzle[0] == component_swizzle[3] &&
				argb_swizzle[1] == component_swizzle[0] &&
				argb_swizzle[2] == component_swizzle[1] &&
				argb_swizzle[3] == component_swizzle[2]);
		}

		texture* image() const
		{
			return m_image_data;
		}

		std::array<GLenum, 4> component_mapping() const
		{
			return{ component_swizzle[3], component_swizzle[0], component_swizzle[1], component_swizzle[2] };
		}

		u32 encoded_component_map() const
		{
			// Unused, OGL supports proper component swizzles
			return 0u;
		}

		void bind(gl::command_context& cmd, GLuint layer) const;
	};

	// Passthrough texture view that simply wraps the original texture in a texture_view interface
	class nil_texture_view : public texture_view
	{
	public:
		nil_texture_view(texture* data);
		~nil_texture_view();
	};

	class viewable_image : public texture
	{
		std::unordered_map<u64, std::unique_ptr<texture_view>> views;

	public:
		using texture::texture;

		texture_view* get_view(const rsx::texture_channel_remap_t& remap, GLenum aspect_flags = image_aspect::color | image_aspect::depth);
		void set_native_component_layout(const std::array<GLenum, 4>& layout);
	};

	// Texture helpers
	std::array<GLenum, 4> apply_swizzle_remap(const std::array<GLenum, 4>& swizzle_remap, const rsx::texture_channel_remap_t& decoded_remap);
}
