#pragma once

#include "Utilities/File.h"
#include "Utilities/lockless.h"
#include "Utilities/address_range.h"
#include "SPUThread.h"
#include "SPUAnalyser.h"
#include <vector>
#include <bitset>
#include <memory>
#include <string>
#include <deque>

// Helper class
class spu_cache
{
	fs::file m_file;

public:
	spu_cache() = default;

	spu_cache(const std::string& loc);

	spu_cache(spu_cache&&) noexcept = default;

	spu_cache& operator=(spu_cache&&) noexcept = default;

	~spu_cache();

	operator bool() const
	{
		return m_file.operator bool();
	}

	std::deque<struct spu_program> get();

	void add(const struct spu_program& func);

	static void initialize(bool build_existing_cache = true);

	struct precompile_data_t
	{
		u32 vaddr;
		std::vector<u32> inst_data;
		std::vector<u32> funcs;
	};

	bool collect_funcs_to_precompile = true;

	lf_queue<precompile_data_t> precompile_funcs;
};

struct spu_program
{
	// Address of the entry point in LS
	u32 entry_point;

	// Address of the data in LS
	u32 lower_bound;

	// Program data with intentionally wrong endianness (on LE platform opcode values are swapped)
	std::vector<u32> data;

	bool operator==(const spu_program& rhs) const noexcept;

	bool operator<(const spu_program& rhs) const noexcept;
};

class spu_item
{
public:
	// SPU program
	const spu_program data;

	// Compiled function pointer
	atomic_t<spu_function_t> compiled = nullptr;

	// Ubertrampoline generated for this item when it was latest
	atomic_t<spu_function_t> trampoline = nullptr;

	atomic_t<u8> cached = false;
	atomic_t<u8> logged = false;

	spu_item(spu_program&& data)
		: data(std::move(data))
	{
	}

	spu_item(const spu_item&) = delete;

	spu_item& operator=(const spu_item&) = delete;
};

// Helper class
class spu_runtime
{
	// All functions (2^20 bunches)
	std::array<lf_bunch<spu_item>, (1 << 20)> m_stuff;

	// Debug module output location
	std::string m_cache_path;

public:
	// Trampoline to spu_recompiler_base::dispatch
	static const spu_function_t tr_dispatch;

	// Trampoline to spu_recompiler_base::branch
	static const spu_function_t tr_branch;

	// Trampoline to legacy interpreter
	static const spu_function_t tr_interpreter;

	// Detect and call any recompiled function
	static const spu_function_t tr_all;

public:
	spu_runtime();

	spu_runtime(const spu_runtime&) = delete;

	spu_runtime& operator=(const spu_runtime&) = delete;

	const std::string& get_cache_path() const
	{
		return m_cache_path;
	}

	// Rebuild ubertrampoline for given identifier (first instruction)
	spu_function_t rebuild_ubertrampoline(u32 id_inst);

private:
	friend class spu_cache;

public:
	// Return new pointer for add()
	spu_item* add_empty(spu_program&&);

	// Find existing function
	spu_function_t find(const u32* ls, u32 addr) const;

	// Generate a patchable trampoline to spu_recompiler_base::branch
	spu_function_t make_branch_patchpoint(u16 data = 0) const;

	// All dispatchers (array allocated in jit memory)
	static std::array<atomic_t<spu_function_t>, (1 << 20)>* const g_dispatcher;

	// Recompiler entry point
	static const spu_function_t g_gateway;

	// Longjmp to the end of the gateway function (native CC)
	static void(*const g_escape)(spu_thread*);

	// Similar to g_escape, but doing tail call to the new function.
	static void(*const g_tail_escape)(spu_thread*, spu_function_t, u8*);

	// Interpreter table (spu_itype -> ptr)
	static std::array<u64, 256> g_interpreter_table;

	// Interpreter entry point
	static spu_function_t g_interpreter;
};

// SPU Recompiler instance base class
class spu_recompiler_base
{
public:
	enum : u8
	{
		s_reg_lr = 0,
		s_reg_sp = 1,
		s_reg_80 = 80,
		s_reg_127 = 127,

