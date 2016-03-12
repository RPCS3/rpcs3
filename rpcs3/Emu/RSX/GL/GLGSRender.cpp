#include "stdafx.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "GLGSRender.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"

#define DUMP_VERTEX_DATA 0

namespace
{
	u32 get_max_depth_value(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 0xFFFF;
		case rsx::surface_depth_format::z24s8: return 0xFFFFFF;
		}
		throw EXCEPTION("Unknow depth format");
	}

	u8 get_pixel_size(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 2;
		case rsx::surface_depth_format::z24s8: return 4;
		}
		throw EXCEPTION("Unknow depth format");
	}

	gl::texture::sized_internal_format to_gl_internal_type(rsx::vertex_base_type type, u8 size)
	{
		/**
		 * The buffer texture spec only allows fetches aligned to 8, 16, 32, etc...
		 * This rules out most 3-component formats, except for the 32-wide RGB32F, RGB32I, RGB32UI
		 */
		static const gl::texture::sized_internal_format vec1_types[]
		{
			gl::texture::sized_internal_format::r16,
			gl::texture::sized_internal_format::r32f,
			gl::texture::sized_internal_format::r16f,
			gl::texture::sized_internal_format::r8,
			gl::texture::sized_internal_format::r16i,
			gl::texture::sized_internal_format::r16,
			gl::texture::sized_internal_format::r8ui
		};

		static const gl::texture::sized_internal_format vec2_types[]
		{
			gl::texture::sized_internal_format::rg16,
			gl::texture::sized_internal_format::rg32f,
			gl::texture::sized_internal_format::rg16f,
			gl::texture::sized_internal_format::rg8,
			gl::texture::sized_internal_format::rg16i,
			gl::texture::sized_internal_format::rg16,
			gl::texture::sized_internal_format::rg8ui
		};

		static const gl::texture::sized_internal_format vec3_types[] //VEC3 COMPONENTS NOT SUPPORTED!
		{
			gl::texture::sized_internal_format::rgba16,
			gl::texture::sized_internal_format::rgb32f,
			gl::texture::sized_internal_format::rgba16f,
			gl::texture::sized_internal_format::rgba8,
			gl::texture::sized_internal_format::rgba16i,
			gl::texture::sized_internal_format::rgba16,
			gl::texture::sized_internal_format::rgba8ui
		};

		static const gl::texture::sized_internal_format vec4_types[]
		{
			gl::texture::sized_internal_format::rgba16,
			gl::texture::sized_internal_format::rgba32f,
			gl::texture::sized_internal_format::rgba16f,
			gl::texture::sized_internal_format::rgba8,
			gl::texture::sized_internal_format::rgba16i,
			gl::texture::sized_internal_format::rgba16,
			gl::texture::sized_internal_format::rgba8ui
		};

		static const gl::texture::sized_internal_format* vec_selectors[]
		{
			nullptr,
			vec1_types,
			vec2_types,
			vec3_types,
			vec4_types
		};

		if (type > rsx::vertex_base_type::ub256)
		{
			throw EXCEPTION("OpenGL error: unknown vertex base type 0x%X.", (u32)type);
		}

		return vec_selectors[size][(int)type];
	}
}

GLGSRender::GLGSRender() : GSRender(frame_type::OpenGL)
{
	shaders_cache.load(rsx::shader_language::glsl);
}

u32 GLGSRender::enable(u32 condition, u32 cap)
{
	if (condition)
	{
		glEnable(cap);
	}
	else
	{
		glDisable(cap);
	}

	return condition;
}

u32 GLGSRender::enable(u32 condition, u32 cap, u32 index)
{
	if (condition)
	{
		glEnablei(cap, index);
	}
	else
	{
		glDisablei(cap, index);
	}

	return condition;
}

extern CellGcmContextData current_context;

