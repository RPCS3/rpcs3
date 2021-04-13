#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "sceNp.h"
#include "sceNpTus.h"

LOG_CHANNEL(sceNpTus);

// Helper functions

static bool validateSlotIds(vm::cptr<SceNpTusSlotId> slotIdArray)
{
	if (!slotIdArray)
	{
		return false;
	}

	// TODO: how to properly iterate?
	//for (usz i = 0; i < slotIdArray.size(); ++i)
	//{
	//	if (slotIdArray[i] < 0)
	//	{
	//		return false;
	//	}
	//}

	return true;
}

s32 sce_np_tus_manager::add_title_context()
{
	if (title_contexts.size() < SCE_NP_TUS_MAX_CTX_NUM)
	{
		sce_np_tus_title_context new_title_context;
		const auto pair = std::make_pair(next_title_context_id, new_title_context);

		if (title_contexts.emplace(pair).second)
		{
			return next_title_context_id++;
		}
	}

	return 0;
}

bool sce_np_tus_manager::check_title_context_id(s32 titleCtxId)
{
	return title_contexts.find(titleCtxId) != title_contexts.end();
}

bool sce_np_tus_manager::remove_title_context_id(s32 titleCtxId)
{
	return title_contexts.erase(titleCtxId) > 0;
}

sce_np_tus_title_context* sce_np_tus_manager::get_title_context(s32 titleCtxId)
{
	if (title_contexts.find(titleCtxId) != title_contexts.end())
	{
		return &title_contexts.at(titleCtxId);
	}

	return nullptr;
}

s32 sce_np_tus_manager::add_transaction_context(s32 titleCtxId)
{
	usz transaction_count = 0;

	for (const auto& title_context : title_contexts)
	{
		const auto& transactions = title_context.second.transaction_contexts;
		transaction_count += transactions.size();
	}

	if (transaction_count < SCE_NP_TUS_MAX_CTX_NUM)
	{
		if (title_contexts.find(titleCtxId) != title_contexts.end())
		{
			sce_np_tus_transaction_context new_transaction;
			new_transaction.id = next_transaction_context_id;

			if (title_contexts.at(titleCtxId).transaction_contexts.emplace(next_transaction_context_id, new_transaction).second)
			{
				return next_transaction_context_id++;
			}
		}
	}

	return 0;
}

bool sce_np_tus_manager::check_transaction_context_id(s32 transId)
{
	return std::any_of(title_contexts.cbegin(), title_contexts.cend(), [&transId](const auto& c)
	{
		return c.second.transaction_contexts.contains(transId);
	});
}

bool sce_np_tus_manager::remove_transaction_context_id(s32 transId)
{
	for (auto& title_context : title_contexts)
	{
		auto& transactions = title_context.second.transaction_contexts;

		if (transactions.find(transId) != transactions.end())
		{
			return transactions.erase(transId) > 0;
		}
	}

	return false;
}

sce_np_tus_transaction_context* sce_np_tus_manager::get_transaction_context(s32 transId)
{
	for (auto& title_context : title_contexts)
	{
		auto& transactions = title_context.second.transaction_contexts;

		if (transactions.find(transId) != transactions.end())
		{
			return &transactions.at(transId);
		}
	}

	return nullptr;
}

void sce_np_tus_manager::terminate()
{
	title_contexts.clear();
	is_initialized = false;
}

// Module Functions

error_code sceNpTusInit(s32 prio)
{
	sceNpTus.warning("sceNpTusInit(prio=%d)", prio);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;
	}

	tus_manager.is_initialized = true;

	return CELL_OK;
}

error_code sceNpTusTerm()
{
	sceNpTus.warning("sceNpTusTerm()");

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	tus_manager.terminate();

	return CELL_OK;
}

error_code sceNpTusCreateTitleCtx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase, vm::cptr<SceNpId> selfNpId)
{
	sceNpTus.todo("sceNpTusCreateTitleCtx(communicationId=*0x%x, passphrase=*0x%x, selfNpId=*0x%x)", communicationId, passphrase, selfNpId);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!communicationId || !passphrase || !selfNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	const auto id = tus_manager.add_title_context();

	if (id <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
	}

	return not_an_error(id);
}

