#pragma once

#include <optional>
#include <condition_variable>
#include <thread>
#include <variant>

#include "Utilities/mutex.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/sceNpCommerce2.h"
#include "Emu/Cell/Modules/sceNpTus.h"
#include "Utilities/Thread.h"

// Used By Score and Tus
struct generic_async_transaction_context
{
	~generic_async_transaction_context();

	generic_async_transaction_context(const SceNpCommunicationId& communicationId, const SceNpCommunicationPassphrase& passphrase, u64 timeout);

	std::optional<s32> get_transaction_status();
	void abort_transaction();
	error_code wait_for_completion();
	void set_result_and_wake(error_code err);

	shared_mutex mutex;
	std::condition_variable_any wake_cond, completion_cond;
	std::optional<error_code> result;
	SceNpCommunicationId communicationId;
	SceNpCommunicationPassphrase passphrase;
	u64 timeout;

	std::thread thread;

	u32 idm_id;
};

struct tdata_invalid
{
};

// Score transaction data

struct tdata_get_board_infos
{
	vm::ptr<SceNpScoreBoardInfo> boardInfo;
};

struct tdata_record_score
{
	vm::ptr<SceNpScoreRankNumber> tmpRank;
};

struct tdata_record_score_data
{
	u32 game_data_size = 0;
	std::vector<u8> game_data;
};

struct tdata_get_score_data
{
	vm::ptr<u32> totalSize;
	u32 recvSize = 0;
	vm::ptr<void> score_data;
	u32 game_data_size = 0;
	std::vector<u8> game_data;
};

struct tdata_get_score_generic
{
	vm::ptr<void> rankArray;
	u32 rankArraySize = 0;
	vm::ptr<SceNpScoreComment> commentArray;
	vm::ptr<void> infoArray;
	u32 infoArraySize = 0;
	u32 arrayNum = 0;
	vm::ptr<CellRtcTick> lastSortDate;
	vm::ptr<SceNpScoreRankNumber> totalRecord;
	bool player_rank_data = false;
	bool deprecated = false;
};

// TUS transaction data

struct tdata_tus_get_variables_generic
{
	vm::ptr<SceNpTusVariable> variableArray;
	s32 arrayNum;
};

struct tdata_tus_get_variable_generic
{
	vm::ptr<SceNpTusVariable> outVariable;
};

struct tdata_tus_set_data
{
	u32 tus_data_size;
	std::vector<u8> tus_data;
};

struct tdata_tus_get_data
{
	u32 recvSize = 0;
	vm::ptr<SceNpTusDataStatus> dataStatus;
	vm::ptr<void> data;
	std::vector<u8> tus_data;
};

struct tdata_tus_get_datastatus_generic
{
	vm::ptr<SceNpTusDataStatus> statusArray;
	s32 arrayNum;
};

// TUS related
struct tus_ctx
{
	tus_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);

	static const u32 id_base = 0x7001;
	static const u32 id_step = 1;
	static const u32 id_count = SCE_NP_TUS_MAX_CTX_NUM;
	SAVESTATE_INIT_POS(50);

	shared_mutex mutex;
	u64 timeout = 60'000'000; // 60 seconds
	SceNpCommunicationId communicationId{};
	SceNpCommunicationPassphrase passphrase{};
};
s32 create_tus_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);
bool destroy_tus_context(s32 ctx_id);

struct tus_transaction_ctx : public generic_async_transaction_context
{
	tus_transaction_ctx(const shared_ptr<tus_ctx>& tus);

	static const u32 id_base  = 0x8001;
	static const u32 id_step  = 1;
	static const u32 id_count = SCE_NP_TUS_MAX_CTX_NUM;
	SAVESTATE_INIT_POS(51);

	std::variant<tdata_invalid, tdata_tus_get_variables_generic, tdata_tus_get_variable_generic, tdata_tus_set_data, tdata_tus_get_data, tdata_tus_get_datastatus_generic> tdata;
};

s32 create_tus_transaction_context(const shared_ptr<tus_ctx>& tus);
bool destroy_tus_transaction_context(s32 ctx_id);

// Score related
struct score_ctx
{
	score_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);

	static const u32 id_base  = 0x2001;
	static const u32 id_step  = 1;
	static const u32 id_count = SCE_NP_SCORE_MAX_CTX_NUM;
	SAVESTATE_INIT_POS(25);

	shared_mutex mutex;
	u64 timeout = 60'000'000; // 60 seconds
	SceNpCommunicationId communicationId{};
	SceNpCommunicationPassphrase passphrase{};
	s32 pcId = 0;
};
s32 create_score_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);
bool destroy_score_context(s32 ctx_id);

struct score_transaction_ctx : public generic_async_transaction_context
{
	score_transaction_ctx(const shared_ptr<score_ctx>& score);

	static const u32 id_base  = 0x1001;
	static const u32 id_step  = 1;
	static const u32 id_count = SCE_NP_SCORE_MAX_CTX_NUM;
	SAVESTATE_INIT_POS(26);

	std::variant<tdata_invalid, tdata_get_board_infos, tdata_record_score, tdata_record_score_data, tdata_get_score_data, tdata_get_score_generic> tdata;
	s32 pcId = 0;
};
s32 create_score_transaction_context(const shared_ptr<score_ctx>& score);
bool destroy_score_transaction_context(s32 ctx_id);

