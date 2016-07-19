#pragma once
#include "GCM.h"
#include "RSXTexture.h"

enum register_type
{
	RSX_FP_REGISTER_TYPE_TEMP = 0,
	RSX_FP_REGISTER_TYPE_INPUT = 1,
	RSX_FP_REGISTER_TYPE_CONSTANT = 2,
	RSX_FP_REGISTER_TYPE_UNKNOWN = 3,
};

enum fp_opcode
{
	RSX_FP_OPCODE_NOP        = 0x00, // No-Operation
	RSX_FP_OPCODE_MOV        = 0x01, // Move
	RSX_FP_OPCODE_MUL        = 0x02, // Multiply
	RSX_FP_OPCODE_ADD        = 0x03, // Add
	RSX_FP_OPCODE_MAD        = 0x04, // Multiply-Add
	RSX_FP_OPCODE_DP3        = 0x05, // 3-component Dot Product
	RSX_FP_OPCODE_DP4        = 0x06, // 4-component Dot Product
	RSX_FP_OPCODE_DST        = 0x07, // Distance
	RSX_FP_OPCODE_MIN        = 0x08, // Minimum
	RSX_FP_OPCODE_MAX        = 0x09, // Maximum
	RSX_FP_OPCODE_SLT        = 0x0A, // Set-If-LessThan
	RSX_FP_OPCODE_SGE        = 0x0B, // Set-If-GreaterEqual
	RSX_FP_OPCODE_SLE        = 0x0C, // Set-If-LessEqual
	RSX_FP_OPCODE_SGT        = 0x0D, // Set-If-GreaterThan
	RSX_FP_OPCODE_SNE        = 0x0E, // Set-If-NotEqual
	RSX_FP_OPCODE_SEQ        = 0x0F, // Set-If-Equal
	RSX_FP_OPCODE_FRC        = 0x10, // Fraction (fract)
	RSX_FP_OPCODE_FLR        = 0x11, // Floor
	RSX_FP_OPCODE_KIL        = 0x12, // Kill fragment
	RSX_FP_OPCODE_PK4        = 0x13, // Pack four signed 8-bit values
	RSX_FP_OPCODE_UP4        = 0x14, // Unpack four signed 8-bit values
	RSX_FP_OPCODE_DDX        = 0x15, // Partial-derivative in x (Screen space derivative w.r.t. x)
	RSX_FP_OPCODE_DDY        = 0x16, // Partial-derivative in y (Screen space derivative w.r.t. y)
	RSX_FP_OPCODE_TEX        = 0x17, // Texture lookup
	RSX_FP_OPCODE_TXP        = 0x18, // Texture sample with projection (Projective texture lookup)
	RSX_FP_OPCODE_TXD        = 0x19, // Texture sample with partial differentiation (Texture lookup with derivatives)
	RSX_FP_OPCODE_RCP        = 0x1A, // Reciprocal
	RSX_FP_OPCODE_RSQ        = 0x1B, // Reciprocal Square Root
	RSX_FP_OPCODE_EX2        = 0x1C, // Exponentiation base 2
	RSX_FP_OPCODE_LG2        = 0x1D, // Log base 2
	RSX_FP_OPCODE_LIT        = 0x1E, // Lighting coefficients
	RSX_FP_OPCODE_LRP        = 0x1F, // Linear Interpolation
	RSX_FP_OPCODE_STR        = 0x20, // Set-If-True
	RSX_FP_OPCODE_SFL        = 0x21, // Set-If-False
	RSX_FP_OPCODE_COS        = 0x22, // Cosine
	RSX_FP_OPCODE_SIN        = 0x23, // Sine
	RSX_FP_OPCODE_PK2        = 0x24, // Pack two 16-bit floats
	RSX_FP_OPCODE_UP2        = 0x25, // Unpack two 16-bit floats
	RSX_FP_OPCODE_POW        = 0x26, // Power
	RSX_FP_OPCODE_PKB        = 0x27, // Pack bytes
	RSX_FP_OPCODE_UPB        = 0x28, // Unpack bytes
	RSX_FP_OPCODE_PK16       = 0x29, // Pack 16 bits
	RSX_FP_OPCODE_UP16       = 0x2A, // Unpack 16
	RSX_FP_OPCODE_BEM        = 0x2B, // Bump-environment map (a.k.a. 2D coordinate transform)
	RSX_FP_OPCODE_PKG        = 0x2C, // Pack with sRGB transformation
	RSX_FP_OPCODE_UPG        = 0x2D, // Unpack gamma
	RSX_FP_OPCODE_DP2A       = 0x2E, // 2-component dot product with scalar addition
	RSX_FP_OPCODE_TXL        = 0x2F, // Texture sample with explicit LOD
	RSX_FP_OPCODE_TXB        = 0x31, // Texture sample with bias
	RSX_FP_OPCODE_TEXBEM     = 0x33,
	RSX_FP_OPCODE_TXPBEM     = 0x34,
	RSX_FP_OPCODE_BEMLUM     = 0x35,
	RSX_FP_OPCODE_REFL       = 0x36, // Reflection vector
	RSX_FP_OPCODE_TIMESWTEX  = 0x37,
	RSX_FP_OPCODE_DP2        = 0x38, // 2-component dot product
	RSX_FP_OPCODE_NRM        = 0x39, // Normalize
	RSX_FP_OPCODE_DIV        = 0x3A, // Division
	RSX_FP_OPCODE_DIVSQ      = 0x3B, // Divide by Square Root
	RSX_FP_OPCODE_LIF        = 0x3C, // Final part of LIT
	RSX_FP_OPCODE_FENCT      = 0x3D, // Fence T?
	RSX_FP_OPCODE_FENCB      = 0x3E, // Fence B?
	RSX_FP_OPCODE_BRK        = 0x40, // Break
	RSX_FP_OPCODE_CAL        = 0x41, // Subroutine call
	RSX_FP_OPCODE_IFE        = 0x42, // If
	RSX_FP_OPCODE_LOOP       = 0x43, // Loop
	RSX_FP_OPCODE_REP        = 0x44, // Repeat
	RSX_FP_OPCODE_RET        = 0x45  // Return
};

