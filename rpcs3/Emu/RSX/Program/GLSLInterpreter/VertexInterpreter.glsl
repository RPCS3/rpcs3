R"(

// Program outputs
layout(location=0) out vec4 dest[16];

#define RSX_SCA_OPCODE_NOP 0x00 // No-Operation
#define RSX_SCA_OPCODE_MOV 0x01 // Move (copy)
#define RSX_SCA_OPCODE_RCP 0x02 // Reciprocal
#define RSX_SCA_OPCODE_RCC 0x03 // Reciprocal clamped
#define RSX_SCA_OPCODE_RSQ 0x04 // Reciprocal square root
#define RSX_SCA_OPCODE_EXP 0x05 // Exponential base 2 (low-precision)
#define RSX_SCA_OPCODE_LOG 0x06 // Logarithm base 2 (low-precision)
#define RSX_SCA_OPCODE_LIT 0x07 // Lighting calculation
#define RSX_SCA_OPCODE_BRA 0x08 // Branch
#define RSX_SCA_OPCODE_BRI 0x09 // Branch by CC register
#define RSX_SCA_OPCODE_CAL 0x0a // Subroutine call
#define RSX_SCA_OPCODE_CLI 0x0b // Subroutine call by CC register
#define RSX_SCA_OPCODE_RET 0x0c // Return from subroutine
#define RSX_SCA_OPCODE_LG2 0x0d // Logarithm base 2
#define RSX_SCA_OPCODE_EX2 0x0e // Exponential base 2
#define RSX_SCA_OPCODE_SIN 0x0f // Sine function
#define RSX_SCA_OPCODE_COS 0x10 // Cosine function
#define RSX_SCA_OPCODE_BRB 0x11 // Branch by Boolean constant
#define RSX_SCA_OPCODE_CLB 0x12 // Subroutine call by Boolean constant
#define RSX_SCA_OPCODE_PSH 0x13 // Push onto stack
#define RSX_SCA_OPCODE_POP 0x14 // Pop from stack
#define RSX_VEC_OPCODE_NOP 0x00 // No-Operation
#define RSX_VEC_OPCODE_MOV 0x01 // Move
#define RSX_VEC_OPCODE_MUL 0x02 // Multiply
#define RSX_VEC_OPCODE_ADD 0x03 // Addition
#define RSX_VEC_OPCODE_MAD 0x04 // Multiply-Add
#define RSX_VEC_OPCODE_DP3 0x05 // 3-component Dot Product
#define RSX_VEC_OPCODE_DPH 0x06 // Homogeneous Dot Product
#define RSX_VEC_OPCODE_DP4 0x07 // 4-component Dot Product
#define RSX_VEC_OPCODE_DST 0x08 // Calculate distance vector
#define RSX_VEC_OPCODE_MIN 0x09 // Minimum
#define RSX_VEC_OPCODE_MAX 0x0a // Maximum
#define RSX_VEC_OPCODE_SLT 0x0b // Set-If-LessThan
#define RSX_VEC_OPCODE_SGE 0x0c // Set-If-GreaterEqual
#define RSX_VEC_OPCODE_ARL 0x0d // Load to address register (round down)
#define RSX_VEC_OPCODE_FRC 0x0e // Extract fractional part (fraction)
#define RSX_VEC_OPCODE_FLR 0x0f // Round down (floor)
#define RSX_VEC_OPCODE_SEQ 0x10 // Set-If-Equal
#define RSX_VEC_OPCODE_SFL 0x11 // Set-If-False
#define RSX_VEC_OPCODE_SGT 0x12 // Set-If-GreaterThan
#define RSX_VEC_OPCODE_SLE 0x13 // Set-If-LessEqual
#define RSX_VEC_OPCODE_SNE 0x14 // Set-If-NotEqual
#define RSX_VEC_OPCODE_STR 0x15 // Set-If-True
#define RSX_VEC_OPCODE_SSG 0x16 // Convert positive values to 1 and negative values to -1
#define RSX_VEC_OPCODE_TXL 0x19 // Texture fetch

#define RSX_VP_REGISTER_TYPE_TEMP      1
#define RSX_VP_REGISTER_TYPE_INPUT     2
#define RSX_VP_REGISTER_TYPE_CONSTANT  3

#define EXEC_LT 1
#define EXEC_EQ 2
#define EXEC_GT 4

