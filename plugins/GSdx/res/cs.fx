#ifndef VS_TME
#define VS_TME 1
#define VS_FST 1
#endif

#ifndef GS_IIP
#define GS_IIP 0
#define GS_PRIM 3
#endif


//
globallycoherent RWByteAddressBuffer VideoMemory : register(u0);

//globallycoherent RWTexture2D<uint> VideoMemory : register(u0); // 8192 * 512 R8_UINT

Buffer<int2> FZBufRow : register(t0);
Buffer<int2> FZBufCol : register(t1);
Texture2D<float4> Palette : register(t2);
Texture2D<float4> TextureL0 : register(t3);
Texture2D<float4> TextureL1 : register(t4);
Texture2D<float4> TextureL2 : register(t5);
Texture2D<float4> TextureL3 : register(t6);
Texture2D<float4> TextureL4 : register(t7);
Texture2D<float4> TextureL5 : register(t8);
Texture2D<float4> TextureL6 : register(t9);

cbuffer VSConstantBuffer : register(c0)
{
	float4 VertexScale;
	float4 VertexOffset;
};

cbuffer PSConstantBuffer : register(c0)
{
	// TODO
};

struct VS_INPUT
{
	uint2 p : POSITION0;
	uint z : POSITION1;
	float2 st : TEXCOORD0;
	float q : TEXCOORD1;
	uint2 uv : TEXCOORD2;
	float4 c : COLOR0;
	float4 f : COLOR1;
};

struct VS_OUTPUT
{
	float4 p : SV_Position;
	float2 z : TEXCOORD0;
	float4 t : TEXCOORD1;
	float4 c : COLOR0;
};

struct GS_OUTPUT
{
	float4 p : SV_Position;
	float2 z : TEXCOORD0;
	float4 t : TEXCOORD1;
	float4 c : COLOR0;
	uint id : SV_PrimitiveID;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.p = float4(input.p, 0.0f, 0.0f) * VertexScale - VertexOffset;

	output.z = float2(input.z & 0xffff, input.z >> 16);

	if(VS_TME)
	{
		if(VS_FST)
		{
			output.t.xy = input.uv;
			output.t.w = 1.0f;
		}
		else
		{
			output.t.xy = input.st;
			output.t.w = input.q;
		}
	}
	else
	{
		output.t.xy = 0;
		output.t.w = 1.0f;
	}

	output.c = input.c;
	output.t.z = input.f.r;

	return output;
}


#if GS_PRIM == 0

[maxvertexcount(1)]
void gs_main(point VS_OUTPUT input[1], inout PointStream<GS_OUTPUT> stream, uint id : SV_PrimitiveID)
{
	GS_OUTPUT output;

	output.p = input[0].p;
	output.z = input[0].z;
	output.t = input[0].t;
	output.c = input[0].c;
	output.id = id;

	stream.Append(output);
}

#elif GS_PRIM == 1

[maxvertexcount(2)]
void gs_main(line VS_OUTPUT input[2], inout LineStream<GS_OUTPUT> stream, uint id : SV_PrimitiveID)
{
	for(int i = 0; i < 2; i++)
	{
		GS_OUTPUT output;

		output.p = input[i].p;
		output.z = input[i].z;
		output.t = input[i].t;
		output.c = input[i].c;
		output.id = id;

		#if GS_IIP == 0
		if(i != 1) output.c = input[1].c;
		#endif

		stream.Append(output);
	}
}

#elif GS_PRIM == 2

[maxvertexcount(3)]
void gs_main(triangle VS_OUTPUT input[3], inout TriangleStream<GS_OUTPUT> stream, uint id : SV_PrimitiveID)
{
	for(int i = 0; i < 3; i++)
	{
		GS_OUTPUT output;

		output.p = input[i].p;
		output.z = input[i].z;
		output.t = input[i].t;
		output.c = input[i].c;
		output.id = id;

		#if GS_IIP == 0
		if(i != 1) output.c = input[2].c;
		#endif

		stream.Append(output);
	}
}

#elif GS_PRIM == 3

[maxvertexcount(4)]
void gs_main(line VS_OUTPUT input[2], inout TriangleStream<GS_OUTPUT> stream, uint id : SV_PrimitiveID)
{
	GS_OUTPUT lt, rb, lb, rt;

	lt.p = input[0].p;
	lt.z = input[1].z;
	lt.t.xy = input[0].t.xy;
	lt.t.zw = input[1].t.zw;
	lt.c = input[0].c;
	lt.id = id;

	#if GS_IIP == 0
	lt.c = input[1].c;
	#endif

	rb.p = input[1].p;
	rb.z = input[1].z;
	rb.t = input[1].t;
	rb.c = input[1].c;
	rb.id = id;

	lb = lt;	
	lb.p.y = rb.p.y;
	lb.t.y = rb.t.y;

	rt = rb;	
	rt.p.y = lt.p.y;
	rt.t.y = lt.t.y;

	stream.Append(lt);
	stream.Append(lb);
	stream.Append(rt);
	stream.Append(rb);
}

#endif

uint CompressColor(float4 f)
{
	// is there a faster way?

	uint4 c = (uint4)(f * 0xff) << uint4(0, 8, 16, 24);

	return c.r | c.g | c.b | c.a;
}

void ps_main(GS_OUTPUT input)
{
	uint c = CompressColor(input.c);
	uint z = (uint)(input.z.y * 0x10000 + input.z.x);

	uint x = (uint)input.p.x;
	uint y = (uint)input.p.y;

	uint2 addr = FZBufRow[y] + FZBufCol[x]; // 16-bit address

	uint2 unaligned = addr.xy & 1; // 16-bit formats can address into the middle of an uint (smallest word size for VideoMemory)

	addr = (addr & ~1) * 2;

	//DeviceMemoryBarrier();

	uint zd = VideoMemory.Load(addr.y);

	if(z < zd) discard;

	VideoMemory.Store(addr.y, z);
	VideoMemory.Store(addr.x, c);

/*
	addr <<= 1;
	
	uint2 fa0 = uint2(addr.x & 0x1fff, addr.x >> 13);
	uint2 fa1 = fa0 + uint2(1, 0);
	uint2 fa2 = fa0 + uint2(2, 0);
	uint2 fa3 = fa0 + uint2(3, 0);

	uint2 za0 = uint2(addr.y & 0x1fff, addr.y >> 13);
	uint2 za1 = za0 + uint2(1, 0);
	uint2 za2 = za0 + uint2(2, 0);
	uint2 za3 = za0 + uint2(3, 0);

	DeviceMemoryBarrier();

	uint zd = 
		(VideoMemory[za0] <<  0) | 
		(VideoMemory[za1] <<  8) | 
		(VideoMemory[za2] << 16) | 
		(VideoMemory[za3] << 24);

	if(zd >= z) discard;

	VideoMemory[za0] = (z >>  0) & 0xff;
	VideoMemory[za1] = (z >>  8) & 0xff;
	VideoMemory[za2] = (z >> 16) & 0xff;
	VideoMemory[za3] = (z >> 24) & 0xff;

	DeviceMemoryBarrier();

	VideoMemory[fa0] = (c >>  0) & 0xff;
	VideoMemory[fa1] = (c >>  8) & 0xff;
	VideoMemory[fa2] = (c >> 16) & 0xff;
	VideoMemory[fa3] = (c >> 24) & 0xff;
*/
}