		s_reg_mfc_eal,
		s_reg_mfc_lsa,
		s_reg_mfc_tag,
		s_reg_mfc_size,

		// Max number of registers (for m_regmod)
		s_reg_max
	};

	// Classify terminator instructions
	enum class term_type : unsigned char
	{
		br,
		ret,
		call,
		fallthrough,
		indirect_call,
		interrupt_call,
	};

	// Value flags (TODO: only is_const is implemented)
	enum class vf : u32
	{
		is_const,
		is_mask,
		is_rel,
		is_null,

		__bitset_enum_max
	};

	enum compare_direction : u32
	{
		CMP_TURNAROUND_FLAG = 0x1,
		CMP_NEGATE_FLAG = 0x100,
		CMP_SLESS = 0,
		CMP_SGREATER = CMP_SLESS | CMP_TURNAROUND_FLAG,
		CMP_EQUAL,
		CMP_EQUAL2 = CMP_EQUAL | CMP_TURNAROUND_FLAG,
		CMP_LLESS,
		CMP_LGREATER = CMP_LLESS | CMP_TURNAROUND_FLAG,
		CMP_SGREATER_EQUAL = CMP_SLESS | CMP_NEGATE_FLAG,
		CMP_SLOWER_EQUAL = CMP_SGREATER | CMP_NEGATE_FLAG,
		CMP_NOT_EQUAL = CMP_EQUAL | CMP_NEGATE_FLAG,
		CMP_NOT_EQUAL2 = CMP_NOT_EQUAL | CMP_TURNAROUND_FLAG,
		CMP_LGREATER_EQUAL = CMP_LLESS | CMP_NEGATE_FLAG,
		CMP_LLOWER_EQUAL = CMP_LGREATER | CMP_NEGATE_FLAG,
		CMP_UNKNOWN,
	};

	struct reg_state_t
	{
		bs_t<vf> flag{+vf::is_null};
		u32 value{};
		u32 tag = umax;
		u32 known_ones{};
		u32 known_zeroes{};
		u32 origin = SPU_LS_SIZE;
		bool is_instruction = false;

		bool is_const() const;

		bool operator&(vf to_test) const;

		bool is_less_than(u32 imm) const;
		bool operator==(const reg_state_t& r) const;
		bool operator==(u32 imm) const;

		// Compare equality but try to ignore changes in unmasked bits
		bool compare_with_mask_indifference(const reg_state_t& r, u32 mask_bits) const;
		bool compare_with_mask_indifference(u32 imm, u32 mask_bits) const;
		bool unequal_with_mask_indifference(const reg_state_t& r, u32 mask_bits) const;

		// Convert constant-based value to mask-based value
		reg_state_t downgrade() const;

		// Connect two register states between different blocks
		reg_state_t merge(const reg_state_t& rhs, u32 current_pc) const;

		// Override value with newer value if needed
		reg_state_t build_on_top_of(const reg_state_t& rhs) const;

		// Get known zeroes mask
		u32 get_known_zeroes() const;

		// Get known ones mask
		u32 get_known_ones() const;

		// Invalidate value if non-constant and reached the point in history of its creation
		void invalidate_if_created(u32 current_pc);

		template <usz Count = 1>
		static std::conditional_t<Count == 1, reg_state_t, std::array<reg_state_t, Count>> make_unknown(u32 pc, u32 current_pc = SPU_LS_SIZE) noexcept
		{
			if constexpr (Count == 1)
			{
				reg_state_t v{};
				v.tag = alloc_tag();
				v.flag = {};
				v.origin = pc;
				v.is_instruction = pc == current_pc;
				return v;
			}
			else
			{
				std::array<reg_state_t, Count> result{};

				for (reg_state_t& state : result)
				{
					state = make_unknown<1>(pc, current_pc);
				}

				return result;
			}
		}

		bool compare_tags(const reg_state_t& rhs) const;