#define GET_BITS bitfieldExtract
#define TEST_BIT(word, bit) (GET_BITS(word, bit, 1) != 0)

#define reg_mov(d, s, m) d = mix(d, s, m)

struct D0
{
    uint addr_swz;
    uvec4 swizzle;
    uint cond;
    bool cond_test_enable;
    bool cond_update_enable_0;
    uint dst_tmp;
    uint addr_reg_sel_1;
    uint cond_reg_sel_1;
    bool saturate;
    bool index_input;
    bool cond_update_enable_1;
    bool vec_result;
};

struct D1
{
	uint input_src;
	uint const_src;
	uint vec_opcode;
	uint sca_opcode;
};

struct D2
{
	uint tex_num;
};

struct D3
{
	bool end;
	bool index_const;
	uint dst;
	uint sca_dst_tmp;
	bvec4 vec_mask;
	bvec4 sca_mask;
};

struct SRC
{
    uint reg_type;
    uint tmp_src;
    uvec4 swizzle;
    bool neg;
	bool abs;
};

D0 unpack_D0(const in uint packed_value)
{
	D0 result;

	result.addr_swz = GET_BITS(packed_value, 0, 2);
	result.swizzle.w = GET_BITS(packed_value, 2, 2);
	result.swizzle.z = GET_BITS(packed_value, 4, 2);
	result.swizzle.y = GET_BITS(packed_value, 6, 2);
	result.swizzle.x = GET_BITS(packed_value, 8, 2);
	result.cond = GET_BITS(packed_value, 10, 3);
	result.cond_test_enable = TEST_BIT(packed_value, 13);
	result.cond_update_enable_0 = TEST_BIT(packed_value, 14);
	result.dst_tmp = GET_BITS(packed_value, 15, 6);
	result.addr_reg_sel_1 = GET_BITS(packed_value, 24, 1);
	result.cond_reg_sel_1 = GET_BITS(packed_value, 25, 1);
	result.saturate = TEST_BIT(packed_value, 26);
	result.index_input = TEST_BIT(packed_value, 27);
	result.cond_update_enable_1 = TEST_BIT(packed_value, 29);
	result.vec_result = TEST_BIT(packed_value, 30);
	return result;
}

D1 unpack_D1(const in uint packed_value)
{
	D1 result;

	result.input_src = GET_BITS(packed_value, 8, 4);
	result.const_src = GET_BITS(packed_value, 12, 10);
	result.vec_opcode = GET_BITS(packed_value, 22, 5);
	result.sca_opcode = GET_BITS(packed_value, 27, 5);
	return result;
}

D2 unpack_D2(const in uint packed_value)
{
	D2 result;

	result.tex_num = GET_BITS(packed_value, 8, 2);
	return result;
}

D3 unpack_D3(const in uint packed_value)
{
	D3 result;

	result.end = TEST_BIT(packed_value, 0);
	result.index_const = TEST_BIT(packed_value, 1);
	result.dst = GET_BITS(packed_value, 2, 5);
	result.sca_dst_tmp = GET_BITS(packed_value, 7, 6);
	result.vec_mask.w = TEST_BIT(packed_value, 13);
	result.vec_mask.z = TEST_BIT(packed_value, 14);
	result.vec_mask.y = TEST_BIT(packed_value, 15);
	result.vec_mask.x = TEST_BIT(packed_value, 16);
	result.sca_mask.w = TEST_BIT(packed_value, 17);
	result.sca_mask.z = TEST_BIT(packed_value, 18);
	result.sca_mask.y = TEST_BIT(packed_value, 19);
	result.sca_mask.x = TEST_BIT(packed_value, 20);
	return result;
}

bool attribute_enabled(const in uint mask)
{
	return (output_mask & mask) != 0;
}

vec4 shuffle(const in vec4 value, const in uvec4 swz)
{
	vec4 result;
	result.x = ref(value, swz.x);
	result.y = ref(value, swz.y);
	result.z = ref(value, swz.z);
	result.w = ref(value, swz.w);
	return result;
}

vec4 _distance(const in vec4 a, const in vec4 b)
{
	// Old-school distance vector
	return vec4(1., a.y * b.y, a.z, b.w);
}

