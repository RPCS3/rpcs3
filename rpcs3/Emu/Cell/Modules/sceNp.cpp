#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Emu/VFS.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/cellUserInfo.h"
#include "Emu/Io/interception.h"
#include "Utilities/StrUtil.h"

#include "sysPrxForUser.h"
#include "Emu/IdManager.h"
#include "Crypto/unedat.h"
#include "Crypto/unself.h"
#include "cellRtc.h"
#include "sceNp.h"
#include "cellSysutil.h"

#include "Emu/Cell/lv2/sys_time.h"
#include "Emu/Cell/lv2/sys_fs.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/NP/np_handler.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/NP/np_structs_extra.h"
#include "Emu/system_config.h"

LOG_CHANNEL(sceNp);

template <>
void fmt_class_string<SceNpError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(GAME_ERR_NOT_XMBBUY_CONTENT);
			STR_CASE(SCE_NP_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_ERROR_ID_NO_SPACE);
			STR_CASE(SCE_NP_ERROR_ID_NOT_FOUND);
			STR_CASE(SCE_NP_ERROR_SESSION_RUNNING);
			STR_CASE(SCE_NP_ERROR_LOGINID_ALREADY_EXISTS);
			STR_CASE(SCE_NP_ERROR_INVALID_TICKET_SIZE);
			STR_CASE(SCE_NP_ERROR_INVALID_STATE);
			STR_CASE(SCE_NP_ERROR_ABORTED);
			STR_CASE(SCE_NP_ERROR_OFFLINE);
			STR_CASE(SCE_NP_ERROR_VARIANT_ACCOUNT_ID);
			STR_CASE(SCE_NP_ERROR_GET_CLOCK);
			STR_CASE(SCE_NP_ERROR_INSUFFICIENT_BUFFER);
			STR_CASE(SCE_NP_ERROR_EXPIRED_TICKET);
			STR_CASE(SCE_NP_ERROR_TICKET_PARAM_NOT_FOUND);
			STR_CASE(SCE_NP_ERROR_UNSUPPORTED_TICKET_VERSION);
			STR_CASE(SCE_NP_ERROR_TICKET_STATUS_CODE_INVALID);
			STR_CASE(SCE_NP_ERROR_INVALID_TICKET_VERSION);
			STR_CASE(SCE_NP_ERROR_ALREADY_USED);
			STR_CASE(SCE_NP_ERROR_DIFFERENT_USER);
			STR_CASE(SCE_NP_ERROR_ALREADY_DONE);
			STR_CASE(SCE_NP_BASIC_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_BASIC_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_BASIC_ERROR_NOT_SUPPORTED);
			STR_CASE(SCE_NP_BASIC_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_BASIC_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_BASIC_ERROR_BAD_ID);
			STR_CASE(SCE_NP_BASIC_ERROR_IDS_DIFFER);
			STR_CASE(SCE_NP_BASIC_ERROR_PARSER_FAILED);
			STR_CASE(SCE_NP_BASIC_ERROR_TIMEOUT);
			STR_CASE(SCE_NP_BASIC_ERROR_NO_EVENT);
			STR_CASE(SCE_NP_BASIC_ERROR_EXCEEDS_MAX);
			STR_CASE(SCE_NP_BASIC_ERROR_INSUFFICIENT);
			STR_CASE(SCE_NP_BASIC_ERROR_NOT_REGISTERED);
			STR_CASE(SCE_NP_BASIC_ERROR_DATA_LOST);
			STR_CASE(SCE_NP_BASIC_ERROR_BUSY);
			STR_CASE(SCE_NP_BASIC_ERROR_STATUS);
			STR_CASE(SCE_NP_BASIC_ERROR_CANCEL);
			STR_CASE(SCE_NP_BASIC_ERROR_INVALID_MEMORY_CONTAINER);
			STR_CASE(SCE_NP_BASIC_ERROR_INVALID_DATA_ID);
			STR_CASE(SCE_NP_BASIC_ERROR_BROKEN_DATA);
			STR_CASE(SCE_NP_BASIC_ERROR_BLOCKLIST_ADD_FAILED);
			STR_CASE(SCE_NP_BASIC_ERROR_BLOCKLIST_IS_FULL);
			STR_CASE(SCE_NP_BASIC_ERROR_SEND_FAILED);
			STR_CASE(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
			STR_CASE(SCE_NP_BASIC_ERROR_INSUFFICIENT_DISK_SPACE);
			STR_CASE(SCE_NP_BASIC_ERROR_INTERNAL_FAILURE);
			STR_CASE(SCE_NP_BASIC_ERROR_DOES_NOT_EXIST);
			STR_CASE(SCE_NP_BASIC_ERROR_INVALID);
			STR_CASE(SCE_NP_BASIC_ERROR_UNKNOWN);
			STR_CASE(SCE_NP_EXT_ERROR_CONTEXT_DOES_NOT_EXIST);
			STR_CASE(SCE_NP_EXT_ERROR_CONTEXT_ALREADY_EXISTS);
			STR_CASE(SCE_NP_EXT_ERROR_NO_CONTEXT);
			STR_CASE(SCE_NP_EXT_ERROR_NO_ORIGIN);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_UTIL_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_UTIL_ERROR_INSUFFICIENT);
			STR_CASE(SCE_NP_UTIL_ERROR_PARSER_FAILED);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_PROTOCOL_ID);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_NP_ID);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_NP_LOBBY_ID);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_NP_ROOM_ID);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_NP_ENV);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_TITLEID);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_CHARACTER);
			STR_CASE(SCE_NP_UTIL_ERROR_INVALID_ESCAPE_STRING);
			STR_CASE(SCE_NP_UTIL_ERROR_UNKNOWN_TYPE);
			STR_CASE(SCE_NP_UTIL_ERROR_UNKNOWN);
			STR_CASE(SCE_NP_UTIL_ERROR_NOT_MATCH);
			STR_CASE(SCE_NP_UTIL_ERROR_UNKNOWN_PLATFORM_TYPE);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_INVALID_MEMORY_CONTAINER);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_INSUFFICIENT);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_CANCEL);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_STATUS);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_BUSY);
			STR_CASE(SCE_NP_FRIENDLIST_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_PROFILE_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_PROFILE_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_PROFILE_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_PROFILE_ERROR_NOT_SUPPORTED);
			STR_CASE(SCE_NP_PROFILE_ERROR_INSUFFICIENT);
			STR_CASE(SCE_NP_PROFILE_ERROR_CANCEL);
			STR_CASE(SCE_NP_PROFILE_ERROR_STATUS);
			STR_CASE(SCE_NP_PROFILE_ERROR_BUSY);
			STR_CASE(SCE_NP_PROFILE_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_PROFILE_ERROR_ABORT);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_NO_TITLE_SET);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_NO_LOGIN);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_TRANSACTION_STILL_REFERENCED);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_ABORTED);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_NO_RESOURCE);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_BAD_RESPONSE);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_BODY_TOO_LARGE);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_HTTP_SERVER);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INVALID_SIGNATURE);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_TIMEOUT);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_UNKNOWN_TYPE);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INVALID_ID);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INVALID_TICKET);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_CLIENT_HANDLE_ALREADY_EXISTS);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_BUFFER);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INVALID_TYPE);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_TRANSACTION_ALREADY_END);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_TRANSACTION_BEFORE_END);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_BUSY_BY_ANOTEHR_TRANSACTION);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_TOO_LARGE_RANGE);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_INVALID_PARTITION);
			STR_CASE(SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_BAD_REQUEST);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_TICKET);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_SIGNATURE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_EXPIRED_TICKET);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_NPID);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INTERNAL_SERVER_ERROR);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_VERSION_NOT_SUPPORTED);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_SERVICE_UNAVAILABLE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_PLAYER_BANNED);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_CENSORED);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_RECORD_FORBIDDEN);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_USER_PROFILE_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_UPLOADER_DATA_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_QUOTA_MASTER_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_TITLE_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_BLACKLISTED_USER_ID);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_GAME_RANKING_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_STORE_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_NOT_BEST_SCORE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_LATEST_UPDATE_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_BOARD_MASTER_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_GAME_DATA_MASTER_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ANTICHEAT_DATA);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_LARGE_DATA);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_USER_NPID);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ENVIRONMENT);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ONLINE_NAME_CHARACTER);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ONLINE_NAME_LENGTH);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ABOUT_ME_CHARACTER);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ABOUT_ME_LENGTH);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_SCORE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_OVER_THE_RANKING_LIMIT);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_FAIL_TO_CREATE_SIGNATURE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_MASTER_INFO_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_OVER_THE_GAME_DATA_LIMIT);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_SELF_DATA_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_USER_NOT_ASSIGNED);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_GAME_DATA_ALREADY_EXISTS);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_MANY_RESULTS);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_NOT_RECORDABLE_VERSION);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_USER_STORAGE_TITLE_MASTER_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_VIRTUAL_USER);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_USER_STORAGE_DATA_NOT_FOUND);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_CONDITIONS_NOT_SATISFIED);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_BEFORE_SERVICE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_END_OF_SERVICE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_MAINTENANCE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_BEFORE_SERVICE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_END_OF_SERVICE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_MAINTENANCE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_TITLE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_TITLE_USER_STORAGE_BEFORE_SERVICE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_TITLE_USER_STORAGE_END_OF_SERVICE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_TITLE_USER_STORAGE_MAINTENANCE);
			STR_CASE(SCE_NP_COMMUNITY_SERVER_ERROR_UNSPECIFIED);
			STR_CASE(SCE_NP_DRM_ERROR_LICENSE_NOT_FOUND);
			STR_CASE(SCE_NP_DRM_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_DRM_ERROR_INVALID_PARAM);
			STR_CASE(SCE_NP_DRM_ERROR_SERVER_RESPONSE);
			STR_CASE(SCE_NP_DRM_ERROR_NO_ENTITLEMENT);
			STR_CASE(SCE_NP_DRM_ERROR_BAD_ACT);
			STR_CASE(SCE_NP_DRM_ERROR_BAD_FORMAT);
			STR_CASE(SCE_NP_DRM_ERROR_NO_LOGIN);
			STR_CASE(SCE_NP_DRM_ERROR_INTERNAL);
			STR_CASE(SCE_NP_DRM_ERROR_BAD_PERM);
			STR_CASE(SCE_NP_DRM_ERROR_UNKNOWN_VERSION);
			STR_CASE(SCE_NP_DRM_ERROR_TIME_LIMIT);
			STR_CASE(SCE_NP_DRM_ERROR_DIFFERENT_ACCOUNT_ID);
			STR_CASE(SCE_NP_DRM_ERROR_DIFFERENT_DRM_TYPE);
			STR_CASE(SCE_NP_DRM_ERROR_SERVICE_NOT_STARTED);
			STR_CASE(SCE_NP_DRM_ERROR_BUSY);
			STR_CASE(SCE_NP_DRM_ERROR_IO);
			STR_CASE(SCE_NP_DRM_ERROR_FORMAT);
			STR_CASE(SCE_NP_DRM_ERROR_FILENAME);
			STR_CASE(SCE_NP_DRM_ERROR_K_LICENSEE);
			STR_CASE(SCE_NP_AUTH_EINVAL);
			STR_CASE(SCE_NP_AUTH_ENOMEM);
			STR_CASE(SCE_NP_AUTH_ESRCH);
			STR_CASE(SCE_NP_AUTH_EBUSY);
			STR_CASE(SCE_NP_AUTH_EABORT);
			STR_CASE(SCE_NP_AUTH_EEXIST);
			STR_CASE(SCE_NP_AUTH_EINVALID_ARGUMENT);
			STR_CASE(SCE_NP_AUTH_ERROR_SERVICE_END);
			STR_CASE(SCE_NP_AUTH_ERROR_SERVICE_DOWN);
			STR_CASE(SCE_NP_AUTH_ERROR_SERVICE_BUSY);
			STR_CASE(SCE_NP_AUTH_ERROR_SERVER_MAINTENANCE);
			STR_CASE(SCE_NP_AUTH_ERROR_INVALID_DATA_LENGTH);
			STR_CASE(SCE_NP_AUTH_ERROR_INVALID_USER_AGENT);
			STR_CASE(SCE_NP_AUTH_ERROR_INVALID_VERSION);
			STR_CASE(SCE_NP_AUTH_ERROR_INVALID_SERVICE_ID);
			STR_CASE(SCE_NP_AUTH_ERROR_INVALID_CREDENTIAL);
			STR_CASE(SCE_NP_AUTH_ERROR_INVALID_ENTITLEMENT_ID);
			STR_CASE(SCE_NP_AUTH_ERROR_INVALID_CONSUMED_COUNT);
			STR_CASE(SCE_NP_AUTH_ERROR_INVALID_CONSOLE_ID);
			STR_CASE(SCE_NP_AUTH_ERROR_CONSOLE_ID_SUSPENDED);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_CLOSED);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_SUSPENDED);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_EULA);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT1);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT2);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT3);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT4);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT5);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT6);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT7);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT8);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT9);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT10);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT11);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT12);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT13);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT14);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT15);
			STR_CASE(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT16);
			STR_CASE(SCE_NP_AUTH_ERROR_UNKNOWN);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_INSUFFICIENT);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_PARSER_FAILED);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_INVALID_PROTOCOL_ID);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_INVALID_EXTENSION);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_INVALID_TEXT);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_UNKNOWN_TYPE);
			STR_CASE(SCE_NP_CORE_UTIL_ERROR_UNKNOWN);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_INSUFFICIENT);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_INVALID_FORMAT);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_INVALID_HANDLE);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_INVALID_ICON);
			STR_CASE(SCE_NP_CORE_PARSER_ERROR_UNKNOWN);
			STR_CASE(SCE_NP_CORE_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_CORE_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_CORE_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_CORE_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_CORE_ERROR_ID_NOT_AVAILABLE);
			STR_CASE(SCE_NP_CORE_ERROR_USER_OFFLINE);
			STR_CASE(SCE_NP_CORE_ERROR_SESSION_RUNNING);
			STR_CASE(SCE_NP_CORE_ERROR_SESSION_NOT_ESTABLISHED);
			STR_CASE(SCE_NP_CORE_ERROR_SESSION_INVALID_STATE);
			STR_CASE(SCE_NP_CORE_ERROR_SESSION_ID_TOO_LONG);
			STR_CASE(SCE_NP_CORE_ERROR_SESSION_INVALID_NAMESPACE);
			STR_CASE(SCE_NP_CORE_ERROR_CONNECTION_TIMEOUT);
			STR_CASE(SCE_NP_CORE_ERROR_GETSOCKOPT);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_NOT_INITIALIZED);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_NO_CERT);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_NO_TRUSTWORTHY_CA);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_INVALID_CERT);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_CERT_VERIFY);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_CN_CHECK);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_HANDSHAKE_FAILED);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_SEND);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_RECV);
			STR_CASE(SCE_NP_CORE_ERROR_SSL_CREATE_CTX);
			STR_CASE(SCE_NP_CORE_ERROR_PARSE_PEM);
			STR_CASE(SCE_NP_CORE_ERROR_INVALID_INITIATE_STREAM);
			STR_CASE(SCE_NP_CORE_ERROR_SASL_NOT_SUPPORTED);
			STR_CASE(SCE_NP_CORE_ERROR_NAMESPACE_ALREADY_EXISTS);
			STR_CASE(SCE_NP_CORE_ERROR_FROM_ALREADY_EXISTS);
			STR_CASE(SCE_NP_CORE_ERROR_MODULE_NOT_REGISTERED);
			STR_CASE(SCE_NP_CORE_ERROR_MODULE_FROM_NOT_FOUND);
			STR_CASE(SCE_NP_CORE_ERROR_UNKNOWN_NAMESPACE);
			STR_CASE(SCE_NP_CORE_ERROR_INVALID_VERSION);
			STR_CASE(SCE_NP_CORE_ERROR_LOGIN_TIMEOUT);
			STR_CASE(SCE_NP_CORE_ERROR_TOO_MANY_SESSIONS);
			STR_CASE(SCE_NP_CORE_ERROR_SENDLIST_NOT_FOUND);
			STR_CASE(SCE_NP_CORE_ERROR_NO_ID);
			STR_CASE(SCE_NP_CORE_ERROR_LOAD_CERTS);
			STR_CASE(SCE_NP_CORE_ERROR_NET_SELECT);
			STR_CASE(SCE_NP_CORE_ERROR_DISCONNECTED);
			STR_CASE(SCE_NP_CORE_ERROR_TICKET_TOO_SMALL);
			STR_CASE(SCE_NP_CORE_ERROR_INVALID_TICKET);
			STR_CASE(SCE_NP_CORE_ERROR_INVALID_ONLINEID);
			STR_CASE(SCE_NP_CORE_ERROR_GETHOSTBYNAME);
			STR_CASE(SCE_NP_CORE_ERROR_UNDEFINED_STREAM_ERROR);
			STR_CASE(SCE_NP_CORE_ERROR_INTERNAL);
			STR_CASE(SCE_NP_CORE_ERROR_DNS_HOST_NOT_FOUND);
			STR_CASE(SCE_NP_CORE_ERROR_DNS_TRY_AGAIN);
			STR_CASE(SCE_NP_CORE_ERROR_DNS_NO_RECOVERY);
			STR_CASE(SCE_NP_CORE_ERROR_DNS_NO_DATA);
			STR_CASE(SCE_NP_CORE_ERROR_DNS_NO_ADDRESS);
			STR_CASE(SCE_NP_CORE_SERVER_ERROR_CONFLICT);
			STR_CASE(SCE_NP_CORE_SERVER_ERROR_NOT_AUTHORIZED);
			STR_CASE(SCE_NP_CORE_SERVER_ERROR_REMOTE_CONNECTION_FAILED);
			STR_CASE(SCE_NP_CORE_SERVER_ERROR_RESOURCE_CONSTRAINT);
			STR_CASE(SCE_NP_CORE_SERVER_ERROR_SYSTEM_SHUTDOWN);
			STR_CASE(SCE_NP_CORE_SERVER_ERROR_UNSUPPORTED_CLIENT_VERSION);
			STR_CASE(SCE_NP_DRM_INSTALL_ERROR_FORMAT);
			STR_CASE(SCE_NP_DRM_INSTALL_ERROR_CHECK);
			STR_CASE(SCE_NP_DRM_INSTALL_ERROR_UNSUPPORTED);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_SERVICE_IS_END);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_SERVICE_STOP_TEMPORARILY);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_SERVICE_IS_BUSY);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_INVALID_USER_CREDENTIAL);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_INVALID_PRODUCT_ID);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_ACCOUNT_IS_CLOSED);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_ACCOUNT_IS_SUSPENDED);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_ACTIVATED_CONSOLE_IS_FULL);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_CONSOLE_NOT_ACTIVATED);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_PRIMARY_CONSOLE_CANNOT_CHANGED);
			STR_CASE(SCE_NP_DRM_SERVER_ERROR_UNKNOWN);
			STR_CASE(SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_SIGNALING_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_SIGNALING_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_SIGNALING_ERROR_CTXID_NOT_AVAILABLE);
			STR_CASE(SCE_NP_SIGNALING_ERROR_CTX_NOT_FOUND);
			STR_CASE(SCE_NP_SIGNALING_ERROR_REQID_NOT_AVAILABLE);
			STR_CASE(SCE_NP_SIGNALING_ERROR_REQ_NOT_FOUND);
			STR_CASE(SCE_NP_SIGNALING_ERROR_PARSER_CREATE_FAILED);
			STR_CASE(SCE_NP_SIGNALING_ERROR_PARSER_FAILED);
			STR_CASE(SCE_NP_SIGNALING_ERROR_INVALID_NAMESPACE);
			STR_CASE(SCE_NP_SIGNALING_ERROR_NETINFO_NOT_AVAILABLE);
			STR_CASE(SCE_NP_SIGNALING_ERROR_PEER_NOT_RESPONDING);
			STR_CASE(SCE_NP_SIGNALING_ERROR_CONNID_NOT_AVAILABLE);
			STR_CASE(SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND);
			STR_CASE(SCE_NP_SIGNALING_ERROR_PEER_UNREACHABLE);
			STR_CASE(SCE_NP_SIGNALING_ERROR_TERMINATED_BY_PEER);
			STR_CASE(SCE_NP_SIGNALING_ERROR_TIMEOUT);
			STR_CASE(SCE_NP_SIGNALING_ERROR_CTX_MAX);
			STR_CASE(SCE_NP_SIGNALING_ERROR_RESULT_NOT_FOUND);
			STR_CASE(SCE_NP_SIGNALING_ERROR_CONN_IN_PROGRESS);
			STR_CASE(SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_SIGNALING_ERROR_OWN_NP_ID);
			STR_CASE(SCE_NP_SIGNALING_ERROR_TOO_MANY_CONN);
			STR_CASE(SCE_NP_SIGNALING_ERROR_TERMINATED_BY_MYSELF);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_NOT_SUPPORTED);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_INSUFFICIENT);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_CANCEL);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_STATUS);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_BUSY);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_ABORT);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_NOT_REGISTERED);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_EXCEEDS_MAX);
			STR_CASE(SCE_NP_CUSTOM_MENU_ERROR_INVALID_CHARACTER);
		}

		return unknown;
	});
}

