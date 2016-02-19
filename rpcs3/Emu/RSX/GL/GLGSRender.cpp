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

	u32 to_gl_internal_type(rsx::vertex_base_type type, u8 size)
	{
		/**
		 * The buffer texture spec only allows fetches aligned to 8, 16, 32, etc...
		 * This rules out most 3-component formats, except for the 32-wide RGB32F, RGB32I, RGB32UI
		 */
		const u32 vec1_types[] = { GL_R16, GL_R32F, GL_R16F, GL_R8, GL_R32I, GL_R16F, GL_R8 };
		const u32 vec2_types[] = { GL_RG16, GL_RG32F, GL_RG16F, GL_RG8, GL_RG32I, GL_RG16F, GL_RG8 };
		const u32 vec3_types[] = { GL_RGBA16, GL_RGB32F, GL_RGBA16F, GL_RGBA8, GL_RGB32I, GL_RGBA16F, GL_RGBA8 };	//VEC3 COMPONENTS NOT SUPPORTED!
		const u32 vec4_types[] = { GL_RGBA16, GL_RGBA32F, GL_RGBA16F, GL_RGBA8, GL_RGBA32I, GL_RGBA16F, GL_RGBA8 };

		const u32* vec_selectors[] = { 0, vec1_types, vec2_types, vec3_types, vec4_types };

		if (type > rsx::vertex_base_type::ub256)
			throw EXCEPTION("OpenGL error: unknown vertex base type 0x%X.", (u32)type);

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
		__glcheck enable(blend_mrt & 2, GL_BLEND, GL_COLOR_ATTACHMENT1);
		__glcheck enable(blend_mrt & 4, GL_BLEND, GL_COLOR_ATTACHMENT2);
		__glcheck enable(blend_mrt & 8, GL_BLEND, GL_COLOR_ATTACHMENT3);
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
			case rsx::vertex_base_type::ub256: gl::buffer_pointer::type::u8;
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
		if (!textures[i].enabled())
		{
			continue;
		}

		int location;
		if (m_program->uniforms.has_location("tex" + std::to_string(i), &location))
		{
			u32 target = GL_TEXTURE_2D;
			if (textures[i].format() & CELL_GCM_TEXTURE_UN)
				target = GL_TEXTURE_RECTANGLE;

			m_gl_textures[i].set_target(target);

			__glcheck m_gl_texture_cache.upload_texture(i, textures[i], m_gl_textures[i]);
			glProgramUniform1i(m_program->id(), location, i);
		}
	}

	//initialize vertex attributes

	//merge all vertex arrays
	std::vector<u8> vertex_arrays_data;
	u32 vertex_arrays_offsets[rsx::limits::vertex_count];

	const std::string reg_table[] =
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

			if (!vertex_info.size) // disabled
				continue;

			int location;
			if (!m_program->uniforms.has_location(reg_table[index] + "_buffer", &location))
				continue;

			const u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
			u32 data_size = element_size * vertex_draw_count;
			u32 gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);

			auto &buffer = m_gl_attrib_buffers[index].buffer;
			auto &texture = m_gl_attrib_buffers[index].texture;

			vertex_arrays_data.resize(data_size);
			u8 *src = reinterpret_cast<u8*>(inline_vertex_array.data());
			u8 *dst = vertex_arrays_data.data();

			src += offsets[index];

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

			void *vertex_data = static_cast<void*>(vertex_arrays_data.data());
			std::vector<float> conversion_buf;

			//Normalize diffuse color and specular color from 0-255 to 0-1; texelFetch does not normalize texels
			if (index == 3 || index == 4)
			{
				if (vertex_info.type == rsx::vertex_base_type::ub ||
					vertex_info.type == rsx::vertex_base_type::s1)
				{
					const u32 num_values = vertex_draw_count * vertex_info.size;
					conversion_buf.resize(num_values);
					u8 *source_values = (u8*)vertex_data;

					for (u32 i = 0; i < num_values; ++i)
					{
						conversion_buf[i] = (float)source_values[i] / 255.f;
					}

					gl_type = to_gl_internal_type(rsx::vertex_base_type::f, vertex_info.size);
					vertex_data = conversion_buf.data();
					data_size *= sizeof(float);
				}
			}

			buffer->data(data_size, nullptr);
			buffer->sub_data(0, data_size, vertex_data);

			//Attach buffer to texture
			texture->copy_from(*buffer, gl_type);

			//Link texture to uniform
			m_program->uniforms.texture(location, index + rsx::limits::vertex_count, *texture);
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
			bool enabled = !!(input_mask & (1 << index));
			if (!enabled)
				continue;

			int location;
			if (!m_program->uniforms.has_location(reg_table[index]+"_buffer", &location))
				continue;

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

				u32 gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);
				u32 data_size = element_size * vertex_draw_count;

				auto &buffer = m_gl_attrib_buffers[index].buffer;
				auto &texture = m_gl_attrib_buffers[index].texture;

				std::chrono::time_point<std::chrono::system_clock> u0 = std::chrono::system_clock::now();

				void *vertex_data = static_cast<void*>(vertex_array.data());
				std::vector<float> conversion_buf;

				//Normalize color inputs if given in ub format
				if (index == 3 || index == 4)
				{
					if (vertex_info.type == rsx::vertex_base_type::ub ||
						vertex_info.type == rsx::vertex_base_type::s1)
					{
						const u32 num_values = vertex_draw_count * vertex_info.size;
						conversion_buf.resize(num_values);
						u8 *source_values = (u8*)vertex_data;

						for (u32 i = 0; i < num_values; ++i)
						{
							conversion_buf[i] = (float)source_values[i] / 255.f;
						}

						gl_type = to_gl_internal_type(rsx::vertex_base_type::f, vertex_info.size);
						vertex_data = conversion_buf.data();
						data_size *= sizeof(float);
					}
				}

				buffer->data(data_size, nullptr);
				buffer->sub_data(0, data_size, vertex_data);

				//Attach buffer to texture
				texture->copy_from(*buffer, gl_type);

				//Link texture to uniform
				m_program->uniforms.texture(location, index + rsx::limits::vertex_count, *texture);
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
					const u32 gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);
					const size_t data_size = vertex_data.size();

					auto &buffer = m_gl_attrib_buffers[index].buffer;
					auto &texture = m_gl_attrib_buffers[index].texture;

					buffer->data(data_size, nullptr);
					buffer->sub_data(0, data_size, vertex_data.data());

					//Attach buffer to texture
					texture->copy_from(*buffer, gl_type);

					//Link texture to uniform
					m_program->uniforms.texture(location, index + rsx::limits::vertex_count, *texture);
					break;
				}
				default:
					LOG_ERROR(RSX, "bad non array vertex data format (type = %d, size = %d)", vertex_info.type, vertex_info.size);
					break;
				}
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

	write_buffers();

	rsx::thread::end();
}

