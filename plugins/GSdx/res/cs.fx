struct Vertex
{
	float2 st;
	uint c;
	float q;
	uint xy, z;
	uint uv, f;
};

RWByteAddressBuffer VideoMemory : register(u0);

StructuredBuffer<Vertex> VertexBuffer : register(t0);
Buffer<uint> IndexBuffer : register(t1);

Buffer<int> FrameRowOffset : register(t2);
Buffer<int> FrameColOffset : register(t3);
Buffer<int> ZBufRowOffset : register(t4);
Buffer<int> ZBufColOffset : register(t5);

cbuffer DrawingEnvironment : register(c0)
{
	// TODO
};

// one group is 16x8 pixels and one thread does 2 pixels, otherwise could not read-merge-write 16-bit targets safely
// neighburing pixels are next to eachother in memory, at least we don't have to calculate the address twice

// TODO: they say groupshared memory is faster, try unswizzling the corresponding chunk of memory initially (how to do that once by only one thread?) then write-back when finished, unless it was untouched

[numthreads(8, 8, 1)] 
void cs_main(uint3 gid : SV_GroupID, uint3 tid : SV_GroupThreadID) 
{
	uint count;

	IndexBuffer.GetDimensions(count);

	// #if GS_PRIM == 2 (triangle)

	for(uint i = 0; i < count; i += 3)
	{
		Vertex v0 = VertexBuffer[IndexBuffer[i + 0]];
		Vertex v1 = VertexBuffer[IndexBuffer[i + 1]];
		Vertex v2 = VertexBuffer[IndexBuffer[i + 2]];

		uint x = gid.x + tid.x * 2;
		uint y = gid.y + tid.y;

		uint fa = FrameRowOffset[y] + FrameColOffset[x];
		uint za = ZBufRowOffset[y] + ZBufColOffset[x];

		// TODO: quickly reject if x, y is outside the triangle
		// TODO: calculate interpolated values at x, y
		// TODO: run the GS pipeline
		// TODO: repeat for x+1, y
		// TODO: output two pixels (might be better to process a single pixel, more threads, if there is no 16-bit target involved)

		// testing...

		uint4 c = VideoMemory.Load4(fa); // does this load 4*4 bytes? or 4 bytes each expanded uint?

		c = (v0.c >> uint4(0, 8, 16, 24)) & 0xff; // => ushr r1.yzw, r1.xxxx, l(0, 8, 16, 24), v0.c auto-converted to uint4 and per-component shift in one instruction, SSE is embarrassed

		VideoMemory.Store4(fa, c); // same question, 4*4 bytes or compressed to uint
	}

	// #endif
}

// TODO: DrawPoint (this is going to be a waste of resources)
// TODO: DrawLine (line hit-test, will it work?)
// TODO: DrawSprite (similar to DrawTriangle)
// TODO: if read-backs are too slow, implement GSState::Write/FlushWrite/Read/clut.Write in a compute shader
// TODO: unswizzle pages from VideoMemory to the texture cache (if they are marked as valid, otherwise upload from GSLocalMemory::m_vm8)
