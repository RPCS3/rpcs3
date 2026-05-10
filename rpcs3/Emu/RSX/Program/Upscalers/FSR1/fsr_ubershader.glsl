R"(
#version 450

#define A_GPU 1
#define A_GLSL 1

%FFX_DEFINITIONS%

#if defined(SAMPLE_EASU) || defined(SAMPLE_RCAS)
	layout(%push_block%) uniform const_buffer
	{
		uvec4 Const0;
		#if SAMPLE_EASU
			uvec4 Const1;
			uvec4 Const2;
			uvec4 Const3;
		#endif
		uvec4 Sample;
	};
#endif

%FFX_A_IMPORT%

layout(set=0,binding=0) uniform sampler2D InputTexture;
layout(set=0,binding=1,rgba8) uniform writeonly image2D OutputTexture;

#if A_HALF
	#if SAMPLE_EASU
		#define FSR_EASU_H 1
		AH4 FsrEasuRH(AF2 p) { AH4 res = AH4(textureGather(InputTexture, p, 0)); return res; }
		AH4 FsrEasuGH(AF2 p) { AH4 res = AH4(textureGather(InputTexture, p, 1)); return res; }
		AH4 FsrEasuBH(AF2 p) { AH4 res = AH4(textureGather(InputTexture, p, 2)); return res; }
	#endif
	#if SAMPLE_RCAS
		#define FSR_RCAS_H
		AH4 FsrRcasLoadH(ASW2 p) { return AH4(texelFetch(InputTexture, ASU2(p), 0)); }
		void FsrRcasInputH(inout AH1 r,inout AH1 g,inout AH1 b){}
	#endif
#else
	#if SAMPLE_EASU
		#define FSR_EASU_F 1
		AF4 FsrEasuRF(AF2 p) { AF4 res = textureGather(InputTexture, p, 0); return res; }
		AF4 FsrEasuGF(AF2 p) { AF4 res = textureGather(InputTexture, p, 1); return res; }
		AF4 FsrEasuBF(AF2 p) { AF4 res = textureGather(InputTexture, p, 2); return res; }
	#endif
	#if SAMPLE_RCAS
		#define FSR_RCAS_F
		AF4 FsrRcasLoadF(ASU2 p) { return texelFetch(InputTexture, ASU2(p), 0); }
		void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}
	#endif
#endif

#if defined(SAMPLE_EASU) || defined(SAMPLE_RCAS)
%FFX_FSR_IMPORT%
#endif

void CurrFilter(AU2 pos)
{
#if SAMPLE_BILINEAR
	AF2 pp = (AF2(pos) * AF2_AU2(Const0.xy) + AF2_AU2(Const0.zw)) * AF2_AU2(Const1.xy) + AF2(0.5, -0.5) * AF2_AU2(Const1.zw);
	imageStore(OutputTexture, ASU2(pos), textureLod(InputTexture, pp, 0.0));
#endif
#if SAMPLE_EASU
	#if SAMPLE_SLOW_FALLBACK
		AF3 c;
		FsrEasuF(c, pos, Const0, Const1, Const2, Const3);
		if( Sample.x == 1 )
			c *= c;
		imageStore(OutputTexture, ASU2(pos), AF4(c, 1));
	#else
		AH3 c;
		FsrEasuH(c, pos, Const0, Const1, Const2, Const3);
		if( Sample.x == 1 )
			c *= c;
		imageStore(OutputTexture, ASU2(pos), AH4(c, 1));
	#endif
#endif
#if SAMPLE_RCAS
	#if SAMPLE_SLOW_FALLBACK
		AF3 c;
		FsrRcasF(c.r, c.g, c.b, pos, Const0);
		if( Sample.x == 1 )
			c *= c;
		imageStore(OutputTexture, ASU2(pos), AF4(c, 1));
	#else
		AH3 c;
		FsrRcasH(c.r, c.g, c.b, pos, Const0);
		if( Sample.x == 1 )
			c *= c;
		imageStore(OutputTexture, ASU2(pos), AH4(c, 1));
	#endif
#endif
}

layout(local_size_x=64) in;
void main()
{
	// Do remapping of local xy in workgroup for a more PS-like swizzle pattern.
	AU2 gxy = ARmp8x8(gl_LocalInvocationID.x) + AU2(gl_WorkGroupID.x << 4u, gl_WorkGroupID.y << 4u);
	CurrFilter(gxy);
	gxy.x += 8u;
	CurrFilter(gxy);
	gxy.y += 8u;
	CurrFilter(gxy);
	gxy.x -= 8u;
	CurrFilter(gxy);
}
)"