void GLGSRender::begin()
{
	//LOG_NOTICE(RSX, "begin()");

	rsx::thread::begin();

	if (!load_program())
	{
		//no program - no drawing
		return;
	}

	init_buffers();

	u32 color_mask = rsx::method_registers[NV4097_SET_COLOR_MASK];
	bool color_mask_b = !!(color_mask & 0xff);
	bool color_mask_g = !!((color_mask >> 8) & 0xff);
	bool color_mask_r = !!((color_mask >> 16) & 0xff);
	bool color_mask_a = !!((color_mask >> 24) & 0xff);

	__glcheck glColorMask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
	__glcheck glDepthMask(rsx::method_registers[NV4097_SET_DEPTH_MASK]);
	__glcheck glStencilMask(rsx::method_registers[NV4097_SET_STENCIL_MASK]);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_DEPTH_TEST_ENABLE], GL_DEPTH_TEST))
	{
		__glcheck glDepthFunc(rsx::method_registers[NV4097_SET_DEPTH_FUNC]);
		__glcheck glDepthMask(rsx::method_registers[NV4097_SET_DEPTH_MASK]);
	}

	if (glDepthBoundsEXT && (__glcheck enable(rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE], GL_DEPTH_BOUNDS_TEST_EXT)))
	{
		__glcheck glDepthBoundsEXT((f32&)rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_MIN], (f32&)rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_MAX]);
	}

	__glcheck glDepthRange((f32&)rsx::method_registers[NV4097_SET_CLIP_MIN], (f32&)rsx::method_registers[NV4097_SET_CLIP_MAX]);
	__glcheck enable(rsx::method_registers[NV4097_SET_DITHER_ENABLE], GL_DITHER);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_ALPHA_TEST_ENABLE], GL_ALPHA_TEST))
	{
		//TODO: NV4097_SET_ALPHA_REF must be converted to f32
		//glcheck(glAlphaFunc(rsx::method_registers[NV4097_SET_ALPHA_FUNC], rsx::method_registers[NV4097_SET_ALPHA_REF]));
	}

	if (__glcheck enable(rsx::method_registers[NV4097_SET_BLEND_ENABLE], GL_BLEND))
	{
		u32 sfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR];
		u32 dfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR];
		u16 sfactor_rgb = sfactor;
		u16 sfactor_a = sfactor >> 16;
		u16 dfactor_rgb = dfactor;
		u16 dfactor_a = dfactor >> 16;

		__glcheck glBlendFuncSeparate(sfactor_rgb, dfactor_rgb, sfactor_a, dfactor_a);

		if (m_surface.color_format == rsx::surface_color_format::w16z16y16x16) //TODO: check another color formats
		{
			u32 blend_color = rsx::method_registers[NV4097_SET_BLEND_COLOR];
			u32 blend_color2 = rsx::method_registers[NV4097_SET_BLEND_COLOR2];

			u16 blend_color_r = blend_color;
			u16 blend_color_g = blend_color >> 16;
			u16 blend_color_b = blend_color2;
			u16 blend_color_a = blend_color2 >> 16;

			__glcheck glBlendColor(blend_color_r / 65535.f, blend_color_g / 65535.f, blend_color_b / 65535.f, blend_color_a / 65535.f);
		}
		else
		{
			u32 blend_color = rsx::method_registers[NV4097_SET_BLEND_COLOR];
			u8 blend_color_r = blend_color;
			u8 blend_color_g = blend_color >> 8;
			u8 blend_color_b = blend_color >> 16;
			u8 blend_color_a = blend_color >> 24;

			__glcheck glBlendColor(blend_color_r / 255.f, blend_color_g / 255.f, blend_color_b / 255.f, blend_color_a / 255.f);
		}

		u32 equation = rsx::method_registers[NV4097_SET_BLEND_EQUATION];
		u16 equation_rgb = equation;
		u16 equation_a = equation >> 16;

		__glcheck glBlendEquationSeparate(equation_rgb, equation_a);
	}
	
	if (__glcheck enable(rsx::method_registers[NV4097_SET_STENCIL_TEST_ENABLE], GL_STENCIL_TEST))
	{
		__glcheck glStencilFunc(rsx::method_registers[NV4097_SET_STENCIL_FUNC], rsx::method_registers[NV4097_SET_STENCIL_FUNC_REF],
			rsx::method_registers[NV4097_SET_STENCIL_FUNC_MASK]);
		__glcheck glStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_FAIL], rsx::method_registers[NV4097_SET_STENCIL_OP_ZFAIL],
			rsx::method_registers[NV4097_SET_STENCIL_OP_ZPASS]);

		if (rsx::method_registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE]) {
			__glcheck glStencilMaskSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_MASK]);
			__glcheck glStencilFuncSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC],
				rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC_REF], rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC_MASK]);
			__glcheck glStencilOpSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_FAIL],
				rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL], rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZPASS]);
		}
	}

	__glcheck glShadeModel(rsx::method_registers[NV4097_SET_SHADE_MODE]);

	if (u32 blend_mrt = rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT])
	{
		__glcheck enable(blend_mrt & 2, GL_BLEND, 1);
		__glcheck enable(blend_mrt & 4, GL_BLEND, 2);
		__glcheck enable(blend_mrt & 8, GL_BLEND, 3);
	}
	
	if (__glcheck enable(rsx::method_registers[NV4097_SET_LOGIC_OP_ENABLE], GL_LOGIC_OP))
	{
		__glcheck glLogicOp(rsx::method_registers[NV4097_SET_LOGIC_OP]);
	}

	u32 line_width = rsx::method_registers[NV4097_SET_LINE_WIDTH];
	__glcheck glLineWidth((line_width >> 3) + (line_width & 7) / 8.f);
	__glcheck enable(rsx::method_registers[NV4097_SET_LINE_SMOOTH_ENABLE], GL_LINE_SMOOTH);

	//TODO
	//NV4097_SET_ANISO_SPREAD

	//TODO
	/*
	glcheck(glFogi(GL_FOG_MODE, rsx::method_registers[NV4097_SET_FOG_MODE]));
	f32 fog_p0 = (f32&)rsx::method_registers[NV4097_SET_FOG_PARAMS + 0];
	f32 fog_p1 = (f32&)rsx::method_registers[NV4097_SET_FOG_PARAMS + 1];

	f32 fog_start = (2 * fog_p0 - (fog_p0 - 2) / fog_p1) / (fog_p0 - 1);
	f32 fog_end = (2 * fog_p0 - 1 / fog_p1) / (fog_p0 - 1);

	glFogf(GL_FOG_START, fog_start);
	glFogf(GL_FOG_END, fog_end);
	*/
	//NV4097_SET_FOG_PARAMS

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_POINT_ENABLE], GL_POLYGON_OFFSET_POINT);
	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_LINE_ENABLE], GL_POLYGON_OFFSET_LINE);
	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE], GL_POLYGON_OFFSET_FILL);

	__glcheck glPolygonOffset((f32&)rsx::method_registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR],
		(f32&)rsx::method_registers[NV4097_SET_POLYGON_OFFSET_BIAS]);

	//NV4097_SET_SPECULAR_ENABLE
	//NV4097_SET_TWO_SIDE_LIGHT_EN
	//NV4097_SET_FLAT_SHADE_OP
	//NV4097_SET_EDGE_FLAG

	u32 clip_plane_control = rsx::method_registers[NV4097_SET_USER_CLIP_PLANE_CONTROL];
	u8 clip_plane_0 = clip_plane_control & 0xf;
	u8 clip_plane_1 = (clip_plane_control >> 4) & 0xf;
	u8 clip_plane_2 = (clip_plane_control >> 8) & 0xf;
	u8 clip_plane_3 = (clip_plane_control >> 12) & 0xf;
	u8 clip_plane_4 = (clip_plane_control >> 16) & 0xf;
	u8 clip_plane_5 = (clip_plane_control >> 20) & 0xf;

	//TODO
	if (__glcheck enable(clip_plane_0, GL_CLIP_DISTANCE0)) {}
	if (__glcheck enable(clip_plane_1, GL_CLIP_DISTANCE1)) {}
	if (__glcheck enable(clip_plane_2, GL_CLIP_DISTANCE2)) {}
	if (__glcheck enable(clip_plane_3, GL_CLIP_DISTANCE3)) {}
	if (__glcheck enable(clip_plane_4, GL_CLIP_DISTANCE4)) {}
	if (__glcheck enable(clip_plane_5, GL_CLIP_DISTANCE5)) {}

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE], GL_POLYGON_OFFSET_FILL);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_POLYGON_STIPPLE], GL_POLYGON_STIPPLE))
	{
		__glcheck glPolygonStipple((GLubyte*)(rsx::method_registers + NV4097_SET_POLYGON_STIPPLE_PATTERN));
	}

	__glcheck glPolygonMode(GL_FRONT, rsx::method_registers[NV4097_SET_FRONT_POLYGON_MODE]);
	__glcheck glPolygonMode(GL_BACK, rsx::method_registers[NV4097_SET_BACK_POLYGON_MODE]);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_CULL_FACE_ENABLE], GL_CULL_FACE))
	{
		__glcheck glCullFace(rsx::method_registers[NV4097_SET_CULL_FACE]);
	}

	__glcheck glFrontFace(rsx::method_registers[NV4097_SET_FRONT_FACE] ^ 1);

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_SMOOTH_ENABLE], GL_POLYGON_SMOOTH);

	//NV4097_SET_COLOR_KEY_COLOR
	//NV4097_SET_SHADER_CONTROL
	//NV4097_SET_ZMIN_MAX_CONTROL
	//NV4097_SET_ANTI_ALIASING_CONTROL
	//NV4097_SET_CLIP_ID_TEST_ENABLE

	if (__glcheck enable(rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE], GL_PRIMITIVE_RESTART))
	{
		__glcheck glPrimitiveRestartIndex(rsx::method_registers[NV4097_SET_RESTART_INDEX]);
	}

	if (__glcheck enable(rsx::method_registers[NV4097_SET_LINE_STIPPLE], GL_LINE_STIPPLE))
	{
		u32 line_stipple_pattern = rsx::method_registers[NV4097_SET_LINE_STIPPLE_PATTERN];
		u16 factor = line_stipple_pattern;
		u16 pattern = line_stipple_pattern >> 16;
		__glcheck glLineStipple(factor, pattern);
	}
}