		static reg_state_t from_value(u32 value) noexcept;
		static u32 alloc_tag(bool reset = false) noexcept;
	};

	struct reduced_loop_t
	{
		bool active = false; // Single block loop detected
		bool failed = false;
		u32 loop_pc = SPU_LS_SIZE;
		u32 loop_end = SPU_LS_SIZE;

		// False: single-block loop
		// True: loop with a trailing block of aftermath (iteration update) stuff (like for (u32 i = 0; i < 10; /*update*/ i++))
		bool is_two_block_loop = false;
		bool has_cond_state = false;

		// Loop stay-in state requirement
		u64 cond_val_mask = umax;
		u64 cond_val_min = 0;
		u64 cond_val_size = 0;
		compare_direction cond_val_compare{};
		u64 cond_val_incr = 0;
		bool cond_val_incr_is_immediate = false;
		u64 cond_val_register_argument_idx = umax;
		u64 cond_val_register_idx = umax;
		bool cond_val_incr_before_cond = false;
		bool cond_val_incr_before_cond_taken_in_account = false;
		bool cond_val_is_immediate = false;

		// Loop attributes
		bool is_constant_expression = false;
		bool is_secret = false;

		struct supplemental_condition_t
		{
			u64 immediate_value = umax;
			u64 type_size = 0;
			compare_direction val_compare{};
		};

		// Supplemental loop condition:
		// Inner conditions that depend on extrnal values (not produced inside the loop)
		// all should evaluate to false in order for the optimization to work (at the moment)
		// So succeeding can be treated linearly 
		u64 expected_sup_conds = 0;
		u64 current_sup_conds_index = 0;
		std::vector<supplemental_condition_t> sup_conds;

		void take_cond_val_incr_before_cond_into_account()
		{
			if (cond_val_is_immediate && cond_val_incr_before_cond_taken_in_account && !cond_val_incr_before_cond_taken_in_account)
			{
				cond_val_min -= cond_val_incr;
				cond_val_min &= cond_val_mask;
				cond_val_incr_before_cond_taken_in_account = true;
			}
		}

		std::bitset<s_reg_max> loop_args;
		std::bitset<s_reg_max> loop_dicts;
		std::bitset<s_reg_max> loop_writes;

		struct origin_t
		{
			std::bitset<s_reg_max> regs{};
			u32 modified = 0;
			spu_itype_t mod1_type = spu_itype::UNK;
			spu_itype_t mod2_type = spu_itype::UNK;
			spu_itype_t mod3_type = spu_itype::UNK;
			u32 IMM = 0;

private:
			// Internal, please access using fixed order
			spu_itype_t access_type(u32 i) const
			{
				if (i > modified)
				{
					return spu_itype::UNK;
				}

				switch (i)
				{
				case 1: return mod1_type;
				case 2: return mod2_type;
				case 3: return mod3_type;
				default: return spu_itype::UNK;
				}

				return spu_itype::UNK;
			}
public:

			spu_itype_t reverse1_type()
			{
				return access_type(modified); 
			}

			spu_itype_t reverse2_type()
			{
				return access_type(modified - 1); 
			}

			spu_itype_t reverse3_type()
			{
				return access_type(modified - 2);
			}

			origin_t& join_with_this(const origin_t& rhs)
			{
				regs |= rhs.regs;
				return *this;
			}

			origin_t& join_with_this(u32 rhs)
			{
				regs.set(rhs);
				return *this;
			}

			origin_t& add_register_origin(u32 reg_val)
			{
				regs.set(reg_val);
				return *this;
			}

			bool is_single_reg_access(u32 reg_val) const
			{
				if (!modified)
				{
					return true;
				}

				return regs.count() == 1 && regs.test(reg_val);
			}