union OPDEST
{
	u32 HEX;

	struct
	{
		u32 end              : 1; // Set to 1 if this is the last instruction
		u32 dest_reg         : 6; // Destination register index
		u32 fp16             : 1; // Destination is a half register (H0 to H47)
		u32 set_cond         : 1; // Condition Code Registers (CC0 and CC1) are updated
		u32 mask_x           : 1;
		u32 mask_y           : 1;
		u32 mask_z           : 1;
		u32 mask_w           : 1;
		u32 src_attr_reg_num : 4;
		u32 tex_num          : 4;
		u32 exp_tex          : 1; // _bx2
		u32 prec             : 2;
		u32 opcode           : 6;
		u32 no_dest          : 1;
		u32 saturate         : 1; // _sat
	};
};

union SRC0
{
	u32 HEX;

	struct
	{
		u32 reg_type        : 2;
		u32 tmp_reg_index   : 6;
		u32 fp16            : 1;
		u32 swizzle_x       : 2;
		u32 swizzle_y       : 2;
		u32 swizzle_z       : 2;
		u32 swizzle_w       : 2;
		u32 neg             : 1;
		u32 exec_if_lt      : 1;
		u32 exec_if_eq      : 1;
		u32 exec_if_gr      : 1;
		u32 cond_swizzle_x  : 2;
		u32 cond_swizzle_y  : 2;
		u32 cond_swizzle_z  : 2;
		u32 cond_swizzle_w  : 2;
		u32 abs : 1;
		u32 cond_mod_reg_index : 1;
		u32 cond_reg_index     : 1;
	};
};

union SRC1
{
	u32 HEX;