template<typename T, int count>
struct apply_attrib_t;

template<typename T>
struct apply_attrib_t<T, 1>
{
	static void func(gl::glsl::program& program, int location, const T* data)
	{
		program.attribs[location] = data[0];
	}
};

template<typename T>
struct apply_attrib_t<T, 2>
{
	static void func(gl::glsl::program& program, int location, const T* data)
	{
		program.attribs[location] = color2_base<T>{ data[0], data[1] };
	}
};

template<typename T>
struct apply_attrib_t<T, 3>
{
	static void func(gl::glsl::program& program, int location, const T* data)
	{
		program.attribs[location] = color3_base<T>{ data[0], data[1], data[2] };
	}
};
template<typename T>
struct apply_attrib_t<T, 4>
{
	static void func(gl::glsl::program& program, int location, const T* data)
	{
		program.attribs[location] = color4_base<T>{ data[0], data[1], data[2], data[3] };
	}
};


template<typename T, int count>
void apply_attrib_array(gl::glsl::program& program, int location, const std::vector<u8>& data)
{
	for (size_t offset = 0; offset < data.size(); offset += count * sizeof(T))
	{
		apply_attrib_t<T, count>::func(program, location, (T*)(data.data() + offset));
	}
}

namespace
{
	gl::buffer_pointer::type gl_types(rsx::vertex_base_type type)
	{
		switch (type)
		{
			case rsx::vertex_base_type::s1: return gl::buffer_pointer::type::s16;
			case rsx::vertex_base_type::f: return gl::buffer_pointer::type::f32;
			case rsx::vertex_base_type::sf: return gl::buffer_pointer::type::f16;
			case rsx::vertex_base_type::ub: return gl::buffer_pointer::type::u8;
			case rsx::vertex_base_type::s32k: return gl::buffer_pointer::type::s32;
			case rsx::vertex_base_type::cmp: return gl::buffer_pointer::type::s16; // Needs conversion
			case rsx::vertex_base_type::ub256: return gl::buffer_pointer::type::u8;
		}
		throw EXCEPTION("unknow vertex type");
	}

	bool gl_normalized(rsx::vertex_base_type type)
	{
		switch (type)
		{
		case rsx::vertex_base_type::s1:
		case rsx::vertex_base_type::ub:
		case rsx::vertex_base_type::cmp:
			return true;
		case rsx::vertex_base_type::f:
		case rsx::vertex_base_type::sf:
		case rsx::vertex_base_type::ub256:
		case rsx::vertex_base_type::s32k:
			return false;
		}
		throw EXCEPTION("unknow vertex type");
	}
}

