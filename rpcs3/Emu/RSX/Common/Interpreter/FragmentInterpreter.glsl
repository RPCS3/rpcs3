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

#define GET_BITS(word, offset, count) bitfieldExtract(inst.words[word], offset, count)
#define TEST_BIT(word, offset) (GET_BITS(word, offset, 1) > 0)

#define reg_mov(d, s, m) d = mix(d, s, m)

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

vec4 read_src(const in int index)
{
	const uint type = GET_BITS(index + 1, 0, 2);
	vec4 value;

	switch (type)
	{
	case RSX_FP_REGISTER_TYPE_TEMP:
	{
		const uint i = GET_BITS(index + 1, 2, 6);
		if (TEST_BIT(index + 1, 8))
		{
			value = regs16[i];
		}
		else
		{
			value = regs32[i];
		}
		break;
	}
	case RSX_FP_REGISTER_TYPE_INPUT:
	{
		const uint i = GET_BITS(0, 13, 4);
		switch (i)
		{
		case 0:
			// TODO: wpos
			value = vec4(0.); break;
		case 1:
			value = gl_FrontFacing? in_regs[3] : in_regs[1]; break;
		case 2:
			value = gl_FrontFacing? in_regs[4] : in_regs[2]; break;
		case 3:
			value = fetch_fog_value(fog_mode, in_regs[5]); break;
		case 13:
			value = in_regs[6]; break;
		case 14:
			value = gl_FrontFacing? vec4(1.) : vec4(-1.); break;
		default:
			value = in_regs[i + 3]; break;
		}

		break;
	}
	case RSX_FP_REGISTER_TYPE_CONSTANT:
	{
		inst_length = 2;
		uvec4 result =
			((fp_instructions[ip + 1] << 8) & uvec4(0xFF00FF00)) |
			((fp_instructions[ip + 1] >> 8) & uvec4(0x00FF00FF));
		value = uintBitsToFloat(result);
		break;
	}
	}

	value = shuffle(value, GET_BITS(index + 1, 9, 8));

	// abs
	if (index == 0)
	{
		value = (TEST_BIT(1, 29))? abs(value) : value;
	}
	else
	{
		value = (TEST_BIT(index + 1, 18))? abs(value) : value;
	}

	// neg
	return (TEST_BIT(index + 1, 17))? -value : value;
}

vec4 read_cond()
{
	return shuffle(cc[GET_BITS(1, 31, 1)], GET_BITS(1, 21, 8));
}

vec4 _texture(in vec4 coord, float bias)
{
	const uint tex_num = GET_BITS(0, 17, 4);
	if (!IS_TEXTURE_RESIDENT(tex_num))
	{
		return vec4(0., 0., 0., 1.);
	}

	const uint type = bitfieldExtract(texture_control, int(tex_num + tex_num), 2);
	coord.xy *= texture_parameters[tex_num].scale;

	switch (type)
	{
	case 0:
		return texture(SAMPLER1D(tex_num), coord.x, bias);
	case 1:
		return texture(SAMPLER2D(tex_num), coord.xy, bias);
	case 2:
		return texture(SAMPLER3D(tex_num), coord.xyz, bias);
	case 3:
		return texture(SAMPLERCUBE(tex_num), coord.xyz, bias);
	}

	return vec4(0.);
}

vec4 _textureLod(in vec4 coord, float lod)
{
	const uint tex_num = GET_BITS(0, 17, 4);
	if (!IS_TEXTURE_RESIDENT(tex_num))
	{
		return vec4(0., 0., 0., 1.);
	}

	const uint type = bitfieldExtract(texture_control, int(tex_num + tex_num), 2);
	coord.xy *= texture_parameters[tex_num].scale;

	switch (type)
	{
	case 0:
		return textureLod(SAMPLER1D(tex_num), coord.x, lod);
	case 1:
		return textureLod(SAMPLER2D(tex_num), coord.xy, lod);
	case 2:
		return textureLod(SAMPLER3D(tex_num), coord.xyz, lod);
	case 3:
		return textureLod(SAMPLERCUBE(tex_num), coord.xyz, lod);
	}

	return vec4(0.);
}

