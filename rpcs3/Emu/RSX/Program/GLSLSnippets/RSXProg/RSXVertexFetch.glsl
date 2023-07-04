R"(
#ifdef _INTEL_GLSL
// For intel GPUs which cannot access vectors in indexed mode (driver bug? or glsl version too low?)
// Note: Tested on Mesa iris with HD 530 and compilant path works fine, may be a bug on Windows proprietary drivers
void mov(inout uvec4 vector, const in int index, const in uint scalar)
{
	switch(index)
	{
		case 0: vector.x = scalar; return;
		case 1: vector.y = scalar; return;
		case 2: vector.z = scalar; return;
		case 3: vector.w = scalar; return;
	}
}

uint ref(const in uvec4 vector, const in int index)
{
	switch(index)
	{
		case 0: return vector.x;
		case 1: return vector.y;
		case 2: return vector.z;
		case 3: return vector.w;
	}
}
#else
#define mov(v, i, s) v[i] = s
#define ref(v, i) v[i]
#endif

#ifdef VULKAN
#define _gl_VertexID gl_VertexIndex
#else
#define _gl_VertexID gl_VertexID
#endif

struct attribute_desc
{
	uint type;
	uint attribute_size;
	uint starting_offset;
	uint stride;
	uint frequency;
	bool swap_bytes;
	bool is_volatile;
	bool modulo;
};

uint gen_bits(const in uint x, const in uint y, const in uint z, const in uint w, const in bool swap)
{
	return (swap) ?
		_set_bits(_set_bits(_set_bits(w, z, 8, 8), y, 16, 8), x, 24, 8) :
		_set_bits(_set_bits(_set_bits(x, y, 8, 8), z, 16, 8), w, 24, 8);
}

uint gen_bits(const in uint x, const in uint y, const in bool swap)
{
	return (swap) ? _set_bits(y, x, 8, 8) : _set_bits(x, y, 8, 8);
}

// NOTE: (int(n) or int(n)) is broken on some NVIDIA and INTEL hardware when the sign bit is involved.
// See https://github.com/RPCS3/rpcs3/issues/8990
vec4 sext(const in ivec4 bits)
{
	// convert raw 16 bit values into signed 32-bit float4 counterpart
	bvec4 sign_check = lessThan(bits, ivec4(0x8000));
	return _select(bits - 65536, bits, sign_check);
}

float sext(const in int bits)
{
	return (bits < 0x8000) ? float(bits) : float(bits - 65536); 
}

vec4 fetch_attribute(const in attribute_desc desc, const in int vertex_id, usamplerBuffer input_stream)
{
	const int elem_size_table[] = { 0, 2, 4, 2, 1, 2, 4, 1 };
	const float scaling_table[] = { 1., 32767.5, 1., 1., 255., 1., 32767., 1. };
	const int elem_size = elem_size_table[desc.type];
	const vec4 scale = scaling_table[desc.type].xxxx;

	uvec4 tmp, result = uvec4(0u);
	vec4 ret;
	int n, i = int((vertex_id * desc.stride) + desc.starting_offset);

	for (n = 0; n < desc.attribute_size; n++)
	{
		tmp.x = texelFetch(input_stream, i++).x;
		if (elem_size == 2)
		{
			tmp.y = texelFetch(input_stream, i++).x;
			tmp.x = gen_bits(tmp.x, tmp.y, desc.swap_bytes);
		}
		else if (elem_size == 4)
		{
			tmp.y = texelFetch(input_stream, i++).x;
			tmp.z = texelFetch(input_stream, i++).x;
			tmp.w = texelFetch(input_stream, i++).x;
			tmp.x = gen_bits(tmp.x, tmp.y, tmp.z, tmp.w, desc.swap_bytes);
		}

		mov(result, n, tmp.x);
	}

	// Actual decoding step is done in vector space, outside the loop
	if (desc.type == VTX_FMT_SNORM16 || desc.type == VTX_FMT_SINT16)
	{
		ret = sext(ivec4(result));
		ret = fma(vec4(0.5), vec4(desc.type == VTX_FMT_SNORM16), ret);
	}
	else if (desc.type == VTX_FMT_FLOAT32)
	{
		ret = uintBitsToFloat(result);
	}
	else if (desc.type == VTX_FMT_FLOAT16)
	{
		tmp.x = _set_bits(result.x, result.y, 16, 16);
		tmp.y = _set_bits(result.z, result.w, 16, 16);
		ret.xy = unpackHalf2x16(tmp.x);
		ret.zw = unpackHalf2x16(tmp.y);
	}
	else if (elem_size == 1) // (desc.type == VTX_FMT_UINT8 || desc.type == VTX_FMT_UNORM8)
	{
		// Ignore bswap on single byte channels
		ret = vec4(result);
	}
	else // if (desc.type == VTX_FMT_COMP32)
	{
		result = uvec4(_get_bits(result.x, 0, 11),
			_get_bits(result.x, 11, 11),
			_get_bits(result.x, 22, 10),
			uint(scale.x));
		ret = sext(ivec4(result) << ivec4(5, 5, 6, 0));
	}

	if (desc.attribute_size < 4)
	{
		ret.w = scale.x;
	}

	return ret / scale; 
}

attribute_desc fetch_desc(const in int location)
{
	// Each descriptor is 64 bits wide
	// [0-8] attribute stride
	// [8-24] attribute divisor
	// [24-27] attribute type
	// [27-30] attribute size
	// [30-31] reserved
	// [32-60] starting offset
	// [60-61] swap bytes flag
	// [61-62] volatile flag
	// [62-63] modulo enable flag;

#ifdef VULKAN
	// Fetch parameters streamed separately from draw parameters
	uvec2 attrib = texelFetch(vertex_layout_stream, location + int(layout_ptr_offset)).xy;
#else
	// Data is packed into a ubo
	const int block = (location >> 1);
	const int sub_block = (location & 1) << 1;
	uvec2 attrib = uvec2(
		ref(input_attributes_blob[block], sub_block + 0),
		ref(input_attributes_blob[block], sub_block + 1));
#endif

	attribute_desc result;
	result.stride = _get_bits(attrib.x, 0, 8);
	result.frequency = _get_bits(attrib.x, 8, 16);
	result.type = _get_bits(attrib.x, 24, 3);
	result.attribute_size = _get_bits(attrib.x, 27, 3);
	result.starting_offset = _get_bits(attrib.y, 0, 29);
	result.swap_bytes = _test_bit(attrib.y, 29);
	result.is_volatile = _test_bit(attrib.y, 30);
	result.modulo = _test_bit(attrib.y, 31);
	return result;
}

vec4 read_location(const in int location)
{
	int vertex_id;
	attribute_desc desc = fetch_desc(location);

	if (desc.frequency == 0)
	{
		vertex_id = 0;
	}
	else if (desc.modulo)
	{
		// if a vertex modifier is active; vertex_base must be 0 and is ignored
		vertex_id = (_gl_VertexID + int(vertex_index_offset)) % int(desc.frequency);
	}
	else
	{
		vertex_id = (_gl_VertexID - int(vertex_base_index)) / int(desc.frequency); 
	}

	if (desc.is_volatile)
	{
		return fetch_attribute(desc, vertex_id, volatile_input_stream);
	}
	else
	{
		return fetch_attribute(desc, vertex_id, persistent_input_stream);
	}
}

)"
