R"(
#define GAMMA_R_MASK  (1 << GAMMA_R_BIT)
#define GAMMA_G_MASK  (1 << GAMMA_G_BIT)
#define GAMMA_B_MASK  (1 << GAMMA_B_BIT)
#define GAMMA_A_MASK  (1 << GAMMA_A_BIT)
#define EXPAND_R_MASK (1 << EXPAND_R_BIT)
#define EXPAND_G_MASK (1 << EXPAND_G_BIT)
#define EXPAND_B_MASK (1 << EXPAND_B_BIT)
#define EXPAND_A_MASK (1 << EXPAND_A_BIT)
#define SEXT_R_MASK   (1 << SEXT_R_BIT)
#define SEXT_G_MASK   (1 << SEXT_G_BIT)
#define SEXT_B_MASK   (1 << SEXT_B_BIT)
#define SEXT_A_MASK   (1 << SEXT_A_BIT)
#define WRAP_S_MASK   (1 << WRAP_S_BIT)
#define WRAP_T_MASK   (1 << WRAP_T_BIT)
#define WRAP_R_MASK   (1 << WRAP_R_BIT)

#define GAMMA_CTRL_MASK  (GAMMA_R_MASK | GAMMA_G_MASK | GAMMA_B_MASK | GAMMA_A_MASK)
#define SIGN_EXPAND_MASK (EXPAND_R_MASK | EXPAND_G_MASK | EXPAND_B_MASK | EXPAND_A_MASK)
#define SEXT_MASK        (SEXT_R_MASK | SEXT_G_MASK | SEXT_B_MASK | SEXT_A_MASK)
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
#define COORD_PROJ1(index, coord2) COORD_SCALE1(index, coord2.x / coord2.y)
#define COORD_PROJ2(index, coord3) COORD_SCALE2(index, coord3.xy / coord3.z)
#define COORD_PROJ3(index, coord4) COORD_SCALE3(index, coord4.xyz / coord4.w)

#ifdef _ENABLE_TEX1D
#define TEX1D(index, coord1) _process_texel(texture(TEX_NAME(index), COORD_SCALE1(index, coord1)), TEX_FLAGS(index))
#define TEX1D_BIAS(index, coord1, bias) _process_texel(texture(TEX_NAME(index), COORD_SCALE1(index, coord1), bias), TEX_FLAGS(index))
#define TEX1D_LOD(index, coord1, lod) _process_texel(textureLod(TEX_NAME(index), COORD_SCALE1(index, coord1), lod), TEX_FLAGS(index))
#define TEX1D_GRAD(index, coord1, dpdx, dpdy) _process_texel(textureGrad(TEX_NAME(index), COORD_SCALE1(index, coord1), dpdx, dpdy), TEX_FLAGS(index))
#define TEX1D_PROJ(index, coord4) _process_texel(texture(TEX_NAME(index), COORD_PROJ1(index, coord4.xw)), TEX_FLAGS(index))
#endif

#ifdef _ENABLE_TEX2D
#define TEX2D(index, coord2) _process_texel(texture(TEX_NAME(index), COORD_SCALE2(index, coord2)), TEX_FLAGS(index))
#define TEX2D_BIAS(index, coord2, bias) _process_texel(texture(TEX_NAME(index), COORD_SCALE2(index, coord2), bias), TEX_FLAGS(index))
#define TEX2D_LOD(index, coord2, lod) _process_texel(textureLod(TEX_NAME(index), COORD_SCALE2(index, coord2), lod), TEX_FLAGS(index))
#define TEX2D_GRAD(index, coord2, dpdx, dpdy) _process_texel(textureGrad(TEX_NAME(index), COORD_SCALE2(index, coord2), dpdx, dpdy), TEX_FLAGS(index))
#define TEX2D_PROJ(index, coord4) _process_texel(texture(TEX_NAME(index), COORD_PROJ2(index, coord4.xyw)), TEX_FLAGS(index))
#endif

#ifdef _ENABLE_SHADOW
#ifdef _EMULATED_TEXSHADOW
	#define SHADOW_COORD(index, coord3) _texcoord_xform_shadow(coord3, texture_parameters[index])
	#define SHADOW_COORD4(index, coord4) _texcoord_xform_shadow(coord4, texture_parameters[index])
	#define SHADOW_COORD_PROJ(index, coord4) _texcoord_xform_shadow(coord4.xyz / coord4.w, texture_parameters[index])

	#define TEX2D_SHADOW(index, coord3) texture(TEX_NAME(index), SHADOW_COORD(index, coord3))
	#define TEX3D_SHADOW(index, coord4) texture(TEX_NAME(index), SHADOW_COORD4(index, coord4))
	#define TEX2D_SHADOWPROJ(index, coord4) texture(TEX_NAME(index), SHADOW_COORD_PROJ(index, coord4))
