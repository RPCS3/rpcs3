#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Utilities/Thread.h"

#include "Emu/Cell/lv2/sys_spu.h"

#include <thread>

// SONIC THE HEDGEDOG: a fix for a race condition between SPUs and PPUs causing missing graphics (SNR is overriden when non-empty)
void WaitForSPUsToEmptySNRs(ppu_thread& ppu, u32 spu_id, u32 snr_mask)
{
	ppu.state += cpu_flag::wait;

	auto [spu, group] = lv2_spu_group::get_thread(spu_id);

	if ((!spu && spu_id != umax) || snr_mask % 4 == 0)
	{
		return;
	}

	// Wait until specified SNRs are reported empty at least once
	for (bool has_busy = true; has_busy && !ppu.is_stopped(); std::this_thread::yield())
	{
		has_busy = false;

		auto for_one = [&](u32, spu_thread& spu)
		{
			if ((snr_mask & 1) && spu.ch_snr1.get_count())
			{
				has_busy = true;
				return;
			}

			if ((snr_mask & 2) && spu.ch_snr2.get_count())
			{
				has_busy = true;
			}
		};

		if (spu)
		{
			// Wait for a single SPU
			for_one(spu->id, *spu);
		}
		else
		{
			// Wait for all SPUs
			idm::select<named_thread<spu_thread>>(for_one);
		}
	}
}

DECLARE(ppu_module_manager::hle_patches)("RPCS3_HLE_LIBRARY", []()
{
	REG_FUNC(RPCS3_HLE_LIBRARY, WaitForSPUsToEmptySNRs);
});
