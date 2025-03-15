R"(
layout(location=0) out vec4 ocol0;
layout(location=1) out vec4 ocol1;
layout(location=2) out vec4 ocol2;
layout(location=3) out vec4 ocol3;

layout(location=0) in vec4 in_regs[16];

#define RSX_FP_OPCODE_NOP 0x00 // No-Operation
#define RSX_FP_OPCODE_MOV 0x01 // Move
#define RSX_FP_OPCODE_MUL 0x02 // Multiply
#define RSX_FP_OPCODE_ADD 0x03 // Add
#define RSX_FP_OPCODE_MAD 0x04 // Multiply-Add
#define RSX_FP_OPCODE_DP3 0x05 // 3-component Dot Product
#define RSX_FP_OPCODE_DP4 0x06 // 4-component Dot Product
#define RSX_FP_OPCODE_DST 0x07 // Distance
#define RSX_FP_OPCODE_MIN 0x08 // Minimum
#define RSX_FP_OPCODE_MAX 0x09 // Maximum
#define RSX_FP_OPCODE_SLT 0x0A // Set-If-LessThan
#define RSX_FP_OPCODE_SGE 0x0B // Set-If-GreaterEqual
#define RSX_FP_OPCODE_SLE 0x0C // Set-If-LessEqual
#define RSX_FP_OPCODE_SGT 0x0D // Set-If-GreaterThan
#define RSX_FP_OPCODE_SNE 0x0E // Set-If-NotEqual
#define RSX_FP_OPCODE_SEQ 0x0F // Set-If-Equal
#define RSX_FP_OPCODE_FRC 0x10 // Fraction (fract)
#define RSX_FP_OPCODE_FLR 0x11 // Floor
#define RSX_FP_OPCODE_KIL 0x12 // Kill fragment
#define RSX_FP_OPCODE_PK4 0x13 // Pack four signed 8-bit values
#define RSX_FP_OPCODE_UP4 0x14 // Unpack four signed 8-bit values
#define RSX_FP_OPCODE_DDX 0x15 // Partial-derivative in x (Screen space derivative w.r.t. x)
#define RSX_FP_OPCODE_DDY 0x16 // Partial-derivative in y (Screen space derivative w.r.t. y)
#define RSX_FP_OPCODE_TEX 0x17 // Texture lookup
#define RSX_FP_OPCODE_TXP 0x18 // Texture sample with projection (Projective texture lookup)
#define RSX_FP_OPCODE_TXD 0x19 // Texture sample with partial differentiation (Texture lookup with derivatives)
#define RSX_FP_OPCODE_RCP 0x1A // Reciprocal
#define RSX_FP_OPCODE_RSQ 0x1B // Reciprocal Square Root
#define RSX_FP_OPCODE_EX2 0x1C // Exponentiation base 2
#define RSX_FP_OPCODE_LG2 0x1D // Log base 2
#define RSX_FP_OPCODE_LIT 0x1E // Lighting coefficients
#define RSX_FP_OPCODE_LRP 0x1F // Linear Interpolation
#define RSX_FP_OPCODE_STR 0x20 // Set-If-True
#define RSX_FP_OPCODE_SFL 0x21 // Set-If-False
#define RSX_FP_OPCODE_COS 0x22 // Cosine
#define RSX_FP_OPCODE_SIN 0x23 // Sine
#define RSX_FP_OPCODE_PK2 0x24 // Pack two 16-bit floats
#define RSX_FP_OPCODE_UP2 0x25 // Unpack two 16-bit floats
#define RSX_FP_OPCODE_POW 0x26 // Power
#define RSX_FP_OPCODE_PKB 0x27 // Pack bytes
#define RSX_FP_OPCODE_UPB 0x28 // Unpack bytes
#define RSX_FP_OPCODE_PK16 0x29 // Pack 16 bits
#define RSX_FP_OPCODE_UP16 0x2A // Unpack 16
#define RSX_FP_OPCODE_BEM 0x2B // Bump-environment map (a.k.a. 2D coordinate transform)
#define RSX_FP_OPCODE_PKG 0x2C // Pack with sRGB transformation
#define RSX_FP_OPCODE_UPG 0x2D // Unpack gamma
#define RSX_FP_OPCODE_DP2A 0x2E // 2-component dot product with scalar addition
#define RSX_FP_OPCODE_TXL 0x2F // Texture sample with explicit LOD
#define RSX_FP_OPCODE_TXB 0x31 // Texture sample with bias
#define RSX_FP_OPCODE_TEXBEM 0x33
#define RSX_FP_OPCODE_TXPBEM 0x34
#define RSX_FP_OPCODE_BEMLUM 0x35
#define RSX_FP_OPCODE_REFL 0x36 // Reflection vector
#define RSX_FP_OPCODE_TIMESWTEX 0x37
#define RSX_FP_OPCODE_DP2 0x38 // 2-component dot product
#define RSX_FP_OPCODE_NRM 0x39 // Normalize
#define RSX_FP_OPCODE_DIV 0x3A // Division
#define RSX_FP_OPCODE_DIVSQ 0x3B // Divide by Square Root
#define RSX_FP_OPCODE_LIF 0x3C // Final part of LIT
#define RSX_FP_OPCODE_FENCT 0x3D // Fence T?
#define RSX_FP_OPCODE_FENCB 0x3E // Fence B?
#define RSX_FP_OPCODE_BRK 0x40 // Break
#define RSX_FP_OPCODE_CAL 0x41 // Subroutine call
#define RSX_FP_OPCODE_IFE 0x42 // If
#define RSX_FP_OPCODE_LOOP 0x43 // Loop
#define RSX_FP_OPCODE_REP 0x44 // Repeat
#define RSX_FP_OPCODE_RET 0x45 // Return

