#pragma once

#include "program_util.h"
#include "Assembler/FPOpcodes.h"

#include <string>
#include <vector>

enum register_type
{
	RSX_FP_REGISTER_TYPE_TEMP = 0,
	RSX_FP_REGISTER_TYPE_INPUT = 1,
	RSX_FP_REGISTER_TYPE_CONSTANT = 2,
	RSX_FP_REGISTER_TYPE_UNKNOWN = 3,
};

enum register_precision
{
	RSX_FP_PRECISION_REAL = 0,
	RSX_FP_PRECISION_HALF = 1,
	RSX_FP_PRECISION_FIXED12 = 2,
	RSX_FP_PRECISION_FIXED9 = 3,
	RSX_FP_PRECISION_SATURATE = 4,
	RSX_FP_PRECISION_UNKNOWN = 5 // Unknown what this actually does; seems to do nothing on hwtests but then why would their compiler emit it?
};

using enum rsx::assembler::FP_opcode;

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

	struct
	{
		u32                  : 9;
		u32 write_mask       : 4;
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
		u32 src0_prec_mod    : 3; // Precision modifier for src0 (many games)
		u32 src1_prec_mod    : 3; // Precision modifier for src1 (CoD:MW series)
		u32 src2_prec_mod    : 3; // Precision modifier for src2 (unproven, should affect MAD instruction)
		u32 scale            : 3;
		u32 opcode_hi        : 1; // Opcode high bit
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

union SRC_Common
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
	};
};

constexpr const char* rsx_fp_input_attr_regs[] =
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
	struct data_storage_helper
	{
		void* data_ptr = nullptr;
		std::vector<char> local_storage{};

		data_storage_helper() = default;

		data_storage_helper(void* ptr)
		{
			data_ptr = ptr;
			local_storage.clear();
		}

		data_storage_helper(const data_storage_helper& other)
		{
			this->operator=(other);
		}

		data_storage_helper(data_storage_helper&& other) noexcept
			: data_ptr(other.data_ptr)
			, local_storage(std::move(other.local_storage))
		{
			other.data_ptr = nullptr;
		}

		data_storage_helper& operator=(const data_storage_helper& other)
		{
			if (this == &other) return *this;

			if (other.data_ptr == other.local_storage.data())
			{
				local_storage = other.local_storage;
				data_ptr = local_storage.data();
			}
			else
			{
				data_ptr = other.data_ptr;
				local_storage.clear();
			}

			return *this;
		}

		data_storage_helper& operator=(data_storage_helper&& other) noexcept
		{
			if (this == &other) return *this;

			data_ptr = other.data_ptr;
			local_storage = std::move(other.local_storage);
			other.data_ptr = nullptr;

			return *this;
		}

		void deep_copy(u32 max_length)
		{
			if (local_storage.empty() && data_ptr)
			{
				local_storage.resize(max_length);
				std::memcpy(local_storage.data(), data_ptr, max_length);
				data_ptr = local_storage.data();
			}
		}

	} mutable data{};

	u32 offset = 0;
	u32 ucode_length = 0;
	u32 total_length = 0;
	u32 ctrl = 0;
	u32 texcoord_control_mask = 0;
	u32 mrt_buffers_count = 0;

	bool two_sided_lighting = false;

	rsx::fragment_program_texture_state texture_state;
	rsx::fragment_program_texture_config texture_params;

	bool valid = false;

	RSXFragmentProgram() = default;

	rsx::texture_dimension_extended get_texture_dimension(u8 id) const
	{
		return rsx::texture_dimension_extended{static_cast<u8>((texture_state.texture_dimensions >> (id * 2)) & 0x3)};
	}

	bool texcoord_is_2d(u8 index) const
	{
		return !!(texcoord_control_mask & (1u << index));
	}

	bool texcoord_is_point_coord(u8 index) const
	{
		index += 16;
		return !!(texcoord_control_mask & (1u << index));
	}

	static RSXFragmentProgram clone(const RSXFragmentProgram& prog)
	{
		RSXFragmentProgram result = prog;
		result.clone_data();
		return result;
	}

	void* get_data() const
	{
		return data.data_ptr;
	}

	void clone_data() const
	{
		ensure(ucode_length);
		data.deep_copy(ucode_length);
	}
};
