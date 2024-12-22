#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "sceNp.h"
#include "sceNpTus.h"

#include "Emu/NP/np_handler.h"

LOG_CHANNEL(sceNpTus);

bool is_slot_array_valid(vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum)
{
	for (s32 i = 0; i < arrayNum; i++)
	{
		if (slotIdArray[i] < 0)
		{
			return false;
		}
	}

	return true;
}

const SceNpOnlineId& get_scenp_online_id(const SceNpId& target_npid)
{
	return target_npid.handle;
}

const SceNpOnlineId& get_scenp_online_id(const SceNpTusVirtualUserId& target_npid)
{
	return target_npid;
}

error_code sceNpTusInit(s32 prio)
{
	sceNpTus.warning("sceNpTusInit(prio=%d)", prio);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph.is_NP_TUS_init = true;

	return CELL_OK;
}

error_code sceNpTusTerm()
{
	sceNpTus.warning("sceNpTusTerm()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph.is_NP_TUS_init = false;

	return CELL_OK;
}

error_code sceNpTusCreateTitleCtx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase, vm::cptr<SceNpId> selfNpId)
{
	sceNpTus.warning("sceNpTusCreateTitleCtx(communicationId=*0x%x, passphrase=*0x%x, selfNpId=*0x%x)", communicationId, passphrase, selfNpId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!communicationId || !passphrase || !selfNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	s32 id = create_tus_context(communicationId, passphrase);

	if (id > 0)
	{
		return not_an_error(id);
	}

	return id;
}

error_code sceNpTusDestroyTitleCtx(s32 titleCtxId)
{
	sceNpTus.warning("sceNpTusDestroyTitleCtx(titleCtxId=%d)", titleCtxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!destroy_tus_context(titleCtxId))
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;

	return CELL_OK;
}

error_code sceNpTusCreateTransactionCtx(s32 titleCtxId)
{
	sceNpTus.warning("sceNpTusCreateTransactionCtx(titleCtxId=%d)", titleCtxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto tus = idm::get_unlocked<tus_ctx>(titleCtxId);

	if (!tus)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	s32 id = create_tus_transaction_context(tus);

	if (id > 0)
	{
		return not_an_error(id);
	}

	return id;
}

error_code sceNpTusDestroyTransactionCtx(s32 transId)
{
	sceNpTus.warning("sceNpTusDestroyTransactionCtx(transId=%d)", transId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!destroy_tus_transaction_context(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code sceNpTusSetTimeout(s32 ctxId, u32 timeout)
{
	sceNpTus.warning("sceNpTusSetTimeout(ctxId=%d, timeout=%d)", ctxId, timeout);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (timeout < 10'000'000) // 10 seconds
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (auto trans = idm::get_unlocked<tus_transaction_ctx>(ctxId))
	{
		trans->timeout = timeout;
	}
	else if (auto tus = idm::get_unlocked<tus_ctx>(ctxId))
	{
		tus->timeout = timeout;
	}
	else
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code sceNpTusAbortTransaction(s32 transId)
{
	sceNpTus.warning("sceNpTusAbortTransaction(transId=%d)", transId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto trans = idm::get_unlocked<tus_transaction_ctx>(transId);
	if (!trans)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	trans->abort_transaction();

	return CELL_OK;
}

error_code sceNpTusWaitAsync(s32 transId, vm::ptr<s32> result)
{
	sceNpTus.warning("sceNpTusWaitAsync(transId=%d, result=*0x%x)", transId, result);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto trans = idm::get_unlocked<tus_transaction_ctx>(transId);
	if (!trans)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	*result = trans->wait_for_completion();

	return CELL_OK;
}

error_code sceNpTusPollAsync(s32 transId, vm::ptr<s32> result)
{
	sceNpTus.warning("sceNpTusPollAsync(transId=%d, result=*0x%x)", transId, result);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto trans = idm::get_unlocked<tus_transaction_ctx>(transId);
	if (!trans)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	auto res = trans->get_transaction_status();

	if (!res)
	{
		return not_an_error(1);
	}

	*result = *res;
	return CELL_OK;
}

template<typename T>
error_code scenp_tus_set_multislot_variable(s32 transId, T targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option, bool vuser, bool async)
{
	if (!slotIdArray || !variableArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0 || option)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	if (!is_slot_array_valid(slotIdArray, arrayNum))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_set_multislot_variable(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotIdArray, variableArray, arrayNum, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusSetMultiSlotVariable(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusSetMultiSlotVariable(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, variableArray, arrayNum, option);
	return scenp_tus_set_multislot_variable(transId, targetNpId, slotIdArray, variableArray, arrayNum, option, false, false);
}

error_code sceNpTusSetMultiSlotVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusSetMultiSlotVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, variableArray, arrayNum, option);
	return scenp_tus_set_multislot_variable(transId, targetVirtualUserId, slotIdArray, variableArray, arrayNum, option, true, false);
}

error_code sceNpTusSetMultiSlotVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusSetMultiSlotVariableAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, variableArray, arrayNum, option);
	return scenp_tus_set_multislot_variable(transId, targetNpId, slotIdArray, variableArray, arrayNum, option, false, true);
}

error_code sceNpTusSetMultiSlotVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusSetMultiSlotVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, variableArray, arrayNum, option);
	return scenp_tus_set_multislot_variable(transId, targetVirtualUserId, slotIdArray, variableArray, arrayNum, option, true, true);
}

template<typename T>
error_code scenp_tus_get_multislot_variable(s32 transId, T targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option, bool vuser, bool async)
{
	if (!slotIdArray || !variableArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0 || option)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (variableArraySize != arrayNum * sizeof(SceNpTusVariable))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	if (!is_slot_array_valid(slotIdArray, arrayNum))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_get_multislot_variable(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotIdArray, variableArray, arrayNum, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusGetMultiSlotVariable(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiSlotVariable(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_multislot_variable(transId, targetNpId, slotIdArray, variableArray, variableArraySize, arrayNum, option, false, false);
}

error_code sceNpTusGetMultiSlotVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiSlotVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_multislot_variable(transId, targetVirtualUserId, slotIdArray, variableArray, variableArraySize, arrayNum, option, true, false);
}

error_code sceNpTusGetMultiSlotVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiSlotVariableAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_multislot_variable(transId, targetNpId, slotIdArray, variableArray, variableArraySize, arrayNum, option, false, true);
}

error_code sceNpTusGetMultiSlotVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiSlotVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_multislot_variable(transId, targetVirtualUserId, slotIdArray, variableArray, variableArraySize, arrayNum, option, true, true);
}

template<typename T>
error_code scenp_tus_get_multiuser_variable(s32 transId, T targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option, bool vuser, bool async)
{
	if (!variableArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0 || option)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_NPID_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	if (variableArraySize != arrayNum * sizeof(SceNpTusVariable))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!targetNpIdArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	std::vector<SceNpOnlineId> online_ids;
	for (s32 i = 0; i < arrayNum; i++)
	{
		online_ids.push_back(get_scenp_online_id(targetNpIdArray[i]));
	}

	nph.tus_get_multiuser_variable(trans_ctx, std::move(online_ids), slotId, variableArray, arrayNum, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusGetMultiUserVariable(s32 transId, vm::cptr<SceNpId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiUserVariable(transId=%d, targetNpIdArray=*0x%x, slotId=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpIdArray, slotId, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_multiuser_variable(transId, targetNpIdArray, slotId, variableArray, variableArraySize, arrayNum, option, false, false);
}

error_code sceNpTusGetMultiUserVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiUserVariableVUser(transId=%d, targetVirtualUserIdArray=*0x%x, slotId=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserIdArray, slotId, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_multiuser_variable(transId, targetVirtualUserIdArray, slotId, variableArray, variableArraySize, arrayNum, option, true, false);
}

error_code sceNpTusGetMultiUserVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiUserVariableAsync(transId=%d, targetNpIdArray=*0x%x, slotId=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpIdArray, slotId, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_multiuser_variable(transId, targetNpIdArray, slotId, variableArray, variableArraySize, arrayNum, option, false, true);
}

error_code sceNpTusGetMultiUserVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiUserVariableVUserAsync(transId=%d, targetVirtualUserIdArray=*0x%x, slotId=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserIdArray, slotId, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_multiuser_variable(transId, targetVirtualUserIdArray, slotId, variableArray, variableArraySize, arrayNum, option, true, true);
}

error_code scenp_tus_get_friends_variable(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option, bool async)
{
	if (!variableArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	// Undocumented behaviour and structure unknown
	// Also checks a u32* at offset 4 of the struct for nullptr in which case it behaves like option == nullptr
	if (option && *static_cast<u32*>(option.get_ptr()) != 0xC)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (sortType < SCE_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE || sortType > SCE_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_SELECTED_FRIENDS_NUM)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	if (variableArraySize != arrayNum * sizeof(SceNpTusVariable))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_get_friends_variable(trans_ctx, slotId, includeSelf, sortType, variableArray, arrayNum, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusGetFriendsVariable(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetFriendsVariable(transId=%d, slotId=%d, includeSelf=%d, sortType=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, slotId, includeSelf, sortType, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_friends_variable(transId, slotId, includeSelf, sortType, variableArray, variableArraySize, arrayNum, option, false);
}

error_code sceNpTusGetFriendsVariableAsync(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusVariable> variableArray, u32 variableArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetFriendsVariableAsync(transId=%d, slotId=%d, includeSelf=%d, sortType=%d, variableArray=*0x%x, variableArraySize=%d, arrayNum=%d, option=*0x%x)", transId, slotId, includeSelf, sortType, variableArray, variableArraySize, arrayNum, option);
	return scenp_tus_get_friends_variable(transId, slotId, includeSelf, sortType, variableArray, variableArraySize, arrayNum, option, true);
}

template<typename T>
error_code scenp_tus_add_and_get_variable(s32 transId, T targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u32 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option, bool vuser, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if ((option && option->size != sizeof(SceNpTusAddAndGetVariableOptParam)) || outVariableSize != sizeof(SceNpTusVariable))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_add_and_get_variable(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotId, inVariable, outVariable, option, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusAddAndGetVariable(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u32 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option)
{
	sceNpTus.warning("sceNpTusAddAndGetVariable(transId=%d, targetNpId=*0x%x, slotId=%d, inVariable=%d, outVariable=*0x%x, outVariableSize=%d, option=*0x%x)", transId, targetNpId, slotId, inVariable, outVariable, outVariableSize, option);
	return scenp_tus_add_and_get_variable(transId, targetNpId, slotId, inVariable, outVariable, outVariableSize, option, false, false);
}

error_code sceNpTusAddAndGetVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u32 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option)
{
	sceNpTus.warning("sceNpTusAddAndGetVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, inVariable=%d, outVariable=*0x%x, outVariableSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, inVariable, outVariable, outVariableSize, option);
	return scenp_tus_add_and_get_variable(transId, targetVirtualUserId, slotId, inVariable, outVariable, outVariableSize, option, true, false);
}

error_code sceNpTusAddAndGetVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u32 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option)
{
	sceNpTus.warning("sceNpTusAddAndGetVariableAsync(transId=%d, targetNpId=*0x%x, slotId=%d, inVariable=%d, outVariable=*0x%x, outVariableSize=%d, option=*0x%x)", transId, targetNpId, slotId, inVariable, outVariable, outVariableSize, option);
	return scenp_tus_add_and_get_variable(transId, targetNpId, slotId, inVariable, outVariable, outVariableSize, option, false, true);
}

error_code sceNpTusAddAndGetVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, u32 outVariableSize, vm::ptr<SceNpTusAddAndGetVariableOptParam> option)
{
	sceNpTus.warning("sceNpTusAddAndGetVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, inVariable=%d, outVariable=*0x%x, outVariableSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, inVariable, outVariable, outVariableSize, option);
	return scenp_tus_add_and_get_variable(transId, targetVirtualUserId, slotId, inVariable, outVariable, outVariableSize, option, true, true);
}

template<typename T>
error_code scenp_tus_try_and_set_variable(s32 transId, T targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u32 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option, bool vuser, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!resultVariable)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if ((option && option->size != sizeof(SceNpTusTryAndSetVariableOptParam)) || resultVariableSize != sizeof(SceNpTusVariable))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_try_and_set_variable(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotId, opeType, variable, resultVariable, option, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusTryAndSetVariable(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u32 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option)
{
	sceNpTus.warning("sceNpTusTryAndSetVariable(transId=%d, targetNpId=*0x%x, slotId=%d, opeType=%d, variable=%d, resultVariable=*0x%x, resultVariableSize=%d, option=*0x%x)", transId, targetNpId, slotId, opeType, variable, resultVariable, resultVariableSize, option);
	return scenp_tus_try_and_set_variable(transId, targetNpId, slotId, opeType, variable, resultVariable, resultVariableSize, option, false, false);
}

error_code sceNpTusTryAndSetVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u32 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option)
{
	sceNpTus.warning("sceNpTusTryAndSetVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, opeType=%d, variable=%d, resultVariable=*0x%x, resultVariableSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, opeType, variable, resultVariable, resultVariableSize, option);
	return scenp_tus_try_and_set_variable(transId, targetVirtualUserId, slotId, opeType, variable, resultVariable, resultVariableSize, option, true, false);
}

error_code sceNpTusTryAndSetVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u32 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option)
{
	sceNpTus.warning("sceNpTusTryAndSetVariableAsync(transId=%d, targetNpId=*0x%x, slotId=%d, opeType=%d, variable=%d, resultVariable=*0x%x, resultVariableSize=%d, option=*0x%x)", transId, targetNpId, slotId, opeType, variable, resultVariable, resultVariableSize, option);
	return scenp_tus_try_and_set_variable(transId, targetNpId, slotId, opeType, variable, resultVariable, resultVariableSize, option, false, true);
}

error_code sceNpTusTryAndSetVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, u32 resultVariableSize, vm::ptr<SceNpTusTryAndSetVariableOptParam> option)
{
	sceNpTus.warning("sceNpTusTryAndSetVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, opeType=%d, variable=%d, resultVariable=*0x%x, resultVariableSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, opeType, variable, resultVariable, resultVariableSize, option);
	return scenp_tus_try_and_set_variable(transId, targetVirtualUserId, slotId, opeType, variable, resultVariable, resultVariableSize, option, true, true);
}

template<typename T>
error_code scenp_tus_delete_multislot_variable(s32 transId, T targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option, bool vuser, bool async)
{
	if (!slotIdArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0 || option)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	if (!is_slot_array_valid(slotIdArray, arrayNum))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_delete_multislot_variable(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotIdArray, arrayNum, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusDeleteMultiSlotVariable(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusDeleteMultiSlotVariable(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, arrayNum, option);
	return scenp_tus_delete_multislot_variable(transId, targetNpId, slotIdArray, arrayNum, option, false, false);
}

error_code sceNpTusDeleteMultiSlotVariableVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusDeleteMultiSlotVariableVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, arrayNum, option);
	return scenp_tus_delete_multislot_variable(transId, targetVirtualUserId, slotIdArray, arrayNum, option, true, false);
}

error_code sceNpTusDeleteMultiSlotVariableAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusDeleteMultiSlotVariableAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, arrayNum, option);
	return scenp_tus_delete_multislot_variable(transId, targetNpId, slotIdArray, arrayNum, option, false, true);
}

error_code sceNpTusDeleteMultiSlotVariableVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusDeleteMultiSlotVariableVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, arrayNum, option);
	return scenp_tus_delete_multislot_variable(transId, targetVirtualUserId, slotIdArray, arrayNum, option, true, true);
}

template<typename T>
error_code scenp_tus_set_data(s32 transId, T targetNpId, SceNpTusSlotId slotId, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u32 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option, bool vuser, bool async)
{
	if (slotId < 0 || !data || !totalSize)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if ((option && option->size != sizeof(SceNpTusSetDataOptParam)) || (info && infoStructSize != sizeof(SceNpTusDataInfo)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_set_data(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotId, totalSize, sendSize, data, info, option, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusSetData(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u32 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option)
{
	sceNpTus.warning("sceNpTusSetData(transId=%d, targetNpId=*0x%x, slotId=%d, totalSize=%d, sendSize=%d, data=*0x%x, info=*0x%x, infoStructSize=%d, option=*0x%x)", transId, targetNpId, slotId, totalSize, sendSize, data, info, infoStructSize, option);
	return scenp_tus_set_data(transId, targetNpId, slotId, totalSize, sendSize, data, info, infoStructSize, option, false, false);
}

error_code sceNpTusSetDataVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u32 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option)
{
	sceNpTus.warning("sceNpTusSetDataVUser(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, totalSize=%d, sendSize=%d, data=*0x%x, info=*0x%x, infoStructSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, totalSize, sendSize, data, info, infoStructSize, option);
	return scenp_tus_set_data(transId, targetVirtualUserId, slotId, totalSize, sendSize, data, info, infoStructSize, option, true, false);
}

error_code sceNpTusSetDataAsync(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u32 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option)
{
	sceNpTus.warning("sceNpTusSetDataAsync(transId=%d, targetNpId=*0x%x, slotId=%d, totalSize=%d, sendSize=%d, data=*0x%x, info=*0x%x, infoStructSize=%d, option=*0x%x)", transId, targetNpId, slotId, totalSize, sendSize, data, info, infoStructSize, option);
	return scenp_tus_set_data(transId, targetNpId, slotId, totalSize, sendSize, data, info, infoStructSize, option, false, true);
}

error_code sceNpTusSetDataVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, u32 infoStructSize, vm::ptr<SceNpTusSetDataOptParam> option)
{
	sceNpTus.warning("sceNpTusSetDataVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, totalSize=%d, sendSize=%d, data=*0x%x, info=*0x%x, infoStructSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, totalSize, sendSize, data, info, infoStructSize, option);
	return scenp_tus_set_data(transId, targetVirtualUserId, slotId, totalSize, sendSize, data, info, infoStructSize, option, true, true);
}

template<typename T>
error_code scenp_tus_get_data(s32 transId, T targetNpId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u32 dataStatusSize, vm::ptr<void> data, u32 recvSize, vm::ptr<void> option, bool vuser, bool async)
{
	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option || slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (dataStatusSize != sizeof(SceNpTusDataStatus))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_get_data(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotId, dataStatus, data, recvSize, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusGetData(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u32 dataStatusSize, vm::ptr<void> data, u32 recvSize, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetData(transId=%d, targetNpId=*0x%x, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, targetNpId, slotId, dataStatus, dataStatusSize, data, recvSize, option);
	return scenp_tus_get_data(transId, targetNpId, slotId, dataStatus, dataStatusSize, data, recvSize, option, false, false);
}

error_code sceNpTusGetDataVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u32 dataStatusSize, vm::ptr<void> data, u32 recvSize, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetDataVUser(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, dataStatus, dataStatusSize, data, recvSize, option);
	return scenp_tus_get_data(transId, targetVirtualUserId, slotId, dataStatus, dataStatusSize, data, recvSize, option, true, false);
}

error_code sceNpTusGetDataAsync(s32 transId, vm::cptr<SceNpId> targetNpId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u32 dataStatusSize, vm::ptr<void> data, u32 recvSize, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetDataAsync(transId=%d, targetNpId=*0x%x, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, targetNpId, slotId, dataStatus, dataStatusSize, data, recvSize, option);
	return scenp_tus_get_data(transId, targetNpId, slotId, dataStatus, dataStatusSize, data, recvSize, option, false, true);
}

error_code sceNpTusGetDataVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, u32 dataStatusSize, vm::ptr<void> data, u32 recvSize, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetDataVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, targetVirtualUserId, slotId, dataStatus, dataStatusSize, data, recvSize, option);
	return scenp_tus_get_data(transId, targetVirtualUserId, slotId, dataStatus, dataStatusSize, data, recvSize, option, true, true);
}

template<typename T>
error_code scenp_tus_get_multislot_data_status(s32 transId, T targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option, bool vuser, bool async)
{
	if (!slotIdArray || !statusArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0 || option)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (statusArraySize != arrayNum * sizeof(SceNpTusDataStatus))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	if (!is_slot_array_valid(slotIdArray, arrayNum))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!targetNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_get_multislot_data_status(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotIdArray, statusArray, arrayNum, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusGetMultiSlotDataStatus(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiSlotDataStatus(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_multislot_data_status(transId, targetNpId, slotIdArray, statusArray, statusArraySize, arrayNum, option, false, false);
}

error_code sceNpTusGetMultiSlotDataStatusVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiSlotDataStatusVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_multislot_data_status(transId, targetVirtualUserId, slotIdArray, statusArray, statusArraySize, arrayNum, option, true, false);
}

error_code sceNpTusGetMultiSlotDataStatusAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiSlotDataStatusAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_multislot_data_status(transId, targetNpId, slotIdArray, statusArray, statusArraySize, arrayNum, option, false, true);
}

error_code sceNpTusGetMultiSlotDataStatusVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiSlotDataStatusVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_multislot_data_status(transId, targetVirtualUserId, slotIdArray, statusArray, statusArraySize, arrayNum, option, true, true);
}

template<typename T>
error_code scenp_tus_get_multiuser_data_status(s32 transId, T targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option, bool vuser, bool async)
{
	if (!statusArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0 || option)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_NPID_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	if (statusArraySize != arrayNum * sizeof(SceNpTusDataStatus))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!targetNpIdArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	std::vector<SceNpOnlineId> online_ids;
	for (s32 i = 0; i < arrayNum; i++)
	{
		online_ids.push_back(get_scenp_online_id(targetNpIdArray[i]));
	}

	nph.tus_get_multiuser_data_status(trans_ctx, std::move(online_ids), slotId, statusArray, arrayNum, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusGetMultiUserDataStatus(s32 transId, vm::cptr<SceNpId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiUserDataStatus(transId=%d, targetNpIdArray=*0x%x, slotId=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpIdArray, slotId, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_multiuser_data_status(transId, targetNpIdArray, slotId, statusArray, statusArraySize, arrayNum, option, false, false);
}

error_code sceNpTusGetMultiUserDataStatusVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiUserDataStatusVUser(transId=%d, targetVirtualUserIdArray=*0x%x, slotId=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_multiuser_data_status(transId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayNum, option, true, false);
}

error_code sceNpTusGetMultiUserDataStatusAsync(s32 transId, vm::cptr<SceNpId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiUserDataStatusAsync(transId=%d, targetNpIdArray=*0x%x, slotId=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetNpIdArray, slotId, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_multiuser_data_status(transId, targetNpIdArray, slotId, statusArray, statusArraySize, arrayNum, option, false, true);
}

error_code sceNpTusGetMultiUserDataStatusVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetMultiUserDataStatusVUserAsync(transId=%d, targetVirtualUserIdArray=*0x%x, slotId=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_multiuser_data_status(transId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayNum, option, true, true);
}

error_code scenp_tus_get_friends_data_status(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option, bool async)
{
	if (!statusArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	// Undocumented behaviour and structure unknown
	// Also checks a u32* at offset 4 of the struct for nullptr in which case it behaves like option == nullptr
	if (option && *static_cast<u32*>(option.get_ptr()) != 0xC)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (sortType != SCE_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE && sortType != SCE_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_SELECTED_FRIENDS_NUM)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	if (statusArraySize != arrayNum * sizeof(SceNpTusDataStatus))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (slotId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_get_friends_data_status(trans_ctx, slotId, includeSelf, sortType, statusArray, arrayNum, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpTusGetFriendsDataStatus(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetFriendsDataStatus(transId=%d, slotId=%d, includeSelf=%d, sortType=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, slotId, includeSelf, sortType, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_friends_data_status(transId, slotId, includeSelf, sortType, statusArray, statusArraySize, arrayNum, option, false);
}

error_code sceNpTusGetFriendsDataStatusAsync(s32 transId, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusDataStatus> statusArray, u32 statusArraySize, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusGetFriendsDataStatusAsync(transId=%d, slotId=%d, includeSelf=%d, sortType=%d, statusArray=*0x%x, statusArraySize=%d, arrayNum=%d, option=*0x%x)", transId, slotId, includeSelf, sortType, statusArray, statusArraySize, arrayNum, option);
	return scenp_tus_get_friends_data_status(transId, slotId, includeSelf, sortType, statusArray, statusArraySize, arrayNum, option, true);
}

template<typename T>
error_code scenp_tus_delete_multislot_data(s32 transId, T targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option, bool vuser, bool async)
{
	if (!slotIdArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum == 0 || option)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
	}

	if (!is_slot_array_valid(slotIdArray, arrayNum))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	// Probable vsh behaviour
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.tus_delete_multislot_data(trans_ctx, get_scenp_online_id(*targetNpId.get_ptr()), slotIdArray, arrayNum, vuser, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}


error_code sceNpTusDeleteMultiSlotData(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusDeleteMultiSlotData(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, arrayNum, option);
	return scenp_tus_delete_multislot_data(transId, targetNpId, slotIdArray, arrayNum, option, false, false);
}

error_code sceNpTusDeleteMultiSlotDataVUser(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusDeleteMultiSlotDataVUser(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, arrayNum, option);
	return scenp_tus_delete_multislot_data(transId, targetVirtualUserId, slotIdArray, arrayNum, option, true, false);
}

error_code sceNpTusDeleteMultiSlotDataAsync(s32 transId, vm::cptr<SceNpId> targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusDeleteMultiSlotDataAsync(transId=%d, targetNpId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetNpId, slotIdArray, arrayNum, option);
	return scenp_tus_delete_multislot_data(transId, targetNpId, slotIdArray, arrayNum, option, false, true);
}

error_code sceNpTusDeleteMultiSlotDataVUserAsync(s32 transId, vm::cptr<SceNpTusVirtualUserId> targetVirtualUserId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, vm::ptr<void> option)
{
	sceNpTus.warning("sceNpTusDeleteMultiSlotDataVUserAsync(transId=%d, targetVirtualUserId=*0x%x, slotIdArray=*0x%x, arrayNum=%d, option=*0x%x)", transId, targetVirtualUserId, slotIdArray, arrayNum, option);
	return scenp_tus_delete_multislot_data(transId, targetVirtualUserId, slotIdArray, arrayNum, option, true, true);
}

void scenp_tss_no_file(const shared_ptr<tus_transaction_ctx>& trans, vm::ptr<SceNpTssDataStatus> dataStatus)
{
	// TSS are files stored on PSN by developers, no dumps available atm
	std::memset(dataStatus.get_ptr(), 0, sizeof(SceNpTssDataStatus));
	trans->result = not_an_error(0);
}

error_code sceNpTssGetData(s32 transId, SceNpTssSlotId slotId, vm::ptr<SceNpTssDataStatus> dataStatus, u32 dataStatusSize, vm::ptr<void> data, u32 recvSize, vm::ptr<SceNpTssGetDataOptParam> option)
{
	sceNpTus.warning("sceNpTssGetData(transId=%d, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, slotId, dataStatus, dataStatusSize, data, recvSize, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	if (!dataStatus)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	scenp_tss_no_file(trans_ctx, dataStatus);

	return CELL_OK;
}

error_code sceNpTssGetDataAsync(s32 transId, SceNpTssSlotId slotId, vm::ptr<SceNpTssDataStatus> dataStatus, u32 dataStatusSize, vm::ptr<void> data, u32 recvSize, vm::ptr<SceNpTssGetDataOptParam> option)
{
	sceNpTus.warning("sceNpTssGetDataAsync(transId=%d, slotId=%d, dataStatus=*0x%x, dataStatusSize=%d, data=*0x%x, recvSize=%d, option=*0x%x)", transId, slotId, dataStatus, dataStatusSize, data, recvSize, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_TUS_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	if (!dataStatus)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	auto trans_ctx = idm::get_unlocked<tus_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	scenp_tss_no_file(trans_ctx, dataStatus);

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
