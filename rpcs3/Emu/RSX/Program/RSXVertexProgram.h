#pragma once

#include "program_util.h"

#include <vector>
#include <bitset>
#include <set>

enum vp_reg_type
{
	RSX_VP_REGISTER_TYPE_TEMP = 1,
	RSX_VP_REGISTER_TYPE_INPUT = 2,
	RSX_VP_REGISTER_TYPE_CONSTANT = 3,
};

enum sca_opcode
{
	RSX_SCA_OPCODE_NOP = 0x00, // No-Operation
	RSX_SCA_OPCODE_MOV = 0x01, // Move (copy)
	RSX_SCA_OPCODE_RCP = 0x02, // Reciprocal
	RSX_SCA_OPCODE_RCC = 0x03, // Reciprocal clamped
	RSX_SCA_OPCODE_RSQ = 0x04, // Reciprocal square root
	RSX_SCA_OPCODE_EXP = 0x05, // Exponential base 2 (low-precision)
	RSX_SCA_OPCODE_LOG = 0x06, // Logarithm base 2 (low-precision)
	RSX_SCA_OPCODE_LIT = 0x07, // Lighting calculation
	RSX_SCA_OPCODE_BRA = 0x08, // Branch
	RSX_SCA_OPCODE_BRI = 0x09, // Branch by CC register
	RSX_SCA_OPCODE_CAL = 0x0a, // Subroutine call
	RSX_SCA_OPCODE_CLI = 0x0b, // Subroutine call by CC register
	RSX_SCA_OPCODE_RET = 0x0c, // Return from subroutine
	RSX_SCA_OPCODE_LG2 = 0x0d, // Logarithm base 2
	RSX_SCA_OPCODE_EX2 = 0x0e, // Exponential base 2
	RSX_SCA_OPCODE_SIN = 0x0f, // Sine function
	RSX_SCA_OPCODE_COS = 0x10, // Cosine function
	RSX_SCA_OPCODE_BRB = 0x11, // Branch by Boolean constant
	RSX_SCA_OPCODE_CLB = 0x12, // Subroutine call by Boolean constant
	RSX_SCA_OPCODE_PSH = 0x13, // Push onto stack
	RSX_SCA_OPCODE_POP = 0x14, // Pop from stack
};

enum vec_opcode
{
	RSX_VEC_OPCODE_NOP = 0x00, // No-Operation
	RSX_VEC_OPCODE_MOV = 0x01, // Move
	RSX_VEC_OPCODE_MUL = 0x02, // Multiply
	RSX_VEC_OPCODE_ADD = 0x03, // Addition
	RSX_VEC_OPCODE_MAD = 0x04, // Multiply-Add
	RSX_VEC_OPCODE_DP3 = 0x05, // 3-component Dot Product
	RSX_VEC_OPCODE_DPH = 0x06, // Homogeneous Dot Product
	RSX_VEC_OPCODE_DP4 = 0x07, // 4-component Dot Product
	RSX_VEC_OPCODE_DST = 0x08, // Calculate distance vector
	RSX_VEC_OPCODE_MIN = 0x09, // Minimum
	RSX_VEC_OPCODE_MAX = 0x0a, // Maximum
	RSX_VEC_OPCODE_SLT = 0x0b, // Set-If-LessThan
	RSX_VEC_OPCODE_SGE = 0x0c, // Set-If-GreaterEqual
	RSX_VEC_OPCODE_ARL = 0x0d, // Load to address register (round down)
	RSX_VEC_OPCODE_FRC = 0x0e, // Extract fractional part (fraction)
	RSX_VEC_OPCODE_FLR = 0x0f, // Round down (floor)
	RSX_VEC_OPCODE_SEQ = 0x10, // Set-If-Equal
	RSX_VEC_OPCODE_SFL = 0x11, // Set-If-False
	RSX_VEC_OPCODE_SGT = 0x12, // Set-If-GreaterThan
	RSX_VEC_OPCODE_SLE = 0x13, // Set-If-LessEqual
	RSX_VEC_OPCODE_SNE = 0x14, // Set-If-NotEqual
	RSX_VEC_OPCODE_STR = 0x15, // Set-If-True
	RSX_VEC_OPCODE_SSG = 0x16, // Convert positive values to 1 and negative values to -1
	RSX_VEC_OPCODE_TXL = 0x19, // Texture fetch
};

