R"(
#version 430
layout(local_size_x=%ws, local_size_y=1, local_size_z=1) in;
layout(%set, binding=%loc, std430) buffer ssbo{ uint data[]; };
%ub

#define KERNEL_SIZE %ks

// Generic swap routines
#define bswap_u16(bits)     (bits & 0xFF) << 8 | (bits & 0xFF00) >> 8 | (bits & 0xFF0000) << 8 | (bits & 0xFF000000) >> 8
#define bswap_u32(bits)     (bits & 0xFF) << 24 | (bits & 0xFF00) << 8 | (bits & 0xFF0000) >> 8 | (bits & 0xFF000000) >> 24
#define bswap_u16_u32(bits) (bits & 0xFFFF) << 16 | (bits & 0xFFFF0000) >> 16

// Depth format conversions
#define d24_to_f32(bits)             floatBitsToUint(float(bits) / 16777215.f)
#define f32_to_d24(bits)             uint(uintBitsToFloat(bits) * 16777215.f)
#define d24f_to_f32(bits)            (bits << 7)
#define f32_to_d24f(bits)            (bits >> 7)
#define d24x8_to_f32(bits)           d24_to_f32(bits >> 8)
#define d24x8_to_d24x8_swapped(bits) (bits & 0xFF00) | (bits & 0xFF0000) >> 16 | (bits & 0xFF) << 16
#define f32_to_d24x8_swapped(bits)   d24x8_to_d24x8_swapped(f32_to_d24(bits))

uint linear_invocation_id()
{
	uint size_in_x = (gl_NumWorkGroups.x * gl_WorkGroupSize.x);
	return (gl_GlobalInvocationID.y * size_in_x) + gl_GlobalInvocationID.x;
}

%md
void main()
{
	uint invocation_id = linear_invocation_id();
	uint index = invocation_id * KERNEL_SIZE;
	uint value;
	%vars

)"