error_code sceNpTusDestroyTitleCtx(s32 titleCtxId)
{
	sceNpTus.todo("sceNpTusDestroyTitleCtx(titleCtxId=%d)", titleCtxId);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_title_context_id(titleCtxId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code sceNpTusCreateTransactionCtx(s32 titleCtxId)
{
	sceNpTus.todo("sceNpTusCreateTransactionCtx(titleCtxId=%d)", titleCtxId);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_title_context_id(titleCtxId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	const auto id = tus_manager.add_transaction_context(titleCtxId);

	if (id <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
	}

	return not_an_error(id);
}

error_code sceNpTusDestroyTransactionCtx(s32 transId)
{
	sceNpTus.todo("sceNpTusDestroyTransactionCtx(transId=%d)", transId);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.remove_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code sceNpTusSetTimeout(s32 ctxId, u32 timeout)
{
	sceNpTus.todo("sceNpTusSetTimeout(ctxId=%d, timeout=%d)", ctxId, timeout);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto title_context       = tus_manager.get_title_context(ctxId);
	auto transaction_context = title_context ? tus_manager.get_transaction_context(ctxId) : nullptr;

	if (!title_context && !transaction_context)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (timeout < 10000000) // 10 seconds
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (title_context)
	{
		for (auto& [key, val] : title_context->transaction_contexts)
		{
			val.timeout = timeout;
		}
	}
	else if (transaction_context)
	{
		transaction_context->timeout = timeout;
	}

	return CELL_OK;
}

error_code sceNpTusAbortTransaction(s32 transId)
{
	sceNpTus.todo("sceNpTusAbortTransaction(transId=%d)", transId);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto transaction_context = tus_manager.get_transaction_context(transId);

	if (!transaction_context)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	transaction_context->abort = true;

	return CELL_OK;
}

error_code sceNpTusWaitAsync(s32 transId, vm::ptr<s32> result)
{
	sceNpTus.todo("sceNpTusWaitAsync(transId=%d, result=*0x%x)", transId, result);

	*result = 0;

	const bool processing_completed = true;
	return not_an_error(processing_completed ? 0 : 1);
}

error_code sceNpTusPollAsync(s32 transId, vm::ptr<s32> result)
{
	sceNpTus.todo("sceNpTusPollAsync(transId=%d, result=*0x%x)", transId, result);

	*result = 0;

	const bool processing_completed = true;
	return not_an_error(processing_completed ? 0 : 1);
}

error_code sceNpTusSetMultiSlotVariable(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusSetMultiSlotVariable(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, variableArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !slotIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusSetMultiSlotVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusSetMultiSlotVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, variableArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !slotIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusSetMultiSlotVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusSetMultiSlotVariableAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, variableArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !slotIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusSetMultiSlotVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusSetMultiSlotVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, variableArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !slotIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusGetMultiSlotVariable(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiSlotVariable(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !slotIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	const s32 variables_read = 0;
	return not_an_error(variables_read);
}

error_code sceNpTusGetMultiSlotVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiSlotVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	const s32 variables_read = 0;
	return not_an_error(variables_read);
}

error_code sceNpTusGetMultiSlotVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiSlotVariableAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusGetMultiSlotVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiSlotVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusGetMultiUserVariable(s32 transId, vm::cptr<SceNpId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiUserVariable(transId=%d, targetNpIdArray=*0x%x, slotId=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpIdArray, slotId, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_USER_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	const s32 variables_read = 0;
	return not_an_error(variables_read);
}

error_code sceNpTusGetMultiUserVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiUserVariableVUser(transId=%d, targetVirtualUserIdArray=*0x%x, slotId=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserIdArray, slotId, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_USER_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	const s32 variables_read = 0;
	return not_an_error(variables_read);
}

error_code sceNpTusGetMultiUserVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiUserVariableAsync(transId=%d, targetNpIdArray=*0x%x, slotId=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpIdArray, slotId, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_USER_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	return CELL_OK;
}

error_code sceNpTusGetMultiUserVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiUserVariableVUserAsync(transId=%d, targetVirtualUserIdArray=*0x%x, slotId=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserIdArray, slotId, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserIdArray || !variableArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_USER_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	return CELL_OK;
}

error_code sceNpTusGetFriendsVariable(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusVariable> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetFriendsVariable(transId=%d, slotId=%d, includeSelf=%d, sortType=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, slotId, includeSelf, sortType, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!variableArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || arrayNum <= 0 || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SELECTED_FRIENDS_NUM)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	const s32 variables_read = 0;
	return not_an_error(variables_read);
}

error_code sceNpTusGetFriendsVariableAsync(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusVariable> variableArray, u64 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetFriendsVariableAsync(transId=%d, slotId=%d, includeSelf=%d, sortType=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, slotId, includeSelf, sortType, variableArray, variableArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!variableArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || arrayNum <= 0 || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SELECTED_FRIENDS_NUM)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	return CELL_OK;
}

error_code sceNpTusAddAndGetVariable(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u64 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option)
{
	sceNpTus.todo("sceNpTusAddAndGetVariable(transId=%d, targetNpId=*0x%x, slotId=%d, inVariable=%d, outVariable=*0x%x, outVariableSize=%d, option=*0x%x)", transId, targetNpId, slotId, inVariable, outVariable, outVariableSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusAddAndGetVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u64 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option)
{
	sceNpTus.todo("sceNpTusAddAndGetVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, inVariable=%d, outVariable=*0x%x, outVariableSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, inVariable, outVariable, outVariableSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusAddAndGetVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u64 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option)
{
	sceNpTus.todo("sceNpTusAddAndGetVariableAsync(transId=%d, targetNpId=*0x%x, slotId=%d, inVariable=%d, outVariable=*0x%x, outVariableSize=%d, option=*0x%x)", transId, targetNpId, slotId, inVariable, outVariable, outVariableSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusAddAndGetVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u64 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option)
{
	sceNpTus.todo("sceNpTusAddAndGetVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, inVariable=%d, outVariable=*0x%x, outVariableSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, inVariable, outVariable, outVariableSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusTryAndSetVariable(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u64 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option)
{
	sceNpTus.todo("sceNpTusTryAndSetVariable(transId=%d, targetNpId=*0x%x, slotId=%d, opeType=%d, variable=%d, resultVariable=*0x%x, resultVariableSize=%d, option=*0x%x)", transId, targetNpId, slotId, opeType, variable, resultVariable, resultVariableSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !resultVariable)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusTryAndSetVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u64 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option)
{
	sceNpTus.todo("sceNpTusTryAndSetVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, opeType=%d, variable=%d, resultVariable=*0x%x, resultVariableSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, opeType, variable, resultVariable, resultVariableSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !resultVariable)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusTryAndSetVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u64 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option)
{
	sceNpTus.todo("sceNpTusTryAndSetVariableAsync(transId=%d, targetNpId=*0x%x, slotId=%d, opeType=%d, variable=%d, resultVariable=*0x%x, resultVariableSize=%d, option=*0x%x)", transId, targetNpId, slotId, opeType, variable, resultVariable, resultVariableSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !resultVariable)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusTryAndSetVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u64 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option)
{
	sceNpTus.todo("sceNpTusTryAndSetVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, opeType=%d, variable=%d, resultVariable=*0x%x, resultVariableSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, opeType, variable, resultVariable, resultVariableSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !resultVariable)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusDeleteMultiSlotVariable(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusDeleteMultiSlotVariable(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !slotIdArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusDeleteMultiSlotVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusDeleteMultiSlotVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !slotIdArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusDeleteMultiSlotVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusDeleteMultiSlotVariableAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !slotIdArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusDeleteMultiSlotVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusDeleteMultiSlotVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !slotIdArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusSetData(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, u64 totalSize, u64 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u64 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option)
{
	sceNpTus.todo("sceNpTusSetData(transId=%d, targetNpId=*0x%x, slotId=%d, totalSize=%d, sendSize=%d, data=*0x%x, info=*0x%x, infoStructSize=%d, option=*0x%x)", transId, targetNpId, slotId, totalSize, sendSize, data, info, infoStructSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !data || totalSize == 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	const s32 data_size = 0;
	return not_an_error(data_size);
}

error_code sceNpTusSetDataVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, u64 totalSize, u64 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u64 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option)
{
	sceNpTus.todo("sceNpTusSetDataAsync(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, totalSize=%d, sendSize=%d, data=*0x%x, info=*0x%x, infoStructSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, totalSize, sendSize, data, info, infoStructSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !data || totalSize == 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	const s32 data_size = 0;
	return not_an_error(data_size);
}

error_code sceNpTusSetDataAsync(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, u64 totalSize, u64 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u64 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option)
{
	sceNpTus.todo("sceNpTusSetDataAsync(transId=%d, targetNpId=*0x%x, slotId=%d, totalSize=%d, sendSize=%d, data=*0x%x, info=*0x%x, infoStructSize=%d, option=*0x%x)", transId, targetNpId, slotId, totalSize, sendSize, data, info, infoStructSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !data || totalSize == 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusSetDataVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, u64 totalSize, u64 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u64 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option)
{
	sceNpTus.todo("sceNpTusSetDataAsync(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, totalSize=%d, sendSize=%d, data=*0x%x, info=*0x%x, infoStructSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, totalSize, sendSize, data, info, infoStructSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !data || totalSize == 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusGetData(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u64 dataStatusSize, vm::ptr<void> data, u64 recvSize, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetData(transId=%d, targetNpId=*0x%x, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, targetNpId, slotId, dataStatus, dataStatusSize, data, recvSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	const s32 data_size = 0;
	return not_an_error(data_size);
}

error_code sceNpTusGetDataVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u64 dataStatusSize, vm::ptr<void> data, u64 recvSize, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetDataVUser(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, dataStatus, dataStatusSize, data, recvSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	const s32 data_size = 0;
	return not_an_error(data_size);
}

error_code sceNpTusGetDataAsync(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u64 dataStatusSize, vm::ptr<void> data, u64 recvSize, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetDataAsync(transId=%d, targetNpId=*0x%x, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, targetNpId, slotId, dataStatus, dataStatusSize, data, recvSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	memcpy(&dataStatus->ownerId, targetNpId.get_ptr(), sizeof(SceNpId));
	memcpy(&dataStatus->lastChangedAuthorId, targetNpId.get_ptr(), sizeof(SceNpId));
	dataStatus->hasData = 0;
	dataStatus->dataSize = 0;

	return CELL_OK;
}

error_code sceNpTusGetDataVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u64 dataStatusSize, vm::ptr<void> data, u64 recvSize, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetDataVUser(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, dataStatus, dataStatusSize, data, recvSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTusGetMultiSlotDataStatus(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiSlotDataStatus(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !statusArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	const s32 status_count = 0;
	return not_an_error(status_count);
}

error_code sceNpTusGetMultiSlotDataStatusVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiSlotDataStatusVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !statusArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	const s32 status_count = 0;
	return not_an_error(status_count);
}

error_code sceNpTusGetMultiSlotDataStatusAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiSlotDataStatusAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !statusArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusGetMultiSlotDataStatusVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiSlotDataStatusVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !statusArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusGetMultiUserDataStatus(s32 transId, vm::cptr<SceNpId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiUserDataStatus(transId=%d, targetNpIdArray=*0x%x, slotId=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpIdArray, slotId, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpIdArray || !statusArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_USER_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	const s32 status_count = 0;
	return not_an_error(status_count);
}

error_code sceNpTusGetMultiUserDataStatusVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiUserDataStatusVUser(transId=%d, targetVirtualUserIdArray=*0x%x, slotId=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserIdArray || !statusArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_USER_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	const s32 status_count = 0;
	return not_an_error(status_count);
}

error_code sceNpTusGetMultiUserDataStatusAsync(s32 transId, vm::cptr<SceNpId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiUserDataStatusAsync(transId=%d, targetNpIdArray=*0x%x, slotId=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpIdArray, slotId, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpIdArray || !statusArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_USER_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	return CELL_OK;
}

error_code sceNpTusGetMultiUserDataStatusVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetMultiUserDataStatusVUserAsync(transId=%d, targetVirtualUserIdArray=*0x%x, slotId=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserIdArray || !statusArray || arrayNum <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_USER_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	return CELL_OK;
}

error_code sceNpTusGetFriendsDataStatus(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetFriendsDataStatus(transId=%d, slotId=%d, includeSelf=%d, sortType=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, slotId, includeSelf, sortType, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!statusArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || arrayNum < 0 || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SELECTED_FRIENDS_NUM)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	const s32 status_count = 0;
	return not_an_error(status_count);
}

error_code sceNpTusGetFriendsDataStatusAsync(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusDataStatus> statusArray, u64 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusGetFriendsDataStatusAsync(transId=%d, slotId=%d, includeSelf=%d, sortType=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, slotId, includeSelf, sortType, statusArray, statusArraySize, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!statusArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || arrayNum < 0 || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SELECTED_FRIENDS_NUM)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	return CELL_OK;
}

error_code sceNpTusDeleteMultiSlotData(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusDeleteMultiSlotData(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !slotIdArray || arrayNum < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusDeleteMultiSlotDataVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusDeleteMultiSlotDataVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !slotIdArray || arrayNum < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusDeleteMultiSlotDataAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusDeleteMultiSlotDataAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetNpId || !slotIdArray || arrayNum < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTusDeleteMultiSlotDataVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.todo("sceNpTusDeleteMultiSlotDataVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, arrayNum, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!targetVirtualUserId || !slotIdArray || arrayNum < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || !validateSlotIds(slotIdArray))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	return CELL_OK;
}

error_code sceNpTssGetData(s32 transId, SceNpTssSlotId slotId, vm::ptr<SceNpTssDataStatus> dataStatus, u64 dataStatusSize, vm::ptr<void> data, u64 recvSize, vm::ptr<SceNpTssGetDataOptParam> option)
{
	sceNpTus.todo("sceNpTssGetData(transId=%d, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, slotId, dataStatus, dataStatusSize, data, recvSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!dataStatus)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTssGetDataAsync(s32 transId, SceNpTssSlotId slotId, vm::ptr<SceNpTssDataStatus> dataStatus, u64 dataStatusSize, vm::ptr<void> data, u64 recvSize, vm::ptr<SceNpTssGetDataOptParam> option)
{
	sceNpTus.todo("sceNpTssGetDataAsync(transId=%d, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, slotId, dataStatus, dataStatusSize, data, recvSize, option);

	auto& tus_manager = g_fxo->get<sce_np_tus_manager>();
	std::scoped_lock lock(tus_manager.mtx);

	if (!tus_manager.is_initialized)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!tus_manager.check_transaction_context_id(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!dataStatus)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpTssGetDataNoLimit()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	return CELL_OK;
}

error_code sceNpTssGetDataNoLimitAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	return CELL_OK;
}


DECLARE(ppu_module_manager::sceNpTus)("sceNpTus", []()
{
	REG_FUNC(sceNpTus, sceNpTusInit);
	REG_FUNC(sceNpTus, sceNpTusTerm);
	REG_FUNC(sceNpTus, sceNpTusCreateTitleCtx);
	REG_FUNC(sceNpTus, sceNpTusDestroyTitleCtx);
	REG_FUNC(sceNpTus, sceNpTusCreateTransactionCtx);
	REG_FUNC(sceNpTus, sceNpTusDestroyTransactionCtx);
	REG_FUNC(sceNpTus, sceNpTusSetTimeout);
	REG_FUNC(sceNpTus, sceNpTusAbortTransaction);
	REG_FUNC(sceNpTus, sceNpTusWaitAsync);
	REG_FUNC(sceNpTus, sceNpTusPollAsync);
	REG_FUNC(sceNpTus, sceNpTusSetMultiSlotVariable);
	REG_FUNC(sceNpTus, sceNpTusSetMultiSlotVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusSetMultiSlotVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusSetMultiSlotVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotVariable);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserVariable);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetFriendsVariable);
	REG_FUNC(sceNpTus, sceNpTusGetFriendsVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusAddAndGetVariable);
	REG_FUNC(sceNpTus, sceNpTusAddAndGetVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusAddAndGetVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusAddAndGetVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusTryAndSetVariable);
	REG_FUNC(sceNpTus, sceNpTusTryAndSetVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusTryAndSetVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusTryAndSetVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotVariable);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusSetData);
	REG_FUNC(sceNpTus, sceNpTusSetDataVUser);
	REG_FUNC(sceNpTus, sceNpTusSetDataAsync);
	REG_FUNC(sceNpTus, sceNpTusSetDataVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetData);
	REG_FUNC(sceNpTus, sceNpTusGetDataVUser);
	REG_FUNC(sceNpTus, sceNpTusGetDataAsync);
	REG_FUNC(sceNpTus, sceNpTusGetDataVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotDataStatus);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotDataStatusVUser);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotDataStatusAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotDataStatusVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserDataStatus);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserDataStatusVUser);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserDataStatusAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserDataStatusVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetFriendsDataStatus);
	REG_FUNC(sceNpTus, sceNpTusGetFriendsDataStatusAsync);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotData);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotDataVUser);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotDataAsync);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotDataVUserAsync);
	REG_FUNC(sceNpTus, sceNpTssGetData);
	REG_FUNC(sceNpTus, sceNpTssGetDataAsync);
	REG_FUNC(sceNpTus, sceNpTssGetDataNoLimit);
	REG_FUNC(sceNpTus, sceNpTssGetDataNoLimitAsync);
});
