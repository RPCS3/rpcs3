#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/IdManager.h"
#include "Crypto/unedat.h"
#include "Crypto/unself.h"
#include "cellRtc.h"
#include "sceNp.h"

logs::channel sceNp("sceNp");

s32 g_psn_connection_status = SCE_NP_MANAGER_STATUS_OFFLINE;

s32 sceNpInit(u32 poolsize, vm::ptr<void> poolptr)
{
	sceNp.warning("sceNpInit(poolsize=0x%x, poolptr=*0x%x)", poolsize, poolptr);

	if (poolsize == 0)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}
	else if (poolsize < 128 * 1024)
	{
		return SCE_NP_ERROR_INSUFFICIENT_BUFFER;
	}

	if (!poolptr)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

s32 sceNpTerm()
{
	sceNp.warning("sceNpTerm()");

	return CELL_OK;
}

s32 npDrmIsAvailable(vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	const std::string& enc_drm_path = drm_path.get_ptr();

	if (!fs::is_file(vfs::get(enc_drm_path)))
	{
		sceNp.warning("npDrmIsAvailable(): '%s' not found", enc_drm_path);
		return CELL_ENOENT;
	}

	std::string k_licensee_str = "";
	std::array<u8, 0x10> k_licensee{0};

	if (k_licensee_addr)
	{
		for (s8 i = 0; i < 0x10; i++)
		{
			k_licensee[i] = *(k_licensee_addr + i);
			k_licensee_str += fmt::format("%02x", k_licensee[i]);
		}

		sceNp.notice("npDrmIsAvailable(): KLicense key %s", k_licensee_str);
	}

	auto npdrmkeys = fxm::get_always<LoadedNpdrmKeys_t>();

	npdrmkeys->devKlic.fill(0);
	npdrmkeys->rifKey.fill(0);

	// todo: profile for rap_dir_path
	std::string rap_dir_path = "/dev_hdd0/home/00000001/exdata/";

	const std::string& enc_drm_path_local = vfs::get(enc_drm_path);
	const fs::file enc_file(enc_drm_path_local);

	u32 magic;

	enc_file.read<u32>(magic);
	enc_file.seek(0);

	if (magic == "SCE\0"_u32)
	{
		if (k_licensee_addr == vm::null)
			k_licensee = get_default_self_klic();

		if (verify_npdrm_self_headers(enc_file, k_licensee.data()))
		{
			npdrmkeys->devKlic = std::move(k_licensee);
		}
		else
		{
			sceNp.error("npDrmIsAvailable(): Failed to verify sce file %s", enc_drm_path);
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
			npdrmkeys->devKlic = std::move(k_licensee);

			if (fs::is_file(vfs::get(rap_file)))
				npdrmkeys->rifKey = GetEdatRifKeyFromRapFile(fs::file{ vfs::get(rap_file) });
			else
				sceNp.warning("npDrmIsAvailable(): Rap file not found: %s", rap_file.c_str());
		}
		else
		{
			sceNp.error("npDrmIsAvailable(): Failed to verify npd file %s", enc_drm_path);
			return SCE_NP_DRM_ERROR_NO_ENTITLEMENT;
		}
	}
	else
	{
		// for now assume its just unencrypted
		sceNp.notice("npDrmIsAvailable(): Assuming npdrm file is unencrypted at %s", enc_drm_path);
	}
	return CELL_OK;
}

s32 sceNpDrmIsAvailable(vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	sceNp.warning("sceNpDrmIsAvailable(k_licensee=*0x%x, drm_path=%s)", k_licensee_addr, drm_path);

	return npDrmIsAvailable(k_licensee_addr, drm_path);
}

s32 sceNpDrmIsAvailable2(vm::cptr<u8> k_licensee_addr, vm::cptr<char> drm_path)
{
	sceNp.warning("sceNpDrmIsAvailable2(k_licensee=*0x%x, drm_path=%s)", k_licensee_addr, drm_path);

	return npDrmIsAvailable(k_licensee_addr, drm_path);
}