void GLGSRender::end()
{
	//LOG_NOTICE(RSX, "end()");

	if (!draw_fbo)
	{
		rsx::thread::end();
		return;
	}

	//LOG_NOTICE(Log::RSX, "draw()");

	draw_fbo.bind();
	m_program->use();

	//setup textures
	for (int i = 0; i < rsx::limits::textures_count; ++i)
	{
		int location;
		if (m_program->uniforms.has_location("tex" + std::to_string(i), &location))
		{
			__glcheck rsx::gl_texture::bind(m_texture_cache, textures[i]);
			__glcheck glProgramUniform1i(m_program->id(), location, i);
		}
	}

	//initialize vertex attributes

	//merge all vertex arrays
	std::vector<u8> vertex_arrays_data;
	u32 vertex_arrays_offsets[rsx::limits::vertex_count];

	static const std::string reg_table[] =
	{
		"in_pos", "in_weight", "in_normal",
		"in_diff_color", "in_spec_color",
		"in_fog",
		"in_point_size", "in_7",
		"in_tc0", "in_tc1", "in_tc2", "in_tc3",
		"in_tc4", "in_tc5", "in_tc6", "in_tc7"
	};

	u32 input_mask = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK];

	std::vector<u8> vertex_index_array;
	vertex_draw_count = 0;
	u32 min_index, max_index;

	if (draw_command == rsx::draw_command::indexed)
	{
		rsx::index_array_type type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);
		u32 type_size = gsl::narrow<u32>(get_index_type_size(type));
		for (const auto& first_count : first_count_commands)
		{
			vertex_draw_count += first_count.second;
		}

		vertex_index_array.resize(vertex_draw_count * type_size);

		switch (type)
		{
		case rsx::index_array_type::u32:
			std::tie(min_index, max_index) = write_index_array_data_to_buffer_untouched(gsl::span<u32>((u32*)vertex_index_array.data(), vertex_draw_count), first_count_commands);
			break;
		case rsx::index_array_type::u16:
			std::tie(min_index, max_index) = write_index_array_data_to_buffer_untouched(gsl::span<u16>((u16*)vertex_index_array.data(), vertex_draw_count), first_count_commands);
			break;
		}
	}

	if (draw_command == rsx::draw_command::inlined_array)
	{
		u32 stride = 0;
		u32 offsets[rsx::limits::vertex_count] = { 0 };

		for (u32 i = 0; i < rsx::limits::vertex_count; ++i)
		{
			const auto &info = vertex_arrays_info[i];
			if (!info.size) continue;

			offsets[i] = stride;
			stride += rsx::get_vertex_type_size_on_host(info.type, info.size);
		}

		vertex_draw_count = (u32)(inline_vertex_array.size() * sizeof(u32)) / stride;

		for (int index = 0; index < rsx::limits::vertex_count; ++index)
		{
			auto &vertex_info = vertex_arrays_info[index];

			int location;
			if (!m_program->uniforms.has_location(reg_table[index] + "_buffer", &location))
				continue;

			if (!vertex_info.size) // disabled, bind a null sampler
			{
				__glcheck glActiveTexture(GL_TEXTURE0 + index + rsx::limits::textures_count);
				__glcheck glBindTexture(GL_TEXTURE_BUFFER, 0);
				__glcheck glProgramUniform1i(m_program->id(), location, index + rsx::limits::textures_count);
				continue;
			}

			const u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
			u32 data_size = element_size * vertex_draw_count;
			gl::texture::sized_internal_format gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);

			auto &buffer = m_gl_attrib_buffers[index].buffer;
			auto &texture = m_gl_attrib_buffers[index].texture;

			vertex_arrays_data.resize(data_size);
			u8 *src = reinterpret_cast<u8*>(inline_vertex_array.data());
			u8 *dst = vertex_arrays_data.data();

			src += offsets[index];

			//TODO: properly handle compressed data
			for (u32 i = 0; i < vertex_draw_count; ++i)
			{
				if (vertex_info.type == rsx::vertex_base_type::ub && vertex_info.size == 4)
				{
					dst[0] = src[3];
					dst[1] = src[2];
					dst[2] = src[1];
					dst[3] = src[0];
				}
				else
					memcpy(dst, src, element_size);

				src += stride;
				dst += element_size;
			}

			buffer.data(data_size, vertex_arrays_data.data());

			//Attach buffer to texture
			texture.copy_from(buffer, gl_type);

			//Link texture to uniform
			m_program->uniforms.texture(location, index + rsx::limits::textures_count, texture);
		}
	}

	if (draw_command == rsx::draw_command::array)
	{
		for (const auto &first_count : first_count_commands)
		{
			vertex_draw_count += first_count.second;
		}
	}

	if (draw_command == rsx::draw_command::array || draw_command == rsx::draw_command::indexed)
	{
		for (int index = 0; index < rsx::limits::vertex_count; ++index)
		{
			int location;
			if (!m_program->uniforms.has_location(reg_table[index]+"_buffer", &location))
				continue;

			bool enabled = !!(input_mask & (1 << index));
			if (!enabled)
			{
				glActiveTexture(GL_TEXTURE0 + index + rsx::limits::textures_count);
				glBindTexture(GL_TEXTURE_BUFFER, 0);
				glProgramUniform1i(m_program->id(), location, index + rsx::limits::textures_count);
				continue;
			}

			if (vertex_arrays_info[index].size > 0)
			{
				auto &vertex_info = vertex_arrays_info[index];
				// Active vertex array
				std::vector<gsl::byte> vertex_array;

				// Fill vertex_array
				u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
				vertex_array.resize(vertex_draw_count * element_size);

				// Get source pointer
				u32 base_offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
				u32 offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
				u32 address = base_offset + rsx::get_address(offset & 0x7fffffff, offset >> 31);
				const gsl::byte *src_ptr = gsl::narrow_cast<const gsl::byte*>(vm::base(address));

				if (draw_command == rsx::draw_command::array)
				{
					size_t offset = 0;
					gsl::span<gsl::byte> dest_span(vertex_array);
					for (const auto &first_count : first_count_commands)
					{
						write_vertex_array_data_to_buffer(dest_span.subspan(offset), src_ptr, first_count.first, first_count.second, vertex_info.type, vertex_info.size, vertex_info.stride);
						offset += first_count.second * element_size;
					}
				}
				if (draw_command == rsx::draw_command::indexed)
				{
					vertex_array.resize((max_index + 1) * element_size);
					gsl::span<gsl::byte> dest_span(vertex_array);
					write_vertex_array_data_to_buffer(dest_span, src_ptr, 0, max_index + 1, vertex_info.type, vertex_info.size, vertex_info.stride);
				}

				size_t size = vertex_array.size();
				size_t position = vertex_arrays_data.size();
				vertex_arrays_offsets[index] = gsl::narrow<u32>(position);
				vertex_arrays_data.resize(position + size);

				gl::texture::sized_internal_format gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);

				auto& attrib_pair = m_gl_attrib_buffers[index];

				attrib_pair.buffer.data(element_size * vertex_draw_count, vertex_array.data());

				//Attach buffer to texture
				attrib_pair.texture.copy_from(attrib_pair.buffer, gl_type);

				//Link texture to uniform
				m_program->uniforms.texture(location, index + rsx::limits::textures_count, attrib_pair.texture);
			}
			else if (register_vertex_info[index].size > 0)
			{
				//Untested!
				auto &vertex_data = register_vertex_data[index];
				auto &vertex_info = register_vertex_info[index];

				switch (vertex_info.type)
				{
				case rsx::vertex_base_type::f:
				{
					const u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
					gl::texture::sized_internal_format gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);

					auto& attrib_pair = m_gl_attrib_buffers[index];

					attrib_pair.buffer.data(vertex_data.size(), vertex_data.data());

					//Attach buffer to texture
					attrib_pair.texture.copy_from(attrib_pair.buffer, gl_type);

					//Link texture to uniform
					m_program->uniforms.texture(location, index + rsx::limits::textures_count, attrib_pair.texture);
					break;
				}
				default:
					LOG_ERROR(RSX, "bad non array vertex data format (type = %d, size = %d)", vertex_info.type, vertex_info.size);
					break;
				}
			}
			else
			{
				glActiveTexture(GL_TEXTURE0 + index + rsx::limits::textures_count);
				glBindTexture(GL_TEXTURE_BUFFER, 0);
				glProgramUniform1i(m_program->id(), location, index + rsx::limits::textures_count);
				continue;
			}
		}
	}