#define EXEC_LT 1
#define EXEC_EQ 2
#define EXEC_GT 4

#define RSX_FP_REGISTER_TYPE_TEMP 0
#define RSX_FP_REGISTER_TYPE_INPUT 1
#define RSX_FP_REGISTER_TYPE_CONSTANT 2
#define RSX_FP_REGISTER_TYPE_UNKNOWN 3

#define CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT 0xe
#define CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS 0x40

#define GET_BITS(bitfield, offset, count) bitfieldExtract(bitfield, offset, count)
#define TEST_BIT(bitfield, offset) (GET_BITS(bitfield, offset, 1) > 0)

#define GET_INST_BITS(word, offset, count) GET_BITS(inst.words[word], offset, count)
#define TEST_INST_BIT(word, offset) (GET_INST_BITS(word, offset, 1) > 0)

#define select mix
#define reg_mov(d, s, m) d = select(d, s, m)

// GPR set
vec4 vr0, vr1;     // GP vector register
uint ur0, ur1;     // GP unsigned register (scalar)
uvec4 uvr0;        // GP unsigned register (vector)
bvec4 bvr0, bvr1;  // GP boolean register (vector)
float sr0;         // GP scalar register

vec4 vrr;          // value return (dst register)
vec4 s0, s1, s2;   // instruction src (src0, src1, src2)

vec4 wpos;         // window position register WPOS
vec4 fogc;         // Fog coordinate register FOGC

const vec4 vr_zero = vec4(0.);
const vec4 vr_one = vec4(1.);

bool shader_attribute(const in uint mask)
{
	return (shader_control & mask) != 0;
}

vec4 _distance(const in vec4 a, const in vec4 b)
{
	// Old-school distance vector
	return vec4(1., a.y * b.y, a.z, b.w);
}

vec4 shuffle(const in vec4 value, const in uint code)
{
	switch (code)
	{
	case 0xE4:
		return value;
	case 0x24:
		return value.xyzx;
	case 0xA4:
		return value.xyzz;
	case 0x00:
		return value.xxxx;
	case 0x55:
		return value.yyyy;
	case 0xAA:
		return value.zzzz;
	case 0xFF:
		return value.wwww;
	case 0x04:
		return value.xyxx;
	default:
		uint x = bitfieldExtract(code, 0, 2);
		uint y = bitfieldExtract(code, 2, 2);
		uint z = bitfieldExtract(code, 4, 2);
		uint w = bitfieldExtract(code, 6, 2);
		return vec4(value[x], value[y], value[z], value[w]);
	}
}