			bool is_loop_dictator(u32 reg_val, bool test_predictable = false, bool should_predictable = true) const
			{
				if (!modified)
				{
					return false;
				}

				if (regs.count() >= 1 && regs.test(reg_val))
				{
					if (!test_predictable)
					{
						return true;
					}

					if (modified > 1)
					{
						return should_predictable ^ true;
					}

					switch (mod1_type)
					{
					case spu_itype::A:
					{
						if (regs.count() == 2)
						{
							return should_predictable;
						}

						return should_predictable ^ true;
					}
					case spu_itype::AI:
					case spu_itype::AHI:
					{
						if (IMM && regs.count() == 1)
						{
							return should_predictable;
						}

						return should_predictable ^ true;
					}
					default: break;
					}

					return should_predictable ^ true;
				}

				return false;
			}

			bool is_predictable_loop_dictator(u32 reg_val) const
			{
				return is_loop_dictator(reg_val, true, true);
			}

			bool is_non_predictable_loop_dictator(u32 reg_val) const
			{
				return is_loop_dictator(reg_val, true, false);
			}

			bool is_null(u32 reg_val) const noexcept
			{
				if (modified)
				{
					return false;
				}

				if (regs.count() - (regs.test(reg_val) ? 1 : 0))
				{
					return false;
				}

				return true;
			}

			origin_t& add_instruction_modifier(spu_itype_t inst_type, u32 imm = 0)
			{
				if (inst_type == spu_itype::UNK)
				{
					mod1_type = spu_itype::UNK;
					mod2_type = spu_itype::UNK;
					mod3_type = spu_itype::UNK;
					IMM = umax;
					modified = 1;
					return *this;
				}

				if (modified == 1)
				{
					if (modified == 3)
					{
						mod1_type = spu_itype::UNK;
						mod2_type = spu_itype::UNK;
						mod3_type = spu_itype::UNK;
						IMM = umax;
						modified = 1;
						return *this;
					}

					bool is_ok = false;
					switch (inst_type)
					{
					case spu_itype::XSBH:
					{
						const auto prev_type = modified == 1 ? mod1_type : mod2_type;
						is_ok &= mod1_type == spu_itype::CEQB || mod1_type == spu_itype::CEQBI || mod1_type == spu_itype::CGTB || mod1_type == spu_itype::CGTBI || mod1_type == spu_itype::CLGTB || mod1_type == spu_itype::CLGTBI;
						break;
					}
					case spu_itype::ANDI:
					{
						const auto prev_type = modified == 1 ? mod1_type : mod2_type;
						is_ok &= mod1_type == spu_itype::CEQB || mod1_type == spu_itype::CEQBI || mod1_type == spu_itype::CGTB || mod1_type == spu_itype::CGTBI || mod1_type == spu_itype::CLGTB || mod1_type == spu_itype::CLGTBI;
						is_ok &= (spu_opcode_t{imm}.si10 & 0xff) == 0xff;
						break;
					}
					case spu_itype::CEQ:
					case spu_itype::CEQH:
					case spu_itype::CEQB:
					case spu_itype::CGT:
					case spu_itype::CGTH:
					case spu_itype::CGTB:
					case spu_itype::CLGT:
					case spu_itype::CLGTH:
					case spu_itype::CLGTB:
					{
						is_ok = modified == 1 && (mod1_type == spu_itype::AI || mod1_type == spu_itype::AHI);
						IMM = imm;
						break;
					}
					case spu_itype::CEQI:
					case spu_itype::CEQHI:
					case spu_itype::CEQBI:
					case spu_itype::CGTI:
					case spu_itype::CGTHI:
					case spu_itype::CGTBI:
					case spu_itype::CLGTI:
					case spu_itype::CLGTHI:
					case spu_itype::CLGTBI:
					{
						is_ok = modified == 1 && (mod1_type == spu_itype::AI || mod1_type == spu_itype::AHI);
						IMM = spu_opcode_t{imm}.si10;
						break;
					}
					}

					if (!is_ok)
					{
						mod1_type = spu_itype::UNK;
						mod2_type = spu_itype::UNK;
						mod3_type = spu_itype::UNK;
						IMM = umax;
						modified = 1;
						return *this;
					}

					(modified == 1 ? mod2_type : mod3_type) = inst_type;
					modified++;
					return *this;
				}

				mod1_type = inst_type;
				modified = 1;

				switch (inst_type)
				{
				case spu_itype::AHI:
				{
					IMM = spu_opcode_t{imm}.duplicate_si10();
					return *this;
				}
				case spu_itype::AI:
				case spu_itype::ORI:
				case spu_itype::XORI:
				case spu_itype::ANDI:
				
				case spu_itype::CEQI:
				case spu_itype::CEQHI:
				case spu_itype::CEQBI:
				case spu_itype::CGTI:
				case spu_itype::CGTHI:
				case spu_itype::CGTBI:
				case spu_itype::CLGTI:
				case spu_itype::CLGTHI:
				case spu_itype::CLGTBI:
				{
					IMM = spu_opcode_t{imm}.si10;
					return *this;
				}
				case spu_itype::ILA:
				{
					IMM = spu_opcode_t{imm}.i18;
					return *this;
				}
				case spu_itype::IOHL:
				case spu_itype::ILH:
				case spu_itype::ILHU:
				{
					IMM = spu_opcode_t{imm}.i16;
					return *this;
				}
				default:
				{
					IMM = imm;
					break;
				}
				}

				return *this;
			}
		};

