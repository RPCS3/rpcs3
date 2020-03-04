#include "atomic2.hpp"
#include "Utilities/JIT.h"
#include "Utilities/asm.h"
#include "Utilities/sysinfo.h"

//
static const bool s_use_rtm = utils::has_rtm();

template <unsigned Count>
static const auto commit_tx = build_function_asm<s32(*)(const stx::multi_cas_item*)>([](asmjit::X86Assembler& c, auto& args)
{
	static_assert(Count <= 8);
	using namespace asmjit;

	// Fill registers with item data
	c.lea(x86::rax, x86::qword_ptr(args[0], 120));

	if constexpr (Count >= 1)
	{
		c.mov(x86::rcx, x86::qword_ptr(x86::rax, -120));
		c.mov(x86::rdx, x86::qword_ptr(x86::rax, -112));
		c.mov(x86::r8, x86::qword_ptr(x86::rax, -104));
	}
	if constexpr (Count >= 2)
	{
		c.mov(x86::r9, x86::qword_ptr(x86::rax, -96));
		c.mov(x86::r10, x86::qword_ptr(x86::rax, -88));
		c.mov(x86::r11, x86::qword_ptr(x86::rax, -80));
	}
	if constexpr (Count >= 3)
	{
		if (utils::has_avx())
		{
			c.vzeroupper();
		}

#ifdef _WIN32
		c.push(x86::rsi);
#endif
		c.mov(x86::rsi, x86::qword_ptr(x86::rax, -72));
		c.movups(x86::xmm0, x86::oword_ptr(x86::rax, -64));
	}
	if constexpr (Count >= 4)
	{
#ifdef _WIN32
		c.push(x86::rdi);
#endif
		c.mov(x86::rdi, x86::qword_ptr(x86::rax, -48));
		c.movups(x86::xmm1, x86::oword_ptr(x86::rax, -40));
	}
	if constexpr (Count >= 5)
	{
		c.push(x86::rbx);
		c.mov(x86::rbx, x86::qword_ptr(x86::rax, -24));
		c.movups(x86::xmm2, x86::oword_ptr(x86::rax, -16));
	}
	if constexpr (Count >= 6)
	{
		c.push(x86::rbp);
		c.mov(x86::rbp, x86::qword_ptr(x86::rax));
		c.movups(x86::xmm3, x86::oword_ptr(x86::rax, 8));
	}
	if constexpr (Count >= 7)
	{
		c.push(x86::r12);
		c.mov(x86::r12, x86::qword_ptr(x86::rax, 24));
		c.movups(x86::xmm4, x86::oword_ptr(x86::rax, 32));
	}
	if constexpr (Count >= 8)
	{
		c.push(x86::r13);
		c.mov(x86::r13, x86::qword_ptr(x86::rax, 48));
		c.movups(x86::xmm5, x86::oword_ptr(x86::rax, 56));
	}

	// Begin transaction
	Label begin = c.newLabel();
	Label fall = c.newLabel();
	Label stop = c.newLabel();
	Label wait = c.newLabel();
	Label ret = c.newLabel();
	c.bind(begin);
	c.xbegin(fall);

	// Compare phase
	if constexpr (Count >= 1)
	{
		c.cmp(x86::qword_ptr(x86::rcx), x86::rdx);
		c.jne(stop);
	}
	if constexpr (Count >= 2)
	{
		c.cmp(x86::qword_ptr(x86::r9), x86::r10);
		c.jne(stop);
	}
	if constexpr (Count >= 3)
	{
		c.movq(x86::rax, x86::xmm0);
		c.cmp(x86::qword_ptr(x86::rsi), x86::rax);
		c.jne(stop);
	}
	if constexpr (Count >= 4)
	{
		c.movq(x86::rax, x86::xmm1);
		c.cmp(x86::qword_ptr(x86::rdi), x86::rax);
		c.jne(stop);
	}
	if constexpr (Count >= 5)
	{
		c.movq(x86::rax, x86::xmm2);
		c.cmp(x86::qword_ptr(x86::rbx), x86::rax);
		c.jne(stop);
	}
	if constexpr (Count >= 6)
	{
		c.movq(x86::rax, x86::xmm3);
		c.cmp(x86::qword_ptr(x86::rbp), x86::rax);
		c.jne(stop);
	}
	if constexpr (Count >= 7)
	{
		c.movq(x86::rax, x86::xmm4);
		c.cmp(x86::qword_ptr(x86::r12), x86::rax);
		c.jne(stop);
	}
	if constexpr (Count >= 8)
	{
		c.movq(x86::rax, x86::xmm5);
		c.cmp(x86::qword_ptr(x86::r13), x86::rax);
		c.jne(stop);
	}

	// Check for transactions in progress
	if constexpr (Count >= 1)
	{
		c.cmp(x86::qword_ptr(x86::rcx, 8), 0);
		c.jne(wait);
	}
	if constexpr (Count >= 2)
	{
		c.cmp(x86::qword_ptr(x86::r9, 8), 0);
		c.jne(wait);
	}
	if constexpr (Count >= 3)
	{
		c.cmp(x86::qword_ptr(x86::rsi, 8), 0);
		c.jne(wait);
	}
	if constexpr (Count >= 4)
	{
		c.cmp(x86::qword_ptr(x86::rdi, 8), 0);
		c.jne(wait);
	}
	if constexpr (Count >= 5)
	{
		c.cmp(x86::qword_ptr(x86::rbx, 8), 0);
		c.jne(wait);
	}
	if constexpr (Count >= 6)
	{
		c.cmp(x86::qword_ptr(x86::rbp, 8), 0);
		c.jne(wait);
	}
	if constexpr (Count >= 7)
	{
		c.cmp(x86::qword_ptr(x86::r12, 8), 0);
		c.jne(wait);
	}
	if constexpr (Count >= 8)
	{
		c.cmp(x86::qword_ptr(x86::r13, 8), 0);
		c.jne(wait);
	}

	// Write phase
	if constexpr (Count >= 1)
		c.mov(x86::qword_ptr(x86::rcx), x86::r8);
	if constexpr (Count >= 2)
		c.mov(x86::qword_ptr(x86::r9), x86::r11);
	if constexpr (Count >= 3)
		c.movhps(x86::qword_ptr(x86::rsi), x86::xmm0);
	if constexpr (Count >= 4)
		c.movhps(x86::qword_ptr(x86::rdi), x86::xmm1);
	if constexpr (Count >= 5)
		c.movhps(x86::qword_ptr(x86::rbx), x86::xmm2);
	if constexpr (Count >= 6)
		c.movhps(x86::qword_ptr(x86::rbp), x86::xmm3);
	if constexpr (Count >= 7)
		c.movhps(x86::qword_ptr(x86::r12), x86::xmm4);
	if constexpr (Count >= 8)
		c.movhps(x86::qword_ptr(x86::r13), x86::xmm5);

	// End transaction (success)
	c.xend();
	c.mov(x86::eax, 1);
	c.bind(ret);
	if constexpr (Count >= 8)
		c.pop(x86::r13);
	if constexpr (Count >= 7)
		c.pop(x86::r12);
	if constexpr (Count >= 6)
		c.pop(x86::rbp);
	if constexpr (Count >= 5)
		c.pop(x86::rbx);
#ifdef _WIN32
	if constexpr (Count >= 4)
		c.pop(x86::rdi);
	if constexpr (Count >= 3)
		c.pop(x86::rsi);
#endif
	c.ret();

	// Transaction abort
	c.bind(stop);
	build_transaction_abort(c, 0xff);

	// Abort when there is still a chance of success
	c.bind(wait);
	build_transaction_abort(c, 0x00);

	// Transaction fallback: return zero
	c.bind(fall);
	c.test(x86::eax, _XABORT_RETRY);
	c.jnz(begin);
	c.sar(x86::eax, 24);
	c.jmp(ret);
});