bvec4 test_cond(const in vec4 cond, const in uint mode)
{
	switch (mode)
	{
	case EXEC_GT | EXEC_EQ | EXEC_LT:
		return bvec4(true);
	case EXEC_GT | EXEC_EQ:
		return greaterThanEqual(cond, vec4(0.));
	case EXEC_LT | EXEC_EQ:
		return lessThanEqual(cond, vec4(0.));
	case EXEC_LT | EXEC_GT:
		return notEqual(cond, vec4(0.));
	case EXEC_GT:
		return greaterThan(cond, vec4(0.));
	case EXEC_LT:
		return lessThan(cond, vec4(0.));
	case EXEC_EQ:
		return equal(cond, vec4(0.));
	default:
		return bvec4(false);
	}
}

// Local registers
uvec4 instr;
vec4 temp[32];
ivec4 a[2] = { ivec4(0), ivec4(0) };
vec4 cc[2] = { vec4(0), vec4(0) };

D0 d0;
D1 d1;
D2 d2;
D3 d3;

vec4 get_cond()
{
	return shuffle(cc[d0.cond_reg_sel_1], d0.swizzle);
}

void write_sca(in float value)
{
	if (d0.saturate)
	{
		value = clamp(value, 0, 1);
	}

	if (d3.sca_dst_tmp == 0x3f)
	{
		if (!d0.vec_result)
		{
			reg_mov(dest[d3.dst], vec4(value), d3.sca_mask);
		}
		else
		{
			reg_mov(cc[d0.cond_reg_sel_1], vec4(value), d3.sca_mask);
		}
	}
	else
	{
		reg_mov(temp[d3.sca_dst_tmp], vec4(value), d3.sca_mask);
	}
}

void write_vec(in vec4 value)
{
	if (d0.saturate)
	{
		value = clamp(value, 0, 1);
	}

	bvec4 write_mask = d3.vec_mask;
	if (d0.cond_test_enable)
	{
		const bvec4 mask = test_cond(get_cond(), d0.cond);
		write_mask = bvec4(uvec4(write_mask) & uvec4(mask));
	}

	if (d0.dst_tmp == 0x3f && !d0.vec_result)
	{
		reg_mov(cc[d0.cond_reg_sel_1], value, write_mask);
	}
	else
	{
		if (d0.vec_result && d3.dst < 16)
		{
			reg_mov(dest[d3.dst], value, write_mask);
		}

		if (d0.dst_tmp != 0x3f)
		{
			reg_mov(temp[d0.dst_tmp], value, write_mask);
		}
	}
}

void write_output(const in int oid, const in int mask_bit)
{
	if (!attribute_enabled(1 << mask_bit))
	{
		dest[oid] = vec4(0., 0., 0., 1.);
	}
}

// Cannot dynamically index into the gl_ClipDistance array without causing problems due to it's unknown size
#define write_clip_distance(plane, mask_bit, test, value)\
	if (test && attribute_enabled(1 << mask_bit))\
		gl_ClipDistance[plane] = value;\
	else\
		gl_ClipDistance[plane] = 0.5f;\

ivec4 read_addr_reg()
{
	return a[d0.addr_reg_sel_1];
}

int branch_addr()
{
	uint addr_h = GET_BITS(instr.z, 0, 6);
	uint addr_l = GET_BITS(instr.w, 29, 3);
	return int((addr_h << 3) + addr_l);
}

bool static_branch()
{
	uint mask = (1 << GET_BITS(instr.w, 23, 5));
	bool cond = TEST_BIT(instr.w, 28);
	bool actual = (transform_branch_bits & mask) != 0;

	return (cond == actual);
}

bool dynamic_branch()
{
	if (d0.cond == (EXEC_LT | EXEC_GT | EXEC_EQ)) return true;
	if (d0.cond == 0) return false;

	return any(test_cond(get_cond(), d0.cond));
}