		static origin_t make_reg(u32 reg_val) noexcept
		{
			origin_t org{};
			org.add_register_origin(reg_val);
			return org;
		}

		const origin_t* find_reg(u32 reg_val) const noexcept
		{
			for (auto& pair : regs)
			{
				if (pair.first == reg_val)
				{
					return &pair.second;
				}
			}

			return nullptr;
		}

		origin_t* find_reg(u32 reg_val) noexcept
		{
			return const_cast<origin_t*>(std::as_const(*this).find_reg(reg_val));
		}

		bool is_reg_null(u32 reg_val) const noexcept
		{
			if (const auto reg_found = find_reg(reg_val))
			{
				return reg_found->is_null(reg_val);
			}

			return true;
		}

		origin_t get_reg(u32 reg_val) noexcept
		{
			const auto org = find_reg(reg_val);
			return org ? *org : regs.emplace_back(reg_val, std::remove_reference_t<decltype(*org)>{}).second;
		}

		std::vector<std::pair<u8, origin_t>> regs;

		// Return old state for error reporting
		reduced_loop_t discard()
		{
			const reduced_loop_t old = *this;
			*this = reduced_loop_t{};
			return old;
		}
	};

protected:
	spu_runtime* m_spurt{};

	u32 m_pos;
	u32 m_size;
	u64 m_hash_start;

	// Bit indicating start of the block
	std::bitset<0x10000> m_block_info;

	// GPR modified by the instruction (-1 = not set)
	std::array<u8, 0x10000> m_regmod;

	std::bitset<0x10000> m_use_ra;
	std::bitset<0x10000> m_use_rb;
	std::bitset<0x10000> m_use_rc;

	// List of possible targets for the instruction (entry shouldn't exist for simple instructions)
	std::unordered_map<u32, std::vector<u32>, value_hash<u32, 2>> m_targets;

	// List of block predecessors
	std::unordered_map<u32, std::vector<u32>, value_hash<u32, 2>> m_preds;

	// List of function entry points and return points (set after BRSL, BRASL, BISL, BISLED)
	std::bitset<0x10000> m_entry_info;

	// Set after return points and disjoint chunks
	std::bitset<0x10000> m_ret_info;

	// Basic block information
	struct block_info
	{
		// Address of the chunk entry point (chunk this block belongs to)
		u32 chunk = 0x40000;

		// Number of instructions
		u16 size = 0;

		// Internal use flag
		bool analysed = false;

		// Terminator instruction type
		term_type terminator;

		// Bit mask of the registers modified in the block
		std::bitset<s_reg_max> reg_mod{};

		// Set if last modifying instruction produces xfloat
		std::bitset<s_reg_max> reg_mod_xf{};

		// Set if the initial register value in this block may be xfloat
		std::bitset<s_reg_max> reg_maybe_xf{};