//	glDraw* will fail without at least attrib0 defined if we are on compatibility profile
//	Someone should really test AMD behaviour here, Nvidia is too permissive. There is no buffer currently bound, but on NV it works ok
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);

	/**
	 * Validate fails if called right after linking a program because the VS and FS both use textures bound using different
	 * samplers. So far only sampler2D has been largely used, hiding the problem. This call shall also degrade performance further
	 * if used every draw call. Fixes shader validation issues on AMD.
	 */
	m_program->validate();

	if (draw_command == rsx::draw_command::indexed)
	{
		m_ebo.data(vertex_index_array.size(), vertex_index_array.data());

		rsx::index_array_type indexed_type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);

		if (indexed_type == rsx::index_array_type::u32)
			__glcheck glDrawElements(gl::draw_mode(draw_mode), vertex_draw_count, GL_UNSIGNED_INT, nullptr);
		if (indexed_type == rsx::index_array_type::u16)
			__glcheck glDrawElements(gl::draw_mode(draw_mode), vertex_draw_count, GL_UNSIGNED_SHORT, nullptr);
	}
	else
	{
		draw_fbo.draw_arrays(draw_mode, vertex_draw_count);
	}

	invalidate_buffers();

	rsx::thread::end();

	m_texture_cache.update_protection();
}

void GLGSRender::set_viewport()
{
	u32 viewport_horizontal = rsx::method_registers[NV4097_SET_VIEWPORT_HORIZONTAL];
	u32 viewport_vertical = rsx::method_registers[NV4097_SET_VIEWPORT_VERTICAL];

	u16 viewport_x = viewport_horizontal & 0xffff;
	u16 viewport_y = viewport_vertical & 0xffff;
	u16 viewport_w = (viewport_horizontal >> 16) * m_surface.width_mult;
	u16 viewport_h = (viewport_vertical >> 16) * m_surface.height_mult;

	u32 scissor_horizontal = rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL];
	u32 scissor_vertical = rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL];
	u16 scissor_x = scissor_horizontal;
	u16 scissor_w = (scissor_horizontal >> 16) * m_surface.width_mult;
	u16 scissor_y = scissor_vertical;
	u16 scissor_h = (scissor_vertical >> 16) * m_surface.width_mult;

	u32 shader_window = rsx::method_registers[NV4097_SET_SHADER_WINDOW];

	rsx::window_origin shader_window_origin = rsx::to_window_origin((shader_window >> 12) & 0xf);

	//TODO
	if (true || shader_window_origin == rsx::window_origin::bottom)
	{
		__glcheck glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
		__glcheck glScissor(scissor_x, scissor_y, scissor_w, scissor_h);
	}
	else
	{
		u16 shader_window_height = shader_window & 0xfff;

		__glcheck glViewport(viewport_x, shader_window_height - viewport_y - viewport_h - 1, viewport_w, viewport_h);
		__glcheck glScissor(scissor_x, shader_window_height - scissor_y - scissor_h - 1, scissor_w, scissor_h);
	}

	glEnable(GL_SCISSOR_TEST);
}

void GLGSRender::on_init_thread()
{
	GSRender::on_init_thread();

	gl::init();
	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_VERSION));
	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_VENDOR));

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	m_vao.create();
	m_vbo.create();
	m_ebo.create();
	m_scale_offset_buffer.create(16 * sizeof(float));
	m_vertex_constants_buffer.create(512 * 4 * sizeof(float));
	m_fragment_constants_buffer.create();

	m_flip_fbo.create();
	draw_fbo.create();

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_scale_offset_buffer.id());
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_vertex_constants_buffer.id());
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_fragment_constants_buffer.id());

	m_vao.array_buffer = m_vbo;
	m_vao.element_array_buffer = m_ebo;

	for (texture_buffer_pair &pair : m_gl_attrib_buffers)
	{
		pair.texture.create(gl::texture::target::texture_buffer);
		pair.buffer.create();
	}
}

void GLGSRender::on_exit()
{
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);

	m_prog_buffer.clear();

	if (draw_fbo)
		draw_fbo.remove();

	for (auto &tex : m_draw_tex_color)
		if (tex) tex.remove();

	if (m_draw_tex_depth_stencil)
		m_draw_tex_depth_stencil.remove();

	if (m_flip_fbo)
		m_flip_fbo.remove();

	if (m_flip_tex_color)
		m_flip_tex_color.remove();

	if (m_vbo)
		m_vbo.remove();

	if (m_ebo)
		m_ebo.remove();

	if (m_vao)
		m_vao.remove();

	if (m_scale_offset_buffer)
		m_scale_offset_buffer.remove();

	if (m_vertex_constants_buffer)
		m_vertex_constants_buffer.remove();

	if (m_fragment_constants_buffer)
		m_fragment_constants_buffer.remove();

	for (texture_buffer_pair &pair : m_gl_attrib_buffers)
	{
		pair.buffer.remove();
		pair.texture.remove();
	}
}

