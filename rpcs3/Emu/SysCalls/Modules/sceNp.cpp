#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/FS/vfsDir.h"
#include "Crypto/unedat.h"
#include "sceNp.h"

//void sceNp_init();
//Module sceNp(0x0016, sceNp_init);
Module *sceNp = nullptr;

int sceNpInit(u32 mem_size, u32 mem_addr)
{
	sceNp->Log("sceNpInit(mem_size=0x%x, mem_addr=0x%x)", mem_size, mem_addr);
	return CELL_OK;
}

int sceNpTerm()
{
	sceNp->Log("sceNpTerm");
	return CELL_OK;
}

int sceNpDrmIsAvailable(u32 k_licensee_addr, u32 drm_path_addr)
{
	sceNp->Warning("sceNpDrmIsAvailable(k_licensee_addr=0x%x, drm_path_addr=0x%x)", k_licensee_addr, drm_path_addr);

	std::string drm_path = Memory.ReadString(drm_path_addr);
	if (!Emu.GetVFS().ExistsFile(drm_path))
	{
		sceNp->Warning("sceNpDrmIsAvailable(): '%s' not found", drm_path.c_str());
		return CELL_ENOENT;
	}

	std::string k_licensee_str = "0";
	u8 k_licensee[0x10];

	if (k_licensee_addr)
	{
		for (int i = 0; i < 0x10; i++)
		{
			k_licensee[i] = Memory.Read8(k_licensee_addr + i);
			k_licensee_str += fmt::Format("%02x", k_licensee[i]);
		}
	}

	sceNp->Warning("sceNpDrmIsAvailable: Found DRM license file at %s", drm_path.c_str());
	sceNp->Warning("sceNpDrmIsAvailable: Using k_licensee 0x%s", k_licensee_str.c_str());

	// Set the necessary file paths.
	std::string drm_file_name = fmt::AfterLast(drm_path,'/');

	// TODO: Make more explicit what this actually does (currently it copies "XXXXXXXX" from drm_path (== "/dev_hdd0/game/XXXXXXXXX/*" assumed)
	std::string titleID = drm_path.substr(15, 9);

	// TODO: These shouldn't use current dir
	std::string enc_drm_path = drm_path;
	std::string dec_drm_path = "/dev_hdd1/" + titleID + "/" + drm_file_name;
	std::string rap_path = "/dev_usb000/";
	
	// Search dev_usb000 for a compatible RAP file. 
	vfsDir *raps_dir = new vfsDir(rap_path);
	if (!raps_dir->IsOpened())
		sceNp->Warning("sceNpDrmIsAvailable: Can't find RAP file for DRM!");
	else
	{
		const std::vector<DirEntryInfo> &entries = raps_dir->GetEntries();
		for (auto &entry:  entries)
		{
			if (entry.name.find(titleID) != std::string::npos )
			{
				rap_path += entry.name;
				break;
			}
		}
	}

	// Create a new directory under dev_hdd1/titleID to hold the decrypted data.
	// TODO: These shouldn't use current dir
	std::string tmp_dir = "./dev_hdd1/" + titleID;
	if (!rExists(tmp_dir))
		rMkdir("./dev_hdd1/" + titleID);

	// Decrypt this EDAT using the supplied k_licensee and matching RAP file.
	std::string enc_drm_path_local, dec_drm_path_local, rap_path_local;
	Emu.GetVFS().GetDevice(enc_drm_path, enc_drm_path_local);
	Emu.GetVFS().GetDevice(dec_drm_path, dec_drm_path_local);
	Emu.GetVFS().GetDevice(rap_path, rap_path_local);

	DecryptEDAT(enc_drm_path_local, dec_drm_path_local, 8, rap_path_local, k_licensee, false);
	return CELL_OK;
}