		// Set if register is used in floating pont instruction
		std::bitset<s_reg_max> reg_maybe_float{};

		// Set if register is used as shuffle mask
		std::bitset<s_reg_max> reg_maybe_shuffle_mask{};

		// Number of times registers are used (before modified)
		std::array<u32, s_reg_max> reg_use{};

		// Bit mask of the trivial (u32 x 4) constant value resulting in this block
		std::bitset<s_reg_max> reg_const{};

		// Bit mask of register saved onto the stack before use
		std::bitset<s_reg_max> reg_save_dom{};

		// Address of the function
		u32 func = 0x40000;

		// Value subtracted from $SP in this block, negative if something funny is done on $SP
		u32 stack_sub = 0;

		// Constant values associated with reg_const
		std::array<u32, s_reg_max> reg_val32;

		// Registers loaded from the stack in this block (stack offset)
		std::array<u32, s_reg_max> reg_load_mod{};

		// Single source of the reg value (dominating block address within the same chunk) or a negative number
		std::array<u32, s_reg_max> reg_origin, reg_origin_abs;

		// All possible successor blocks
		std::vector<u32> targets;

		// All predeccessor blocks
		std::vector<u32> preds;
	};

	// Sorted basic block info
	std::map<u32, block_info> m_bbs;

	// Sorted advanced block (chunk) list
	std::vector<u32> m_chunks;

	// Function information
	struct func_info
	{
		// Size to the end of last basic block
		u16 size = 0;

		// Determines whether a function is eligible for optimizations
		bool good = false;

		// Call targets
		std::vector<u32> calls;

		// Register save info (stack offset)
		std::array<u32, s_reg_max> reg_save_off{};
	};

	// Sorted function info
	std::map<u32, func_info> m_funcs;

	// TODO: Add patterns
	// Not a bitset to allow more possibilities
	enum class inst_attr : u8
	{
		none,
		omit,
		putllc16,
		putllc0,
		rchcnt_loop,
		reduced_loop,
	};

	std::vector<inst_attr> m_inst_attrs;

	struct pattern_info
	{
		// Info via integral
		u64 info{};

		// Info via additional erased-typed pointer
		std::shared_ptr<void> info_ptr;
	};

	std::map<u32, pattern_info> m_patterns;

	void add_pattern(inst_attr attr, u32 start, u64 info, std::shared_ptr<void> info_ptr = nullptr);

private:
	// For private use
	std::bitset<0x10000> m_bits;

	// For private use
	std::vector<u32> workload;

public:
	spu_recompiler_base();

	virtual ~spu_recompiler_base();

	// Initialize
	virtual void init() = 0;

	// Compile function
	virtual spu_function_t compile(spu_program&&) = 0;

	// Default dispatch function fallback (second arg is unused)
	static void dispatch(spu_thread&, void*, u8* rip);

	// Target for the unresolved patch point (second arg is unused)
	static void branch(spu_thread&, void*, u8* rip);

	// Legacy interpreter loop
	static void old_interpreter(spu_thread&, void* ls, u8*);

	// Get the function data at specified address
	spu_program analyse(const be_t<u32>* ls, u32 entry_point, std::map<u32, std::vector<u32>>* out_target_list = nullptr);

	// Print analyser internal state
	void dump(const spu_program& result, std::string& out, u32 block_min = 0, u32 block_max = SPU_LS_SIZE);

	// Get SPU Runtime
	spu_runtime& get_runtime()
	{
		if (!m_spurt)
		{
			init();
		}

		return *m_spurt;
	}

	// Create recompiler instance (ASMJIT)
	static std::unique_ptr<spu_recompiler_base> make_asmjit_recompiler();

	// Create recompiler instance (LLVM)
	static std::unique_ptr<spu_recompiler_base> make_llvm_recompiler(u8 magn = 0);

	// Create recompiler instance (interpreter-based LLVM)
	static std::unique_ptr<spu_recompiler_base> make_fast_llvm_recompiler();
};
