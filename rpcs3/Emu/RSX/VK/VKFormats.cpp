#include "stdafx.h"
#include "VKFormats.h"
#include "VKHelpers.h"
#include "vkutils/device.h"
#include "vkutils/image.h"

namespace vk
{
	VkFormat get_compatible_depth_surface_format(const gpu_formats_support& support, rsx::surface_depth_format2 format)
	{
		switch (format)
		{
		case rsx::surface_depth_format2::z16_uint:
			return VK_FORMAT_D16_UNORM;
		case rsx::surface_depth_format2::z16_float:
			return VK_FORMAT_D32_SFLOAT;
		case rsx::surface_depth_format2::z24s8_uint:
		{
			if (support.d24_unorm_s8) return VK_FORMAT_D24_UNORM_S8_UINT;
			if (support.d32_sfloat_s8) return VK_FORMAT_D32_SFLOAT_S8_UINT;
			fmt::throw_exception("No hardware support for z24s8");
		}
		case rsx::surface_depth_format2::z24s8_float:
		{
			if (support.d32_sfloat_s8) return VK_FORMAT_D32_SFLOAT_S8_UINT;
			fmt::throw_exception("No hardware support for z24s8_float");
		}
		default:
			break;
		}
		fmt::throw_exception("Invalid format (0x%x)", static_cast<u32>(format));
	}

	minification_filter get_min_filter(rsx::texture_minify_filter min_filter)
	{
		switch (min_filter)
		{
		case rsx::texture_minify_filter::nearest: return { VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, false };
		case rsx::texture_minify_filter::linear: return { VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, false };
		case rsx::texture_minify_filter::nearest_nearest: return { VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, true };
		case rsx::texture_minify_filter::linear_nearest: return { VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, true };
		case rsx::texture_minify_filter::nearest_linear: return { VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR, true };
		case rsx::texture_minify_filter::linear_linear: return { VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, true };
		case rsx::texture_minify_filter::convolution_min: return { VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, false };
		default:
			fmt::throw_exception("Invalid min filter");
		}
	}

	VkFilter get_mag_filter(rsx::texture_magnify_filter mag_filter)
	{
		switch (mag_filter)
		{
		case rsx::texture_magnify_filter::nearest: return VK_FILTER_NEAREST;
		case rsx::texture_magnify_filter::linear: return VK_FILTER_LINEAR;
		case rsx::texture_magnify_filter::convolution_mag: return VK_FILTER_LINEAR;
		default:
			break;
		}

		fmt::throw_exception("Invalid mag filter (0x%x)", static_cast<u32>(mag_filter));
	}

	VkBorderColor get_border_color(u32 color)
	{
		switch (color)
		{
		case 0x00000000:
		{
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		}
		case 0xFFFFFFFF:
		{
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		}
		case 0xFF000000:
		{
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		}
		default:
		{
			return VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
		}
		}
	}

	VkSamplerAddressMode vk_wrap_mode(rsx::texture_wrap_mode gcm_wrap)
	{
		switch (gcm_wrap)
		{
		case rsx::texture_wrap_mode::wrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case rsx::texture_wrap_mode::mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case rsx::texture_wrap_mode::clamp_to_edge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case rsx::texture_wrap_mode::border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case rsx::texture_wrap_mode::clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case rsx::texture_wrap_mode::mirror_once_clamp_to_edge: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		case rsx::texture_wrap_mode::mirror_once_border: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		case rsx::texture_wrap_mode::mirror_once_clamp: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		default:
			fmt::throw_exception("Unhandled texture clamp mode");
		}
	}

	float max_aniso(rsx::texture_max_anisotropy gcm_aniso)
	{
		switch (gcm_aniso)
		{
		case rsx::texture_max_anisotropy::x1: return 1.0f;
		case rsx::texture_max_anisotropy::x2: return 2.0f;
		case rsx::texture_max_anisotropy::x4: return 4.0f;
		case rsx::texture_max_anisotropy::x6: return 6.0f;
		case rsx::texture_max_anisotropy::x8: return 8.0f;
		case rsx::texture_max_anisotropy::x10: return 10.0f;
		case rsx::texture_max_anisotropy::x12: return 12.0f;
		case rsx::texture_max_anisotropy::x16: return 16.0f;
		default:
			break;
		}

		fmt::throw_exception("Texture anisotropy error: bad max aniso (%d)", static_cast<u32>(gcm_aniso));
	}