void write_dst(in vec4 value)
{
	bvec4 inst_mask = bvec4(
		TEST_BIT(0, 9),
		TEST_BIT(0, 10),
		TEST_BIT(0, 11),
		TEST_BIT(0, 12));

	if (TEST_BIT(0, 8)) // SET COND
	{
		uint index = GET_BITS(1, 30, 1);
		reg_mov(cc[index], value, inst_mask);
	}

	if (TEST_BIT(0, 30)) // NO DEST
	{
		return;
	}

	if (TEST_BIT(0, 31)) // SAT
	{
		value = clamp(value, 0, 1);
	}

	const uint exec_mask = GET_BITS(1, 18, 3);
	if (exec_mask != 0x7)
	{
		bvec4 write_mask;
		const vec4 cond = read_cond();

		switch (exec_mask)
		{
		case 0:
			return;
		case EXEC_GT | EXEC_EQ:
			write_mask = greaterThanEqual(cond, vec4(0.)); break;
		case EXEC_LT | EXEC_EQ:
			write_mask = lessThanEqual(cond, vec4(0.)); break;
		case EXEC_LT | EXEC_GT:
			write_mask = notEqual(cond, vec4(0.)); break;
		case EXEC_GT:
			write_mask = greaterThan(cond, vec4(0.)); break;
		case EXEC_LT:
			write_mask = lessThan(cond, vec4(0.)); break;
		case EXEC_EQ:
			write_mask = equal(cond, vec4(0.)); break;
		}

		inst_mask = bvec4(uvec4(inst_mask) & uvec4(write_mask));
	}

	const uint scale = GET_BITS(2, 28, 3);
	value *= modifier_scale[scale];

	const uint index = GET_BITS(0, 1, 6);
	if (TEST_BIT(0, 7))
	{
		reg_mov(regs16[index], value, inst_mask);
	}
	else
	{
		reg_mov(regs32[index], value, inst_mask);
	}
}

void initialize()
{
	// Initialize registers
	// NOTE: Register count is the number of 'full' registers that will be consumed. Hardware seems to do some renaming.
	// NOTE: Attempting to zero-initialize all the registers will slow things to a crawl!

	uint register_count = bitfieldExtract(shader_control, 24, 6);
	uint i = 0, j = 0;
	while (register_count > 0)
	{
		regs32[i++] = vec4(0.);
		regs16[j++] = vec4(0.);
		regs16[j++] = vec4(0.);
		register_count--;
	}
}