	struct
	{
		u32 reg_type         : 2;
		u32 tmp_reg_index    : 6;
		u32 fp16             : 1;
		u32 swizzle_x        : 2;
		u32 swizzle_y        : 2;
		u32 swizzle_z        : 2;
		u32 swizzle_w        : 2;
		u32 neg              : 1;
		u32 abs              : 1;
		u32 input_mod_src0   : 3;
		u32                  : 6;
		u32 scale            : 3;
		u32 opcode_is_branch : 1;
	};

	struct
	{
		u32 else_offset      : 31;
		u32                  : 1;
	};

	// LOOP, REP
	struct
	{
		u32                  : 2;
		u32 end_counter      : 8; // End counter value for LOOP or rep count for REP
		u32 init_counter     : 8; // Initial counter value for LOOP
		u32                  : 1;
		u32 increment        : 8; // Increment value for LOOP
	};
};

union SRC2
{
	u32 HEX;

	u32 end_offset;

	struct
	{
		u32 reg_type         : 2;
		u32 tmp_reg_index    : 6;
		u32 fp16             : 1;
		u32 swizzle_x        : 2;
		u32 swizzle_y        : 2;
		u32 swizzle_z        : 2;
		u32 swizzle_w        : 2;
		u32 neg              : 1;
		u32 abs              : 1;
		u32 addr_reg         : 11;
		u32 use_index_reg    : 1;
		u32 perspective_corr : 1;
	};
};

static const char* rsx_fp_input_attr_regs[] =
{
	"WPOS", "COL0", "COL1", "FOGC", "TEX0",
	"TEX1", "TEX2", "TEX3", "TEX4", "TEX5",
	"TEX6", "TEX7", "TEX8", "TEX9", "SSA"
};

static const std::string rsx_fp_op_names[] =
{
	"NOP", "MOV", "MUL", "ADD", "MAD", "DP3", "DP4",
	"DST", "MIN", "MAX", "SLT", "SGE", "SLE", "SGT",
	"SNE", "SEQ", "FRC", "FLR", "KIL", "PK4", "UP4",
	"DDX", "DDY", "TEX", "TXP", "TXD", "RCP", "RSQ",
	"EX2", "LG2", "LIT", "LRP", "STR", "SFL", "COS",
	"SIN", "PK2", "UP2", "POW", "PKB", "UPB", "PK16",
	"UP16", "BEM", "PKG", "UPG", "DP2A", "TXL", "NULL",
	"TXB", "NULL", "TEXBEM", "TXPBEM", "BEMLUM", "REFL", "TIMESWTEX",
	"DP2", "NRM", "DIV", "DIVSQ", "LIF", "FENCT", "FENCB",
	"NULL", "BRK", "CAL", "IFE", "LOOP", "REP", "RET"
};

struct RSXFragmentProgram
{
	u32 size;
	void *addr;
	u32 offset;
	u32 ctrl;
	u16 unnormalized_coords;
	rsx::comparison_function alpha_func;
	bool front_back_color_enabled : 1;
	bool back_color_diffuse_output : 1;
	bool back_color_specular_output : 1;
	bool front_color_diffuse_output : 1;
	bool front_color_specular_output : 1;
	u32 texture_dimensions;
	rsx::window_origin origin_mode;
	rsx::window_pixel_center pixel_center_mode;
	rsx::fog_mode fog_equation;
	u16 height;

	rsx::texture_dimension_extended get_texture_dimension(u8 id) const
	{
		return (rsx::texture_dimension_extended)((texture_dimensions >> (id * 2)) & 0x3);
	}

	void set_texture_dimension(const std::array<rsx::texture_dimension_extended, 16> &dimensions)
	{
		size_t id = 0;
		for (const rsx::texture_dimension_extended &dim : dimensions)
		{
			texture_dimensions &= ~(0x3 << (id * 2));
			u8 d = (u8)dim;
			texture_dimensions |= ((d & 0x3) << (id * 2));
			id++;
		}
	}

	RSXFragmentProgram()
		: size(0)
		, addr(0)
		, offset(0)
		, ctrl(0)
		, unnormalized_coords(0)
		, texture_dimensions(0)
	{
	}
};
