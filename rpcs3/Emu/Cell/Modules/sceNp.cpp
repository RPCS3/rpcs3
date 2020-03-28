#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Emu/Cell/PPUModule.h"

#include "sysPrxForUser.h"
#include "Emu/IdManager.h"
#include "Crypto/unedat.h"
#include "Crypto/unself.h"
#include "cellRtc.h"
#include "sceNp.h"
#include "cellSysutil.h"

#include "Emu/NP/np_handler.h"

LOG_CHANNEL(sceNp);

template<>
void fmt_class_string<SceNpError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
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

error_code sceNpInit(u32 poolsize, vm::ptr<void> poolptr)
{
	sceNp.warning("sceNpInit(poolsize=0x%x, poolptr=*0x%x)", poolsize, poolptr);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (nph->is_NP_init)
	{
		return SCE_NP_ERROR_ALREADY_INITIALIZED;
	}

	if (poolsize == 0)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}
	else if (poolsize < SCE_NP_MIN_POOLSIZE)
	{
		return SCE_NP_ERROR_INSUFFICIENT_BUFFER;
	}

	if (!poolptr)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	nph->init_NP(poolsize, poolptr);
	nph->is_NP_init = true;

	return CELL_OK;
}

error_code sceNpTerm()
{
	sceNp.warning("sceNpTerm()");

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph->terminate_NP();
	nph->is_NP_init = false;

	return CELL_OK;
}

error_code npDrmIsAvailable(vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	std::array<u8, 0x10> k_licensee{};

	if (k_licensee_addr)
	{
		std::copy_n(k_licensee_addr.get_ptr(), k_licensee.size(), k_licensee.begin());
		sceNp.notice("npDrmIsAvailable(): KLicense key %s", *reinterpret_cast<be_t<v128, 1>*>(k_licensee.data()));
	}

	const std::string enc_drm_path(drm_path.get_ptr(), std::find(drm_path.get_ptr(), drm_path.get_ptr() + 0x100, '\0'));

	sceNp.warning(u8"npDrmIsAvailable(): drm_path=“%s”", enc_drm_path);

	if (!fs::is_file(vfs::get(enc_drm_path)))
	{
		sceNp.warning(u8"npDrmIsAvailable(): “%s” not found", enc_drm_path);
		return CELL_ENOENT;
	}

	auto npdrmkeys = g_fxo->get<loaded_npdrm_keys>();

	npdrmkeys->devKlic.fill(0);
	npdrmkeys->rifKey.fill(0);

	std::string rap_dir_path = "/dev_hdd0/home/" + Emu.GetUsr() + "/exdata/";

	const std::string& enc_drm_path_local = vfs::get(enc_drm_path);
	const fs::file enc_file(enc_drm_path_local);

	u32 magic;

	enc_file.read<u32>(magic);
	enc_file.seek(0);

	if (magic == "SCE\0"_u32)
	{
		if (!k_licensee_addr)
			k_licensee = get_default_self_klic();

		if (verify_npdrm_self_headers(enc_file, k_licensee.data()))
		{
			npdrmkeys->devKlic = std::move(k_licensee);
		}
		else
		{
			sceNp.error(u8"npDrmIsAvailable(): Failed to verify sce file “%s”", enc_drm_path);
			return SCE_NP_DRM_ERROR_NO_ENTITLEMENT;
		}
	}
	else if (magic == "NPD\0"_u32)
	{
		// edata / sdata files

		std::string contentID;

		if (VerifyEDATHeaderWithKLicense(enc_file, enc_drm_path_local, k_licensee, &contentID))
		{
			const std::string rap_file = rap_dir_path + contentID + ".rap";
			npdrmkeys->devKlic         = std::move(k_licensee);

			if (fs::is_file(vfs::get(rap_file)))
				npdrmkeys->rifKey = GetEdatRifKeyFromRapFile(fs::file{vfs::get(rap_file)});
			else
				sceNp.warning(u8"npDrmIsAvailable(): Rap file not found: “%s”", rap_file.c_str());
		}
		else
		{
			sceNp.error(u8"npDrmIsAvailable(): Failed to verify npd file “%s”", enc_drm_path);
			return SCE_NP_DRM_ERROR_NO_ENTITLEMENT;
		}
	}
	else
	{
		// for now assume its just unencrypted
		sceNp.notice(u8"npDrmIsAvailable(): Assuming npdrm file is unencrypted at “%s”", enc_drm_path);
	}
	return CELL_OK;
}

error_code sceNpDrmIsAvailable(vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	sceNp.warning("sceNpDrmIsAvailable(k_licensee=*0x%x, drm_path=*0x%x)", k_licensee_addr, drm_path);

	return npDrmIsAvailable(k_licensee_addr, drm_path);
}

error_code sceNpDrmIsAvailable2(vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	sceNp.warning("sceNpDrmIsAvailable2(k_licensee=*0x%x, drm_path=*0x%x)", k_licensee_addr, drm_path);

	return npDrmIsAvailable(k_licensee_addr, drm_path);
}

error_code sceNpDrmVerifyUpgradeLicense(vm::cptr<char> content_id)
{
	sceNp.warning("sceNpDrmVerifyUpgradeLicense(content_id=*0x%x)", content_id);

	if (!content_id)
	{
		return SCE_NP_DRM_ERROR_INVALID_PARAM;
	}

	const std::string content_str(content_id.get_ptr(), std::find(content_id.get_ptr(), content_id.get_ptr() + 0x2f, '\0'));

	sceNp.warning("sceNpDrmVerifyUpgradeLicense(): content_id=“%s”", content_id);

	if (!fs::is_file(vfs::get("/dev_hdd0/home/" + Emu.GetUsr() + "/exdata/" + content_str + ".rap")))
	{
		// Game hasn't been purchased therefore no RAP file present
		return SCE_NP_DRM_ERROR_LICENSE_NOT_FOUND;
	}

	// Game has been purchased and there's a RAP file present
	return CELL_OK;
}

error_code sceNpDrmVerifyUpgradeLicense2(vm::cptr<char> content_id)
{
	sceNp.warning("sceNpDrmVerifyUpgradeLicense2(content_id=*0x%x)", content_id);

	if (!content_id)
	{
		return SCE_NP_DRM_ERROR_INVALID_PARAM;
	}

	const std::string content_str(content_id.get_ptr(), std::find(content_id.get_ptr(), content_id.get_ptr() + 0x2f, '\0'));

	sceNp.warning("sceNpDrmVerifyUpgradeLicense2(): content_id=“%s”", content_id);

	if (!fs::is_file(vfs::get("/dev_hdd0/home/" + Emu.GetUsr() + "/exdata/" + content_str + ".rap")))
	{
		// Game hasn't been purchased therefore no RAP file present
		return SCE_NP_DRM_ERROR_LICENSE_NOT_FOUND;
	}

	// Game has been purchased and there's a RAP file present
	return CELL_OK;
}