int sceNpDrmIsAvailable2(u32 k_licensee_addr, u32 drm_path_addr)
{
	sceNp->Warning("sceNpDrmIsAvailable2(k_licensee_addr=0x%x, drm_path_addr=0x%x)", k_licensee_addr, drm_path_addr);

	std::string drm_path = Memory.ReadString(drm_path_addr);
	if (!Emu.GetVFS().ExistsFile(drm_path))
	{
		sceNp->Warning("sceNpDrmIsAvailable2(): '%s' not found", drm_path.c_str());
		return CELL_ENOENT;
	}

	std::string k_licensee_str = "0";
	u8 k_licensee[0x10];

	if (k_licensee_addr)
	{
		for (int i = 0; i < 0x10; i++)
		{
			k_licensee[i] = Memory.Read8(k_licensee_addr + i);
			k_licensee_str += fmt::Format("%02x", k_licensee[i]);
		}
	}

	sceNp->Warning("sceNpDrmIsAvailable2: Found DRM license file at %s", drm_path.c_str());
	sceNp->Warning("sceNpDrmIsAvailable2: Using k_licensee 0x%s", k_licensee_str.c_str());

	// Set the necessary file paths.
	std::string drm_file_name = fmt::AfterLast(drm_path, '/');

	// TODO: Make more explicit what this actually does (currently it copies "XXXXXXXX" from drm_path (== "/dev_hdd0/game/XXXXXXXXX/*" assumed)
	std::string titleID = drm_path.substr(15, 9);

	// TODO: These shouldn't use current dir
	std::string enc_drm_path = drm_path;
	std::string dec_drm_path = "/dev_hdd1/" + titleID + "/" + drm_file_name;
	std::string rap_path = "/dev_usb000/";

	// Search dev_usb000 for a compatible RAP file. 
	vfsDir *raps_dir = new vfsDir(rap_path);
	if (!raps_dir->IsOpened())
		sceNp->Warning("sceNpDrmIsAvailable2: Can't find RAP file for DRM!");
	else
	{
		const std::vector<DirEntryInfo> &entries = raps_dir->GetEntries();
		for (auto &entry : entries)
		{
			if (entry.name.find(titleID) != std::string::npos)
			{
				rap_path += entry.name;
				break;
			}
		}
	}

	// Create a new directory under dev_hdd1/titleID to hold the decrypted data.
	// TODO: These shouldn't use current dir
	std::string tmp_dir = "./dev_hdd1/" + titleID;
	if (!rExists(tmp_dir))
		rMkdir("./dev_hdd1/" + titleID);

	// Decrypt this EDAT using the supplied k_licensee and matching RAP file.
	std::string enc_drm_path_local, dec_drm_path_local, rap_path_local;
	Emu.GetVFS().GetDevice(enc_drm_path, enc_drm_path_local);
	Emu.GetVFS().GetDevice(dec_drm_path, dec_drm_path_local);
	Emu.GetVFS().GetDevice(rap_path, rap_path_local);

	DecryptEDAT(enc_drm_path_local, dec_drm_path_local, 8, rap_path_local, k_licensee, false);
	return CELL_OK;
}

int sceNpDrmVerifyUpgradeLicense(u32 content_id_addr)
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmExecuteGamePurchase()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmGetTimelimit(u32 drm_path_addr, mem64_t time_remain_usec)
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetStatus(mem32_t status)
{
	sceNp->Log("sceNpManagerGetStatus(status_addr=0x%x)", status.GetAddr());

	// TODO: Check if sceNpInit() was called, if not return SCE_NP_ERROR_NOT_INITIALIZED

	// TODO: Support different statuses
	status = SCE_NP_MANAGER_STATUS_OFFLINE;
	return CELL_OK;
}

int sceNpManagerSubSignout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetChildProductSkuInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSendMessageGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendListEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreRecordGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetDataFlagAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetMatchingInvitationEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetTicket()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingQuickMatchGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetConnectionInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreRecordScore()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAddPlayersHistory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetAccountAge()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetPsHandle()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreWaitAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuUserData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAddBlockListEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupUserProfileWithAvatarSizeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansRankingByClanIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreDestroyTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendPresenceByNpId2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetCtxOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetResultGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductCategoryStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreSetPlayerCharacterId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingSetCtxOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAddFriend()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreSetTimeout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansRankingByClanId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingAcceptInvitationGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetNumOfChildCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreSanitizeCommentAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpProfileAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupUserProfileWithAvatarSize()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetMessageEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetMyLanguages()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendPresenceByIndex()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreInit()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSearchJoinRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingKickRoomMember()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetConnectionFromPeerAddress()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryDescription()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetAvatarUrl()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingCreateRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupAbortTransaction()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSetPresence()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductCategoryResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRegisterContextSensitiveHandler()
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

