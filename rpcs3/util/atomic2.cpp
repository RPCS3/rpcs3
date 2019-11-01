#include "atomic2.hpp"
#include "Utilities/JIT.h"
#include "Utilities/asm.h"
#include "Utilities/sysinfo.h"

//
static const bool s_use_rtm = utils::has_rtm();

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
	const u32 start = __rdtsc();

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

	// Try TSX if available
	if (s_use_rtm)
	{
		// TODO
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

			if (item.m_addr->load() == item.m_old && atomic_storage<s64>::load(item.m_addr->m_data[1]) == id)
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
		cmp.m_data[0] = m_list[0].m_old;
		cmp.m_data[1] = 0;

		while (auto ptr = m_list[0].m_addr)
		{
			if (ptr->load() != m_list[0].m_old)
			{
				return false;
			}

			cmp.m_data[1] = atomic_storage<s64>::load(ptr->m_data[1]);

			if (!cmp.m_data[1] && cmpxchg16(ptr->m_data, cmp.m_data, 0, m_list[0].m_new))
			{
				return true;
			}
			else if (cmp.m_data[0] != m_list[0].m_old)
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
				else
				{
					return false;
				}
			}
		}

		// Unreachable
		std::abort();
	}

	// Allocate global record and copy data
	const u64 id = rec_alloc();

	for (u32 i = 0; i < (m_count / 2 + 1); i++)
	{
		std::memcpy(s_records[id].m_list + i * 2, m_list + i * 2, sizeof(multi_cas_item) * 2);
	}

	s_records[id].m_count = m_count;
	s_records[id].m_state = s_ref_one;

	// Try to install CAS items
	for (u32 i = 0; i < m_count && (s_records[id].m_state & s_state_mask) == s_state_undef; i++)
	{
		atomic2 cmp;
		cmp.m_data[0] = m_list[i].m_old;
		cmp.m_data[1] = 0;

		while (auto ptr = m_list[i].m_addr)
		{
			if (ptr->load() != m_list[i].m_old)
			{
				s_records[id].m_state |= s_state_failure;
				break;
			}

			cmp.m_data[1] = atomic_storage<s64>::load(ptr->m_data[1]);

			if (!cmp.m_data[1] && cmpxchg16(ptr->m_data, cmp.m_data, id, m_list[i].m_old))
			{
				break;
			}
			else if (cmp.m_data[0] != m_list[i].m_old)
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
				else
				{
					s_records[id].m_state |= s_state_failure;
					break;
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

		if (item.m_addr->load() == item.m_old && atomic_storage<s64>::load(item.m_addr->m_data[1]) == id)
		{
			// Restore old or set new
			cmpxchg16(item.m_addr->m_data, cmp.m_data, 0, ok ? item.m_new : item.m_old);
		}
	}

	rec_unref(id);
	return ok;
}
