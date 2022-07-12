R"(
#version 430
#extension GL_ARB_shader_stencil_export : enable

#define ENABLE_DEPTH_STENCIL_LOAD %stencil_export_supported

#define FMT_GL_DEPTH_COMPONENT16      0x81A5
#define FMT_GL_DEPTH_COMPONENT32F     0x8CAC
#define FMT_GL_DEPTH24_STENCIL8       0x88F0
#define FMT_GL_DEPTH32F_STENCIL8      0x8CAD

#define FMT_GL_RGBA8                  0x8058
#define FMT_GL_BGRA8                  0x80E1
#define FMT_GL_RGB565                 0x8D62
#define FMT_GL_RGB5_A1                0x8057
#define FMT_GL_BGR5_A1                0x99F0
#define FMT_GL_RGBA4                  0x8056
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

layout(location=0) out vec4 fragColor;

layout(%set, binding=%loc, std430) readonly restrict buffer RawDataBlock
{
	uint data[];
};

#if USE_UBO
layout(%push_block) uniform UnpackConfiguration
{
	uint swap_bytes;
	uint src_pitch;
	uint format;
};
#else
	uniform uint swap_bytes;
	uniform uint src_pitch;
	uniform uint format;
#endif

uint getTexelOffset()
{
	const ivec2 coords = ivec2(gl_FragCoord.xy);
	return coords.y * src_pitch + coords.x;
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

uvec2 readUint24_8(const in uint address)
{
	const uint raw_value = data[address];
	const uint stencil = bitfieldExtract(raw_value, 0, 8);

	if (swap_bytes != 0)
	{
		const uint depth = min(bswap_u32(raw_value), 0xffffff);
		return uvec2(depth, stencil);
	}

	return uvec2(
		bitfieldExtract(raw_value, 8, 24),
		stencil
	);
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

void main()
{
	const uint texel_address = getTexelOffset();
	uint utmp;
	uvec2 utmp2;

	switch (format)
	{
	// Depth formats
	case FMT_GL_DEPTH_COMPONENT16:
		gl_FragDepth = readFixed16(texel_address);
		break;
	case FMT_GL_DEPTH_COMPONENT32F:
		gl_FragDepth = readFloat16(texel_address);
		break;

#if ENABLE_DEPTH_STENCIL_LOAD

	// Depth-stencil formats. Unsupported on NVIDIA due to missing extensions.
	case FMT_GL_DEPTH24_STENCIL8:
	case FMT_GL_DEPTH32F_STENCIL8:
		utmp2 = readUint24_8(texel_address);
		gl_FragDepth = float(utmp2.x) / 0xffffff;
		gl_FragStencilRefARB = int(utmp2.y);
		break;

#endif

	// Simple color
	case FMT_GL_RGBA8:
		fragColor = readFixed8x4(texel_address);
		break;
	case FMT_GL_BGRA8:
		fragColor = readFixed8x4(texel_address).bgra;
		break;
	case FMT_GL_R8:
		fragColor.r = readFixed8(texel_address);
		break;
	case FMT_GL_R16:
		fragColor.r = readFixed16(texel_address);
		break;
	case FMT_GL_R32F:
		fragColor.r = readFloat32(texel_address);
		break;
	case FMT_GL_RG8:
		fragColor.rg = readFixed8x2(texel_address);
		break;
	case FMT_GL_RG8_SNORM:
		fragColor.rg = readFixed8x2Snorm(texel_address);
		break;
	case FMT_GL_RG16:
		fragColor.rg = readFixed16x2(texel_address);
		break;
	case FMT_GL_RG16F:
		fragColor.rg = readFloat16x2(texel_address);
		break;
	case FMT_GL_RGBA16F:
		fragColor = readFloat16x4(texel_address);
		break;
	case FMT_GL_RGBA32F:
		fragColor = readFloat32x4(texel_address);
		break;

	// Packed color
	case FMT_GL_RGB565:
		utmp = readUint16(texel_address);
		fragColor.b = bitfieldExtract(utmp, 0, 5) / 31.f;
		fragColor.g = bitfieldExtract(utmp, 5, 6) / 63.f;
		fragColor.r = bitfieldExtract(utmp, 11, 5) / 31.f;
		break;
	case FMT_GL_BGR5_A1:
		utmp = readUint16(texel_address);
		fragColor.b = bitfieldExtract(utmp, 0, 5) / 31.f;
		fragColor.g = bitfieldExtract(utmp, 5, 5) / 31.f;
		fragColor.r = bitfieldExtract(utmp, 10, 5) / 31.f;
		fragColor.a = bitfieldExtract(utmp, 15, 1) * 1.f;
		break;
	case FMT_GL_RGB5_A1:
		utmp = readUint16(texel_address);
		fragColor.a = bitfieldExtract(utmp, 0, 1) * 1.f;
		fragColor.b = bitfieldExtract(utmp, 1, 5) / 31.f;
		fragColor.g = bitfieldExtract(utmp, 6, 5) / 31.f;
		fragColor.r = bitfieldExtract(utmp, 11, 5) / 31.f;
		break;
	case FMT_GL_RGBA4:
		utmp = readUint16(texel_address);
		fragColor.b = bitfieldExtract(utmp, 0, 4) / 15.f;
		fragColor.g = bitfieldExtract(utmp, 4, 4) / 15.f;
		fragColor.r = bitfieldExtract(utmp, 8, 4) / 15.f;
		fragColor.a = bitfieldExtract(utmp, 12, 4) / 15.f;
		break;
	}
}
)"