s32 sceNpDrmVerifyUpgradeLicense(vm::cptr<char> content_id)
{
	sceNp.warning("sceNpDrmVerifyUpgradeLicense(content_id=%s)", content_id);

	if (!fs::is_file(vfs::get("/dev_hdd0/home/00000001/exdata/") + content_id.get_ptr() + ".rap"))
	{
		// Game hasn't been purchased therefore no RAP file present
		return SCE_NP_DRM_ERROR_LICENSE_NOT_FOUND;
	}

	// Game has been purchased and there's a RAP file present
	return CELL_OK;
}

s32 sceNpDrmVerifyUpgradeLicense2(vm::cptr<char> content_id)
{
	sceNp.warning("sceNpDrmVerifyUpgradeLicense2(content_id=%s)", content_id);

	if (!fs::is_file(vfs::get("/dev_hdd0/home/00000001/exdata/") + content_id.get_ptr() + ".rap"))
	{
		// Game hasn't been purchased therefore no RAP file present
		return SCE_NP_DRM_ERROR_LICENSE_NOT_FOUND;
	}

	// Game has been purchased and there's a RAP file present
	return CELL_OK;
}

s32 sceNpDrmExecuteGamePurchase()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpDrmGetTimelimit(vm::cptr<char> path, vm::ptr<u64> time_remain)
{
	sceNp.warning("sceNpDrmGetTimelimit(path=%s, time_remain=*0x%x)", path, time_remain);

	*time_remain = 0x7FFFFFFFFFFFFFFFULL;

	return CELL_OK;
}

s32 sceNpDrmProcessExitSpawn(vm::cptr<u8> klicensee, vm::cptr<char> path, u32 argv_addr, u32 envp_addr, u32 data_addr, u32 data_size, u32 prio, u64 flags)
{
	sceNp.warning("sceNpDrmProcessExitSpawn() -> sys_game_process_exitspawn");

	sceNp.warning("klicensee: 0x%x", klicensee);
	npDrmIsAvailable(klicensee, path);

	sys_game_process_exitspawn(path, argv_addr, envp_addr, data_addr, data_size, prio, flags);

	return CELL_OK;
}

s32 sceNpDrmProcessExitSpawn2(vm::cptr<u8> klicensee, vm::cptr<char> path, u32 argv_addr, u32 envp_addr, u32 data_addr, u32 data_size, u32 prio, u64 flags)
{
	sceNp.warning("sceNpDrmProcessExitSpawn2() -> sys_game_process_exitspawn2");
	
	sceNp.warning("klicensee: 0x%x", klicensee);
	npDrmIsAvailable(klicensee, path);

	sys_game_process_exitspawn2(path, argv_addr, envp_addr, data_addr, data_size, prio, flags);

	return CELL_OK;
}

s32 sceNpBasicRegisterHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicRegisterContextSensitiveHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicUnregisterHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicSetPresence()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicSetPresenceDetails()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicSetPresenceDetails2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicSendMessage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicSendMessageGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicSendMessageAttachment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicRecvMessageAttachment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicRecvMessageAttachmentLoad()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicRecvMessageCustom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicMarkMessageAsUsed()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicAddFriend()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicGetFriendListEntryCount(vm::ptr<u32> count)
{
	sceNp.warning("sceNpBasicGetFriendListEntryCount(count=*0x%x)", count);

	// TODO: Check if there are any friends
	*count = 0;

	return CELL_OK;
}

s32 sceNpBasicGetFriendListEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicGetFriendPresenceByIndex()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicGetFriendPresenceByIndex2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicGetFriendPresenceByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicGetFriendPresenceByNpId2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicAddPlayersHistory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicAddPlayersHistoryAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpBasicGetPlayersHistoryEntryCount(u32 options, vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetPlayersHistoryEntryCount(options=%d, count=*0x%x)", options, count);

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Check if there are players histories
	*count = 0;

	return CELL_OK;
}

s32 sceNpBasicGetPlayersHistoryEntry(u32 options, u32 index, vm::ptr<SceNpId> npid)
{
	sceNp.todo("sceNpBasicGetPlayersHistoryEntry(options=%d, index=%d, npid=*0x%x)", options, index, npid);

	return CELL_OK;
}

s32 sceNpBasicAddBlockListEntry(vm::cptr<SceNpId> npid)
{
	sceNp.todo("sceNpBasicAddBlockListEntry(npid=*0x%x)", npid);

	return CELL_OK;
}

s32 sceNpBasicGetBlockListEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetBlockListEntryCount(count=*0x%x)", count);

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Check if there are block lists
	*count = 0;

	return CELL_OK;
}

s32 sceNpBasicGetBlockListEntry(u32 index, vm::ptr<SceNpId> npid)
{
	sceNp.todo("sceNpBasicGetBlockListEntry(index=%d, npid=*0x%x)", index, npid);

	return CELL_OK;
}

s32 sceNpBasicGetMessageAttachmentEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetMessageAttachmentEntryCount(count=*0x%x)", count);

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Check if there are message attachments
	*count = 0;

	return CELL_OK;
}

s32 sceNpBasicGetMessageAttachmentEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetMessageAttachmentEntry(index=%d, from=*0x%x)", index, from);

	return CELL_OK;
}

s32 sceNpBasicGetCustomInvitationEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetCustomInvitationEntryCount(count=*0x%x)", count);

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Check if there are custom invitations
	*count = 0;

	return CELL_OK;
}

s32 sceNpBasicGetCustomInvitationEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetCustomInvitationEntry(index=%d, from=*0x%x)", index, from);

	return CELL_OK;
}

s32 sceNpBasicGetMatchingInvitationEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetMatchingInvitationEntryCount(count=*0x%x)", count);

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Check if there are matching invitations
	*count = 0;

	return CELL_OK;
}

s32 sceNpBasicGetMatchingInvitationEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetMatchingInvitationEntry(index=%d, from=*0x%x)", index, from);

	return CELL_OK;
}

s32 sceNpBasicGetClanMessageEntryCount(vm::ptr<u32> count)
{
	sceNp.todo("sceNpBasicGetClanMessageEntryCount(count=*0x%x)", count);

	if (!count)
	{
		return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Check if there are clan messages
	*count = 0;

	return CELL_OK;
}

s32 sceNpBasicGetClanMessageEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetClanMessageEntry(index=%d, from=*0x%x)", index, from);

	return CELL_OK;
}

s32 sceNpBasicGetMessageEntryCount(u32 type, vm::ptr<u32> count)
{
	sceNp.warning("sceNpBasicGetMessageEntryCount(type=%d, count=*0x%x)", type, count);

	// TODO: Check if there are messages
	*count = 0;

	return CELL_OK;
}

s32 sceNpBasicGetMessageEntry(u32 type, u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.todo("sceNpBasicGetMessageEntry(type=%d, index=%d, from=*0x%x)", type, index, from);

	return CELL_OK;
}

s32 sceNpBasicGetEvent(vm::ptr<s32> event, vm::ptr<SceNpUserInfo> from, vm::ptr<s32> data, vm::ptr<u32> size)
{
	sceNp.warning("sceNpBasicGetEvent(event=*0x%x, from=*0x%x, data=*0x%x, size=*0x%x)", event, from, data, size);

	// TODO: Check for other error and pass other events
	*event = SCE_NP_BASIC_EVENT_OFFLINE;

	return CELL_OK;
}

s32 sceNpCommerceCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceInitProductCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceDestroyProductCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetProductCategoryStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetProductCategoryFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetProductCategoryResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetProductCategoryAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetProductId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetProductName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetCategoryDescription()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetCategoryId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetCategoryImageURL()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetCategoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetCategoryName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetCurrencyCode()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetCurrencyDecimals()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetCurrencyInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetNumOfChildCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetNumOfChildProductSku()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetSkuDescription()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetSkuId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetSkuImageURL()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetSkuName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetSkuPrice()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetSkuUserData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceSetDataFlagStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetDataFlagStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceSetDataFlagFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetDataFlagFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetDataFlagState()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetDataFlagAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetChildCategoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceGetChildProductSkuInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceDoCheckoutStartAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCommerceDoCheckoutFinishAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCustomMenuRegisterActions()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCustomMenuActionSetActivation()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpCustomMenuRegisterExceptionList()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpFriendlist()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpFriendlistCustom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpFriendlistAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupInit()
{
	sceNp.warning("sceNpLookupInit()");

	return CELL_OK;
}

s32 sceNpLookupTerm()
{
	sceNp.warning("sceNpLookupTerm()");

	return CELL_OK;
}

