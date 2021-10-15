#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/sceNpCommerce2.h"

// Score related
struct score_ctx
{
	score_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);

	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 32;

	SceNpCommunicationId communicationId{};
	SceNpCommunicationPassphrase passphrase{};
};
s32 create_score_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);
bool destroy_score_context(s32 ctx_id);

struct score_transaction_ctx
{
	score_transaction_ctx(s32 score_context_id);

	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 32;

	s32 score_context_id      = 0;
};
s32 create_score_transaction_context(s32 score_context_id);
bool destroy_score_transaction_context(s32 ctx_id);

// Match2 related
struct match2_ctx
{
	match2_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);

	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 255;

	SceNpCommunicationId communicationId{};
	SceNpCommunicationPassphrase passphrase{};

	vm::ptr<SceNpMatching2ContextCallback> context_callback{};
	vm::ptr<void> context_callback_param{};

	SceNpMatching2RequestOptParam default_match2_optparam{};
};
u16 create_match2_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);
bool check_match2_context(u16 ctx_id);
std::shared_ptr<match2_ctx> get_match2_context(u16 ctx_id);
bool destroy_match2_context(u16 ctx_id);

struct lookup_title_ctx
{
	lookup_title_ctx(vm::cptr<SceNpCommunicationId> communicationId);

	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 32;

	SceNpCommunicationId communicationId{};
	SceNpCommunicationPassphrase passphrase{};
};
s32 create_lookup_title_context(vm::cptr<SceNpCommunicationId> communicationId);
bool destroy_lookup_title_context(s32 ctx_id);

struct lookup_transaction_ctx
{
	lookup_transaction_ctx(s32 lt_ctx);

	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 32;

	s32 lt_ctx = 0;
};
s32 create_lookup_transaction_context(s32 lt_ctx);
bool destroy_lookup_transaction_context(s32 ctx_id);

struct commerce2_ctx
{
	commerce2_ctx(u32 version, vm::cptr<SceNpId> npid, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg);

	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 32;

	u32 version{};
	SceNpId npid{};
	vm::ptr<SceNpCommerce2Handler> context_callback{};
	vm::ptr<void> context_callback_param{};
};
s32 create_commerce2_context(u32 version, vm::cptr<SceNpId> npid, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg);
std::shared_ptr<commerce2_ctx> get_commerce2_context(u16 ctx_id);
bool destroy_commerce2_context(s32 ctx_id);

struct signaling_ctx
{
	signaling_ctx(vm::ptr<SceNpId> npid, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg);

	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 32;

	SceNpId npid{};
	vm::ptr<SceNpSignalingHandler> handler{};
	vm::ptr<void> arg{};
};
s32 create_signaling_context(vm::ptr<SceNpId> npid, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg);
bool destroy_signaling_context(s32 ctx_id);