int sceNpBasicSendMessageAttachment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerSubSignin()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCustomMenuRegisterActions()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingJoinRoomGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSetRoomInfoNoLimit()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetCachedInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetClanMessageEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingAddExtendedHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerUnregisterCallback()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetTicketParam()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetMessageAttachmentEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupDestroyTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupTitleStorageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	
	return CELL_OK;
}

int sceNpBasicSetPresenceDetails2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupInit()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerSubSigninAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingActivateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendPresenceByIndex2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRecvMessageAttachmentLoad()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingCancelPeerNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductCategoryAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceDestroyProductCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetContentRatingFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreCreateTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSetRoomInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupSetTimeout()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetNumOfChildProductSku()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetBlockListEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomMemberListLocal()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClanMemberGameData()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupPollAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetPeerNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuPrice()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCurrencyCode()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansRankingByRangeAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreCensorCommentAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreCensorComment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerRequestTicket()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRecvMessageCustom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int	_sceNpSysutilClientFree()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerRequestTicket2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupTerm()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupTitleSmallStorageAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSendInvitationGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceInitProductCategory()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceSetDataFlagFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetMessageAttachmentEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetDataFlagFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryImageURL()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCustomMenuRegisterExceptionList()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingTerminateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreTerm()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceSetDataFlagStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetLocalNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupTitleStorage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetPlayersHistoryEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetEntitlementById()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingSetRoomSearchFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScorePollAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuDescription()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetCustomInvitationEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmProcessExitSpawn()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicUnregisterHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceDoCheckoutFinishAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetMatchingInvitationEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCurrencyDecimals()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendListEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomInfoNoLimit()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCurrencyInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetAccountRegion()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRecvMessageAttachment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupAvatarImage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetEntitlementIdList()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreCreateTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetChildCategoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetPlayersHistoryEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicRegisterHandler()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicAddPlayersHistoryAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
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

int sceNpDrmVerifyUpgradeLicense2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSetPresenceDetails()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClanMemberGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetClanMessageEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupAvatarImageAsync()
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

int sceNpScoreDestroyTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetConnectionStatus()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupTitleSmallStorage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuImageURL()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupCreateTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpProfileCallGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductCategoryFinish()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetDataFlagState()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetCustomInvitationEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetPeerNetInfoResult()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilCmpNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingKickRoomMemberWithOpt()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupWaitAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpFriendlistCustom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGrantOwnership()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}
int sceNpCommerceGetDataFlagStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetBoardInfoAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupUserProfile()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetEvent()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicMarkMessageAsUsed()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomListLimitGUI()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceDoCheckoutStartAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmProcessExitSpawn2()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerRegisterCallback()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingGetConnectionFromNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetClansMembersRankingByNpIdAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupCreateTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetProductName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetChatRestrictionFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetCategoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicSendMessage()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetMessageEntryCount()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int	_sceNpSysutilClientMalloc()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceGetSkuName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreAbortTransaction()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingGetRoomSearchFlag()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpFriendlist()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreRecordScoreAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreSanitizeComment()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}
int sceNpBasicGetBlockListEntry()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetOnlineName()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetBoardInfo()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpFriendlistAbortGui()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilCmpNpIdInOrder()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreRecordGameDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpMatchingLeaveRoom()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCustomMenuActionSetActivation()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupDestroyTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpScoreGetRankingByRange()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpCommerceCreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingDeactivateConnection()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpBasicGetFriendPresenceByNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetNpId()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpLookupUserProfileAsync()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilBandwidthTestShutdown()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilBandwidthTestInitStart()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilBandwidthTestGetStatus()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpUtilBandwidthTestAbort()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpSignalingDestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