void nv4097_clear_surface(u32 arg, GLGSRender* renderer)
{
	//LOG_NOTICE(RSX, "nv4097_clear_surface(0x%x)", arg);

	enum
	{
		depth = 1 << 0,
		stencil = 1 << 1,
		depth_stencil = depth | stencil,
		color_mask = 0xf0,
	};

	if ((arg & (depth_stencil | color_mask)) == 0)
	{
		//do nothing
		return;
	}

	/*
	u16 clear_x = rsx::method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL];
	u16 clear_y = rsx::method_registers[NV4097_SET_CLEAR_RECT_VERTICAL];
	u16 clear_w = rsx::method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL] >> 16;
	u16 clear_h = rsx::method_registers[NV4097_SET_CLEAR_RECT_VERTICAL] >> 16;
	glScissor(clear_x, clear_y, clear_w, clear_h);
	*/

	renderer->init_buffers(true);
	renderer->draw_fbo.bind();

	GLbitfield mask = 0;

	if (renderer->cached_depth_buffer && (arg & depth_stencil))
	{
		if (arg & depth)
		{
			rsx::surface_depth_format surface_depth_format = rsx::to_surface_depth_format((rsx::method_registers[NV4097_SET_SURFACE_FORMAT] >> 5) & 0x7);
			u32 max_depth_value = get_max_depth_value(surface_depth_format);

			u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;

			__glcheck glDepthMask(GL_TRUE);
			__glcheck glClearDepth(double(clear_depth) / max_depth_value);
			mask |= GLenum(gl::buffers::depth);
		}

		if (arg & stencil)
		{
			u8 clear_stencil = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;

			__glcheck glStencilMask(rsx::method_registers[NV4097_SET_STENCIL_MASK]);
			__glcheck glClearStencil(clear_stencil);

			mask |= GLenum(gl::buffers::stencil);
		}

		if (gl::cached_texture* texture = renderer->cached_depth_buffer)
		{
			if ((arg & depth_stencil) == depth_stencil)
			{
				texture->ignore(gl::cache_buffers::local);
				texture->invalidate(gl::cache_buffers::host);
			}
			else
			{
				texture->sync(gl::cache_buffers::local);
			}
		}
	}

	if (arg & color_mask)
	{
		u32 clear_color = rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE];
		u8 clear_a = clear_color >> 24;
		u8 clear_r = clear_color >> 16;
		u8 clear_g = clear_color >> 8;
		u8 clear_b = clear_color;

		glColorMask(((arg & 0x20) ? 1 : 0), ((arg & 0x40) ? 1 : 0), ((arg & 0x80) ? 1 : 0), ((arg & 0x10) ? 1 : 0));
		glClearColor(clear_r / 255.f, clear_g / 255.f, clear_b / 255.f, clear_a / 255.f);

		bool exists_active_color_surface = false;

		rsx::for_each_active_color_surface([&](int index)
		{
			gl::cached_texture* texture = renderer->cached_color_buffers[index];

			if (texture)
			{
				exists_active_color_surface = true;

				if ((arg & color_mask) == color_mask)
				{
					texture->ignore(gl::cache_buffers::local);
					texture->invalidate(gl::cache_buffers::host);
				}
				else
				{
					texture->sync(gl::cache_buffers::local);
				}
			}
		});

		if (exists_active_color_surface)
		{
			mask |= GLenum(gl::buffers::color);
		}
	}

	if (mask)
	{
		__glcheck glClear(mask);
	}
}

using rsx_method_impl_t = void(*)(u32, GLGSRender*);

static const std::unordered_map<u32, rsx_method_impl_t> g_gl_method_tbl =
{
	{ NV4097_CLEAR_SURFACE, nv4097_clear_surface }
};

bool GLGSRender::do_method(u32 cmd, u32 arg)
{
	auto found = g_gl_method_tbl.find(cmd);

	if (found == g_gl_method_tbl.end())
	{
		return false;
	}

	found->second(arg, this);
	return true;
}

bool GLGSRender::load_program()
{
#if 1
	RSXVertexProgram vertex_program = get_current_vertex_program();
	RSXFragmentProgram fragment_program = get_current_fragment_program();

	__glcheck m_program = &m_prog_buffer.getGraphicPipelineState(vertex_program, fragment_program, nullptr);
	__glcheck m_program->use();

#else
	std::vector<u32> vertex_program;
	u32 transform_program_start = rsx::method_registers[NV4097_SET_TRANSFORM_PROGRAM_START];
	vertex_program.reserve((512 - transform_program_start) * 4);

	for (int i = transform_program_start; i < 512; ++i)
	{
		vertex_program.resize((i - transform_program_start) * 4 + 4);
		memcpy(vertex_program.data() + (i - transform_program_start) * 4, transform_program + i * 4, 4 * sizeof(u32));

		D3 d3;
		d3.HEX = transform_program[i * 4 + 3];

		if (d3.end)
			break;
	}

	u32 shader_program = rsx::method_registers[NV4097_SET_SHADER_PROGRAM];

	std::string fp_shader; ParamArray fp_parr; u32 fp_size;
	GLFragmentDecompilerThread decompile_fp(fp_shader, fp_parr,
		rsx::get_address(shader_program & ~0x3, (shader_program & 0x3) - 1), fp_size, rsx::method_registers[NV4097_SET_SHADER_CONTROL]);

	std::string vp_shader; ParamArray vp_parr;
	GLVertexDecompilerThread decompile_vp(vertex_program, vp_shader, vp_parr);
	decompile_fp.Task();
	decompile_vp.Task();

	LOG_NOTICE(RSX, "fp: %s", fp_shader.c_str());
	LOG_NOTICE(RSX, "vp: %s", vp_shader.c_str());

	static bool first = true;
	gl::glsl::shader fp(gl::glsl::shader::type::fragment, fp_shader);
	gl::glsl::shader vp(gl::glsl::shader::type::vertex, vp_shader);

	(m_program.recreate() += { fp.compile(), vp.compile() }).make();
#endif
	size_t max_buffer_sz =(size_t) m_vertex_constants_buffer.size();
	size_t fragment_constants_sz = m_prog_buffer.get_fragment_constants_buffer_size(fragment_program);
	if (fragment_constants_sz > max_buffer_sz)
		max_buffer_sz = fragment_constants_sz;

	std::vector<u8> client_side_buf(max_buffer_sz);

	fill_scale_offset_data(client_side_buf.data(), false);
	m_scale_offset_buffer.data(m_scale_offset_buffer.size(), nullptr);
	m_scale_offset_buffer.sub_data(0, m_scale_offset_buffer.size(), client_side_buf.data());

	fill_vertex_program_constants_data(client_side_buf.data());
	m_vertex_constants_buffer.data(m_vertex_constants_buffer.size(), nullptr);
	m_vertex_constants_buffer.sub_data(0, m_vertex_constants_buffer.size(), client_side_buf.data());

	m_prog_buffer.fill_fragment_constans_buffer({ reinterpret_cast<float*>(client_side_buf.data()), gsl::narrow<int>(fragment_constants_sz) }, fragment_program);
	m_fragment_constants_buffer.data(fragment_constants_sz, nullptr);
	m_fragment_constants_buffer.sub_data(0, fragment_constants_sz, client_side_buf.data());

	return true;
}

struct color_swizzle
{
	gl::texture::channel a = gl::texture::channel::a;
	gl::texture::channel r = gl::texture::channel::r;
	gl::texture::channel g = gl::texture::channel::g;
	gl::texture::channel b = gl::texture::channel::b;

	color_swizzle() = default;
	color_swizzle(gl::texture::channel a, gl::texture::channel r, gl::texture::channel g, gl::texture::channel b)
		: a(a), r(r), g(g), b(b)
	{
	}
};

struct color_format
{
	gl::texture::type type;
	gl::texture::format format;
	bool swap_bytes;
	int channel_count;
	int channel_size;
	color_swizzle swizzle;
};

