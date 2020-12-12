#include "stdafx.h"
#include "np_contexts.h"

#include "Emu/IdManager.h"

score_ctx::score_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	ASSERT(!communicationId->term && strlen(communicationId->data) == 9);
	memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
	memcpy(&this->passphrase, passphrase.get_ptr(), sizeof(SceNpCommunicationPassphrase));
}
s32 create_score_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	return static_cast<s32>(idm::make<score_ctx>(communicationId, passphrase));
}
bool destroy_score_context(s32 ctx_id)
{
	return idm::remove<score_ctx>(static_cast<u32>(ctx_id));
}

score_transaction_ctx::score_transaction_ctx(s32 score_context_id)
{
	this->score_context_id = score_context_id;
}
s32 create_score_transaction_context(s32 score_context_id)
{
	return static_cast<s32>(idm::make<score_transaction_ctx>(score_context_id));
}
bool destroy_score_transaction_context(s32 ctx_id)
{
	return idm::remove<score_transaction_ctx>(static_cast<u32>(ctx_id));
}

match2_ctx::match2_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	ASSERT(!communicationId->term && strlen(communicationId->data) == 9);
	memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
	memcpy(&this->passphrase, passphrase.get_ptr(), sizeof(SceNpCommunicationPassphrase));
}
u16 create_match2_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
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
	ASSERT(!communicationId->term && strlen(communicationId->data) == 9);
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
    : version(version),
    context_callback(handler),
    context_callback_param(arg)
{
	memcpy(&this->npid, npid.get_ptr(), sizeof(SceNpId));
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
    : handler(handler), arg(arg)
{
	memcpy(&this->npid, npid.get_ptr(), sizeof(SceNpId));
}
s32 create_signaling_context(vm::ptr<SceNpId> npid, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg)
{
	return static_cast<s32>(idm::make<signaling_ctx>(npid, handler, arg));
}
bool destroy_signaling_context(s32 ctx_id)
{
	return idm::remove<signaling_ctx>(static_cast<u32>(ctx_id));
}
