#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNpCommon.h"

extern psv_log_base sceNpScore;

typedef u32 SceNpScoreBoardId;
typedef s64 SceNpScoreValue;
typedef u32 SceNpScoreRankNumber;
typedef s32 SceNpScorePcId;

struct SceNpScoreGameInfo
{
	u32 infoSize;
	u8 pad[4];
	u8 data[192];
};

struct SceNpScoreComment
{
	char utf8Comment[64];
};

struct SceNpScoreRankData
{
	SceNpId npId;
	u8 reserved[49];
	u8 pad0[3];
	SceNpScorePcId pcId;
	SceNpScoreRankNumber serialRank;
	SceNpScoreRankNumber rank;
	SceNpScoreRankNumber highestRank;
	s32 hasGameData;
	u8 pad1[4];
	SceNpScoreValue scoreValue;
	u64 recordDate;
};

struct SceNpScorePlayerRankData
{
	s32 hasData;
	u8 pad0[4];
	SceNpScoreRankData rankData;
};

struct SceNpScoreBoardInfo
{
	u32 rankLimit;
	u32 updateMode;
	u32 sortMode;
	u32 uploadNumLimit;
	u32 uploadSizeLimit;
};

struct SceNpScoreNpIdPcId
{
	SceNpId npId;
	SceNpScorePcId pcId;
	u8 pad[4];
};