s32 sceNpLookupCreateTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupDestroyTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupCreateTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupDestroyTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupSetTimeout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupAbortTransaction()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupWaitAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupPollAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupUserProfile()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupUserProfileAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupUserProfileWithAvatarSize()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupUserProfileWithAvatarSizeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupAvatarImage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupAvatarImageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupTitleStorage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupTitleStorageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);

	return CELL_OK;
}

s32 sceNpLookupTitleSmallStorage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpLookupTitleSmallStorageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerRegisterCallback(vm::ptr<SceNpManagerCallback> callback, vm::ptr<void> arg)
{
	sceNp.todo("sceNpManagerRegisterCallback(callback=*0x%x, arg=*0x%x)", callback, arg);

	if (!callback)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

s32 sceNpManagerUnregisterCallback()
{
	sceNp.todo("sceNpManagerUnregisterCallback()");

	return CELL_OK;
}

s32 sceNpManagerGetStatus(vm::ptr<s32> status)
{
	sceNp.warning("sceNpManagerGetStatus(status=*0x%x)", status);

	if (!status)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	*status = g_psn_connection_status;

	return CELL_OK;
}

s32 sceNpManagerGetNetworkTime(vm::ptr<CellRtcTick> pTick)
{
	sceNp.todo("sceNpManagerGetNetworkTime(pTick=*0x%x)", pTick);

	if (!pTick)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 sceNpManagerGetOnlineId(vm::ptr<SceNpOnlineId> onlineId)
{
	sceNp.todo("sceNpManagerGetOnlineId(onlineId=*0x%x)", onlineId);

	if (!onlineId)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_LOGGING_IN || g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 sceNpManagerGetNpId(ppu_thread& ppu, vm::ptr<SceNpId> npId)
{
	sceNp.todo("sceNpManagerGetNpId(npId=*0x%x)", npId);

	if (!npId)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_LOGGING_IN || g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 sceNpManagerGetOnlineName(vm::ptr<SceNpOnlineName> onlineName)
{
	sceNp.todo("sceNpManagerGetOnlineName(onlineName=*0x%x)", onlineName);

	if (!onlineName)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 sceNpManagerGetAvatarUrl(vm::ptr<SceNpAvatarUrl> avatarUrl)
{
	sceNp.todo("sceNpManagerGetAvatarUrl(avatarUrl=*0x%x)", avatarUrl);

	if (!avatarUrl)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 sceNpManagerGetMyLanguages(vm::ptr<SceNpMyLanguages> myLanguages)
{
	sceNp.todo("sceNpManagerGetMyLanguages(myLanguages=*0x%x)", myLanguages);

	if (!myLanguages)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 sceNpManagerGetAccountRegion(vm::ptr<SceNpCountryCode> countryCode, vm::ptr<s32> language)
{
	sceNp.todo("sceNpManagerGetAccountRegion(countryCode=*0x%x, language=*0x%x)", countryCode, language);

	if (!countryCode || !language)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_LOGGING_IN || g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 sceNpManagerGetAccountAge(vm::ptr<s32> age)
{
	sceNp.todo("sceNpManagerGetAccountAge(age=*0x%x)", age);

	if (!age)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_LOGGING_IN || g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 sceNpManagerGetContentRatingFlag(vm::ptr<s32> isRestricted, vm::ptr<s32> age)
{
	sceNp.warning("sceNpManagerGetContentRatingFlag(isRestricted=*0x%x, age=*0x%x)", isRestricted, age);

	if (!isRestricted || !age)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_LOGGING_IN || g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	// TODO: read user's parental control information
	*isRestricted = 0;
	*age = 18;

	return CELL_OK;
}

s32 sceNpManagerGetChatRestrictionFlag(vm::ptr<s32> isRestricted)
{
	sceNp.todo("sceNpManagerGetChatRestrictionFlag(isRestricted=*0x%x)", isRestricted);

	if (!isRestricted)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	if (g_psn_connection_status == SCE_NP_MANAGER_STATUS_OFFLINE)
	{
		return SCE_NP_ERROR_OFFLINE;
	}

	if (g_psn_connection_status != SCE_NP_MANAGER_STATUS_LOGGING_IN || g_psn_connection_status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return SCE_NP_ERROR_INVALID_STATE;
	}

	// TODO: read user's parental control information
	*isRestricted = 0;

	return CELL_OK;
}

s32 sceNpManagerGetCachedInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerGetPsHandle()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerRequestTicket()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerRequestTicket2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerGetTicket()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerGetTicketParam()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerGetEntitlementIdList()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerGetEntitlementById()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerSubSignin()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerSubSigninAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpManagerSubSignout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingGetResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingGetResultGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingSetRoomInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingSetRoomInfoNoLimit()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomInfoNoLimit()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingSetRoomSearchFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomSearchFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomMemberListLocal()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomListLimitGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingKickRoomMember()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingKickRoomMemberWithOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingQuickMatchGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingSendInvitationGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingAcceptInvitationGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingCreateRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingJoinRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingLeaveRoom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingSearchJoinRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpMatchingGrantOwnership()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpProfileCallGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpProfileAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreInit()
{
	sceNp.warning("sceNpScoreInit()");

	return CELL_OK;
}

s32 sceNpScoreTerm()
{
	sceNp.warning("sceNpScoreTerm()");

	return CELL_OK;
}

s32 sceNpScoreCreateTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreDestroyTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreCreateTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreDestroyTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreSetTimeout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreSetPlayerCharacterId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreWaitAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScorePollAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetBoardInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetBoardInfoAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreRecordScore()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreRecordScoreAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreRecordGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreRecordGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetRankingByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetRankingByNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreCensorComment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreCensorCommentAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreSanitizeComment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreSanitizeCommentAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetRankingByNpIdPcId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetRankingByNpIdPcIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreAbortTransaction()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansMembersRankingByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansMembersRankingByNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansMembersRankingByNpIdPcId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansMembersRankingByNpIdPcIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClanMemberGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClanMemberGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansRankingByClanId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansRankingByClanIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansMembersRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpScoreGetClansMembersRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingAddExtendedHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingSetCtxOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingGetCtxOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingActivateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingDeactivateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingTerminateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingGetConnectionStatus()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingGetConnectionInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingGetConnectionFromNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingGetConnectionFromPeerAddress()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingGetLocalNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingGetPeerNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingCancelPeerNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpSignalingGetPeerNetInfoResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpUtilCanonicalizeNpIdForPs3()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpUtilCanonicalizeNpIdForPsp()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpUtilCmpNpId(vm::ptr<SceNpId> id1, vm::ptr<SceNpId> id2)
{
	sceNp.warning("sceNpUtilCmpNpId(id1=*0x%x, id2=*0x%x)", id1, id2);

	// TODO: Improve the comparison.
	if (strcmp(id1->handle.data, id2->handle.data) != 0)
	{
		return SCE_NP_UTIL_ERROR_NOT_MATCH;
	}

	return CELL_OK;
}

s32 sceNpUtilCmpNpIdInOrder()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpUtilCmpOnlineId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpUtilGetPlatformType()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 sceNpUtilSetPlatformType()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 _sceNpSysutilClientMalloc()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 _sceNpSysutilClientFree()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

s32 _Z33_sce_np_sysutil_send_empty_packetiPN16sysutil_cxmlutil11FixedMemoryEPKcS3_()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _Z27_sce_np_sysutil_send_packetiRN4cxml8DocumentE()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _Z36_sce_np_sysutil_recv_packet_fixedmemiPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentERNS2_7ElementE()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _Z40_sce_np_sysutil_recv_packet_fixedmem_subiPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentERNS2_7ElementE()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _Z27_sce_np_sysutil_recv_packetiRN4cxml8DocumentERNS_7ElementE()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _Z29_sce_np_sysutil_cxml_set_npidRN4cxml8DocumentERNS_7ElementEPKcPK7SceNpId()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _Z31_sce_np_sysutil_send_packet_subiRN4cxml8DocumentE()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _Z37sce_np_matching_set_matching2_runningb()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _Z32_sce_np_sysutil_cxml_prepare_docPN16sysutil_cxmlutil11FixedMemoryERN4cxml8DocumentEPKcRNS2_7ElementES6_i()
{
	fmt::throw_exception("Unimplemented" HERE);
}


DECLARE(ppu_module_manager::sceNp)("sceNp", []()
{
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