void message_data::print() const
{
	sceNp.notice("commId: %s, msgId: %d, mainType: %d, subType: %d, subject: %s, body: %s, data_size: %d", static_cast<const char *>(commId.data), msgId, mainType, subType, subject, body, data.size());
}

extern void lv2_sleep(u64 timeout, ppu_thread* ppu = nullptr);

error_code sceNpInit(u32 poolsize, vm::ptr<void> poolptr)
{
	sceNp.warning("sceNpInit(poolsize=0x%x, poolptr=*0x%x)", poolsize, poolptr);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	std::lock_guard lock(nph.mutex_status);

	if (nph.is_NP_init)
	{
		return SCE_NP_ERROR_ALREADY_INITIALIZED;
	}

	if (poolsize == 0 || !poolptr)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (poolsize < SCE_NP_MIN_POOL_SIZE)
	{
		return SCE_NP_ERROR_INSUFFICIENT_BUFFER;
	}

	nph.init_NP(poolsize, poolptr);
	nph.is_NP_init = true;

	return CELL_OK;
}

error_code sceNpTerm()
{
	sceNp.warning("sceNpTerm()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	std::lock_guard lock(nph.mutex_status);

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph.terminate_NP();
	nph.is_NP_init = false;

	return CELL_OK;
}

error_code npDrmIsAvailable(vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	if (!drm_path)
	{
		return SCE_NP_DRM_ERROR_INVALID_PARAM;
	}

	u128 k_licensee{};

	if (k_licensee_addr)
	{
		std::memcpy(&k_licensee, k_licensee_addr.get_ptr(), sizeof(k_licensee));
		sceNp.notice("npDrmIsAvailable(): KLicense key or KLIC=%s", std::bit_cast<be_t<u128>>(k_licensee));
	}

	if (Emu.GetFakeCat() == "PE")
	{
		std::memcpy(&k_licensee, NP_PSP_KEY_2, sizeof(k_licensee));
		sceNp.success("npDrmIsAvailable(): PSP remaster KLicense key applied.");
	}

	const std::string enc_drm_path(drm_path.get_ptr(), std::find(drm_path.get_ptr(), drm_path.get_ptr() + 0x100, '\0'));

	sceNp.warning(u8"npDrmIsAvailable(): drm_path=“%s”", enc_drm_path);

	auto& npdrmkeys = g_fxo->get<loaded_npdrm_keys>();

	const auto [fs_error, ppath, real_path, enc_file, type] = lv2_file::open(enc_drm_path, 0, 0);

	if (fs_error)
	{
		return {fs_error, enc_drm_path};
	}

	u32 magic;

	enc_file.read<u32>(magic);
	enc_file.seek(0);

	if (magic == "SCE\0"_u32)
	{
		if (!k_licensee_addr)
			k_licensee = get_default_self_klic();

		if (verify_npdrm_self_headers(enc_file, reinterpret_cast<u8*>(&k_licensee)))
		{
			npdrmkeys.install_decryption_key(k_licensee);
		}
		else
		{
			sceNp.error(u8"npDrmIsAvailable(): Failed to verify sce file “%s”", enc_drm_path);
			return {SCE_NP_DRM_ERROR_NO_ENTITLEMENT, enc_drm_path};
		}
	}
	else if (magic == "NPD\0"_u32)
	{
		// edata / sdata files

		NPD_HEADER npd;

		if (VerifyEDATHeaderWithKLicense(enc_file, enc_drm_path, reinterpret_cast<u8*>(&k_licensee), &npd))
		{
			// Check if RAP-free
			if (npd.license == 3)
			{
				npdrmkeys.install_decryption_key(k_licensee);
			}
			else
			{
				const std::string rap_file = rpcs3::utils::get_rap_file_path(npd.content_id);

				if (fs::file rap_fd{rap_file}; rap_fd && rap_fd.size() >= sizeof(u128))
				{
					npdrmkeys.install_decryption_key(GetEdatRifKeyFromRapFile(rap_fd));
				}
				else
				{
					sceNp.error(u8"npDrmIsAvailable(): Rap file not found: “%s”", rap_file);
					return {SCE_NP_DRM_ERROR_LICENSE_NOT_FOUND, enc_drm_path};
				}
			}
		}
		else
		{
			sceNp.error(u8"npDrmIsAvailable(): Failed to verify npd file “%s”", enc_drm_path);
			return {SCE_NP_DRM_ERROR_NO_ENTITLEMENT, enc_drm_path};
		}
	}
	else
	{
		// for now assume its just unencrypted
		sceNp.notice(u8"npDrmIsAvailable(): Assuming npdrm file is unencrypted at “%s”", enc_drm_path);
	}

	return CELL_OK;
}

error_code sceNpDrmIsAvailable(ppu_thread& ppu, vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	sceNp.warning("sceNpDrmIsAvailable(k_licensee=*0x%x, drm_path=*0x%x)", k_licensee_addr, drm_path);

	if (!drm_path)
	{
		return SCE_NP_DRM_ERROR_INVALID_PARAM;
	}

	lv2_obj::sleep(ppu);

	const auto ret = npDrmIsAvailable(k_licensee_addr, drm_path);
	lv2_sleep(50'000, &ppu);

	return ret;
}

error_code sceNpDrmIsAvailable2(ppu_thread& ppu, vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	sceNp.warning("sceNpDrmIsAvailable2(k_licensee=*0x%x, drm_path=*0x%x)", k_licensee_addr, drm_path);

	if (!drm_path)
	{
		return SCE_NP_DRM_ERROR_INVALID_PARAM;
	}

	lv2_obj::sleep(ppu);

	const auto ret = npDrmIsAvailable(k_licensee_addr, drm_path);

	// TODO: Accurate sleep time
	//lv2_sleep(20000, &ppu);

	return ret;
}

error_code npDrmVerifyUpgradeLicense(vm::cptr<char> content_id)
{
	if (!content_id)
	{
		return SCE_NP_DRM_ERROR_INVALID_PARAM;
	}

	const std::string content_str(content_id.get_ptr(), std::find(content_id.get_ptr(), content_id.get_ptr() + 0x2f, '\0'));
	sceNp.warning("npDrmVerifyUpgradeLicense(): content_id=%s", content_id);

	if (!rpcs3::utils::verify_c00_unlock_edat(content_str))
		return SCE_NP_DRM_ERROR_LICENSE_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpDrmVerifyUpgradeLicense(vm::cptr<char> content_id)
{
	sceNp.warning("sceNpDrmVerifyUpgradeLicense(content_id=*0x%x)", content_id);

	return npDrmVerifyUpgradeLicense(content_id);
}

error_code sceNpDrmVerifyUpgradeLicense2(vm::cptr<char> content_id)
{
	sceNp.warning("sceNpDrmVerifyUpgradeLicense2(content_id=*0x%x)", content_id);

	return npDrmVerifyUpgradeLicense(content_id);
}

error_code sceNpDrmExecuteGamePurchase()
{
	sceNp.todo("sceNpDrmExecuteGamePurchase()");

	// TODO:
	// 0. Check if the game can be purchased (return GAME_ERR_NOT_XMBBUY_CONTENT otherwise)
	// 1. Send game termination request
	// 2. "Buy game" transaction (a.k.a. do nothing for now)
	// 3. Reboot game with CELL_GAME_ATTRIBUTE_XMBBUY attribute set (cellGameBootCheck)

	return CELL_OK;
}

error_code sceNpDrmGetTimelimit(vm::cptr<char> path, vm::ptr<u64> time_remain)
{
	sceNp.warning("sceNpDrmGetTimelimit(path=%s, time_remain=*0x%x)", path, time_remain);

	if (!path || !time_remain)
	{
		return SCE_NP_DRM_ERROR_INVALID_PARAM;
	}

	vm::var<s64> sec;
	vm::var<s64> nsec;

	// Get system time (real or fake) to compare to
	error_code ret = sys_time_get_current_time(sec, nsec);
	if (ret != CELL_OK)
	{
		return ret;
	}

	const std::string enc_drm_path(path.get_ptr(), std::find(path.get_ptr(), path.get_ptr() + 0x100, '\0'));
	const auto [fs_error, ppath, real_path, enc_file, type] = lv2_file::open(enc_drm_path, 0, 0);

	if (fs_error)
	{
		return {fs_error, enc_drm_path};
	}

	u32 magic;
	NPD_HEADER npd;

	enc_file.read<u32>(magic);
	enc_file.seek(0);

	// Read expiration time from NPD header which is Unix timestamp in milliseconds
	if (magic == "SCE\0"_u32)
	{
		if (!get_npdrm_self_header(enc_file, npd))
		{
			sceNp.error("sceNpDrmGetTimelimit(): Failed to read NPD header from sce file '%s'", enc_drm_path);
			return {SCE_NP_DRM_ERROR_BAD_FORMAT, enc_drm_path};
		}
	}
	else if (magic == "NPD\0"_u32)
	{
		// edata / sdata files
		EDAT_HEADER edat;
		read_npd_edat_header(&enc_file, npd, edat);
	}
	else
	{
		// Unknown file type
		return {SCE_NP_DRM_ERROR_BAD_FORMAT, enc_drm_path};
	}

	// Convert time to milliseconds
	s64 msec = *sec * 1000ll + *nsec / 1000ll;

	// Return the remaining time in microseconds
	if (npd.activate_time != 0 && msec < npd.activate_time)
	{
		return SCE_NP_DRM_ERROR_SERVICE_NOT_STARTED;
	}

	if (npd.expire_time == 0)
	{
		*time_remain = SCE_NP_DRM_TIME_INFO_ENDLESS;
		return CELL_OK;
	}

	if (msec >= npd.expire_time)
	{
		return SCE_NP_DRM_ERROR_TIME_LIMIT;
	}

	*time_remain = (npd.expire_time - msec) * 1000ll;
	return CELL_OK;
}

error_code sceNpDrmProcessExitSpawn(ppu_thread& ppu, vm::cptr<u8> klicensee, vm::cptr<char> path, vm::cpptr<char> argv, vm::cpptr<char> envp, u32 data, u32 data_size, s32 prio, u64 flags)
{
	sceNp.warning("sceNpDrmProcessExitSpawn(klicensee=*0x%x, path=*0x%x, argv=**0x%x, envp=**0x%x, data=*0x%x, data_size=0x%x, prio=%d, flags=0x%x)", klicensee, path, argv, envp, data, data_size, prio, flags);

	if (s32 error = npDrmIsAvailable(klicensee, path))
	{
		return error;
	}

	ppu_execute<&sys_game_process_exitspawn>(ppu, path, argv, envp, data, data_size, prio, flags);
	return CELL_OK;
}

error_code sceNpDrmProcessExitSpawn2(ppu_thread& ppu, vm::cptr<u8> klicensee, vm::cptr<char> path, vm::cpptr<char> argv, vm::cpptr<char> envp, u32 data, u32 data_size, s32 prio, u64 flags)
{
	sceNp.warning("sceNpDrmProcessExitSpawn2(klicensee=*0x%x, path=*0x%x, argv=**0x%x, envp=**0x%x, data=*0x%x, data_size=0x%x, prio=%d, flags=0x%x)", klicensee, path, argv, envp, data, data_size, prio, flags);

	if (s32 error = npDrmIsAvailable(klicensee, path))
	{
		return error;
	}

	// TODO: check if SCE_NP_DRM_EXITSPAWN2_EXIT_WO_FINI logic is implemented
	ppu_execute<&sys_game_process_exitspawn2>(ppu, path, argv, envp, data, data_size, prio, flags);
	return CELL_OK;
}

error_code sceNpBasicRegisterHandler(vm::cptr<SceNpCommunicationId> context, vm::ptr<SceNpBasicEventHandler> handler, vm::ptr<void> arg)
{
	sceNp.warning("sceNpBasicRegisterHandler(context=*0x%x(%s), handler=*0x%x, arg=*0x%x)", context, context ? static_cast<const char *>(context->data) : "", handler, arg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (!context || !handler)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	memcpy(&nph.basic_handler.context, context.get_ptr(), sizeof(nph.basic_handler.context));
	nph.basic_handler.handler_func = handler;
	nph.basic_handler.handler_arg = arg;
	nph.basic_handler.registered = true;
	nph.basic_handler.context_sensitive = false;

	return CELL_OK;
}

error_code sceNpBasicRegisterContextSensitiveHandler(vm::cptr<SceNpCommunicationId> context, vm::ptr<SceNpBasicEventHandler> handler, vm::ptr<void> arg)
{
	sceNp.notice("sceNpBasicRegisterContextSensitiveHandler(context=*0x%x, handler=*0x%x, arg=*0x%x)", context, handler, arg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (!context || !handler)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	memcpy(&nph.basic_handler.context, context.get_ptr(), sizeof(nph.basic_handler.context));
	nph.basic_handler.handler_func = handler;
	nph.basic_handler.handler_arg = arg;
	nph.basic_handler.registered = true;
	nph.basic_handler.context_sensitive = true;

	return CELL_OK;
}

error_code sceNpBasicUnregisterHandler()
{
	sceNp.notice("sceNpBasicUnregisterHandler()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	nph.basic_handler.registered = false;

	return CELL_OK;
}

error_code sceNpBasicSetPresence(vm::cptr<u8> data, u32 size)
{
	sceNp.todo("sceNpBasicSetPresence(data=*0x%x, size=%d)", data, size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	// TODO: Correct but causes issues atm(breaks bomberman ultra)
	// if (!data || !data[0])
	// {
	// 	return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	// }

	if (size > SCE_NP_BASIC_MAX_PRESENCE_SIZE)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpBasicSetPresenceDetails(vm::cptr<SceNpBasicPresenceDetails> pres, u32 options)
{
	sceNp.todo("sceNpBasicSetPresenceDetails(pres=*0x%x, options=0x%x)", pres, options);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!pres || options > SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (pres->size > SCE_NP_BASIC_MAX_PRESENCE_SIZE)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpBasicSetPresenceDetails2(vm::cptr<SceNpBasicPresenceDetails2> pres, u32 options)
{
	sceNp.todo("sceNpBasicSetPresenceDetails2(pres=*0x%x, options=0x%x)", pres, options);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!pres || options > SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (pres->size > SCE_NP_BASIC_MAX_PRESENCE_SIZE)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpBasicSendMessage(vm::cptr<SceNpId> to, vm::cptr<void> data, u32 size)
{
	sceNp.todo("sceNpBasicSendMessage(to=*0x%x, data=*0x%x, size=%d)", to, data, size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!to || to->handle.data[0] == '\0' || !data || !size)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (size > SCE_NP_BASIC_MAX_MESSAGE_SIZE)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpBasicSendMessageGui(vm::cptr<SceNpBasicMessageDetails> msg, sys_memory_container_t containerId)
{
	sceNp.warning("sceNpBasicSendMessageGui(msg=*0x%x, containerId=%d)", msg, containerId);

	if (msg)
	{
		sceNp.notice("sceNpBasicSendMessageGui: msgId: %d, mainType: %d, subType: %d, msgFeatures: %d, count: %d", msg->msgId, msg->mainType, msg->subType, msg->msgFeatures, msg->count);
		for (u32 i = 0; i < msg->count; i++)
		{
			sceNp.trace("sceNpBasicSendMessageGui: NpId[%d] = %s", i, static_cast<char*>(&msg->npids[i].handle.data[0]));
		}
		sceNp.notice("sceNpBasicSendMessageGui: subject: %s", msg->subject);
		sceNp.notice("sceNpBasicSendMessageGui: body: %s", msg->body);
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!msg || msg->count > SCE_NP_BASIC_SEND_MESSAGE_MAX_RECIPIENTS || (msg->msgFeatures & ~SCE_NP_BASIC_MESSAGE_FEATURES_ALL_FEATURES) ||
		msg->mainType > SCE_NP_BASIC_MESSAGE_MAIN_TYPE_URL_ATTACHMENT || msg->msgId != 0ull)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: SCE_NP_BASIC_ERROR_NOT_SUPPORTED, might be in between argument checks

	if (msg->size > SCE_NP_BASIC_MAX_MESSAGE_SIZE)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return not_an_error(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
	}

	// Prepare message data
	message_data msg_data = {
		.commId      = nph.basic_handler.context,
		.msgId       = msg->msgId,
		.mainType    = msg->mainType,
		.subType     = msg->subType,
		.msgFeatures = msg->msgFeatures};
	std::set<std::string> npids;

	for (u32 i = 0; i < msg->count; i++)
	{
		npids.insert(std::string(msg->npids[i].handle.data));
	}

	if (msg->subject)
	{
		msg_data.subject = std::string(msg->subject.get_ptr());
	}

	if (msg->body)
	{
		msg_data.body = std::string(msg->body.get_ptr());
	}

	if (msg->size)
	{
		msg_data.data.assign(msg->data.get_ptr(), msg->data.get_ptr() + msg->size);
	}

	sceNp.trace("Message Data:\n%s", fmt::buf_to_hexstring(msg->data.get_ptr(), msg->size));

	bool result = false;

	input::SetIntercepted(true);

	Emu.BlockingCallFromMainThread([=, &result, msg_data = std::move(msg_data), npids = std::move(npids)]() mutable
	{
		auto send_dlg = Emu.GetCallbacks().get_sendmessage_dialog();
		result = send_dlg->Exec(msg_data, npids);
	});

	input::SetIntercepted(false);

	s32 callback_result = result ? 0 : -1;
	s32 event           = 0;

	switch (msg->mainType)
	{
	case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_DATA_ATTACHMENT:
		event = SCE_NP_BASIC_EVENT_SEND_ATTACHMENT_RESULT;
		break;
	case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_GENERAL:
		event = SCE_NP_BASIC_EVENT_SEND_MESSAGE_RESULT;
		break;
	case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_ADD_FRIEND:
		event = SCE_NP_BASIC_EVENT_ADD_FRIEND_RESULT;
		break;
	case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE:
		event = SCE_NP_BASIC_EVENT_SEND_INVITATION_RESULT;
		break;
	case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_CUSTOM_DATA:
		event = SCE_NP_BASIC_EVENT_SEND_CUSTOM_DATA_RESULT;
		break;
	case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_URL_ATTACHMENT:
		event = SCE_NP_BASIC_EVENT_SEND_URL_ATTACHMENT_RESULT;
		break;
	}

	nph.send_basic_event(event, callback_result, 0);

	return CELL_OK;
}

error_code sceNpBasicSendMessageAttachment(vm::cptr<SceNpId> to, vm::cptr<char> subject, vm::cptr<char> body, vm::cptr<char> data, u32 size, sys_memory_container_t containerId)
{
	sceNp.todo("sceNpBasicSendMessageAttachment(to=*0x%x, subject=%s, body=%s, data=%s, size=%d, containerId=%d)", to, subject, body, data, size, containerId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!to || to->handle.data[0] == '\0' || !data || !size)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: SCE_NP_BASIC_ERROR_NOT_SUPPORTED, might be in between argument checks

	if (strlen(subject.get_ptr()) > SCE_NP_BASIC_SUBJECT_CHARACTER_MAX || strlen(body.get_ptr()) > SCE_NP_BASIC_BODY_CHARACTER_MAX)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return not_an_error(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
	}

	return CELL_OK;
}

error_code sceNpBasicRecvMessageAttachment(sys_memory_container_t containerId)
{
	sceNp.todo("sceNpBasicRecvMessageAttachment(containerId=%d)", containerId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	return CELL_OK;
}

error_code sceNpBasicRecvMessageAttachmentLoad(SceNpBasicAttachmentDataId id, vm::ptr<void> buffer, vm::ptr<u32> size)
{
	sceNp.warning("sceNpBasicRecvMessageAttachmentLoad(id=%d, buffer=*0x%x, size=*0x%x)", id, buffer, size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!buffer || !size)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (id != SCE_NP_BASIC_SELECTED_INVITATION_DATA && id != SCE_NP_BASIC_SELECTED_MESSAGE_DATA)
	{
		return SCE_NP_BASIC_ERROR_INVALID_DATA_ID;
	}

	const auto opt_msg = nph.get_message_selected(id);
	if (!opt_msg)
	{
		return SCE_NP_BASIC_ERROR_INVALID_DATA_ID;
	}

	// Not sure about this
	// nph.clear_message_selected(id);

	const auto msg_pair = opt_msg.value();
	const auto msg = msg_pair->second;

	const u32 orig_size = *size;
	const u32 size_to_copy = std::min(static_cast<u32>(msg.data.size()), orig_size);
	memcpy(buffer.get_ptr(), msg.data.data(), size_to_copy);

	sceNp.trace("Message Data received:\n%s", fmt::buf_to_hexstring(static_cast<u8*>(buffer.get_ptr()), size_to_copy));

	*size = size_to_copy;
	if (size_to_copy < msg.data.size())
	{
		return SCE_NP_BASIC_ERROR_DATA_LOST;
	}

	return CELL_OK;
}

error_code sceNpBasicRecvMessageCustom(u16 mainType, u32 recvOptions, sys_memory_container_t containerId)
{
	sceNp.warning("sceNpBasicRecvMessageCustom(mainType=%d, recvOptions=%d, containerId=%d)", mainType, recvOptions, containerId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	// TODO: SCE_NP_BASIC_ERROR_NOT_SUPPORTED

	if ((recvOptions & ~SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_ALL_OPTIONS))
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	bool result = false;

	input::SetIntercepted(true);

	SceNpBasicMessageRecvAction recv_result{};
	u64 chosen_msg_id{};

	Emu.BlockingCallFromMainThread([=, &result, &recv_result, &chosen_msg_id]()
	{
		auto recv_dlg = Emu.GetCallbacks().get_recvmessage_dialog();
		result = recv_dlg->Exec(static_cast<SceNpBasicMessageMainType>(mainType), static_cast<SceNpBasicMessageRecvOptions>(recvOptions), recv_result, chosen_msg_id);
	});

	input::SetIntercepted(false);

	if (!result)
	{
		return SCE_NP_BASIC_ERROR_CANCEL;
	}

	const auto msg_pair = nph.get_message(chosen_msg_id).value();
	const auto& msg     = msg_pair->second;

	const u32 event_to_send = (mainType == SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE) ? SCE_NP_BASIC_EVENT_RECV_INVITATION_RESULT : SCE_NP_BASIC_EVENT_RECV_CUSTOM_DATA_RESULT;
	np::basic_event to_add{};
	to_add.event = event_to_send;
	strcpy_trunc(to_add.from.userId.handle.data, msg_pair->first);
	strcpy_trunc(to_add.from.name.data, msg_pair->first);
	to_add.data.resize(sizeof(SceNpBasicExtendedAttachmentData));
	SceNpBasicExtendedAttachmentData* att_data = reinterpret_cast<SceNpBasicExtendedAttachmentData*>(to_add.data.data());
	att_data->flags                            = 0; // ?
	att_data->msgId                            = chosen_msg_id;
	att_data->data.id = (mainType == SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE) ? SCE_NP_BASIC_SELECTED_INVITATION_DATA : SCE_NP_BASIC_SELECTED_MESSAGE_DATA;
	att_data->data.size                        = static_cast<u32>(msg.data.size());
	att_data->userAction                       = recv_result;
	att_data->markedAsUsed                     = (recvOptions & SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_PRESERVE) ? 0 : 1;

	extra_nps::print_SceNpBasicExtendedAttachmentData(att_data);

	nph.set_message_selected(att_data->data.id, chosen_msg_id);

	// Is this sent if used from home menu but not from sceNpBasicRecvMessageCustom, not sure
	// sysutil_send_system_cmd(CELL_SYSUTIL_NP_INVITATION_SELECTED, 0);

	nph.queue_basic_event(to_add);
	nph.send_basic_event(event_to_send, 0, 0);

	return CELL_OK;
}

error_code sceNpBasicMarkMessageAsUsed(SceNpBasicMessageId msgId)
{
	sceNp.todo("sceNpBasicMarkMessageAsUsed(msgId=%d)", msgId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	//if (!msgId > ?)
	//{
	//	return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	//}

	return CELL_OK;
}

error_code sceNpBasicAbortGui()
{
	sceNp.todo("sceNpBasicAbortGui()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	// TODO: abort GUI interaction

	return CELL_OK;
}

error_code sceNpBasicAddFriend(vm::cptr<SceNpId> contact, vm::cptr<char> body, sys_memory_container_t containerId)
{
	sceNp.todo("sceNpBasicAddFriend(contact=*0x%x, body=%s, containerId=%d)", contact, body, containerId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!contact || contact->handle.data[0] == '\0')
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: SCE_NP_BASIC_ERROR_NOT_SUPPORTED, might be in between argument checks

	if (strlen(body.get_ptr()) > SCE_NP_BASIC_BODY_CHARACTER_MAX)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return not_an_error(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendListEntryCount(vm::ptr<u32> count)
{
	sceNp.trace("sceNpBasicGetFriendListEntryCount(count=*0x%x)", count);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	*count = nph.get_num_friends();

	return CELL_OK;
}

error_code sceNpBasicGetFriendListEntry(u32 index, vm::ptr<SceNpId> npid)
{
	sceNp.trace("sceNpBasicGetFriendListEntry(index=%d, npid=*0x%x)", index, npid);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_BASIC_ERROR_BUSY;
	}

	const auto [error, res_npid] = nph.get_friend_by_index(index);

	if (error)
	{
		return error;
	}

	memcpy(npid.get_ptr(), &res_npid.value(), sizeof(SceNpId));

	return CELL_OK;
}

error_code sceNpBasicGetFriendPresenceByIndex(u32 index, vm::ptr<SceNpUserInfo> user, vm::ptr<SceNpBasicPresenceDetails> pres, u32 options)
{
	sceNp.todo("sceNpBasicGetFriendPresenceByIndex(index=%d, user=*0x%x, pres=*0x%x, options=%d)", index, user, pres, options);

	if (!pres)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!user)
	{
		// TODO: check index and (options & SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS) depending on fw
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendPresenceByIndex2(u32 index, vm::ptr<SceNpUserInfo> user, vm::ptr<SceNpBasicPresenceDetails2> pres, u32 options)
{
	sceNp.todo("sceNpBasicGetFriendPresenceByIndex2(index=%d, user=*0x%x, pres=*0x%x, options=%d)", index, user, pres, options);

	if (!pres)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!user)
	{
		// TODO: check index and (options & SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS) depending on fw
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendPresenceByNpId(vm::cptr<SceNpId> npid, vm::ptr<SceNpBasicPresenceDetails> pres, u32 options)
{
	sceNp.todo("sceNpBasicGetFriendPresenceByNpId(npid=*0x%x, pres=*0x%x, options=%d)", npid, pres, options);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!pres)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		// TODO: check (options & SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS) depending on fw
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendPresenceByNpId2(vm::cptr<SceNpId> npid, vm::ptr<SceNpBasicPresenceDetails2> pres, u32 options)
{
	sceNp.todo("sceNpBasicGetFriendPresenceByNpId2(npid=*0x%x, pres=*0x%x, options=%d)", npid, pres, options);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!pres)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		// TODO: check (options & SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS) depending on fw
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicAddPlayersHistory(vm::cptr<SceNpId> npid, vm::ptr<char> description)
{
	sceNp.todo("sceNpBasicAddPlayersHistory(npid=*0x%x, description=*0x%x)", npid, description);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid || npid->handle.data[0] == '\0')
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (description && strlen(description.get_ptr()) > SCE_NP_BASIC_DESCRIPTION_CHARACTER_MAX)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpBasicAddPlayersHistoryAsync(vm::cptr<SceNpId> npids, u32 count, vm::ptr<char> description, vm::ptr<u32> reqId)
{
	sceNp.todo("sceNpBasicAddPlayersHistoryAsync(npids=*0x%x, count=%d, description=*0x%x, reqId=*0x%x)", npids, count, description, reqId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (count > SCE_NP_BASIC_PLAYER_HISTORY_MAX_PLAYERS)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (!npids)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	for (u32 i = 0; i < count; i++)
	{
		if (npids[i].handle.data[0] == '\0')
		{
			return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
		}
	}

	if (description && strlen(description.get_ptr()) > SCE_NP_BASIC_DESCRIPTION_CHARACTER_MAX)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	auto req_id = nph.add_players_to_history(npids, count);

	if (reqId)
	{
		*reqId = req_id;
	}

	return CELL_OK;
}

error_code sceNpBasicGetPlayersHistoryEntryCount(u32 options, vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetPlayersHistoryEntryCount(options=%d, count=*0x%x)", options, count);

	if (options > SCE_NP_BASIC_PLAYERS_HISTORY_OPTIONS_ALL)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are players histories
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetPlayersHistoryEntry(u32 options, u32 index, vm::ptr<SceNpId> npid)
{
	sceNp.warning("sceNpBasicGetPlayersHistoryEntry(options=%d, index=%d, npid=*0x%x)", options, index, npid);

	if (options > SCE_NP_BASIC_PLAYERS_HISTORY_OPTIONS_ALL)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!npid)
	{
		// TODO: Check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicAddBlockListEntry(vm::cptr<SceNpId> npid)
{
	sceNp.warning("sceNpBasicAddBlockListEntry(npid=*0x%x)", npid);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!npid || npid->handle.data[0] == '\0')
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return not_an_error(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
	}

	return CELL_OK;
}

error_code sceNpBasicGetBlockListEntryCount(vm::ptr<u32> count)
{
	sceNp.warning("sceNpBasicGetBlockListEntryCount(count=*0x%x)", count);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are block lists
	*count = nph.get_num_blocks();

	return CELL_OK;
}

error_code sceNpBasicGetBlockListEntry(u32 index, vm::ptr<SceNpId> npid)
{
	sceNp.todo("sceNpBasicGetBlockListEntry(index=%d, npid=*0x%x)", index, npid);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetMessageAttachmentEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetMessageAttachmentEntryCount(count=*0x%x)", count);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are message attachments
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetMessageAttachmentEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetMessageAttachmentEntry(index=%d, from=*0x%x)", index, from);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetCustomInvitationEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetCustomInvitationEntryCount(count=*0x%x)", count);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are custom invitations
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetCustomInvitationEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetCustomInvitationEntry(index=%d, from=*0x%x)", index, from);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetMatchingInvitationEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetMatchingInvitationEntryCount(count=*0x%x)", count);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are matching invitations
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetMatchingInvitationEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetMatchingInvitationEntry(index=%d, from=*0x%x)", index, from);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetClanMessageEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetClanMessageEntryCount(count=*0x%x)", count);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are clan messages
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetClanMessageEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetClanMessageEntry(index=%d, from=*0x%x)", index, from);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetMessageEntryCount(u32 type, vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetMessageEntryCount(type=%d, count=*0x%x)", type, count);

	// TODO: verify this check and its location
	if (type > SCE_NP_BASIC_MESSAGE_INFO_TYPE_BOOTABLE_CUSTOM_DATA_MESSAGE)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are messages
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetMessageEntry(u32 type, u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetMessageEntry(type=%d, index=%d, from=*0x%x)", type, index, from);

	// TODO: verify this check and its location
	if (type > SCE_NP_BASIC_MESSAGE_INFO_TYPE_BOOTABLE_CUSTOM_DATA_MESSAGE)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetEvent(vm::ptr<s32> event, vm::ptr<SceNpUserInfo> from, vm::ptr<u8> data, vm::ptr<u32> size)
{
	sceNp.warning("sceNpBasicGetEvent(event=*0x%x, from=*0x%x, data=*0x%x, size=*0x%x)", event, from, data, size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!nph.basic_handler.registered)
	{
		return SCE_NP_BASIC_ERROR_NOT_REGISTERED;
	}

	if (!event || !from || !data || !size)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	//*event = SCE_NP_BASIC_EVENT_OFFLINE; // This event only indicates a contact is offline, not the current status of the connection

	return nph.get_basic_event(event, from, data, size);
}

error_code sceNpCommerceCreateCtx(u32 version, vm::ptr<SceNpId> npId, vm::ptr<SceNpCommerceHandler> handler, vm::ptr<void> arg, vm::ptr<u32> ctx_id)
{
	sceNp.todo("sceNpCommerceCreateCtx(version=%d, event=*0x%x, from=*0x%x, arg=*0x%x, ctx_id=*0x%x)", version, npId, handler, arg, ctx_id);
	return CELL_OK;
}

error_code sceNpCommerceDestroyCtx(u32 ctx_id)
{
	sceNp.todo("sceNpCommerceDestroyCtx(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerceInitProductCategory(vm::ptr<SceNpCommerceProductCategory> pc, vm::cptr<void> data, u64 data_size)
{
	sceNp.todo("sceNpCommerceInitProductCategory(pc=*0x%x, data=*0x%x, data_size=%d)", pc, data, data_size);
	return CELL_OK;
}

void sceNpCommerceDestroyProductCategory(vm::ptr<SceNpCommerceProductCategory> pc)
{
	sceNp.todo("sceNpCommerceDestroyProductCategory(pc=*0x%x)", pc);
}

error_code sceNpCommerceGetProductCategoryStart(u32 ctx_id, vm::cptr<char> category_id, s32 lang_code, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpCommerceGetProductCategoryStart(ctx_id=%d, category_id=%s, lang_code=%d, req_id=*0x%x)", ctx_id, category_id, lang_code, req_id);
	return CELL_OK;
}

error_code sceNpCommerceGetProductCategoryFinish(u32 req_id)
{
	sceNp.todo("sceNpCommerceGetProductCategoryFinish(req_id=%d)", req_id);
	return CELL_OK;
}

error_code sceNpCommerceGetProductCategoryResult(u32 req_id, vm::ptr<void> buf, u64 buf_size, vm::ptr<u64> fill_size)
{
	sceNp.todo("sceNpCommerceGetProductCategoryResult(req_id=%d, buf=*0x%x, buf_size=%d, fill_size=*0x%x)", req_id, buf, buf_size, fill_size);
	return CELL_OK;
}

error_code sceNpCommerceGetProductCategoryAbort(u32 req_id)
{
	sceNp.todo("sceNpCommerceGetProductCategoryAbort(req_id=%d)", req_id);
	return CELL_OK;
}

vm::cptr<char> sceNpCommerceGetProductId(vm::ptr<SceNpCommerceProductSkuInfo> info)
{
	sceNp.todo("sceNpCommerceGetProductId(info=*0x%x)", info);
	return vm::null;
}

vm::cptr<char> sceNpCommerceGetProductName(vm::ptr<SceNpCommerceProductSkuInfo> info)
{
	sceNp.todo("sceNpCommerceGetProductName(info=*0x%x)", info);
	return vm::null;
}

vm::cptr<char> sceNpCommerceGetCategoryDescription(vm::ptr<SceNpCommerceCategoryInfo> info)
{
	sceNp.todo("sceNpCommerceGetCategoryDescription(info=*0x%x)", info);
	return vm::null;
}

vm::cptr<char> sceNpCommerceGetCategoryId(vm::ptr<SceNpCommerceCategoryInfo> info)
{
	sceNp.todo("sceNpCommerceGetCategoryId(info=*0x%x)", info);
	return vm::null;
}

vm::cptr<char> sceNpCommerceGetCategoryImageURL(vm::ptr<SceNpCommerceCategoryInfo> info)
{
	sceNp.todo("sceNpCommerceGetCategoryImageURL(info=*0x%x)", info);
	return vm::null;
}

error_code sceNpCommerceGetCategoryInfo(vm::ptr<SceNpCommerceProductCategory> pc, vm::ptr<SceNpCommerceCategoryInfo> info)
{
	sceNp.todo("sceNpCommerceGetCategoryInfo(pc=*0x%x, info=*0x%x)", pc, info);
	return CELL_OK;
}

vm::cptr<char> sceNpCommerceGetCategoryName(vm::ptr<SceNpCommerceCategoryInfo> info)
{
	sceNp.todo("sceNpCommerceGetCategoryName(info=*0x%x)", info);
	return vm::null;
}

vm::cptr<char> sceNpCommerceGetCurrencyCode(vm::ptr<SceNpCommerceCurrencyInfo> info)
{
	sceNp.todo("sceNpCommerceGetCurrencyCode(info=*0x%x)", info);
	return vm::null;
}

u32 sceNpCommerceGetCurrencyDecimals(vm::ptr<SceNpCommerceCurrencyInfo> info)
{
	sceNp.todo("sceNpCommerceGetCurrencyDecimals(info=*0x%x)", info);
	return 0;
}

error_code sceNpCommerceGetCurrencyInfo(vm::ptr<SceNpCommerceProductCategory> pc, vm::ptr<SceNpCommerceCurrencyInfo> info)
{
	sceNp.todo("sceNpCommerceGetCurrencyInfo(pc=*0x%x, info=*0x%x)", pc, info);
	return CELL_OK;
}

error_code sceNpCommerceGetNumOfChildCategory(vm::ptr<SceNpCommerceProductCategory> pc, vm::ptr<u32> num)
{
	sceNp.todo("sceNpCommerceGetNumOfChildCategory(pc=*0x%x, num=*0x%x)", pc, num);
	return CELL_OK;
}

error_code sceNpCommerceGetNumOfChildProductSku(vm::ptr<SceNpCommerceProductCategory> pc, vm::ptr<u32> num)
{
	sceNp.todo("sceNpCommerceGetNumOfChildProductSku(pc=*0x%x, num=*0x%x)", pc, num);
	return CELL_OK;
}

vm::cptr<char> sceNpCommerceGetSkuDescription(vm::ptr<SceNpCommerceProductSkuInfo> info)
{
	sceNp.todo("sceNpCommerceGetSkuDescription(info=*0x%x)", info);
	return vm::null;
}

vm::cptr<char> sceNpCommerceGetSkuId(vm::ptr<SceNpCommerceProductSkuInfo> info)
{
	sceNp.todo("sceNpCommerceGetSkuId(info=*0x%x)", info);
	return vm::null;
}

vm::cptr<char> sceNpCommerceGetSkuImageURL(vm::ptr<SceNpCommerceProductSkuInfo> info)
{
	sceNp.todo("sceNpCommerceGetSkuImageURL(info=*0x%x)", info);
	return vm::null;
}

vm::cptr<char> sceNpCommerceGetSkuName(vm::ptr<SceNpCommerceProductSkuInfo> info)
{
	sceNp.todo("sceNpCommerceGetSkuName(info=*0x%x)", info);
	return vm::null;
}

void sceNpCommerceGetSkuPrice(vm::ptr<SceNpCommerceProductSkuInfo> info, vm::ptr<SceNpCommercePrice> price)
{
	sceNp.todo("sceNpCommerceGetSkuPrice(info=*0x%x, price=*0x%x)", info, price);
}

vm::cptr<char> sceNpCommerceGetSkuUserData(vm::ptr<SceNpCommerceProductSkuInfo> info)
{
	sceNp.todo("sceNpCommerceGetSkuUserData(info=*0x%x)", info);
	return vm::null;
}

error_code sceNpCommerceSetDataFlagStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpCommerceGetDataFlagStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpCommerceSetDataFlagFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpCommerceGetDataFlagFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpCommerceGetDataFlagState()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpCommerceGetDataFlagAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpCommerceGetChildCategoryInfo(vm::ptr<SceNpCommerceProductCategory> pc, u32 child_index, vm::ptr<SceNpCommerceCategoryInfo> info)
{
	sceNp.todo("sceNpCommerceGetChildCategoryInfo(pc=*0x%x, child_index=%d, info=*0x%x)", pc, child_index, info);
	return CELL_OK;
}

error_code sceNpCommerceGetChildProductSkuInfo(vm::ptr<SceNpCommerceProductCategory> pc, u32 child_index, vm::ptr<SceNpCommerceProductSkuInfo> info)
{
	sceNp.todo("sceNpCommerceGetChildProductSkuInfo(pc=*0x%x, child_index=%d, info=*0x%x)", pc, child_index, info);
	return CELL_OK;
}

error_code sceNpCommerceDoCheckoutStartAsync(u32 ctx_id, vm::cpptr<char> sku_ids, u32 sku_num, sys_memory_container_t container, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpCommerceDoCheckoutStartAsync(ctx_id=%d, sku_ids=*0x%x, sku_num=%d, container=%d, req_id=*0x%x)", ctx_id, sku_ids, sku_num, container, req_id);
	return CELL_OK;
}

error_code sceNpCommerceDoCheckoutFinishAsync(u32 req_id)
{
	sceNp.todo("sceNpCommerceDoCheckoutFinishAsync(req_id=%d)", req_id);
	return CELL_OK;
}

error_code sceNpCustomMenuRegisterActions(vm::cptr<SceNpCustomMenu> menu, vm::ptr<SceNpCustomMenuEventHandler> handler, vm::ptr<void> userArg, u64 options)
{
	sceNp.todo("sceNpCustomMenuRegisterActions(menu=*0x%x, handler=*0x%x, userArg=*0x%x, options=0x%x)", menu, handler, userArg, options);

	// TODO: Is there any error if options is not 0 ?

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	if (!menu || !handler) // TODO: handler check might come later
	{
		return SCE_NP_CUSTOM_MENU_ERROR_INVALID_ARGUMENT;
	}

	if (menu->numActions > SCE_NP_CUSTOM_MENU_ACTION_ITEMS_TOTAL_MAX)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_EXCEEDS_MAX;
	}

	std::vector<np::np_handler::custom_menu_action> actions;

	for (u32 i = 0; i < menu->numActions; i++)
	{
		if (!menu->actions[i].name)
		{
			return SCE_NP_CUSTOM_MENU_ERROR_INVALID_ARGUMENT;
		}

		// TODO: Is there any error if menu->actions[i].options is not 0 ?

		// TODO: check name
		if (false)
		{
			return SCE_NP_UTIL_ERROR_INVALID_CHARACTER;
		}

		if (!memchr(menu->actions[i].name.get_ptr(), '\0', SCE_NP_CUSTOM_MENU_ACTION_CHARACTER_MAX))
		{
			return SCE_NP_CUSTOM_MENU_ERROR_EXCEEDS_MAX;
		}

		np::np_handler::custom_menu_action action{};
		action.id   = static_cast<s32>(actions.size());
		action.mask = menu->actions[i].mask;
		action.name = menu->actions[i].name.get_ptr();

		sceNp.notice("Registering menu action: id=%d, mask=0x%x, name=%s", action.id, action.mask, action.name);

		actions.push_back(std::move(action));
	}

	// TODO: add the custom menu to the friendlist and profile dialogs
	std::lock_guard lock(nph.mutex_custom_menu);
	nph.custom_menu_handler = handler;
	nph.custom_menu_user_arg = userArg;
	nph.custom_menu_actions = std::move(actions);
	nph.custom_menu_registered = true;
	nph.custom_menu_activation = {};
	nph.custom_menu_exception_list = {};

	return CELL_OK;
}

error_code sceNpCustomMenuActionSetActivation(vm::cptr<SceNpCustomMenuIndexArray> array, u64 options)
{
	sceNp.todo("sceNpCustomMenuActionSetActivation(array=*0x%x, options=0x%x)", array, options);

	// TODO: Is there any error if options is not 0 ?

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	std::lock_guard lock(nph.mutex_custom_menu);

	if (!nph.custom_menu_registered)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_REGISTERED;
	}

	if (!array)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_INVALID_ARGUMENT;
	}

	nph.custom_menu_activation = *array;

	return CELL_OK;
}

error_code sceNpCustomMenuRegisterExceptionList(vm::cptr<SceNpCustomMenuActionExceptions> items, u32 numItems, u64 options)
{
	sceNp.todo("sceNpCustomMenuRegisterExceptionList(items=*0x%x, numItems=%d, options=0x%x)", items, numItems, options);

	// TODO: Is there any error if options is not 0 ?

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	std::lock_guard lock(nph.mutex_custom_menu);

	if (!nph.custom_menu_registered)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_REGISTERED;
	}

	if (numItems > SCE_NP_CUSTOM_MENU_EXCEPTION_ITEMS_MAX)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_EXCEEDS_MAX;
	}

	if (!items)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_INVALID_ARGUMENT;
	}

	nph.custom_menu_exception_list.clear();

	for (u32 i = 0; i < numItems; i++)
	{
		// TODO: Are the exceptions checked ?
		nph.custom_menu_exception_list.push_back(items[i]);
	}

	return CELL_OK;
}

error_code sceNpFriendlist(vm::ptr<SceNpFriendlistResultHandler> resultHandler, vm::ptr<void> userArg, sys_memory_container_t containerId)
{
	sceNp.warning("sceNpFriendlist(resultHandler=*0x%x, userArg=*0x%x, containerId=%d)", resultHandler, userArg, containerId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_FRIENDLIST_ERROR_NOT_INITIALIZED;
	}

	// TODO: SCE_NP_FRIENDLIST_ERROR_BUSY

	if (!resultHandler)
	{
		return SCE_NP_FRIENDLIST_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpFriendlistCustom(SceNpFriendlistCustomOptions options, vm::ptr<SceNpFriendlistResultHandler> resultHandler, vm::ptr<void> userArg, sys_memory_container_t containerId)
{
	sceNp.warning("sceNpFriendlistCustom(options=0x%x, resultHandler=*0x%x, userArg=*0x%x, containerId=%d)", options, resultHandler, userArg, containerId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_FRIENDLIST_ERROR_NOT_INITIALIZED;
	}

	// TODO: SCE_NP_FRIENDLIST_ERROR_BUSY

	if (!resultHandler)
	{
		return SCE_NP_FRIENDLIST_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpFriendlistAbortGui()
{
	sceNp.todo("sceNpFriendlistAbortGui()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_FRIENDLIST_ERROR_NOT_INITIALIZED;
	}

	// TODO: abort friendlist GUI interaction

	return CELL_OK;
}

error_code sceNpLookupInit()
{
	sceNp.warning("sceNpLookupInit()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph.is_NP_Lookup_init = true;

	return CELL_OK;
}

error_code sceNpLookupTerm()
{
	sceNp.todo("sceNpLookupTerm()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph.is_NP_Lookup_init = false;

	return CELL_OK;
}

error_code sceNpLookupCreateTitleCtx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpId> selfNpId)
{
	sceNp.warning("sceNpLookupCreateTitleCtx(communicationId=*0x%x(%s), selfNpId=0x%x)", communicationId, communicationId ? communicationId->data : "", selfNpId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!communicationId || !selfNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	return not_an_error(create_lookup_title_context(communicationId));
}

error_code sceNpLookupDestroyTitleCtx(s32 titleCtxId)
{
	sceNp.warning("sceNpLookupDestroyTitleCtx(titleCtxId=%d)", titleCtxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!destroy_lookup_title_context(titleCtxId))
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;

	return CELL_OK;
}

error_code sceNpLookupCreateTransactionCtx(s32 titleCtxId)
{
	sceNp.warning("sceNpLookupCreateTransactionCtx(titleCtxId=%d)", titleCtxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return not_an_error(create_lookup_transaction_context(titleCtxId));
}

error_code sceNpLookupDestroyTransactionCtx(s32 transId)
{
	sceNp.warning("sceNpLookupDestroyTransactionCtx(transId=%d)", transId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!destroy_lookup_transaction_context(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupSetTimeout(s32 ctxId, usecond_t timeout)
{
	sceNp.todo("sceNpLookupSetTimeout(ctxId=%d, timeout=%d)", ctxId, timeout);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (timeout < 10000000) // 10 seconds
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpLookupAbortTransaction(s32 transId)
{
	sceNp.todo("sceNpLookupAbortTransaction(transId=%d)", transId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupWaitAsync(s32 transId, vm::ptr<s32> result)
{
	sceNp.todo("sceNpLookupWaitAsync(transId=%d, result=%d)", transId, result);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupPollAsync(s32 transId, vm::ptr<s32> result)
{
	sceNp.todo("sceNpLookupPollAsync(transId=%d, result=*0x%x)", transId, result);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	*result = 0;

	return CELL_OK;
}

error_code sceNpLookupNpId(s32 transId, vm::cptr<SceNpOnlineId> onlineId, vm::ptr<SceNpId> npId, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupNpId(transId=%d, onlineId=*0x%x, npId=*0x%x, option=*0x%x)", transId, onlineId, npId, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!onlineId || !npId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupNpIdAsync(s32 transId, vm::ptr<SceNpOnlineId> onlineId, vm::ptr<SceNpId> npId, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupNpIdAsync(transId=%d, onlineId=*0x%x, npId=*0x%x, prio=%d, option=*0x%x)", transId, onlineId, npId, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!onlineId || !npId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupUserProfile(s32 transId, vm::cptr<SceNpId> npId, vm::ptr<SceNpUserInfo> userInfo, vm::ptr<SceNpAboutMe> aboutMe, vm::ptr<SceNpMyLanguages> languages,
    vm::ptr<SceNpCountryCode> countryCode, vm::ptr<SceNpAvatarImage> avatarImage, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupUserProfile(transId=%d, npId=*0x%x, userInfo=*0x%x, aboutMe=*0x%x, languages=*0x%x, countryCode=*0x%x, avatarImage=*0x%x, option=*0x%x)", transId, npId, userInfo, aboutMe,
	    languages, countryCode, avatarImage, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupUserProfileAsync(s32 transId, vm::cptr<SceNpId> npId, vm::ptr<SceNpUserInfo> userInfo, vm::ptr<SceNpAboutMe> aboutMe, vm::ptr<SceNpMyLanguages> languages,
    vm::ptr<SceNpCountryCode> countryCode, vm::ptr<SceNpAvatarImage> avatarImage, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupUserProfile(transId=%d, npId=*0x%x, userInfo=*0x%x, aboutMe=*0x%x, languages=*0x%x, countryCode=*0x%x, avatarImage=*0x%x, prio=%d, option=*0x%x)", transId, npId, userInfo,
	    aboutMe, languages, countryCode, avatarImage, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupUserProfileWithAvatarSize(s32 transId, s32 avatarSizeType, vm::cptr<SceNpId> npId, vm::ptr<SceNpUserInfo> userInfo, vm::ptr<SceNpAboutMe> aboutMe,
    vm::ptr<SceNpMyLanguages> languages, vm::ptr<SceNpCountryCode> countryCode, vm::ptr<void> avatarImageData, u64 avatarImageDataMaxSize, vm::ptr<u64> avatarImageDataSize, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupUserProfileWithAvatarSize(transId=%d, avatarSizeType=%d, npId=*0x%x, userInfo=*0x%x, aboutMe=*0x%x, languages=*0x%x, countryCode=*0x%x, avatarImageData=*0x%x, "
	           "avatarImageDataMaxSize=%d, avatarImageDataSize=*0x%x, option=*0x%x)",
	    transId, avatarSizeType, npId, userInfo, aboutMe, languages, countryCode, avatarImageData, avatarImageDataMaxSize, avatarImageDataSize, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupUserProfileWithAvatarSizeAsync(s32 transId, s32 avatarSizeType, vm::cptr<SceNpId> npId, vm::ptr<SceNpUserInfo> userInfo, vm::ptr<SceNpAboutMe> aboutMe,
    vm::ptr<SceNpMyLanguages> languages, vm::ptr<SceNpCountryCode> countryCode, vm::ptr<void> avatarImageData, u64 avatarImageDataMaxSize, vm::ptr<u64> avatarImageDataSize, s32 prio,
    vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupUserProfileWithAvatarSizeAsync(transId=%d, avatarSizeType=%d, npId=*0x%x, userInfo=*0x%x, aboutMe=*0x%x, languages=*0x%x, countryCode=*0x%x, avatarImageData=*0x%x, "
	           "avatarImageDataMaxSize=%d, avatarImageDataSize=*0x%x, prio=%d, option=*0x%x)",
	    transId, avatarSizeType, npId, userInfo, aboutMe, languages, countryCode, avatarImageData, avatarImageDataMaxSize, avatarImageDataSize, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupAvatarImage(s32 transId, vm::ptr<SceNpAvatarUrl> avatarUrl, vm::ptr<SceNpAvatarImage> avatarImage, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupAvatarImage(transId=%d, avatarUrl=*0x%x, avatarImage=*0x%x, option=*0x%x)", transId, avatarUrl, avatarImage, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!avatarUrl || !avatarImage)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupAvatarImageAsync(s32 transId, vm::ptr<SceNpAvatarUrl> avatarUrl, vm::ptr<SceNpAvatarImage> avatarImage, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupAvatarImageAsync(transId=%d, avatarUrl=*0x%x, avatarImage=*0x%x, prio=%d, option=*0x%x)", transId, avatarUrl, avatarImage, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!avatarUrl || !avatarImage)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupTitleStorage()
{
	UNIMPLEMENTED_FUNC(sceNp);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupTitleStorageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupTitleSmallStorage(s32 transId, vm::ptr<void> data, u64 maxSize, vm::ptr<u64> contentLength, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupTitleSmallStorage(transId=%d, data=*0x%x, maxSize=%d, contentLength=*0x%x, option=*0x%x)", transId, data, maxSize, contentLength, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!data)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	//if (something > maxSize)
	//{
	//	return SCE_NP_COMMUNITY_ERROR_BODY_TOO_LARGE;
	//}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupTitleSmallStorageAsync(s32 transId, vm::ptr<void> data, u64 maxSize, vm::ptr<u64> contentLength, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupTitleSmallStorageAsync(transId=%d, data=*0x%x, maxSize=%d, contentLength=*0x%x, prio=%d, option=*0x%x)", transId, data, maxSize, contentLength, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!data)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	//	if (something > maxSize)
	//{
	//	return SCE_NP_COMMUNITY_ERROR_BODY_TOO_LARGE;
	//}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	// TSS are game specific data we don't have access to, set buf to 0, return size 0
	std::memset(data.get_ptr(), 0, maxSize);
	*contentLength = 0;

	return CELL_OK;
}

error_code sceNpManagerRegisterCallback(vm::ptr<SceNpManagerCallback> callback, vm::ptr<void> arg)
{
	sceNp.warning("sceNpManagerRegisterCallback(callback=*0x%x, arg=*0x%x)", callback, arg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!callback)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	nph.manager_cb = callback;
	nph.manager_cb_arg = arg;

	return CELL_OK;
}

error_code sceNpManagerUnregisterCallback()
{
	sceNp.warning("sceNpManagerUnregisterCallback()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph.manager_cb.set(0);

	return CELL_OK;
}

error_code sceNpManagerGetStatus(vm::ptr<s32> status)
{
	sceNp.trace("sceNpManagerGetStatus(status=*0x%x)", status);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		//return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!status)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	*status = nph.get_psn_status();

	return CELL_OK;
}

error_code sceNpManagerGetNetworkTime(vm::ptr<CellRtcTick> pTick)
{
	sceNp.warning("sceNpManagerGetNetworkTime(pTick=*0x%x)", pTick);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		//return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!pTick)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	vm::var<s64> sec;
	vm::var<s64> nsec;

	error_code ret = sys_time_get_current_time(sec, nsec);

	if (ret != CELL_OK)
	{
		return ret;
	}

	// Taken from cellRtc
	pTick->tick = *nsec / 1000 + *sec * cellRtcGetTickResolution() + 62135596800000000ULL;

	return CELL_OK;
}

error_code sceNpManagerGetOnlineId(vm::ptr<SceNpOnlineId> onlineId)
{
	sceNp.warning("sceNpManagerGetOnlineId(onlineId=*0x%x)", onlineId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		//return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!onlineId)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memcpy(onlineId.get_ptr(), &nph.get_online_id(), onlineId.size());

	return CELL_OK;
}

error_code sceNpManagerGetNpId(ppu_thread&, vm::ptr<SceNpId> npId)
{
	sceNp.trace("sceNpManagerGetNpId(npId=*0x%x)", npId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	// if (!nph.is_NP_init)
	// {
	// 	return SCE_NP_ERROR_NOT_INITIALIZED;
	// }

	if (!npId)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memcpy(npId.get_ptr(), &nph.get_npid(), npId.size());

	return CELL_OK;
}

error_code sceNpManagerGetOnlineName(vm::ptr<SceNpOnlineName> onlineName)
{
	sceNp.warning("sceNpManagerGetOnlineName(onlineName=*0x%x)", onlineName);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!onlineName)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memcpy(onlineName.get_ptr(), &nph.get_online_name(), onlineName.size());

	return CELL_OK;
}

error_code sceNpManagerGetAvatarUrl(vm::ptr<SceNpAvatarUrl> avatarUrl)
{
	sceNp.warning("sceNpManagerGetAvatarUrl(avatarUrl=*0x%x)", avatarUrl);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!avatarUrl)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memcpy(avatarUrl.get_ptr(), &nph.get_avatar_url(), avatarUrl.size());

	return CELL_OK;
}

error_code sceNpManagerGetMyLanguages(vm::ptr<SceNpMyLanguages> myLanguages)
{
	sceNp.warning("sceNpManagerGetMyLanguages(myLanguages=*0x%x)", myLanguages);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!myLanguages)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	myLanguages->language1 = SCE_NP_LANG_ENGLISH_US;
	myLanguages->language2 = g_cfg.sys.language != CELL_SYSUTIL_LANG_ENGLISH_US ? g_cfg.sys.language : -1;
	myLanguages->language3 = -1;

	return CELL_OK;
}

error_code sceNpManagerGetAccountRegion(vm::ptr<SceNpCountryCode> countryCode, vm::ptr<s32> language)
{
	sceNp.warning("sceNpManagerGetAccountRegion(countryCode=*0x%x, language=*0x%x)", countryCode, language);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!countryCode || !language)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memset(countryCode.get_ptr(), 0, sizeof(countryCode));
	countryCode->data[0] = 'u';
	countryCode->data[1] = 's';

	*language = CELL_SYSUTIL_LANG_ENGLISH_US;

	return CELL_OK;
}

error_code sceNpManagerGetAccountAge(vm::ptr<s32> age)
{
	sceNp.warning("sceNpManagerGetAccountAge(age=*0x%x)", age);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!age)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	*age = 18;

	return CELL_OK;
}

error_code sceNpManagerGetContentRatingFlag(vm::ptr<s32> isRestricted, vm::ptr<s32> age)
{
	sceNp.warning("sceNpManagerGetContentRatingFlag(isRestricted=*0x%x, age=*0x%x)", isRestricted, age);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!isRestricted || !age)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	// TODO: read user's parental control information
	*isRestricted = 0;
	*age          = 18;

	return CELL_OK;
}

error_code sceNpManagerGetChatRestrictionFlag(vm::ptr<s32> isRestricted)
{
	sceNp.trace("sceNpManagerGetChatRestrictionFlag(isRestricted=*0x%x)", isRestricted);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!isRestricted)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	// TODO: read user's parental control information
	*isRestricted = 0;

	return CELL_OK;
}

error_code sceNpManagerGetCachedInfo(CellSysutilUserId userId, vm::ptr<SceNpManagerCacheParam> param)
{
	sceNp.warning("sceNpManagerGetCachedInfo(userId=%d, param=*0x%x)", userId, param);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!param)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (userId != CELL_SYSUTIL_USERID_CURRENT && userId != Emu.GetUsrId())
	{
		return CELL_ENOENT;
	}

	param->size = sizeof(SceNpManagerCacheParam);
	param->onlineId = nph.get_online_id();
	param->npId = nph.get_npid();
	param->onlineName = nph.get_online_name();
	param->avatarUrl = nph.get_avatar_url();

	return CELL_OK;
}

error_code sceNpManagerGetPsHandle(vm::ptr<SceNpOnlineId> onlineId)
{
	sceNp.warning("sceNpManagerGetPsHandle(onlineId=*0x%x)", onlineId);

	return sceNpManagerGetOnlineId(onlineId);
}

error_code sceNpManagerRequestTicket(vm::cptr<SceNpId> npId, vm::cptr<char> serviceId, vm::cptr<void> cookie, u32 cookieSize, vm::cptr<char> entitlementId, u32 consumedCount)
{
	sceNp.warning("sceNpManagerRequestTicket(npId=*0x%x, serviceId=%s, cookie=*0x%x, cookieSize=%d, entitlementId=%s, consumedCount=%d)", npId, serviceId, cookie, cookieSize, entitlementId, consumedCount);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !serviceId || cookieSize > SCE_NP_COOKIE_MAX_SIZE)
	{
		return SCE_NP_AUTH_EINVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	nph.req_ticket(0x00020001, npId.get_ptr(), serviceId.get_ptr(), static_cast<const u8*>(cookie.get_ptr()), cookieSize, entitlementId.get_ptr(), consumedCount);

	return CELL_OK;
}

error_code sceNpManagerRequestTicket2(vm::cptr<SceNpId> npId, vm::cptr<SceNpTicketVersion> version, vm::cptr<char> serviceId,
	vm::cptr<void> cookie, u32 cookieSize, vm::cptr<char> entitlementId, u32 consumedCount)
{
	sceNp.warning("sceNpManagerRequestTicket2(npId=*0x%x, version=*0x%x, serviceId=%s, cookie=*0x%x, cookieSize=%d, entitlementId=%s, consumedCount=%d)", npId, version, serviceId, cookie, cookieSize,
	    entitlementId, consumedCount);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !serviceId || cookieSize > SCE_NP_COOKIE_MAX_SIZE)
	{
		return SCE_NP_AUTH_EINVALID_ARGUMENT;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	nph.req_ticket(0x00020001, npId.get_ptr(), serviceId.get_ptr(), static_cast<const u8*>(cookie.get_ptr()), cookieSize, entitlementId.get_ptr(), consumedCount);

	return CELL_OK;
}

error_code sceNpManagerGetTicket(vm::ptr<void> buffer, vm::ptr<u32> bufferSize)
{
	sceNp.warning("sceNpManagerGetTicket(buffer=*0x%x, bufferSize=*0x%x)", buffer, bufferSize);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!bufferSize)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	const auto& ticket = nph.get_ticket();
	*bufferSize = static_cast<u32>(ticket.size());

	if (!buffer)
	{
		return CELL_OK;
	}

	if (*bufferSize < ticket.size())
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	memcpy(buffer.get_ptr(), ticket.data(), ticket.size());

	return CELL_OK;
}

error_code sceNpManagerGetTicketParam(s32 paramId, vm::ptr<SceNpTicketParam> param)
{
	sceNp.notice("sceNpManagerGetTicketParam(paramId=%d, param=*0x%x)", paramId, param);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!param || paramId < SCE_NP_TICKET_PARAM_SERIAL_ID || paramId > SCE_NP_TICKET_PARAM_SUBJECT_DOB)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	const auto& ticket = nph.get_ticket();

	if (ticket.empty())
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	if (!ticket.get_value(paramId, param))
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

error_code sceNpManagerGetEntitlementIdList(vm::ptr<SceNpEntitlementId> entIdList, u32 entIdListNum)
{
	sceNp.todo("sceNpManagerGetEntitlementIdList(entIdList=*0x%x, entIdListNum=%d)", entIdList, entIdListNum);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return not_an_error(0);
}

error_code sceNpManagerGetEntitlementById(vm::cptr<char> entId, vm::ptr<SceNpEntitlement> ent)
{
	sceNp.todo("sceNpManagerGetEntitlementById(entId=%s, ent=*0x%x)", entId, ent);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!entId)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpManagerGetSigninId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpManagerSubSignin(CellSysutilUserId userId, vm::ptr<SceNpManagerSubSigninCallback> cb_func, vm::ptr<void> cb_arg, s32 flag)
{
	sceNp.todo("sceNpManagerSubSignin(userId=%d, cb_func=*0x%x, cb_arg=*0x%x, flag=%d)", userId, cb_func, cb_arg, flag);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpManagerSubSigninAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpManagerSubSignout(vm::ptr<SceNpId> npId)
{
	sceNp.todo("sceNpManagerSubSignout(npId=*0x%x)", npId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpMatchingCreateCtx(vm::ptr<SceNpId> npId, vm::ptr<SceNpMatchingHandler> handler, vm::ptr<void> arg, vm::ptr<u32> ctx_id)
{
	sceNp.todo("sceNpMatchingCreateCtx(npId=*0x%x, handler=*0x%x, arg=*0x%x, ctx_id=*0x%x)", npId, handler, arg, ctx_id);

	return CELL_OK;
}

error_code sceNpMatchingDestroyCtx(u32 ctx_id)
{
	sceNp.todo("sceNpMatchingDestroyCtx(ctx_id=%d)", ctx_id);

	return CELL_OK;
}

error_code sceNpMatchingGetResult(u32 ctx_id, u32 req_id, vm::ptr<void> buf, vm::ptr<u64> size, vm::ptr<s32> event)
{
	sceNp.todo("sceNpMatchingGetResult(ctx_id=%d, req_id=%d, buf=*0x%x, size=*0x%x, event=*0x%x)", ctx_id, req_id, buf, size, event);

	return CELL_OK;
}

error_code sceNpMatchingGetResultGUI(vm::ptr<void> buf, vm::ptr<u64> size, vm::ptr<s32> event)
{
	sceNp.todo("sceNpMatchingGetResultGUI(buf=*0x%x, size=*0x%x, event=*0x%x)", buf, size, event);

	return CELL_OK;
}

error_code sceNpMatchingSetRoomInfo(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingSetRoomInfo(ctx_id=%d, lobby_id=*0x%x, room_id=*0x%x, attr=*0x%x, req_id=*0x%x)", ctx_id, lobby_id, room_id, attr, req_id);

	return CELL_OK;
}

error_code sceNpMatchingSetRoomInfoNoLimit(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingSetRoomInfoNoLimit(ctx_id=%d, lobby_id=*0x%x, room_id=*0x%x, attr=*0x%x, req_id=*0x%x)", ctx_id, lobby_id, room_id, attr, req_id);

	return CELL_OK;
}

error_code sceNpMatchingGetRoomInfo(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingGetRoomInfo(ctx_id=%d, lobby_id=*0x%x, room_id=*0x%x, attr=*0x%x, req_id=*0x%x)", ctx_id, lobby_id, room_id, attr, req_id);

	return CELL_OK;
}

error_code sceNpMatchingGetRoomInfoNoLimit(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingGetRoomInfoNoLimit(ctx_id=%d, lobby_id=*0x%x, room_id=*0x%x, attr=*0x%x, req_id=*0x%x)", ctx_id, lobby_id, room_id, attr, req_id);

	return CELL_OK;
}

error_code sceNpMatchingSetRoomSearchFlag(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, s32 flag, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingSetRoomSearchFlag(ctx_id=%d, lobby_id=*0x%x, room_id=*0x%x, flag=%d, req_id=*0x%x)", ctx_id, lobby_id, room_id, flag, req_id);

	return CELL_OK;
}

error_code sceNpMatchingGetRoomSearchFlag(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingGetRoomSearchFlag(ctx_id=%d, lobby_id=*0x%x, room_id=*0x%x, req_id=*0x%x)", ctx_id, lobby_id, room_id, req_id);

	return CELL_OK;
}

error_code sceNpMatchingGetRoomMemberListLocal(u32 ctx_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<u64> buflen, vm::ptr<void> buf)
{
	sceNp.todo("sceNpMatchingGetRoomMemberListLocal(ctx_id=%d, room_id=*0x%x, buflen=*0x%x, buf=*0x%x)", ctx_id, room_id, buflen, buf);

	return CELL_OK;
}

error_code sceNpMatchingGetRoomListLimitGUI(u32 ctx_id, vm::ptr<SceNpCommunicationId> communicationId, vm::ptr<SceNpMatchingReqRange> range, vm::ptr<SceNpMatchingSearchCondition> cond,
    vm::ptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo(
	    "sceNpMatchingGetRoomListLimitGUI(ctx_id=%d, communicationId=*0x%x, range=*0x%x, cond=*0x%x, attr=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, communicationId, range, cond, attr, handler, arg);

	return CELL_OK;
}

error_code sceNpMatchingKickRoomMember(u32 ctx_id, vm::cptr<SceNpRoomId> room_id, vm::cptr<SceNpId> user_id, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingKickRoomMember(ctx_id=%d, room_id=*0x%x, user_id=*0x%x, req_id=*0x%x)", ctx_id, room_id, user_id, req_id);

	return CELL_OK;
}

error_code sceNpMatchingKickRoomMemberWithOpt(u32 ctx_id, vm::cptr<SceNpRoomId> room_id, vm::cptr<SceNpId> user_id, vm::cptr<void> opt, s32 opt_len, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingKickRoomMemberWithOpt(ctx_id=%d, room_id=*0x%x, user_id=*0x%x, opt=*0x%x, opt_len=%d, req_id=*0x%x)", ctx_id, room_id, user_id, opt, opt_len, req_id);

	return CELL_OK;
}

error_code sceNpMatchingQuickMatchGUI(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingSearchCondition> cond, s32 available_num, s32 timeout,
    vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpMatchingQuickMatchGUI(ctx_id=%d, communicationId=*0x%x, cond=*0x%x, available_num=%d, timeout=%d, handler=*0x%x, arg=*0x%x)", ctx_id, communicationId, cond, available_num, timeout,
	    handler, arg);

	return CELL_OK;
}

error_code sceNpMatchingSendInvitationGUI(u32 ctx_id, vm::cptr<SceNpRoomId> room_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpId> dsts, s32 num, s32 slot_type,
    vm::cptr<char> subject, vm::cptr<char> body, sys_memory_container_t container, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpMatchingSendInvitationGUI(ctx_id=%d, room_id=*0x%x, communicationId=*0x%x, dsts=*0x%x, num=%d, slot_type=%d, subject=%s, body=%s, container=%d, handler=*0x%x, arg=*0x%x)", ctx_id,
	    room_id, communicationId, dsts, num, slot_type, subject, body, container, handler, arg);

	return CELL_OK;
}

error_code sceNpMatchingAcceptInvitationGUI(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, sys_memory_container_t container, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpMatchingAcceptInvitationGUI(ctx_id=%d, communicationId=*0x%x, container=%d, handler=*0x%x, arg=*0x%x)", ctx_id, communicationId, container, handler, arg);

	return CELL_OK;
}

error_code sceNpMatchingCreateRoomGUI(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpMatchingCreateRoomGUI(ctx_id=%d, communicationId=*0x%x, attr=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, communicationId, attr, handler, arg);

	return CELL_OK;
}

error_code sceNpMatchingJoinRoomGUI(u32 ctx_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpMatchingJoinRoomGUI(ctx_id=%d, room_id=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, room_id, handler, arg);

	return CELL_OK;
}

error_code sceNpMatchingLeaveRoom(u32 ctx_id, vm::cptr<SceNpRoomId> room_id, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingLeaveRoom(ctx_id=%d, room_id=*0x%x, req_id=*0x%x)", ctx_id, room_id, req_id);
	return CELL_OK;
}

error_code sceNpMatchingSearchJoinRoomGUI(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingSearchCondition> cond, vm::cptr<SceNpMatchingAttr> attr,
    vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpMatchingSearchJoinRoomGUI(ctx_id=%d, communicationId=*0x%x, cond=*0x%x, attr=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, communicationId, cond, attr, handler, arg);

	return CELL_OK;
}

error_code sceNpMatchingGrantOwnership(u32 ctx_id, vm::cptr<SceNpRoomId> room_id, vm::cptr<SceNpId> user_id, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpMatchingGrantOwnership(ctx_id=%d, room_id=*0x%x, user_id=*0x%x, req_id=*0x%x)", ctx_id, room_id, user_id, req_id);

	return CELL_OK;
}

error_code sceNpProfileCallGui(vm::cptr<SceNpId> npid, vm::ptr<SceNpProfileResultHandler> handler, vm::ptr<void> userArg, u64 options)
{
	sceNp.todo("sceNpProfileCallGui(npid=*0x%x, handler=*0x%x, userArg=*0x%x, options=0x%x)", npid, handler, userArg, options);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!handler)
	{
		return SCE_NP_PROFILE_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpProfileAbortGui()
{
	sceNp.todo("sceNpProfileAbortGui()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpScoreInit()
{
	sceNp.warning("sceNpScoreInit()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph.is_NP_Score_init = true;

	return CELL_OK;
}

error_code sceNpScoreTerm()
{
	sceNp.warning("sceNpScoreTerm()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph.is_NP_Score_init = false;

	return CELL_OK;
}

error_code sceNpScoreCreateTitleCtx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase, vm::cptr<SceNpId> selfNpId)
{
	sceNp.warning("sceNpScoreCreateTitleCtx(communicationId=*0x%x, passphrase=*0x%x, selfNpId=*0x%x)", communicationId, passphrase, selfNpId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!communicationId || !passphrase || !selfNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	s32 id = create_score_context(communicationId, passphrase);

	if (id > 0)
	{
		return not_an_error(id);
	}

	return id;
}

error_code sceNpScoreDestroyTitleCtx(s32 titleCtxId)
{
	sceNp.warning("sceNpScoreDestroyTitleCtx(titleCtxId=%d)", titleCtxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!destroy_score_context(titleCtxId))
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;

	return CELL_OK;
}

error_code sceNpScoreCreateTransactionCtx(s32 titleCtxId)
{
	sceNp.warning("sceNpScoreCreateTransactionCtx(titleCtxId=%d)", titleCtxId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (nph.get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto score = idm::get<score_ctx>(titleCtxId);

	if (!score)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	s32 id = create_score_transaction_context(score);

	if (id > 0)
	{
		return not_an_error(id);
	}

	return id;
}

error_code sceNpScoreDestroyTransactionCtx(s32 transId)
{
	sceNp.warning("sceNpScoreDestroyTransactionCtx(transId=%d)", transId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!destroy_score_transaction_context(transId))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreSetTimeout(s32 ctxId, usecond_t timeout)
{
	sceNp.warning("sceNpScoreSetTimeout(ctxId=%d, timeout=%d)", ctxId, timeout);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (timeout < 10'000'000) // 10 seconds
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (static_cast<u32>(ctxId) >= score_transaction_ctx::id_base)
	{
		auto trans = idm::get<score_transaction_ctx>(ctxId);
		if (!trans)
		{
			return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
		}
		trans->timeout = timeout;
	}
	else
	{
		auto score = idm::get<score_ctx>(ctxId);
		if (!ctxId)
		{
			return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
		}
		score->timeout = timeout;
	}

	return CELL_OK;
}

error_code sceNpScoreSetPlayerCharacterId(s32 ctxId, SceNpScorePcId pcId)
{
	sceNp.warning("sceNpScoreSetPlayerCharacterId(ctxId=%d, pcId=%d)", ctxId, pcId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (pcId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (static_cast<u32>(ctxId) >= score_transaction_ctx::id_base)
	{
		auto trans = idm::get<score_transaction_ctx>(ctxId);
		if (!trans)
		{
			return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
		}
		trans->pcId = pcId;
	}
	else
	{
		auto score = idm::get<score_ctx>(ctxId);
		if (!ctxId)
		{
			return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
		}
		score->pcId = pcId;
	}

	return CELL_OK;
}

error_code sceNpScoreWaitAsync(s32 transId, vm::ptr<s32> result)
{
	sceNp.warning("sceNpScoreWaitAsync(transId=%d, result=*0x%x)", transId, result);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto trans = idm::get<score_transaction_ctx>(transId);
	if (!trans)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	*result = trans->wait_for_completion();

	return CELL_OK;
}

error_code sceNpScorePollAsync(s32 transId, vm::ptr<s32> result)
{
	sceNp.warning("sceNpScorePollAsync(transId=%d, result=*0x%x)", transId, result);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto trans = idm::get<score_transaction_ctx>(transId);
	if (!trans)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	auto res = trans->get_score_transaction_status();

	if (!res)
	{
		return not_an_error(1);
	}

	*result = *res;
	return CELL_OK;
}

error_code scenp_score_get_board_info(s32 transId, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, vm::ptr<void> option, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!boardInfo)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	nph.get_board_infos(trans_ctx, boardId, boardInfo, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreGetBoardInfo(s32 transId, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetBoardInfo(transId=%d, boardId=%d, boardInfo=*0x%x, option=*0x%x)", transId, boardId, boardInfo, option);

	return scenp_score_get_board_info(transId, boardId, boardInfo, option, false);
}

error_code sceNpScoreGetBoardInfoAsync(s32 transId, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, s32 prio, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetBoardInfo(transId=%d, boardId=%d, boardInfo=*0x%x, prio=%d, option=*0x%x)", transId, boardId, boardInfo, prio, option);

	return scenp_score_get_board_info(transId, boardId, boardInfo, option, true);
}

error_code scenp_score_record_score(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, vm::cptr<SceNpScoreGameInfo> gameInfo,
    vm::ptr<SceNpScoreRankNumber> tmpRank, vm::ptr<SceNpScoreRecordOptParam> option, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	const u8* data = nullptr;
	u32 data_size = 0;

	if (!gameInfo)
	{
		if (option && option->vsInfo)
		{
			if (option->size != 0xCu)
			{
				return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
			}

			if (option->reserved)
			{
				return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
			}

			data = &option->vsInfo->data[0];
			data_size = option->vsInfo->infoSize;
		}
	}
	else
	{
		data = &gameInfo->nativeData[0];
		data_size = 64;
	}

	if (option)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	nph.record_score(trans_ctx, boardId, score, scoreComment, data, data_size, tmpRank, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreRecordScore(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, vm::cptr<SceNpScoreGameInfo> gameInfo,
    vm::ptr<SceNpScoreRankNumber> tmpRank, vm::ptr<SceNpScoreRecordOptParam> option)
{
	sceNp.warning("sceNpScoreRecordScore(transId=%d, boardId=%d, score=%d, scoreComment=*0x%x, gameInfo=*0x%x, tmpRank=*0x%x, option=*0x%x)", transId, boardId, score, scoreComment, gameInfo, tmpRank, option);

	return scenp_score_record_score(transId, boardId, score, scoreComment, gameInfo, tmpRank, option, false);
}

error_code sceNpScoreRecordScoreAsync(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, vm::cptr<SceNpScoreGameInfo> gameInfo,
    vm::ptr<SceNpScoreRankNumber> tmpRank, s32 prio, vm::ptr<SceNpScoreRecordOptParam> option)
{
	sceNp.warning("sceNpScoreRecordScoreAsync(transId=%d, boardId=%d, score=%d, scoreComment=*0x%x, gameInfo=*0x%x, tmpRank=*0x%x, prio=%d, option=*0x%x)", transId, boardId, score, scoreComment, gameInfo,
	    tmpRank, prio, option);

	return scenp_score_record_score(transId, boardId, score, scoreComment, gameInfo, tmpRank, option, true);
}

error_code scenp_score_record_game_data(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::ptr<void> /* option */, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!data)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (!transId)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!nph.is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	nph.record_score_data(trans_ctx, boardId, score, totalSize, sendSize, static_cast<const u8*>(data.get_ptr()), async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreRecordGameData(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreRecordGameData(transId=%d, boardId=%d, score=%d, totalSize=%d, sendSize=%d, data=*0x%x, option=*0x%x)", transId, boardId, score, totalSize, sendSize, data, option);

	return scenp_score_record_game_data(transId, boardId, score, totalSize, sendSize, data, option, false);
}

error_code sceNpScoreRecordGameDataAsync(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, u32 totalSize, u32 sendSize, vm::cptr<void> data, s32 prio, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreRecordGameDataAsync(transId=%d, boardId=%d, score=%d, totalSize=%d, sendSize=%d, data=*0x%x, prio=%d, option=*0x%x)", transId, boardId, score, totalSize, sendSize, data, prio, option);

	return scenp_score_record_game_data(transId, boardId, score, totalSize, sendSize, data, option, true);
}

error_code scenp_score_get_game_data(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> data, vm::ptr<void> /* option */, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !data)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	nph.get_score_data(trans_ctx, boardId, *npId, totalSize, recvSize, data, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreGetGameData(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> data, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetGameData(transId=%d, boardId=%d, npId=*0x%x, totalSize=*0x%x, recvSize=%d, data=*0x%x, option=*0x%x)", transId, boardId, npId, totalSize, recvSize, data, option);

	return scenp_score_get_game_data(transId, boardId, npId, totalSize, recvSize, data, option, false);
}

error_code sceNpScoreGetGameDataAsync(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> data, s32 prio, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetGameDataAsync(transId=%d, boardId=%d, npId=*0x%x, totalSize=*0x%x, recvSize=%d, data=*0x%x, prio=%d, option=*0x%x)", transId, boardId, npId, totalSize, recvSize, data, prio,
		option);

	return scenp_score_get_game_data(transId, boardId, npId, totalSize, recvSize, data, option, true);
}

template <typename T>
error_code scenp_score_get_ranking_by_npid(s32 transId, SceNpScoreBoardId boardId, T npIdArray, u32 npIdArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize,
	vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
	vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> /* option */, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npIdArray)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (!arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_NPID_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);
	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	if (commentArray && commentArraySize != (arrayNum * sizeof(SceNpScoreComment)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (infoArray && infoArraySize != (arrayNum * sizeof(SceNpScoreGameInfo)) && infoArraySize != (arrayNum * sizeof(SceNpScoreVariableSizeGameInfo)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	// Function can actually accept SceNpScoreRankData though it is undocumented
	if (rankArraySize != (arrayNum * sizeof(SceNpScorePlayerRankData)) && rankArraySize != (arrayNum * sizeof(SceNpScoreRankData)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	std::vector<std::pair<SceNpId, s32>> npid_vec;

	static constexpr bool is_npid     = std::is_same<T, vm::cptr<SceNpId>>::value;
	static constexpr bool is_npidpcid = std::is_same<T, vm::cptr<SceNpScoreNpIdPcId>>::value;
	static_assert(is_npid || is_npidpcid, "T should be vm::cptr<SceNpId> or vm::cptr<SceNpScoreNpIdPcId>");

	if constexpr (is_npid)
	{
		if (npIdArraySize != (arrayNum * sizeof(SceNpId)))
		{
			return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
		}

		for (u32 index = 0; index < arrayNum; index++)
		{
			npid_vec.push_back(std::make_pair(npIdArray[index], 0));
		}
	}
	else if constexpr (is_npidpcid)
	{
		if (npIdArraySize != (arrayNum * sizeof(SceNpScoreNpIdPcId)))
		{
			return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
		}

		for (u32 index = 0; index < arrayNum; index++)
		{
			npid_vec.push_back(std::make_pair(npIdArray[index].npId, npIdArray[index].pcId));
		}
	}

	nph.get_score_npid(trans_ctx, boardId, npid_vec, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreGetRankingByNpId(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npIdArray, u32 npIdArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetRankingByNpId(transId=%d, boardId=%d, npIdArray=*0x%x, npIdArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, boardId, npIdArray, npIdArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, option);

	return scenp_score_get_ranking_by_npid(transId, boardId, npIdArray, npIdArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray,
		infoArraySize, arrayNum, lastSortDate, totalRecord, option, false);
}

error_code sceNpScoreGetRankingByNpIdAsync(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npIdArray, u32 npIdArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetRankingByNpIdAsync(transId=%d, boardId=%d, npIdArray=*0x%x, npIdArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, boardId, npIdArray, npIdArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, prio, option);

	return scenp_score_get_ranking_by_npid(transId, boardId, npIdArray, npIdArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray,
		infoArraySize, arrayNum, lastSortDate, totalRecord, option, true);
}

error_code scenp_score_get_ranking_by_range(s32 transId, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);
	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (option)
	{
		vm::ptr<u32> opt_ptr = vm::static_ptr_cast<u32>(option);
		if (opt_ptr[0] != 0xCu)
		{
			return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
		}

		if (opt_ptr[1])
		{
			vm::ptr<u32> ssr_ptr = vm::cast(opt_ptr[1]);
			startSerialRank = *ssr_ptr;
		}

		// It also uses opt_ptr[2] for unknown purposes
	}

	if (!startSerialRank)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (!rankArray || !totalRecord || !lastSortDate)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_RANGE_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_LARGE_RANGE;
	}

	if (commentArray && commentArraySize != (arrayNum * sizeof(SceNpScoreComment)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (infoArray && infoArraySize != (arrayNum * sizeof(SceNpScoreGameInfo)) && infoArraySize != (arrayNum * sizeof(SceNpScoreVariableSizeGameInfo)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (rankArraySize != (arrayNum * sizeof(SceNpScoreRankData)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	nph.get_score_range(trans_ctx, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreGetRankingByRange(s32 transId, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetRankingByRange(transId=%d, boardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, infoArraySize=%d, "
	           "arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, option);

	return scenp_score_get_ranking_by_range(transId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize,
		arrayNum, lastSortDate, totalRecord, option, false);
}

error_code sceNpScoreGetRankingByRangeAsync(s32 transId, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetRankingByRangeAsync(transId=%d, boardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, prio, option);

	return scenp_score_get_ranking_by_range(transId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize,
		arrayNum, lastSortDate, totalRecord, option, true);
}

error_code scenp_score_get_friends_ranking(s32 transId, SceNpScoreBoardId boardId, s32 includeSelf, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray,
    u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option,
	bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);
	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	if (!rankArray || !totalRecord || !lastSortDate)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_SELECTED_FRIENDS_NUM || option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (commentArray && commentArraySize != (arrayNum * sizeof(SceNpScoreComment)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (infoArray && infoArraySize != (arrayNum * sizeof(SceNpScoreGameInfo)) && infoArraySize != (arrayNum * sizeof(SceNpScoreVariableSizeGameInfo)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (rankArraySize != (arrayNum * sizeof(SceNpScoreRankData)))
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	nph.get_score_friend(trans_ctx, boardId, includeSelf, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, async);

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreGetFriendsRanking(s32 transId, SceNpScoreBoardId boardId, s32 includeSelf, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray,
    u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetFriendsRanking(transId=%d, boardId=%d, includeSelf=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, infoArraySize=%d, "
	           "arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, boardId, includeSelf, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, option);

	return scenp_score_get_friends_ranking(transId, boardId, includeSelf, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize,
		arrayNum, lastSortDate, totalRecord, option, false);
}

error_code sceNpScoreGetFriendsRankingAsync(s32 transId, SceNpScoreBoardId boardId, s32 includeSelf, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray,
    u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio,
    vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetFriendsRankingAsync(transId=%d, boardId=%d, includeSelf=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, infoArraySize=%d, "
	           "arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, boardId, includeSelf, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, prio, option);

	return scenp_score_get_friends_ranking(transId, boardId, includeSelf, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize,
		arrayNum, lastSortDate, totalRecord, option, true);
}

error_code scenp_score_censor_comment(s32 transId, vm::cptr<char> comment, vm::ptr<void> option, bool async)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!comment)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (strlen(comment.get_ptr()) > SCE_NP_SCORE_CENSOR_COMMENT_MAXLEN || option) // option check at least until fw 4.71
	{
		// TODO: is SCE_NP_SCORE_CENSOR_COMMENT_MAXLEN + 1 allowed ?
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	// TODO: actual implementation of this
	trans_ctx->result = CELL_OK;

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreCensorComment(s32 transId, vm::cptr<char> comment, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreCensorComment(transId=%d, comment=%s, option=*0x%x)", transId, comment, option);

	return scenp_score_censor_comment(transId, comment, option, false);
}

error_code sceNpScoreCensorCommentAsync(s32 transId, vm::cptr<char> comment, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreCensorCommentAsync(transId=%d, comment=%s, prio=%d, option=*0x%x)", transId, comment, prio, option);

	return scenp_score_censor_comment(transId, comment, option, true);
}

error_code scenp_score_sanitize_comment(s32 transId, vm::cptr<char> comment, vm::ptr<char> sanitizedComment, vm::ptr<void> /* option */, bool async)
{
	if (!sanitizedComment)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!comment)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	const auto comment_len = strlen(comment.get_ptr());

	if (comment_len > SCE_NP_SCORE_CENSOR_COMMENT_MAXLEN)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	auto trans_ctx = idm::get<score_transaction_ctx>(transId);

	if (!trans_ctx)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	// TODO: actual implementation of this
	memcpy(sanitizedComment.get_ptr(), comment.get_ptr(), comment_len + 1);
	trans_ctx->result = CELL_OK;

	if (async)
	{
		return CELL_OK;
	}

	return *trans_ctx->result;
}

error_code sceNpScoreSanitizeComment(s32 transId, vm::cptr<char> comment, vm::ptr<char> sanitizedComment, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreSanitizeComment(transId=%d, comment=%s, sanitizedComment=*0x%x, option=*0x%x)", transId, comment, sanitizedComment, option);

	return scenp_score_sanitize_comment(transId, comment, sanitizedComment, option, false);
}

error_code sceNpScoreSanitizeCommentAsync(s32 transId, vm::cptr<char> comment, vm::ptr<char> sanitizedComment, s32 prio, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreSanitizeCommentAsync(transId=%d, comment=%s, sanitizedComment=*0x%x, prio=%d, option=*0x%x)", transId, comment, sanitizedComment, prio, option);

	return scenp_score_sanitize_comment(transId, comment, sanitizedComment, option, true);
}

error_code sceNpScoreGetRankingByNpIdPcId(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpScoreNpIdPcId> idArray, u32 idArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetRankingByNpIdPcId(transId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, option);

	return scenp_score_get_ranking_by_npid(transId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray,
		infoArraySize, arrayNum, lastSortDate, totalRecord, option, false);
}

error_code sceNpScoreGetRankingByNpIdPcIdAsync(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpScoreNpIdPcId> idArray, u32 idArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray,
    u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.warning("sceNpScoreGetRankingByNpIdPcIdAsync(transId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, prio, option);

	return scenp_score_get_ranking_by_npid(transId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray,
		infoArraySize, arrayNum, lastSortDate, totalRecord, option, true);
}

error_code sceNpScoreAbortTransaction(s32 transId)
{
	sceNp.todo("sceNpScoreAbortTransaction(transId=%d)", transId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	auto trans = idm::get<score_transaction_ctx>(transId);
	if (!trans)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	trans->abort_score_transaction();

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByNpId(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> idArray, u32 idArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray,
    u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize, vm::ptr<SceNpScoreClansMemberDescription> descriptArray,
    u32 descriptArraySize, u32 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByNpId(transId=%d, clanId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo,
	    lastSortDate, totalRecord, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!idArray || !rankArray || !totalRecord || !lastSortDate || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_NPID_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByNpIdAsync(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> idArray, u32 idArraySize,
    vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize,
    vm::ptr<SceNpScoreClansMemberDescription> descriptArray, u32 descriptArraySize, u32 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByNpIdAsync(transId=%d, clanId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo,
	    lastSortDate, totalRecord, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!idArray || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByNpIdPcId(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> idArray, u32 idArraySize,
    vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize,
    vm::ptr<SceNpScoreClansMemberDescription> descriptArray, u32 descriptArraySize, u32 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByNpIdPcId(transId=%d, clanId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo,
	    lastSortDate, totalRecord, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!idArray || !rankArray || !totalRecord || !lastSortDate || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_NPID_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByNpIdPcIdAsync(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> idArray, u32 idArraySize,
    vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize,
    vm::ptr<SceNpScoreClansMemberDescription> descriptArray, u32 descriptArraySize, u32 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo(
	    "sceNpScoreGetClansMembersRankingByNpIdPcIdAsync(transId=%d, clanId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	    "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo,
	    lastSortDate, totalRecord, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!idArray || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansRankingByRange(s32 transId, SceNpScoreClansBoardId clanBoardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreClanIdRankData> rankArray, u32 rankArraySize,
    vm::ptr<void> reserved1, u32 reservedSize1, vm::ptr<void> reserved2, u32 reservedSize2, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord,
    vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansRankingByRange(transId=%d, clanBoardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, reserved1=*0x%x, reservedSize1=%d, reserved2=*0x%x, reservedSize2=%d, "
	           "arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanBoardId, startSerialRank, rankArray, rankArraySize, reserved1, reservedSize1, reserved2, reservedSize2, arrayNum, lastSortDate, totalRecord, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!rankArray || !totalRecord || !lastSortDate || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (!startSerialRank || reserved1 || reservedSize1 || reserved2 || reservedSize2 || option) // reserved and option checks at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_CLAN_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_LARGE_RANGE;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansRankingByRangeAsync(s32 transId, SceNpScoreClansBoardId clanBoardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreClanIdRankData> rankArray, u32 rankArraySize,
    vm::ptr<void> reserved1, u32 reservedSize1, vm::ptr<void> reserved2, u32 reservedSize2, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio,
    vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansRankingByRangeAsync(transId=%d, clanBoardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, reserved1=*0x%x, reservedSize1=%d, reserved2=*0x%x, "
	           "reservedSize2=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanBoardId, startSerialRank, rankArray, rankArraySize, reserved1, reservedSize1, reserved2, reservedSize2, arrayNum, lastSortDate, totalRecord, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (!startSerialRank || reserved1 || reservedSize1 || reserved2 || reservedSize2 || option) // reserved and option checks at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClanMemberGameData(
    s32 transId, SceNpScoreBoardId boardId, SceNpClanId clanId, vm::cptr<SceNpId> npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> data, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClanMemberGameData(transId=%d, boardId=%d, clanId=%d, npId=*0x%x, totalSize=*0x%x, recvSize=%d, data=*0x%x, option=*0x%x)", transId, boardId, clanId, npId, totalSize,
	    recvSize, data, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !totalSize || !data)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClanMemberGameDataAsync(
    s32 transId, SceNpScoreBoardId boardId, SceNpClanId clanId, vm::cptr<SceNpId> npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> data, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClanMemberGameDataAsync(transId=%d, boardId=%d, clanId=%d, npId=*0x%x, totalSize=*0x%x, recvSize=%d, data=*0x%x, prio=%d, option=*0x%x)", transId, boardId, clanId, npId,
	    totalSize, recvSize, data, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansRankingByClanId(s32 transId, SceNpScoreClansBoardId clanBoardId, vm::cptr<SceNpClanId> clanIdArray, u32 clanIdArraySize, vm::ptr<SceNpScoreClanIdRankData> rankArray,
    u32 rankArraySize, vm::ptr<void> reserved1, u32 reservedSize1, vm::ptr<void> reserved2, u32 reservedSize2, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansRankingByClanId(transId=%d, clanBoardId=%d, clanIdArray=*0x%x, clanIdArraySize=%d, rankArray=*0x%x, rankArraySize=%d, reserved1=*0x%x, reservedSize1=%d, "
	           "reserved2=*0x%x, reservedSize2=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanBoardId, clanIdArray, clanIdArraySize, rankArray, rankArraySize, reserved1, reservedSize1, reserved2, reservedSize2, arrayNum, lastSortDate, totalRecord, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!clanIdArray || !rankArray || !totalRecord || !lastSortDate || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (reserved1 || reservedSize1 || reserved2 || reservedSize2 || option) // reserved and option checks at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_NPID_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansRankingByClanIdAsync(s32 transId, SceNpScoreClansBoardId clanBoardId, vm::cptr<SceNpClanId> clanIdArray, u32 clanIdArraySize, vm::ptr<SceNpScoreClanIdRankData> rankArray,
    u32 rankArraySize, vm::ptr<void> reserved1, u32 reservedSize1, vm::ptr<void> reserved2, u32 reservedSize2, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansRankingByRangeAsync(transId=%d, clanBoardId=%d, clanIdArray=*0x%x, clanIdArraySize=%d, rankArray=*0x%x, rankArraySize=%d, reserved1=*0x%x, reservedSize1=%d, "
	           "reserved2=*0x%x, reservedSize2=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanBoardId, clanIdArray, clanIdArraySize, rankArray, rankArraySize, reserved1, reservedSize1, reserved2, reservedSize2, arrayNum, lastSortDate, totalRecord, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (reserved1 || reservedSize1 || reserved2 || reservedSize2 || option) // reserved and option checks at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByRange(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreClanIdRankData> rankArray,
    u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize, vm::ptr<SceNpScoreClansMemberDescription> descriptArray,
    u32 descriptArraySize, u32 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByRange(transId=%d, clanId=%d, boardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo, lastSortDate,
	    totalRecord, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!rankArray || !totalRecord || !lastSortDate || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (!startSerialRank || option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_RANGE_NUM_PER_TRANS)
	{
		return SCE_NP_COMMUNITY_ERROR_TOO_LARGE_RANGE;
	}

	if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByRangeAsync(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreClanIdRankData> rankArray,
    u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u32 infoArraySize, vm::ptr<SceNpScoreClansMemberDescription> descriptArray,
    u32 descriptArraySize, u32 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByRangeAsync(transId=%d, clanId=%d, boardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo, lastSortDate,
	    totalRecord, prio, option);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (!startSerialRank || option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingCreateCtx(vm::ptr<SceNpId> npId, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg, vm::ptr<u32> ctx_id)
{
	sceNp.warning("sceNpSignalingCreateCtx(npId=*0x%x, handler=*0x%x, arg=*0x%x, ctx_id=*0x%x)", npId, handler, arg, ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !ctx_id)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	//	if (current_contexts > SCE_NP_SIGNALING_CTX_MAX)
	//{
	//	return SCE_NP_SIGNALING_ERROR_CTX_MAX;
	//}

	*ctx_id = create_signaling_context(npId, handler, arg);

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh.set_sig_cb(*ctx_id, handler, arg);

	return CELL_OK;
}

error_code sceNpSignalingDestroyCtx(u32 ctx_id)
{
	sceNp.warning("sceNpSignalingDestroyCtx(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!destroy_signaling_context(ctx_id))
	{
		return SCE_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpSignalingAddExtendedHandler(u32 ctx_id, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg)
{
	sceNp.warning("sceNpSignalingAddExtendedHandler(ctx_id=%d, handler=*0x%x, arg=*0x%x)", ctx_id, handler, arg);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh.set_ext_sig_cb(ctx_id, handler, arg);

	return CELL_OK;
}

error_code sceNpSignalingSetCtxOpt(u32 ctx_id, s32 optname, s32 optval)
{
	sceNp.todo("sceNpSignalingSetCtxOpt(ctx_id=%d, optname=%d, optval=%d)", ctx_id, optname, optval);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!optname || !optval)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingGetCtxOpt(u32 ctx_id, s32 optname, vm::ptr<s32> optval)
{
	sceNp.todo("sceNpSignalingGetCtxOpt(ctx_id=%d, optname=%d, optval=*0x%x)", ctx_id, optname, optval);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!optname || !optval)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingActivateConnection(u32 ctx_id, vm::ptr<SceNpId> npId, vm::ptr<u32> conn_id)
{
	sceNp.warning("sceNpSignalingActivateConnection(ctx_id=%d, npId=*0x%x, conn_id=*0x%x)", ctx_id, npId, conn_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !conn_id)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	if (np::is_same_npid(nph.get_npid(), *npId))
		return SCE_NP_SIGNALING_ERROR_OWN_NP_ID;

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	*conn_id = sigh.init_sig1(*npId);

	return CELL_OK;
}

error_code sceNpSignalingDeactivateConnection(u32 ctx_id, u32 conn_id)
{
	sceNp.todo("sceNpSignalingDeactivateConnection(ctx_id=%d, conn_id=%d)", ctx_id, conn_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpSignalingTerminateConnection(u32 ctx_id, u32 conn_id)
{
	sceNp.warning("sceNpSignalingTerminateConnection(ctx_id=%d, conn_id=%d)", ctx_id, conn_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();

	sigh.stop_sig(conn_id);

	return CELL_OK;
}

error_code sceNpSignalingGetConnectionStatus(u32 ctx_id, u32 conn_id, vm::ptr<s32> conn_status, vm::ptr<np_in_addr> peer_addr, vm::ptr<np_in_port_t> peer_port)
{
	sceNp.warning("sceNpSignalingGetConnectionStatus(ctx_id=%d, conn_id=%d, conn_status=*0x%x, peer_addr=*0x%x, peer_port=*0x%x)", ctx_id, conn_id, conn_status, peer_addr, peer_port);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!conn_status)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();

	const auto si = sigh.get_sig_infos(conn_id);

	if (!si)
	{
		*conn_status = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
		return SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
	}

	*conn_status = si->conn_status;

	if (peer_addr)
		(*peer_addr).np_s_addr = si->addr; // infos.addr is already BE
	if (peer_port)
		*peer_port = si->port;

	return CELL_OK;
}

error_code sceNpSignalingGetConnectionInfo(u32 ctx_id, u32 conn_id, s32 code, vm::ptr<SceNpSignalingConnectionInfo> info)
{
	sceNp.warning("sceNpSignalingGetConnectionInfo(ctx_id=%d, conn_id=%d, code=%d, info=*0x%x)", ctx_id, conn_id, code, info);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	const auto si = sigh.get_sig_infos(conn_id);

	if (!si)
		return SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND;

	switch (code)
	{
		case SCE_NP_SIGNALING_CONN_INFO_RTT:
		{
			info->rtt = si->rtt;
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_BANDWIDTH:
		{
			info->bandwidth = 100'000'000; // 100 MBPS HACK
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_PEER_NPID:
		{
			info->npId = si->npid;
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_PEER_ADDRESS:
		{
			info->address.port = std::bit_cast<u16, be_t<u16>>(si->port);
			info->address.addr.np_s_addr = si->addr;
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_MAPPED_ADDRESS:
		{
			info->address.port = std::bit_cast<u16, be_t<u16>>(si->mapped_port);
			info->address.addr.np_s_addr = si->mapped_addr;
			break;
		}
		case SCE_NP_SIGNALING_CONN_INFO_PACKET_LOSS:
		{
			info->packet_loss = 0; // HACK
			break;
		}
		default:
		{
			return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
		}
	}

	return CELL_OK;
}

error_code sceNpSignalingGetConnectionFromNpId(u32 ctx_id, vm::ptr<SceNpId> npId, vm::ptr<u32> conn_id)
{
	sceNp.notice("sceNpSignalingGetConnectionFromNpId(ctx_id=%d, npId=*0x%x, conn_id=*0x%x)", ctx_id, npId, conn_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !conn_id)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	if (np::is_same_npid(*npId, nph.get_npid()))
	{
		return SCE_NP_SIGNALING_ERROR_OWN_NP_ID;
	}

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	const auto found_conn_id = sigh.get_conn_id_from_npid(*npId);

	if (!found_conn_id)
	{
		return SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
	}

	*conn_id = *found_conn_id;

	return CELL_OK;
}

error_code sceNpSignalingGetConnectionFromPeerAddress(u32 ctx_id, np_in_addr_t peer_addr, np_in_port_t peer_port, vm::ptr<u32> conn_id)
{
	sceNp.warning("sceNpSignalingGetConnectionFromPeerAddress(ctx_id=%d, peer_addr=0x%x, peer_port=%d, conn_id=*0x%x)", ctx_id, peer_addr, peer_port, conn_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!conn_id)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
	const auto found_conn_id = sigh.get_conn_id_from_addr(peer_addr, peer_port);

	if (!found_conn_id)
	{
		return SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
	}

	*conn_id = *found_conn_id;

	return CELL_OK;
}

error_code sceNpSignalingGetLocalNetInfo(u32 ctx_id, vm::ptr<SceNpSignalingNetInfo> info)
{
	sceNp.warning("sceNpSignalingGetLocalNetInfo(ctx_id=%d, info=*0x%x)", ctx_id, info);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!info || info->size != sizeof(SceNpSignalingNetInfo))
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	info->local_addr  = nph.get_local_ip_addr();
	info->mapped_addr = nph.get_public_ip_addr();

	// Pure speculation below
	info->nat_status    = SCE_NP_SIGNALING_NETINFO_NAT_STATUS_TYPE2;
	info->upnp_status   = nph.get_upnp_status();
	info->npport_status = SCE_NP_SIGNALING_NETINFO_NPPORT_STATUS_OPEN;
	info->npport        = SCE_NP_PORT;

	return CELL_OK;
}

error_code sceNpSignalingGetPeerNetInfo(u32 ctx_id, vm::ptr<SceNpId> npId, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpSignalingGetPeerNetInfo(ctx_id=%d, npId=*0x%x, req_id=*0x%x)", ctx_id, npId, req_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !req_id)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingCancelPeerNetInfo(u32 ctx_id, u32 req_id)
{
	sceNp.todo("sceNpSignalingCancelPeerNetInfo(ctx_id=%d, req_id=%d)", ctx_id, req_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpSignalingGetPeerNetInfoResult(u32 ctx_id, u32 req_id, vm::ptr<SceNpSignalingNetInfo> info)
{
	sceNp.todo("sceNpSignalingGetPeerNetInfoResult(ctx_id=%d, req_id=%d, info=*0x%x)", ctx_id, req_id, info);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		// TODO: check info->size
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpUtilCanonicalizeNpIdForPs3()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpUtilCanonicalizeNpIdForPsp()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpUtilCmpNpId(vm::ptr<SceNpId> id1, vm::ptr<SceNpId> id2)
{
	sceNp.trace("sceNpUtilCmpNpId(id1=*0x%x(%s), id2=*0x%x(%s))", id1, id1 ? id1->handle.data : "", id2, id2 ? id2->handle.data : "");

	if (!id1 || !id2)
	{
		return SCE_NP_UTIL_ERROR_INVALID_ARGUMENT;
	}

	if (!np::is_same_npid(*id1, *id2))
		return not_an_error(SCE_NP_UTIL_ERROR_NOT_MATCH);

	return CELL_OK;
}

error_code sceNpUtilCmpNpIdInOrder(vm::cptr<SceNpId> id1, vm::cptr<SceNpId> id2, vm::ptr<s32> order)
{
	sceNp.trace("sceNpUtilCmpNpIdInOrder(id1=*0x%x, id2=*0x%x, order=*0x%x)", id1, id2, order);

	if (!id1 || !id2)
	{
		return SCE_NP_UTIL_ERROR_INVALID_ARGUMENT;
	}

	// if (id1->reserved[0] != 1 || id2->reserved[0] != 1)
	// {
	// 	return SCE_NP_UTIL_ERROR_INVALID_NP_ID;
	// }

	if (s32 res = strncmp(id1->handle.data, id2->handle.data, 16))
	{
		*order = std::clamp<s32>(res, -1, 1);
		return CELL_OK;
	}

	if (s32 res = memcmp(id1->unk1, id2->unk1, 4))
	{
		*order = std::clamp<s32>(res, -1, 1);
		return CELL_OK;
	}

	const u8 opt14 = id1->opt[4];
	const u8 opt24 = id2->opt[4];

	if (opt14 == 0 && opt24 == 0)
	{
		*order = 0;
		return CELL_OK;
	}

	if (opt14 != 0 && opt24 != 0)
	{
		s32 res = memcmp(id1->unk1 + 1, id2->unk1 + 1, 4);
		*order = std::clamp<s32>(res, -1, 1);
		return CELL_OK;
	}

	s32 res = memcmp((opt14 != 0 ? id1 : id2)->unk1 + 1, "ps3", 4);
	*order = std::clamp<s32>(res, -1, 1);
	return CELL_OK;
}

error_code sceNpUtilCmpOnlineId(vm::cptr<SceNpId> id1, vm::cptr<SceNpId> id2)
{
	sceNp.warning("sceNpUtilCmpOnlineId(id1=*0x%x, id2=*0x%x)", id1, id2);

	if (!id1 || !id2)
	{
		return SCE_NP_UTIL_ERROR_INVALID_ARGUMENT;
	}

	// if (id1->reserved[0] != 1 || id2->reserved[0] != 1)
	// {
	// 	return SCE_NP_UTIL_ERROR_INVALID_NP_ID;
	// }

	if (strncmp(id1->handle.data, id2->handle.data, 16) != 0)
	{
		return SCE_NP_UTIL_ERROR_NOT_MATCH;
	}

	return CELL_OK;
}

error_code sceNpUtilGetPlatformType(vm::cptr<SceNpId> npId)
{
	sceNp.warning("sceNpUtilGetPlatformType(npId=*0x%x)", npId);

	if (!npId)
	{
		return SCE_NP_UTIL_ERROR_INVALID_ARGUMENT;
	}

	switch (npId->unk1[1])
	{
	case "ps4\0"_u32:
		return not_an_error(SCE_NP_PLATFORM_TYPE_PS4);
	case "psp2"_u32:
		return not_an_error(SCE_NP_PLATFORM_TYPE_VITA);
	case "ps3\0"_u32:
		return not_an_error(SCE_NP_PLATFORM_TYPE_PS3);
	case 0u:
		return not_an_error(SCE_NP_PLATFORM_TYPE_NONE);
	default:
		break;
	}

	return SCE_NP_UTIL_ERROR_UNKNOWN_PLATFORM_TYPE;
}

error_code sceNpUtilSetPlatformType(vm::ptr<SceNpId> npId, SceNpPlatformType platformType)
{
	sceNp.warning("sceNpUtilSetPlatformType(npId=*0x%x, platformType=%d)", npId, platformType);

	if (!npId)
	{
		return SCE_NP_UTIL_ERROR_INVALID_ARGUMENT;
	}

	switch (platformType)
	{
	case SCE_NP_PLATFORM_TYPE_PS4:
		npId->unk1[1] = "ps4\0"_u32; break;
	case SCE_NP_PLATFORM_TYPE_VITA:
		npId->unk1[1] = "psp2"_u32; break;
	case SCE_NP_PLATFORM_TYPE_PS3:
		npId->unk1[1] = "ps3\0"_u32; break;
	case SCE_NP_PLATFORM_TYPE_NONE:
		npId->unk1[1] = 0; break;
	default:
		return SCE_NP_UTIL_ERROR_UNKNOWN_PLATFORM_TYPE;
	}

	return CELL_OK;
}

error_code _sceNpSysutilClientMalloc()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code _sceNpSysutilClientFree()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 _Z33_sce_np_sysutil_send_empty_packetiPN16sysutil_cxmlutil11FixedMemoryEPKcS3_()
{
	sceNp.todo("_Z33_sce_np_sysutil_send_empty_packetiPN16sysutil_cxmlutil11FixedMemoryEPKcS3_()");
	return CELL_OK;
}

s32 _Z27_sce_np_sysutil_send_packetiRN4cxml8DocumentE()
{
	sceNp.todo("_Z27_sce_np_sysutil_send_packetiRN4cxml8DocumentE()");
	return CELL_OK;
}

s32 _Z36_sce_np_sysutil_recv_packet_fixedmemiPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentERNS2_7ElementE()
{
	sceNp.todo("_Z36_sce_np_sysutil_recv_packet_fixedmemiPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentERNS2_7ElementE()");
	return CELL_OK;
}

s32 _Z40_sce_np_sysutil_recv_packet_fixedmem_subiPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentERNS2_7ElementE()
{
	sceNp.todo("_Z40_sce_np_sysutil_recv_packet_fixedmem_subiPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentERNS2_7ElementE()");
	return CELL_OK;
}

s32 _Z27_sce_np_sysutil_recv_packetiRN4cxml8DocumentERNS_7ElementE()
{
	sceNp.todo("_Z27_sce_np_sysutil_recv_packetiRN4cxml8DocumentERNS_7ElementE()");
	return CELL_OK;
}

s32 _Z29_sce_np_sysutil_cxml_set_npidRN4cxml8DocumentERNS_7ElementEPKcPK7SceNpId()
{
	sceNp.todo("_Z29_sce_np_sysutil_cxml_set_npidRN4cxml8DocumentERNS_7ElementEPKcPK7SceNpId()");
	return CELL_OK;
}

s32 _Z31_sce_np_sysutil_send_packet_subiRN4cxml8DocumentE()
{
	sceNp.todo("_Z31_sce_np_sysutil_send_packet_subiRN4cxml8DocumentE()");
	return CELL_OK;
}

s32 _Z37sce_np_matching_set_matching2_runningb()
{
	sceNp.todo("_Z37sce_np_matching_set_matching2_runningb()");
	return CELL_OK;
}

s32 _Z32_sce_np_sysutil_cxml_prepare_docPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentEPKcRNS2_7ElementES6_i()
{
	sceNp.todo("_Z32_sce_np_sysutil_cxml_prepare_docPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentEPKcRNS2_7ElementES6_i()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNp)
("sceNp", []() {
	REG_FUNC(sceNp, sceNpInit);
	REG_FUNC(sceNp, sceNpTerm);
	REG_FUNC(sceNp, sceNpDrmIsAvailable);
	REG_FUNC(sceNp, sceNpDrmIsAvailable2);
	REG_FUNC(sceNp, sceNpDrmVerifyUpgradeLicense);
	REG_FUNC(sceNp, sceNpDrmVerifyUpgradeLicense2);
	REG_FUNC(sceNp, sceNpDrmExecuteGamePurchase);
	REG_FUNC(sceNp, sceNpDrmGetTimelimit);
	REG_FUNC(sceNp, sceNpDrmProcessExitSpawn);
	REG_FUNC(sceNp, sceNpDrmProcessExitSpawn2);
	REG_FUNC(sceNp, sceNpBasicRegisterHandler);
	REG_FUNC(sceNp, sceNpBasicRegisterContextSensitiveHandler);
	REG_FUNC(sceNp, sceNpBasicUnregisterHandler);
	REG_FUNC(sceNp, sceNpBasicSetPresence);
	REG_FUNC(sceNp, sceNpBasicSetPresenceDetails);
	REG_FUNC(sceNp, sceNpBasicSetPresenceDetails2);
	REG_FUNC(sceNp, sceNpBasicSendMessage);
	REG_FUNC(sceNp, sceNpBasicSendMessageGui);
	REG_FUNC(sceNp, sceNpBasicSendMessageAttachment);
	REG_FUNC(sceNp, sceNpBasicRecvMessageAttachment);
	REG_FUNC(sceNp, sceNpBasicRecvMessageAttachmentLoad);
	REG_FUNC(sceNp, sceNpBasicRecvMessageCustom);
	REG_FUNC(sceNp, sceNpBasicMarkMessageAsUsed);
	REG_FUNC(sceNp, sceNpBasicAbortGui);
	REG_FUNC(sceNp, sceNpBasicAddFriend);
	REG_FUNC(sceNp, sceNpBasicGetFriendListEntryCount);
	REG_FUNC(sceNp, sceNpBasicGetFriendListEntry);
	REG_FUNC(sceNp, sceNpBasicGetFriendPresenceByIndex);
	REG_FUNC(sceNp, sceNpBasicGetFriendPresenceByIndex2);
	REG_FUNC(sceNp, sceNpBasicGetFriendPresenceByNpId);
	REG_FUNC(sceNp, sceNpBasicGetFriendPresenceByNpId2);
	REG_FUNC(sceNp, sceNpBasicAddPlayersHistory);
	REG_FUNC(sceNp, sceNpBasicAddPlayersHistoryAsync);
	REG_FUNC(sceNp, sceNpBasicGetPlayersHistoryEntryCount);
	REG_FUNC(sceNp, sceNpBasicGetPlayersHistoryEntry);
	REG_FUNC(sceNp, sceNpBasicAddBlockListEntry);
	REG_FUNC(sceNp, sceNpBasicGetBlockListEntryCount);
	REG_FUNC(sceNp, sceNpBasicGetBlockListEntry);
	REG_FUNC(sceNp, sceNpBasicGetMessageAttachmentEntryCount);
	REG_FUNC(sceNp, sceNpBasicGetMessageAttachmentEntry);
	REG_FUNC(sceNp, sceNpBasicGetCustomInvitationEntryCount);
	REG_FUNC(sceNp, sceNpBasicGetCustomInvitationEntry);
	REG_FUNC(sceNp, sceNpBasicGetMatchingInvitationEntryCount);
	REG_FUNC(sceNp, sceNpBasicGetMatchingInvitationEntry);
	REG_FUNC(sceNp, sceNpBasicGetClanMessageEntryCount);
	REG_FUNC(sceNp, sceNpBasicGetClanMessageEntry);
	REG_FUNC(sceNp, sceNpBasicGetMessageEntryCount);
	REG_FUNC(sceNp, sceNpBasicGetMessageEntry);
	REG_FUNC(sceNp, sceNpBasicGetEvent);
	REG_FUNC(sceNp, sceNpCommerceCreateCtx);
	REG_FUNC(sceNp, sceNpCommerceDestroyCtx);
	REG_FUNC(sceNp, sceNpCommerceInitProductCategory);
	REG_FUNC(sceNp, sceNpCommerceDestroyProductCategory);
	REG_FUNC(sceNp, sceNpCommerceGetProductCategoryStart);
	REG_FUNC(sceNp, sceNpCommerceGetProductCategoryFinish);
	REG_FUNC(sceNp, sceNpCommerceGetProductCategoryResult);
	REG_FUNC(sceNp, sceNpCommerceGetProductCategoryAbort);
	REG_FUNC(sceNp, sceNpCommerceGetProductId);
	REG_FUNC(sceNp, sceNpCommerceGetProductName);
	REG_FUNC(sceNp, sceNpCommerceGetCategoryDescription);
	REG_FUNC(sceNp, sceNpCommerceGetCategoryId);
	REG_FUNC(sceNp, sceNpCommerceGetCategoryImageURL);
	REG_FUNC(sceNp, sceNpCommerceGetCategoryInfo);
	REG_FUNC(sceNp, sceNpCommerceGetCategoryName);
	REG_FUNC(sceNp, sceNpCommerceGetCurrencyCode);
	REG_FUNC(sceNp, sceNpCommerceGetCurrencyDecimals);
	REG_FUNC(sceNp, sceNpCommerceGetCurrencyInfo);
	REG_FUNC(sceNp, sceNpCommerceGetNumOfChildCategory);
	REG_FUNC(sceNp, sceNpCommerceGetNumOfChildProductSku);
	REG_FUNC(sceNp, sceNpCommerceGetSkuDescription);
	REG_FUNC(sceNp, sceNpCommerceGetSkuId);
	REG_FUNC(sceNp, sceNpCommerceGetSkuImageURL);
	REG_FUNC(sceNp, sceNpCommerceGetSkuName);
	REG_FUNC(sceNp, sceNpCommerceGetSkuPrice);
	REG_FUNC(sceNp, sceNpCommerceGetSkuUserData);
	REG_FUNC(sceNp, sceNpCommerceSetDataFlagStart);
	REG_FUNC(sceNp, sceNpCommerceGetDataFlagStart);
	REG_FUNC(sceNp, sceNpCommerceSetDataFlagFinish);
	REG_FUNC(sceNp, sceNpCommerceGetDataFlagFinish);
	REG_FUNC(sceNp, sceNpCommerceGetDataFlagState);
	REG_FUNC(sceNp, sceNpCommerceGetDataFlagAbort);
	REG_FUNC(sceNp, sceNpCommerceGetChildCategoryInfo);
	REG_FUNC(sceNp, sceNpCommerceGetChildProductSkuInfo);
	REG_FUNC(sceNp, sceNpCommerceDoCheckoutStartAsync);
	REG_FUNC(sceNp, sceNpCommerceDoCheckoutFinishAsync);
	REG_FUNC(sceNp, sceNpCustomMenuRegisterActions);
	REG_FUNC(sceNp, sceNpCustomMenuActionSetActivation);
	REG_FUNC(sceNp, sceNpCustomMenuRegisterExceptionList);
	REG_FUNC(sceNp, sceNpFriendlist);
	REG_FUNC(sceNp, sceNpFriendlistCustom);
	REG_FUNC(sceNp, sceNpFriendlistAbortGui);
	REG_FUNC(sceNp, sceNpLookupInit);
	REG_FUNC(sceNp, sceNpLookupTerm);
	REG_FUNC(sceNp, sceNpLookupCreateTitleCtx);
	REG_FUNC(sceNp, sceNpLookupDestroyTitleCtx);
	REG_FUNC(sceNp, sceNpLookupCreateTransactionCtx);
	REG_FUNC(sceNp, sceNpLookupDestroyTransactionCtx);
	REG_FUNC(sceNp, sceNpLookupSetTimeout);
	REG_FUNC(sceNp, sceNpLookupAbortTransaction);
	REG_FUNC(sceNp, sceNpLookupWaitAsync);
	REG_FUNC(sceNp, sceNpLookupPollAsync);
	REG_FUNC(sceNp, sceNpLookupNpId);
	REG_FUNC(sceNp, sceNpLookupNpIdAsync);
	REG_FUNC(sceNp, sceNpLookupUserProfile);
	REG_FUNC(sceNp, sceNpLookupUserProfileAsync);
	REG_FUNC(sceNp, sceNpLookupUserProfileWithAvatarSize);
	REG_FUNC(sceNp, sceNpLookupUserProfileWithAvatarSizeAsync);
	REG_FUNC(sceNp, sceNpLookupAvatarImage);
	REG_FUNC(sceNp, sceNpLookupAvatarImageAsync);
	REG_FUNC(sceNp, sceNpLookupTitleStorage);
	REG_FUNC(sceNp, sceNpLookupTitleStorageAsync);
	REG_FUNC(sceNp, sceNpLookupTitleSmallStorage);
	REG_FUNC(sceNp, sceNpLookupTitleSmallStorageAsync);
	REG_FUNC(sceNp, sceNpManagerRegisterCallback);
	REG_FUNC(sceNp, sceNpManagerUnregisterCallback);
	REG_FUNC(sceNp, sceNpManagerGetStatus);
	REG_FUNC(sceNp, sceNpManagerGetNetworkTime);
	REG_FUNC(sceNp, sceNpManagerGetOnlineId);
	REG_FUNC(sceNp, sceNpManagerGetNpId);
	REG_FUNC(sceNp, sceNpManagerGetOnlineName);
	REG_FUNC(sceNp, sceNpManagerGetAvatarUrl);
	REG_FUNC(sceNp, sceNpManagerGetMyLanguages);
	REG_FUNC(sceNp, sceNpManagerGetAccountRegion);
	REG_FUNC(sceNp, sceNpManagerGetAccountAge);
	REG_FUNC(sceNp, sceNpManagerGetContentRatingFlag);
	REG_FUNC(sceNp, sceNpManagerGetChatRestrictionFlag);
	REG_FUNC(sceNp, sceNpManagerGetCachedInfo);
	REG_FUNC(sceNp, sceNpManagerGetPsHandle);
	REG_FUNC(sceNp, sceNpManagerRequestTicket);
	REG_FUNC(sceNp, sceNpManagerRequestTicket2);
	REG_FUNC(sceNp, sceNpManagerGetTicket);
	REG_FUNC(sceNp, sceNpManagerGetTicketParam);
	REG_FUNC(sceNp, sceNpManagerGetEntitlementIdList);
	REG_FUNC(sceNp, sceNpManagerGetEntitlementById);
	REG_FUNC(sceNp, sceNpManagerGetSigninId);
	REG_FUNC(sceNp, sceNpManagerSubSignin);
	REG_FUNC(sceNp, sceNpManagerSubSigninAbortGui);
	REG_FUNC(sceNp, sceNpManagerSubSignout);
	REG_FUNC(sceNp, sceNpMatchingCreateCtx);
	REG_FUNC(sceNp, sceNpMatchingDestroyCtx);
	REG_FUNC(sceNp, sceNpMatchingGetResult);
	REG_FUNC(sceNp, sceNpMatchingGetResultGUI);
	REG_FUNC(sceNp, sceNpMatchingSetRoomInfo);
	REG_FUNC(sceNp, sceNpMatchingSetRoomInfoNoLimit);
	REG_FUNC(sceNp, sceNpMatchingGetRoomInfo);
	REG_FUNC(sceNp, sceNpMatchingGetRoomInfoNoLimit);
	REG_FUNC(sceNp, sceNpMatchingSetRoomSearchFlag);
	REG_FUNC(sceNp, sceNpMatchingGetRoomSearchFlag);
	REG_FUNC(sceNp, sceNpMatchingGetRoomMemberListLocal);
	REG_FUNC(sceNp, sceNpMatchingGetRoomListLimitGUI);
	REG_FUNC(sceNp, sceNpMatchingKickRoomMember);
	REG_FUNC(sceNp, sceNpMatchingKickRoomMemberWithOpt);
	REG_FUNC(sceNp, sceNpMatchingQuickMatchGUI);
	REG_FUNC(sceNp, sceNpMatchingSendInvitationGUI);
	REG_FUNC(sceNp, sceNpMatchingAcceptInvitationGUI);
	REG_FUNC(sceNp, sceNpMatchingCreateRoomGUI);
	REG_FUNC(sceNp, sceNpMatchingJoinRoomGUI);
	REG_FUNC(sceNp, sceNpMatchingLeaveRoom);
	REG_FUNC(sceNp, sceNpMatchingSearchJoinRoomGUI);
	REG_FUNC(sceNp, sceNpMatchingGrantOwnership);
	REG_FUNC(sceNp, sceNpProfileCallGui);
	REG_FUNC(sceNp, sceNpProfileAbortGui);
	REG_FUNC(sceNp, sceNpScoreInit);
	REG_FUNC(sceNp, sceNpScoreTerm);
	REG_FUNC(sceNp, sceNpScoreCreateTitleCtx);
	REG_FUNC(sceNp, sceNpScoreDestroyTitleCtx);
	REG_FUNC(sceNp, sceNpScoreCreateTransactionCtx);
	REG_FUNC(sceNp, sceNpScoreDestroyTransactionCtx);
	REG_FUNC(sceNp, sceNpScoreSetTimeout);
	REG_FUNC(sceNp, sceNpScoreSetPlayerCharacterId);
	REG_FUNC(sceNp, sceNpScoreWaitAsync);
	REG_FUNC(sceNp, sceNpScorePollAsync);
	REG_FUNC(sceNp, sceNpScoreGetBoardInfo);
	REG_FUNC(sceNp, sceNpScoreGetBoardInfoAsync);
	REG_FUNC(sceNp, sceNpScoreRecordScore);
	REG_FUNC(sceNp, sceNpScoreRecordScoreAsync);
	REG_FUNC(sceNp, sceNpScoreRecordGameData);
	REG_FUNC(sceNp, sceNpScoreRecordGameDataAsync);
	REG_FUNC(sceNp, sceNpScoreGetGameData);
	REG_FUNC(sceNp, sceNpScoreGetGameDataAsync);
	REG_FUNC(sceNp, sceNpScoreGetRankingByNpId);
	REG_FUNC(sceNp, sceNpScoreGetRankingByNpIdAsync);
	REG_FUNC(sceNp, sceNpScoreGetRankingByRange);
	REG_FUNC(sceNp, sceNpScoreGetRankingByRangeAsync);
	REG_FUNC(sceNp, sceNpScoreGetFriendsRanking);
	REG_FUNC(sceNp, sceNpScoreGetFriendsRankingAsync);
	REG_FUNC(sceNp, sceNpScoreCensorComment);
	REG_FUNC(sceNp, sceNpScoreCensorCommentAsync);
	REG_FUNC(sceNp, sceNpScoreSanitizeComment);
	REG_FUNC(sceNp, sceNpScoreSanitizeCommentAsync);
	REG_FUNC(sceNp, sceNpScoreGetRankingByNpIdPcId);
	REG_FUNC(sceNp, sceNpScoreGetRankingByNpIdPcIdAsync);
	REG_FUNC(sceNp, sceNpScoreAbortTransaction);
	REG_FUNC(sceNp, sceNpScoreGetClansMembersRankingByNpId);
	REG_FUNC(sceNp, sceNpScoreGetClansMembersRankingByNpIdAsync);
	REG_FUNC(sceNp, sceNpScoreGetClansMembersRankingByNpIdPcId);
	REG_FUNC(sceNp, sceNpScoreGetClansMembersRankingByNpIdPcIdAsync);
	REG_FUNC(sceNp, sceNpScoreGetClansMembersRankingByRange);
	REG_FUNC(sceNp, sceNpScoreGetClansMembersRankingByRangeAsync);
	REG_FUNC(sceNp, sceNpScoreGetClanMemberGameData);
	REG_FUNC(sceNp, sceNpScoreGetClanMemberGameDataAsync);
	REG_FUNC(sceNp, sceNpScoreGetClansRankingByClanId);
	REG_FUNC(sceNp, sceNpScoreGetClansRankingByClanIdAsync);
	REG_FUNC(sceNp, sceNpScoreGetClansRankingByRange);
	REG_FUNC(sceNp, sceNpScoreGetClansRankingByRangeAsync);
	REG_FUNC(sceNp, sceNpSignalingCreateCtx);
	REG_FUNC(sceNp, sceNpSignalingDestroyCtx);
	REG_FUNC(sceNp, sceNpSignalingAddExtendedHandler);
	REG_FUNC(sceNp, sceNpSignalingSetCtxOpt);
	REG_FUNC(sceNp, sceNpSignalingGetCtxOpt);
	REG_FUNC(sceNp, sceNpSignalingActivateConnection);
	REG_FUNC(sceNp, sceNpSignalingDeactivateConnection);
	REG_FUNC(sceNp, sceNpSignalingTerminateConnection);
	REG_FUNC(sceNp, sceNpSignalingGetConnectionStatus);
	REG_FUNC(sceNp, sceNpSignalingGetConnectionInfo);
	REG_FUNC(sceNp, sceNpSignalingGetConnectionFromNpId);
	REG_FUNC(sceNp, sceNpSignalingGetConnectionFromPeerAddress);
	REG_FUNC(sceNp, sceNpSignalingGetLocalNetInfo);
	REG_FUNC(sceNp, sceNpSignalingGetPeerNetInfo);
	REG_FUNC(sceNp, sceNpSignalingCancelPeerNetInfo);
	REG_FUNC(sceNp, sceNpSignalingGetPeerNetInfoResult);
	REG_FUNC(sceNp, sceNpUtilCanonicalizeNpIdForPs3);
	REG_FUNC(sceNp, sceNpUtilCanonicalizeNpIdForPsp);
	REG_FUNC(sceNp, sceNpUtilCmpNpId);
	REG_FUNC(sceNp, sceNpUtilCmpNpIdInOrder);
	REG_FUNC(sceNp, sceNpUtilCmpOnlineId);
	REG_FUNC(sceNp, sceNpUtilGetPlatformType);
	REG_FUNC(sceNp, sceNpUtilSetPlatformType);
	REG_FUNC(sceNp, _sceNpSysutilClientMalloc);
	REG_FUNC(sceNp, _sceNpSysutilClientFree);
	REG_FUNC(sceNp, _Z33_sce_np_sysutil_send_empty_packetiPN16sysutil_cxmlutil11FixedMemoryEPKcS3_);
	REG_FUNC(sceNp, _Z27_sce_np_sysutil_send_packetiRN4cxml8DocumentE);
	REG_FUNC(sceNp, _Z36_sce_np_sysutil_recv_packet_fixedmemiPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentERNS2_7ElementE);
	REG_FUNC(sceNp, _Z40_sce_np_sysutil_recv_packet_fixedmem_subiPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentERNS2_7ElementE);
	REG_FUNC(sceNp, _Z27_sce_np_sysutil_recv_packetiRN4cxml8DocumentERNS_7ElementE);
	REG_FUNC(sceNp, _Z29_sce_np_sysutil_cxml_set_npidRN4cxml8DocumentERNS_7ElementEPKcPK7SceNpId);
	REG_FUNC(sceNp, _Z31_sce_np_sysutil_send_packet_subiRN4cxml8DocumentE);
	REG_FUNC(sceNp, _Z37sce_np_matching_set_matching2_runningb);
	REG_FUNC(sceNp, _Z32_sce_np_sysutil_cxml_prepare_docPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentEPKcRNS2_7ElementES6_i);
});
