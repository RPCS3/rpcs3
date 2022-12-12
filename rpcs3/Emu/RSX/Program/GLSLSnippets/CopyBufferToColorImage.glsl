R"(
#version 450
layout(local_size_x = %ws, local_size_y = 1, local_size_z = 1) in;

#define IMAGE_LOCATION(x) (x + %image_slot)
#define SSBO_LOCATION(x)  (x + %ssbo_slot)

layout(%set, binding=IMAGE_LOCATION(0)) uniform writeonly restrict image2D output2D;

#define FMT_GL_RGBA8                  0x8058
#define FMT_GL_BGRA8                  0x80E1
#define FMT_GL_R8                     0x8229
#define FMT_GL_R16                    0x822A
#define FMT_GL_R32F                   0x822E
#define FMT_GL_RG8                    0x822B
#define FMT_GL_RG8_SNORM              0x8F95
#define FMT_GL_RG16                   0x822C
#define FMT_GL_RG16F                  0x822F
#define FMT_GL_RGBA16F                0x881A
#define FMT_GL_RGBA32F                0x8814

#define bswap_u16(bits) (bits & 0xFF) << 8 | (bits & 0xFF00) >> 8 | (bits & 0xFF0000) << 8 | (bits & 0xFF000000) >> 8
#define bswap_u32(bits) (bits & 0xFF) << 24 | (bits & 0xFF00) << 8 | (bits & 0xFF0000) >> 8 | (bits & 0xFF000000) >> 24

layout(%set, binding=SSBO_LOCATION(0), std430) readonly restrict buffer RawDataBlock
{
	uint data[];
};

#if USE_UBO
layout(%push_block) uniform UnpackConfiguration
{
	uint swap_bytes;
	uint src_pitch;
	uint format;
	uint reserved;
	ivec2 region_offset;
	ivec2 region_size;
};
#else
	uniform uint swap_bytes;
	uniform uint src_pitch;
	uniform uint format;
	uniform ivec2 region_offset;
	uniform ivec2 region_size;
#endif

uint linear_invocation_id()
{
	uint size_in_x = (gl_NumWorkGroups.x * gl_WorkGroupSize.x);
	return (gl_GlobalInvocationID.y * size_in_x) + gl_GlobalInvocationID.x;
}

ivec2 linear_id_to_output_coord(uint index)
{
	return ivec2(int(index % src_pitch), int(index / src_pitch));
}

// Decoders. Beware of multi-wide swapped types (e.g swap(16x2) != swap(32x1))
uint readUint8(const in uint address)
{
	const uint block = address / 4;
	const uint offset = address % 4;
	return bitfieldExtract(data[block], int(offset) * 8, 8);
}

uint readUint16(const in uint address)
{
	const uint block = address / 2;
	const uint offset = address % 2;
	const uint value = bitfieldExtract(data[block], int(offset) * 16, 16);

	if (swap_bytes != 0)
	{
		return bswap_u16(value);
	}

	return value;
}

uint readUint32(const in uint address)
{
	const uint value = data[address];
	return (swap_bytes != 0) ? bswap_u32(value) : value;
}

uvec2 readUint8x2(const in uint address)
{
	const uint raw = readUint16(address);
	return uvec2(bitfieldExtract(raw, 0, 8), bitfieldExtract(raw, 8, 8));
}

ivec2 readInt8x2(const in uint address)
{
	const ivec2 raw = ivec2(readUint8x2(address));
	return raw - (ivec2(greaterThan(raw, ivec2(127))) * 256);
}

#define readFixed8(address) readUint8(address) / 255.f
#define readFixed8x2(address) readUint8x2(address) / 255.f
#define readFixed8x2Snorm(address) readInt8x2(address) / 127.f

vec4 readFixed8x4(const in uint address)
{
	const uint raw = readUint32(address);
	return uvec4(
		bitfieldExtract(raw, 0, 8),
		bitfieldExtract(raw, 8, 8),
		bitfieldExtract(raw, 16, 8),
		bitfieldExtract(raw, 24, 8)
	) / 255.f;
}

#define readFixed16(address) readUint16(uint(address)) / 65535.f
#define readFixed16x2(address) vec2(readFixed16(address * 2 + 0), readFixed16(address * 2 + 1))
#define readFixed16x4(address) vec4(readFixed16(address * 4 + 0), readFixed16(address * 4 + 1), readFixed16(address * 4 + 2), readFixed16(address * 4 + 3))

#define readFloat16(address) unpackHalf2x16(readUint16(uint(address))).x
#define readFloat16x2(address) vec2(readFloat16(address * 2 + 0), readFloat16(address * 2 + 1))
#define readFloat16x4(address) vec4(readFloat16(address * 4 + 0), readFloat16(address * 4 + 1), readFloat16(address * 4 + 2), readFloat16(address * 4 + 3))

#define readFloat32(address) uintBitsToFloat(readUint32(address))
#define readFloat32x4(address) uintBitsToFloat(uvec4(readUint32(address * 4 + 0), readUint32(address * 4 + 1), readUint32(address * 4 + 2), readUint32(address * 4 + 3)))

#define KERNEL_SIZE %wks

void write_output(const in uint invocation_id)
{
	vec4 outColor;
	uint utmp;

	switch (format)
	{
	// Simple color
	case FMT_GL_RGBA8:
		outColor = readFixed8x4(invocation_id);
		break;
	case FMT_GL_BGRA8:
		outColor = readFixed8x4(invocation_id).bgra;
		break;
	case FMT_GL_R8:
		outColor.r = readFixed8(invocation_id);
		break;
	case FMT_GL_R16:
		outColor.r = readFixed16(invocation_id);
		break;
	case FMT_GL_R32F:
		outColor.r = readFloat32(invocation_id);
		break;
	case FMT_GL_RG8:
		outColor.rg = readFixed8x2(invocation_id);
		break;
	case FMT_GL_RG8_SNORM:
		outColor.rg = readFixed8x2Snorm(invocation_id);
		break;
	case FMT_GL_RG16:
		outColor.rg = readFixed16x2(invocation_id);
		break;
	case FMT_GL_RG16F:
		outColor.rg = readFloat16x2(invocation_id);
		break;
	case FMT_GL_RGBA16F:
		outColor = readFloat16x4(invocation_id);
		break;
	case FMT_GL_RGBA32F:
		outColor = readFloat32x4(invocation_id);
		break;
	}

	const ivec2 coord = linear_id_to_output_coord(invocation_id);
	if (any(greaterThan(coord, region_size)))
	{
		return;
	}

	imageStore(output2D, coord + region_offset, outColor);
}

void main()
{
	uint index = linear_invocation_id() * KERNEL_SIZE;

	for (int loop = 0; loop < KERNEL_SIZE; ++loop, ++index)
	{
		write_output(index);
	}
}
)"
