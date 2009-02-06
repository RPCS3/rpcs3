#include <Cg/cg.h>
#include <Cg/cgGL.h>

#define NUM_FILTERS 2 // texture filtering
#define NUM_TYPES 5 // types of texture read modes
#define NUM_TEXWRAPS 4 // texture wrapping

#define SHADER_REDUCED 1	// equivalent to ps2.0
#define SHADER_ACCURATE 2   // for older cards with less accurate math (ps2.x+)

#define NUM_SHADERS (NUM_FILTERS*NUM_TYPES*NUM_TEXWRAPS*32) // # shaders for a given ps

const static char* g_pShaders[] = { "full", "reduced", "accurate", "accurate-reduced" };
const static char* g_pPsTexWrap[] = { "-DREPEAT", "-DCLAMP", "-DREGION_REPEAT", NULL };
const static char* g_pTexTypes[] = { "32", "tex32", "clut32", "tex32to16", "tex16to8h" };

#define TEXWRAP_REPEAT 0
#define TEXWRAP_CLAMP 1
#define TEXWRAP_REGION_REPEAT 2
#define TEXWRAP_REPEAT_CLAMP 3

inline int GET_SHADER_INDEX(int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int context, int ps)
{
	return type + texfilter*NUM_TYPES + NUM_FILTERS*NUM_TYPES*texwrap + NUM_TEXWRAPS*NUM_FILTERS*NUM_TYPES*(fog+2*writedepth+4*testaem+8*exactcolor+16*context+32*ps);
}

extern CGcontext g_cgcontext;

static CGprogram LoadShaderFromType(const char* srcdir, const char* srcfile, int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int ps, int context)
{
	assert( texwrap < NUM_TEXWRAPS);
	assert( type < NUM_TYPES );

	char str[255], strctx[255];
	sprintf(str, "Texture%s%d_%sPS", fog?"Fog":"", texfilter, g_pTexTypes[type]);
	sprintf(strctx, "-I%s%s", srcdir, context?"ctx1":"ctx0");

	vector<const char*> macros;
	macros.push_back(strctx);
#ifdef _DEBUG
	macros.push_back("-bestprecision");
#endif
	if( g_pPsTexWrap[texwrap] != NULL ) macros.push_back(g_pPsTexWrap[texwrap]);
	if( writedepth ) macros.push_back("-DWRITE_DEPTH");
	if( testaem ) macros.push_back("-DTEST_AEM");
	if( exactcolor ) macros.push_back("-DEXACT_COLOR");
	if( ps & SHADER_ACCURATE ) macros.push_back("-DACCURATE_DECOMPRESSION");
	macros.push_back(NULL);

	CGprogram prog = cgCreateProgramFromFile(g_cgcontext, CG_SOURCE, srcfile, CG_PROFILE_ARBFP1, str, &macros[0]);
	if( !cgIsProgram(prog) ) {
		printf("Failed to load shader %s: \n%s\n", str, cgGetLastListing(g_cgcontext));
		return NULL;
	}

	return prog;
}

struct SHADERHEADER
{
	unsigned int index, offset, size; // if highest bit of index is set, pixel shader
};

#define SH_WRITEDEPTH 0x2000 // depth is written
#define SH_CONTEXT1 0x1000 // context1 is used

#define SH_REGULARVS 0x8000
#define SH_TEXTUREVS 0x8001
#define SH_REGULARFOGVS 0x8002
#define SH_TEXTUREFOGVS 0x8003
#define SH_REGULARPS 0x8004
#define SH_REGULARFOGPS 0x8005
#define SH_BITBLTVS 0x8006
#define SH_BITBLTPS 0x8007
#define SH_BITBLTDEPTHPS 0x8009
#define SH_CRTCTARGPS 0x800a
#define SH_CRTCPS 0x800b
#define SH_CRTC24PS 0x800c
#define SH_ZEROPS 0x800e
#define SH_BASETEXTUREPS 0x800f
#define SH_BITBLTAAPS 0x8010
#define SH_CRTCTARGINTERPS 0x8012
#define SH_CRTCINTERPS 0x8013
#define SH_CRTC24INTERPS 0x8014
#define SH_BITBLTDEPTHMRTPS 0x8016
#define SH_CONVERT16TO32PS 0x8020
#define SH_CONVERT32TO16PS 0x8021
#define SH_CRTC_NEARESTPS 0x8022
#define SH_CRTCINTER_NEARESTPS 0x8023
