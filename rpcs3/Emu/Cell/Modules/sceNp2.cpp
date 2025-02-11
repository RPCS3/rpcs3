#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "sceNp.h"
#include "sceNp2.h"
#include "Emu/NP/np_handler.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/NP/signaling_handler.h"
#include "cellSysutil.h"

LOG_CHANNEL(sceNp2);

template <>
void fmt_class_string<SceNpMatching2Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error) {
		switch (error)
		{
			STR_CASE(SCE_NP_MATCHING2_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CONTEXT_MAX);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_EXISTS);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_SERVER_NOT_FOUND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_SERVER_ID);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_WORLD_ID);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_LOBBY_ID);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_ROOM_ID);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_MEMBER_ID);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_ATTRIBUTE_ID);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_CASTTYPE);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_SORT_METHOD);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_MAX_SLOT);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_MATCHING_SPACE);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_BLOCK_KICK_FLAG);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_MESSAGE_TARGET);
			STR_CASE(SCE_NP_MATCHING2_ERROR_RANGE_FILTER_MAX);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INSUFFICIENT_BUFFER);
			STR_CASE(SCE_NP_MATCHING2_ERROR_DESTINATION_DISAPPEARED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_REQUEST_TIMEOUT);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_ALIGNMENT);
			STR_CASE(SCE_NP_MATCHING2_ERROR_REQUEST_CB_QUEUE_OVERFLOW);
			STR_CASE(SCE_NP_MATCHING2_ERROR_EVENT_CB_QUEUE_OVERFLOW);
			STR_CASE(SCE_NP_MATCHING2_ERROR_MSG_CB_QUEUE_OVERFLOW);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CONNECTION_CLOSED_BY_SERVER);
			STR_CASE(SCE_NP_MATCHING2_ERROR_SSL_VERIFY_FAILED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_SSL_HANDSHAKE);
			STR_CASE(SCE_NP_MATCHING2_ERROR_SSL_SEND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_SSL_RECV);
			STR_CASE(SCE_NP_MATCHING2_ERROR_JOINED_SESSION_MAX);
			STR_CASE(SCE_NP_MATCHING2_ERROR_ALREADY_JOINED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_SESSION_TYPE);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CLAN_LOBBY_NOT_EXIST);
			STR_CASE(SCE_NP_MATCHING2_ERROR_NP_SIGNED_OUT);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CONTEXT_UNAVAILABLE);
			STR_CASE(SCE_NP_MATCHING2_ERROR_SERVER_NOT_AVAILABLE);
			STR_CASE(SCE_NP_MATCHING2_ERROR_NOT_ALLOWED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_ABORTED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_REQUEST_NOT_FOUND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_SESSION_DESTROYED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CONTEXT_STOPPED);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_REQUEST_PARAMETER);
			STR_CASE(SCE_NP_MATCHING2_ERROR_NOT_NP_SIGN_IN);
			STR_CASE(SCE_NP_MATCHING2_ERROR_ROOM_NOT_FOUND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_ROOM_MEMBER_NOT_FOUND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_LOBBY_NOT_FOUND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_LOBBY_MEMBER_NOT_FOUND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_EVENT_DATA_NOT_FOUND);
			STR_CASE(SCE_NP_MATCHING2_ERROR_KEEPALIVE_TIMEOUT);
			STR_CASE(SCE_NP_MATCHING2_ERROR_TIMEOUT_TOO_SHORT);
			STR_CASE(SCE_NP_MATCHING2_ERROR_TIMEDOUT);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CREATE_HEAP);
			STR_CASE(SCE_NP_MATCHING2_ERROR_INVALID_ATTRIBUTE_SIZE);
			STR_CASE(SCE_NP_MATCHING2_ERROR_CANNOT_ABORT);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_NO_DNS_SERVER);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_INVALID_PACKET);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_TIMEOUT);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_NO_RECORD);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_RES_PACKET_FORMAT);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_RES_SERVER_FAILURE);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_NO_HOST);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_RES_NOT_IMPLEMENTED);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_RES_SERVER_REFUSED);
			STR_CASE(SCE_NP_MATCHING2_RESOLVER_ERROR_RESP_TRUNCATED);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_SERVICE_UNAVAILABLE);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_BUSY);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_END_OF_SERVICE);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_INTERNAL_SERVER_ERROR);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_PLAYER_BANNED);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_FORBIDDEN);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_BLOCKED);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_UNSUPPORTED_NP_ENV);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_INVALID_TICKET);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_INVALID_SIGNATURE);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_EXPIRED_TICKET);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_ENTITLEMENT_REQUIRED);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_CONTEXT);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_CLOSED);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_TITLE);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_WORLD);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_LOBBY);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_LOBBY_INSTANCE);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM_INSTANCE);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_PASSWORD_MISMATCH);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_LOBBY_FULL);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_ROOM_FULL);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_GROUP_FULL);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_USER);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_TITLE_PASSPHRASE_MISMATCH);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_DUPLICATE_LOBBY);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_DUPLICATE_ROOM);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_JOIN_GROUP_LABEL);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_GROUP);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NO_PASSWORD);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_MAX_OVER_SLOT_GROUP);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_MAX_OVER_PASSWORD_MASK);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_DUPLICATE_GROUP_LABEL);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_REQUEST_OVERFLOW);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_ALREADY_JOINED);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_NAT_TYPE_MISMATCH);
			STR_CASE(SCE_NP_MATCHING2_SERVER_ERROR_ROOM_INCONSISTENCY);
			// STR_CASE(SCE_NP_MATCHING2_NET_ERRNO_BASE);
			// STR_CASE(SCE_NP_MATCHING2_NET_H_ERRNO_BASE);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<SceNpOauthError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error) {
		switch (error)
		{
			STR_CASE(SCE_NP_OAUTH_ERROR_UNKNOWN);
			STR_CASE(SCE_NP_OAUTH_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_OAUTH_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_OAUTH_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_OAUTH_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_OAUTH_ERROR_OUT_OF_BUFFER);
			STR_CASE(SCE_NP_OAUTH_ERROR_BAD_RESPONSE);
			STR_CASE(SCE_NP_OAUTH_ERROR_ABORTED);
			STR_CASE(SCE_NP_OAUTH_ERROR_SIGNED_OUT);
			STR_CASE(SCE_NP_OAUTH_ERROR_REQUEST_NOT_FOUND);
			STR_CASE(SCE_NP_OAUTH_ERROR_SSL_ERR_CN_CHECK);
			STR_CASE(SCE_NP_OAUTH_ERROR_SSL_ERR_UNKNOWN_CA);
			STR_CASE(SCE_NP_OAUTH_ERROR_SSL_ERR_NOT_AFTER_CHECK);
			STR_CASE(SCE_NP_OAUTH_ERROR_SSL_ERR_NOT_BEFORE_CHECK);
			STR_CASE(SCE_NP_OAUTH_ERROR_SSL_ERR_INVALID_CERT);
			STR_CASE(SCE_NP_OAUTH_ERROR_SSL_ERR_INTERNAL);
			STR_CASE(SCE_NP_OAUTH_ERROR_REQUEST_MAX);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_BANNED_CONSOLE);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_INVALID_LOGIN);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_INACTIVE_ACCOUNT);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_SUSPENDED_ACCOUNT);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_SUSPENDED_DEVICE);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_PASSWORD_EXPIRED);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_TOSUA_MUST_BE_RE_ACCEPTED);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_TOSUA_MUST_BE_RE_ACCEPTED_FOR_SUBACCOUNT);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_BANNED_ACCOUNT);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_SERVICE_END);
			STR_CASE(SCE_NP_OAUTH_SERVER_ERROR_SERVICE_UNAVAILABLE);
		}

		return unknown;
	});
}