error_code sceNpDrmExecuteGamePurchase()
{
	sceNp.todo("sceNpDrmExecuteGamePurchase()");
	return CELL_OK;
}

error_code sceNpDrmGetTimelimit(vm::cptr<char> path, vm::ptr<u64> time_remain)
{
	sceNp.todo("sceNpDrmGetTimelimit(path=%s, time_remain=*0x%x)", path, time_remain);

	if (!path || !time_remain)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	*time_remain = 0x7FFFFFFFFFFFFFFFULL;

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

	ppu_execute<&sys_game_process_exitspawn2>(ppu, path, argv, envp, data, data_size, prio, flags);
	return CELL_OK;
}

error_code sceNpBasicRegisterHandler(vm::cptr<SceNpCommunicationId> context, vm::ptr<SceNpBasicEventHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpBasicRegisterHandler(context=*0x%x, handler=*0x%x, arg=*0x%x)", context, handler, arg);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!context || !handler)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicRegisterContextSensitiveHandler(vm::cptr<SceNpCommunicationId> context, vm::ptr<SceNpBasicEventHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpBasicRegisterContextSensitiveHandler(context=*0x%x, handler=*0x%x, arg=*0x%x)", context, handler, arg);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!context || !handler)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicUnregisterHandler()
{
	sceNp.todo("sceNpBasicUnregisterHandler()");

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpBasicSetPresence(vm::cptr<void> data, u64 size)
{
	sceNp.todo("sceNpBasicSetPresence(data=*0x%x, size=%d)", data, size);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (size > SCE_NP_BASIC_MAX_PRESENCE_SIZE)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpBasicSetPresenceDetails(vm::cptr<SceNpBasicPresenceDetails> pres, u32 options)
{
	sceNp.todo("sceNpBasicSetPresenceDetails(pres=*0x%x, options=0x%x)", pres, options);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
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

error_code sceNpBasicSendMessage(vm::cptr<SceNpId> to, vm::cptr<void> data, u64 size)
{
	sceNp.todo("sceNpBasicSendMessage(to=*0x%x, data=*0x%x, size=%d)", to, data, size);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
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
	sceNp.todo("sceNpBasicSendMessageGui(msg=*0x%x, containerId=%d)", msg, containerId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!msg || msg->count > SCE_NP_BASIC_SEND_MESSAGE_MAX_RECIPIENTS || msg->npids.handle.data[0] == '\0' || !(msg->msgFeatures & SCE_NP_BASIC_MESSAGE_FEATURES_ALL_FEATURES))
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (msg->size > SCE_NP_BASIC_MAX_MESSAGE_SIZE)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return not_an_error(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
	}

	return CELL_OK;
}

error_code sceNpBasicSendMessageAttachment(vm::cptr<SceNpId> to, vm::cptr<char> subject, vm::cptr<char> body, vm::cptr<char> data, u64 size, sys_memory_container_t containerId)
{
	sceNp.todo("sceNpBasicSendMessageAttachment(to=*0x%x, subject=%s, body=%s, data=%s, size=%d, containerId=%d)", to, subject, body, data, size, containerId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!to || to->handle.data[0] == '\0' || !data || !size)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (strlen(subject.get_ptr()) > SCE_NP_BASIC_BODY_CHARACTER_MAX || strlen(body.get_ptr()) > SCE_NP_BASIC_BODY_CHARACTER_MAX)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return not_an_error(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
	}

	return CELL_OK;
}

error_code sceNpBasicRecvMessageAttachment(sys_memory_container_t containerId)
{
	sceNp.todo("sceNpBasicRecvMessageAttachment(containerId=%d)", containerId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpBasicRecvMessageAttachmentLoad(u32 id, vm::ptr<void> buffer, vm::ptr<u64> size)
{
	sceNp.todo("sceNpBasicRecvMessageAttachmentLoad(id=%d, buffer=*0x%x, size=*0x%x)", id, buffer, size);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!buffer || !size)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (id > SCE_NP_BASIC_SELECTED_MESSAGE_DATA)
	{
		return SCE_NP_BASIC_ERROR_INVALID_DATA_ID;
	}

	return CELL_OK;
}

error_code sceNpBasicRecvMessageCustom(u16 mainType, u32 recvOptions, sys_memory_container_t containerId)
{
	sceNp.todo("sceNpBasicRecvMessageCustom(mainType=%d, recvOptions=%d, containerId=%d)", mainType, recvOptions, containerId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!(recvOptions & SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_ALL_OPTIONS))
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicMarkMessageAsUsed(SceNpBasicMessageId msgId)
{
	sceNp.todo("sceNpBasicMarkMessageAsUsed(msgId=%d)", msgId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpBasicAddFriend(vm::cptr<SceNpId> contact, vm::cptr<char> body, sys_memory_container_t containerId)
{
	sceNp.todo("sceNpBasicAddFriend(contact=*0x%x, body=%s, containerId=%d)", contact, body, containerId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!contact || contact->handle.data[0] == '\0')
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (strlen(body.get_ptr()) > SCE_NP_BASIC_BODY_CHARACTER_MAX)
	{
		return SCE_NP_BASIC_ERROR_EXCEEDS_MAX;
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return not_an_error(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendListEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetFriendListEntryCount(count=*0x%x)", count);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are any friends
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetFriendListEntry(u32 index, vm::ptr<SceNpId> npid)
{
	sceNp.todo("sceNpBasicGetFriendListEntry(index=%d, npid=*0x%x)", index, npid);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendPresenceByIndex(u32 index, vm::ptr<SceNpUserInfo> user, vm::ptr<SceNpBasicPresenceDetails> pres, u32 options)
{
	sceNp.todo("sceNpBasicGetFriendPresenceByIndex(index=%d, user=*0x%x, pres=*0x%x, options=%d)", index, user, pres, options);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!user || !pres)
	{
		// TODO: check index and (options & SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS) depending on fw
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendPresenceByIndex2(u32 index, vm::ptr<SceNpUserInfo> user, vm::ptr<SceNpBasicPresenceDetails2> pres, u32 options)
{
	sceNp.todo("sceNpBasicGetFriendPresenceByIndex2(index=%d, user=*0x%x, pres=*0x%x, options=%d)", index, user, pres, options);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!user || !pres)
	{
		// TODO: check index and (options & SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS) depending on fw
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendPresenceByNpId(vm::cptr<SceNpId> npid, vm::ptr<SceNpBasicPresenceDetails> pres, u32 options)
{
	sceNp.todo("sceNpBasicGetFriendPresenceByNpId(npid=*0x%x, pres=*0x%x, options=%d)", npid, pres, options);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid || !pres)
	{
		// TODO: check (options & SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS) depending on fw
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicGetFriendPresenceByNpId2(vm::cptr<SceNpId> npid, vm::ptr<SceNpBasicPresenceDetails2> pres, u32 options)
{
	sceNp.todo("sceNpBasicGetFriendPresenceByNpId2(npid=*0x%x, pres=*0x%x, options=%d)", npid, pres, options);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid || !pres)
	{
		// TODO: check (options & SCE_NP_BASIC_PRESENCE_OPTIONS_ALL_OPTIONS) depending on fw
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpBasicAddPlayersHistory(vm::cptr<SceNpId> npid, vm::ptr<char> description)
{
	sceNp.todo("sceNpBasicAddPlayersHistory(npid=*0x%x, description=*0x%x)", npid, description);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	return CELL_OK;
}

error_code sceNpBasicGetPlayersHistoryEntryCount(u32 options, vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetPlayersHistoryEntryCount(options=%d, count=*0x%x)", options, count);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		// TODO: Check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicAddBlockListEntry(vm::cptr<SceNpId> npid)
{
	sceNp.warning("sceNpBasicAddBlockListEntry(npid=*0x%x)", npid);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid || npid->handle.data[0] == '\0')
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return not_an_error(SCE_NP_BASIC_ERROR_NOT_CONNECTED);
	}

	return CELL_OK;
}

error_code sceNpBasicGetBlockListEntryCount(vm::ptr<u32> count)
{
	sceNp.warning("sceNpBasicGetBlockListEntryCount(count=*0x%x)", count);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	// TODO: Check if there are block lists
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetBlockListEntry(u32 index, vm::ptr<SceNpId> npid)
{
	sceNp.todo("sceNpBasicGetBlockListEntry(index=%d, npid=*0x%x)", index, npid);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetMessageAttachmentEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetMessageAttachmentEntryCount(count=*0x%x)", count);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetCustomInvitationEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetCustomInvitationEntryCount(count=*0x%x)", count);

	if (!count)
	{
		return SCE_NP_AUTH_EINVAL;
	}

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	// TODO: Find the correct test which returns SCE_NP_AUTH_ESRCH
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_AUTH_ESRCH;
	}

	// TODO: Check if there are custom invitations
	*count = 0;

	return CELL_OK;
}

error_code sceNpBasicGetCustomInvitationEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetCustomInvitationEntry(index=%d, from=*0x%x)", index, from);

	if (!from)
	{
		// TODO: check index
		return SCE_NP_AUTH_EINVAL;
	}

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_AUTH_ESRCH;
	}

	return CELL_OK;
}

error_code sceNpBasicGetMatchingInvitationEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetMatchingInvitationEntryCount(count=*0x%x)", count);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetClanMessageEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetClanMessageEntryCount(count=*0x%x)", count);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetMessageEntryCount(u32 type, vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetMessageEntryCount(type=%d, count=*0x%x)", type, count);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!from)
	{
		// TODO: check index
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Find the correct test which returns SCE_NP_ERROR_ID_NOT_FOUND
	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_ID_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpBasicGetEvent(vm::ptr<s32> event, vm::ptr<SceNpUserInfo> from, vm::ptr<s32> data, vm::ptr<u32> size)
{
	sceNp.warning("sceNpBasicGetEvent(event=*0x%x, from=*0x%x, data=*0x%x, size=*0x%x)", event, from, data, size);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;
	}

	if (!event || !from || !data || !size)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Check for other error and pass other events
	//*event = SCE_NP_BASIC_EVENT_OFFLINE; // This event only indicates a contact is offline, not the current status of the connection

	return not_an_error(SCE_NP_BASIC_ERROR_NO_EVENT);
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	if (!menu || !handler)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpCustomMenuActionSetActivation(vm::cptr<SceNpCustomMenuIndexArray> array, u64 options)
{
	sceNp.todo("sceNpCustomMenuActionSetActivation(array=*0x%x, options=0x%x)", array, options);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	if (!array)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpCustomMenuRegisterExceptionList(vm::cptr<SceNpCustomMenuActionExceptions> items, u32 numItems, u64 options)
{
	sceNp.todo("sceNpCustomMenuRegisterExceptionList(items=*0x%x, numItems=%d, options=0x%x)", items, numItems, options);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	if (!items)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_INVALID_ARGUMENT;
	}

	if (numItems > SCE_NP_CUSTOM_MENU_ACTION_ITEMS_MAX)
	{
		// TODO: what about SCE_NP_CUSTOM_MENU_ACTION_ITEMS_TOTAL_MAX
		return SCE_NP_CUSTOM_MENU_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpFriendlist(vm::ptr<SceNpFriendlistResultHandler> resultHandler, vm::ptr<void> userArg, sys_memory_container_t containerId)
{
	sceNp.warning("sceNpFriendlist(resultHandler=*0x%x, userArg=*0x%x, containerId=%d)", resultHandler, userArg, containerId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	if (!resultHandler)
	{
		return SCE_NP_FRIENDLIST_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpFriendlistCustom(SceNpFriendlistCustomOptions options, vm::ptr<SceNpFriendlistResultHandler> resultHandler, vm::ptr<void> userArg, sys_memory_container_t containerId)
{
	sceNp.warning("sceNpFriendlistCustom(options=0x%x, resultHandler=*0x%x, userArg=*0x%x, containerId=%d)", options, resultHandler, userArg, containerId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	if (!resultHandler)
	{
		return SCE_NP_FRIENDLIST_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpFriendlistAbortGui()
{
	sceNp.todo("sceNpFriendlistAbortGui()");

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_CUSTOM_MENU_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupInit()
{
	sceNp.todo("sceNpLookupInit()");

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;
	}

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph->is_NP_Lookup_init = true;

	return CELL_OK;
}

error_code sceNpLookupTerm()
{
	sceNp.todo("sceNpLookupTerm()");

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph->is_NP_Lookup_init = false;

	return CELL_OK;
}

error_code sceNpLookupCreateTitleCtx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpId> selfNpId)
{
	sceNp.todo("sceNpLookupCreateTitleCtx(communicationId=*0x%x, selfNpId=0x%x)", communicationId, selfNpId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!communicationId || !selfNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	return not_an_error(nph->create_lookup_context(communicationId));
}

error_code sceNpLookupDestroyTitleCtx(s32 titleCtxId)
{
	sceNp.todo("sceNpLookupDestroyTitleCtx(titleCtxId=%d)", titleCtxId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!nph->destroy_lookup_context(titleCtxId))
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;

	return CELL_OK;
}

error_code sceNpLookupCreateTransactionCtx(s32 titleCtxId)
{
	sceNp.todo("sceNpLookupCreateTransactionCtx(titleCtxId=%d)", titleCtxId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupDestroyTransactionCtx(s32 transId)
{
	sceNp.todo("sceNpLookupDestroyTransactionCtx(transId=%d)", transId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupSetTimeout(s32 ctxId, usecond_t timeout)
{
	sceNp.todo("sceNpLookupSetTimeout(ctxId=%d, timeout=%d)", ctxId, timeout);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (timeout > 10000000) // 10 seconds
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpLookupAbortTransaction(s32 transId)
{
	sceNp.todo("sceNpLookupAbortTransaction(transId=%d)", transId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupWaitAsync(s32 transId, vm::ptr<s32> result)
{
	sceNp.todo("sceNpLookupWaitAsync(transId=%d, result=%d)", transId, result);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupPollAsync(s32 transId, vm::ptr<s32> result)
{
	sceNp.todo("sceNpLookupPollAsync(transId=%d, result=%d)", transId, result);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupNpId(s32 transId, vm::cptr<SceNpOnlineId> onlineId, vm::ptr<SceNpId> npId, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupNpId(transId=%d, onlineId=*0x%x, npId=*0x%x, option=*0x%x)", transId, onlineId, npId, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupNpIdAsync(s32 transId, vm::ptr<SceNpOnlineId> onlineId, vm::ptr<SceNpId> npId, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupNpIdAsync(transId=%d, onlineId=*0x%x, npId=*0x%x, prio=%d, option=*0x%x)", transId, onlineId, npId, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupAvatarImage(s32 transId, vm::ptr<SceNpAvatarUrl> avatarUrl, vm::ptr<SceNpAvatarImage> avatarImage, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupAvatarImage(transId=%d, avatarUrl=*0x%x, avatarImage=*0x%x, option=*0x%x)", transId, avatarUrl, avatarImage, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupAvatarImageAsync(s32 transId, vm::ptr<SceNpAvatarUrl> avatarUrl, vm::ptr<SceNpAvatarImage> avatarImage, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupAvatarImageAsync(transId=%d, avatarUrl=*0x%x, avatarImage=*0x%x, prio=%d, option=*0x%x)", transId, avatarUrl, avatarImage, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupTitleStorage()
{
	UNIMPLEMENTED_FUNC(sceNp);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupTitleStorageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpLookupTitleSmallStorage(s32 transId, vm::ptr<void> data, u64 maxSize, vm::ptr<u64> contentLength, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupTitleSmallStorage(transId=%d, data=*0x%x, maxSize=%d, contentLength=*0x%x, option=*0x%x)", transId, data, maxSize, contentLength, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpLookupTitleSmallStorageAsync(s32 transId, vm::ptr<void> data, u64 maxSize, vm::ptr<u64> contentLength, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpLookupTitleSmallStorageAsync(transId=%d, data=*0x%x, maxSize=%d, contentLength=*0x%x, prio=%d, option=*0x%x)", transId, data, maxSize, contentLength, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Lookup_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpManagerRegisterCallback(vm::ptr<SceNpManagerCallback> callback, vm::ptr<void> arg)
{
	sceNp.warning("sceNpManagerRegisterCallback(callback=*0x%x, arg=*0x%x)", callback, arg);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!callback)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpManagerUnregisterCallback()
{
	sceNp.warning("sceNpManagerUnregisterCallback()");

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpManagerGetStatus(vm::ptr<s32> status)
{
	sceNp.trace("sceNpManagerGetStatus(status=*0x%x)", status);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!status)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	*status = nph->get_psn_status();

	return CELL_OK;
}

error_code sceNpManagerGetNetworkTime(vm::ptr<CellRtcTick> pTick)
{
	sceNp.warning("sceNpManagerGetNetworkTime(pTick=*0x%x)", pTick);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!pTick)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	// FIXME: Get the network time
	auto now    = std::chrono::system_clock::now();
	pTick->tick = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

	return CELL_OK;
}

error_code sceNpManagerGetOnlineId(vm::ptr<SceNpOnlineId> onlineId)
{
	sceNp.todo("sceNpManagerGetOnlineId(onlineId=*0x%x)", onlineId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!onlineId)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memcpy(onlineId.get_ptr(), &nph->get_online_id(), onlineId.size());

	return CELL_OK;
}

error_code sceNpManagerGetNpId(ppu_thread& ppu, vm::ptr<SceNpId> npId)
{
	sceNp.todo("sceNpManagerGetNpId(npId=*0x%x)", npId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!npId)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memcpy(npId.get_ptr(), &nph->get_npid(), npId.size());

	return CELL_OK;
}

error_code sceNpManagerGetOnlineName(vm::ptr<SceNpOnlineName> onlineName)
{
	sceNp.warning("sceNpManagerGetOnlineName(onlineName=*0x%x)", onlineName);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!onlineName)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memcpy(onlineName.get_ptr(), &nph->get_online_name(), onlineName.size());

	return CELL_OK;
}

error_code sceNpManagerGetAvatarUrl(vm::ptr<SceNpAvatarUrl> avatarUrl)
{
	sceNp.warning("sceNpManagerGetAvatarUrl(avatarUrl=*0x%x)", avatarUrl);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!avatarUrl)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	memcpy(avatarUrl.get_ptr(), &nph->get_avatar_url(), avatarUrl.size());

	return CELL_OK;
}

error_code sceNpManagerGetMyLanguages(vm::ptr<SceNpMyLanguages> myLanguages)
{
	sceNp.warning("sceNpManagerGetMyLanguages(myLanguages=*0x%x)", myLanguages);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!myLanguages)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

error_code sceNpManagerGetAccountRegion(vm::ptr<SceNpCountryCode> countryCode, vm::ptr<s32> language)
{
	sceNp.warning("sceNpManagerGetAccountRegion(countryCode=*0x%x, language=*0x%x)", countryCode, language);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!countryCode || !language)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

error_code sceNpManagerGetAccountAge(vm::ptr<s32> age)
{
	sceNp.warning("sceNpManagerGetAccountAge(age=*0x%x)", age);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!age)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

error_code sceNpManagerGetContentRatingFlag(vm::ptr<s32> isRestricted, vm::ptr<s32> age)
{
	sceNp.warning("sceNpManagerGetContentRatingFlag(isRestricted=*0x%x, age=*0x%x)", isRestricted, age);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!isRestricted || !age)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!isRestricted)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	// TODO: read user's parental control information
	*isRestricted = 0;

	return CELL_OK;
}

error_code sceNpManagerGetCachedInfo(CellSysutilUserId userId, vm::ptr<SceNpManagerCacheParam> param)
{
	sceNp.todo("sceNpManagerGetChatRestrictionFlag(userId=%d, param=*0x%x)", userId, param);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!param)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpManagerGetPsHandle()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

error_code sceNpManagerRequestTicket(vm::cptr<SceNpId> npId, vm::cptr<char> serviceId, vm::cptr<void> cookie, u32 cookieSize, vm::cptr<char> entitlementId, u32 consumedCount)
{
	sceNp.todo("sceNpManagerRequestTicket(npId=*0x%x, serviceId=%s, cookie=*0x%x, cookieSize=%d, entitlementId=%s, consumedCount=%d)", npId, serviceId, cookie, cookieSize, entitlementId, consumedCount);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!serviceId || !cookie || cookieSize > SCE_NP_COOKIE_MAX_SIZE || !entitlementId)
	{
		return SCE_NP_AUTH_EINVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

error_code sceNpManagerRequestTicket2(vm::cptr<SceNpId> npId, vm::cptr<SceNpTicketVersion> version, vm::cptr<char> serviceId,
	vm::cptr<void> cookie, u32 cookieSize, vm::cptr<char> entitlementId, u32 consumedCount)
{
	sceNp.todo("sceNpManagerRequestTicket2(npId=*0x%x, version=*0x%x, serviceId=%s, cookie=*0x%x, cookieSize=%d, entitlementId=%s, consumedCount=%d)", npId, version, serviceId, cookie, cookieSize,
	    entitlementId, consumedCount);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!serviceId || !cookie || cookieSize > SCE_NP_COOKIE_MAX_SIZE || !entitlementId)
	{
		return SCE_NP_AUTH_EINVALID_ARGUMENT;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return not_an_error(SCE_NP_ERROR_OFFLINE);
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_LOGGING_IN && nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

error_code sceNpManagerGetTicket(vm::ptr<void> buffer, vm::ptr<u32> bufferSize)
{
	sceNp.todo("sceNpManagerGetTicket(buffer=*0x%x, bufferSize=*0x%x)", buffer, bufferSize);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!bufferSize)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpManagerGetTicketParam(s32 paramId, vm::ptr<SceNpTicketParam> param)
{
	sceNp.todo("sceNpManagerGetTicketParam(paramId=%d, param=*0x%x)", paramId, param);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!param)
	{
		// TODO: check paramId
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpManagerGetEntitlementIdList(vm::ptr<SceNpEntitlementId> entIdList, u32 entIdListNum)
{
	sceNp.todo("sceNpManagerGetEntitlementIdList(entIdList=*0x%x, entIdListNum=%d)", entIdList, entIdListNum);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpManagerGetEntitlementById(vm::cptr<char> entId, vm::ptr<SceNpEntitlement> ent)
{
	sceNp.todo("sceNpManagerGetEntitlementById(entId=%s, ent=*0x%x)", entId, ent);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpScoreInit()
{
	sceNp.warning("sceNpScoreInit()");

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;
	}

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph->is_NP_Score_init = true;

	return CELL_OK;
}

error_code sceNpScoreTerm()
{
	sceNp.warning("sceNpScoreTerm()");

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!nph->is_NP_init)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	nph->is_NP_Score_init = false;

	return CELL_OK;
}

error_code sceNpScoreCreateTitleCtx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase, vm::cptr<SceNpId> selfNpId)
{
	sceNp.todo("sceNpScoreCreateTitleCtx(communicationId=*0x%x, passphrase=*0x%x, selfNpId=*0x%x)", communicationId, passphrase, selfNpId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!communicationId || !passphrase || !selfNpId)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	return not_an_error(nph->create_score_context(communicationId, passphrase));
}

error_code sceNpScoreDestroyTitleCtx(s32 titleCtxId)
{
	sceNp.todo("sceNpScoreDestroyTitleCtx(titleCtxId=%d)", titleCtxId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!nph->destroy_score_context(titleCtxId))
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;

	return CELL_OK;
}

error_code sceNpScoreCreateTransactionCtx(s32 titleCtxId)
{
	sceNp.todo("sceNpScoreCreateTransactionCtx(titleCtxId=%d)", titleCtxId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (nph->get_psn_status() == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreDestroyTransactionCtx(s32 transId)
{
	sceNp.todo("sceNpScoreDestroyTransactionCtx(transId=%d)", transId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpScoreSetTimeout(s32 ctxId, usecond_t timeout)
{
	sceNp.todo("sceNpScoreSetTimeout(ctxId=%d, timeout=%d)", ctxId, timeout);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (timeout > 10000000) // 10 seconds
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreSetPlayerCharacterId(s32 ctxId, SceNpScorePcId pcId)
{
	sceNp.todo("sceNpScoreSetPlayerCharacterId(ctxId=%d, pcId=%d)", ctxId, pcId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (pcId < 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreWaitAsync(s32 transId, vm::ptr<s32> result)
{
	sceNp.todo("sceNpScoreWaitAsync(transId=%d, result=*0x%x)", transId, result);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (transId <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code sceNpScorePollAsync(s32 transId, vm::ptr<s32> result)
{
	sceNp.todo("sceNpScorePollAsync(transId=%d, result=*0x%x)", transId, result);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (transId <= 0)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetBoardInfo(s32 transId, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetBoardInfo(transId=%d, boardId=%d, boardInfo=*0x%x, option=*0x%x)", transId, boardId, boardInfo, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetBoardInfoAsync(s32 transId, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetBoardInfo(transId=%d, boardId=%d, boardInfo=*0x%x, prio=%d, option=*0x%x)", transId, boardId, boardInfo, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreRecordScore(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, vm::cptr<SceNpScoreGameInfo> gameInfo,
    vm::ptr<SceNpScoreRankNumber> tmpRank, vm::ptr<SceNpScoreRecordOptParam> option)
{
	sceNp.todo(
	    "sceNpScoreRecordScore(transId=%d, boardId=%d, score=%d, scoreComment=*0x%x, gameInfo=*0x%x, tmpRank=*0x%x, option=*0x%x)", transId, boardId, score, scoreComment, gameInfo, tmpRank, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreRecordScoreAsync(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, vm::cptr<SceNpScoreGameInfo> gameInfo,
    vm::ptr<SceNpScoreRankNumber> tmpRank, s32 prio, vm::ptr<SceNpScoreRecordOptParam> option)
{
	sceNp.todo("sceNpScoreRecordScoreAsync(transId=%d, boardId=%d, score=%d, scoreComment=*0x%x, gameInfo=*0x%x, tmpRank=*0x%x, prio=%d, option=*0x%x)", transId, boardId, score, scoreComment, gameInfo,
	    tmpRank, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreRecordGameData(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, u64 totalSize, u64 sendSize, vm::cptr<void> data, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreRecordGameData(transId=%d, boardId=%d, score=%d, totalSize=%d, sendSize=%d, data=*0x%x, option=*0x%x)", transId, boardId, score, totalSize, sendSize, data, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreRecordGameDataAsync(s32 transId, SceNpScoreBoardId boardId, SceNpScoreValue score, u64 totalSize, u64 sendSize, vm::cptr<void> data, s32 prio, vm::ptr<void> option)
{
	sceNp.todo(
	    "sceNpScoreRecordGameDataAsync(transId=%d, boardId=%d, score=%d, totalSize=%d, sendSize=%d, data=*0x%x, prio=%d, option=*0x%x)", transId, boardId, score, totalSize, sendSize, data, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetGameData(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npId, vm::ptr<u64> totalSize, u64 recvSize, vm::ptr<void> data, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetGameDataAsync(transId=%d, boardId=%d, npId=*0x%x, totalSize=*0x%x, recvSize=%d, data=*0x%x, option=*0x%x)", transId, boardId, npId, totalSize, recvSize, data, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetGameDataAsync(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npId, vm::ptr<u64> totalSize, u64 recvSize, vm::ptr<void> data, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetGameDataAsync(transId=%d, boardId=%d, npId=*0x%x, totalSize=*0x%x, recvSize=%d, data=*0x%x, prio=%d, option=*0x%x)", transId, boardId, npId, totalSize, recvSize, data, prio,
	    option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetRankingByNpId(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npIdArray, u64 npIdArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray, u64 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetRankingByNpId(transId=%d, boardId=%d, npIdArray=*0x%x, npIdArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, boardId, npIdArray, npIdArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npIdArray || !rankArray || !totalRecord || !lastSortDate || !arrayNum)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetRankingByNpIdAsync(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> npIdArray, u64 npIdArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray, u64 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetRankingByNpIdAsync(transId=%d, boardId=%d, npIdArray=*0x%x, npIdArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, boardId, npIdArray, npIdArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!npIdArray || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetRankingByRange(s32 transId, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u64 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetRankingByRange(transId=%d, boardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, infoArraySize=%d, "
	           "arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetRankingByRangeAsync(s32 transId, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u64 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetRankingByRangeAsync(transId=%d, boardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

error_code sceNpScoreGetFriendsRanking(s32 transId, SceNpScoreBoardId boardId, s32 includeSelf, vm::ptr<SceNpScoreRankData> rankArray, u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray,
    u64 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetFriendsRanking(transId=%d, boardId=%d, includeSelf=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, infoArraySize=%d, "
	           "arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, boardId, includeSelf, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!rankArray || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_SELECTED_FRIENDS_NUM || option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetFriendsRankingAsync(s32 transId, SceNpScoreBoardId boardId, s32 includeSelf, vm::ptr<SceNpScoreRankData> rankArray, u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray,
    u64 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio,
    vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetFriendsRankingAsync(transId=%d, boardId=%d, includeSelf=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, infoArraySize=%d, "
	           "arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, boardId, includeSelf, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (!rankArray || !arrayNum)
	{
		return SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
	}

	if (arrayNum > SCE_NP_SCORE_MAX_SELECTED_FRIENDS_NUM || option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreCensorComment(s32 transId, vm::cptr<char> comment, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreCensorComment(transId=%d, comment=%s, option=*0x%x)", transId, comment, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreCensorCommentAsync(s32 transId, vm::cptr<char> comment, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreCensorCommentAsync(transId=%d, comment=%s, prio=%d, option=*0x%x)", transId, comment, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	return CELL_OK;
}

error_code sceNpScoreSanitizeComment(s32 transId, vm::cptr<char> comment, vm::ptr<char> sanitizedComment, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreSanitizeComment(transId=%d, comment=%s, sanitizedComment=*0x%x, option=*0x%x)", transId, comment, sanitizedComment, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreSanitizeCommentAsync(s32 transId, vm::cptr<char> comment, vm::ptr<char> sanitizedComment, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreSanitizeCommentAsync(transId=%d, comment=%s, sanitizedComment=*0x%x, prio=%d, option=*0x%x)", transId, comment, sanitizedComment, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	return CELL_OK;
}

error_code sceNpScoreGetRankingByNpIdPcId(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpScoreNpIdPcId> idArray, u64 idArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray, u64 rankArraySize,
    vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetRankingByNpIdPcId(transId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetRankingByNpIdPcIdAsync(s32 transId, SceNpScoreBoardId boardId, vm::cptr<SceNpScoreNpIdPcId> idArray, u64 idArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray,
    u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetRankingByNpIdPcIdAsync(transId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, infoArray=*0x%x, "
	           "infoArraySize=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, arrayNum, lastSortDate, totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

error_code sceNpScoreAbortTransaction(s32 transId)
{
	sceNp.todo("sceNpScoreAbortTransaction(transId=%d)", transId);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByNpId(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> idArray, u64 idArraySize, vm::ptr<SceNpScorePlayerRankData> rankArray,
    u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u64 infoArraySize, vm::ptr<SceNpScoreClansMemberDescription> descriptArray,
    u64 descriptArraySize, u64 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByNpId(transId=%d, clanId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo,
	    lastSortDate, totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByNpIdAsync(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> idArray, u64 idArraySize,
    vm::ptr<SceNpScorePlayerRankData> rankArray, u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u64 infoArraySize,
    vm::ptr<SceNpScoreClansMemberDescription> descriptArray, u64 descriptArraySize, u64 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByNpIdAsync(transId=%d, clanId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo,
	    lastSortDate, totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

error_code sceNpScoreGetClansMembersRankingByNpIdPcId(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> idArray, u64 idArraySize,
    vm::ptr<SceNpScorePlayerRankData> rankArray, u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u64 infoArraySize,
    vm::ptr<SceNpScoreClansMemberDescription> descriptArray, u64 descriptArraySize, u64 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByNpIdPcId(transId=%d, clanId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo,
	    lastSortDate, totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByNpIdPcIdAsync(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, vm::cptr<SceNpId> idArray, u64 idArraySize,
    vm::ptr<SceNpScorePlayerRankData> rankArray, u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u64 infoArraySize,
    vm::ptr<SceNpScoreClansMemberDescription> descriptArray, u64 descriptArraySize, u64 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo(
	    "sceNpScoreGetClansMembersRankingByNpIdPcIdAsync(transId=%d, clanId=%d, boardId=%d, idArray=*0x%x, idArraySize=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	    "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanId, boardId, idArray, idArraySize, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo,
	    lastSortDate, totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

error_code sceNpScoreGetClansRankingByRange(s32 transId, SceNpScoreClansBoardId clanBoardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreClanIdRankData> rankArray, u64 rankArraySize,
    vm::ptr<void> reserved1, u64 reservedSize1, vm::ptr<void> reserved2, u64 reservedSize2, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord,
    vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansRankingByRange(transId=%d, clanBoardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, reserved1=*0x%x, reservedSize1=%d, reserved2=*0x%x, reservedSize2=%d, "
	           "arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanBoardId, startSerialRank, rankArray, rankArraySize, reserved1, reservedSize1, reserved2, reservedSize2, arrayNum, lastSortDate, totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansRankingByRangeAsync(s32 transId, SceNpScoreClansBoardId clanBoardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreClanIdRankData> rankArray, u64 rankArraySize,
    vm::ptr<void> reserved1, u64 reservedSize1, vm::ptr<void> reserved2, u64 reservedSize2, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio,
    vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansRankingByRangeAsync(transId=%d, clanBoardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, reserved1=*0x%x, reservedSize1=%d, reserved2=*0x%x, "
	           "reservedSize2=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanBoardId, startSerialRank, rankArray, rankArraySize, reserved1, reservedSize1, reserved2, reservedSize2, arrayNum, lastSortDate, totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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
    s32 transId, SceNpScoreBoardId boardId, SceNpClanId clanId, vm::cptr<SceNpId> npId, vm::ptr<u64> totalSize, u64 recvSize, vm::ptr<void> data, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClanMemberGameData(transId=%d, boardId=%d, clanId=%d, npId=*0x%x, totalSize=*0x%x, recvSize=%d, data=*0x%x, option=*0x%x)", transId, boardId, clanId, npId, totalSize,
	    recvSize, data, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClanMemberGameDataAsync(
    s32 transId, SceNpScoreBoardId boardId, SceNpClanId clanId, vm::cptr<SceNpId> npId, vm::ptr<u64> totalSize, u64 recvSize, vm::ptr<void> data, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClanMemberGameDataAsync(transId=%d, boardId=%d, clanId=%d, npId=*0x%x, totalSize=*0x%x, recvSize=%d, data=*0x%x, prio=%d, option=*0x%x)", transId, boardId, clanId, npId,
	    totalSize, recvSize, data, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
	{
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	}

	if (option) // option check at least until fw 4.71
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansRankingByClanId(s32 transId, SceNpScoreClansBoardId clanBoardId, vm::cptr<SceNpClanId> clanIdArray, u64 clanIdArraySize, vm::ptr<SceNpScoreClanIdRankData> rankArray,
    u64 rankArraySize, vm::ptr<void> reserved1, u64 reservedSize1, vm::ptr<void> reserved2, u64 reservedSize2, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansRankingByClanId(transId=%d, clanBoardId=%d, clanIdArray=*0x%x, clanIdArraySize=%d, rankArray=*0x%x, rankArraySize=%d, reserved1=*0x%x, reservedSize1=%d, "
	           "reserved2=*0x%x, reservedSize2=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanBoardId, clanIdArray, clanIdArraySize, rankArray, rankArraySize, reserved1, reservedSize1, reserved2, reservedSize2, arrayNum, lastSortDate, totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansRankingByClanIdAsync(s32 transId, SceNpScoreClansBoardId clanBoardId, vm::cptr<SceNpClanId> clanIdArray, u64 clanIdArraySize, vm::ptr<SceNpScoreClanIdRankData> rankArray,
    u64 rankArraySize, vm::ptr<void> reserved1, u64 reservedSize1, vm::ptr<void> reserved2, u64 reservedSize2, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate,
    vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansRankingByRangeAsync(transId=%d, clanBoardId=%d, clanIdArray=*0x%x, clanIdArraySize=%d, rankArray=*0x%x, rankArraySize=%d, reserved1=*0x%x, reservedSize1=%d, "
	           "reserved2=*0x%x, reservedSize2=%d, arrayNum=%d, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanBoardId, clanIdArray, clanIdArraySize, rankArray, rankArraySize, reserved1, reservedSize1, reserved2, reservedSize2, arrayNum, lastSortDate, totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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
    u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u64 infoArraySize, vm::ptr<SceNpScoreClansMemberDescription> descriptArray,
    u64 descriptArraySize, u64 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByRange(transId=%d, clanId=%d, boardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, option=*0x%x)",
	    transId, clanId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo, lastSortDate,
	    totalRecord, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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

	if (nph->get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID;
	}

	return CELL_OK;
}

error_code sceNpScoreGetClansMembersRankingByRangeAsync(s32 transId, SceNpClanId clanId, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreClanIdRankData> rankArray,
    u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u64 commentArraySize, vm::ptr<SceNpScoreGameInfo> infoArray, u64 infoArraySize, vm::ptr<SceNpScoreClansMemberDescription> descriptArray,
    u64 descriptArraySize, u64 arrayNum, vm::ptr<SceNpScoreClanBasicInfo> clanInfo, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, s32 prio, vm::ptr<void> option)
{
	sceNp.todo("sceNpScoreGetClansMembersRankingByRangeAsync(transId=%d, clanId=%d, boardId=%d, startSerialRank=%d, rankArray=*0x%x, rankArraySize=%d, commentArray=*0x%x, commentArraySize=%d, "
	           "infoArray=*0x%x, infoArraySize=%d, descriptArray=*0x%x, descriptArraySize=%d, arrayNum=%d, clanInfo=*0x%x, lastSortDate=*0x%x, totalRecord=*0x%x, prio=%d, option=*0x%x)",
	    transId, clanId, boardId, startSerialRank, rankArray, rankArraySize, commentArray, commentArraySize, infoArray, infoArraySize, descriptArray, descriptArraySize, arrayNum, clanInfo, lastSortDate,
	    totalRecord, prio, option);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_Score_init)
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
	sceNp.todo("sceNpSignalingCreateCtx(npId=*0x%x, handler=*0x%x, arg=*0x%x, ctx_id=*0x%x)", npId, handler, arg, ctx_id);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	return CELL_OK;
}

error_code sceNpSignalingDestroyCtx(u32 ctx_id)
{
	sceNp.todo("sceNpSignalingDestroyCtx(ctx_id=%d)", ctx_id);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpSignalingAddExtendedHandler(u32 ctx_id, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg)
{
	sceNp.todo("sceNpSignalingAddExtendedHandler(ctx_id=%d, handler=*0x%x, arg=*0x%x)", ctx_id, handler, arg);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpSignalingSetCtxOpt(u32 ctx_id, s32 optname, s32 optval)
{
	sceNp.todo("sceNpSignalingSetCtxOpt(ctx_id=%d, optname=%d, optval=%d)", ctx_id, optname, optval);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!optname || !optval)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingActivateConnection(u32 ctx_id, vm::ptr<SceNpId> npId, u32 conn_id)
{
	sceNp.todo("sceNpSignalingActivateConnection(ctx_id=%d, npId=*0x%x, conn_id=%d)", ctx_id, npId, conn_id);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !conn_id)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingDeactivateConnection(u32 ctx_id, u32 conn_id)
{
	sceNp.todo("sceNpSignalingDeactivateConnection(ctx_id=%d, conn_id=%d)", ctx_id, conn_id);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpSignalingTerminateConnection(u32 ctx_id, u32 conn_id)
{
	sceNp.todo("sceNpSignalingTerminateConnection(ctx_id=%d, conn_id=%d)", ctx_id, conn_id);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpSignalingGetConnectionStatus(u32 ctx_id, u32 conn_id, vm::ptr<s32> conn_status, vm::ptr<np_in_addr> peer_addr, vm::ptr<np_in_port_t> peer_port)
{
	sceNp.todo("sceNpSignalingGetConnectionStatus(ctx_id=%d, conn_id=%d, conn_status=*0x%x, peer_addr=*0x%x, peer_port=*0x%x)", ctx_id, conn_id, conn_status, peer_addr, peer_port);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!conn_status)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingGetConnectionInfo(u32 ctx_id, u32 conn_id, s32 code, vm::ptr<SceNpSignalingConnectionInfo> info)
{
	sceNp.todo("sceNpSignalingGetConnectionInfo(ctx_id=%d, conn_id=%d, code=%d, info=*0x%x)", ctx_id, conn_id, code, info);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingGetConnectionFromNpId(u32 ctx_id, vm::ptr<SceNpId> npId, vm::ptr<u32> conn_id)
{
	sceNp.todo("sceNpSignalingGetConnectionFromNpId(ctx_id=%d, npId=*0x%x, conn_id=*0x%x)", ctx_id, npId, conn_id);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!npId || !conn_id)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingGetConnectionFromPeerAddress(u32 ctx_id, vm::ptr<np_in_addr> peer_addr, np_in_port_t peer_port, vm::ptr<u32> conn_id)
{
	sceNp.todo("sceNpSignalingGetConnectionFromPeerAddress(ctx_id=%d, peer_addr=*0x%x, peer_port=%d, conn_id=*0x%x)", ctx_id, peer_addr, peer_port, conn_id);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	if (!conn_id)
	{
		return SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpSignalingGetLocalNetInfo(u32 ctx_id, vm::ptr<SceNpSignalingNetInfo> info)
{
	sceNp.todo("sceNpSignalingGetLocalNetInfo(ctx_id=%d, info=*0x%x)", ctx_id, info);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

error_code sceNpSignalingGetPeerNetInfo(u32 ctx_id, vm::ptr<SceNpId> npId, vm::ptr<u32> req_id)
{
	sceNp.todo("sceNpSignalingGetPeerNetInfo(ctx_id=%d, npId=*0x%x, req_id=*0x%x)", ctx_id, npId, req_id);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
	{
		return SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpSignalingGetPeerNetInfoResult(u32 ctx_id, u32 req_id, vm::ptr<SceNpSignalingNetInfo> info)
{
	sceNp.todo("sceNpSignalingGetPeerNetInfoResult(ctx_id=%d, req_id=%d, info=*0x%x)", ctx_id, req_id, info);

	const auto nph = g_fxo->get<named_thread<np_handler>>();

	if (!nph->is_NP_init)
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
	sceNp.warning("sceNpUtilCmpNpId(id1=*0x%x(%s), id2=*0x%x(%s))", id1, id1->handle.data, id2, id2->handle.data);

	if (!id1 || !id2)
	{
		return SCE_NP_UTIL_ERROR_INVALID_ARGUMENT;
	}

	// Unknown what this constant means
	if (id1->reserved[0] != 1 || id2->reserved[0] != 1)
	{
		return SCE_NP_UTIL_ERROR_INVALID_NP_ID;
	}

	if (strncmp(id1->handle.data, id2->handle.data, 16) || id1->unk1[0] != id2->unk1[0])
	{
		return SCE_NP_UTIL_ERROR_NOT_MATCH;
	}

	if (id1->unk1[1] != id2->unk1[1])
	{
		// If either is zero they match
		if (id1->opt[4] && id2->opt[4])
		{
			return SCE_NP_UTIL_ERROR_NOT_MATCH;
		}
	}

	return CELL_OK;
}

error_code sceNpUtilCmpNpIdInOrder(vm::cptr<SceNpId> id1, vm::cptr<SceNpId> id2, vm::ptr<s32> order)
{
	sceNp.warning("sceNpUtilCmpNpIdInOrder(id1=*0x%x, id2=*0x%x, order=*0x%x)", id1, id2, order);

	if (!id1 || !id2)
	{
		return SCE_NP_UTIL_ERROR_INVALID_ARGUMENT;
	}

	if (id1->reserved[0] != 1 || id2->reserved[0] != 1)
	{
		return SCE_NP_UTIL_ERROR_INVALID_NP_ID;
	}

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

	if (id1->reserved[0] != 1 || id2->reserved[0] != 1)
	{
		return SCE_NP_UTIL_ERROR_INVALID_NP_ID;
	}

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
