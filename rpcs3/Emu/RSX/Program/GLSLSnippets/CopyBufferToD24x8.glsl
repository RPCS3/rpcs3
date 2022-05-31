R"(
#version 430
#extension GL_ARB_shader_stencil_export : enable

layout(%set, binding=%loc, std430) readonly restrict buffer RawDataBlock
{
	uint data[];
};

#if USE_UBO
layout(%push_block) uniform UnpackConfiguration
{
	uint swap_bytes;
	uint src_pitch;
};
#else
	uniform int swap_bytes;
	uniform int src_pitch;
#endif

int getDataOffset()
{
	const ivec2 coords = ivec2(gl_FragCoord.xy);
	return coords.y * src_pitch + coords.x;
}

void main()
{
	const int virtual_address = getDataOffset();
	uint real_data = data[virtual_address];

	const uint stencil_byte = bitfieldExtract(real_data, 0, 8);
	uint depth_bytes;

	if (swap_bytes > 0)
	{
		// CCBBAA00 -> 00AABBCC -> AABBCC. Stencil byte does not actually move
		depth_bytes = bitfieldExtract(real_data, 24, 8) | (bitfieldExtract(real_data, 16, 8) << 8) | (bitfieldExtract(real_data, 8, 8) << 24);
	}
	else
	{
		depth_bytes = bitfieldExtract(real_data, 8, 24);
	}

	gl_FragDepth = float(depth_bytes) / 0xffffff;
	gl_FragStencilRefARB = int(stencil_byte);
}
)"