error_code sceNpMatching2Init2(u32 stackSize, s32 priority, vm::ptr<SceNpMatching2UtilityInitParam> param);
error_code sceNpMatching2Term(ppu_thread& ppu);
error_code sceNpMatching2Term2();

error_code generic_match2_error_check(const named_thread<np::np_handler>& nph, SceNpMatching2ContextId ctxId, vm::cptr<void> reqParam)
{
	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!reqParam)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	if (!ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
	}

	if (!check_match2_context(ctxId))
	{
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNp2Init(u32 poolsize, vm::ptr<void> poolptr)
{
	sceNp2.warning("sceNp2Init(poolsize=0x%x, poolptr=*0x%x)", poolsize, poolptr);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	{
		std::lock_guard lock(nph.mutex_status);
		if (nph.is_NP2_init)
		{
			return SCE_NP_ERROR_ALREADY_INITIALIZED;
		}
	}

	const u32 result = std::bit_cast<u32>(sceNpInit(poolsize, poolptr));

	if (result && result != SCE_NP_ERROR_ALREADY_INITIALIZED)
	{
		return result;
	}

	nph.is_NP2_init = true;

	return CELL_OK;
}

error_code sceNpMatching2Init(u32 stackSize, s32 priority)
{
	sceNp2.todo("sceNpMatching2Init(stackSize=0x%x, priority=%d)", stackSize, priority);
	return sceNpMatching2Init2(stackSize, priority, vm::null); // > SDK 2.4.0
}

error_code sceNpMatching2Init2(u32 stackSize, s32 priority, vm::ptr<SceNpMatching2UtilityInitParam> param)
{
	sceNp2.warning("sceNpMatching2Init2(stackSize=0x%x, priority=%d, param=*0x%x)", stackSize, priority, param);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
	}

	// TODO:
	// 1. Create an internal thread
	// 2. Create heap area to be used by the NP matching 2 utility
	// 3. Set maximum lengths for the event data queues in the system

	nph.is_NP2_Match2_init = true;

	return CELL_OK;
}

error_code sceNp2Term(ppu_thread& ppu)
{
	sceNp2.warning("sceNp2Term()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	{
		std::lock_guard lock(nph.mutex_status);
		if (!nph.is_NP2_init)
		{
			return SCE_NP_ERROR_NOT_INITIALIZED;
		}
	}

	// TODO: does this return on error_code ?
	sceNpMatching2Term(ppu);
	//	cellSysutilUnregisterCallbackDispatcher();
	sceNpTerm();

	nph.is_NP2_init = false;

	return CELL_OK;
}

error_code sceNpMatching2Term(ppu_thread&)
{
	sceNp2.warning("sceNpMatching2Term()");
	return sceNpMatching2Term2(); // > SDK 2.4.0
}

error_code sceNpMatching2Term2()
{
	sceNp2.warning("sceNpMatching2Term2()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	{
		std::lock_guard lock(nph.mutex_status);
		if (!nph.is_NP2_init)
		{
			return SCE_NP_ERROR_NOT_INITIALIZED;
		}

		if (!nph.is_NP2_Match2_init)
		{
			return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
		}

		nph.is_NP2_Match2_init = false;
	}

	idm::clear<match2_ctx>();

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh.clear_match2_ctx();

	return CELL_OK;
}

error_code sceNpMatching2DestroyContext(SceNpMatching2ContextId ctxId)
{
	sceNp2.warning("sceNpMatching2DestroyContext(ctxId=%d)", ctxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!destroy_match2_context(ctxId))
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh.remove_match2_ctx(ctxId);

	return CELL_OK;
}

