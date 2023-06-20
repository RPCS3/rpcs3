R"(
#define GAMMA_R_MASK  (1 << GAMMA_R_BIT)
#define GAMMA_G_MASK  (1 << GAMMA_G_BIT)
#define GAMMA_B_MASK  (1 << GAMMA_B_BIT)
#define GAMMA_A_MASK  (1 << GAMMA_A_BIT)
#define EXPAND_R_MASK (1 << EXPAND_R_BIT)
#define EXPAND_G_MASK (1 << EXPAND_G_BIT)
#define EXPAND_B_MASK (1 << EXPAND_B_BIT)
#define EXPAND_A_MASK (1 << EXPAND_A_BIT)

#define GAMMA_CTRL_MASK  (GAMMA_R_MASK | GAMMA_G_MASK | GAMMA_B_MASK | GAMMA_A_MASK)
#define SIGN_EXPAND_MASK (EXPAND_R_MASK | EXPAND_G_MASK | EXPAND_B_MASK | EXPAND_A_MASK)
#define FILTERED_MASK    (FILTERED_MAG_BIT | FILTERED_MIN_BIT)

#ifdef _ENABLE_TEXTURE_EXPAND
	uint _texture_flag_override = 0;
	#define _enable_texture_expand() _texture_flag_override = SIGN_EXPAND_MASK
	#define _disable_texture_expand() _texture_flag_override = 0
	#define TEX_FLAGS(index) (texture_parameters[index].flags | _texture_flag_override)
#else
	#define TEX_FLAGS(index) texture_parameters[index].flags
#endif

#define TEX_NAME(index) tex##index
#define TEX_NAME_STENCIL(index) tex##index##_stencil

#define COORD_SCALE1(index, coord1) _texcoord_xform(coord1, texture_parameters[index])
#define COORD_SCALE2(index, coord2) _texcoord_xform(coord2, texture_parameters[index])
#define COORD_SCALE3(index, coord3) _texcoord_xform(coord3, texture_parameters[index])

#ifdef _ENABLE_TEX1D
#define TEX1D(index, coord1) _process_texel(texture(TEX_NAME(index), COORD_SCALE1(index, coord1)), TEX_FLAGS(index))
#define TEX1D_BIAS(index, coord1, bias) _process_texel(texture(TEX_NAME(index), COORD_SCALE1(index, coord1), bias), TEX_FLAGS(index))
#define TEX1D_LOD(index, coord1, lod) _process_texel(textureLod(TEX_NAME(index), COORD_SCALE1(index, coord1), lod), TEX_FLAGS(index))
#define TEX1D_GRAD(index, coord1, dpdx, dpdy) _process_texel(textureGrad(TEX_NAME(index), COORD_SCALE1(index, coord1), dpdx, dpdy), TEX_FLAGS(index))
#define TEX1D_PROJ(index, coord4) _process_texel(textureProj(TEX_NAME(index), vec2(COORD_SCALE1(index, coord4.x), coord4.w)), TEX_FLAGS(index))
#endif

#ifdef _ENABLE_TEX2D
#define TEX2D(index, coord2) _process_texel(texture(TEX_NAME(index), COORD_SCALE2(index, coord2)), TEX_FLAGS(index))
#define TEX2D_BIAS(index, coord2, bias) _process_texel(texture(TEX_NAME(index), COORD_SCALE2(index, coord2), bias), TEX_FLAGS(index))
#define TEX2D_LOD(index, coord2, lod) _process_texel(textureLod(TEX_NAME(index), COORD_SCALE2(index, coord2), lod), TEX_FLAGS(index))
#define TEX2D_GRAD(index, coord2, dpdx, dpdy) _process_texel(textureGrad(TEX_NAME(index), COORD_SCALE2(index, coord2), dpdx, dpdy), TEX_FLAGS(index))
#define TEX2D_PROJ(index, coord4) _process_texel(textureProj(TEX_NAME(index), vec4(COORD_SCALE2(index, coord4.xy), coord4.z, coord4.w)), TEX_FLAGS(index))
#endif

#ifdef _ENABLE_SHADOW
#ifdef _EMULATED_TEXSHADOW
	#define SHADOW_COORD(index, coord3) vec3(COORD_SCALE2(index, coord3.xy), _test_bit(TEX_FLAGS(index), DEPTH_FLOAT)? coord3.z : min(float(coord3.z), 1.0))
	#define SHADOW_COORD4(index, coord4) vec4(SHADOW_COORD(index, coord4.xyz), coord4.w)
	#define SHADOW_COORD_PROJ(index, coord4) vec4(COORD_SCALE2(index, coord4.xy), _test_bit(TEX_FLAGS(index), DEPTH_FLOAT)? coord4.z : min(coord4.z, coord4.w), coord4.w)

	#define TEX2D_SHADOW(index, coord3) texture(TEX_NAME(index), SHADOW_COORD(index, coord3))
	#define TEX3D_SHADOW(index, coord4) texture(TEX_NAME(index), SHADOW_COORD4(index, coord4))
	#define TEX2D_SHADOWPROJ(index, coord4) textureProj(TEX_NAME(index), SHADOW_COORD_PROJ(index, coord4))