// 4095 records max
static constexpr u64 s_rec_gcount = 4096 / 64;

// Global record pool
static stx::multi_cas_record s_records[s_rec_gcount * 64]{};

// Allocation bits (without first element)
static atomic_t<u64> s_rec_bits[s_rec_gcount]{1};

static constexpr u64 s_state_mask = 3;
static constexpr u64 s_state_undef = 0;
static constexpr u64 s_state_failure = 1;
static constexpr u64 s_state_success = 2;
static constexpr u64 s_ref_mask = ~s_state_mask;
static constexpr u64 s_ref_one = s_state_mask + 1;

static u64 rec_alloc()
{
	const u32 start = static_cast<u32>(__rdtsc());

	for (u32 i = 0;; i++)
	{
		const u32 group = (i + start) % s_rec_gcount;

		const auto [bits, ok] = s_rec_bits[group].fetch_op([](u64& bits)
		{
			if (~bits)
			{
				// Set lowest clear bit
				bits |= bits + 1;
				return true;
			}

			return false;
		});

		if (ok)
		{
			// Find lowest clear bit
			return group * 64 + utils::cnttz64(~bits, false);
		}
	}

	// TODO: unreachable
	std::abort();
	return 0;
}

static bool cmpxchg16(s64(&dest)[2], s64(&cmp_res)[2], s64 exch_high, s64 exch_low)
{
#ifdef _MSC_VER
	return !!_InterlockedCompareExchange128(dest, exch_high, exch_low, cmp_res);
#else
	s64 exch[2]{exch_low, exch_high};
	return __atomic_compare_exchange(&dest, &cmp_res, &exch, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}

bool stx::multi_cas_record::commit() const noexcept
{
	// Transaction cancelled
	if (m_count == 0)
	{
		return true;
	}

	static auto rec_unref = [](u64 id)
	{
		if (id && id < s_rec_gcount * 64)
		{
			auto [_, ok] = s_records[id].m_state.fetch_op([](u64& state)
			{
				if (state < s_ref_one)
				{
					return 0;
				}

				state -= s_ref_one;

				if (state < s_ref_one)
				{
					state = 0;
					return 2;
				}

				return 1;
			});

			if (ok > 1)
			{
				s_rec_bits[id / 64] &= ~(u64{1} << (id % 64));
			}
		}
	};

	// Helper function to complete successful transaction
	static auto rec_complete = [](u64 id)
	{
		for (u32 i = 0; i < s_records[id].m_count; i++)
		{
			auto& item = s_records[id].m_list[i];

			atomic2 cmp;
			cmp.m_data[0] = item.m_old;
			cmp.m_data[1] = id;

			if (item.m_addr->load() == item.m_old && atomic_storage<s64>::load(item.m_addr->m_data[1]) == static_cast<s64>(id))
			{
				if (cmpxchg16(item.m_addr->m_data, cmp.m_data, 0, item.m_new))
				{
				}
			}
		}
	};

	// Helper function to deal with existing transaction
	static auto rec_try_abort = [](u64 id) -> u64
	{
		if (id >= s_rec_gcount * 64)
		{
			std::abort();
		}

		auto [_old, ok] = s_records[id].m_state.fetch_op([](u64& state)
		{
			if (state < s_ref_one)
			{
				// Don't reference if no references
				return false;
			}

			if ((state & s_state_mask) == s_state_undef)
			{
				// Break transaction if possible
				state |= s_state_failure;
			}

			state += s_ref_one;
			return true;
		});

		if (!ok)
		{
			return 0;
		}

		if ((_old & s_state_mask) != s_state_success)
		{
			// Allow to overwrite failing transaction
			return id;
		}

		// Help to complete
		rec_complete(id);
		rec_unref(id);
		return 0;
	};

	// Single CAS path
	if (m_count == 1)
	{
		atomic2 cmp;

		while (auto ptr = m_list[0].m_addr)
		{
			if (ptr->load() != m_list[0].m_old)
			{
				return false;
			}

			cmp.m_data[0] = m_list[0].m_old;
			cmp.m_data[1] = atomic_storage<s64>::load(ptr->m_data[1]);

			if (!cmp.m_data[1] && cmpxchg16(ptr->m_data, cmp.m_data, 0, m_list[0].m_new))
			{
				return true;
			}
			else if (cmp.m_data[0] != static_cast<s64>(m_list[0].m_old))
			{
				return false;
			}
			else if (cmp.m_data[1])
			{
				if (u64 _id = rec_try_abort(cmp.m_data[1]))
				{
					if (cmpxchg16(ptr->m_data, cmp.m_data, 0, m_list[0].m_new))
					{
						rec_unref(_id);
						return true;
					}

					rec_unref(_id);
				}
			}
		}

		// Unreachable
		std::abort();
	}

	// Try TSX if available
	if (s_use_rtm)
	{
		switch (m_count)
		{
		case 2: if (s32 r = commit_tx<2>(m_list)) return r > 0; break;
		case 3: if (s32 r = commit_tx<3>(m_list)) return r > 0; break;
		case 4: if (s32 r = commit_tx<4>(m_list)) return r > 0; break;
		case 5: if (s32 r = commit_tx<5>(m_list)) return r > 0; break;
		case 6: if (s32 r = commit_tx<6>(m_list)) return r > 0; break;
		case 7: if (s32 r = commit_tx<7>(m_list)) return r > 0; break;
		case 8: if (s32 r = commit_tx<8>(m_list)) return r > 0; break;
		}
	}

	// Allocate global record and copy data
	const u64 id = rec_alloc();

	for (u32 i = 0; i < (m_count + 1) / 2; i++)
	{
		std::memcpy(s_records[id].m_list + i * 2, m_list + i * 2, sizeof(multi_cas_item) * 2);
	}

	s_records[id].m_count = m_count;
	s_records[id].m_state = s_ref_one;

	// Try to install CAS items
	for (u32 i = 0; i < m_count && (s_records[id].m_state & s_state_mask) == s_state_undef; i++)
	{
		atomic2 cmp;

		while (auto ptr = m_list[i].m_addr)
		{
			if (ptr->load() != m_list[i].m_old)
			{
				s_records[id].m_state |= s_state_failure;
				break;
			}

			cmp.m_data[0] = m_list[i].m_old;
			cmp.m_data[1] = atomic_storage<s64>::load(ptr->m_data[1]);

			if (!cmp.m_data[1] && cmpxchg16(ptr->m_data, cmp.m_data, id, m_list[i].m_old))
			{
				break;
			}
			else if (cmp.m_data[0] != static_cast<s64>(m_list[i].m_old))
			{
				s_records[id].m_state |= s_state_failure;
				break;
			}
			else if (cmp.m_data[1])
			{
				if (u64 _id = rec_try_abort(cmp.m_data[1]))
				{
					if (cmpxchg16(ptr->m_data, cmp.m_data, id, m_list[i].m_old))
					{
						rec_unref(_id);
						break;
					}

					rec_unref(_id);
				}
			}
		}
	}

	// Try to acknowledge transaction success
	auto [_, ok] = s_records[id].m_state.fetch_op([](u64& state)
	{
		if (state & s_state_failure)
		{
			return false;
		}

		state |= s_state_success;
		return true;
	});

	// Complete transaction on success, or cleanup on failure
	for (u32 i = 0; i < m_count; i++)
	{
		auto& item = m_list[i];

		atomic2 cmp;
		cmp.m_data[0] = item.m_old;
		cmp.m_data[1] = id;

		if (item.m_addr->load() == item.m_old && atomic_storage<s64>::load(item.m_addr->m_data[1]) == static_cast<s64>(id))
		{
			// Restore old or set new
			if (cmpxchg16(item.m_addr->m_data, cmp.m_data, 0, ok ? item.m_new : item.m_old))
			{
			}
		}
	}

	rec_unref(id);
	return ok;
}