void main()
{
	initialize();

	vec4 value, s0, s1, s2;
	inst.end = false;
	bool handled;

	while (!inst.end)
	{
		ip += inst_length;
		inst_length = 1;

		// Decode instruction
		// endian swap + word swap
		inst.words =
			((fp_instructions[ip] << 8) & uvec4(0xFF00FF00)) |
			((fp_instructions[ip] >> 8) & uvec4(0x00FF00FF));

		inst.opcode = GET_BITS(0, 24, 6);
		inst.end = TEST_BIT(0, 0);

		// Class 1, no input/output
		switch (inst.opcode)
		{
		case RSX_FP_OPCODE_NOP:
		case RSX_FP_OPCODE_FENCT:
		case RSX_FP_OPCODE_FENCB:
			continue;
		case RSX_FP_OPCODE_KIL:
			discard; return;
		}

		// Class 2, 1 input
		s0 = read_src(0);
		handled = true;
		switch (inst.opcode)
		{
		case RSX_FP_OPCODE_MOV:
			value = s0; break;
		case RSX_FP_OPCODE_FRC:
			value = fract(s0); break;
		case RSX_FP_OPCODE_FLR:
			value = floor(s0); break;
		case RSX_FP_OPCODE_DDX:
			value = dFdx(s0); break;
		case RSX_FP_OPCODE_DDY:
			value = dFdy(s0); break;
		case RSX_FP_OPCODE_RCP:
			value = (1.f / s0.xxxx); break;
		case RSX_FP_OPCODE_RSQ:
			value = inversesqrt(s0.xxxx); break;
		case RSX_FP_OPCODE_EX2:
			value = exp2(s0.xxxx); break;
		case RSX_FP_OPCODE_LG2:
			value = log2(s0.xxxx); break;
		case RSX_FP_OPCODE_STR:
			value = vec4(1.); break;
		case RSX_FP_OPCODE_SFL:
			value = vec4(0.); break;
		case RSX_FP_OPCODE_COS:
			value = cos(s0.xxxx); break;
		case RSX_FP_OPCODE_SIN:
			value = sin(s0.xxxx); break;
		case RSX_FP_OPCODE_NRM:
			value.xyz = normalize(s0.xyz); break;
		case RSX_FP_OPCODE_TEX:
			value = _texture(s0, 0.f); break;
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
				value = s0 * s1; break;
			case RSX_FP_OPCODE_ADD:
				value = s0 + s1; break;
			case RSX_FP_OPCODE_DP2:
				value = dot(s0.xy, s1.xy).xxxx; break;
			case RSX_FP_OPCODE_DP3:
				value = dot(s0.xyz, s1.xyz).xxxx; break;
			case RSX_FP_OPCODE_DP4:
				value = dot(s0, s1).xxxx; break;
			case RSX_FP_OPCODE_DST:
				value = _distance(s0, s1); break;
			case RSX_FP_OPCODE_MIN:
				value = min(s0, s1); break;
			case RSX_FP_OPCODE_MAX:
				value = max(s0, s1); break;
			case RSX_FP_OPCODE_SLT:
				value = vec4(lessThan(s0, s1)); break;
			case RSX_FP_OPCODE_SGE:
				value = vec4(greaterThanEqual(s0, s1)); break;
			case RSX_FP_OPCODE_SLE:
				value = vec4(lessThanEqual(s0, s1)); break;
			case RSX_FP_OPCODE_SGT:
				value = vec4(greaterThan(s0, s1)); break;
			case RSX_FP_OPCODE_SNE:
				value = vec4(notEqual(s0, s1)); break;
			case RSX_FP_OPCODE_SEQ:
				value = vec4(equal(s0, s1)); break;
			case RSX_FP_OPCODE_POW:
				value = pow(s0, s1).xxxx; break;
			case RSX_FP_OPCODE_DIV:
				value = s0 / s1.xxxx;
			case RSX_FP_OPCODE_DIVSQ:
				value = s0 * inversesqrt(s1.xxxx); break;
			//case RSX_FP_OPCODE_TXP:
			//case RSX_FP_OPCODE_TXD:
			case RSX_FP_OPCODE_TXL:
				value = _textureLod(s0, s1.x); break;
			case RSX_FP_OPCODE_TXB:
				value = _texture(s0, s1.x); break;
			//case RSX_FP_OPCODE_TEXBEM:
			//case RSX_FP_OPCODE_TXPBEM:
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
				value = fma(s0, s1, s2); break;
			case RSX_FP_OPCODE_LRP:
				value = mix(s1, s2, s0); break;
			case RSX_FP_OPCODE_DP2A:
				value = dot(s0.xy, s1.xy).xxxx + s2.xxxx; break;
			}
		}

		// Flow control
/*		case RSX_FP_OPCODE_BRK:
		case RSX_FP_OPCODE_CAL:
		case RSX_FP_OPCODE_IFE:
		case RSX_FP_OPCODE_LOOP:
		case RSX_FP_OPCODE_REP:
		case RSX_FP_OPCODE_RET:

		// Other
		case RSX_FP_OPCODE_PK4:
		case RSX_FP_OPCODE_UP4:
		case RSX_FP_OPCODE_LIT:
		case RSX_FP_OPCODE_LIF:
		case RSX_FP_OPCODE_PK2:
		case RSX_FP_OPCODE_FENCT:
		case RSX_FP_OPCODE_FENCB:
		case RSX_FP_OPCODE_UP2:
		case RSX_FP_OPCODE_PKB:
		case RSX_FP_OPCODE_UPB:
		case RSX_FP_OPCODE_PK16:
		case RSX_FP_OPCODE_UP16:
		case RSX_FP_OPCODE_BEM:
		case RSX_FP_OPCODE_PKG:
		case RSX_FP_OPCODE_UPG:
		case RSX_FP_OPCODE_BEMLUM:
		case RSX_FP_OPCODE_REFL:
		case RSX_FP_OPCODE_TIMESWTEX:*/

		write_dst(value);
	}

	if (!shader_attribute(CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS))
	{
		ocol0 = regs16[0];
		ocol1 = regs16[4];
		ocol1 = regs16[6];
		ocol1 = regs16[8];
	}
	else
	{
		ocol0 = regs32[0];
		ocol1 = regs32[2];
		ocol1 = regs32[3];
		ocol1 = regs32[4];
	}

	if (shader_attribute(CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT))
	{
		gl_FragDepth = regs32[1].z;
	}
	else
	{
		gl_FragDepth = gl_FragCoord.z;
	}
}

)"
