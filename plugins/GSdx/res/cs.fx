#ifndef VS_TME
#define VS_TME 1
#define VS_FST 1
#endif

#ifndef GS_IIP
#define GS_IIP 0
#define GS_PRIM 2
#endif

#ifndef PS_BATCH_SIZE
#define PS_BATCH_SIZE 2048
#define PS_FPSM PSM_PSMCT32
#define PS_ZPSM PSM_PSMZ16
#endif

#define PSM_PSMCT32		0
#define PSM_PSMCT24		1
#define PSM_PSMCT16		2
#define PSM_PSMCT16S	10
#define PSM_PSMT8		19
#define PSM_PSMT4		20
#define PSM_PSMT8H		27
#define PSM_PSMT4HL		36
#define PSM_PSMT4HH		44
#define PSM_PSMZ32		48
#define PSM_PSMZ24		49
#define PSM_PSMZ16		50
#define PSM_PSMZ16S		58

struct VS_INPUT
{
	float2 st : TEXCOORD0;
	float4 c : COLOR0;
	float q : TEXCOORD1;
	uint2 p : POSITION0;
	uint z : POSITION1;
	uint2 uv : TEXCOORD2;
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

cbuffer VSConstantBuffer : register(c0)
{
	float4 VertexScale;
	float4 VertexOffset;
};

cbuffer PSConstantBuffer : register(c0)
{
	uint2 WriteMask;
};

struct FragmentLinkItem
{
    uint c, z, id, next;
};

RWByteAddressBuffer VideoMemory : register(u0);
RWStructuredBuffer<FragmentLinkItem> FragmentLinkBuffer : register(u1);
RWByteAddressBuffer StartOffsetBuffer : register(u2);
//RWTexture2D<uint> VideoMemory : register(u2); // 8192 * 512 R8_UINT

Buffer<int2> FZRowOffset : register(t0);
Buffer<int2> FZColOffset : register(t1);
Texture2D<float4> Palette : register(t2);
Texture2D<float4> Texture : register(t3);

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.p = float4(input.p, 0.0f, 0.0f) * VertexScale - VertexOffset;
	output.z = float2(input.z & 0xffff, input.z >> 16); // TODO: min(input.z, 0xffffff00) ?

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
	[unroll]
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
	[unroll]
	for(int i = 0; i < 3; i++)
	{
		GS_OUTPUT output;

		output.p = input[i].p;
		output.z = input[i].z;
		output.t = input[i].t;
		output.c = input[i].c;
		output.id = id;

		#if GS_IIP == 0
		if(i != 2) output.c = input[2].c;
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

uint CompressColor32(float4 f)
{
	uint4 c = (uint4)(f * 0xff) << uint4(0, 8, 16, 24);

	return c.r | c.g | c.b | c.a;
}

uint DecompressColor16(uint c)
{
	uint r = (c & 0x001f) << 3;
	uint g = (c & 0x03e0) << 6;
	uint b = (c & 0x7c00) << 9;
	uint a = (c & 0x8000) << 15;

	return r | g | b | a;
}

uint ReadPixel(uint addr)
{
	return VideoMemory.Load(addr) >> ((addr & 2) << 3);
}

void WritePixel(uint addr, uint value, uint psm)
{
	uint tmp;

	switch(psm)
	{
	case PSM_PSMCT32:
	case PSM_PSMZ32:
	case PSM_PSMCT24:
	case PSM_PSMZ24:
		VideoMemory.Store(addr, value);
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMZ16:
	case PSM_PSMZ16S:
		tmp = (addr & 2) << 3;
		value = ((value << tmp) ^ VideoMemory.Load(addr)) & (0x0000ffff << tmp);
		VideoMemory.InterlockedXor(addr, value, tmp);
		break;
	}
}

void ps_main0(GS_OUTPUT input)
{
	uint x = (uint)input.p.x;
	uint y = (uint)input.p.y;

    uint tail = FragmentLinkBuffer.IncrementCounter();

    uint index = (y << 11) + x;
	uint next = 0;

	StartOffsetBuffer.InterlockedExchange(index * 4, tail, next);

    FragmentLinkItem item;
	
	// TODO: preprocess color (tfx, alpha test), z-test

	item.c = CompressColor32(input.c);
	item.z = (uint)(input.z.y * 0x10000 + input.z.x);
	item.id = input.id;
    item.next = next;

    FragmentLinkBuffer[tail] = item;
}

void ps_main1(GS_OUTPUT input)
{
	uint2 pos = (uint2)input.p.xy;

	// sort fragments

    uint StartOffsetIndex = (pos.y << 11) + pos.x;

	int index[PS_BATCH_SIZE];
	int count = 0;

	uint next = StartOffsetBuffer.Load(StartOffsetIndex * 4);

	StartOffsetBuffer.Store(StartOffsetIndex * 4, 0);

	[allow_uav_condition]
    while(next != 0)
	{
		index[count++] = next;

		next = FragmentLinkBuffer[next].next;
	}

	int N2 = 1 << (int)(ceil(log2(count)));

	[allow_uav_condition]
    for(int i = count; i < N2; i++)
    {
		index[i] = 0;
	}

	[allow_uav_condition]
	for(int k = 2; k <= N2; k = 2 * k)
	{
		[allow_uav_condition]
		for(int j = k >> 1; j > 0 ; j = j >> 1) 
		{
			[allow_uav_condition]
			for(int i = 0; i < N2; i++) 
			{
				uint i_id = FragmentLinkBuffer[index[i]].id;

				int ixj = i ^ j;
				
				if(ixj > i)
				{
					uint ixj_id = FragmentLinkBuffer[index[ixj]].id;

					if((i & k) == 0 && i_id > ixj_id)
					{ 
						int temp = index[i];
						index[i] = index[ixj];
						index[ixj] = temp;
					}

					if((i & k) != 0 && i_id < ixj_id)
					{
						int temp = index[i];
						index[i] = index[ixj];
						index[ixj] = temp;
					}
				}
			}
		}
	}

	uint2 addr = (uint2)(FZRowOffset[pos.y] + FZColOffset[pos.x]) << 1;

	uint dc = ReadPixel(addr.x);
	uint dz = ReadPixel(addr.y);

	uint sc = dc;
	uint sz = dz;

	[allow_uav_condition]
	while(--count >= 0)
    {
		FragmentLinkItem f = FragmentLinkBuffer[index[count]];

		// TODO

		if(sz < f.z)
		{
			sc = f.c;
			sz = f.z;
		}
	}
	
	uint c = sc; // (dc & ~WriteMask.x) | (sc & WriteMask.x);
	uint z = 0;//sz; //(dz & ~WriteMask.y) | (sz & WriteMask.y);

	WritePixel(addr.x, c, PS_FPSM);
	WritePixel(addr.y, z, PS_ZPSM);
}