void sceNp_init()
{
	sceNp->AddFunc(0xbd28fdbf, sceNpInit);
	sceNp->AddFunc(0x4885aa18, sceNpTerm);
	sceNp->AddFunc(0xad218faf, sceNpDrmIsAvailable);
	sceNp->AddFunc(0xf042b14f, sceNpDrmIsAvailable2);
	sceNp->AddFunc(0x2ecd48ed, sceNpDrmVerifyUpgradeLicense);
	sceNp->AddFunc(0xbe0e3ee2, sceNpDrmVerifyUpgradeLicense2);
	sceNp->AddFunc(0xf283c143, sceNpDrmExecuteGamePurchase);
	sceNp->AddFunc(0xcf51864b, sceNpDrmGetTimelimit);
	sceNp->AddFunc(0xa7bff757, sceNpManagerGetStatus);
	sceNp->AddFunc(0x000e53cc, sceNpManagerSubSignout);
	sceNp->AddFunc(0x01cd9cfd, sceNpCommerceGetChildProductSkuInfo);
	sceNp->AddFunc(0x01fbbc9b, sceNpBasicSendMessageGui);
	sceNp->AddFunc(0x03c741a7, sceNpMatchingGetResult);
	sceNp->AddFunc(0x04372385, sceNpBasicGetFriendListEntry);
	sceNp->AddFunc(0x04ca5e6a, sceNpScoreRecordGameData);
	sceNp->AddFunc(0x0561448b, sceNpCommerceGetDataFlagAbort);
	sceNp->AddFunc(0x05af1cb8, sceNpBasicGetMatchingInvitationEntry);
	sceNp->AddFunc(0x05d65dff, sceNpScoreGetRankingByNpId);
	sceNp->AddFunc(0x0968aa36, sceNpManagerGetTicket);
	sceNp->AddFunc(0x14497465, sceNpMatchingQuickMatchGUI);
	sceNp->AddFunc(0x155de760, sceNpSignalingGetConnectionInfo);
	sceNp->AddFunc(0x166dcc11, sceNpLookupNpId);
	sceNp->AddFunc(0x1672170e, sceNpScoreRecordScore);
	sceNp->AddFunc(0x168a3117, sceNpBasicAddPlayersHistory);
	sceNp->AddFunc(0x168fcece, sceNpManagerGetAccountAge);
	sceNp->AddFunc(0x16f88a6f, sceNpManagerGetPsHandle);
	sceNp->AddFunc(0x1a2704f7, sceNpScoreWaitAsync);
	sceNp->AddFunc(0x1a3fcb69, sceNpCommerceGetSkuUserData);
	sceNp->AddFunc(0x1ae8a549, sceNpBasicAddBlockListEntry);
	sceNp->AddFunc(0x1fdb3ec2, sceNpLookupUserProfileWithAvatarSizeAsync);
	sceNp->AddFunc(0x21206642, sceNpScoreGetRankingByRangeAsync);
	sceNp->AddFunc(0x227f8763, sceNpScoreGetClansRankingByClanIdAsync);
	sceNp->AddFunc(0x259113b8, sceNpScoreDestroyTitleCtx);
	sceNp->AddFunc(0x260caedd, sceNpBasicGetFriendPresenceByNpId2);
	sceNp->AddFunc(0x2687a127, sceNpSignalingGetCtxOpt);
	sceNp->AddFunc(0x26b3bc94, sceNpMatchingGetResultGUI);
	sceNp->AddFunc(0x26f33146, sceNpCommerceGetProductCategoryStart);
	sceNp->AddFunc(0x2706eaa1, sceNpScoreSetPlayerCharacterId);
	sceNp->AddFunc(0x276c72b2, sceNpSignalingSetCtxOpt);
	sceNp->AddFunc(0x27c69eba, sceNpBasicAddFriend);
	sceNp->AddFunc(0x29dd45dc, sceNpScoreSetTimeout);
	sceNp->AddFunc(0x2a76895a, sceNpScoreGetClansRankingByClanId);
	sceNp->AddFunc(0x2ad7837d, sceNpMatchingAcceptInvitationGUI);
	sceNp->AddFunc(0x2be41ece, sceNpCommerceGetNumOfChildCategory);
	sceNp->AddFunc(0x2cd2a1af, sceNpScoreSanitizeCommentAsync);
	sceNp->AddFunc(0x2e1c5068, sceNpMatchingDestroyCtx);
	sceNp->AddFunc(0x2f2c6b3e, sceNpProfileAbortGui);
	sceNp->AddFunc(0x2fccbfe0, sceNpLookupUserProfileWithAvatarSize);
	sceNp->AddFunc(0x30d1cbde, sceNpBasicGetMessageEntry);
	sceNp->AddFunc(0x32200389, sceNpManagerGetMyLanguages);
	sceNp->AddFunc(0x32c78a6a, sceNpBasicGetFriendPresenceByIndex);
	sceNp->AddFunc(0x32cf311f, sceNpScoreInit);
	sceNp->AddFunc(0x32febb4c, sceNpMatchingSearchJoinRoomGUI);
	sceNp->AddFunc(0x34cc0ca4, sceNpMatchingKickRoomMember);
	sceNp->AddFunc(0x34ce82a0, sceNpSignalingGetConnectionFromPeerAddress);
	sceNp->AddFunc(0x359642a6, sceNpCommerceGetCategoryDescription);
	sceNp->AddFunc(0x36d0c2c5, sceNpManagerGetAvatarUrl);
	sceNp->AddFunc(0x39a69619, sceNpCommerceGetSkuId);
	sceNp->AddFunc(0x3b02418d, sceNpScoreGetGameData);
	sceNp->AddFunc(0x3cc8588a, sceNpMatchingCreateRoomGUI);
	sceNp->AddFunc(0x3d1760dc, sceNpLookupAbortTransaction);
	sceNp->AddFunc(0x3db7914d, sceNpScoreGetRankingByNpIdAsync);
	sceNp->AddFunc(0x3f0808aa, sceNpBasicSetPresence);
	sceNp->AddFunc(0x3f195b3a, sceNpCommerceGetProductCategoryResult);
	sceNp->AddFunc(0x4026eac5, sceNpBasicRegisterContextSensitiveHandler);
	sceNp->AddFunc(0x41ffd4f2, sceNpScoreGetClansMembersRankingByNpIdPcId);
	sceNp->AddFunc(0x433fcb30, sceNpScoreGetClansMembersRankingByNpIdPcIdAsync);
	sceNp->AddFunc(0x43b989f5, sceNpBasicSendMessageAttachment);
	sceNp->AddFunc(0x442381f7, sceNpManagerSubSignin);
	sceNp->AddFunc(0x45f8f3aa, sceNpCustomMenuRegisterActions);
	sceNp->AddFunc(0x474b7b13, sceNpMatchingJoinRoomGUI);
	sceNp->AddFunc(0x481ce0e8, sceNpBasicAbortGui);
	sceNp->AddFunc(0x4a18a89e, sceNpMatchingSetRoomInfoNoLimit);
	sceNp->AddFunc(0x4b9efb7a, sceNpManagerGetCachedInfo);
	sceNp->AddFunc(0x4d5e0670, sceNpScoreGetClansMembersRankingByRangeAsync);
	sceNp->AddFunc(0x4d9c615d, sceNpBasicGetClanMessageEntry);
	sceNp->AddFunc(0x50b86d94, sceNpSignalingAddExtendedHandler);
	sceNp->AddFunc(0x52a6b523, sceNpManagerUnregisterCallback);
	sceNp->AddFunc(0x58fa4fcd, sceNpManagerGetTicketParam);
	sceNp->AddFunc(0x5d543bbe, sceNpBasicGetMessageAttachmentEntry);
	sceNp->AddFunc(0x5de61626, sceNpLookupDestroyTitleCtx);
	sceNp->AddFunc(0x5e117ed5, sceNpLookupTitleStorageAsync);
	sceNp->AddFunc(0x5e849303, sceNpBasicSetPresenceDetails2);
	sceNp->AddFunc(0x5f2d9257, sceNpLookupInit);
	sceNp->AddFunc(0x60440c73, sceNpManagerSubSigninAbortGui);
	sceNp->AddFunc(0x60897c38, sceNpSignalingActivateConnection);
	sceNp->AddFunc(0x6356082e, sceNpSignalingCreateCtx);
	sceNp->AddFunc(0x6453b27b, sceNpBasicGetFriendPresenceByIndex2);
	sceNp->AddFunc(0x64a704cc, sceNpBasicRecvMessageAttachmentLoad);
	sceNp->AddFunc(0x64dbb89d, sceNpSignalingCancelPeerNetInfo);
	sceNp->AddFunc(0x674bb9ff, sceNpCommerceGetProductCategoryAbort);
	sceNp->AddFunc(0x691f429d, sceNpMatchingGetRoomInfo);
	sceNp->AddFunc(0x6cb81eb2, sceNpCommerceDestroyProductCategory);
	sceNp->AddFunc(0x6d4adc3b, sceNpScoreGetClansMembersRankingByRange);
	sceNp->AddFunc(0x6e2ab18b, sceNpCommerceGetCategoryName);
	sceNp->AddFunc(0x6ee62ed2, sceNpManagerGetContentRatingFlag);
	sceNp->AddFunc(0x6f5e8143, sceNpScoreCreateTransactionCtx);
	sceNp->AddFunc(0x6f8fd267, sceNpMatchingSetRoomInfo);
	sceNp->AddFunc(0x71e5af7e, sceNpLookupSetTimeout);
	sceNp->AddFunc(0x7208dc08, sceNpCommerceGetNumOfChildProductSku);
	sceNp->AddFunc(0x73931bd0, sceNpBasicGetBlockListEntryCount);
	sceNp->AddFunc(0x73a2e36b, sceNpMatchingGetRoomMemberListLocal);
	sceNp->AddFunc(0x741fbf24, sceNpScoreGetClanMemberGameData);
	sceNp->AddFunc(0x7508112e, sceNpLookupPollAsync);
	sceNp->AddFunc(0x75eb50cb, sceNpSignalingGetPeerNetInfo);
	sceNp->AddFunc(0x78d7f9ad, sceNpCommerceGetSkuPrice);
	sceNp->AddFunc(0x79225aa3, sceNpCommerceGetCurrencyCode);
	sceNp->AddFunc(0x7b7e9137, sceNpScoreGetClansRankingByRangeAsync);
	sceNp->AddFunc(0x7be47e61, sceNpScoreCensorCommentAsync);
	sceNp->AddFunc(0x7deb244c, sceNpScoreCensorComment);
	sceNp->AddFunc(0x7e2fef28, sceNpManagerRequestTicket);
	sceNp->AddFunc(0x806960ab, sceNpBasicRecvMessageCustom);
	sceNp->AddFunc(0x816c6a5f, _sceNpSysutilClientFree);
	sceNp->AddFunc(0x8297f1ec, sceNpManagerRequestTicket2);
	sceNp->AddFunc(0x8440537c, sceNpLookupTerm);
	sceNp->AddFunc(0x860b1756, sceNpLookupTitleSmallStorageAsync);
	sceNp->AddFunc(0x8b7bbd73, sceNpMatchingSendInvitationGUI);
	sceNp->AddFunc(0x8d1d096c, sceNpCommerceInitProductCategory);
	sceNp->AddFunc(0x8d4518a0, sceNpCommerceSetDataFlagFinish);
	sceNp->AddFunc(0x9153bdf4, sceNpBasicGetMessageAttachmentEntryCount);
	sceNp->AddFunc(0x9281e87a, sceNpCommerceGetDataFlagFinish);
	sceNp->AddFunc(0x936df4aa, sceNpCommerceGetProductId);
	sceNp->AddFunc(0x9452f4f8, sceNpCommerceGetCategoryImageURL);
	sceNp->AddFunc(0x9458f464, sceNpCustomMenuRegisterExceptionList);
	sceNp->AddFunc(0x95c7bba3, sceNpSignalingTerminateConnection);
	sceNp->AddFunc(0x9851f805, sceNpScoreTerm);
	sceNp->AddFunc(0x99ac9952, sceNpCommerceSetDataFlagStart);
	sceNp->AddFunc(0x9ad7fbd1, sceNpSignalingGetLocalNetInfo);
	sceNp->AddFunc(0x9ee9f97e, sceNpLookupTitleStorage);
	sceNp->AddFunc(0xa15f35fe, sceNpBasicGetPlayersHistoryEntryCount);
	sceNp->AddFunc(0xa1709abd, sceNpManagerGetEntitlementById);
	sceNp->AddFunc(0xa284bd1d, sceNpMatchingSetRoomSearchFlag);
	sceNp->AddFunc(0xa7a090e5, sceNpScorePollAsync);
	sceNp->AddFunc(0xa85a4951, sceNpCommerceGetSkuDescription);
	sceNp->AddFunc(0xa8afa7d4, sceNpBasicGetCustomInvitationEntryCount);
	sceNp->AddFunc(0xaa16695f, sceNpDrmProcessExitSpawn);
	sceNp->AddFunc(0xac66568c, sceNpMatchingCreateCtx);
	sceNp->AddFunc(0xacb9ee8e, sceNpBasicUnregisterHandler);
	sceNp->AddFunc(0xaee8cf71, sceNpCommerceGetCategoryId);
	sceNp->AddFunc(0xaf3eba5a, sceNpCommerceDoCheckoutFinishAsync);
	sceNp->AddFunc(0xaf505def, sceNpBasicGetMatchingInvitationEntryCount);
	sceNp->AddFunc(0xaf57d9c9, sceNpCommerceGetCurrencyDecimals);
	sceNp->AddFunc(0xafef640d, sceNpBasicGetFriendListEntryCount);
	sceNp->AddFunc(0xb020684e, sceNpMatchingGetRoomInfoNoLimit);
	sceNp->AddFunc(0xb082003b, sceNpScoreGetClansRankingByRange);
	sceNp->AddFunc(0xb1c02d66, sceNpCommerceGetCurrencyInfo);
	sceNp->AddFunc(0xb1e0718b, sceNpManagerGetAccountRegion);
	sceNp->AddFunc(0xb5cb2d56, sceNpBasicRecvMessageAttachment);
	sceNp->AddFunc(0xb6017827, sceNpLookupAvatarImage);
	sceNp->AddFunc(0xb66d1c46, sceNpManagerGetEntitlementIdList);
	sceNp->AddFunc(0xb9f93bbb, sceNpScoreCreateTitleCtx);
	sceNp->AddFunc(0xba65de6d, sceNpCommerceGetChildCategoryInfo);
	sceNp->AddFunc(0xbab91fc9, sceNpBasicGetPlayersHistoryEntry);
	sceNp->AddFunc(0xbcc09fe7, sceNpBasicRegisterHandler);
	sceNp->AddFunc(0xbcdbb2ab, sceNpBasicAddPlayersHistoryAsync);
	sceNp->AddFunc(0xbdc07fd5, sceNpManagerGetNetworkTime);
	sceNp->AddFunc(0xbe07c708, sceNpManagerGetOnlineId);
	sceNp->AddFunc(0xbe81c71c, sceNpBasicSetPresenceDetails);
	sceNp->AddFunc(0xbef887e5, sceNpScoreGetClanMemberGameDataAsync);
	sceNp->AddFunc(0xbf607ec6, sceNpBasicGetClanMessageEntryCount);
	sceNp->AddFunc(0xbf9eea93, sceNpLookupAvatarImageAsync);
	sceNp->AddFunc(0xc3a991ee, sceNpScoreGetRankingByNpIdPcId);
	sceNp->AddFunc(0xc4b6cd8f, sceNpScoreGetRankingByNpIdPcIdAsync);
	sceNp->AddFunc(0xc5f4cf82, sceNpScoreDestroyTransactionCtx);
	sceNp->AddFunc(0xca0a2d04, sceNpSignalingGetConnectionStatus);
	sceNp->AddFunc(0xca39c4b2, sceNpLookupTitleSmallStorage);
	sceNp->AddFunc(0xccbe2e69, sceNpCommerceGetSkuImageURL);
	sceNp->AddFunc(0xce81c7f0, sceNpLookupCreateTitleCtx);
	sceNp->AddFunc(0xceeebc7a, sceNpProfileCallGui);
	sceNp->AddFunc(0xcfd469e4, sceNpCommerceGetProductCategoryFinish);
	sceNp->AddFunc(0xd03cea35, sceNpCommerceGetDataFlagState);
	sceNp->AddFunc(0xd053f113, sceNpBasicGetCustomInvitationEntry);
	sceNp->AddFunc(0xd0958814, sceNpSignalingGetPeerNetInfoResult);
	sceNp->AddFunc(0xd12e40ae, sceNpLookupNpIdAsync);
	sceNp->AddFunc(0xd208f91d, sceNpUtilCmpNpId);
	sceNp->AddFunc(0xd20d7798, sceNpMatchingKickRoomMemberWithOpt);
	sceNp->AddFunc(0xd737fd2d, sceNpLookupWaitAsync);
	sceNp->AddFunc(0xd7fb1fa6, sceNpFriendlistCustom);
	sceNp->AddFunc(0xdae2d351, sceNpMatchingGrantOwnership);
	sceNp->AddFunc(0xdb2e4dc2, sceNpScoreGetGameDataAsync);
	sceNp->AddFunc(0xdbdb909f, sceNpCommerceGetDataFlagStart);
	sceNp->AddFunc(0xddce7d15, sceNpScoreGetBoardInfoAsync);
	sceNp->AddFunc(0xded17c26, sceNpScoreGetClansMembersRankingByNpId);
	sceNp->AddFunc(0xdfd63b62, sceNpLookupUserProfile);
	sceNp->AddFunc(0xe035f7d6, sceNpBasicGetEvent);
	sceNp->AddFunc(0xe1c9f675, sceNpBasicMarkMessageAsUsed);
	sceNp->AddFunc(0xe24eea19, sceNpMatchingGetRoomListLimitGUI);
	sceNp->AddFunc(0xe2877bea, sceNpCommerceDestroyCtx);
	sceNp->AddFunc(0xe36c660e, sceNpCommerceDoCheckoutStartAsync);
	sceNp->AddFunc(0xe6c8f3f9, sceNpDrmProcessExitSpawn2);
	sceNp->AddFunc(0xe7dcd3b4, sceNpManagerRegisterCallback);
	sceNp->AddFunc(0xe853d388, sceNpSignalingGetConnectionFromNpId);
	sceNp->AddFunc(0xe8a67160, sceNpScoreGetClansMembersRankingByNpIdAsync);
	sceNp->AddFunc(0xea2e9ffc, sceNpLookupCreateTransactionCtx);
	sceNp->AddFunc(0xeb5f2544, sceNpCommerceGetProductName);
	sceNp->AddFunc(0xeb7a3d84, sceNpManagerGetChatRestrictionFlag);
	sceNp->AddFunc(0xeb9df054, sceNpCommerceGetCategoryInfo);
	sceNp->AddFunc(0xec0a1fbf, sceNpBasicSendMessage);
	sceNp->AddFunc(0xecd503de, sceNpBasicGetMessageEntryCount);
	sceNp->AddFunc(0xee0cc40c, _sceNpSysutilClientMalloc);
	sceNp->AddFunc(0xee530059, sceNpCommerceGetSkuName);
	sceNp->AddFunc(0xee5b20d9, sceNpScoreAbortTransaction);
	sceNp->AddFunc(0xee64cf8e, sceNpMatchingGetRoomSearchFlag);
	sceNp->AddFunc(0xf0a9182b, sceNpFriendlist);
	sceNp->AddFunc(0xf0b1e399, sceNpScoreRecordScoreAsync);
	sceNp->AddFunc(0xf1b77918, sceNpScoreSanitizeComment);
	sceNp->AddFunc(0xf2b3338a, sceNpBasicGetBlockListEntry);
	sceNp->AddFunc(0xf42c0df8, sceNpManagerGetOnlineName);
	sceNp->AddFunc(0xf4e0f607, sceNpScoreGetBoardInfo);
	sceNp->AddFunc(0xf59e1da8, sceNpFriendlistAbortGui);
	sceNp->AddFunc(0xf5ff5f31, sceNpUtilCmpNpIdInOrder);
	sceNp->AddFunc(0xf76847c2, sceNpScoreRecordGameDataAsync);
	sceNp->AddFunc(0xf806c54c, sceNpMatchingLeaveRoom);
	sceNp->AddFunc(0xf9732ac8, sceNpCustomMenuActionSetActivation);
	sceNp->AddFunc(0xfb87cf5e, sceNpLookupDestroyTransactionCtx);
	sceNp->AddFunc(0xfbc82301, sceNpScoreGetRankingByRange);
	sceNp->AddFunc(0xfcac355a, sceNpCommerceCreateCtx);
	sceNp->AddFunc(0xfd0eb5ae, sceNpSignalingDeactivateConnection);
	sceNp->AddFunc(0xfd39ae13, sceNpBasicGetFriendPresenceByNpId);
	sceNp->AddFunc(0xfe37a7f4, sceNpManagerGetNpId);
	sceNp->AddFunc(0xff0a2378, sceNpLookupUserProfileAsync);
	sceNp->AddFunc(0x432b3cbf, sceNpUtilBandwidthTestShutdown);
	sceNp->AddFunc(0xc2ced2b7, sceNpUtilBandwidthTestInitStart);
	sceNp->AddFunc(0xc880f37d, sceNpUtilBandwidthTestGetStatus);
	sceNp->AddFunc(0xc99ee313, sceNpUtilBandwidthTestAbort);
	sceNp->AddFunc(0xa8cf8451, sceNpSignalingDestroyCtx);
}