struct instruction_t
{
	uvec4 words;
	uint opcode;
	bool end;
};

const float modifier_scale[] = {1.f, 2.f, 4.f, 8.f, 1.f, 0.5f, 0.25f, 0.125f};

vec4 regs16[48];
vec4 regs32[48];
vec4 cc[2];
int inst_length = 1;
int ip = -1;
instruction_t inst;

#ifdef WITH_FLOW_CTRL
int test_addr = -1;
int jump_addr = -1;
int loop_start_addr = -1;
int loop_end_addr = -1;
int counter = 0;
#endif

vec4 read_src(const in int index)
{
	ur0 = GET_INST_BITS(index + 1, 0, 2);

	switch (ur0)
	{
	case RSX_FP_REGISTER_TYPE_TEMP:
	{
		switch(index)
		{
		case 0:
			ur1 = GET_INST_BITS(1, 2, 6); break;
		case 1:
			ur1 = GET_INST_BITS(2, 2, 6); break;
		case 2:
			ur1 = GET_INST_BITS(3, 2, 6); break;
		}

		if (TEST_INST_BIT(index + 1, 8))
		{
			vr0 = regs16[ur1];
		}
		else
		{
			vr0 = regs32[ur1];
		}
		break;
	}
	case RSX_FP_REGISTER_TYPE_INPUT:
	{
		ur1 = GET_INST_BITS(0, 13, 4);
		switch (ur1)
		{
		case 0:
			vr0 = wpos; break;
		case 1:
			vr0 = gl_FrontFacing? in_regs[3] : in_regs[1]; break;
		case 2:
			vr0 = gl_FrontFacing? in_regs[4] : in_regs[2]; break;
		case 3:
			vr0 = fogc; break;
		case 13:
			vr0 = in_regs[6]; break;
		case 14:
			vr0 = gl_FrontFacing? vr_one : -vr_one; break;
		default:
			ur1 += 3;
			vr0 = in_regs[ur1]; break;
		}

		break;
	}
	case RSX_FP_REGISTER_TYPE_CONSTANT:
	{
		inst_length = 2;
		uvr0 =
			((fp_instructions[ip + 1] << 8) & uvec4(0xFF00FF00)) |
			((fp_instructions[ip + 1] >> 8) & uvec4(0x00FF00FF));
		vr0 = uintBitsToFloat(uvr0);
		break;
	}
	}

	ur1 = GET_INST_BITS(index + 1, 9, 8);
	vr0 = shuffle(vr0, ur1);

	// abs
	if (index == 0)
	{
		if (TEST_INST_BIT(1, 29)) vr0 = abs(vr0);
	}
	else
	{
		ur1 = index + 1;
		if (TEST_INST_BIT(ur1, 18)) vr0 = abs(vr0);
	}

	// neg
	return (TEST_INST_BIT(index + 1, 17))? -vr0 : vr0;
}

vec4 read_cond()
{
	return shuffle(cc[GET_INST_BITS(1, 31, 1)], GET_INST_BITS(1, 21, 8));
}

bvec4 decode_cond(const in uint mode, const in vec4 cond)
{
	switch (mode)
	{
	case EXEC_GT | EXEC_EQ:
		return greaterThanEqual(cond, vr_zero);
	case EXEC_LT | EXEC_EQ:
		return lessThanEqual(cond, vr_zero);
	case EXEC_LT | EXEC_GT:
		return notEqual(cond, vr_zero);
	case EXEC_GT:
		return greaterThan(cond, vr_zero);
	case EXEC_LT:
		return lessThan(cond, vr_zero);
	case EXEC_EQ:
		return equal(cond, vr_zero);
	default:
		return bvec4(vr_zero);
	}
}