error_code sceNpMatching2LeaveLobby(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2LeaveLobbyRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2LeaveLobby(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2RegisterLobbyMessageCallback(SceNpMatching2ContextId ctxId, vm::ptr<SceNpMatching2LobbyMessageCallback> cbFunc, vm::ptr<void> cbFuncArg)
{
	sceNp2.todo("sceNpMatching2RegisterLobbyMessageCallback(ctxId=%d, cbFunc=*0x%x, cbFuncArg=*0x%x)", ctxId, cbFunc, cbFuncArg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetWorldInfoList(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetWorldInfoListRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2GetWorldInfoList(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	if (reqParam->serverId == 0)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_SERVER_ID;
	}

	const u32 request_id = nph.get_world_list(ctxId, optParam, reqParam->serverId);

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2RegisterLobbyEventCallback(SceNpMatching2ContextId ctxId, vm::ptr<SceNpMatching2LobbyEventCallback> cbFunc, vm::ptr<void> cbFuncArg)
{
	sceNp2.todo("sceNpMatching2RegisterLobbyEventCallback(ctxId=%d, cbFunc=*0x%x, cbFuncArg=*0x%x)", ctxId, cbFunc, cbFuncArg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetLobbyMemberDataInternalList(SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetLobbyMemberDataInternalListRequest> reqParam,
    vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2GetLobbyMemberDataInternalList(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2SearchRoom(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SearchRoomRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2SearchRoom(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.search_room(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2SignalingGetConnectionStatus(
    SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, SceNpMatching2RoomMemberId memberId, vm::ptr<s32> connStatus, vm::ptr<np_in_addr> peerAddr, vm::ptr<np_in_port_t> peerPort)
{
	sceNp2.warning("sceNpMatching2SignalingGetConnectionStatus(ctxId=%d, roomId=%d, memberId=%d, connStatus=*0x%x, peerAddr=*0x%x, peerPort=*0x%x)", ctxId, roomId, memberId, connStatus, peerAddr, peerPort);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!connStatus)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	auto [res, npid] = nph.local_get_npid(roomId, memberId);

	if (res)
	{
		*connStatus = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
		return res;
	}

	if (np::is_same_npid(nph.get_npid(), *npid))
	{
		*connStatus = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
		return SCE_NP_SIGNALING_ERROR_OWN_NP_ID;
	}

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();

	auto conn_id = sigh.get_conn_id_from_npid(*npid);
	if (!conn_id)
	{
		*connStatus = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
		return SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
	}

	const auto si = sigh.get_sig_infos(*conn_id);

	if (!si)
	{
		*connStatus = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
		return SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
	}

	*connStatus = si->conn_status;

	if (peerAddr)
	{
		(*peerAddr).np_s_addr = si->addr; // infos.addr is already BE
	}

	if (peerPort)
	{
		*peerPort = si->port;
	}

	return CELL_OK;
}

error_code sceNpMatching2SetUserInfo(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SetUserInfoRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2SetUserInfo(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.set_userinfo(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetClanLobbyId(SceNpMatching2ContextId ctxId, SceNpClanId clanId, vm::ptr<SceNpMatching2LobbyId> lobbyId)
{
	sceNp2.todo("sceNpMatching2GetClanLobbyId(ctxId=%d, clanId=%d, lobbyId=*0x%x)", ctxId, clanId, lobbyId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetLobbyMemberDataInternal(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetLobbyMemberDataInternalRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2GetLobbyMemberDataInternal(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2ContextStart(SceNpMatching2ContextId ctxId)
{
	sceNp2.warning("sceNpMatching2ContextStart(ctxId=%d)", ctxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	const auto ctx = get_match2_context(ctxId);
	if (!ctx)
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;

	if (!ctx->started.compare_and_swap_test(0, 1))
		return SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED;

	if (ctx->context_callback)
	{
		sysutil_register_cb([=, context_callback = ctx->context_callback, context_callback_param = ctx->context_callback_param](ppu_thread& cb_ppu) -> s32
		{
			context_callback(cb_ppu, ctxId, SCE_NP_MATCHING2_CONTEXT_EVENT_Start, SCE_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION, 0, context_callback_param);
			return 0;
		});
	}

	return CELL_OK;
}

error_code sceNpMatching2CreateServerContext(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2CreateServerContextRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2CreateServerContext(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	if (reqParam->serverId == 0)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_SERVER_ID;
	}

	const u32 request_id = nph.create_server_context(ctxId, optParam, reqParam->serverId);

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetMemoryInfo(vm::ptr<SceNpMatching2MemoryInfo> memInfo) // TODO
{
	sceNp2.todo("sceNpMatching2GetMemoryInfo(memInfo=*0x%x)", memInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2LeaveRoom(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2LeaveRoomRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2LeaveRoom(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.leave_room(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2SetRoomDataExternal(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SetRoomDataExternalRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2SetRoomDataExternal(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.set_roomdata_external(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2SignalingGetConnectionInfo(
    SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, SceNpMatching2RoomMemberId memberId, s32 code, vm::ptr<SceNpSignalingConnectionInfo> connInfo)
{
	sceNp2.warning("sceNpMatching2SignalingGetConnectionInfo(ctxId=%d, roomId=%d, memberId=%d, code=%d, connInfo=*0x%x)", ctxId, roomId, memberId, code, connInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!connInfo)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	auto [res, npid] = nph.local_get_npid(roomId, memberId);

	if (res)
		return res;

	if (np::is_same_npid(nph.get_npid(), *npid))
		return SCE_NP_SIGNALING_ERROR_OWN_NP_ID;

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();

	auto conn_id = sigh.get_conn_id_from_npid(*npid);
	if (!conn_id)
		return SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND;

	const auto si = sigh.get_sig_infos(*conn_id);

	if (!si)
		return SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND;

	switch (code)
	{
		case SCE_NP_SIGNALING_CONN_INFO_RTT:
		{
			connInfo->rtt = si->rtt;
			sceNp2.warning("Returning a RTT of %d microseconds", connInfo->rtt);
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_BANDWIDTH:
		{
			connInfo->bandwidth = 100'000'000; // 100 MBPS HACK
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_PEER_NPID:
		{
			connInfo->npId = si->npid;
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_PEER_ADDRESS:
		{
			connInfo->address.port = std::bit_cast<u16, be_t<u16>>(si->port);
			connInfo->address.addr.np_s_addr = si->addr;
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_MAPPED_ADDRESS:
		{
			connInfo->address.port = std::bit_cast<u16, be_t<u16>>(si->mapped_port);
			connInfo->address.addr.np_s_addr = si->mapped_addr;
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_PACKET_LOSS:
		{
			connInfo->packet_loss = 0; // HACK
			break;
		}
		default:
		{
			return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
		}
	}

	return CELL_OK;
}

error_code sceNpMatching2SendRoomMessage(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SendRoomMessageRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2SendRoomMessage(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.send_room_message(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2JoinLobby(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2JoinLobbyRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2JoinLobby(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetRoomMemberDataExternalList(SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetRoomMemberDataExternalListRequest> reqParam,
    vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2GetRoomMemberDataExternalList(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2AbortRequest(SceNpMatching2ContextId ctxId, SceNpMatching2RequestId reqId)
{
	sceNp2.warning("sceNpMatching2AbortRequest(ctxId=%d, reqId=%d)", ctxId, reqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
	}

	if (!check_match2_context(ctxId))
	{
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;
	}

	if (!nph.abort_request(reqId))
		return SCE_NP_MATCHING2_ERROR_REQUEST_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpMatching2GetServerInfo(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetServerInfoRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2GetServerInfo(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.get_server_status(ctxId, optParam, reqParam->serverId);

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetEventData(SceNpMatching2ContextId ctxId, SceNpMatching2EventKey eventKey, vm::ptr<void> buf, u32 bufLen)
{
	sceNp2.notice("sceNpMatching2GetEventData(ctxId=%d, eventKey=%d, buf=*0x%x, bufLen=%d)", ctxId, eventKey, buf, bufLen);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!buf)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	return not_an_error(nph.get_match2_event(eventKey, buf.addr(), bufLen));
}

error_code sceNpMatching2GetRoomSlotInfoLocal(SceNpMatching2ContextId ctxId, const SceNpMatching2RoomId roomId, vm::ptr<SceNpMatching2RoomSlotInfo> roomSlotInfo)
{
	sceNp2.notice("sceNpMatching2GetRoomSlotInfoLocal(ctxId=%d, roomId=%d, roomSlotInfo=*0x%x)", ctxId, roomId, roomSlotInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!roomSlotInfo)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	if (!ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
	}

	if (!roomId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ROOM_ID;
	}

	if (!check_match2_context(ctxId))
	{
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;
	}

	const auto [error, slots] = nph.local_get_room_slots(roomId);

	if (error)
	{
		return error;
	}

	memcpy(roomSlotInfo.get_ptr(), &slots.value(), sizeof(SceNpMatching2RoomSlotInfo));

	return CELL_OK;
}

error_code sceNpMatching2SendLobbyChatMessage(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SendLobbyChatMessageRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2SendLobbyChatMessage(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2AbortContextStart(SceNpMatching2ContextId ctxId)
{
	sceNp2.todo("sceNpMatching2AbortContextStart(ctxId=%d)", ctxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetRoomMemberIdListLocal(SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, s32 sortMethod, vm::ptr<SceNpMatching2RoomMemberId> memberId, u32 memberIdNum)
{
	sceNp2.notice("sceNpMatching2GetRoomMemberIdListLocal(ctxId=%d, roomId=%d, sortMethod=%d, memberId=*0x%x, memberIdNum=%d)", ctxId, roomId, sortMethod, memberId, memberIdNum);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
	}

	if (!roomId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ROOM_ID;
	}

	if (sortMethod != SCE_NP_MATCHING2_SORT_METHOD_JOIN_DATE && sortMethod != SCE_NP_MATCHING2_SORT_METHOD_SLOT_NUMBER)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_SORT_METHOD;
	}

	if (!check_match2_context(ctxId))
	{
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;
	}

	const auto [error, vec_memberids] = nph.local_get_room_memberids(roomId, sortMethod);

	if (error)
	{
		return error;
	}

	u32 num_members = std::min(memberIdNum, static_cast<u32>(vec_memberids.size()));

	if (memberId)
	{
		for (u32 i = 0; i < num_members; i++)
		{
			memberId[i] = vec_memberids[i];
		}
	}

	return not_an_error(num_members);
}

error_code sceNpMatching2JoinRoom(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2JoinRoomRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2JoinRoom(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.join_room(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetRoomMemberDataInternalLocal(SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, SceNpMatching2RoomMemberId memberId, vm::cptr<SceNpMatching2AttributeId> attrId,
	u32 attrIdNum, vm::ptr<SceNpMatching2RoomMemberDataInternal> member, vm::ptr<char> buf, u32 bufLen)
{
	sceNp2.warning("sceNpMatching2GetRoomMemberDataInternalLocal(ctxId=%d, roomId=%d, memberId=%d, attrId=*0x%x, attrIdNum=%d, member=*0x%x, buf=*0x%x, bufLen=%d)", ctxId, roomId, memberId, attrId,
		attrIdNum, member, buf, bufLen);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
	}

	if (!roomId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ROOM_ID;
	}

	if (!memberId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_MEMBER_ID;
	}

	std::vector<SceNpMatching2AttributeId> binattrs_list;
	for (u32 i = 0; i < attrIdNum; i++)
	{
		if (!attrId || attrId[i] < SCE_NP_MATCHING2_ROOMMEMBER_BIN_ATTR_INTERNAL_1_ID || attrId[i] >= SCE_NP_MATCHING2_USER_BIN_ATTR_1_ID)
		{
			return SCE_NP_MATCHING2_ERROR_INVALID_ATTRIBUTE_ID;
		}
		binattrs_list.push_back(attrId[i]);
	}

	if (!check_match2_context(ctxId))
	{
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;
	}

	return nph.local_get_room_member_data(roomId, memberId, binattrs_list, member ? member.get_ptr() : nullptr, buf.addr(), bufLen, ctxId);
}

error_code sceNpMatching2GetCbQueueInfo(SceNpMatching2ContextId ctxId, vm::ptr<SceNpMatching2CbQueueInfo> queueInfo)
{
	sceNp2.todo("sceNpMatching2GetCbQueueInfo(ctxId=%d, queueInfo=*0x%x)", ctxId, queueInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2KickoutRoomMember(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2KickoutRoomMemberRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2KickoutRoomMember(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2ContextStartAsync(SceNpMatching2ContextId ctxId, u32 timeout)
{
	sceNp2.warning("sceNpMatching2ContextStartAsync(ctxId=%d, timeout=%d)", ctxId, timeout);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
	}

	const auto ctx = get_match2_context(ctxId);
	if (!ctx)
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;

	if (!ctx->started.compare_and_swap_test(0, 1))
		return SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED;

	if (ctx->context_callback)
	{
		sysutil_register_cb([=, context_callback = ctx->context_callback, context_callback_param = ctx->context_callback_param](ppu_thread& cb_ppu) -> s32
		{
			context_callback(cb_ppu, ctxId, SCE_NP_MATCHING2_CONTEXT_EVENT_Start, SCE_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION, 0, ctx->context_callback_param);
			return 0;
		});
	}

	return CELL_OK;
}

error_code sceNpMatching2SetSignalingOptParam(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SetSignalingOptParamRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2SetSignalingOptParam(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2RegisterContextCallback(SceNpMatching2ContextId ctxId, vm::ptr<SceNpMatching2ContextCallback> cbFunc, vm::ptr<void> cbFuncArg)
{
	sceNp2.warning("sceNpMatching2RegisterContextCallback(ctxId=%d, cbFunc=*0x%x, cbFuncArg=*0x%x)", ctxId, cbFunc, cbFuncArg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
	}

	const auto ctx = get_match2_context(ctxId);
	if (!ctx)
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;

	ctx->context_callback = cbFunc;
	ctx->context_callback_param = cbFuncArg;

	return CELL_OK;
}

error_code sceNpMatching2SendRoomChatMessage(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SendRoomChatMessageRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2SendRoomChatMessage(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2SetRoomDataInternal(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SetRoomDataInternalRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2SetRoomDataInternal(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.set_roomdata_internal(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetRoomDataInternal(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetRoomDataInternalRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2GetRoomDataInternal(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.get_roomdata_internal(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2SignalingGetPingInfo(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SignalingGetPingInfoRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2SignalingGetPingInfo(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.get_ping_info(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetServerIdListLocal(SceNpMatching2ContextId ctxId, vm::ptr<SceNpMatching2ServerId> serverId, u32 serverIdNum)
{
	sceNp2.notice("sceNpMatching2GetServerIdListLocal(ctxId=%d, serverId=*0x%x, serverIdNum=%d)", ctxId, serverId, serverIdNum);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!check_match2_context(ctxId))
	{
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;
	}

	const auto slist = nph.get_match2_server_list(ctxId);

	u32 num_servs = std::min(static_cast<u32>(slist.size()), serverIdNum);

	if (serverId)
	{
		for (u32 i = 0; i < num_servs; i++)
		{
			serverId[i] = slist[i];
		}
	}

	return not_an_error(static_cast<s32>(slist.size()));
}

error_code sceNpUtilBuildCdnUrl(vm::cptr<char> url, vm::ptr<char> buf, u32 bufSize, vm::ptr<u32> required, vm::ptr<void> option)
{
	sceNp2.todo("sceNpUtilBuildCdnUrl(url=%s, buf=*0x%x, bufSize=%d, required=*0x%x, option=*0x%x)", url, buf, bufSize, required, option);

	if (!url || option) // option check at least until fw 4.71
	{
		// TODO: check url for '?'
		return SCE_NP_UTIL_ERROR_INVALID_ARGUMENT;
	}

	//if (offline)
	//{
	//	return SCE_NP_ERROR_OFFLINE;
	//}

	return CELL_OK;
}

error_code sceNpMatching2GrantRoomOwner(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GrantRoomOwnerRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2GrantRoomOwner(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2CreateContext(
    vm::cptr<SceNpId> npId, vm::cptr<SceNpCommunicationId> commId, vm::cptr<SceNpCommunicationPassphrase> passPhrase, vm::ptr<SceNpMatching2ContextId> ctxId, s32 option)
{
	sceNp2.warning("sceNpMatching2CreateContext(npId=*0x%x, commId=*0x%x(%s), passPhrase=*0x%x, ctxId=*0x%x, option=%d)", npId, commId, commId ? commId->data : "", passPhrase, ctxId, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !commId || !passPhrase || !ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_NP_SIGN_IN;
	}

	*ctxId = create_match2_context(commId, passPhrase, option);

	return CELL_OK;
}

error_code sceNpMatching2GetSignalingOptParamLocal(SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, vm::ptr<SceNpMatching2SignalingOptParam> signalingOptParam)
{
	sceNp2.todo("sceNpMatching2GetSignalingOptParamLocal(ctxId=%d, roomId=%d, signalingOptParam=*0x%x)", ctxId, roomId, signalingOptParam);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2RegisterSignalingCallback(SceNpMatching2ContextId ctxId, vm::ptr<SceNpMatching2SignalingCallback> cbFunc, vm::ptr<void> cbFuncArg)
{
	sceNp2.notice("sceNpMatching2RegisterSignalingCallback(ctxId=%d, cbFunc=*0x%x, cbFuncArg=*0x%x)", ctxId, cbFunc, cbFuncArg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	auto ctx = get_match2_context(ctxId);

	if (!ctx)
	{
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;
	}

	std::lock_guard lock(ctx->mutex);
	ctx->signaling_cb = cbFunc;
	ctx->signaling_cb_arg = cbFuncArg;

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh.add_match2_ctx(ctxId);

	return CELL_OK;
}

error_code sceNpMatching2ClearEventData(SceNpMatching2ContextId ctxId, SceNpMatching2EventKey eventKey)
{
	sceNp2.todo("sceNpMatching2ClearEventData(ctxId=%d, eventKey=%d)", ctxId, eventKey);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetUserInfoList(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetUserInfoListRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2GetUserInfoList(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetRoomMemberDataInternal(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetRoomMemberDataInternalRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2GetRoomMemberDataInternal(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.get_roommemberdata_internal(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2SetRoomMemberDataInternal(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SetRoomMemberDataInternalRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2SetRoomMemberDataInternal(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.set_roommemberdata_internal(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2JoinProhibitiveRoom(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2JoinProhibitiveRoomRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2JoinProhibitiveRoom(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	// TODO: add blocked users
	const u32 request_id = nph.join_room(ctxId, optParam, &reqParam->joinParam);

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2SignalingSetCtxOpt(SceNpMatching2ContextId ctxId, s32 optname, s32 optval)
{
	sceNp2.todo("sceNpMatching2SignalingSetCtxOpt(ctxId=%d, optname=%d, optval=%d)", ctxId, optname, optval);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2DeleteServerContext(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2DeleteServerContextRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2DeleteServerContext(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	if (reqParam->serverId == 0)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_SERVER_ID;
	}

	const u32 request_id = nph.delete_server_context(ctxId, optParam, reqParam->serverId);

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2SetDefaultRequestOptParam(SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2RequestOptParam> optParam)
{
	sceNp2.warning("sceNpMatching2SetDefaultRequestOptParam(ctxId=%d, optParam=*0x%x)", ctxId, optParam);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!optParam)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	const auto ctx = get_match2_context(ctxId);
	if (!ctx)
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;

	memcpy(&ctx->default_match2_optparam, optParam.get_ptr(), sizeof(SceNpMatching2RequestOptParam));

	return CELL_OK;
}

error_code sceNpMatching2RegisterRoomEventCallback(SceNpMatching2ContextId ctxId, vm::ptr<SceNpMatching2RoomEventCallback> cbFunc, vm::ptr<void> cbFuncArg)
{
	sceNp2.warning("sceNpMatching2RegisterRoomEventCallback(ctxId=%d, cbFunc=*0x%x, cbFuncArg=*0x%x)", ctxId, cbFunc, cbFuncArg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	nph.room_event_cb     = cbFunc;
	nph.room_event_cb_ctx = ctxId;
	nph.room_event_cb_arg = cbFuncArg;

	return CELL_OK;
}

error_code sceNpMatching2GetRoomPasswordLocal(SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, vm::ptr<b8> withPassword, vm::ptr<SceNpMatching2SessionPassword> roomPassword)
{
	sceNp2.notice("sceNpMatching2GetRoomPasswordLocal(ctxId=%d, roomId=%d, withPassword=*0x%x, roomPassword=*0x%x)", ctxId, roomId, withPassword, roomPassword);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!ctxId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
	}

	if (!roomId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ROOM_ID;
	}

	if (!check_match2_context(ctxId))
	{
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND;
	}

	const auto [error, password] = nph.local_get_room_password(roomId);

	if (error)
	{
		return error;
	}

	if (password)
	{
		if (withPassword) *withPassword = true;
		if (roomPassword)
		{
			std::memcpy(roomPassword.get_ptr(), &*password, sizeof(SceNpMatching2SessionPassword));
		}
	}
	else
	{
		if (withPassword) *withPassword = false;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetRoomDataExternalList(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetRoomDataExternalListRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2GetRoomDataExternalList(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.get_roomdata_external_list(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2CreateJoinRoom(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2CreateJoinRoomRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.warning("sceNpMatching2CreateJoinRoom(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.create_join_room(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2SignalingGetCtxOpt(SceNpMatching2ContextId ctxId, s32 optname, vm::ptr<s32> optval)
{
	sceNp2.todo("sceNpMatching2SignalingGetCtxOpt(ctxId=%d, optname=%d, optval=*0x%x)", ctxId, optname, optval);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetLobbyInfoList(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2GetLobbyInfoListRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2GetLobbyInfoList(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	const u32 request_id = nph.get_lobby_info_list(ctxId, optParam, reqParam.get_ptr());

	if (assignedReqId)
	{
		*assignedReqId = request_id;
	}

	return CELL_OK;
}

error_code sceNpMatching2GetLobbyMemberIdListLocal(
    SceNpMatching2ContextId ctxId, SceNpMatching2LobbyId lobbyId, vm::ptr<SceNpMatching2LobbyMemberId> memberId, u32 memberIdNum, vm::ptr<SceNpMatching2LobbyMemberId> me)
{
	sceNp2.todo("sceNpMatching2GetLobbyMemberIdListLocal(ctxId=%d, lobbyId=%d, memberId=*0x%x, memberIdNum=%d, me=*0x%x)", ctxId, lobbyId, memberId, memberIdNum, me);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2SendLobbyInvitation(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SendLobbyInvitationRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2SendLobbyInvitation(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2ContextStop(SceNpMatching2ContextId ctxId)
{
	sceNp2.warning("sceNpMatching2ContextStop(ctxId=%d)", ctxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	const auto ctx = get_match2_context(ctxId);

	if (!ctx)
		return SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;

	if (!ctx->started.compare_and_swap_test(1, 0))
		return SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED;
	
	if (ctx->context_callback)
	{
		sysutil_register_cb([=, context_callback = ctx->context_callback, context_callback_param = ctx->context_callback_param](ppu_thread& cb_ppu) -> s32
		{
			context_callback(cb_ppu, ctxId, SCE_NP_MATCHING2_CONTEXT_EVENT_Stop, SCE_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION, 0, context_callback_param);
			return 0;
		});
	}

	return CELL_OK;
}

error_code sceNpMatching2SetLobbyMemberDataInternal(
    SceNpMatching2ContextId ctxId, vm::cptr<SceNpMatching2SetLobbyMemberDataInternalRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, vm::ptr<SceNpMatching2RequestId> assignedReqId)
{
	sceNp2.todo("sceNpMatching2SetLobbyMemberDataInternal(ctxId=%d, reqParam=*0x%x, optParam=*0x%x, assignedReqId=*0x%x)", ctxId, reqParam, optParam, assignedReqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (auto res = generic_match2_error_check(nph, ctxId, reqParam); res != CELL_OK)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpMatching2RegisterRoomMessageCallback(SceNpMatching2ContextId ctxId, vm::ptr<SceNpMatching2RoomMessageCallback> cbFunc, vm::ptr<void> cbFuncArg)
{
	sceNp2.warning("sceNpMatching2RegisterRoomMessageCallback(ctxId=%d, cbFunc=*0x%x, cbFuncArg=*0x%x)", ctxId, cbFunc, cbFuncArg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	nph.room_msg_cb     = cbFunc;
	nph.room_msg_cb_ctx = ctxId;
	nph.room_msg_cb_arg = cbFuncArg;

	return CELL_OK;
}

error_code sceNpMatching2SignalingCancelPeerNetInfo(SceNpMatching2ContextId ctxId, SceNpMatching2SignalingRequestId reqId)
{
	sceNp2.todo("sceNpMatching2SignalingCancelPeerNetInfo(ctxId=%d, reqId=%d)", ctxId, reqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatching2SignalingGetLocalNetInfo(vm::ptr<SceNpMatching2SignalingNetInfo> netinfo)
{
	sceNp2.warning("sceNpMatching2SignalingGetLocalNetInfo(netinfo=*0x%x)", netinfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!netinfo || netinfo->size != sizeof(SceNpMatching2SignalingNetInfo))
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	netinfo->localAddr  = nph.get_local_ip_addr();
	netinfo->mappedAddr = nph.get_public_ip_addr();

	// Pure speculation below
	netinfo->natStatus = SCE_NP_SIGNALING_NETINFO_NAT_STATUS_TYPE2;

	return CELL_OK;
}

error_code sceNpMatching2SignalingGetPeerNetInfo(SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, SceNpMatching2RoomMemberId roomMemberId, vm::ptr<SceNpMatching2SignalingRequestId> reqId)
{
	sceNp2.todo("sceNpMatching2SignalingGetPeerNetInfo(ctxId=%d, roomId=%d, roomMemberId=%d, reqId=*0x%x)", ctxId, roomId, roomMemberId, reqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!reqId)
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpMatching2SignalingGetPeerNetInfoResult(SceNpMatching2ContextId ctxId, SceNpMatching2SignalingRequestId reqId, vm::ptr<SceNpMatching2SignalingNetInfo> netinfo)
{
	sceNp2.todo("sceNpMatching2SignalingGetPeerNetInfoResult(ctxId=%d, reqId=%d, netinfo=*0x%x)", ctxId, reqId, netinfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP2_Match2_init)
	{
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	if (!netinfo || netinfo->size != sizeof(SceNpMatching2SignalingNetInfo))
	{
		return SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpAuthOAuthInit()
{
	sceNp2.todo("sceNpAuthOAuthInit()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (nph.is_NP_Auth_init)
	{
		return SCE_NP_OAUTH_ERROR_ALREADY_INITIALIZED;
	}

	nph.is_NP_Auth_init = true;

	return CELL_OK;
}

error_code sceNpAuthOAuthTerm()
{
	sceNp2.todo("sceNpAuthOAuthTerm()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	// TODO: check if this might throw SCE_NP_OAUTH_ERROR_NOT_INITIALIZED
	nph.is_NP_Auth_init = false;

	return CELL_OK;
}

error_code sceNpAuthCreateOAuthRequest()
{
	sceNp2.todo("sceNpAuthCreateOAuthRequest()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Auth_init)
	{
		return SCE_NP_OAUTH_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpAuthDeleteOAuthRequest(SceNpAuthOAuthRequestId reqId)
{
	sceNp2.todo("sceNpAuthDeleteOAuthRequest(reqId=%d)", reqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Auth_init)
	{
		return SCE_NP_OAUTH_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpAuthAbortOAuthRequest(SceNpAuthOAuthRequestId reqId)
{
	sceNp2.todo("sceNpAuthAbortOAuthRequest(reqId=%d)", reqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Auth_init)
	{
		return SCE_NP_OAUTH_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpAuthGetAuthorizationCode(SceNpAuthOAuthRequestId reqId, vm::cptr<SceNpAuthGetAuthorizationCodeParameter> param, vm::ptr<SceNpAuthorizationCode> authCode, vm::ptr<s32> issuerId)
{
	sceNp2.todo("sceNpAuthGetAuthorizationCode(reqId=%d, param=*0x%x, authCode=*0x%x, issuerId=%d)", reqId, param, authCode, issuerId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Auth_init)
	{
		return SCE_NP_OAUTH_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpAuthGetAuthorizationCode2()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNp2)
("sceNp2", []() {
	REG_FUNC(sceNp2, sceNpMatching2DestroyContext);
	REG_FUNC(sceNp2, sceNpMatching2LeaveLobby);
	REG_FUNC(sceNp2, sceNpMatching2RegisterLobbyMessageCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetWorldInfoList);
	REG_FUNC(sceNp2, sceNpMatching2RegisterLobbyEventCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberDataInternalList);
	REG_FUNC(sceNp2, sceNpMatching2SearchRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetConnectionStatus);
	REG_FUNC(sceNp2, sceNpMatching2SetUserInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetClanLobbyId);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2ContextStart);
	REG_FUNC(sceNp2, sceNpMatching2CreateServerContext);
	REG_FUNC(sceNp2, sceNpMatching2GetMemoryInfo);
	REG_FUNC(sceNp2, sceNpMatching2LeaveRoom);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomDataExternal);
	REG_FUNC(sceNp2, sceNpMatching2Term2);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetConnectionInfo);
	REG_FUNC(sceNp2, sceNpMatching2SendRoomMessage);
	REG_FUNC(sceNp2, sceNpMatching2JoinLobby);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataExternalList);
	REG_FUNC(sceNp2, sceNpMatching2AbortRequest);
	REG_FUNC(sceNp2, sceNpMatching2Term);
	REG_FUNC(sceNp2, sceNpMatching2GetServerInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetEventData);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomSlotInfoLocal);
	REG_FUNC(sceNp2, sceNpMatching2SendLobbyChatMessage);
	REG_FUNC(sceNp2, sceNpMatching2Init);
	REG_FUNC(sceNp2, sceNp2Init);
	REG_FUNC(sceNp2, sceNpMatching2AbortContextStart);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberIdListLocal);
	REG_FUNC(sceNp2, sceNpMatching2JoinRoom);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataInternalLocal);
	REG_FUNC(sceNp2, sceNpMatching2GetCbQueueInfo);
	REG_FUNC(sceNp2, sceNpMatching2KickoutRoomMember);
	REG_FUNC(sceNp2, sceNpMatching2ContextStartAsync);
	REG_FUNC(sceNp2, sceNpMatching2SetSignalingOptParam);
	REG_FUNC(sceNp2, sceNpMatching2RegisterContextCallback);
	REG_FUNC(sceNp2, sceNpMatching2SendRoomChatMessage);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetPingInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetServerIdListLocal);
	REG_FUNC(sceNp2, sceNpUtilBuildCdnUrl);
	REG_FUNC(sceNp2, sceNpMatching2GrantRoomOwner);
	REG_FUNC(sceNp2, sceNpMatching2CreateContext);
	REG_FUNC(sceNp2, sceNpMatching2GetSignalingOptParamLocal);
	REG_FUNC(sceNp2, sceNpMatching2RegisterSignalingCallback);
	REG_FUNC(sceNp2, sceNpMatching2ClearEventData);
	REG_FUNC(sceNp2, sceNp2Term);
	REG_FUNC(sceNp2, sceNpMatching2GetUserInfoList);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2JoinProhibitiveRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingSetCtxOpt);
	REG_FUNC(sceNp2, sceNpMatching2DeleteServerContext);
	REG_FUNC(sceNp2, sceNpMatching2SetDefaultRequestOptParam);
	REG_FUNC(sceNp2, sceNpMatching2RegisterRoomEventCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomPasswordLocal);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomDataExternalList);
	REG_FUNC(sceNp2, sceNpMatching2CreateJoinRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetCtxOpt);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyInfoList);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberIdListLocal);
	REG_FUNC(sceNp2, sceNpMatching2SendLobbyInvitation);
	REG_FUNC(sceNp2, sceNpMatching2ContextStop);
	REG_FUNC(sceNp2, sceNpMatching2Init2);
	REG_FUNC(sceNp2, sceNpMatching2SetLobbyMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2RegisterRoomMessageCallback);
	REG_FUNC(sceNp2, sceNpMatching2SignalingCancelPeerNetInfo);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetLocalNetInfo);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetPeerNetInfo);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetPeerNetInfoResult);

	REG_FUNC(sceNp2, sceNpAuthOAuthInit);
	REG_FUNC(sceNp2, sceNpAuthOAuthTerm);
	REG_FUNC(sceNp2, sceNpAuthCreateOAuthRequest);
	REG_FUNC(sceNp2, sceNpAuthDeleteOAuthRequest);
	REG_FUNC(sceNp2, sceNpAuthAbortOAuthRequest);
	REG_FUNC(sceNp2, sceNpAuthGetAuthorizationCode);
	REG_FUNC(sceNp2, sceNpAuthGetAuthorizationCode2);
});
