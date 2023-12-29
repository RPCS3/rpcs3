#include "stdafx.h"
#include "np_contexts.h"

#include "Emu/IdManager.h"

LOG_CHANNEL(sceNp2);

generic_async_transaction_context::generic_async_transaction_context(const SceNpCommunicationId& communicationId, const SceNpCommunicationPassphrase& passphrase, u64 timeout)
	: communicationId(communicationId), passphrase(passphrase), timeout(timeout)
{
}

generic_async_transaction_context::~generic_async_transaction_context()
{
	if (thread.joinable())
		thread.join();
}

std::optional<s32> generic_async_transaction_context::get_transaction_status()
{
	std::lock_guard lock(mutex);
	return result;
}
void generic_async_transaction_context::abort_transaction()
{
	std::lock_guard lock(mutex);

	result = SCE_NP_COMMUNITY_ERROR_ABORTED;
	wake_cond.notify_one();
}
error_code generic_async_transaction_context::wait_for_completion()
{
	std::unique_lock lock(mutex);

	if (result)
	{
		return *result;
	}

	completion_cond.wait(lock);

	return *result;
}

bool generic_async_transaction_context::set_result_and_wake(error_code err)
{
	result = err;
	wake_cond.notify_one();
	return true;
}

tus_ctx::tus_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	ensure(!communicationId->data[9] && strlen(communicationId->data) == 9);
	memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
	memcpy(&this->passphrase, passphrase.get_ptr(), sizeof(SceNpCommunicationPassphrase));
}
s32 create_tus_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	s32 tus_id = idm::make<tus_ctx>(communicationId, passphrase);

	if (tus_id == id_manager::id_traits<tus_ctx>::invalid)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
	}

	return static_cast<s32>(tus_id);
}
bool destroy_tus_context(s32 ctx_id)
{
	return idm::remove<tus_ctx>(static_cast<u32>(ctx_id));
}

tus_transaction_ctx::tus_transaction_ctx(const std::shared_ptr<tus_ctx>& tus)
	: generic_async_transaction_context(tus->communicationId, tus->passphrase, tus->timeout)
{
}

s32 create_tus_transaction_context(const std::shared_ptr<tus_ctx>& tus)
{
	s32 tus_id = idm::make<tus_transaction_ctx>(tus);

	if (tus_id == id_manager::id_traits<tus_transaction_ctx>::invalid)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
	}

	return static_cast<s32>(tus_id);
}
bool destroy_tus_transaction_context(s32 ctx_id)
{
	return idm::remove<tus_transaction_ctx>(static_cast<u32>(ctx_id));
}

score_ctx::score_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	ensure(!communicationId->data[9] && strlen(communicationId->data) == 9);
	memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
	memcpy(&this->passphrase, passphrase.get_ptr(), sizeof(SceNpCommunicationPassphrase));
}
s32 create_score_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	s32 score_id = idm::make<score_ctx>(communicationId, passphrase);

	if (score_id == id_manager::id_traits<score_ctx>::invalid)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
	}

	return static_cast<s32>(score_id);
}
bool destroy_score_context(s32 ctx_id)
{
	return idm::remove<score_ctx>(static_cast<u32>(ctx_id));
}

score_transaction_ctx::score_transaction_ctx(const std::shared_ptr<score_ctx>& score)
	: generic_async_transaction_context(score->communicationId, score->passphrase, score->timeout)
{
	pcId = score->pcId;
}

s32 create_score_transaction_context(const std::shared_ptr<score_ctx>& score)
{
	s32 trans_id = idm::make<score_transaction_ctx>(score);

	if (trans_id == id_manager::id_traits<score_transaction_ctx>::invalid)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
	}

	return static_cast<s32>(trans_id);
}
bool destroy_score_transaction_context(s32 ctx_id)
{
	return idm::remove<score_transaction_ctx>(static_cast<u32>(ctx_id));
}

match2_ctx::match2_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	ensure(!communicationId->data[9] && strlen(communicationId->data) == 9);
	memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
	memcpy(&this->passphrase, passphrase.get_ptr(), sizeof(SceNpCommunicationPassphrase));
}
u16 create_match2_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	sceNp2.notice("Creating match2 context with communicationId: <%s>", static_cast<const char*>(communicationId->data));
	return static_cast<u16>(idm::make<match2_ctx>(communicationId, passphrase));
}
bool destroy_match2_context(u16 ctx_id)
{
	return idm::remove<match2_ctx>(static_cast<u32>(ctx_id));
}
bool check_match2_context(u16 ctx_id)
{
	return (idm::check<match2_ctx>(ctx_id) != nullptr);
}
std::shared_ptr<match2_ctx> get_match2_context(u16 ctx_id)
{
	return idm::get<match2_ctx>(ctx_id);
}

lookup_title_ctx::lookup_title_ctx(vm::cptr<SceNpCommunicationId> communicationId)
{
	ensure(!communicationId->data[9] && strlen(communicationId->data) == 9);
	memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
}
s32 create_lookup_title_context(vm::cptr<SceNpCommunicationId> communicationId)
{
	return static_cast<s32>(idm::make<lookup_title_ctx>(communicationId));
}
bool destroy_lookup_title_context(s32 ctx_id)
{
	return idm::remove<lookup_title_ctx>(static_cast<u32>(ctx_id));
}

lookup_transaction_ctx::lookup_transaction_ctx(s32 lt_ctx)
{
	this->lt_ctx = lt_ctx;
}
s32 create_lookup_transaction_context(s32 lt_ctx)
{
	return static_cast<s32>(idm::make<lookup_transaction_ctx>(lt_ctx));
}
bool destroy_lookup_transaction_context(s32 ctx_id)
{
	return idm::remove<lookup_transaction_ctx>(static_cast<u32>(ctx_id));
}

commerce2_ctx::commerce2_ctx(u32 version, vm::cptr<SceNpId> npid, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg)
{
	this->version = version;
	memcpy(&this->npid, npid.get_ptr(), sizeof(SceNpId));
	this->context_callback = handler;
	this->context_callback_param = arg;
}
s32 create_commerce2_context(u32 version, vm::cptr<SceNpId> npid, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg)
{
	return static_cast<s32>(idm::make<commerce2_ctx>(version, npid, handler, arg));
}
bool destroy_commerce2_context(s32 ctx_id)
{
	return idm::remove<commerce2_ctx>(static_cast<u32>(ctx_id));
}
std::shared_ptr<commerce2_ctx> get_commerce2_context(u16 ctx_id)
{
	return idm::get_unlocked<commerce2_ctx>(ctx_id);
}

signaling_ctx::signaling_ctx(vm::ptr<SceNpId> npid, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg)
{
	memcpy(&this->npid, npid.get_ptr(), sizeof(SceNpId));
	this->handler = handler;
	this->arg = arg;
}
s32 create_signaling_context(vm::ptr<SceNpId> npid, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg)
{
	return static_cast<s32>(idm::make<signaling_ctx>(npid, handler, arg));
}
bool destroy_signaling_context(s32 ctx_id)
{
	return idm::remove<signaling_ctx>(static_cast<u32>(ctx_id));
}