color_format surface_color_format_to_gl(rsx::surface_color_format color_format)
{
	//color format
	switch (color_format)
	{
	case rsx::surface_color_format::r5g6b5:
		return{ gl::texture::type::ushort_5_6_5, gl::texture::format::bgr, false, 3, 2 };

	case rsx::surface_color_format::a8r8g8b8:
		return{ gl::texture::type::uint_8_8_8_8, gl::texture::format::bgra, false, 4, 1 };

	case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		return{ gl::texture::type::uint_8_8_8_8, gl::texture::format::bgra, false, 4, 1,
		{ gl::texture::channel::one, gl::texture::channel::r, gl::texture::channel::g, gl::texture::channel::b } };

	case rsx::surface_color_format::w16z16y16x16:
		return{ gl::texture::type::f16, gl::texture::format::rgba, true, 4, 2 };

	case rsx::surface_color_format::w32z32y32x32:
		return{ gl::texture::type::f32, gl::texture::format::rgba, true, 4, 4 };

	case rsx::surface_color_format::b8:
	case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
	case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
	case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
	case rsx::surface_color_format::g8b8:
	case rsx::surface_color_format::x32:
	case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
	case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
	case rsx::surface_color_format::a8b8g8r8:
	default:
		LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", color_format);
		return{ gl::texture::type::uint_8_8_8_8, gl::texture::format::bgra, false, 4, 1 };
	}
}

std::pair<gl::texture::type, gl::texture::format> surface_depth_format_to_gl(rsx::surface_depth_format depth_format)
{
	switch (depth_format)
	{
	case rsx::surface_depth_format::z16:
		return std::make_pair(gl::texture::type::ushort, gl::texture::format::depth);

	default:
		LOG_ERROR(RSX, "Surface depth buffer: Unsupported surface depth format (0x%x)", depth_format);
	case rsx::surface_depth_format::z24s8:
		return std::make_pair(gl::texture::type::uint_24_8, gl::texture::format::depth_stencil);
		//return std::make_pair(gl::texture::type::f32, gl::texture::format::depth);
	}
}

[[noreturn]] void unimplemented()
{
	throw EXCEPTION("");
}

u32 surface_format_to_texture_format(rsx::surface_color_format format)
{
	switch (format)
	{
	case rsx::surface_color_format::x1r5g5b5_z1r5g5b5: unimplemented();
	case rsx::surface_color_format::x1r5g5b5_o1r5g5b5: unimplemented();
	case rsx::surface_color_format::r5g6b5: return CELL_GCM_TEXTURE_R5G6B5;
	case rsx::surface_color_format::x8r8g8b8_z8r8g8b8: unimplemented();
	case rsx::surface_color_format::x8r8g8b8_o8r8g8b8: unimplemented();
	case rsx::surface_color_format::a8r8g8b8: return CELL_GCM_TEXTURE_A8R8G8B8;
	case rsx::surface_color_format::b8: return CELL_GCM_TEXTURE_B8;
	case rsx::surface_color_format::g8b8: return CELL_GCM_TEXTURE_G8B8;
	case rsx::surface_color_format::w16z16y16x16: return CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;
	case rsx::surface_color_format::w32z32y32x32: return CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT;
	case rsx::surface_color_format::x32: return CELL_GCM_TEXTURE_X32_FLOAT;
	case rsx::surface_color_format::x8b8g8r8_z8b8g8r8: unimplemented();
	case rsx::surface_color_format::x8b8g8r8_o8b8g8r8: unimplemented();
	case rsx::surface_color_format::a8b8g8r8: unimplemented();
	}

	return CELL_GCM_TEXTURE_A8R8G8B8;
}

gl::texture_info surface_info(rsx::surface_color_format format, u32 offset, u32 location, u32 width, u32 height, u32 pitch)
{
	gl::texture_info info{};

	info.width = width;
	info.height = height;
	info.depth = 1;
	info.pitch = pitch;
	info.target = gl::texture::target::texture2D;
	info.dimension = 2;
	info.start_address = rsx::get_address(offset, location);
	info.mipmap = 1;

	info.format = gl::get_texture_format(surface_format_to_texture_format(format));

	return info;
}