#if defined(WITH_FLOW_CTRL) || defined(WITH_KIL)

bool check_cond()
{
	ur0 = GET_INST_BITS(1, 18, 3);
	if (ur0 == 0x7)
	{
		return true;
	}

	vr0 = read_cond();
	bvr0 = decode_cond(ur0, vr0);
	return any(bvr0);
}

#endif

#ifdef WITH_TEXTURES

#define RSX_SAMPLE_TEXTURE_1D   0
#define RSX_SAMPLE_TEXTURE_2D   1
#define RSX_SAMPLE_TEXTURE_CUBE 2
#define RSX_SAMPLE_TEXTURE_3D   3

// FIXME: Remove when codegen is unified
#define CLAMP_COORDS_BIT 17

float _texcoord_xform(const in float coord, const in sampler_info params)
{
	float result = fma(coord, params.scale_x, params.bias_x);
	if (TEST_BIT(params.flags, CLAMP_COORDS_BIT))
	{
		result = clamp(result, params.clamp_min_x, params.clamp_max_x);
	}

	return result;
}

vec2 _texcoord_xform(const in vec2 coord, const in sampler_info params)
{
	vec2 result = fma(
		coord,
		vec2(params.scale_x, params.scale_y),
		vec2(params.bias_x, params.bias_y)
	);

	if (TEST_BIT(params.flags, CLAMP_COORDS_BIT))
	{
		result = clamp(
			result,
			vec2(params.clamp_min_x, params.clamp_min_y),
			vec2(params.clamp_max_x, params.clamp_max_y)
		);
	}

	return result;
}

vec3 _texcoord_xform(const in vec3 coord, const in sampler_info params)
{
	vec3 result = fma(
		coord,
		vec3(params.scale_x, params.scale_y, params.scale_z),
		vec3(params.bias_x, params.bias_y, params.bias_z)
	);

	// NOTE: Coordinate clamping not supported for CUBE and 3D textures
	return result;
}

vec4 _texture(in vec4 coord, float bias)
{
	ur0 = GET_INST_BITS(0, 17, 4);
	if (!IS_TEXTURE_RESIDENT(ur0))
	{
		return vr_zero;
	}

	ur1 = ur0 + ur0;
	const uint type = GET_BITS(texture_control, int(ur1), 2);

	switch (type)
	{
	case RSX_SAMPLE_TEXTURE_1D:
		coord.x = _texcoord_xform(coord.x, texture_parameters[ur0]);
		vr0 = texture(SAMPLER1D(ur0), coord.x, bias);
		break;
	case RSX_SAMPLE_TEXTURE_2D:
		coord.xy = _texcoord_xform(coord.xy, texture_parameters[ur0]);
		vr0 = texture(SAMPLER2D(ur0), coord.xy, bias);
		break;
	case RSX_SAMPLE_TEXTURE_CUBE:
		coord.xyz = _texcoord_xform(coord.xyz, texture_parameters[ur0]);
		vr0 = texture(SAMPLERCUBE(ur0), coord.xyz, bias);
		break;
	case RSX_SAMPLE_TEXTURE_3D:
		coord.xyz = _texcoord_xform(coord.xyz, texture_parameters[ur0]);
		vr0 = texture(SAMPLER3D(ur0), coord.xyz, bias);
		break;
	}

	if (TEST_INST_BIT(0, 21))
	{
		vr0 = vr0 * 2. - 1.;
	}

	return vr0;
}