#else
	#define COORD_PROJ3_SHADOW(index, coord4) vec3(COORD_PROJ2(index, coord4.xyw), coord4.z / coord4.w)
	#define TEX2D_SHADOW(index, coord3) texture(TEX_NAME(index), vec3(COORD_SCALE2(index, coord3.xy), coord3.z))
	#define TEX3D_SHADOW(index, coord4) texture(TEX_NAME(index), vec4(COORD_SCALE3(index, coord4.xyz), coord4.w))
	#define TEX2D_SHADOWPROJ(index, coord4) texture(TEX_NAME(index), COORD_PROJ3_SHADOW(index, coord4))
#endif
#endif

#ifdef _ENABLE_TEX3D
#define TEX3D(index, coord3) _process_texel(texture(TEX_NAME(index), COORD_SCALE3(index, coord3)), TEX_FLAGS(index))
#define TEX3D_BIAS(index, coord3, bias) _process_texel(texture(TEX_NAME(index), COORD_SCALE3(index, coord3), bias), TEX_FLAGS(index))
#define TEX3D_LOD(index, coord3, lod) _process_texel(textureLod(TEX_NAME(index), COORD_SCALE3(index, coord3), lod), TEX_FLAGS(index))
#define TEX3D_GRAD(index, coord3, dpdx, dpdy) _process_texel(textureGrad(TEX_NAME(index), COORD_SCALE3(index, coord3), dpdx, dpdy), TEX_FLAGS(index))
#define TEX3D_PROJ(index, coord4) _process_texel(texture(TEX_NAME(index), COORD_PROJ3(index, coord4).xyz), TEX_FLAGS(index))
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
	vec2 result = fma(
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

#if defined(_ENABLE_TEX3D) || defined(_ENABLE_SHADOWPROJ)
vec3 _texcoord_xform(const in vec3 coord, const in sampler_info params)
{
	vec3 result = fma(
		coord,
		vec3(params.scale_x, params.scale_y, params.scale_z),
		vec3(params.bias_x, params.bias_y, params.bias_z)
	);

	// NOTE: Coordinate clamping not supported for CUBE and 3D textures
	return result;
}
#endif

#if defined(_ENABLE_SHADOW) && defined(_EMULATED_TEXSHADOW)
#ifdef _ENABLE_TEX2D
vec3 _texcoord_xform_shadow(const in vec3 coord3, const in sampler_info params)
{
	vec3 result;
	if (_test_bit(params.flags, DEPTH_FLOAT))
	{
		// Depth-float buffer, extended range supported
		result.z = coord3.z;
	}
	else
	{
		// Clamp to MAX_DEPTH simulate UINT buffer behavior
		result.z = min(coord3.z, 1.);
	}

	result.xy = _texcoord_xform(coord3.xy, params);
	return result;
}
#endif // TEX2D

#ifdef _ENABLE_TEX3D
vec4 _texcoord_xform_shadow(const in vec4 coord4, const in sampler_info params)
{
	vec4 result;
	if (_test_bit(params.flags, DEPTH_FLOAT))
	{
		// Depth-float buffer, extended range supported
		result.w = coord4.w;
	}
	else
	{
		// Clamp to MAX_DEPTH to simulate UINT buffer behavior
		result.w = min(coord4.w, 1.);
	}

	result.xyz = _texcoord_xform(coord4.xyz, params);
	return result;
}
#endif // TEX3D

#endif // _EMULATE_SHADOW

vec4 _sext_unorm8x4(const in vec4 x)
{
	// TODO: Handle clamped sign-extension
	const vec4 bits = floor(fma(x, vec4(255.f), vec4(0.5f)));
	const bvec4 sign_check = lessThan(bits, vec4(128.f));
	const vec4 ret = _select(bits - 256.f, bits, sign_check);
	return ret / 127.f;
}

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
	if (op_mask != 0u)
	{
		// Expand to signed normalized by decompressing the signal
		mask = uvec4(op_mask) & uvec4(EXPAND_R_MASK, EXPAND_G_MASK, EXPAND_B_MASK, EXPAND_A_MASK);
		convert = (rgba * 2.f - 1.f);
		rgba = _select(rgba, convert, notEqual(mask, uvec4(0)));
	}

	op_mask = control_bits & uint(SEXT_MASK);
	if (op_mask != 0u)
	{
		// Sign-extend the input signal
		mask = uvec4(op_mask) & uvec4(SEXT_R_MASK, SEXT_G_MASK, SEXT_B_MASK, SEXT_A_MASK);
		convert = _sext_unorm8x4(rgba);
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
