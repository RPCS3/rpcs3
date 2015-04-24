#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/lv2/sys_process.h"

#include "Emu/FS/VFS.h"
#include "Utilities/File.h"
#include "Emu/FS/vfsDir.h"
#include "Crypto/unedat.h"
#include "sceNp.h"

extern Module sceNp;

struct sceNpInternal
{
	bool m_bSceNpInitialized;
	bool m_bSceNp2Initialized;
	bool m_bScoreInitialized;
	bool m_bLookupInitialized;
	bool m_bSceNpUtilBandwidthTestInitialized;

	sceNpInternal()
		: m_bSceNpInitialized(false),
		  m_bSceNp2Initialized(false),
		  m_bScoreInitialized(false),
		  m_bLookupInitialized(false),
		  m_bSceNpUtilBandwidthTestInitialized(false)
	{
	}
};

sceNpInternal sceNpInstance;

int sceNpInit(u32 mem_size, u32 mem_addr)
{
	sceNp.Warning("sceNpInit(mem_size=0x%x, mem_addr=0x%x)", mem_size, mem_addr);

	if (sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_ERROR_ALREADY_INITIALIZED;

	sceNpInstance.m_bSceNpInitialized = true;

	return CELL_OK;
}

int sceNp2Init(u32 mem_size, u32 mem_addr)
{
	sceNp.Warning("sceNp2Init(mem_size=0x%x, mem_addr=0x%x)", mem_size, mem_addr);

	if (sceNpInstance.m_bSceNp2Initialized)
		return SCE_NP_ERROR_ALREADY_INITIALIZED;

	sceNpInstance.m_bSceNp2Initialized = true;

	return CELL_OK;
}

int sceNpTerm()
{
	sceNp.Warning("sceNpTerm()");

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	sceNpInstance.m_bSceNpInitialized = false;

	return CELL_OK;
}

int sceNp2Term()
{
	sceNp.Warning("sceNp2Term()");

	if (!sceNpInstance.m_bSceNp2Initialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	sceNpInstance.m_bSceNp2Initialized = false;

	return CELL_OK;
}

int npDrmIsAvailable(u32 k_licensee_addr, vm::ptr<const char> drm_path)
{
	if (!Emu.GetVFS().ExistsFile(drm_path.get_ptr()))
	{
		sceNp.Warning("npDrmIsAvailable(): '%s' not found", drm_path.get_ptr());
		return CELL_ENOENT;
	}

	std::string k_licensee_str = "0";
	u8 k_licensee[0x10];

	if (k_licensee_addr)
	{
		for (int i = 0; i < 0x10; i++)
		{
			k_licensee[i] = vm::read8(k_licensee_addr + i);
			k_licensee_str += fmt::Format("%02x", k_licensee[i]);
		}
	}

	sceNp.Warning("npDrmIsAvailable: Found DRM license file at %s", drm_path.get_ptr());
	sceNp.Warning("npDrmIsAvailable: Using k_licensee 0x%s", k_licensee_str.c_str());

	// Set the necessary file paths.
	std::string drm_file_name = fmt::AfterLast(drm_path.get_ptr(), '/');

	// TODO: Make more explicit what this actually does (currently it copies "XXXXXXXX" from drm_path (== "/dev_hdd0/game/XXXXXXXXX/*" assumed)
	std::string titleID(&drm_path[15], 9);

	// TODO: These shouldn't use current dir
	std::string enc_drm_path = drm_path.get_ptr();
	std::string dec_drm_path = "/dev_hdd1/cache/" + drm_file_name;
	std::string pf_str("00000001");  // TODO: Allow multiple profiles. Use default for now.
	std::string rap_path("/dev_hdd0/home/" + pf_str + "/exdata/");

	// Search dev_usb000 for a compatible RAP file. 
	vfsDir raps_dir(rap_path);
	if (!raps_dir.IsOpened())
		sceNp.Warning("npDrmIsAvailable: Can't find RAP file for DRM!");
	else
	{
		for (const DirEntryInfo *entry : raps_dir)
		{
			if (entry->name.find(titleID) != std::string::npos)
			{
				rap_path += entry->name;
				break;
			}
		}
	}

	// Decrypt this EDAT using the supplied k_licensee and matching RAP file.
	std::string enc_drm_path_local, dec_drm_path_local, rap_path_local;
	Emu.GetVFS().GetDevice(enc_drm_path, enc_drm_path_local);
	Emu.GetVFS().GetDevice(dec_drm_path, dec_drm_path_local);
	Emu.GetVFS().GetDevice(rap_path, rap_path_local);

	if (DecryptEDAT(enc_drm_path_local, dec_drm_path_local, 8, rap_path_local, k_licensee, false) >= 0)
	{
		// If decryption succeeds, replace the encrypted file with it.
		fs::remove_file(enc_drm_path_local);
		fs::rename(dec_drm_path_local, enc_drm_path_local);
	}

	return CELL_OK;
}

int sceNpDrmIsAvailable(u32 k_licensee_addr, vm::ptr<const char> drm_path)
{
	sceNp.Warning("sceNpDrmIsAvailable(k_licensee_addr=0x%x, drm_path_addr=0x%x('%s'))", k_licensee_addr, drm_path.addr(), drm_path.get_ptr());

	return npDrmIsAvailable(k_licensee_addr, drm_path);
}

int sceNpDrmIsAvailable2(u32 k_licensee_addr, vm::ptr<const char> drm_path)
{
	sceNp.Warning("sceNpDrmIsAvailable2(k_licensee_addr=0x%x, drm_path_addr=0x%x('%s'))", k_licensee_addr, drm_path.addr(), drm_path.get_ptr());

	return npDrmIsAvailable(k_licensee_addr, drm_path);
}

int sceNpDrmVerifyUpgradeLicense(vm::ptr<const char> content_id)
{
	sceNp.Todo("sceNpDrmVerifyUpgradeLicense(content_id_addr=0x%x)", content_id.addr());

	return CELL_OK;
}

int sceNpDrmVerifyUpgradeLicense2(vm::ptr<const char> content_id)
{
	sceNp.Todo("sceNpDrmVerifyUpgradeLicense2(content_id_addr=0x%x)", content_id.addr());

	return CELL_OK;
}

int sceNpDrmExecuteGamePurchase()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmGetTimelimit(u32 drm_path_addr, vm::ptr<u64> time_remain_usec)
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmProcessExitSpawn(vm::ptr<const char> path, u32 argv_addr, u32 envp_addr, u32 data_addr, u32 data_size, u32 prio, u64 flags)
{
	sceNp.Warning("sceNpDrmProcessExitSpawn()");
	sceNp.Warning("path: %s", path.get_ptr());
	sceNp.Warning("argv: 0x%x", argv_addr);
	sceNp.Warning("envp: 0x%x", envp_addr);
	sceNp.Warning("data: 0x%x", data_addr);
	sceNp.Warning("data_size: 0x%x", data_size);
	sceNp.Warning("prio: %d", prio);
	sceNp.Warning("flags: %d", flags);

	sys_game_process_exitspawn(path, argv_addr, envp_addr, data_addr, data_size, prio, flags);

	return CELL_OK;
}

int sceNpDrmProcessExitSpawn2(vm::ptr<const char> path, u32 argv_addr, u32 envp_addr, u32 data_addr, u32 data_size, u32 prio, u64 flags)
{
	sceNp.Warning("sceNpDrmProcessExitSpawn2()");
	sceNp.Warning("path: %s", path.get_ptr());
	sceNp.Warning("argv: 0x%x", argv_addr);
	sceNp.Warning("envp: 0x%x", envp_addr);
	sceNp.Warning("data: 0x%x", data_addr);
	sceNp.Warning("data_size: 0x%x", data_size);
	sceNp.Warning("prio: %d", prio);
	sceNp.Warning("flags: %d", flags);

	sys_game_process_exitspawn2(path, argv_addr, envp_addr, data_addr, data_size, prio, flags);

	return CELL_OK;
}

int sceNpBasicRegisterHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRegisterContextSensitiveHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicUnregisterHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSetPresence()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSetPresenceDetails()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSetPresenceDetails2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSendMessage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSendMessageGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSendMessageAttachment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRecvMessageAttachment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRecvMessageAttachmentLoad()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRecvMessageCustom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicMarkMessageAsUsed()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAddFriend()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendListEntryCount(vm::ptr<u32> count)
{
	sceNp.Warning("sceNpBasicGetFriendListEntryCount(count_addr=0x%x)", count.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	// TODO: Check if there are any friends
	*count = 0;

	return CELL_OK;
}

int sceNpBasicGetFriendListEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendPresenceByIndex()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendPresenceByIndex2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendPresenceByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendPresenceByNpId2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAddPlayersHistory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAddPlayersHistoryAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetPlayersHistoryEntryCount(u32 options, vm::ptr<u32> count)
{
	sceNp.Todo("sceNpBasicGetPlayersHistoryEntryCount(options=%d, count_addr=0x%x)", options, count.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetPlayersHistoryEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAddBlockListEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetBlockListEntryCount(u32 count)
{
	sceNp.Todo("sceNpBasicGetBlockListEntryCount(count=%d)", count);

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetBlockListEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetMessageAttachmentEntryCount(vm::ptr<u32> count)
{
	sceNp.Todo("sceNpBasicGetMessageAttachmentEntryCount(count_addr=0x%x)", count.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetMessageAttachmentEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.Todo("sceNpBasicGetMessageAttachmentEntry(index=%d, from_addr=0x%x)", index, from.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetCustomInvitationEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetCustomInvitationEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetMatchingInvitationEntryCount(vm::ptr<u32> count)
{
	sceNp.Todo("sceNpBasicGetMatchingInvitationEntryCount(count_addr=0x%x)", count.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetMatchingInvitationEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.Todo("sceNpBasicGetMatchingInvitationEntry(index=%d, from_addr=0x%x)", index, from.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetClanMessageEntryCount(vm::ptr<u32> count)
{
	sceNp.Todo("sceNpBasicGetClanMessageEntryCount(count_addr=0x%x)", count.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetClanMessageEntry(u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.Todo("sceNpBasicGetClanMessageEntry(index=%d, from_addr=0x%x)", index, from.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetMessageEntryCount(u32 type, vm::ptr<u32> count)
{
	sceNp.Warning("sceNpBasicGetMessageEntryCount(type=%d, count_addr=0x%x)", type, count.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	// TODO: Check if there are messages
	*count = 0;

	return CELL_OK;
}

int sceNpBasicGetMessageEntry(u32 type, u32 index, vm::ptr<SceNpUserInfo> from)
{
	sceNp.Todo("sceNpBasicGetMessageEntry(type=%d, index=%d, from_addr=0x%x)", type, index, from.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpBasicGetEvent(vm::ptr<s32> event, vm::ptr<SceNpUserInfo> from, vm::ptr<s32> data, vm::ptr<u32> size)
{
	sceNp.Warning("sceNpBasicGetEvent(event_addr=0x%x, from_addr=0x%x, data_addr=0x%x, size_addr=0x%x)", event.addr(), from.addr(), data.addr(), size.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_BASIC_ERROR_NOT_INITIALIZED;

	// TODO: Check for other error and pass other events
	*event = SCE_NP_BASIC_EVENT_OFFLINE;

	return CELL_OK;
}

int sceNpCommerceCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceInitProductCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceDestroyProductCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductCategoryStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductCategoryFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductCategoryResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductCategoryAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryDescription()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryImageURL()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCurrencyCode()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCurrencyDecimals()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCurrencyInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetNumOfChildCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetNumOfChildProductSku()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuDescription()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuImageURL()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuPrice()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuUserData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceSetDataFlagStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetDataFlagStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceSetDataFlagFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetDataFlagFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetDataFlagState()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetDataFlagAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetChildCategoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetChildProductSkuInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceDoCheckoutStartAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceDoCheckoutFinishAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCustomMenuRegisterActions()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCustomMenuActionSetActivation()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCustomMenuRegisterExceptionList()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpFriendlist()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpFriendlistCustom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpFriendlistAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupInit()
{
	sceNp.Warning("sceNpLookupInit()");

	// TODO: Make sure the error code returned is right,
	//       since there are no error codes for Lookup utility.
	if (sceNpInstance.m_bLookupInitialized)
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;

	sceNpInstance.m_bLookupInitialized = true;

	return CELL_OK;
}

int sceNpLookupTerm()
{
	sceNp.Warning("sceNpLookupTerm()");

	if (!sceNpInstance.m_bLookupInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	sceNpInstance.m_bLookupInitialized = false;

	return CELL_OK;
}

int sceNpLookupCreateTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupDestroyTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupCreateTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupDestroyTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupSetTimeout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupAbortTransaction()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupWaitAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupPollAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupUserProfile()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupUserProfileAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupUserProfileWithAvatarSize()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupUserProfileWithAvatarSizeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupAvatarImage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupAvatarImageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupTitleStorage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupTitleStorageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);

	return CELL_OK;
}

int sceNpLookupTitleSmallStorage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupTitleSmallStorageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerRegisterCallback()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerUnregisterCallback()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetStatus(vm::ptr<u32> status)
{
	sceNp.Log("sceNpManagerGetStatus(status_addr=0x%x)", status.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	// TODO: Support different statuses
	*status = SCE_NP_MANAGER_STATUS_OFFLINE;

	return CELL_OK;
}

int sceNpManagerGetNetworkTime()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetOnlineId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetOnlineName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetAvatarUrl()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetMyLanguages()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetAccountRegion()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetAccountAge()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetContentRatingFlag(vm::ptr<u32> isRestricted, vm::ptr<u32> age)
{
	sceNp.Warning("sceNpManagerGetContentRatingFlag(isRestricted_addr=0x%x, age_addr=0x%x)", isRestricted.addr(), age.addr());

	if (!sceNpInstance.m_bSceNpInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	// TODO: read user's parental control information
	*isRestricted = 0;
	*age = 18;

	return CELL_OK;
}

int sceNpManagerGetChatRestrictionFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetCachedInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetPsHandle()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerRequestTicket()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerRequestTicket2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetTicket()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetTicketParam()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetEntitlementIdList()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetEntitlementById()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerSubSignin()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerSubSigninAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerSubSignout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetResultGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSetRoomInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSetRoomInfoNoLimit()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomInfoNoLimit()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSetRoomSearchFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomSearchFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomMemberListLocal()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomListLimitGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingKickRoomMember()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingKickRoomMemberWithOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingQuickMatchGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSendInvitationGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingAcceptInvitationGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingCreateRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingJoinRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingLeaveRoom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSearchJoinRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGrantOwnership()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpProfileCallGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpProfileAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreInit()
{
	sceNp.Warning("sceNpScoreInit()");

	if (sceNpInstance.m_bScoreInitialized)
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;

	sceNpInstance.m_bScoreInitialized = true;

	return CELL_OK;
}

int sceNpScoreTerm()
{
	sceNp.Warning("sceNpScoreTerm()");

	if (!sceNpInstance.m_bScoreInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	sceNpInstance.m_bScoreInitialized = false;

	return CELL_OK;
}

int sceNpScoreCreateTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreDestroyTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreCreateTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreDestroyTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreSetTimeout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreSetPlayerCharacterId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreWaitAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScorePollAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetBoardInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetBoardInfoAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreRecordScore()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreRecordScoreAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreRecordGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreRecordGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreCensorComment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreCensorCommentAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreSanitizeComment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreSanitizeCommentAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByNpIdPcId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByNpIdPcIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreAbortTransaction()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByNpIdPcId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByNpIdPcIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClanMemberGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClanMemberGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansRankingByClanId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansRankingByClanIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingAddExtendedHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingSetCtxOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetCtxOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingActivateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingDeactivateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingTerminateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetConnectionStatus()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetConnectionInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetConnectionFromNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetConnectionFromPeerAddress()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetLocalNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetPeerNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingCancelPeerNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetPeerNetInfoResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilCmpNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilCmpNpIdInOrder()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilBandwidthTestInitStart(u32 prio, size_t stack)
{
	UNIMPLEMENTED_FUNC(sceNp);

	if (sceNpInstance.m_bSceNpUtilBandwidthTestInitialized)
		return SCE_NP_ERROR_ALREADY_INITIALIZED;

	sceNpInstance.m_bSceNpUtilBandwidthTestInitialized = true;

	return CELL_OK;
}

int sceNpUtilBandwidthTestGetStatus()
{
	UNIMPLEMENTED_FUNC(sceNp);

	if (!sceNpInstance.m_bSceNpUtilBandwidthTestInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpUtilBandwidthTestShutdown()
{
	UNIMPLEMENTED_FUNC(sceNp);

	if (!sceNpInstance.m_bSceNpUtilBandwidthTestInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	sceNpInstance.m_bSceNpUtilBandwidthTestInitialized = false;

	return CELL_OK;
}

int sceNpUtilBandwidthTestAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);

	if (!sceNpInstance.m_bSceNpUtilBandwidthTestInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int	_sceNpSysutilClientMalloc()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int	_sceNpSysutilClientFree()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

Module sceNp("sceNp", []()
{
	sceNpInstance.m_bSceNpInitialized = false;
	sceNpInstance.m_bSceNp2Initialized = false;
	sceNpInstance.m_bScoreInitialized = false;
	sceNpInstance.m_bLookupInitialized = false;
	sceNpInstance.m_bSceNpUtilBandwidthTestInitialized = false;

	REG_FUNC(sceNp, sceNpInit);
	REG_FUNC(sceNp, sceNp2Init);
	REG_FUNC(sceNp, sceNpUtilBandwidthTestInitStart);
	REG_FUNC(sceNp, sceNpTerm);
	REG_FUNC(sceNp, sceNp2Term);
	REG_FUNC(sceNp, sceNpUtilBandwidthTestShutdown);
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
	REG_FUNC(sceNp, sceNpUtilCmpNpId);
	REG_FUNC(sceNp, sceNpUtilCmpNpIdInOrder);
	REG_FUNC(sceNp, sceNpUtilBandwidthTestGetStatus);
	REG_FUNC(sceNp, sceNpUtilBandwidthTestAbort);
	REG_FUNC(sceNp, _sceNpSysutilClientMalloc);
	REG_FUNC(sceNp, _sceNpSysutilClientFree);
});