union D0
{
	u32 HEX;

	struct
	{
		u32 addr_swz             : 2;
		u32 mask_w               : 2;
		u32 mask_z               : 2;
		u32 mask_y               : 2;
		u32 mask_x               : 2;
		u32 cond                 : 3;
		u32 cond_test_enable     : 1;
		u32 cond_update_enable_0 : 1;
		u32 dst_tmp              : 6;
		u32 src0_abs             : 1;
		u32 src1_abs             : 1;
		u32 src2_abs             : 1;
		u32 addr_reg_sel_1       : 1;
		u32 cond_reg_sel_1       : 1;
		u32 staturate            : 1;
		u32 index_input          : 1;
		u32                      : 1;
		u32 cond_update_enable_1 : 1;
		u32 vec_result           : 1;
		u32                      : 1;
	};

	struct
	{
		u32         : 23;
		u32 iaddrh2 : 1;
		u32         : 8;
	};
};

union D1
{
	u32 HEX;

	struct
	{
		u32 src0h      : 8;
		u32 input_src  : 4;
		u32 const_src  : 10;
		u32 vec_opcode : 5;
		u32 sca_opcode : 5;
	};
};

union D2
{
	u32 HEX;

	struct
	{
		u32 src2h  : 6;
		u32 src1   : 17;
		u32 src0l  : 9;
	};
	struct
	{
		u32 iaddrh : 6;
		u32        : 26;
	};
	struct
	{
		u32         : 8;
		u32 tex_num : 2;	// Actual field may be 4 bits wide, but we only have 4 TIUs
		u32         : 22;
	};
};

union D3
{
	u32 HEX;

	struct
	{
		u32 end             : 1;
		u32 index_const     : 1;
		u32 dst             : 5;
		u32 sca_dst_tmp     : 6;
		u32 vec_writemask_w : 1;
		u32 vec_writemask_z : 1;
		u32 vec_writemask_y : 1;
		u32 vec_writemask_x : 1;
		u32 sca_writemask_w : 1;
		u32 sca_writemask_z : 1;
		u32 sca_writemask_y : 1;
		u32 sca_writemask_x : 1;
		u32 src2l           : 11;
	};

	struct
	{
		u32                 : 23;
		u32 branch_index	: 5;	//Index into transform_program_branch_bits [x]
		u32 brb_cond_true	: 1;	//If set, branch is taken if (b[x]) else if (!b[x])
		u32 iaddrl          : 3;
	};
};

union SRC
{
	union
	{
		u32 HEX;

		struct
		{
			u32 src0l : 9;
			u32 src0h : 8;
		};

		struct
		{
			u32 src1  : 17;
		};

		struct
		{
			u32 src2l : 11;
			u32 src2h : 6;
		};
	};

	struct
	{
		u32 reg_type : 2;
		u32 tmp_src  : 6;
		u32 swz_w    : 2;
		u32 swz_z    : 2;
		u32 swz_y    : 2;
		u32 swz_x    : 2;
		u32 neg      : 1;
	};
};

static const std::string rsx_vp_sca_op_names[] =
{
	"NOP", "MOV", "RCP", "RCC", "RSQ", "EXP", "LOG",
	"LIT", "BRA", "BRI", "CAL", "CLI", "RET", "LG2",
	"EX2", "SIN", "COS", "BRB", "CLB", "PSH", "POP"
};

static const std::string rsx_vp_vec_op_names[] =
{
	"NOP", "MOV", "MUL", "ADD", "MAD", "DP3", "DPH", "DP4",
	"DST", "MIN", "MAX", "SLT", "SGE", "ARL", "FRC", "FLR",
	"SEQ", "SFL", "SGT", "SLE", "SNE", "STR", "SSG", "NULL", "NULL", "TXL"
};

struct RSXVertexProgram
{
	std::vector<u32> data;
	rsx::vertex_program_texture_state texture_state;
	u32 ctrl = 0;
	u32 output_mask = 0;
	u32 base_address = 0;
	u32 entry = 0;
	std::bitset<rsx::max_vertex_program_instructions> instruction_mask;
	std::set<u32> jump_table;

	rsx::texture_dimension_extended get_texture_dimension(u8 id) const
	{
		return rsx::texture_dimension_extended{static_cast<u8>((texture_state.texture_dimensions >> (id * 2)) & 0x3)};
	}
};