void GLGSRender::set_viewport()
{
	u32 viewport_horizontal = rsx::method_registers[NV4097_SET_VIEWPORT_HORIZONTAL];
	u32 viewport_vertical = rsx::method_registers[NV4097_SET_VIEWPORT_VERTICAL];

	u16 viewport_x = viewport_horizontal & 0xffff;
	u16 viewport_y = viewport_vertical & 0xffff;
	u16 viewport_w = viewport_horizontal >> 16;
	u16 viewport_h = viewport_vertical >> 16;

	u32 scissor_horizontal = rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL];
	u32 scissor_vertical = rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL];
	u16 scissor_x = scissor_horizontal;
	u16 scissor_w = scissor_horizontal >> 16;
	u16 scissor_y = scissor_vertical;
	u16 scissor_h = scissor_vertical >> 16;

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

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_scale_offset_buffer.id());
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_vertex_constants_buffer.id());
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_fragment_constants_buffer.id());

	m_vao.array_buffer = m_vbo;
	m_vao.element_array_buffer = m_ebo;

	for (texture_buffer_pair &attrib_buffer : m_gl_attrib_buffers)
	{
		gl::texture *&tex = attrib_buffer.texture;
		tex = new gl::texture(gl::texture::target::textureBuffer);
		tex->create();
		tex->set_target(gl::texture::target::textureBuffer);

		gl::buffer *&buf = attrib_buffer.buffer;
		buf = new gl::buffer();
		buf->create();
	}

	m_gl_texture_cache.initialize_rtt_cache();
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

	for (texture_buffer_pair &attrib_buffer : m_gl_attrib_buffers)
	{
		gl::texture *&tex = attrib_buffer.texture;
		tex->remove();
		delete tex;
		tex = nullptr;

		gl::buffer *&buf = attrib_buffer.buffer;
		buf->remove();
		delete buf;
		buf = nullptr;
	}
}