vec4 read_src(const in int index)
{
	uint src;
	vec4 value;
	bool do_abs = false;

	switch (index)
	{
	case 0:
		src = (GET_BITS(instr.y, 0, 8) << 9) | GET_BITS(instr.z, 23, 9);
		do_abs = TEST_BIT(instr.x, 21);
		break;
	case 1:
		src = GET_BITS(instr.z, 6, 17);
		do_abs = TEST_BIT(instr.x, 22);
		break;
	case 2:
		src = (GET_BITS(instr.z, 0, 6) << 11) | GET_BITS(instr.w, 21, 11);
		do_abs = TEST_BIT(instr.x, 23);
		break;
	}

	uint reg_type = GET_BITS(src, 0, 2);
	uint tmp_src = GET_BITS(src, 2, 6);

	switch (reg_type)
	{
	case RSX_VP_REGISTER_TYPE_TEMP:
		value = temp[tmp_src];
		break;

	case RSX_VP_REGISTER_TYPE_INPUT:
		value = read_location(int(d1.input_src));
		break;

	case RSX_VP_REGISTER_TYPE_CONSTANT:
		if (d3.index_const)
		{
			value = _fetch_constant(d1.const_src + ref(a[d0.addr_reg_sel_1], d0.addr_swz));
		}
		else
		{
			value = _fetch_constant(d1.const_src);
		}
		break;
	}

	if (GET_BITS(src, 8, 8) != 0x1B)
	{
		uvec4 swz = uvec4(
			GET_BITS(src, 14, 2),
			GET_BITS(src, 12, 2),
			GET_BITS(src, 10, 2),
			GET_BITS(src, 8, 2)
		);

		value = shuffle(value, swz);
	}

	if (do_abs)
	{
		value = abs(value);
	}

	if (TEST_BIT(src, 16))
	{
		value = -value;
	}

	return value;
}

