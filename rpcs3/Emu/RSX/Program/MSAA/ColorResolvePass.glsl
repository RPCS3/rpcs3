R"(
#version 430

layout(local_size_x=%WORKGROUP_SIZE_X, local_size_y=%WORKGROUP_SIZE_Y, local_size_z=1) in;

#ifdef VULKAN
layout(set=0, binding=0, %IMAGE_FORMAT) uniform readonly restrict image2DMS multisampled;
layout(set=0, binding=1) uniform writeonly restrict image2D resolve;
#else
layout(binding=0, %IMAGE_FORMAT) uniform readonly restrict image2DMS multisampled;
layout(binding=1) uniform writeonly restrict image2D resolve;
#endif

#if %BGRA_SWAP
#define shuffle(x) (x.bgra)
#else
#define shuffle(x) (x)
#endif

void main()
{
	ivec2 resolve_size = imageSize(resolve);
	ivec2 aa_size = imageSize(multisampled);
	ivec2 sample_count = resolve_size / aa_size;

	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, uvec2(resolve_size)))) return;

	ivec2 resolve_coords = ivec2(gl_GlobalInvocationID.xy);
	ivec2 aa_coords = resolve_coords / sample_count;
	ivec2 sample_loc = ivec2(resolve_coords % sample_count);
	int sample_index = sample_loc.x + (sample_loc.y * sample_count.y);

	vec4 aa_sample = imageLoad(multisampled, aa_coords, sample_index);
	imageStore(resolve, resolve_coords, shuffle(aa_sample));
}

)"