s32 sceNpScoreInit(s32 threadPriority, s32 cpuAffinityMask, vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreTerm(ARMv7Context&)
{
	throw __FUNCTION__;
}

s32 sceNpScoreCreateTitleCtx(vm::psv::ptr<const SceNpCommunicationId> titleId, vm::psv::ptr<const SceNpCommunicationPassphrase> passphrase, vm::psv::ptr<const SceNpId> selfNpId)
{
	throw __FUNCTION__;
}

s32 sceNpScoreDeleteTitleCtx(s32 titleCtxId)
{
	throw __FUNCTION__;
}

s32 sceNpScoreCreateRequest(s32 titleCtxId)
{
	throw __FUNCTION__;
}

s32 sceNpScoreDeleteRequest(s32 reqId)
{
	throw __FUNCTION__;
}

s32 sceNpScoreAbortRequest(s32 reqId)
{
	throw __FUNCTION__;
}

s32 sceNpScoreSetTimeout(s32 id, s32 resolveRetry, s32 resolveTimeout, s32 connTimeout, s32 sendTimeout, s32 recvTimeout)
{
	throw __FUNCTION__;
}

s32 sceNpScoreSetPlayerCharacterId(s32 id, SceNpScorePcId pcId)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetBoardInfo(s32 reqId, SceNpScoreBoardId boardId, vm::psv::ptr<SceNpScoreBoardInfo> boardInfo, vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreRecordScore(
	s32 reqId,
	SceNpScoreBoardId boardId,
	SceNpScoreValue score,
	vm::psv::ptr<const SceNpScoreComment> scoreComment,
	vm::psv::ptr<const SceNpScoreGameInfo> gameInfo,
	vm::psv::ptr<SceNpScoreRankNumber> tmpRank,
	vm::psv::ptr<const u64> compareDate,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreRecordGameData(
	s32 reqId,
	SceNpScoreBoardId boardId,
	SceNpScoreValue score,
	u32 totalSize,
	u32 sendSize,
	vm::psv::ptr<const void> data,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetGameData(
	s32 reqId,
	SceNpScoreBoardId boardId,
	vm::psv::ptr<const SceNpId> npId,
	vm::psv::ptr<u32> totalSize,
	u32 recvSize,
	vm::psv::ptr<void> data,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetRankingByNpId(
	s32 reqId,
	SceNpScoreBoardId boardId,
	vm::psv::ptr<const SceNpId> npIdArray,
	u32 npIdArraySize,
	vm::psv::ptr<SceNpScorePlayerRankData> rankArray,
	u32 rankArraySize,
	vm::psv::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::psv::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::psv::ptr<u64> lastSortDate,
	vm::psv::ptr<SceNpScoreRankNumber> totalRecord,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetRankingByRange(
	s32 reqId,
	SceNpScoreBoardId boardId,
	SceNpScoreRankNumber startSerialRank,
	vm::psv::ptr<SceNpScoreRankData> rankArray,
	u32 rankArraySize,
	vm::psv::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::psv::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::psv::ptr<u64> lastSortDate,
	vm::psv::ptr<SceNpScoreRankNumber> totalRecord,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}


s32 sceNpScoreGetRankingByNpIdPcId(
	s32 reqId,
	SceNpScoreBoardId boardId,
	vm::psv::ptr<const SceNpScoreNpIdPcId> idArray,
	u32 idArraySize,
	vm::psv::ptr<SceNpScorePlayerRankData> rankArray,
	u32 rankArraySize,
	vm::psv::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::psv::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::psv::ptr<u64> lastSortDate,
	vm::psv::ptr<SceNpScoreRankNumber> totalRecord,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreCensorComment(s32 reqId, vm::psv::ptr<const char> comment, vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreSanitizeComment(s32 reqId, vm::psv::ptr<const char> comment, vm::psv::ptr<char> sanitizedComment, vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreWaitAsync(s32 id, vm::psv::ptr<s32> result)
{
	throw __FUNCTION__;
}

s32 sceNpScorePollAsync(s32 reqId, vm::psv::ptr<s32> result)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetBoardInfoAsync(s32 reqId, SceNpScoreBoardId boardId, vm::psv::ptr<SceNpScoreBoardInfo> boardInfo, vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreRecordScoreAsync(
	s32 reqId,
	SceNpScoreBoardId boardId,
	SceNpScoreValue score,
	vm::psv::ptr<const SceNpScoreComment> scoreComment,
	vm::psv::ptr<const SceNpScoreGameInfo> gameInfo,
	vm::psv::ptr<SceNpScoreRankNumber> tmpRank,
	vm::psv::ptr<const u64> compareDate,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreRecordGameDataAsync(
	s32 reqId,
	SceNpScoreBoardId boardId,
	SceNpScoreValue score,
	u32 totalSize,
	u32 sendSize,
	vm::psv::ptr<const void> data,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetGameDataAsync(
	s32 reqId,
	SceNpScoreBoardId boardId,
	vm::psv::ptr<const SceNpId> npId,
	vm::psv::ptr<u32> totalSize,
	u32 recvSize,
	vm::psv::ptr<void> data,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetRankingByNpIdAsync(
	s32 reqId,
	SceNpScoreBoardId boardId,
	vm::psv::ptr<const SceNpId> npIdArray,
	u32 npIdArraySize,
	vm::psv::ptr<SceNpScorePlayerRankData> rankArray,
	u32 rankArraySize,
	vm::psv::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::psv::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::psv::ptr<u64> lastSortDate,
	vm::psv::ptr<SceNpScoreRankNumber> totalRecord,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetRankingByRangeAsync(
	s32 reqId,
	SceNpScoreBoardId boardId,
	SceNpScoreRankNumber startSerialRank,
	vm::psv::ptr<SceNpScoreRankData> rankArray,
	u32 rankArraySize,
	vm::psv::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::psv::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::psv::ptr<u64> lastSortDate,
	vm::psv::ptr<SceNpScoreRankNumber> totalRecord,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreGetRankingByNpIdPcIdAsync(
	s32 reqId,
	SceNpScoreBoardId boardId,
	vm::psv::ptr<const SceNpScoreNpIdPcId> idArray,
	u32 idArraySize,
	vm::psv::ptr<SceNpScorePlayerRankData> rankArray,
	u32 rankArraySize,
	vm::psv::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::psv::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::psv::ptr<u64> lastSortDate,
	vm::psv::ptr<SceNpScoreRankNumber> totalRecord,
	vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreCensorCommentAsync(s32 reqId, vm::psv::ptr<const char> comment, vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpScoreSanitizeCommentAsync(s32 reqId, vm::psv::ptr<const char> comment, vm::psv::ptr<char> sanitizedComment, vm::psv::ptr<void> option)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func<name>(nid, &sceNpScore, #name, name)

psv_log_base sceNpScore("SceNpScore", []()
{
	sceNpScore.on_load = nullptr;
	sceNpScore.on_unload = nullptr;
	sceNpScore.on_stop = nullptr;

	REG_FUNC(0x0433069F, sceNpScoreInit);
	REG_FUNC(0x2050F98F, sceNpScoreTerm);
	REG_FUNC(0x5685F225, sceNpScoreCreateTitleCtx);
	REG_FUNC(0xD30D1993, sceNpScoreCreateRequest);
	REG_FUNC(0xF52EA88A, sceNpScoreDeleteTitleCtx);
	REG_FUNC(0xFFF24BB1, sceNpScoreDeleteRequest);
	REG_FUNC(0x320C0277, sceNpScoreRecordScore);
	REG_FUNC(0x24B09634, sceNpScoreRecordScoreAsync);
	REG_FUNC(0xC2862B67, sceNpScoreRecordGameData);
	REG_FUNC(0x40573917, sceNpScoreRecordGameDataAsync);
	REG_FUNC(0xDFAD64D3, sceNpScoreGetGameData);
	REG_FUNC(0xCE416993, sceNpScoreGetGameDataAsync);
	REG_FUNC(0x427D3412, sceNpScoreGetRankingByRange);
	REG_FUNC(0xC45E3FCD, sceNpScoreGetRankingByRangeAsync);
	REG_FUNC(0xBAE55B34, sceNpScoreGetRankingByNpId);
	REG_FUNC(0x45CD1D00, sceNpScoreGetRankingByNpIdAsync);
	REG_FUNC(0x871F28AA, sceNpScoreGetRankingByNpIdPcId);
	REG_FUNC(0xCE3A9544, sceNpScoreGetRankingByNpIdPcIdAsync);
	REG_FUNC(0xA7E93CE1, sceNpScoreAbortRequest);
	REG_FUNC(0x31733BF3, sceNpScoreWaitAsync);
	REG_FUNC(0x9F2A7AC9, sceNpScorePollAsync);
	REG_FUNC(0x00F90E7B, sceNpScoreGetBoardInfo);
	REG_FUNC(0x3CD9974E, sceNpScoreGetBoardInfoAsync);
	REG_FUNC(0xA0C94D46, sceNpScoreCensorComment);
	REG_FUNC(0xAA0BBF8E, sceNpScoreCensorCommentAsync);
	REG_FUNC(0x6FD2041A, sceNpScoreSanitizeComment);
	REG_FUNC(0x15981858, sceNpScoreSanitizeCommentAsync);
	REG_FUNC(0x5EF44841, sceNpScoreSetTimeout);
	REG_FUNC(0x53D77883, sceNpScoreSetPlayerCharacterId);
});