void main()
{
	// Initialize output registers
	for (int i = 0; i < 16; ++i)
	{
		dest[i] = vec4(0., 0., 0., 1.);
	}

	int callstack[8];
	int stack_ptr = 0;
	int current_instruction = 0;

	d3.end = false;

	while (current_instruction < 512)
	{
		if (d3.end)
		{
			break;
		}

		instr = vp_instructions[current_instruction];
		current_instruction++;

		d0 = unpack_D0(instr.x);
		d1 = unpack_D1(instr.y);
		d2 = unpack_D2(instr.z);
		d3 = unpack_D3(instr.w);

		uint vec_opcode = d1.vec_opcode;
		uint sca_opcode = d1.sca_opcode;

		if (d0.cond_test_enable && d0.cond == 0)
		{
			vec_opcode = RSX_VEC_OPCODE_NOP;
			sca_opcode = RSX_SCA_OPCODE_NOP;
		}

		if (vec_opcode == RSX_VEC_OPCODE_ARL)
		{
			a[d0.dst_tmp] = ivec4(read_src(0));
		}
		else if (vec_opcode != RSX_VEC_OPCODE_NOP)
		{
			vec4 value = read_src(0);
			switch (vec_opcode)
			{
			case RSX_VEC_OPCODE_MOV: break;
			case RSX_VEC_OPCODE_MUL: value *= read_src(1); break;
			case RSX_VEC_OPCODE_ADD: value += read_src(2); break;
			case RSX_VEC_OPCODE_MAD: value = fma(value, read_src(1), read_src(2)); break;
			case RSX_VEC_OPCODE_DP3: value = vec4(dot(value.xyz, read_src(1).xyz)); break;
			case RSX_VEC_OPCODE_DPH: value = vec4(dot(vec4(value.xyz, 1.0), read_src(1))); break;
			case RSX_VEC_OPCODE_DP4: value = vec4(dot(value, read_src(1))); break;
			case RSX_VEC_OPCODE_DST: value = _distance(value, read_src(1)); break;
			case RSX_VEC_OPCODE_MIN: value = min(value, read_src(1)); break;
			case RSX_VEC_OPCODE_MAX: value = max(value, read_src(1)); break;
			case RSX_VEC_OPCODE_SLT: value = vec4(lessThan(value, read_src(1))); break;
			case RSX_VEC_OPCODE_SGE: value = vec4(greaterThanEqual(value, read_src(1))); break;
			case RSX_VEC_OPCODE_FRC: value = fract(value); break;
			case RSX_VEC_OPCODE_FLR: value = floor(value); break;
			case RSX_VEC_OPCODE_SEQ: value = vec4(equal(value, read_src(1))); break;
			case RSX_VEC_OPCODE_SFL: value = vec4(0); break;
			case RSX_VEC_OPCODE_SGT: value = vec4(greaterThan(value, read_src(1))); break;
			case RSX_VEC_OPCODE_SLE: value = vec4(lessThanEqual(value, read_src(1))); break;
			case RSX_VEC_OPCODE_SNE: value = vec4(notEqual(value, read_src(1))); break;
			case RSX_VEC_OPCODE_STR: value = vec4(1); break;
			case RSX_VEC_OPCODE_SSG: value = sign(value); break;
			}

			write_vec(value);
		}

		if (sca_opcode != RSX_SCA_OPCODE_NOP)
		{
			float value = read_src(2).x;
			switch (sca_opcode)
			{
			case RSX_SCA_OPCODE_MOV: break;
			case RSX_SCA_OPCODE_RCP: value = 1.0 / value; break;
			case RSX_SCA_OPCODE_RCC: value = clamp(1.0 / value, 5.42101e-20, 1.884467e19);  break;
			case RSX_SCA_OPCODE_RSQ: value = 1.0 / sqrt(value); break;
			case RSX_SCA_OPCODE_EXP: value = exp(value); break;
			case RSX_SCA_OPCODE_LOG: value = log(value); break;
			//case RSX_SCA_OPCODE_LIT: value = lit_legacy(value); break;
			case RSX_SCA_OPCODE_LG2: value = log2(value); break;
			case RSX_SCA_OPCODE_EX2: value = exp2(value); break;
			case RSX_SCA_OPCODE_SIN: value = sin(value); break;
			case RSX_SCA_OPCODE_COS: value = cos(value); break;

			case RSX_SCA_OPCODE_BRA:
				// Jump by address register
				if (dynamic_branch()) current_instruction = int(read_addr_reg().x);
				continue;
			case RSX_SCA_OPCODE_BRI:
				// Jump immediate
				if (dynamic_branch()) current_instruction = branch_addr();
				continue;
			case RSX_SCA_OPCODE_CAL:
				// Call immediate
				if (dynamic_branch())
				{
					callstack[stack_ptr] = current_instruction;
					stack_ptr++;
					current_instruction = branch_addr();
				}
				continue;
			case RSX_SCA_OPCODE_CLI:
				// Unknown
				continue;
			case RSX_SCA_OPCODE_RET:
				// Return
				if (dynamic_branch())
				{
					if (stack_ptr == 0) return;
					current_instruction = callstack[stack_ptr];
					stack_ptr--;
				}
				continue;
			case RSX_SCA_OPCODE_BRB:
				// Branch by boolean mask
				if (static_branch())
				{
					current_instruction = branch_addr();
				}
				continue;
			case RSX_SCA_OPCODE_CLB:
				// Call by boolean mask
				if (static_branch())
				{
					callstack[stack_ptr] = current_instruction;
					stack_ptr++;
					current_instruction = branch_addr();
				}
				continue;
			//case RSX_SCA_OPCODE_PSH:
			//case RSX_SCA_OPCODE_POP:
			}

			write_sca(value);
		}
	}

	// Unconditionally update COLOR0 and SPECULAR0
	write_output(1, 0);
	write_output(2, 1);

	// Conditionally update COLOR1 and SPECULAR1 depending on 2-sided mask
	if (control == 0)
	{
		dest[3] = dest[1];
		dest[4] = dest[2];
	}
	else
	{
		// 2-sided lighting
		write_output(3, 2);
		write_output(4, 3);
	}

	if (!attribute_enabled(1 << 4))
	{
		dest[5].x = 0;
	}

	if (attribute_enabled(1 << 5))
	{
		gl_PointSize = dest[6].x;
	}
	else
	{
		gl_PointSize = point_size;
	}

	write_clip_distance(0, 6, user_clip_enabled[0].x > 0, dest[5].y * user_clip_factor[0].x);
	write_clip_distance(1, 7, user_clip_enabled[0].y > 0, dest[5].z * user_clip_factor[0].y);
	write_clip_distance(2, 8, user_clip_enabled[0].z > 0, dest[5].w * user_clip_factor[0].z);
	write_clip_distance(3, 9, user_clip_enabled[0].w > 0, dest[6].y * user_clip_factor[0].w);
	write_clip_distance(4, 10, user_clip_enabled[1].x > 0, dest[6].z * user_clip_factor[1].x);
	write_clip_distance(5, 11, user_clip_enabled[1].y > 0, dest[6].w * user_clip_factor[1].y);

	write_output(15, 12);
	write_output(6, 13);
	write_output(7, 14);
	write_output(8, 15);
	write_output(9, 16);
	write_output(10, 17);
	write_output(11, 18);
	write_output(12, 19);
	write_output(13, 20);
	write_output(14, 21);

	vec4 pos = dest[0] * scale_offset_mat;

#ifdef Z_NEGATIVE_ONE_TO_ONE
	pos.z = (pos.z + pos.z) - pos.w;
#endif

	gl_Position = pos;
}

)"