// Match2 related
struct match2_ctx
{
	match2_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase, s32 option);

	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 255; // TODO: constant here?
	SAVESTATE_INIT_POS(27);

	atomic_t<u32> started = 0;

	shared_mutex mutex;

	SceNpCommunicationId communicationId{};
	SceNpCommunicationPassphrase passphrase{};
	bool include_onlinename = false, include_avatarurl = false;

	vm::ptr<SceNpMatching2ContextCallback> context_callback{};
	vm::ptr<void> context_callback_param{};

	SceNpMatching2RequestOptParam default_match2_optparam{};

	vm::ptr<SceNpMatching2SignalingCallback> signaling_cb{};
	vm::ptr<void> signaling_cb_arg{};
};
u16 create_match2_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase, s32 option);
bool check_match2_context(u16 ctx_id);
shared_ptr<match2_ctx> get_match2_context(u16 ctx_id);
bool destroy_match2_context(u16 ctx_id);

struct lookup_title_ctx
{
	lookup_title_ctx(vm::cptr<SceNpCommunicationId> communicationId);

	static const u32 id_base  = 0x3001;
	static const u32 id_step  = 1;
	static const u32 id_count = SCE_NP_LOOKUP_MAX_CTX_NUM;
	SAVESTATE_INIT_POS(28);

	SceNpCommunicationId communicationId{};
	SceNpCommunicationPassphrase passphrase{};
};
s32 create_lookup_title_context(vm::cptr<SceNpCommunicationId> communicationId);
bool destroy_lookup_title_context(s32 ctx_id);

struct lookup_transaction_ctx
{
	lookup_transaction_ctx(s32 lt_ctx);

	static const u32 id_base  = 0x4001;
	static const u32 id_step  = 1;
	static const u32 id_count = SCE_NP_LOOKUP_MAX_CTX_NUM;
	SAVESTATE_INIT_POS(29);

	s32 lt_ctx = 0;
};
s32 create_lookup_transaction_context(s32 lt_ctx);
bool destroy_lookup_transaction_context(s32 ctx_id);

struct commerce2_ctx
{
	commerce2_ctx(u32 version, vm::cptr<SceNpId> npid, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg);

	static const u32 id_base  = 0x5001;
	static const u32 id_step  = 1;
	static const u32 id_count = SCE_NP_COMMERCE2_CTX_MAX;
	SAVESTATE_INIT_POS(30);

	u32 version{};
	SceNpId npid{};
	vm::ptr<SceNpCommerce2Handler> context_callback{};
	vm::ptr<void> context_callback_param{};
};
s32 create_commerce2_context(u32 version, vm::cptr<SceNpId> npid, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg);
shared_ptr<commerce2_ctx> get_commerce2_context(u16 ctx_id);
bool destroy_commerce2_context(u32 ctx_id);

struct signaling_ctx
{
	signaling_ctx(vm::ptr<SceNpId> npid, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg);

	static const u32 id_base  = 0x6001;
	static const u32 id_step  = 1;
	static const u32 id_count = SCE_NP_SIGNALING_CTX_MAX;
	SAVESTATE_INIT_POS(31);

	shared_mutex mutex;

	SceNpId npid{};
	vm::ptr<SceNpSignalingHandler> handler{};
	vm::ptr<void> arg{};
	vm::ptr<SceNpSignalingHandler> ext_handler{};
	vm::ptr<void> ext_arg{};
};
s32 create_signaling_context(vm::ptr<SceNpId> npid, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg);
shared_ptr<signaling_ctx> get_signaling_context(u32 ctx_id);
bool destroy_signaling_context(u32 ctx_id);

struct matching_ctx
{
	matching_ctx(vm::ptr<SceNpId> npid, vm::ptr<SceNpMatchingHandler> handler, vm::ptr<void> arg);

	void queue_callback(u32 req_id, s32 event, s32 error_code) const;
	void queue_gui_callback(s32 event, s32 error_code) const;

	static const u32 id_base = 0x9001;
	static const u32 id_step = 1;
	static const u32 id_count = 1;
	SAVESTATE_INIT_POS(32);

	SceNpId npid{};
	vm::ptr<SceNpMatchingHandler> handler{};
	vm::ptr<void> arg{};

	atomic_t<u32> busy = 0;
	u32 ctx_id = 0;
	vm::ptr<SceNpMatchingGUIHandler> gui_handler{};
	vm::ptr<void> gui_arg{};

	// Used by QuickMatchGUI
	u64 timeout = 0;
	std::unique_ptr<named_thread<std::function<void(SceNpRoomId)>>> thread;
	atomic_t<u32> wakey = 0;

	// To keep track of which callback to use for sceNpMatchingGetRoomListWithoutGUI / sceNpMatchingGetRoomListGUI / sceNpMatchingGetRoomListLimitGUI
	atomic_t<bool> get_room_limit_version = false;
};
s32 create_matching_context(vm::ptr<SceNpId> npid, vm::ptr<SceNpMatchingHandler> handler, vm::ptr<void> arg);
shared_ptr<matching_ctx> get_matching_context(u32 ctx_id);
bool destroy_matching_context(u32 ctx_id);