void nv4097_clear_surface(u32 arg, GLGSRender* renderer)
{
	//LOG_NOTICE(Log::RSX, "nv4097_clear_surface(0x%x)", arg);

	if ((arg & 0xf3) == 0)
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

	if (arg & 0x1)
	{
		rsx::surface_depth_format surface_depth_format = rsx::to_surface_depth_format((rsx::method_registers[NV4097_SET_SURFACE_FORMAT] >> 5) & 0x7);
		u32 max_depth_value = get_max_depth_value(surface_depth_format);

		u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;

		glDepthMask(GL_TRUE);
		glClearDepth(double(clear_depth) / max_depth_value);
		mask |= GLenum(gl::buffers::depth);
	}

	if (arg & 0x2)
	{
		u8 clear_stencil = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;

		__glcheck glStencilMask(rsx::method_registers[NV4097_SET_STENCIL_MASK]);
		glClearStencil(clear_stencil);

		mask |= GLenum(gl::buffers::stencil);
	}

	if (arg & 0xf0)
	{
		u32 clear_color = rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE];
		u8 clear_a = clear_color >> 24;
		u8 clear_r = clear_color >> 16;
		u8 clear_g = clear_color >> 8;
		u8 clear_b = clear_color;

		glColorMask(((arg & 0x20) ? 1 : 0), ((arg & 0x40) ? 1 : 0), ((arg & 0x80) ? 1 : 0), ((arg & 0x10) ? 1 : 0));
		glClearColor(clear_r / 255.f, clear_g / 255.f, clear_b / 255.f, clear_a / 255.f);

		mask |= GLenum(gl::buffers::color);
	}

	glClear(mask);
	renderer->write_buffers();
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

void GLGSRender::init_buffers(bool skip_reading)
{
	u32 surface_format = rsx::method_registers[NV4097_SET_SURFACE_FORMAT];

	u32 clip_horizontal = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL];
	u32 clip_vertical = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL];

	u32 clip_width = clip_horizontal >> 16;
	u32 clip_height = clip_vertical >> 16;
	u32 clip_x = clip_horizontal;
	u32 clip_y = clip_vertical;

	if (!draw_fbo || m_surface.format != surface_format)
	{
		m_surface.unpack(surface_format);
		m_surface.width = clip_width;
		m_surface.height = clip_height;

		LOG_WARNING(RSX, "surface: %dx%d", clip_width, clip_height);

		draw_fbo.recreate();
		m_draw_tex_depth_stencil.recreate(gl::texture::target::texture2D);

		auto format = surface_color_format_to_gl(m_surface.color_format);

		for (int i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			m_draw_tex_color[i].recreate(gl::texture::target::texture2D);
			__glcheck m_draw_tex_color[i].config()
				.size({ (int)m_surface.width, (int)m_surface.height })
				.type(format.type)
				.format(format.format)
				.swizzle(format.swizzle.r, format.swizzle.g, format.swizzle.b, format.swizzle.a);

			__glcheck m_draw_tex_color[i].pixel_pack_settings().swap_bytes(format.swap_bytes).aligment(1);
			__glcheck m_draw_tex_color[i].pixel_unpack_settings().swap_bytes(format.swap_bytes).aligment(1);

			__glcheck draw_fbo.color[i] = m_draw_tex_color[i];
			__glcheck draw_fbo.check();
		}

		switch (m_surface.depth_format)
		{
		case rsx::surface_depth_format::z16:
		{
			__glcheck m_draw_tex_depth_stencil.config()
				.size({ (int)m_surface.width, (int)m_surface.height })
				.type(gl::texture::type::ushort)
				.format(gl::texture::format::depth)
				.internal_format(gl::texture::internal_format::depth16);

			__glcheck draw_fbo.depth = m_draw_tex_depth_stencil;
			break;
		}

		case rsx::surface_depth_format::z24s8:
		{
			__glcheck m_draw_tex_depth_stencil.config()
				.size({ (int)m_surface.width, (int)m_surface.height })
				.type(gl::texture::type::uint_24_8)
				.format(gl::texture::format::depth_stencil)
				.internal_format(gl::texture::internal_format::depth24_stencil8);

			__glcheck draw_fbo.depth_stencil = m_draw_tex_depth_stencil;
			break;
		}

		default:
		{
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface.depth_format);
			assert(0);
			break;
		}
		}

		__glcheck m_draw_tex_depth_stencil.pixel_pack_settings().aligment(1);
		__glcheck m_draw_tex_depth_stencil.pixel_unpack_settings().aligment(1);
	}

	if (!skip_reading)
	{
		read_buffers();
	}

	set_viewport();

	switch (rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
	{
	case rsx::surface_target::none: break;

	case rsx::surface_target::surface_a:
		__glcheck draw_fbo.draw_buffer(draw_fbo.color[0]);
		break;

	case rsx::surface_target::surface_b:
		__glcheck draw_fbo.draw_buffer(draw_fbo.color[1] );
		break;

	case rsx::surface_target::surfaces_a_b:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1] });
		break;

	case rsx::surface_target::surfaces_a_b_c:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1], draw_fbo.color[2] });
		break;

	case rsx::surface_target::surfaces_a_b_c_d:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1], draw_fbo.color[2], draw_fbo.color[3] });
		break;

	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]);
		break;
	}
}

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

