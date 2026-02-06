#pragma once

#include "spu_optimizer.h"
#include <tuple>
#include <optional>

namespace spu_optimizer::pattern
{
	// Placeholder for capturing instruction history indices
	template <u32 Id>
	struct inst_t
	{
		static constexpr u32 id = Id;
	};

	template <u32 Id>
	inline constexpr inst_t<Id> inst{};

	// Result type: tuple of captured history_index values
	template <u32 MaxId>
	struct match_result
	{
		std::array<history_index, MaxId + 1> captures{};

		constexpr history_index get(u32 id) const { return captures[id]; }
		template <u32 Id>
		constexpr history_index get(inst_t<Id>) const { return captures[Id]; }
	};

	// Trait to extract the maximum inst<N> ID used in a pattern
	template <typename T>
	struct max_inst_id { static constexpr u32 value = 0; };

	template <u32 Id>
	struct max_inst_id<inst_t<Id>> { static constexpr u32 value = Id; };

	// Pattern element: either a concrete history_index or a capture placeholder
	template <typename T>
	struct is_inst : std::false_type {};

	template <u32 Id>
	struct is_inst<inst_t<Id>> : std::true_type {};

	// Helper to check if an operand matches and potentially capture it
	template <u32 MaxId>
	struct matcher
	{
		match_result<MaxId>& result;
		bool ok = true;

		// Match against inst<N>: capture or verify same capture
		template <u32 Id>
		bool match_operand(inst_t<Id>, history_index actual)
		{
			if (result.captures[Id] == 0)
			{
				// First time seeing this capture slot, store it
				result.captures[Id] = actual;
				return true;
			}
			else
			{
				// Already captured, verify it matches
				return result.captures[Id] == actual;
			}
		}

		// Match against a concrete history_index
		bool match_operand(history_index expected, history_index actual)
		{
			return expected == actual;
		}
	};

	// Base pattern for instructions with specific operand requirements
	template <spu_itype::type IType, typename Ra, typename Rb, typename Rc>
	struct instruction_pattern
	{
		Ra ra;
		Rb rb;
		Rc rc;

		static constexpr spu_itype::type itype = IType;

		// Compute max capture ID across all operands
		static constexpr u32 max_id = std::max({
			max_inst_id<Ra>::value,
			max_inst_id<Rb>::value,
			max_inst_id<Rc>::value
		});

		constexpr instruction_pattern(Ra a, Rb b, Rc c) : ra(a), rb(b), rc(c) {}
	};

	// Wildcard that matches any operand (don't care)
	struct any_t {};
	inline constexpr any_t any{};

	template <>
	struct max_inst_id<any_t> { static constexpr u32 value = 0; };

	// Helper to match a single operand against a pattern element
	template <u32 MaxId, typename Pat>
	bool match_single(matcher<MaxId>& m, Pat pat, history_index actual)
	{
		if constexpr (std::is_same_v<Pat, any_t>)
			return true;
		else
			return m.match_operand(pat, actual);
	}

	// Base for ternary instructions where ra and rb are commutative (e.g., multiplication operands)
	template <spu_itype::type IType, typename Ra, typename Rb, typename Rc>
	struct commutative_ternary_pattern : instruction_pattern<IType, Ra, Rb, Rc>
	{
		using base = instruction_pattern<IType, Ra, Rb, Rc>;
		using base::base;

		template <u32 MaxId>
		bool try_match(const raw_inst& inst, match_result<MaxId>& result) const
		{
			if (inst.itype != IType)
				return false;

			// Try ra, rb, rc ordering
			auto result_copy = result;
			matcher<MaxId> m{result};
			if (try_match_operands(inst, m))
				return true;

			// Try rb, ra, rc ordering (commutative)
			result = result_copy;
			matcher<MaxId> m2{result};
			return try_match_operands_swapped(inst, m2);
		}

	private:
		template <u32 MaxId>
		bool try_match_operands(const raw_inst& inst, matcher<MaxId>& m) const
		{
			return match_single(m, this->ra, inst.ra) &&
			       match_single(m, this->rb, inst.rb) &&
			       match_single(m, this->rc, inst.rc);
		}