vec4 _textureLod(in vec4 coord, float lod)
{
	ur0 = GET_INST_BITS(0, 17, 4);
	if (!IS_TEXTURE_RESIDENT(ur0))
	{
		return vr_zero;
	}

	ur1 = ur0 + ur0;
	const uint type = GET_BITS(texture_control, int(ur1), 2);

	switch (type)
	{
	case RSX_SAMPLE_TEXTURE_1D:
		coord.x = _texcoord_xform(coord.x, texture_parameters[ur0]);
		vr0 = textureLod(SAMPLER1D(ur0), coord.x, lod);
		break;
	case RSX_SAMPLE_TEXTURE_2D:
		coord.xy = _texcoord_xform(coord.xy, texture_parameters[ur0]);
		vr0 = textureLod(SAMPLER2D(ur0), coord.xy, lod);
		break;
	case RSX_SAMPLE_TEXTURE_CUBE:
		coord.xyz = _texcoord_xform(coord.xyz, texture_parameters[ur0]);
		vr0 = textureLod(SAMPLERCUBE(ur0), coord.xyz, lod);
		break;
	case RSX_SAMPLE_TEXTURE_3D:
		coord.xyz = _texcoord_xform(coord.xyz, texture_parameters[ur0]);
		vr0 = textureLod(SAMPLER3D(ur0), coord.xyz, lod);
		break;
	}

	if (TEST_INST_BIT(0, 21))
	{
		// Normal-expand, v = 2v - 1
		vr0 += vr0;
		vr0 -= 1.;
	}

	return vr0;
}

#endif

void write_dst(const in vec4 value)
{
	uvr0 = uvec4(uint(1 << 9), uint(1 << 10), uint(1 << 11), uint(1 << 12));
	bvr0 = bvec4(uvr0 & inst.words.xxxx);

	if (TEST_INST_BIT(0, 8)) // SET COND
	{
		ur0 = GET_INST_BITS(1, 30, 1);
		reg_mov(cc[ur0], value, bvr0);
	}

	if (TEST_INST_BIT(0, 30)) // NO DEST
	{
		return;
	}

	ur1 = GET_INST_BITS(2, 28, 3);
	sr0 = modifier_scale[ur1];
	vr0 = value * sr0;

	if (TEST_INST_BIT(0, 31)) // SAT
	{
		vr0 = clamp(vr0, 0, 1);
	}

	ur0 = GET_INST_BITS(1, 18, 3);
	if (ur0 != 0x7)
	{
		vr1 = read_cond();
		bvr1 = decode_cond(ur0, vr1);
		bvr0 = bvec4(uvec4(bvr0) & uvec4(bvr1));
	}

	ur1 = GET_INST_BITS(0, 1, 6);
	if (TEST_INST_BIT(0, 7))
	{
		reg_mov(regs16[ur1], vr0, bvr0);
	}
	else
	{
		reg_mov(regs32[ur1], vr0, bvr0);
	}
}

void initialize()
{
	// Initialize registers
	// NOTE: Register count is the number of 'full' registers that will be consumed. Hardware seems to do some renaming.
	// NOTE: Attempting to zero-initialize all the registers will slow things to a crawl!

	uint register_count = GET_BITS(shader_control, 24, 6);
	ur0 = 0, ur1 = 0;
	while (register_count > 0)
	{
		regs32[ur0++] = vr_zero;
		regs16[ur1++] = vr_zero;
		regs16[ur1++] = vr_zero;
		register_count--;
	}

	// Fog coord
	fogc = in_regs[5];
	switch(fog_mode)
	{
	case 0:
		//linear
		fogc.y = fog_param1 * fogc.x + (fog_param0 - 1.);
		break;
	case 1:
		//exponential
		fogc.y = exp(11.084 * (fog_param1 * fogc.x + fog_param0 - 1.5));
		break;
	case 2:
		//exponential2
		fogc.y = exp(-pow(4.709 * (fog_param1 * fogc.x + fog_param0 - 1.5), 2.));
		break;
	case 3:
		//exponential_abs
		fogc.y = exp(11.084 * (fog_param1 * abs(fogc.x) + fog_param0 - 1.5));
		break;
	case 4:
		//exponential2_abs
		fogc.y = exp(-pow(4.709 * (fog_param1 * abs(fogc.x) + fog_param0 - 1.5), 2.));
		break;
	case 5:
		//linear_abs
		fogc.y = fog_param1 * abs(fogc.x) + (fog_param0 - 1.);
		break;
	default:
		fogc = in_regs[5];
	}
	fogc.y = clamp(fogc.y, 0., 1.);

	// WPOS
	vr0 = vec4(abs(wpos_scale), wpos_scale, 1., 1.);
	vr1 = vec4(0., wpos_bias, 0., 0.);
	wpos = gl_FragCoord * vr0 + vr1;

	// Other
	cc[0] = vr_zero;
	cc[1] = vr_zero;
})"