void GLGSRender::read_buffers()
{
	if (!draw_fbo)
		return;

	glDisable(GL_STENCIL_TEST);

	if (rpcs3::state.config.rsx.opengl.read_color_buffers)
	{
		auto color_format = surface_color_format_to_gl(m_surface.color_format);

		auto read_color_buffers = [&](int index, int count)
		{
			u32 width = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
			u32 height = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

			/**
			 * Read color buffers is useless if write buffers is enabled. I havent encountered a case where it is necessary
			 * since the output is usually fed back into the pipeline as a fragment shader input texture
			 * It is included here for completeness
			 */
			for (int i = index; i < index + count; ++i)
			{
				u32 offset = rsx::method_registers[mr_color_offset[i]];
				u32 location = rsx::method_registers[mr_color_dma[i]];
				u32 pitch = rsx::method_registers[mr_color_pitch[i]];

				if (pitch <= 64)
					continue;

				rsx::tiled_region color_buffer = get_tiled_address(offset, location & 0xf);
				u32 texaddr = (u32)((u64)color_buffer.ptr - (u64)vm::base(0));

				bool success = m_gl_texture_cache.explicit_writeback(m_draw_tex_color[i], texaddr, pitch);
				
				//Fall back to slower methods if the image could not be fetched.
				if (!success)
				{
					if (!color_buffer.tile)
					{
						m_draw_tex_color[i].copy_from(color_buffer.ptr, color_format.format, color_format.type);
					}
					else
					{
						u32 range = pitch * height;
						m_gl_texture_cache.remove_in_range(texaddr, range);

						std::unique_ptr<u8[]> buffer(new u8[pitch * height]);
						color_buffer.read(buffer.get(), width, height, pitch);

						__glcheck m_draw_tex_color[i].copy_from(buffer.get(), color_format.format, color_format.type);
					}
				}
			}
		};

		switch (rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
		{
		case rsx::surface_target::none:
			break;

		case rsx::surface_target::surface_a:
			read_color_buffers(0, 1);
			break;

		case rsx::surface_target::surface_b:
			read_color_buffers(1, 1);
			break;

		case rsx::surface_target::surfaces_a_b:
			read_color_buffers(0, 2);
			break;

		case rsx::surface_target::surfaces_a_b_c:
			read_color_buffers(0, 3);
			break;

		case rsx::surface_target::surfaces_a_b_c_d:
			read_color_buffers(0, 4);
			break;
		}
	}

	if (rpcs3::state.config.rsx.opengl.read_depth_buffer)
	{
		//TODO: use pitch
		u32 pitch = rsx::method_registers[NV4097_SET_SURFACE_PITCH_Z];

		if (pitch <= 64)
			return;

		u32 depth_address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA]);
		bool in_cache = m_gl_texture_cache.explicit_writeback(m_draw_tex_depth_stencil, depth_address, pitch);

		if (in_cache)
			return;

		//Read failed. Fall back to slow s/w path...

		auto depth_format = surface_depth_format_to_gl(m_surface.depth_format);
		int pixel_size = get_pixel_size(m_surface.depth_format);
		gl::buffer pbo_depth;

		__glcheck pbo_depth.create(m_surface.width * m_surface.height * pixel_size);
		__glcheck pbo_depth.map([&](GLubyte* pixels)
		{
			u32 depth_address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA]);

			if (m_surface.depth_format == rsx::surface_depth_format::z16)
			{
				u16 *dst = (u16*)pixels;
				const be_t<u16>* src = vm::ps3::_ptr<u16>(depth_address);
				for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}
			else
			{
				u32 *dst = (u32*)pixels;
				const be_t<u32>* src = vm::ps3::_ptr<u32>(depth_address);
				for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}
		}, gl::buffer::access::write);

		__glcheck m_draw_tex_depth_stencil.copy_from(pbo_depth, depth_format.second, depth_format.first);
	}
}