		template <u32 MaxId>
		bool try_match_operands_swapped(const raw_inst& inst, matcher<MaxId>& m) const
		{
			return match_single(m, this->ra, inst.rb) &&  // ra matches inst.rb
			       match_single(m, this->rb, inst.ra) &&  // rb matches inst.ra
			       match_single(m, this->rc, inst.rc);
		}
	};

	// FMA pattern: (ra * rb) + rc
	template <typename Ra, typename Rb, typename Rc>
	struct fma_pattern : commutative_ternary_pattern<spu_itype::FMA, Ra, Rb, Rc>
	{
		using base = commutative_ternary_pattern<spu_itype::FMA, Ra, Rb, Rc>;
		using base::base;
	};

	// FNMS pattern: rc - (ra * rb)
	template <typename Ra, typename Rb, typename Rc>
	struct fnms_pattern : commutative_ternary_pattern<spu_itype::FNMS, Ra, Rb, Rc>
	{
		using base = commutative_ternary_pattern<spu_itype::FNMS, Ra, Rb, Rc>;
		using base::base;
	};

	// FI pattern: float interpolate (ra=value, rb=estimate)
	template <typename Ra, typename Rb>
	struct fi_pattern : instruction_pattern<spu_itype::FI, Ra, Rb, any_t>
	{
		using base = instruction_pattern<spu_itype::FI, Ra, Rb, any_t>;

		constexpr fi_pattern(Ra a, Rb b) : base(a, b, any) {}

		template <u32 MaxId>
		bool try_match(const raw_inst& inst, match_result<MaxId>& result) const
		{
			if (inst.itype != spu_itype::FI)
				return false;

			matcher<MaxId> m{result};
			return match_single(m, this->ra, inst.ra) &&
			       match_single(m, this->rb, inst.rb);
		}
	};

	// FREST pattern: reciprocal estimate (ra only)
	template <typename Ra>
	struct frest_pattern : instruction_pattern<spu_itype::FREST, Ra, any_t, any_t>
	{
		using base = instruction_pattern<spu_itype::FREST, Ra, any_t, any_t>;

		constexpr frest_pattern(Ra a) : base(a, any, any) {}

		template <u32 MaxId>
		bool try_match(const raw_inst& inst, match_result<MaxId>& result) const
		{
			if (inst.itype != spu_itype::FREST)
				return false;

			matcher<MaxId> m{result};
			return match_single(m, this->ra, inst.ra);
		}
	};

	// FRSQEST pattern: reciprocal square root estimate (ra only)
	template <typename Ra>
	struct frsqest_pattern : instruction_pattern<spu_itype::FRSQEST, Ra, any_t, any_t>
	{
		using base = instruction_pattern<spu_itype::FRSQEST, Ra, any_t, any_t>;

		constexpr frsqest_pattern(Ra a) : base(a, any, any) {}

		template <u32 MaxId>
		bool try_match(const raw_inst& inst, match_result<MaxId>& result) const
		{
			if (inst.itype != spu_itype::FRSQEST)
				return false;

			matcher<MaxId> m{result};
			return match_single(m, this->ra, inst.ra);
		}
	};

	// Base for binary instructions where ra and rb are commutative
	template <spu_itype::type IType, typename Ra, typename Rb>
	struct commutative_binary_pattern : instruction_pattern<IType, Ra, Rb, any_t>
	{
		using base = instruction_pattern<IType, Ra, Rb, any_t>;

		constexpr commutative_binary_pattern(Ra a, Rb b) : base(a, b, any) {}

		template <u32 MaxId>
		bool try_match(const raw_inst& inst, match_result<MaxId>& result) const
		{
			if (inst.itype != IType)
				return false;

			// Try ra, rb ordering
			auto result_copy = result;
			matcher<MaxId> m{result};
			if (match_single(m, this->ra, inst.ra) && match_single(m, this->rb, inst.rb))
				return true;

			// Try rb, ra ordering (commutative)
			result = result_copy;
			matcher<MaxId> m2{result};
			return match_single(m2, this->ra, inst.rb) && match_single(m2, this->rb, inst.ra);
		}
	};