	std::array<VkComponentSwizzle, 4> get_component_mapping(u32 format)
	{
		//Component map in ARGB format
		std::array<VkComponentSwizzle, 4> mapping = {};

		switch (format)
		{
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			mapping = { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B }; break;

		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R }; break;

		case CELL_GCM_TEXTURE_A4R4G4B4:
			mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }; break;

		case CELL_GCM_TEXTURE_G8B8:
			mapping = { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R }; break;

		case CELL_GCM_TEXTURE_B8:
			mapping = { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R }; break;

		case CELL_GCM_TEXTURE_X16:
			mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE }; break;

		case CELL_GCM_TEXTURE_X32_FLOAT:
			mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R }; break;

		case CELL_GCM_TEXTURE_Y16_X16:
			mapping = { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R }; break;

		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G }; break;

		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
			mapping = { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B }; break;

		case CELL_GCM_TEXTURE_D8R8G8B8:
			mapping = { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B }; break;

		case CELL_GCM_TEXTURE_D1R5G5B5:
			mapping = { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B }; break;

		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
			mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G }; break;

		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			mapping = { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B }; break;

		case CELL_GCM_TEXTURE_A8R8G8B8:
			mapping = { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B }; break;

		default:
			fmt::throw_exception("Invalid or unsupported component mapping for texture format (0x%x)", format);
		}

		return mapping;
	}

	VkFormat get_compatible_sampler_format(const gpu_formats_support& support, u32 format)
	{
		const bool supports_dxt = vk::get_current_renderer()->get_texture_compression_bc_support();
		switch (format)
		{
#ifndef __APPLE__
		case CELL_GCM_TEXTURE_R5G6B5: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case CELL_GCM_TEXTURE_R6G5B5: return VK_FORMAT_R5G6B5_UNORM_PACK16; // Expand, discard high bit?
		case CELL_GCM_TEXTURE_R5G5B5A1: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
		case CELL_GCM_TEXTURE_D1R5G5B5: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case CELL_GCM_TEXTURE_A1R5G5B5: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case CELL_GCM_TEXTURE_A4R4G4B4: return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
#else
		// assign B8G8R8A8_UNORM to formats that are not supported by Metal
		case CELL_GCM_TEXTURE_R6G5B5: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_R5G6B5: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_R5G5B5A1: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_D1R5G5B5: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_A1R5G5B5: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_A4R4G4B4: return VK_FORMAT_B8G8R8A8_UNORM;
#endif
		case CELL_GCM_TEXTURE_B8: return VK_FORMAT_R8_UNORM;
		case CELL_GCM_TEXTURE_A8R8G8B8: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return supports_dxt ? VK_FORMAT_BC1_RGBA_UNORM_BLOCK : VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return supports_dxt ? VK_FORMAT_BC2_UNORM_BLOCK : VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return supports_dxt ? VK_FORMAT_BC3_UNORM_BLOCK : VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_G8B8: return VK_FORMAT_R8G8_UNORM;
		case CELL_GCM_TEXTURE_DEPTH24_D8: return support.d24_unorm_s8? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D32_SFLOAT_S8_UINT;
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:	return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case CELL_GCM_TEXTURE_DEPTH16: return VK_FORMAT_D16_UNORM;
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return VK_FORMAT_D32_SFLOAT;
		case CELL_GCM_TEXTURE_X16: return VK_FORMAT_R16_UNORM;
		case CELL_GCM_TEXTURE_Y16_X16: return VK_FORMAT_R16G16_UNORM;
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return VK_FORMAT_R16G16_SFLOAT;
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case CELL_GCM_TEXTURE_X32_FLOAT: return VK_FORMAT_R32_SFLOAT;
		case CELL_GCM_TEXTURE_D8R8G8B8: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8: return VK_FORMAT_R8G8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return VK_FORMAT_R8G8_SNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return VK_FORMAT_B8G8R8A8_UNORM;
		default:
			break;
		}
		fmt::throw_exception("Invalid or unsupported sampler format for texture format (0x%x)", format);
	}

	VkFormat get_compatible_srgb_format(VkFormat rgb_format)
	{
		switch (rgb_format)
		{
		case VK_FORMAT_B8G8R8A8_UNORM:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case VK_FORMAT_BC2_UNORM_BLOCK:
			return VK_FORMAT_BC2_SRGB_BLOCK;
		case VK_FORMAT_BC3_UNORM_BLOCK:
			return VK_FORMAT_BC3_SRGB_BLOCK;
		default:
			return rgb_format;
		}
	}

	u8 get_format_texel_width(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8_UNORM:
			return 1;
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			return 2;
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
			return 4;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return 8;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16;
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return 2;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: //TODO: Translate to D24S8
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return 4;
		default:
			break;
		}

		fmt::throw_exception("Unexpected vkFormat 0x%X", static_cast<u32>(format));
	}

	std::pair<u8, u8> get_format_element_size(VkFormat format)
	{
		// Return value is {ELEMENT_SIZE, NUM_ELEMENTS_PER_TEXEL}
		// NOTE: Due to endianness issues, coalesced larger types are preferred
		// e.g UINT1 to hold 4x1 bytes instead of UBYTE4 to hold 4x1

		switch (format)
		{
		//8-bit
		case VK_FORMAT_R8_UNORM:
			return{ 1, 1 };
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
			return{ 2, 1 }; //UNSIGNED_SHORT_8_8
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SRGB:
			return{ 4, 1 }; //UNSIGNED_INT_8_8_8_8
		//16-bit
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R16_UNORM:
			return{ 2, 1 }; //UNSIGNED_SHORT and HALF_FLOAT
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SFLOAT:
			return{ 2, 2 }; //HALF_FLOAT
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return{ 2, 4 }; //HALF_FLOAT
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			return{ 2, 1 }; //UNSIGNED_SHORT_X_Y_Z_W
		//32-bit
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SFLOAT:
			return{ 4, 1 }; //FLOAT
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return{ 4, 4 }; //FLOAT
		//DXT
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
			return{ 4, 1 };
		//Depth
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return{ 2, 1 };
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return{ 4, 1 };
		default:
			break;
		}

		fmt::throw_exception("Unexpected vkFormat 0x%X", static_cast<u32>(format));
	}

	std::pair<bool, u32> get_format_convert_flags(VkFormat format)
	{
		switch (format)
		{
			//8-bit
		case VK_FORMAT_R8_UNORM:
			return{ false, 1 };
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_R8G8B8A8_SRGB:
			return{ true, 4 };
			//16-bit
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			return{ true, 2 };
			//32-bit
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return{ true, 4 };
			//DXT
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
			return{ false, 1 };
			//Depth
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return{ true, 2 };
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return{ true, 4 };
		default:
			break;
		}

		fmt::throw_exception("Unknown vkFormat 0x%x", static_cast<u32>(format));
	}

	bool formats_are_bitcast_compatible(VkFormat format1, VkFormat format2)
	{
		if (format1 == format2) [[likely]]
		{
			return true;
		}

		// Formats are compatible if the following conditions are met:
		// 1. Texel sizes must match
		// 2. Both formats require no transforms (basic memcpy) or...
		// 3. Both formats have the same transform (e.g RG16_UNORM to RG16_SFLOAT, both are down and uploaded with a 2-byte byteswap)

		if (get_format_texel_width(format1) != get_format_texel_width(format2))
		{
			return false;
		}

		const auto transform_a = get_format_convert_flags(format1);
		const auto transform_b = get_format_convert_flags(format2);

		if (transform_a.first == transform_b.first)
		{
			return !transform_a.first || (transform_a.second == transform_b.second);
		}

		return false;
	}

	bool formats_are_bitcast_compatible(image* image1, image* image2)
	{
		if (const u32 transfer_class = image1->format_class() | image2->format_class();
			transfer_class & RSX_FORMAT_CLASS_DEPTH_FLOAT_MASK)
		{
			// If any one of the two images is a depth float, the other must match exactly or bust
			return (image1->format_class() == image2->format_class());
		}

		return formats_are_bitcast_compatible(image1->format(), image2->format());
	}
}