void GLGSRender::write_buffers()
{
	if (!draw_fbo)
		return;

	if (rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		auto color_format = surface_color_format_to_gl(m_surface.color_format);

		auto write_color_buffers = [&](int index, int count)
		{
			u32 width = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
			u32 height = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

			for (int i = index; i < index + count; ++i)
			{
				u32 offset = rsx::method_registers[mr_color_offset[i]];
				u32 location = rsx::method_registers[mr_color_dma[i]];
				u32 pitch = rsx::method_registers[mr_color_pitch[i]];

				if (pitch <= 64)
					continue;

				rsx::tiled_region color_buffer = get_tiled_address(offset, location & 0xf);
				u32 texaddr = (u32)((u64)color_buffer.ptr - (u64)vm::base(0));
				u32 range = pitch * height;

				/**Even tiles are loaded as whole textures during read_buffers from testing.
				 * Need further evaluation to determine correct behavior. Separate paths for both show no difference,
				 * but using the GPU to perform the caching is many times faster.
				 */
				__glcheck m_gl_texture_cache.save_render_target(texaddr, range, m_draw_tex_color[i]);
			}
		};

		switch (rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
		{
		case rsx::surface_target::none:
			break;

		case rsx::surface_target::surface_a:
			write_color_buffers(0, 1);
			break;

		case rsx::surface_target::surface_b:
			write_color_buffers(1, 1);
			break;

		case rsx::surface_target::surfaces_a_b:
			write_color_buffers(0, 2);
			break;

		case rsx::surface_target::surfaces_a_b_c:
			write_color_buffers(0, 3);
			break;

		case rsx::surface_target::surfaces_a_b_c_d:
			write_color_buffers(0, 4);
			break;
		}
	}

	if (rpcs3::state.config.rsx.opengl.write_depth_buffer)
	{
		//TODO: use pitch
		u32 pitch = rsx::method_registers[NV4097_SET_SURFACE_PITCH_Z];

		if (pitch <= 64)
			return;

		auto depth_format = surface_depth_format_to_gl(m_surface.depth_format);
		u32 depth_address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA]);
		u32 range = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height() * 2;

		if (m_surface.depth_format != rsx::surface_depth_format::z16) range *= 2;

		m_gl_texture_cache.save_render_target(depth_address, range, m_draw_tex_depth_stencil);
	}
}

void GLGSRender::flip(int buffer)
{
	//LOG_NOTICE(Log::RSX, "flip(%d)", buffer);
	u32 buffer_width = gcm_buffers[buffer].width;
	u32 buffer_height = gcm_buffers[buffer].height;
	u32 buffer_pitch = gcm_buffers[buffer].pitch;

	rsx::tiled_region buffer_region = get_tiled_address(gcm_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);

	bool skip_read = false;

	/**
	 * Calling read_buffers will overwrite cached content
	 */
	if (draw_fbo)
	{
		skip_read = true;
		/*
		for (uint i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			u32 color_address = rsx::get_address(rsx::method_registers[mr_color_offset[i]], rsx::method_registers[mr_color_dma[i]]);

			if (color_address == buffer_address)
			{
				skip_read = true;
				__glcheck draw_fbo.draw_buffer(draw_fbo.color[i]);
				break;
			}
		}
		*/
	}

	if (!skip_read)
	{
		if (!m_flip_tex_color || m_flip_tex_color.size() != sizei{ (int)buffer_width, (int)buffer_height })
		{
			m_flip_tex_color.recreate(gl::texture::target::texture2D);

			__glcheck m_flip_tex_color.config()
				.size({ (int)buffer_width, (int)buffer_height })
				.type(gl::texture::type::uint_8_8_8_8)
				.format(gl::texture::format::bgra);

			m_flip_tex_color.pixel_unpack_settings().aligment(1).row_length(buffer_pitch / 4);

			__glcheck m_flip_fbo.recreate();
			__glcheck m_flip_fbo.color = m_flip_tex_color;
		}

		__glcheck m_flip_fbo.draw_buffer(m_flip_fbo.color);

		m_flip_fbo.bind();

		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_LOGIC_OP);
		glDisable(GL_CULL_FACE);

		if (buffer_region.tile)
		{
			std::unique_ptr<u8> temp(new u8[buffer_height * buffer_pitch]);
			buffer_region.read(temp.get(), buffer_width, buffer_height, buffer_pitch);
			__glcheck m_flip_tex_color.copy_from(temp.get(), gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8);
		}
		else
		{
			__glcheck m_flip_tex_color.copy_from(buffer_region.ptr, gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8);
		}
	}

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

	if (!skip_read)
	{
		__glcheck m_flip_fbo.blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical());
	}
	else
	{
		__glcheck draw_fbo.blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical());
	}

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
	if (is_writing) return m_gl_texture_cache.mark_as_dirty(address);
	return false;
}