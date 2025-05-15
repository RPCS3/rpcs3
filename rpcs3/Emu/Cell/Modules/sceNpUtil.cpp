#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "sceNp.h"
#include "sceNpUtil.h"

LOG_CHANNEL(sceNpUtil);

struct bandwidth_test
{
	atomic_t<u32> status = SCE_NP_UTIL_BANDWIDTH_TEST_STATUS_NONE;
	atomic_t<bool> abort = false;
	atomic_t<bool> shutdown = false;
	atomic_t<bool> finished = false;
	SceNpUtilBandwidthTestResult test_result{};

	static constexpr auto thread_name = "HLE SceNpBandwidthTest"sv;

	void operator()()
	{
		status = SCE_NP_UTIL_BANDWIDTH_TEST_STATUS_RUNNING;
		u32 fake_sleep_count = 0;

		while (thread_ctrl::state() != thread_state::aborting)
		{
			if (abort || shutdown)
			{
				break;
			}

			// TODO: run proper bandwidth test, probably with cellHttp

			thread_ctrl::wait_for(1000); // wait 1ms

			if (fake_sleep_count++ > 100) // end fake bandwidth test after 100 ms
			{
				break;
			}
		}

		test_result.upload_bps = 100'000'000.0;
		test_result.download_bps = 100'000'000.0;
		test_result.result = CELL_OK;
		status = SCE_NP_UTIL_BANDWIDTH_TEST_STATUS_FINISHED;
		finished = true;
	}
};

struct sce_np_util_manager
{
	shared_mutex mtx{};
	std::unique_ptr<named_thread<bandwidth_test>> bandwidth_test_thread;

	~sce_np_util_manager()
	{
		join_thread();
	}

	void join_thread()
	{
		bandwidth_test_thread.reset();
	}
};

error_code sceNpUtilBandwidthTestInitStart([[maybe_unused]] ppu_thread& ppu, u32 prio, u32 stack)
{
	sceNpUtil.todo("sceNpUtilBandwidthTestInitStart(prio=%d, stack=%d)", prio, stack);

	auto& util_manager = g_fxo->get<sce_np_util_manager>();
	std::lock_guard lock(util_manager.mtx);

	if (util_manager.bandwidth_test_thread)
	{
		return SCE_NP_ERROR_ALREADY_INITIALIZED;
	}

	util_manager.bandwidth_test_thread = std::make_unique<named_thread<bandwidth_test>>();

	return CELL_OK;
}

error_code sceNpUtilBandwidthTestGetStatus()
{
	sceNpUtil.notice("sceNpUtilBandwidthTestGetStatus()");

	auto& util_manager = g_fxo->get<sce_np_util_manager>();
	std::lock_guard lock(util_manager.mtx);

	if (!util_manager.bandwidth_test_thread)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return not_an_error(util_manager.bandwidth_test_thread->status);
}

error_code sceNpUtilBandwidthTestShutdown([[maybe_unused]] ppu_thread& ppu, vm::ptr<SceNpUtilBandwidthTestResult> result)
{
	sceNpUtil.warning("sceNpUtilBandwidthTestShutdown(result=*0x%x)", result);

	auto& util_manager = g_fxo->get<sce_np_util_manager>();
	std::lock_guard lock(util_manager.mtx);

	if (!util_manager.bandwidth_test_thread)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	util_manager.bandwidth_test_thread->shutdown = true;

	while (!util_manager.bandwidth_test_thread->finished)
	{
		thread_ctrl::wait_for(1000);
	}

	if (result) // TODO: what happens when this is null ?
	{
		*result = util_manager.bandwidth_test_thread->test_result;
	}

	util_manager.join_thread();

	return CELL_OK;
}

error_code sceNpUtilBandwidthTestAbort()
{
	sceNpUtil.todo("sceNpUtilBandwidthTestAbort()");

	auto& util_manager = g_fxo->get<sce_np_util_manager>();
	std::lock_guard lock(util_manager.mtx);

	if (!util_manager.bandwidth_test_thread)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	// TODO: cellHttpTransactionAbortConnection();

	util_manager.bandwidth_test_thread->abort = true;

	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNpUtil)("sceNpUtil", []()
{
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestInitStart);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestShutdown);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestGetStatus);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestAbort);
});