void GLGSRender::init_buffers(bool skip_reading)
{
	static const u32 mr_color_offset[rsx::limits::color_buffers_count] =
	{
		NV4097_SET_SURFACE_COLOR_AOFFSET,
		NV4097_SET_SURFACE_COLOR_BOFFSET,
		NV4097_SET_SURFACE_COLOR_COFFSET,
		NV4097_SET_SURFACE_COLOR_DOFFSET
	};

	static const u32 mr_color_dma[rsx::limits::color_buffers_count] =
	{
		NV4097_SET_CONTEXT_DMA_COLOR_A,
		NV4097_SET_CONTEXT_DMA_COLOR_B,
		NV4097_SET_CONTEXT_DMA_COLOR_C,
		NV4097_SET_CONTEXT_DMA_COLOR_D
	};

	static const u32 mr_color_pitch[rsx::limits::color_buffers_count] =
	{
		NV4097_SET_SURFACE_PITCH_A,
		NV4097_SET_SURFACE_PITCH_B,
		NV4097_SET_SURFACE_PITCH_C,
		NV4097_SET_SURFACE_PITCH_D
	};

	static const gl::texture_view null_texture{ gl::texture::target::texture2D, 0 };

	u32 surface_format = rsx::method_registers[NV4097_SET_SURFACE_FORMAT];

	u32 clip_horizontal = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL];
	u32 clip_vertical = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL];

	u32 clip_width = clip_horizontal >> 16;
	u32 clip_height = clip_vertical >> 16;
	u32 clip_x = clip_horizontal & 0xffff;
	u32 clip_y = clip_vertical & 0xffff;

	m_surface.unpack(surface_format);
	m_surface.width = clip_width * m_surface.width_mult + clip_x;
	m_surface.height = clip_height * m_surface.height_mult + clip_y;

	rsx::for_each_active_color_surface([&](int index)
	{
		u32 offset = rsx::method_registers[mr_color_offset[index]];
		u32 location = rsx::method_registers[mr_color_dma[index]];
		u32 pitch = rsx::method_registers[mr_color_pitch[index]];
		bool swizzled = m_surface.type == CELL_GCM_SURFACE_SWIZZLE;

		gl::texture_info info = surface_info(m_surface.color_format, offset, location, m_surface.width, m_surface.height, pitch);

		info.swizzled = swizzled;

		cached_color_buffers[index] = &m_texture_cache.entry(info, skip_reading ? gl::cache_buffers::none : gl::cache_buffers::local);
		draw_fbo.color[index] = cached_color_buffers[index]->view();
	});

	{
		u32 offset = rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET];
		u32 location = rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA];
		u32 pitch = rsx::method_registers[NV4097_SET_SURFACE_PITCH_Z] & ~63;

		int bpp;
		switch (m_surface.depth_format)
		{
		case rsx::surface_depth_format::z16:
			bpp = 2;
			break;

		case rsx::surface_depth_format::z24s8:
			bpp = 4;
			break;
		}

		if (pitch && pitch < bpp * m_surface.width)
		{
			__glcheck draw_fbo.depth_stencil = null_texture;
			cached_depth_buffer = nullptr;
		}
		else
		{
			if (!pitch)
			{
				pitch = m_surface.width * bpp;
			}

			gl::texture_info info{};

			info.width = m_surface.width;
			info.height = m_surface.height;
			info.depth = 1;
			info.pitch = pitch;
			info.dimension = 2;
			info.start_address = rsx::get_address(offset, location);
			info.target = gl::texture::target::texture2D;
			info.format.bpp = bpp;
			info.mipmap = 1;

			switch (m_surface.depth_format)
			{
			case rsx::surface_depth_format::z16:
				info.format.flags = gl::texture_flags::swap_bytes;
				info.format.type = gl::texture::type::ushort;
				info.format.internal_format = gl::texture::sized_internal_format::depth16;
				info.format.format = gl::texture::format::depth;
				break;

			case rsx::surface_depth_format::z24s8:
				info.format.flags = gl::texture_flags::swap_bytes;
				info.format.type = gl::texture::type::uint_24_8;
				info.format.internal_format = gl::texture::sized_internal_format::depth24_stencil8;
				info.format.format = gl::texture::format::depth_stencil;
				break;
			}

			info.format.remap = { GL_ZERO, GL_ZERO, GL_ZERO, GL_ZERO };

			__glcheck cached_depth_buffer = &m_texture_cache.entry(info, skip_reading ? gl::cache_buffers::none : gl::cache_buffers::local);

			switch (m_surface.depth_format)
			{
			case rsx::surface_depth_format::z16:
				__glcheck draw_fbo.depth = cached_depth_buffer->view();
				__glcheck draw_fbo.stencil = null_texture;
				break;

			case rsx::surface_depth_format::z24s8:
				__glcheck draw_fbo.depth_stencil = cached_depth_buffer->view();
				break;
			}
		}
	}

	__glcheck draw_fbo.bind();

	{
		auto info = rsx::get_active_color_surfaces();

		static const GLenum color_buffers[rsx::limits::color_buffers_count] =
		{
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3
		};

		__glcheck glDrawBuffers(info.second, color_buffers + info.first);
	}

	__glcheck set_viewport();
}

void GLGSRender::invalidate_buffers()
{
	for_each_active_surface([](gl::cached_texture& texture)
	{
		const bool write_buffers_directly = false;

		texture.invalidate(gl::cache_buffers::host);

		if (write_buffers_directly)
		{
			texture.sync(gl::cache_buffers::host);
		}
	});
}

void GLGSRender::flip(int buffer)
{
	//LOG_WARNING(RSX, "flip(%d)", buffer);
	u32 buffer_width = gcm_buffers[buffer].width;
	u32 buffer_height = gcm_buffers[buffer].height;
	u32 buffer_pitch = gcm_buffers[buffer].pitch;

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_LOGIC_OP);
	glDisable(GL_CULL_FACE);

	gl::cached_texture& texture = m_texture_cache.entry(surface_info(rsx::surface_color_format::a8r8g8b8, gcm_buffers[buffer].offset,
		CELL_GCM_LOCATION_LOCAL, buffer_width, buffer_height, buffer_pitch), gl::cache_buffers::local);

	//std::lock_guard<gl::cached_texture> lock(texture);

	m_flip_fbo.bind();
	m_flip_fbo.color = texture.view();
	m_flip_fbo.draw_buffer(m_flip_fbo.color);

	areai screen_area = coordi({}, { (int)buffer_width, (int)buffer_height });

	coordi aspect_ratio;
	if (1) //enable aspect ratio
	{
		sizei csize = m_frame->client_size();
		sizei new_size = csize;

		const double aq = (double)buffer_width / buffer_height;
		const double rq = (double)new_size.width / new_size.height;
		const double q = aq / rq;

		if (q > 1.0)
		{
			new_size.height = int(new_size.height / q);
			aspect_ratio.y = (csize.height - new_size.height) / 2;
		}
		else if (q < 1.0)
		{
			new_size.width = int(new_size.width * q);
			aspect_ratio.x = (csize.width - new_size.width) / 2;
		}

		aspect_ratio.size = new_size;
	}
	else
	{
		aspect_ratio.size = m_frame->client_size();
	}

	gl::screen.clear(gl::buffers::color_depth_stencil);

	__glcheck m_flip_fbo.blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical());

	m_frame->flip(m_context);
}


u64 GLGSRender::timestamp() const
{
	GLint64 result;
	glGetInteger64v(GL_TIMESTAMP, &result);
	return result;
}

bool GLGSRender::on_access_violation(u32 address, bool is_writing)
{
	if (auto region = m_texture_cache.find_region(address))
	{
		//std::lock_guard<gl::protected_region> lock(*region);

		if (is_writing)
		{
			region->for_each([this](gl::cached_texture& texture)
			{
				invoke([&]()
				{
					texture.sync(gl::cache_buffers::host);
					texture.invalidate(gl::cache_buffers::local);
				});
			});

			region->unprotect();
		}
		else
		{
			region->for_each([this](gl::cached_texture& texture)
			{
				invoke([&]()
				{
					texture.sync(gl::cache_buffers::host);
				});
			});

			region->unprotect(gl::cache_access::read);
		}

		return true;
	}

	return false;
}

void GLGSRender::for_each_active_surface(std::function<void(gl::cached_texture& texture)> callback)
{
	rsx::for_each_active_color_surface([&](int index)
	{
		gl::cached_texture *texture = cached_color_buffers[index];

		if (texture)
		{
			callback(*texture);
		}
	});

	if (cached_depth_buffer)
	{
		callback(*cached_depth_buffer);
	}
}