	// FM pattern: ra * rb (commutative)
	template <typename Ra, typename Rb>
	struct fm_pattern : commutative_binary_pattern<spu_itype::FM, Ra, Rb>
	{
		using base = commutative_binary_pattern<spu_itype::FM, Ra, Rb>;
		using base::base;
	};

	// ILHU pattern: immediate load halfword upper
	template <typename Ra = any_t>
	struct ilhu_pattern : instruction_pattern<spu_itype::ILHU, Ra, any_t, any_t>
	{
		using base = instruction_pattern<spu_itype::ILHU, Ra, any_t, any_t>;

		constexpr ilhu_pattern(Ra a = any) : base(a, any, any) {}

		template <u32 MaxId>
		bool try_match(const raw_inst& inst, match_result<MaxId>& result) const
		{
			if (inst.itype != spu_itype::ILHU)
				return false;

			if constexpr (!std::is_same_v<Ra, any_t>)
			{
				matcher<MaxId> m{result};
				return match_single(m, this->ra, inst.ra);
			}
			return true;
		}
	};

	// IOHL pattern: immediate or halfword lower
	template <typename Ra>
	struct iohl_pattern : instruction_pattern<spu_itype::IOHL, Ra, any_t, any_t>
	{
		using base = instruction_pattern<spu_itype::IOHL, Ra, any_t, any_t>;

		constexpr iohl_pattern(Ra a) : base(a, any, any) {}

		template <u32 MaxId>
		bool try_match(const raw_inst& inst, match_result<MaxId>& result) const
		{
			if (inst.itype != spu_itype::IOHL)
				return false;

			matcher<MaxId> m{result};
			return match_single(m, this->ra, inst.ra);
		}
	};

	// Factory functions for patterns
	template <typename Ra, typename Rb, typename Rc>
	constexpr auto FMA(Ra a, Rb b, Rc c) { return fma_pattern<Ra, Rb, Rc>(a, b, c); }

	template <typename Ra, typename Rb, typename Rc>
	constexpr auto FNMS(Ra a, Rb b, Rc c) { return fnms_pattern<Ra, Rb, Rc>(a, b, c); }

	template <typename Ra, typename Rb>
	constexpr auto FI(Ra a, Rb b) { return fi_pattern<Ra, Rb>(a, b); }

	template <typename Ra>
	constexpr auto FREST(Ra a) { return frest_pattern<Ra>(a); }

	template <typename Ra>
	constexpr auto FRSQEST(Ra a) { return frsqest_pattern<Ra>(a); }

	template <typename Ra, typename Rb>
	constexpr auto FM(Ra a, Rb b) { return fm_pattern<Ra, Rb>(a, b); }

	template <typename Ra = any_t>
	constexpr auto ILHU(Ra a = any) { return ilhu_pattern<Ra>(a); }

	template <typename Ra>
	constexpr auto IOHL(Ra a) { return iohl_pattern<Ra>(a); }

	// Compute max ID across a pattern
	template <typename Pattern>
	constexpr u32 pattern_max_id = Pattern::max_id;

	// Main match function
	template <typename Pattern>
	auto match(const raw_inst& inst, Pattern pattern)
		-> std::optional<match_result<pattern_max_id<Pattern>>>
	{
		constexpr u32 MaxId = pattern_max_id<Pattern>;
		match_result<MaxId> result{};

		if (pattern.try_match(inst, result))
			return result;

		return std::nullopt;
	}

	// Match with access to history for chained pattern matching
	template <typename Pattern>
	auto match(const std::vector<raw_inst>& history, history_index h, Pattern pattern)
		-> std::optional<match_result<pattern_max_id<Pattern>>>
	{
		if (h < 128)
			return std::nullopt;

		const u32 idx = h - 128;
		if (idx >= history.size())
			return std::nullopt;

		return match(history[idx], pattern);
	}

} // namespace spu_optimizer::pattern