R"(

void main()
{
	initialize();

	inst.end = false;
	bool handled;

#ifdef WITH_STIPPLING
	uvr0.xy = uvec2(gl_FragCoord.xy) % uvec2(32u); // x,y location
	ur0 = uvr0.y * 32u + uvr0.x;                   // linear address
	ur1 = ur0 & 31u;                               // address % 32 -> fetch bit offset
	ur1 = (1u << ur1);                             // address mask
	uvr0.x = (ur0 >> 7u);                          // address to uvec4 row (each row has 32x4 bits)
	ur0 = (ur0 >> 5u) & 3u;                        // address to uvec4 word (address / 32) % 4

	if ((stipple_pattern[uvr0.x][ur0] & ur1) == 0u)
	{
		discard;
		inst.end = true;
	}
#endif

	while (!inst.end)
	{
		ip += inst_length;
		inst_length = 1;

#ifdef WITH_FLOW_CTRL
		if (ip == test_addr)
		{
			ip = jump_addr;
			test_addr = -1;
			jump_addr = -1;
		}
		else if (ip == loop_end_addr)
		{
			if (counter > 0)
			{
				counter--;
				ip = loop_start_addr;
			}
			else
			{
				loop_end_addr = -1;
				loop_start_addr = -1;
			}
		}
#endif

		// Decode instruction
		// endian swap + word swap
		inst.words =
			((fp_instructions[ip] << 8) & uvec4(0xFF00FF00)) |
			((fp_instructions[ip] >> 8) & uvec4(0x00FF00FF));

		inst.opcode = GET_INST_BITS(0, 24, 6);
		inst.end = TEST_INST_BIT(0, 0);

#ifdef WITH_FLOW_CTRL
		if (TEST_INST_BIT(2, 31))
		{
			// Flow control
			switch (inst.opcode | (1 << 6))
			{
			//case RSX_FP_OPCODE_CAL:
				// Function call not yet found in the wild for this hw class
			case RSX_FP_OPCODE_RET:
				inst.end = true;
				continue;
			case RSX_FP_OPCODE_IFE:
				if (check_cond())
				{
					// Go down IF path
					if (inst.words.z < inst.words.w)
					{
						test_addr = int(inst.words.z >> 2);
						jump_addr = int(inst.words.w >> 2);
					}
					// If simple IF..ENDIF, do nothing
				}
				else
				{
					// Go to ELSE path
					ip = int(inst.words.z >> 2);
					inst_length = 0;
				}
				continue;
			case RSX_FP_OPCODE_LOOP:
			case RSX_FP_OPCODE_REP:
				if (check_cond())
				{
					counter = int(GET_INST_BITS(2, 2, 8) - GET_INST_BITS(2, 10, 8));
					counter /= int(GET_INST_BITS(2, 19, 8));
					loop_start_addr = ip + 1;
					loop_end_addr = int(inst.words.w >> 2);
				}
				else
				{
					ip = int(inst.words.w >> 2);
					inst_length = 0;
				}
				continue;
			case RSX_FP_OPCODE_BRK:
				if (loop_end_addr > 0)
				{
					ip = loop_end_addr;
					inst_length = 0;
					counter = 0;
				}
				continue;
			}

			continue;
		}
#endif

		// Class 1, no input/output
		switch (inst.opcode)
		{
		case RSX_FP_OPCODE_NOP:
		case RSX_FP_OPCODE_FENCT:
		case RSX_FP_OPCODE_FENCB:
			continue;
#ifdef WITH_KIL
		case RSX_FP_OPCODE_KIL:
			if (check_cond())
			{
				discard;
				return;
			}
			continue;
#endif
		}

		// Class 2, 1 input
		s0 = read_src(0);
		handled = true;
		switch (inst.opcode)
		{
		case RSX_FP_OPCODE_MOV:
			vrr = s0; break;
		case RSX_FP_OPCODE_FRC:
			vrr = fract(s0); break;
		case RSX_FP_OPCODE_FLR:
			vrr = floor(s0); break;
		case RSX_FP_OPCODE_DDX:
			vrr = dFdx(s0); break;
		case RSX_FP_OPCODE_DDY:
			vrr = dFdy(s0); break;
		case RSX_FP_OPCODE_RCP:
			vrr = (1.f / s0.xxxx); break;
		case RSX_FP_OPCODE_RSQ:
			vrr = inversesqrt(s0.xxxx); break;
		case RSX_FP_OPCODE_EX2:
			vrr = exp2(s0.xxxx); break;
		case RSX_FP_OPCODE_LG2:
			vrr = log2(s0.xxxx); break;
		case RSX_FP_OPCODE_STR:
			vrr = vr_one; break;
		case RSX_FP_OPCODE_SFL:
			vrr = vr_zero; break;
		case RSX_FP_OPCODE_COS:
			vrr = cos(s0.xxxx); break;
		case RSX_FP_OPCODE_SIN:
			vrr = sin(s0.xxxx); break;
		case RSX_FP_OPCODE_NRM:
			vrr.xyz = normalize(s0.xyz); break;

#ifdef WITH_TEXTURES
		case RSX_FP_OPCODE_TEX:
			vrr = _texture(s0, 0.f); break;
		case RSX_FP_OPCODE_TXP:
			vrr = _texture(vec4(s0.xyz / s0.w, s0.w), 0.f); break;
#endif

#ifdef WITH_PACKING
		case RSX_FP_OPCODE_PK2:
			vrr = vec4(uintBitsToFloat(packHalf2x16(s0.xy))); break;
		case RSX_FP_OPCODE_PK4:
			vrr = vec4(uintBitsToFloat(packSnorm4x8(s0))); break;
		case RSX_FP_OPCODE_PK16:
			vrr = vec4(uintBitsToFloat(packSnorm2x16(s0.xy))); break;
		case RSX_FP_OPCODE_PKG:
			// Should be similar to PKB but with gamma correction, see description of PK4UBG in khronos page
		case RSX_FP_OPCODE_PKB:
			vrr = vec4(uintBitsToFloat(packUnorm4x8(s0))); break;
		case RSX_FP_OPCODE_UP2:
			vrr = unpackHalf2x16(floatBitsToUint(s0.x)).xyxy; break;
		case RSX_FP_OPCODE_UP4:
			vrr = unpackSnorm4x8(floatBitsToUint(s0.x)); break;
		case RSX_FP_OPCODE_UP16:
			vrr = unpackSnorm2x16(floatBitsToUint(s0.x)).xyxy; break;
		case RSX_FP_OPCODE_UPG:
			// Same as UPB with gamma correction
		case RSX_FP_OPCODE_UPB:
			vrr = unpackUnorm4x8(floatBitsToUint(s0.x)); break;
#endif
		default:
			handled = false;
		}

		if (!handled)
		{
			// Class 3, 2 inputs
			s1 = read_src(1);
			handled = true;
			switch (inst.opcode)
			{
			case RSX_FP_OPCODE_MUL:
				vrr = s0 * s1; break;
			case RSX_FP_OPCODE_ADD:
				vrr = s0 + s1; break;
			case RSX_FP_OPCODE_DP2:
				vrr = dot(s0.xy, s1.xy).xxxx; break;
			case RSX_FP_OPCODE_DP3:
				vrr = dot(s0.xyz, s1.xyz).xxxx; break;
			case RSX_FP_OPCODE_DP4:
				vrr = dot(s0, s1).xxxx; break;
			case RSX_FP_OPCODE_DST:
				vrr = _distance(s0, s1); break;
			case RSX_FP_OPCODE_MIN:
				vrr = min(s0, s1); break;
			case RSX_FP_OPCODE_MAX:
				vrr = max(s0, s1); break;
			case RSX_FP_OPCODE_SLT:
				vrr = vec4(lessThan(s0, s1)); break;
			case RSX_FP_OPCODE_SGE:
				vrr = vec4(greaterThanEqual(s0, s1)); break;
			case RSX_FP_OPCODE_SLE:
				vrr = vec4(lessThanEqual(s0, s1)); break;
			case RSX_FP_OPCODE_SGT:
				vrr = vec4(greaterThan(s0, s1)); break;
			case RSX_FP_OPCODE_SNE:
				vrr = vec4(notEqual(s0, s1)); break;
			case RSX_FP_OPCODE_SEQ:
				vrr = vec4(equal(s0, s1)); break;
			case RSX_FP_OPCODE_POW:
				vrr = pow(s0, s1).xxxx; break;
			case RSX_FP_OPCODE_DIV:
				vrr = s0 / s1.xxxx; break;
			case RSX_FP_OPCODE_DIVSQ:
				bvr0 = bvec4(s0);
				sr0 = inversesqrt(s1.x);
				vr0 = s0 * sr0;
				vrr = select(s0, vr0, bvr0);
				break;
			case RSX_FP_OPCODE_REFL:
				vrr = reflect(s0, s1); break;

#ifdef WITH_TEXTURES
			//case RSX_FP_OPCODE_TXD:
			case RSX_FP_OPCODE_TXL:
				vrr = _textureLod(s0, s1.x); break;
			case RSX_FP_OPCODE_TXB:
				vrr = _texture(s0, s1.x); break;
			//case RSX_FP_OPCODE_TEXBEM:
			//case RSX_FP_OPCODE_TXPBEM:
#endif
			default:
				handled = false;
			}
		}

		if (!handled)
		{
			// Class 4, 3 inputs
			s2 = read_src(2);
			switch (inst.opcode)
			{
			case RSX_FP_OPCODE_MAD:
				vrr = fma(s0, s1, s2); break;
			case RSX_FP_OPCODE_LRP:
				vrr = mix(s1, s2, s0); break;
			case RSX_FP_OPCODE_DP2A:
				vrr = dot(s0.xy, s1.xy).xxxx + s2.xxxx; break;
			}
		}
#if 0
		// Other
		case RSX_FP_OPCODE_BEM:
		case RSX_FP_OPCODE_BEMLUM:
		case RSX_FP_OPCODE_LIT:
		case RSX_FP_OPCODE_LIF:
		case RSX_FP_OPCODE_TIMESWTEX:
#endif
		write_dst(vrr);
	}

#ifdef WITH_HALF_OUTPUT_REGISTER
		ocol0 = regs16[0];
		ocol1 = regs16[4];
		ocol2 = regs16[6];
		ocol3 = regs16[8];
#else
		ocol0 = regs32[0];
		ocol1 = regs32[2];
		ocol2 = regs32[3];
		ocol3 = regs32[4];
#endif

#ifdef WITH_DEPTH_EXPORT
	gl_FragDepth = regs32[1].z;
#endif

// Typically an application will pick one strategy and stick with it
#ifdef ALPHA_TEST_GEQUAL
	if (ocol0.a < alpha_ref) discard; // gequal
#endif
#ifdef ALPHA_TEST_GREATER
	if (ocol0.a <= alpha_ref) discard; // greater
#endif
#ifdef ALPHA_TEST_LESS
	if (ocol0.a >= alpha_ref) discard; // less
#endif
#ifdef ALPHA_TEST_LEQUAL
	if (ocol0.a > alpha_ref) discard; // lequal
#endif
#ifdef ALPHA_TEST_EQUAL
	if (ocol0.a != alpha_ref) discard; // equal
#endif
#ifdef ALPHA_TEST_NEQUAL
	if (ocol0.a == alpha_ref) discard; // nequal
#endif
}

)"