#else
	#define TEX2D_SHADOW(index, coord3) texture(TEX_NAME(index), vec3(COORD_SCALE2(index, coord3.xy), coord3.z))
	#define TEX3D_SHADOW(index, coord4) texture(TEX_NAME(index), vec4(COORD_SCALE3(index, coord4.xyz), coord4.w))
	#define TEX2D_SHADOWPROJ(index, coord4) textureProj(TEX_NAME(index), vec4(COORD_SCALE2(index, coord4.xy), coord4.zw))
#endif
#endif

#ifdef _ENABLE_TEX3D
#define TEX3D(index, coord3) _process_texel(texture(TEX_NAME(index), COORD_SCALE3(index, coord3)), TEX_FLAGS(index))
#define TEX3D_BIAS(index, coord3, bias) _process_texel(texture(TEX_NAME(index), COORD_SCALE3(index, coord3), bias), TEX_FLAGS(index))
#define TEX3D_LOD(index, coord3, lod) _process_texel(textureLod(TEX_NAME(index), COORD_SCALE3(index, coord3), lod), TEX_FLAGS(index))
#define TEX3D_GRAD(index, coord3, dpdx, dpdy) _process_texel(textureGrad(TEX_NAME(index), COORD_SCALE3(index, coord3), dpdx, dpdy), TEX_FLAGS(index))
#define TEX3D_PROJ(index, coord4) _process_texel(texture(TEX_NAME(index), COORD_SCALE3(index, coord4.xyz) / coord4.w), TEX_FLAGS(index))
#endif

#ifdef _ENABLE_TEX1D
float _texcoord_xform(const in float coord, const in sampler_info params)
{
	float result = fma(coord, params.scale_x, params.bias_x);
	if (_test_bit(params.flags, CLAMP_COORDS_BIT))
	{
		result = clamp(result, params.clamp_min_x, params.clamp_max_x);
	}

	return result;
}
#endif

#ifdef _ENABLE_TEX2D
vec2 _texcoord_xform(const in vec2 coord, const in sampler_info params)
{
	float result = fma(
		coord,
		vec2(params.scale_x, params.scale_y),
		vec2(params.bias_x, params.bias_y)
	);

	if (_test_bit(params.flags, CLAMP_COORDS_BIT))
	{
		result = clamp(
			result,
			vec2(params.clamp_min_x, params.clamp_min_y),
			vec2(params.clamp_max_x, params.clamp_max_y)
		);
	}

	return result;
}
#endif

#ifdef _ENABLE_TEX3D
vec3 _texcoord_xform(const in vec3 coord, const in sampler_info params)
{
	float result = fma(
		coord,
		vec3(params.scale_x, params.scale_y, params.scale_z),
		vec3(params.bias_x, params.bias_y, params.bias_z)
	);

	// NOTE: Coordinate clamping not supported for CUBE and 3D textures
	return result;
}
#endif

vec4 _process_texel(in vec4 rgba, const in uint control_bits)
{
	if (control_bits == 0)
	{
		return rgba;
	}

	if (_test_bit(control_bits, ALPHAKILL))
	{
		// Alphakill
		if (rgba.a < 0.000001)
		{
			_kill();
			return rgba;
		}
	}

	if (_test_bit(control_bits, RENORMALIZE))
	{
		// Renormalize to 8-bit (PS3) accuracy
		rgba = floor(rgba * 255.);
		rgba /= 255.;
	}

	uvec4 mask;
	vec4 convert;
	uint op_mask = control_bits & uint(SIGN_EXPAND_MASK);

	if (op_mask != 0)
	{
		// Expand to signed normalized
		mask = uvec4(op_mask) & uvec4(EXPAND_R_MASK, EXPAND_G_MASK, EXPAND_B_MASK, EXPAND_A_MASK);
		convert = (rgba * 2.f - 1.f);
		rgba = _select(rgba, convert, notEqual(mask, uvec4(0)));
	}

	op_mask = control_bits & uint(GAMMA_CTRL_MASK);
	if (op_mask != 0u)
	{
		// Gamma correction
		mask = uvec4(op_mask) & uvec4(GAMMA_R_MASK, GAMMA_G_MASK, GAMMA_B_MASK, GAMMA_A_MASK);
		convert = srgb_to_linear(rgba);
		return _select(rgba, convert, notEqual(mask, uvec4(0)));
	}

	return rgba;
}

)"
